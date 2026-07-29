#include "AttractorField.h"


void AttractorField::Generate( const int _count, const double _range, const double _securityDistance, const double _minMass, const double _maxMass )
{
    m_attractors.reserve( _count );
    for( int i{ 0 }; i < _count; i++ ) {
        const auto mass{ Maths::Random( _minMass, _maxMass ) };
        const auto radius{ mass * massSizeRatio };
        // pick a position outside the starting security zone and not colliding with
        // an already placed attractor:
        Vector position;
        for( bool colliding{ true }; colliding; ) {
            position = {};
            while( position.u > -_securityDistance && position.u < _securityDistance ) position.u = Maths::Random( -_range, _range );
            while( position.v > -_securityDistance && position.v < _securityDistance ) position.v = Maths::Random( -_range, _range );
            colliding = false;
            for( const auto & attractor : m_attractors )
                if( Maths::Collision( position, radius, attractor.position, attractor.radius ) ) {
                    colliding = true;
                    break;
                }
        }
        m_attractors.emplace_back( Attractor{ position, mass, 10.0 * mass, radius } );
        m_cells[ _CellKey( _Cell( position.u ), _Cell( position.v ) ) ].emplace_back( i );
        m_maxMass = std::max( m_maxMass, mass );
    }
}


void AttractorField::Remove( const size_t _index )
{
    auto & attractor{ m_attractors.at( _index ) };
    attractor.alive = false;
    auto & cell{ m_cells.find( _CellKey( _Cell( attractor.position.u ), _Cell( attractor.position.v ) ) )->second };
    std::erase( cell, static_cast< int >( _index ) );
}


long long AttractorField::_Cell( const double _coordinate )
{
    return static_cast< long long >( std::floor( _coordinate / m_cellSize ) );
}


long long AttractorField::_CellKey( const long long _cellX, const long long _cellY )
{
    return ( _cellX << 32 ) ^ ( _cellY & 0xffffffffll );
}
