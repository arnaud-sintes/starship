#include "Maths.h"


bool Maths::Collision( const Vector & _circleA, const double _radiusA, const Vector & _circleB, const double _radiusB )
{
    const double u{ _circleA.u - _circleB.u };
    const double v{ _circleA.v - _circleB.v };
    return std::sqrt( u * u + v * v ) <= _radiusA + _radiusB;
}


bool Maths::Collision( const Vector & _circle, const double _radius, const Vector & _segmentA, const Vector & _segmentB )
{
    const Vector d{ _segmentB - _segmentA };
    const Vector f{ _segmentA - _circle };
    double a{ d.DotProd( d ) };
    double b{ 2 * f.DotProd( d ) };
    double c{ f.DotProd( f ) - _radius * _radius };
    double discriminant{ b * b - 4 * a * c };
    if( discriminant < 0 )
        return false;
    discriminant = std::sqrt( discriminant );
    double t1{ ( -b - discriminant ) / ( 2 * a ) };
    double t2{ ( -b + discriminant ) / ( 2 * a ) };
    if( t1 >= 0 && t1 <= 1 )
        return true;
    if( t2 >= 0 && t2 <= 1 )
        return true;
    return false;
}


double Maths::NormalizeAngle( const double _angle )
{
    if( _angle > Maths::Pi )
        return _angle - Maths::Pi2;
    else
    if( _angle < -Maths::Pi )
        return _angle + Maths::Pi2;
    return _angle;
}


double Maths::Random( const double _min, const double _max )
{
    static std::random_device rndDev;
    static std::mt19937 rnd{ rndDev() };
    constexpr unsigned int maxRandom{ 10000 };
    static std::uniform_int_distribution< std::mt19937::result_type > rndDist{ 0, maxRandom };
    return static_cast< double >( rndDist( rnd ) ) * ( _max - _min ) / maxRandom + _min;
}


void Maths::Increase( double & _value, const double _increaser, const double _max )
{
    if( _value < ( _max - _increaser ) )
        _value += _increaser;
}


void Maths::Decrease( double & _value, const double _reducer )
{
    if( _value == 0 )
        return;
    if( ( _value > 0 && _value <= _reducer ) || ( _value < 0 && _value >= _reducer ) )
        _value = 0;
    else
        _value += _reducer * ( _value > 0 ? -1 : 1 );
}