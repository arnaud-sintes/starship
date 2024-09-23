#pragma once

#include "core/Win32.h"


// ----------------
class OpenGL
{
public:
    OpenGL( const Win32::Windows & _windows );
    ~OpenGL();

public:
    class Context;
    Context MakeCurrent() const;

private:
    const Win32::Handle & m_dc;
    Win32::Handle m_rc;
};


// ----------------
class OpenGL::Context
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