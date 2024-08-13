#pragma once

#include "Graphics.h"


// --------------
class StarField
{
public:
    StarField( const size_t _windowWidth, const size_t _windowHeight );

public:
    void Draw( cairo_t & _cairo, const Vector & _speed );

private:
    struct Layer;
    struct Star;

private:
    const size_t m_windowWidth;
    const size_t m_windowHeight;
    std::random_device m_rndDev;
    std::mt19937 m_rnd;
    const size_t m_dimension;
    std::uniform_int_distribution< std::mt19937::result_type > m_rndPos;
    std::vector< Star > m_field;
};


// --------------
struct StarField::Layer
{
    int density;
    double speedMin, speedMax;
    double colorMin, colorMax;
    double radiusMin, radiusMax;
};


// --------------
struct StarField::Star
{
    double x, y;
    double speed;
    double c;
    double radius;
};