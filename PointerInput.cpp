#include "PointerInput.h"


namespace
{
    constexpr double aimRadius{ 320 };       // logical, flying: how far from the ship the stick puts the cursor
    constexpr double menuSpeed{ 900 };       // logical per second, in the menus, at full tilt
    constexpr double triggerPressed{ 0.12 }; // of the trigger range
    constexpr double assistMinAngle{ 0.12 }, assistMaxAngle{ 0.4 }; // rad, how close to the line to the enemy the cursor must come to stick (wider for a big or near enemy)
    constexpr double assistSlack{ 30 };      // world pixels added to the enemy radius
    constexpr double assistReleaseFactor{ 1.7 }, assistReleaseAngle{ 0.05 }; // it lets go further away than it catches
    constexpr int mouseTolerance{ 3 };       // physical pixels the system cursor may differ from where it was left
}


void PointerInput::Update( const Win32::Windows & _windows, const View & _view, const bool _aiming, const double _tickSeconds, const std::optional< AimTarget > & _target )
{
    m_gamepad.Poll();
    m_menuVertical = m_gamepad.DpadUp() || m_gamepad.LeftStick().v < -0.5 ? -1 : ( m_gamepad.DpadDown() || m_gamepad.LeftStick().v > 0.5 ? 1 : 0 );
    m_menuDirection = m_gamepad.DpadLeft() || m_gamepad.LeftStick().u < -0.5 ? -1 : ( m_gamepad.DpadRight() || m_gamepad.LeftStick().u > 0.5 ? 1 : 0 );

    // which device is in use: the one touched last (when both are at the same time, the current one stays)
    const auto mouse{ _windows.CursorPosition() };
    if( !m_started ) {
        m_started = true;
        m_syncedMouse = mouse;
        m_cursor = _view.ToLogical( mouse );
    }
    const auto mouseUsed{ std::abs( mouse.x - m_syncedMouse.x ) > mouseTolerance || std::abs( mouse.y - m_syncedMouse.y ) > mouseTolerance
        || _windows.LeftMouseButtonPressed() || _windows.RightMouseButtonPressed() || _windows.MiddleMouseButtonPressed() };
    const auto gamepadUsed{ m_gamepad.Connected() && m_gamepad.AnyInput() };
    if( mouseUsed && !gamepadUsed )
        m_device = eDevice::mouse;
    else if( gamepadUsed && !mouseUsed )
        m_device = eDevice::gamepad;

    if( m_device == eDevice::mouse ) {
        m_cursor = _view.ToLogical( mouse );
        m_syncedMouse = mouse;
        m_left = _windows.LeftMouseButtonPressed();
        m_right = _windows.RightMouseButtonPressed();
        m_hold = _windows.MiddleMouseButtonPressed();
        return;
    }

    const auto & stick{ m_gamepad.LeftStick() };
    const auto previous{ m_cursor };
    if( _aiming ) {
        const Vector center{ _view.logical.width * 0.5, _view.logical.height * 0.5 };
        // where the player points: the stick, or where the cursor was left (released, the ship keeps its heading)
        const auto left{ m_cursor - center };
        const auto pointed{ stick.DistanceSquared() > 0 ? stick.Normalized() : ( left.DistanceSquared() > 1e-6 ? left.Normalized() : Vector{} ) };
        auto heading{ pointed };
        if( _target && ( pointed.u != 0 || pointed.v != 0 ) ) {
            const auto distance{ std::max( std::sqrt( _target->offset.DistanceSquared() ), 1.0 ) };
            const auto toTarget{ _target->offset * ( 1 / distance ) };
            const auto catchAngle{ std::clamp( std::atan2( _target->radius + assistSlack, distance ), assistMinAngle, assistMaxAngle ) };
            const auto limit{ m_stuck && m_stuckId == _target->id ? catchAngle * assistReleaseFactor + assistReleaseAngle : catchAngle };
            const auto error{ std::acos( std::clamp( pointed.DotProd( toTarget ), -1.0, 1.0 ) ) };
            m_stuck = error < limit;
            m_stuckId = _target->id;
            if( m_stuck )
                heading = toTarget; // on the line, and following the enemy
        }
        else
            m_stuck = false;
        if( heading.u != 0 || heading.v != 0 )
            m_cursor = center + heading * aimRadius;
    }
    else {
        m_cursor += stick * ( std::sqrt( stick.DistanceSquared() ) * menuSpeed * _tickSeconds ); // the farther the stick, the faster: fine near the center
        m_cursor.u = std::clamp( m_cursor.u, 0.0, static_cast< double >( _view.logical.width ) );
        m_cursor.v = std::clamp( m_cursor.v, 0.0, static_cast< double >( _view.logical.height ) );
    }
    if( m_cursor != previous ) { // the system cursor follows, so that taking the mouse back does not jump
        m_syncedMouse = _view.ToPhysical( m_cursor );
        _windows.SetCursorPosition( m_syncedMouse );
    }
    m_left = m_gamepad.RightTrigger() > triggerPressed || ( !_aiming && m_gamepad.ButtonA() ); // R2 shoots (clicks), A clicks in the menus
    m_right = _aiming && m_gamepad.ButtonA(); // A is the thrust
    m_hold = _aiming && m_gamepad.ButtonB();
}
