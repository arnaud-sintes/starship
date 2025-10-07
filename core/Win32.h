#pragma once

#include "Maths.h"
#include "std.h"


// ----------------
class Win32
{
private:
    Win32() = default;
    ~Win32() = default;

public:
    struct Handle;
    class Windows;
    static void ShowConsole( const bool _show = true );
    static void SetProcessRealtimePriority();
    static void SetThreadRealtimePriority( const bool _realtime = true );
    static std::optional< std::string > GetTemporaryFolder();

private:
    using FnHook = std::function< void( const unsigned long _key, const bool _pressed ) >;
    static void _InstallKeyboardHook( const bool _install, const Handle & _wnd, FnHook && _hook = nullptr );
    static Handle _GetModuleHandle();
    static void _ShowWindow( const Handle & _wnd, const bool _show = true );
    static void _ResizeWindow( const Handle & _wnd, const Dimension_ui & _dimension );
    static void _SetWindowPos( const Handle & _wnd, const Position_i & _position );
    static Dimension_ui _GetRelatedMonitorDimension( const Handle & _wnd );

    struct Class;
    static Class _CreateClass( const std::wstring & _name );
};


// ----------------
struct Win32::Handle
{
    void * handle{ nullptr };
    const bool mustBeClosed{ true };
    
    ~Handle();

    template< typename _Target >
    _Target As() const { return static_cast< _Target >( handle ); }
};


// ----------------
struct Win32::Class
{
    const std::wstring name;
    const Handle instance;

    Class( const std::wstring & _name );
    ~Class();
};


// ----------------
class Win32::Windows
{
    friend Win32;

public:
    Windows( const std::wstring & _name, const Dimension_ui & _dimension, const bool _fullscreen = false, const bool _doubleBuffer = true );
    ~Windows();

public:
    bool Dispatch();
    enum class eKey : unsigned long {
        space = 0x20,
        up = 0x26,
        down = 0x28,
        right = 0x27,
        left = 0x25,
        lControl = 0xA2,
        rControl = 0xA3,
        lShift =  0xA0,
        rShift = 0xA1,        
    };
    bool KeyPressed( const eKey _key ) const { return m_keyPressed.at( static_cast< unsigned char >( _key ) ); }
    bool LeftMouseButtonPressed() const { return m_leftMouseButtonPressed; }
    bool RightMouseButtonPressed() const { return m_rightMouseButtonPressed; }
    void ShowCursor( const bool _show = true ) const;
    Position_i CursorPosition() const;
    const Handle & GetWindowHandle() const { return m_wnd; }
    const Handle & GetDeviceContext() const { return m_dc; }
    const Dimension_ui & GetDimension() const{ return m_dimension; }
    void Show( const bool _show = true ) const;
    void SetPos( const Position_i & _position ) const;
    void Resize( const Dimension_ui & _dimension ) const;
    void Center() const;

private:
    const Class m_class;
    const Dimension_ui m_dimension;
    const Handle m_wnd;
    const Handle m_dc;
    std::array< std::atomic< bool >, 255 > m_keyPressed;
    bool m_leftMouseButtonPressed{ false };
    bool m_rightMouseButtonPressed{ false };
};
