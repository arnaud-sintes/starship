#pragma once

#include "Win32.h"


// ----------------
class OpenGLSurface
{
public:
    OpenGLSurface( const Win32::Windows & _windows );
    ~OpenGLSurface();

public:
    class Context;
    Context MakeCurrent() const;

private:
    const Win32::Handle & m_dc;
    Win32::Handle m_rc;
};


// ----------------
class OpenGLSurface::Context
{
public:
    Context( const Win32::Handle & _dc, const Win32::Handle & _rc );
    ~Context();

public:
    void Viewport( const Dimension_ui & _dimension ) const;
    void Clear( const Color_d & _color ) const;

private:
    const Win32::Handle & m_dc;
};