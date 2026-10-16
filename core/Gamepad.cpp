#include "Gamepad.h"

#define NOMINMAX // keep std::min/max usable
#include <windows.h>
#include <Xinput.h>

#pragma comment( lib, "xinput.lib" )


namespace
{
    constexpr double stickDeadZone{ 0.24 };   // radial, as a ratio of the full range
    constexpr double triggerDeadZone{ 30 / 255.0 };
    constexpr int scanPeriod{ 120 };          // ticks between two scans of the slots while no pad is plugged (polling an empty slot is slow)

    double Trigger( const unsigned char _raw )
    {
        const auto value{ _raw / 255.0 };
        return value <= triggerDeadZone ? 0 : ( value - triggerDeadZone ) / ( 1 - triggerDeadZone );
    }
}


void Gamepad::_Reset()
{
    m_slot = -1;
    m_leftStick = {};
    m_leftTrigger = m_rightTrigger = 0;
    m_buttonA = m_buttonB = m_anyButton = m_dpadLeft = m_dpadRight = m_dpadUp = m_dpadDown = false;
}


void Gamepad::Poll()
{
    XINPUT_STATE state{};
    if( m_slot >= 0 && ::XInputGetState( static_cast< DWORD >( m_slot ), &state ) != ERROR_SUCCESS )
        _Reset(); // unplugged
    if( m_slot < 0 ) {
        if( m_scanCountdown-- > 0 )
            return;
        m_scanCountdown = scanPeriod;
        for( int slot{ 0 }; slot < XUSER_MAX_COUNT; slot++ )
            if( ::XInputGetState( static_cast< DWORD >( slot ), &state ) == ERROR_SUCCESS ) {
                m_slot = slot; // plugged: the first pad found
                break;
            }
        if( m_slot < 0 )
            return;
    }

    const auto & pad{ state.Gamepad };
    const Vector raw{ pad.sThumbLX / 32767.0, -pad.sThumbLY / 32767.0 };
    const auto length{ std::min( std::sqrt( raw.DistanceSquared() ), 1.0 ) };
    m_leftStick = length <= stickDeadZone ? Vector{} : raw.Normalized() * ( ( length - stickDeadZone ) / ( 1 - stickDeadZone ) );
    m_leftTrigger = Trigger( pad.bLeftTrigger );
    m_rightTrigger = Trigger( pad.bRightTrigger );
    m_buttonA = ( pad.wButtons & XINPUT_GAMEPAD_A ) != 0;
    m_buttonB = ( pad.wButtons & XINPUT_GAMEPAD_B ) != 0;
    m_dpadLeft = ( pad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT ) != 0;
    m_dpadRight = ( pad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT ) != 0;
    m_dpadUp = ( pad.wButtons & XINPUT_GAMEPAD_DPAD_UP ) != 0;
    m_dpadDown = ( pad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN ) != 0;
    m_anyButton = pad.wButtons != 0;
}
