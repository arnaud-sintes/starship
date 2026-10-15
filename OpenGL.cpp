#include "OpenGL.h"

#include <windows.h>
#include <gl/GL.h>

#include <fstream>

// opengl library:
#pragma comment( lib, "opengl32.lib" )


// ----------------

OpenGL::OpenGL( const Win32::Windows & _windows )
    : m_dc{ _windows.GetDeviceContext() }
    , m_rc{ ::wglCreateContext( m_dc.As< ::HDC >() ), false }
{}


OpenGL::~OpenGL()
{
    ::wglDeleteContext( m_rc.As< ::HGLRC >() );
}


OpenGL::Context OpenGL::MakeCurrent() const
{
    return { m_dc, m_rc };
}


// ----------------
OpenGL::Context::Context( const Win32::Handle & _dc, const Win32::Handle & _rc )
    : m_dc{ _dc }
{
    ::wglMakeCurrent( m_dc.As< ::HDC >(), _rc.As< ::HGLRC >() );
}


OpenGL::Context::~Context()
{
    ::glFlush();
	::SwapBuffers( m_dc.As< ::HDC >() );
	::wglMakeCurrent( nullptr, nullptr );
}


void OpenGL::Context::Viewport( const View & _view ) const
{
    ::glViewport( 0, 0, _view.physical.width, _view.physical.height );
}


void OpenGL::Context::Clear( const Color_d & _color ) const
{
    const auto color{ _color.ToType< float >() };
    ::glClearColor( color.r, color.g, color.b, 1 );
    ::glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );
}


void OpenGL::Context::SaveScreenshot( const std::string & _path ) const
{
    GLint viewport[ 4 ]{};
    ::glGetIntegerv( GL_VIEWPORT, viewport );
    const auto width{ viewport[ 2 ] }, height{ viewport[ 3 ] };
    const auto rowBytes{ ( width * 3 + 3 ) & ~3 }; // rows are padded to 4 bytes, as the default GL pack alignment does
    std::vector< unsigned char > pixels( static_cast< size_t >( rowBytes ) * static_cast< size_t >( height ) );
    ::glReadBuffer( GL_BACK );
    ::glReadPixels( 0, 0, width, height, 0x80E0 /* GL_BGR_EXT */, GL_UNSIGNED_BYTE, pixels.data() ); // bottom-up, like a BMP
    const auto put32{ []( std::string & _out, const unsigned int _value ) { for( int i{ 0 }; i < 4; i++ ) _out.push_back( static_cast< char >( ( _value >> ( 8 * i ) ) & 0xFF ) ); } };
    std::string header{ "BM" };
    put32( header, static_cast< unsigned int >( 54 + pixels.size() ) );
    put32( header, 0 );
    put32( header, 54 );
    put32( header, 40 );
    put32( header, static_cast< unsigned int >( width ) );
    put32( header, static_cast< unsigned int >( height ) );
    header.push_back( 1 ); header.push_back( 0 ); // planes
    header.push_back( 24 ); header.push_back( 0 ); // bits per pixel
    for( int i{ 0 }; i < 6; i++ )
        put32( header, 0 ); // no compression, the other fields are free
    std::ofstream file{ _path, std::ios::binary };
    file.write( header.data(), static_cast< std::streamsize >( header.size() ) );
    file.write( reinterpret_cast< const char * >( pixels.data() ), static_cast< std::streamsize >( pixels.size() ) );
}


void OpenGL::Context::SetSwapInterval( const int _interval ) const
{
    using PFNWGLSWAPINTERVALEXTPROC = BOOL( WINAPI * )( int );
    const auto wglSwapIntervalEXT{ reinterpret_cast< PFNWGLSWAPINTERVALEXTPROC >( ::wglGetProcAddress( "wglSwapIntervalEXT" ) ) };
    if( wglSwapIntervalEXT != nullptr )
        wglSwapIntervalEXT( _interval );
}