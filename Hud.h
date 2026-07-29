#pragma once

#include "NanoVGRenderer.h"
#include "World.h"


// --------------
class Hud
{
public:
    Hud( const Dimension_ui & _screenDimension );

public:
    void Draw( const World & _world, const NanoVGRenderer::Frame & _frame );

private:
    void _Bar( const NanoVGRenderer::Frame & _frame, const Vector & _position, const double _width, const double _height, const double _rate, const Color_d & _color ) const;
    Color_d _StateColor( const double _rate, const Color_d & _healthy ) const;

private:
    const Dimension_ui m_screenDimension;
    double m_pulse{ 0 }; // critical-state blink animation
};
