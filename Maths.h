#pragma once

#include "Vector.h"


// --------------
class Maths
{
public:
    inline static const double Pi{ 3.1415927 };
    inline static const double Pi2{ Pi * 2 };
    inline static const double PiHalf{ Pi / 2 };
    inline static const double PiQuarter{ Pi / 4 };

public:
    static bool Collision( const Vector & _circleA, const double _radiusA, const Vector & _circleB, const double _radiusB );
    static bool Collision( const Vector & _circle, const double _radius, const Vector & _segmentA, const Vector & _segmentB );
    static double NormalizeAngle( const double _angle );
    static double Random( const double _min, const double _max );
    static void Increase( double & _value, const double _increaser, const double _max );
    static void Decrease( double & _value, const double _reducer );
};
