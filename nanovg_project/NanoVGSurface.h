#pragma once

#include "OpenGLSurface.h"


// ----------------
class NanoVGSurface
{
public:
    NanoVGSurface( const Win32::Windows & _windows );
    ~NanoVGSurface();

public:
    class Frame;
    Frame CreateFrame( const Dimension_ui & _dimension, const Color_d & _color ) const;
    bool CreateFont( const std::string & _name, const std::string & _path ) const;

private:
    const OpenGLSurface m_ogl;
    void * m_context;
};


// ----------------
class NanoVGSurface::Frame
{
public:
    Frame( const Dimension_ui & _dimension, const Color_d & _color, const OpenGLSurface & _ogl, void * _context );
    ~Frame();

public:
    void Line( const Position_d & _a, const Position_d & _b, const Color_d & _color, const double _strokeWidth ) const;
    void FillCircle( const Position_d & _position, const double _radius, const Color_d & _color ) const;
    void StrokeCircle( const Position_d & _position, const double _radius, const Color_d & _color, const double _strokeWidth ) const;
    void FillArc( const Position_d & _position, const double _radius, const double _angleA, const double _angleB, const Color_d & _color, const bool _clockWise = true ) const;
    void StrokeArc( const Position_d & _position, const double _radius, const double _angleA, const double _angleB, const Color_d & _color, const double _strokeWidth, const bool _clockWise = true ) const;
    void Text( const Position_d & _position, const std::string & _fontName, const double _size, const std::string & _text, const Color_d & _color );

private:
    const OpenGLSurface::Context m_glContext;
    void * m_context;
};
