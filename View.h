#pragma once

#include "core/Maths.h"


// ----------------
// Maps the logical design coordinates (world, HUD and input) to the physical client
// area. The logical height is fixed - the gameplay scale is identical everywhere -
// while the logical width follows the client aspect ratio, so the frame always fills
// the screen edge to edge: no letterbox bars, no distortion.
struct View
{
    const Dimension_ui logical;
    const Dimension_ui physical;
    const double scale; // physical/logical height ratio, drives the tessellation quality

    View( const unsigned int _logicalHeight, const Dimension_ui & _physical )
        : logical{ ( _physical.width * _logicalHeight + _physical.height / 2 ) / _physical.height, _logicalHeight }
        , physical{ _physical }
        , scale{ static_cast< double >( _physical.height ) / _logicalHeight }
    {}

    Vector ToLogical( const Position_i & _physicalPosition ) const
    {
        return { _physicalPosition.x * static_cast< double >( logical.width ) / physical.width,
                 _physicalPosition.y * static_cast< double >( logical.height ) / physical.height };
    }
};
