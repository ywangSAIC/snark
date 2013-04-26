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
// 3. All advertising materials mentioning features or use of this software
//    must display the following acknowledgement:
//    This product includes software developed by the The University of Sydney.
// 4. Neither the name of the The University of Sydney nor the
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


/// @author Cedric Wohlleber

#include <Eigen/Core>
#include <snark/graphics/qt3d/rotation_matrix.h>
#include "./ModelReader.h"
#include "./Texture.h"

namespace snark { namespace graphics { namespace View {

/// constructor
/// @param viewer reference to the viewer
/// @param options csv options for the position input
/// @param file model filename
/// @param flip flip model around the x-axis
/// @param c color used for the label
/// @param label text displayed as label
ModelReader::ModelReader( QGLView& viewer
                        , comma::csv::options& options
                        , const std::string& file
                        , bool flip
                        , double scale
                        , snark::graphics::View::coloured* c
                        , const std::string& label )
    : Reader( viewer, options, 1, c, 1, label, QVector3D( 0, 1, 1 ) ) // TODO make offset configurable ?
    , m_file( file )
    , m_flip( flip )
    , scale_( scale )
    , coloured_( c )
{
}

void ModelReader::start()
{
    if( m_file.substr( m_file.size() - 3, 3 ) == "ply" )
    {
        boost::optional< QColor4ub > color;
        if( dynamic_cast< const Fixed* >( coloured_ ) ) { color = coloured_->color( Eigen::Vector3d( 0, 0, 0 ), 0, 0, QColor4ub() ); } // quick and dirty
        m_plyLoader = PlyLoader( m_file, color, scale_ );
    }
    if( !m_plyLoader )
    {
        if( !comma::math::equal( scale_, 1.0 ) ) { std::cerr << "view-points: warning: --scale supported only for ply models; others: todo" << std::endl; }
        m_scene = QGLAbstractScene::loadScene( QLatin1String( m_file.c_str() ) );
    }
    m_thread.reset( new boost::thread( boost::bind( &Reader::read, boost::ref( *this ) ) ) );
}

void ModelReader::update( const Eigen::Vector3d& offset )
{
    updatePoint( offset );
}

bool ModelReader::empty() const
{
    return !m_point;
}

const Eigen::Vector3d& ModelReader::somePoint() const
{
    boost::mutex::scoped_lock lock( m_mutex );
    return *m_point;
}

void ModelReader::render( QGLPainter* painter )
{
    painter->modelViewMatrix().push();
    painter->modelViewMatrix().translate( m_translation );
    painter->modelViewMatrix().rotate( m_quaternion );
    if( m_flip )
    {
        painter->modelViewMatrix().rotate( 180, 1, 0, 0 );
    }
    if( m_plyLoader )
    {
        m_plyLoader->draw( painter );
    }
    else
    {
        QGLSceneNode* node = m_scene->mainNode();
    //     painter->setStandardEffect( QGL::LitMaterial ); // no effect ?
        node->draw(painter);
    }
    painter->modelViewMatrix().pop();
    if( !m_label.empty() )
    {
        drawLabel( painter, m_translation );
    }
}

bool ModelReader::readOnce()
{
    if( !m_stream ) // quick and dirty: handle named pipes
    {
        if( !m_istream() ) { return true; }
        m_stream.reset( new comma::csv::input_stream< PointWithId >( *m_istream(), options ) );
    }
    const PointWithId* p = m_stream->read();
    if( p == NULL ) { m_shutdown = true; return false; }
    boost::mutex::scoped_lock lock( m_mutex );
    m_point = p->point;
    m_orientation = p->orientation;
    m_color = m_colored->color( p->point, p->id, p->scalar, p->color );
    return true;
}


} } } // namespace snark { namespace graphics { namespace View {
