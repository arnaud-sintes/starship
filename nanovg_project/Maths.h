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
};


using Color_d = Color< double >;