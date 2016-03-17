// This file is part of snark, a generic and flexible library for robotics research
// Copyright (c) 2011 The University of Sydney
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
// 3. Neither the name of the University of Sydney nor the
//    names of its contributors may be used to endorse or promote products
//    derived from this software without specific prior written permission.
//
// NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
// GRANTED BY THIS LICENSE.  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
// HOLDERS AND CONTRIBUTORS \"AS IS\" AND ANY EXPRESS OR IMPLIED
// WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
// IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

/// @author dave jennings

#include <cmath>
#include <comma/application/verbose.h>
#include <comma/csv/options.h>
#include <comma/csv/stream.h>
#include "../../math/applications/frame.h"

typedef std::pair< Eigen::Vector3d, Eigen::Vector3d > point_pair_t;

struct input_points
{
    point_pair_t points;
    comma::uint32 block;

    input_points() : block( 0 ) {}
};

static const std::string default_fields = comma::join( comma::csv::names< point_pair_t >(), ',' );
static const std::string standard_output_fields = comma::join( comma::csv::names< snark::applications::position >(), ',' );

static void usage( bool verbose = false )
{
    std::cerr << std::endl;
    std::cerr << "give the transform that will align a point cloud with a reference point cloud" << std::endl;
    std::cerr << std::endl;
    std::cerr << "usage: " << comma::verbose.app_name() << " [<options>]" << std::endl;
    std::cerr << std::endl;
    std::cerr << "default input fields: " << default_fields << std::endl;
    std::cerr << std::endl;
    std::cerr << "The input data is assumed to consist of matched points from the reference" << std::endl;
    std::cerr << "point cloud and the point cloud to be aligned. The output is a transform" << std::endl;
    std::cerr << "in the form " << standard_output_fields << " (angles in radians) that can be given to" << std::endl;
    std::cerr << "points-frame to align the data." << std::endl;
    std::cerr << std::endl;
    if( verbose )
    {
        std::cerr << "csv stream options: " << std::endl;
        std::cerr << comma::csv::options::usage() << std::endl;
        std::cerr << std::endl;
    }
    std::cerr << "options: " << std::endl;
    std::cerr << "    --help,-h; show this help, --help --verbose for more help" << std::endl;
    std::cerr << "    --verbose,-v; more output" << std::endl;
    std::cerr << "    --output-fields: show output fields and exit" << std::endl;
    std::cerr << "    --output-format: show output format and exit" << std::endl;
    std::cerr << "    --output-error: include the error estimate in output" << std::endl;
    std::cerr << std::endl;
    std::cerr << "examples: " << std::endl;
    std::cerr << "    -- output the " << standard_output_fields << " transform --" << std::endl;
    std::cerr << "    cat combined-points.csv | points-align" << std::endl;
    std::cerr << std::endl;
    std::cerr << "    -- align the point cloud to the reference point cloud --" << std::endl;
    std::cerr << "    frame=$( cat combined-points.csv | points-align )" << std::endl;
    std::cerr << "    cat points.csv | points-frame --from $frame --fields=x,y,z" << std::endl;
    std::cerr << std::endl;
    exit( 0 );
}

struct position_with_error
{
    comma::uint32 block;
    snark::applications::position position;
    double error;

    position_with_error() : block( 0 ), error( 0 ) {}
    position_with_error( comma::uint32 block, const snark::applications::position& position, double error )
        : block( block )
        , position( position )
        , error( error )
    {}
};

namespace comma { namespace visiting {

template <> struct traits< input_points >
{
    template < typename K, typename V > static void visit( const K&, input_points& p, V& v )
    {
        v.apply( "block", p.block );
        v.apply( "first", p.points.first );
        v.apply( "second", p.points.second );
    }

    template < typename K, typename V > static void visit( const K&, const input_points& p, V& v )
    {
        v.apply( "block", p.block );
        v.apply( "first", p.points.first );
        v.apply( "second", p.points.second );
    }
};

template <> struct traits< position_with_error >
{
    template < typename Key, class Visitor >
    static void visit( const Key&, position_with_error& p, Visitor& v )
    {
        v.apply( "block", p.block );
        v.apply( "x", p.position.coordinates.x() );
        v.apply( "y", p.position.coordinates.y() );
        v.apply( "z", p.position.coordinates.z() );
        v.apply( "roll", p.position.orientation.x() );
        v.apply( "pitch", p.position.orientation.y() );
        v.apply( "yaw", p.position.orientation.z() );
        v.apply( "error", p.error );
    }

    template < typename Key, class Visitor >
    static void visit( const Key&, const position_with_error& p, Visitor& v )
    {
        v.apply( "block", p.block );
        v.apply( "x", p.position.coordinates.x() );
        v.apply( "y", p.position.coordinates.y() );
        v.apply( "z", p.position.coordinates.z() );
        v.apply( "roll", p.position.orientation.x() );
        v.apply( "pitch", p.position.orientation.y() );
        v.apply( "yaw", p.position.orientation.z() );
        v.apply( "error", p.error );
    }
};

} } // namespace comma { namespace visiting {

std::string get_output_fields( comma::csv::options csv_options, bool output_error )
{
    std::string output_fields( standard_output_fields );
    if( output_error ) { output_fields += ",error"; }
    if( csv_options.has_field( "block" )) { output_fields = output_fields + ",block"; }
    return output_fields;
}

std::string get_output_format( comma::csv::options csv_options, bool output_error )
{
    std::string output_format( comma::csv::format::value< snark::applications::position >() );
    if( output_error ) { output_format += ",d"; }
    if( csv_options.has_field( "block" )) { output_format = output_format + ",ui"; }
    return output_format;
} 

void output_transform( Eigen::MatrixXd source, Eigen::MatrixXd target
                     , comma::uint32 block, comma::csv::options output_csv )
{
    comma::verbose << "Loaded " << target.cols() << " pairs of points" << std::endl;

    Eigen::Matrix4d estimate = Eigen::umeyama( source, target );

    comma::verbose << "umeyama estimate\n" << estimate << std::endl;

    Eigen::Matrix3d rotation( estimate.block< 3, 3 >( 0, 0 ));
    Eigen::Vector3d translation( estimate.block< 3, 1 >( 0, 3 ));

    Eigen::Vector3d orientation( snark::rotation_matrix::roll_pitch_yaw( rotation ));

    comma::verbose << "rotation matrix\n" << rotation << std::endl;
    comma::verbose << "translation ( " << comma::join( translation, ',' ) << " )" << std::endl;
    comma::verbose << "orientation (rad) ( " << comma::join( orientation, ',' ) << " )" << std::endl;
    comma::verbose << "orientation (deg) ( " << comma::join( orientation * 180 / M_PI, ',' ) << " )" << std::endl;

    // Convert source and target to a form that allows multiplication by estimate.
    // Change each vector from size 3 to size 4 and store 1 in the fourth row.
    source.conservativeResize( source.rows()+1, Eigen::NoChange );
    source.row( source.rows()-1 ) = Eigen::MatrixXd::Constant( 1, source.cols(), 1 );

    target.conservativeResize( target.rows()+1, Eigen::NoChange );
    target.row( target.rows()-1 ) = Eigen::MatrixXd::Constant( 1, target.cols(), 1 );

    double error = ( estimate * source - target ).norm() / target.norm();

    comma::csv::output_stream< position_with_error > ostream( std::cout, output_csv );
    ostream.write( position_with_error( block
                                      , snark::applications::position( translation, orientation )
                                      , error ));
}

int main( int ac, char** av )
{
    try
    {
        comma::command_line_options options( ac, av, usage );
        comma::csv::options csv( options );
        csv.full_xpath = true;
        if( csv.fields.empty() ) { csv.fields = default_fields; }
        comma::verbose << "csv.fields=" << csv.fields << std::endl;
        bool output_error = options.exists( "--output-error" );

        std::string output_fields = get_output_fields( csv, output_error );
        std::string output_format = get_output_format( csv, output_error );

        if( options.exists( "--output-fields" ))
        {
            std::cout << output_fields << std::endl;
            return 0;
        }

        if( options.exists( "--output-format" ))
        {
            std::cout << output_format << std::endl;
            return 0;
        }

        comma::csv::options output_csv;
        output_csv.fields = output_fields;
        comma::verbose << "output fields=" << output_fields << std::endl;
        if( csv.binary() ) { output_csv.format( output_format ); }

        Eigen::MatrixXd source( 3, 0 );
        Eigen::MatrixXd target( 3, 0 );
        unsigned int block = 0;

        comma::csv::input_stream< input_points > istream( std::cin, csv );
        while( istream.ready() || ( std::cin.good() && !std::cin.eof() ) )
        {
            const input_points* p = istream.read();
            if( !p ) break;

            comma::verbose << "block " << p->block << std::endl;

            if( p->block != block )
            {
                output_transform( source, target, block, output_csv );
                source.resize( Eigen::NoChange, 0 );
                target.resize( Eigen::NoChange, 0 );
                block = p->block;
            }

            target.conservativeResize( Eigen::NoChange, target.cols()+1 );
            source.conservativeResize( Eigen::NoChange, source.cols()+1 );
            
            target(0, target.cols()-1) = p->points.first(0);
            target(1, target.cols()-1) = p->points.first(1);
            target(2, target.cols()-1) = p->points.first(2);

            source(0, source.cols()-1) = p->points.second(0);
            source(1, source.cols()-1) = p->points.second(1);
            source(2, source.cols()-1) = p->points.second(2);
        }
        output_transform( source, target, block, output_csv );
    }
    catch( std::exception& ex )
    {
        std::cerr << comma::verbose.app_name() << ": " << ex.what() << std::endl;
    }
    catch( ... )
    {
        std::cerr << comma::verbose.app_name() << ": unknown exception" << std::endl;
    }
    return 0;
}
