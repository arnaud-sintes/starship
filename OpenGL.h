#pragma once

#include "core/Win32.h"
#include "View.h"


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
    void Viewport( const View & _view ) const;
    void Clear( const Color_d & _color ) const;
    void SetSwapInterval( const int _interval ) const; // 0 disables vsync (frame pacing is handled by the Timer)

private:
    const Win32::Handle & m_dc;
};