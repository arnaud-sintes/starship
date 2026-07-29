#pragma once

#include "NanoVGRenderer.h"
#include "World.h"


// --------------
class Hud
{
public:
    Hud( const Dimension_ui & _screenDimension );

public:
    void Draw( const World & _world, const NanoVGRenderer::Frame & _frame ) const;

private:
    const Dimension_ui m_screenDimension;
};
