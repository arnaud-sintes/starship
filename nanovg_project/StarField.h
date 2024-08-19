#pragma once

#include "NanoVGRenderer.h"


// --------------
class StarField
{
public:
    StarField( const Dimension_ui & _dimension );

public:
    void Draw( NanoVGRenderer::Frame & _frame, const Vector & _speed );

private:
    struct Layer;
    struct Star;

private:
    const Dimension_ui m_dimension;
    std::random_device m_rndDev;
    std::mt19937 m_rnd;
    const size_t m_maxDimension;
    std::uniform_int_distribution< std::mt19937::result_type > m_rndPos;
    std::list< Star > m_field;
};


// --------------
struct StarField::Layer
{
    const int density;
    const double speedMin, speedMax;
    const double colorMin, colorMax;
    const double radiusMin, radiusMax;
};


// --------------
struct StarField::Star
{
    double x, y;
    const double speed;
    const double c;
    const double radius;
};