#pragma once

#include "core/Maths.h"


// --------------
struct Attractor
{
    Vector position;
    double mass{ 0 };
    double shield{ 0 };
    double radius{ 0 }; // mass * AttractorField::massSizeRatio, cached
    bool alive{ true };
};


// --------------
// Static field of strange attractors over a uniform spatial grid: attractors never
// move, so both the physics queries (attraction, collisions) and the visibility
// queries only visit the few cells around the query area instead of the whole field.
class AttractorField
{
public:
    static constexpr double massSizeRatio{ 50 };
    static constexpr double distanceThreshold{ 40 };

public:
    void Generate( const int _count, const double _range, const double _securityDistance, const double _minMass, const double _maxMass );

    std::vector< Attractor > & All() { return m_attractors; }
    const std::vector< Attractor > & All() const { return m_attractors; }
    double MaxMass() const { return m_maxMass; }
    double MaxRadius() const { return m_maxMass * massSizeRatio; }

    void Remove( const size_t _index ); // marks dead and unregisters from the grid

    // visits every live attractor that may interact within _range of _center, the
    // attractors' own extent included (grid-level filtering only, do the precise
    // distance check in the callback):
    template< typename _Fn >
    void ForEachInRange( const Vector & _center, const double _range, _Fn && _fn )
    {
        const double range{ _range + MaxRadius() };
        _ForEachInRect( _center - Vector{ range, range }, _center + Vector{ range, range }, std::forward< _Fn >( _fn ) );
    }

    // visits every live attractor whose *body or halo* may intersect the world-space
    // rectangle, expanding by _margin (e.g. halo radius factor):
    template< typename _Fn >
    void ForEachInRect( const Vector & _min, const Vector & _max, const double _margin, _Fn && _fn ) const
    {
        const_cast< AttractorField * >( this )->_ForEachInRect( _min - Vector{ _margin, _margin }, _max + Vector{ _margin, _margin }, std::forward< _Fn >( _fn ) );
    }

private:
    template< typename _Fn >
    void _ForEachInRect( const Vector & _min, const Vector & _max, _Fn && _fn )
    {
        const auto cellMinX{ _Cell( _min.u ) }, cellMinY{ _Cell( _min.v ) };
        const auto cellMaxX{ _Cell( _max.u ) }, cellMaxY{ _Cell( _max.v ) };
        for( auto cellY{ cellMinY }; cellY <= cellMaxY; cellY++ )
            for( auto cellX{ cellMinX }; cellX <= cellMaxX; cellX++ ) {
                const auto itCell{ m_cells.find( _CellKey( cellX, cellY ) ) };
                if( itCell == m_cells.end() )
                    continue;
                for( const auto index : itCell->second ) {
                    auto & attractor{ m_attractors.at( index ) };
                    if( attractor.alive )
                        _fn( index, attractor );
                }
            }
    }

    static long long _Cell( const double _coordinate );
    static long long _CellKey( const long long _cellX, const long long _cellY );

private:
    // cells only index the attractor's center; queries expand by MaxRadius() where
    // the attractor's own extent matters:
    static constexpr double m_cellSize{ 256 };
    std::vector< Attractor > m_attractors;
    std::unordered_map< long long, std::vector< int > > m_cells;
    double m_maxMass{ 0 };
};
