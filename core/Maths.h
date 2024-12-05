#pragma once


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
template< typename _Type >
struct Color
{
    _Type r, g, b;

    Color operator *( const _Type _v ) const { return { r * _v, g *_v, b *_v }; }

    template< typename _Type >
    Color< _Type > ToType() const { return { static_cast< _Type >( r ), static_cast< _Type >( g ), static_cast< _Type >( b ) }; }

    static Color FireColor( double _ratio )
    {
        _ratio = ( 1 - ( _ratio * _ratio ) ) * 3;
        return Color{ _ratio > 1 ? 1 : _ratio,
            _ratio > 2 ? 1 : ( _ratio > 1 ? _ratio - 1 : 0 ),
            _ratio > 2 ? _ratio - 2 : 0 };
    }
};


using Color_d = Color< double >;

inline static const Color_d colorWhite{ 1, 1, 1 };


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
struct Maths
{
    inline static const double Pi{ 3.1415927 };
    inline static const double Pi2{ Pi * 2 };
    inline static const double PiHalf{ Pi / 2 };
    inline static const double PiQuarter{ Pi / 4 };

    static bool Collision( const Vector & _circleA, const double _radiusA, const Vector & _circleB, const double _radiusB );
    static bool Collision( const Vector & _circle, const double _radius, const Vector & _segmentA, const Vector & _segmentB );
    static double NormalizeAngle( const double _angle );
    static double Random( const double _min, const double _max );
    static void Increase( double & _value, const double _increaser, const double _max );
    static void Decrease( double & _value, const double _reducer );
};