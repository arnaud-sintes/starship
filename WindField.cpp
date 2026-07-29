#include "WindField.h"


WindField::WindField( const int _frameRate )
    : m_frameRate{ _frameRate }
{
    m_base.current = { 0.05, 0.2 }; // historical initial solar wind
}


void WindField::_UpdateFlow( Flow & _flow, const double _minMagnitude, const double _maxMagnitude ) const
{
    if( _flow.index == 0 ) {
        _flow.from = _flow.current;
        _flow.to = Vector::From( Maths::Random( 0, Maths::Pi2 ), Maths::Random( _minMagnitude, _maxMagnitude ) );
        _flow.count = m_frameRate * static_cast< int >( Maths::Random( 3, 10 ) ); // between 3 and 10 secs transitions
    }
    const double rate{ static_cast< double >( _flow.index++ ) / static_cast< double >( _flow.count ) };
    if( _flow.index == _flow.count )
        _flow.index = 0;
    _flow.current = _flow.from * ( 1.0 - rate ) + ( _flow.to * rate );
}


void WindField::Update()
{
    _UpdateFlow( m_base, 0.1, 0.3 );
    for( auto & cell : m_cells )
        _UpdateFlow( cell, 0, 0.25 );
}


Vector WindField::At( const Vector & _position ) const
{
    // wrap the position into the tiling grid:
    constexpr double tile{ gridSize * cellSize };
    double u{ std::fmod( _position.u, tile ) };
    if( u < 0 )
        u += tile;
    double v{ std::fmod( _position.v, tile ) };
    if( v < 0 )
        v += tile;

    // bilinear interpolation of the four surrounding cell waves:
    const double cellU{ u / cellSize };
    const double cellV{ v / cellSize };
    const int x0{ static_cast< int >( cellU ) % gridSize };
    const int y0{ static_cast< int >( cellV ) % gridSize };
    const int x1{ ( x0 + 1 ) % gridSize };
    const int y1{ ( y0 + 1 ) % gridSize };
    const double fracU{ cellU - std::floor( cellU ) };
    const double fracV{ cellV - std::floor( cellV ) };
    const auto & wave00{ m_cells.at( y0 * gridSize + x0 ).current };
    const auto & wave10{ m_cells.at( y0 * gridSize + x1 ).current };
    const auto & wave01{ m_cells.at( y1 * gridSize + x0 ).current };
    const auto & wave11{ m_cells.at( y1 * gridSize + x1 ).current };
    const auto top{ wave00 * ( 1 - fracU ) + wave10 * fracU };
    const auto bottom{ wave01 * ( 1 - fracU ) + wave11 * fracU };
    return m_base.current + top * ( 1 - fracV ) + bottom * fracV;
}
