#pragma once

#include "NanoVGRenderer.h"


// --------------
// Fake-3D naval mine: a shaded sphere with horns distributed on it, tumbling in space. The horns are
// rotated in 3D and projected, far ones are drawn behind the body and dimmed, near ones in front.
struct SpikedBall
{
    struct Style
    {
        int horns{ 8 };
        double bodyRadius{ 12 };
        double tipDistance{ 18 };   // distance of the horn tips from the center
        double hornWidth{ 3 };
        double tipRadius{ 2 };
        double yaw{ 0 }, pitch{ 0 }; // orientation of the tumbling ball
        Color_d body, horn, tip, highlight;
        Color_d ember{ 0, 0, 0, 0 };  // optional glowing spot orbiting on the surface (alpha 0: none)
        double emberAngle{ 0 }, emberRadius{ 2.5 };
    };

    static constexpr int maxHorns{ 12 };

    static void Draw( const NanoVGRenderer::Frame & _frame, const Vector & _center, const Style & _style );
};
