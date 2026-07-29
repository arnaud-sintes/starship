#pragma once

#include "core/Maths.h"


// --------------
// Solar wind waves: a global drift (the historical solar wind) plus a coarse grid of
// per-cell wave vectors, each evolving smoothly toward random targets. The grid tiles
// space, so any world position samples a wind vector through cheap bilinear
// interpolation - no storage growth, unbounded world.
class WindField
{
public:
    WindField( const int _frameRate );

public:
    void Update(); // once per tick: evolve the base drift and every cell wave
    Vector At( const Vector & _position ) const; // bilinearly interpolated wind vector
    const Vector & Base() const { return m_base.current; }

private:
    struct Flow
    {
        Vector current;
        Vector from, to;
        int index{ 0 }, count{ 0 };
    };
    void _UpdateFlow( Flow & _flow, const double _minMagnitude, const double _maxMagnitude ) const;

private:
    static constexpr int gridSize{ 16 }; // cells per axis, the field tiles every gridSize * cellSize units
    static constexpr double cellSize{ 384 };

    const int m_frameRate;
    Flow m_base; // global drift, the cell waves modulate around it
    std::array< Flow, gridSize * gridSize > m_cells;
};
