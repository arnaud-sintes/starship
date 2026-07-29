#include "OpenGL.h"

#include <windows.h>
#include <gl/GL.h>

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


void OpenGL::Context::SetSwapInterval( const int _interval ) const
{
    using PFNWGLSWAPINTERVALEXTPROC = BOOL( WINAPI * )( int );
    const auto wglSwapIntervalEXT{ reinterpret_cast< PFNWGLSWAPINTERVALEXTPROC >( ::wglGetProcAddress( "wglSwapIntervalEXT" ) ) };
    if( wglSwapIntervalEXT != nullptr )
        wglSwapIntervalEXT( _interval );
}