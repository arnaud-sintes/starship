#pragma once

#include "View.h"
#include "core/Win32.h"
#include "core/Gamepad.h"


// --------------
// The pointer of the game (a cursor and two buttons), fed by the mouse or by the gamepad, whichever the player touched last:
// the gamepad is detected on the fly, and the game follows the device in use, with no jump when it changes (the system cursor is
// kept on the virtual one).
//   gamepad, while flying: the left stick puts the cursor around the ship, in the direction it points (the ship aims there at once)
//   gamepad, in the menus: the left stick moves the cursor, A (or R2) clicks
//   gamepad, while flying, aim assist: a cursor that comes close to the line from the ship to the closest enemy sticks to it (the ship
//     follows the enemy), and lets go when the stick pulls it too far away
//   gamepad, while flying: the right trigger (R2) is the left button (shoot), A the right one (the main thrust engine)
class PointerInput
{
public:
    enum class eDevice
    {
        mouse,
        gamepad,
    };

    struct AimTarget // what the aim assist sticks to
    {
        unsigned long long id;
        Vector offset;  // from the ship, in the world
        double radius;
    };

public:
    // once per tick; _aiming: flying, not in a menu; _target: the closest enemy, if any
    void Update( const Win32::Windows & _windows, const View & _view, const bool _aiming, const double _tickSeconds, const std::optional< AimTarget > & _target );

    const Vector & Cursor() const { return m_cursor; } // logical coordinates
    bool Left() const { return m_left; }
    bool Right() const { return m_right; }
    eDevice Device() const { return m_device; }
    bool UsingGamepad() const { return m_device == eDevice::gamepad; }

private:
    Gamepad m_gamepad;
    eDevice m_device{ eDevice::mouse };
    Vector m_cursor;
    Position_i m_syncedMouse{ 0, 0 }; // where the system cursor is expected: it moves away from it when the mouse is used
    bool m_started{ false };
    bool m_stuck{ false }; // the cursor follows the closest enemy
    unsigned long long m_stuckId{ 0 };
    bool m_left{ false }, m_right{ false };
};
