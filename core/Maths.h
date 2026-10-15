#pragma once

#include "std.h"


// ----------------
template< typename _Type >
struct Dimension
{
    _Type width, height;

    Dimension operator -( const Dimension & _b ) const { return { width - _b.width, height - _b.height }; }
    Dimension operator >>( const _Type _v ) const { return { width >> _v, height >> _v }; }
    
    template< typename _Type >
    Dimension< _Type > ToType() const { return { static_cast< _Type >( width ), static_cast< _Type >( height ) }; }

    template< typename _Target >
    _Target As() const { return { width, height }; }

    template< typename _Target, typename _Type >
    _Target As() const { return { static_cast< _Type >( width ), static_cast< _Type >( height ) }; }
};


using Dimension_ui = Dimension< unsigned int >;


// ----------------
template< typename _Type >
struct Position
{
    _Type x, y;

    template< typename _Type >
    Position< _Type > ToType() const { return { static_cast< _Type >( x ), static_cast< _Type >( y ) }; }
};


using Position_ui = Position< unsigned int >;
using Position_i = Position< int >;
using Position_d = Position< double >;


// ----------------
    
enum eFadeColor {
    orange,
    rose,
    radium,
    green,
    violet,
    azure,
};

template< typename _Type >
struct Color
{
    _Type r, g, b, a{ 1 };

    Color operator *( const _Type _v ) const { return { r * _v, g *_v, b *_v, a *_v }; }

    template< typename _Type >
    Color< _Type > ToType() const { return { static_cast< _Type >( r ), static_cast< _Type >( g ), static_cast< _Type >( b ), static_cast< _Type >( a ) }; }

    static Color FadeOrange( double _ratio )
    {
        _ratio = ( 1 - ( _ratio * _ratio ) ) * 3;
        return Color{ _ratio > 1 ? 1 : _ratio,
            _ratio > 2 ? 1 : ( _ratio > 1 ? _ratio - 1 : 0 ),
            _ratio > 2 ? _ratio - 2 : 0 };
    }

    static Color FadeRose( double _ratio )
    {
        const auto color{ FadeOrange( _ratio ) };
        return Color{ color.r, color.b, color.g };
    }

    static Color FadeRadium( double _ratio )
    {
        const auto color{ FadeOrange( _ratio ) };
        return Color{ color.g, color.r, color.b };
    }

    static Color FadeGreen( double _ratio )
    {
        const auto color{ FadeOrange( _ratio ) };
        return Color{ color.b, color.r, color.g };
    }

    static Color FadeViolet( double _ratio )
    {
        const auto color{ FadeOrange( _ratio ) };
        return Color{ color.g, color.b, color.r };
    }

    static Color FadeAzure( double _ratio )
    {
        const auto color{ FadeOrange( _ratio ) };
        return Color{ color.b, color.g, color.r };
    }

    static Color FadeColor( const eFadeColor _color, double _ratio )
    {
        switch( _color ) {
            case eFadeColor::rose: return FadeRose( _ratio );
            case eFadeColor::radium: return FadeRadium( _ratio );
            case eFadeColor::green: return FadeGreen( _ratio );
            case eFadeColor::violet: return FadeViolet( _ratio );
            case eFadeColor::azure: return FadeAzure( _ratio );
            case eFadeColor::orange: default: return FadeOrange( _ratio );
        }
    }
};


using Color_d = Color< double >;

inline static const Color_d colorWhite{ 1, 1, 1, 1 };


// --------------
struct Vector_f
{
    float u{ 0 }, v{ 0 };
};


struct Vector
{
    double u{ 0 }, v{ 0 };

    static Vector From( const double _orientation, const double _distance );
    static Vector From( const Position_d & _position ) { return { _position.x, _position.y }; }
    double DotProd( const Vector & _other ) const;
    double CrossProd( const Vector & _other ) const;
    double Distance() const;
    double DistanceSquared() const { return u * u + v * v; }
    Vector & Normalize();
    Vector Normalized() const;
    double Orientation() const;
    inline static const double gravitationalConstant{ 6.6743 };
    Vector InfiniteAttraction( const Vector & _attracted, const double _attractorMass ) const;
    Vector ProximityAttraction( const Vector & _attracted, const double _attractorMass, const double _distanceThreshold ) const;
    Vector & operator += ( const Vector & _other );
    Vector operator + ( const Vector & _other ) const;
    Vector & operator -= ( const Vector & _other );
    Vector operator - ( const Vector & _other ) const;
    Vector & operator *= ( const double _value );
    Vector operator * ( const double _value ) const;
    bool operator != ( const Vector & _other ) const { return u != _other.u || v != _other.v; }
    bool operator == ( const Vector & _other ) const { return !operator != ( _other ); }

    operator Position_d() const { return { u, v }; }

    Vector_f ToFloat() const { return { static_cast< float >( u ), static_cast< float >( v ) }; }
};


// --------------
// fake 3D: objects keep a few points in space, rotate them and drop z when drawing (z points toward the viewer)
struct Vector3
{
    double x{ 0 }, y{ 0 }, z{ 0 };

    // the oblique projection of the fake 3D: z is height, it shifts the point up the screen (the ground plane is the game's)
    static constexpr double obliqueShiftX{ 0.1 }, obliqueShiftY{ -0.55 };
    static Vector3 Viewer() { return { -obliqueShiftX, -obliqueShiftY, 1 }; } // toward the viewer: what is seen along the projection
    Vector Oblique() const { return { x + obliqueShiftX * z, y + obliqueShiftY * z }; }

    Vector3 RotatedX( const double _angle ) const { const auto c{ std::cos( _angle ) }, s{ std::sin( _angle ) }; return { x, y * c - z * s, y * s + z * c }; }
    Vector3 RotatedY( const double _angle ) const { const auto c{ std::cos( _angle ) }, s{ std::sin( _angle ) }; return { x * c + z * s, y, z * c - x * s }; }
    Vector3 RotatedZ( const double _angle ) const { const auto c{ std::cos( _angle ) }, s{ std::sin( _angle ) }; return { x * c - y * s, x * s + y * c, z }; }
    Vector3 operator * ( const double _value ) const { return { x * _value, y * _value, z * _value }; }
    Vector3 operator + ( const Vector3 & _other ) const { return { x + _other.x, y + _other.y, z + _other.z }; }
    Vector3 operator - ( const Vector3 & _other ) const { return { x - _other.x, y - _other.y, z - _other.z }; }
    double DotProd( const Vector3 & _other ) const { return x * _other.x + y * _other.y + z * _other.z; }
    Vector3 CrossProd( const Vector3 & _other ) const { return { y * _other.z - z * _other.y, z * _other.x - x * _other.z, x * _other.y - y * _other.x }; }
    Vector3 Normalized() const { const auto length{ std::sqrt( DotProd( *this ) ) }; return length > 0 ? *this * ( 1 / length ) : *this; }
};


// --------------
struct Maths
{
    inline static const double Pi{ std::numbers::pi };
    inline static const double Pi2{ Pi * 2 };
    inline static const double PiHalf{ Pi / 2 };
    inline static const double PiQuarter{ Pi / 4 };

    static bool Collision( const Vector & _circleA, const double _radiusA, const Vector & _circleB, const double _radiusB );
    static std::optional< Vector > Collision( const Vector & _circle, const double _radius, const Vector & _segmentA, const Vector & _segmentB );
    static double NormalizeAngle( const double _angle );
    static double Random( const double _min, const double _max );
    static void Increase( double & _value, const double _increaser, const double _max );
    static void Decrease( double & _value, const double _reducer );
};