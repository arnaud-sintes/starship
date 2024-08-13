#include "Graphics.h"


Graphics::Color Graphics::FireColor( double _ratio )
{
    _ratio = ( 1 - ( _ratio * _ratio ) ) * 3;
    return Color{ _ratio > 1 ? 1 : _ratio,
        _ratio > 2 ? 1 : ( _ratio > 1 ? _ratio - 1 : 0 ),
        _ratio > 2 ? _ratio - 2 : 0 };
}


void Graphics::Stroke( cairo_t & _cairo, const Vector & _position, const double _radius, const Arc & _arc, const Color & _color, const double _strokeWidth )
{
    cairo_new_path( &_cairo );
    cairo_set_source_rgb( &_cairo, _color.r, _color.g, _color.b );
    cairo_arc( &_cairo, _position.u, _position.v, _radius, _arc.a, _arc.b );
    cairo_set_line_width( &_cairo, _strokeWidth );
    cairo_stroke( &_cairo );
}


void Graphics::Fill( cairo_t & _cairo, const Vector & _position, const double _radius, const Arc & _arc, const Color & _color )
{
    cairo_new_path( &_cairo );
    cairo_set_source_rgb( &_cairo, _color.r, _color.g, _color.b );
    cairo_arc( &_cairo, _position.u, _position.v, _radius, _arc.a, _arc.b );
    cairo_fill( &_cairo );
}


void Graphics::Line( cairo_t & _cairo, const Vector & _a, const Vector & _b, const Color & _color, const double _strokeWidth, const bool _dash )
{
    cairo_new_path( &_cairo );
    cairo_set_source_rgb( &_cairo, _color.r, _color.g, _color.b );
    cairo_set_line_width( &_cairo, _strokeWidth );
    cairo_move_to( &_cairo, _a.u, _a.v );
    cairo_line_to( &_cairo, _b.u, _b.v );
    if( _dash ) {
        cairo_save( &_cairo );
        static double dashes[]{ 6, 3 };
        cairo_set_dash( &_cairo, dashes, 2, 0 );
    }
    cairo_stroke( &_cairo );
    if( _dash )
        cairo_restore( &_cairo );
}


void Graphics::Text( cairo_t & _cairo, const Vector & _position, const double _size, const Color & _color, const std::string & _text )
{
    cairo_new_path( &_cairo );
    cairo_set_source_rgb( &_cairo, _color.r, _color.g, _color.b );
    cairo_set_font_size( &_cairo, _size );
    cairo_move_to( &_cairo, _position.u, _position.v );
    cairo_show_text( &_cairo, _text.c_str() );
}