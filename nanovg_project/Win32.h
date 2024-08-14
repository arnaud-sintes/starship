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

private:
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
    Windows( const std::wstring & _name, const Dimension_ui & _dimension );
    ~Windows();

public:
    bool Dispatch() const;
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
};
