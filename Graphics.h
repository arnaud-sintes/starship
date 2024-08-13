#pragma once

#include "Maths.h"

#include "cairo.h"


// --------------
class Graphics
{
public:
    // --------------
    struct Color
    {
        const double r, g, b;
    };

    inline static const Color colorWhite{ 1, 1, 1 };

    // --------------
    struct Arc
    {
        const double a, b;
    };

    inline static const Arc arcFull{ 0, Maths::Pi2 };

public:
    static Color FireColor( double _ratio );
    static void Stroke( cairo_t & _cairo, const Vector & _position, const double _radius, const Arc & _arc, const Color & _color, const double _strokeWidth );
    static void Fill( cairo_t & _cairo, const Vector & _position, const double _radius, const Arc & _arc, const Color & _color );
    static void Line( cairo_t & _cairo, const Vector & _a, const Vector & _b, const Color & _color, const double _strokeWidth, const bool _dash );
    static void Text( cairo_t & _cairo, const Vector & _position, const double _size, const Color & _color, const std::string & _text );
};