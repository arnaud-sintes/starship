#pragma once

#include "Maths.h"


// ----------------
// An XInput gamepad (Xbox-style; other pads work through their XInput mode or Steam Input), polled once per tick.
// It is detected and lost on the fly: while none is plugged, the four slots are only scanned about every two seconds.
class Gamepad
{
public:
    void Poll();

    bool Connected() const { return m_slot >= 0; }
    const Vector & LeftStick() const { return m_leftStick; } // length 0..1 once the dead zone is removed, screen orientation (up is -v)
    double LeftTrigger() const { return m_leftTrigger; }     // 0..1, 0 under the dead zone
    double RightTrigger() const { return m_rightTrigger; }
    bool ButtonA() const { return m_buttonA; }
    bool AnyInput() const { return m_leftStick.DistanceSquared() > 0 || m_leftTrigger > 0 || m_rightTrigger > 0 || m_anyButton; } // something is being touched

private:
    void _Reset();

    int m_slot{ -1 };
    int m_scanCountdown{ 0 };
    Vector m_leftStick;
    double m_leftTrigger{ 0 }, m_rightTrigger{ 0 };
    bool m_buttonA{ false }, m_anyButton{ false };
};
