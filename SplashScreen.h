#pragma once

#include "NanoVGRenderer.h"


// --------------
// The title screen shown before the game: a hyperspace warp slowing down, three tumbling rings and thin technical circles around the
// game title, which settles out of a chromatic glitch. Drawing only; the animation advances with every drawn frame.
class SplashScreen
{
public:
    explicit SplashScreen( const Dimension_ui & _logical );

public:
    void Draw( const NanoVGRenderer::Frame & _frame, const std::string & _version );
    double Seconds() const { return m_time; } // since the first frame

private:
    const Dimension_ui m_logical;
    double m_time{ 0 };
    double m_warp{ 0 }; // cycle phase of the star streaks
};
