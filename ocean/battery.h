#ifndef SNARK_OCEAN_BATTERY_H
#define SNARK_OCEAN_BATTERY_H
#include <comma/base/types.h>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/date_time/posix_time/posix_time_duration.hpp>
#include <boost/array.hpp>
#include <vector>
#include <iomanip>
#include <iostream>
#include "hex_value.h"
#include "units.h"
#include "commands.h"

namespace snark { namespace ocean {

struct address {
//    static const ocean8 temperature = 0x08;
//    static const ocean8 voltage =     0x09;
//    static const ocean8 current = 0x0a;
//    static const ocean8 avg_current = 0x0b;
//    static const ocean8 rel_state_of_charge = 0x0d;
//    static const ocean8 remaining_capacity = 0x0f;
//    static const ocean8 run_time_to_empty = 0x011;
//    static const ocean8 status = 0x016;
     enum { temperature = 0x08, voltage = 0x09, current = 0x0a, avg_current=0x0b, rel_state_of_charge=0x0d, remaining_capacity=0x0f,
            run_time_to_empty=0x11, status=0x16
     };
    
};
struct battery_state {
        enum { charging=0x00, fully_discharged=0x10, fully_charged=0x20, discharging=0x40, initialised=0x80, uninitialised };
};

struct battery_t
{

    static std::string state_to_string( int st ) {
        switch( st )
        {
            case battery_state::initialised:
                return "IN";
                break;
            case battery_state::uninitialised:
                return "UN";
                break;
            case battery_state::fully_discharged:
                return "FD";
                break;
            case battery_state::fully_charged:
                return "FC";
                break;
            case battery_state::discharging:
                return "DC";
                break;
            default:
                return "CH";
                break;

        }
    }

    uint8 id;
    voltage_t voltage;
    current_t current;
    current_t avg_current;
    temperature_t temperature;  
    power_t remaining_capacity;
    double chargePc; // Charge percentage
    boost::posix_time::time_duration time_to_empty;
    int state;
    
    // void operator&( const data_t& data );
    
    template < int P >
    void operator&( const hex_data_t< P >& line_data )
    {
        for( typename boost::array< data_t, P >::const_iterator it=line_data.values.begin(); it!=line_data.values.end(); ++it ) { *this & ( *it ); } 
    }

    battery_t() : id(0), chargePc(-999), state( battery_state::uninitialised ) {}
    battery_t( uint8 id_ ) : id( id_ ), chargePc(-999), state( battery_state::uninitialised ) {}


void operator&(const data_t& data)
{
    // std::cerr << " address " << data.address() << std::endl;
    switch( data.address() )
    {
        case address::temperature:
        {
            static const double unit = 0.1; // Kelvin
            temperature = data.value() * unit * kelvin; // 0.1k unit
            break;
        }
        case address::voltage:
        {
            voltage = data.value() / 1000.0 * volt; // millivolts to volts
            break;
        }
        case address::current:
        {
            current = data.value.cast() / 1000.0 * ampere; //mAmp to Amps
            // std::cerr << "got current: " << current.value() << std::endl;
            break;
        }
        case address::avg_current:
        {
            avg_current = data.value.cast() / 1000.0 * ampere; //mAmp to Amps
            break;
        }
        case address::remaining_capacity:
        {
            remaining_capacity = data.value.cast() / 100.0 * watt; // unit is 10mWh
        }
        case address::rel_state_of_charge:
        {
            chargePc = data.value();    // percentage, unit is %
            break;
        }
        case address::run_time_to_empty:
        {
            time_to_empty = boost::posix_time::minutes( data.value() );
        }
        case address::status:
        {
            if( !(data.value() &  battery_state::initialised) ) 
            {
                state = battery_state::uninitialised;
                return;
            }
            comma::uint16 val = data.value() & 0x0070;  // masks out everything including 'initialised' flag
            switch( val )
            {
                case battery_state::discharging:
                    state = battery_state::discharging;
                    break;
                case battery_state::fully_charged:
                    state = battery_state::fully_charged;
                    break;
                case battery_state::fully_discharged:
                    state = battery_state::fully_discharged;
                    break;
                default:
                    state = battery_state::charging;
                    break;
            }
            // std::cerr << "battery: " << int(id) <<  " state: " << state << " value: " << data.value() << " val: " << val << std::endl;
            break;
        }
        default:
        {
            return;
        }
    }
}
};

// Removes checksum wrappers, TODO throws exception on incorrect checksum
std::string& strip( std::string& line )
{
    /// '$B15,....,FF00%B2' becomes B15,....,FF00
    //std::size_t pos = ( line[ line.size() - 3 ] == '%' ? line.size()-4 : std::string::npos );
    std::size_t pos = line.find_first_of( '%', line.size() - 4 );
    if( pos != std::string::npos ) { --pos; }
    //std::cerr << "char: " << line[ line.size() - 3 ] << std::endl;
    line = line.substr( 1, pos);
    return line;
}


template < int N >
struct controller_t
{
    // struct state_t {
    //     enum { unknown=-1, AC, FC, FD, NG };
    // };
    
    uint8 id;
    int state;
    boost::array< battery_t, N > batteries;
    typedef typename boost::array< battery_t, N >::const_iterator const_iter;
    
    
    power_t total_power;
    current_t total_current;
    voltage_t avg_voltage;
    double avgCharge; // percentage


    controller_t() : id(0), state( battery_state::uninitialised ), avgCharge(-999) { set_battery_id(); }
    controller_t( uint8 id_ ) : id( id_ ), state( battery_state::uninitialised ), avgCharge(-999) { set_battery_id(); }

    void set_battery_id()
    {
        for( std::size_t i=0; i<=N; ++i ) { batteries[i].id = i + 1; }
    }
    
    void operator&( const data_t& data );
    
    /// Populate the controller and controller's batteries with hex data ( pushed data from controller )
    template < int P >
    void operator&( const hex_data_t< P >& line_data )
    {
        // std::cerr << "controller update: " << int( line_data.controller_id ) << " - Battery: " <<
        //     int( line_data.battery_id ) << std::endl;

        if( line_data.controller_id != id ) { 
            return; 
        }
        if( line_data.battery_id > N ) 
        {
            std::cerr << "battery " << line_data.battery_id  << "is not on controller " << int(id) << std::endl;
            return;
        }

        batteries[ line_data.battery_id - 1 ] & line_data;
        
//         for( typename hex_data_t< P >::value_iter it=line_data.begin(); it!=line_data.end(); ++it ) { *this & ( *it ); } 
    }
    /// Get consolidated values from the batteries, e.g. total power or total current
    void consolidate()
    {
        total_current = 0*ampere;
        total_power = 0*watt;
        avg_voltage = 0*volt;
        avgCharge = 0;
        for( const_iter it=batteries.begin(); it!=batteries.end(); ++it ) 
        { 
            total_current += it->avg_current;
            total_power += it->remaining_capacity;
            avg_voltage += it->voltage;
            avgCharge += it->chargePc;
        } 
        //avg_voltage = ( avg_voltage.value()/N ) * volt;
        avg_voltage /= N;
        avgCharge /= N;
        state = batteries.front().state;
    }
};

} } // namespace snark { namespace ocean {

#endif // SNARK_OCEAN_BATTERY_H