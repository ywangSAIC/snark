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

/// @author vsevolod vlaskine

#include <fstream>
#include <sstream>
#include <boost/algorithm/string/regex.hpp>
#include <comma/name_value/serialize.h>
#include "load.h"
#include "traits.h"

namespace snark { namespace terrain { namespace dem { namespace srtm {

void load( header& h, const std::string& filename )
{
    std::ifstream ifs( &filename[0] );
    if( !ifs.is_open() ) { COMMA_THROW( comma::exception, "failed to open '" << filename << "'" ); }
    std::string s;
    while( !ifs.eof() && ifs.good() ) // quick and dirty, not terribly efficient
    {
        std::string line;
        std::getline( ifs, line, '\r' );
        if( line.empty() ) { continue; }
        s += boost::replace_regex_copy( line, boost::regex( "  *" ), std::string( "=" ), boost::format_sed ) + '\n';
    }
    std::istringstream iss( s );
    h = comma::read_path_value< header >( iss ); // debug: comma::write_json( h, std::cerr );
}

header load_header( const std::string& filename )
{
    header h;
    load( h, filename );
    return h;
}

} } } } // namespace snark { namespace terrain { namespace dem { namespace srtm {
