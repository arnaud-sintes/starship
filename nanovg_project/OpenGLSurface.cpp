#include "OpenGLSurface.h"

#include <windows.h>
#include <gl/GL.h>

// opengl library:
#pragma comment( lib, "opengl32.lib" )


// ----------------

OpenGLSurface::OpenGLSurface( const Win32::Windows & _windows )
    : m_dc{ _windows.GetDeviceContext() }
    , m_rc{ ::wglCreateContext( m_dc.As< ::HDC >() ), false }
{}


OpenGLSurface::~OpenGLSurface()
{
    ::wglDeleteContext( m_rc.As< ::HGLRC >() );
}


OpenGLSurface::Context OpenGLSurface::MakeCurrent() const
{
    return { m_dc, m_rc };
}


// ----------------
OpenGLSurface::Context::Context( const Win32::Handle & _dc, const Win32::Handle & _rc )
    : m_dc{ _dc }
{
    ::wglMakeCurrent( m_dc.As< ::HDC >(), _rc.As< ::HGLRC >() );
}


OpenGLSurface::Context::~Context()
{
    ::glFlush();
	::SwapBuffers( m_dc.As< ::HDC >() );
	::wglMakeCurrent( nullptr, nullptr );
}


void OpenGLSurface::Context::Viewport( const Dimension_ui & _dimension ) const
{
    ::glViewport( 0, 0, _dimension.width, _dimension.height );
}
    

void OpenGLSurface::Context::Clear( const Color_d & _color ) const
{
    const auto color{ _color.ToType< float >() };
    ::glClearColor( color.r, color.g, color.b, 1 );
    ::glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );
}