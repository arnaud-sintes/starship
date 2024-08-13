#pragma once

#include "std.h"


// --------------
class Vector
{
public:
    double u{ 0 }, v{ 0 };

public:
    static Vector From( const double _orientation, const double _distance );
    double DotProd( const Vector & _other ) const;
    double CrossProd( const Vector & _other ) const;
    double Distance() const;
    Vector & Normalize();
    Vector Normalized() const;
    double Orientation() const;
    Vector & operator += ( const Vector & _other );
    Vector operator + ( const Vector & _other ) const;
    Vector & operator -= ( const Vector & _other );
    Vector operator - ( const Vector & _other ) const;
    Vector & operator *= ( const double _value );
    Vector operator * ( const double _value ) const;
};
