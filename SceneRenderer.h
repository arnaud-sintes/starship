#pragma once

#include "NanoVGRenderer.h"
#include "StarField.h"
#include "World.h"


// --------------
// Draws the world into a NanoVG frame. Read-only over the simulation; everything is
// culled against the ship-centered screen rectangle before touching NanoVG.
class SceneRenderer
{
public:
    SceneRenderer( const Dimension_ui & _screenDimension );

public:
    void DrawBackdrop( const World & _world, const NanoVGRenderer::Frame & _frame ); // starfield only (prologue)
    void Draw( const World & _world, const NanoVGRenderer::Frame & _frame );

private:
    bool _Visible( const World & _world, const Vector & _worldPosition, const double _radius ) const;
    void _DrawWarden( const Enemy & _enemy, const NanoVGRenderer::Frame & _frame, const Vector & _translation );
    void _DrawForeman( const Enemy & _enemy, const NanoVGRenderer::Frame & _frame, const Vector & _translation );
    void _DrawSingularity( const World & _world, const NanoVGRenderer::Frame & _frame, const Vector & _translation );
    void _DrawAttractors( const World & _world, const NanoVGRenderer::Frame & _frame, const Vector & _translation );

private:
    const Dimension_ui m_screenDimension;
    const Vector m_screenCenter;
    StarField m_starField;
    double m_plasmaShieldReflectAnimation{ 0 };
    double m_decoyPing{ 0 };
    double m_singularityAnim{ 0 };
    double m_singularityDust{ 0 }; // cycle phase of the star dust being sucked in
    double m_foremanAnim{ 0 };
    double m_foremanOpen{ 0 }; // armor petals of the rear core: 0 closed .. 1 open
    double m_foremanCoreSpin{ 0 }; // angle of the thin rings around the rear core: turns faster as it warms up and opens
    double m_wardenAnim{ 0 };

    struct VisibleAttractor
    {
        Vector position; // screen space
        double radius;
        Color_d bodyColor;
        double intensity;
        bool bodyVisible;
        bool haloVisible;
    };
    std::vector< VisibleAttractor > m_visibleAttractors; // reused each frame
};
