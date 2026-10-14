#pragma once

#include "NanoVGRenderer.h"


// --------------
class StarField
{
public:
    StarField( const Dimension_ui & _dimension );

public:
    void Draw( const NanoVGRenderer::Frame & _frame, const Vector & _speed );
    void SetDensity( const double _density ) { m_targetDensity = _density; } // [ 0, 1 ], reached progressively

private:
    struct Layer;
    struct Star;

private:
    const Dimension_ui m_dimension;
    std::random_device m_rndDev;
    std::mt19937 m_rnd;
    const size_t m_maxDimension;
    std::uniform_int_distribution< std::mt19937::result_type > m_rndPos;
    std::vector< Star > m_field;
    double m_density{ 1 };       // share of the field currently drawn
    double m_targetDensity{ 1 };
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
    double speed;
    double c;
    double radius;
    double rank; // [ 0, 1 ) fixed: drawn while below the field density
};
