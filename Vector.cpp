#include "Vector.h"

#include "std.h"


Vector Vector::From( const double _orientation, const double _distance )
{
    return { std::cos( _orientation ) * _distance, std::sin( _orientation ) * _distance };
}


double Vector::DotProd( const Vector & _other ) const
{
    return u * _other.u + v * _other.v;
}


double Vector::CrossProd( const Vector & _other ) const
{
    return u * _other.v + v * _other.u;
}


double Vector::Distance() const
{
    return std::sqrt( u * u + v * v );
}


Vector & Vector::Normalize()
{
    const auto distance{ Distance() };
    u /= distance;
    v /= distance;
    return *this;
}


Vector Vector::Normalized() const
{
    const auto distance{ Distance() };
    return { u / distance, v / distance };
}


double Vector::Orientation() const
{
    const auto distance{ Distance() };
    return std::atan2( v / distance, u / distance );
}


Vector & Vector::operator += ( const Vector & _other )
{
    u += _other.u;
    v += _other.v;
    return *this;
}


Vector Vector::operator + ( const Vector & _other ) const
{
    return { u + _other.u, v + _other.v };
}


Vector & Vector::operator -= ( const Vector & _other )
{
    u -= _other.u;
    v -= _other.v;
    return *this;
}


Vector Vector::operator - ( const Vector & _other ) const
{
    return { u - _other.u, v - _other.v };
}


Vector & Vector::operator *= ( const double _value )
{
    u *= _value;
    v *= _value;
    return *this;
}


Vector Vector::operator * ( const double _value ) const
{
    return { u * _value, v * _value };
}