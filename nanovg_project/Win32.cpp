#include "Win32.h"

#include <windows.h>
#undef GetModuleHandle


// ----------------

void Win32::ShowConsole( const bool _show )
{
    _ShowWindow( { ::GetConsoleWindow(), false }, _show );
}


void Win32::_InstallKeyboardHook( const bool _install, const Handle & _wnd, FnHook && _hook )
{
    // hooks:
    static std::mutex hooksMtx;
    static std::unordered_map< void *, FnHook > hooks;

    // add or remove hooks:
    {
        std::unique_lock lock{ hooksMtx };
        if( _install )
            hooks.emplace( _wnd.handle, std::move( _hook ) );
        else
            hooks.erase( _wnd.handle );
    }

    // install global keyboard hook (first time):
    static const auto fnKeyPress{ []( const unsigned long _key, const bool _pressed ){
            std::unique_lock lock{ hooksMtx };
            for( const auto & hook : hooks )
                if( ::GetForegroundWindow() == static_cast< HWND >( hook.first ) )
                    hook.second( _key, _pressed );
        } };
    static HHOOK keyboardHook{ ::SetWindowsHookEx( WH_KEYBOARD_LL, []( const int _nCode, const WPARAM _wParam, const LPARAM _lParam ) {
            auto press{ ( PKBDLLHOOKSTRUCT ) _lParam };
            if( press->vkCode == 0x0d && ( press->flags & LLKHF_EXTENDED ) != 0 )
		        press->vkCode = 0x0e;
            const bool down{ _wParam == WM_KEYDOWN || _wParam == WM_SYSKEYDOWN };
            if( ( _wParam == WM_KEYDOWN || _wParam == WM_SYSKEYDOWN ) || ( _wParam == WM_KEYUP || _wParam == WM_SYSKEYUP ) )
                fnKeyPress( press->vkCode, down );
            return CallNextHookEx( keyboardHook, _nCode, _wParam, _lParam );
        }, nullptr, 0 ) };

    // stop global keyboard hook (last call):
    static int hookCount{ 0 };
    hookCount += _install ? 1 : -1;
    if( hookCount == 0 )
        ::UnhookWindowsHookEx( keyboardHook );
}


Win32::Handle Win32::_GetModuleHandle()
{
    return { ::GetModuleHandleW( nullptr ), false };
}


void Win32::_ShowWindow( const Handle & _wnd, const bool _show )
{
    ::ShowWindow( _wnd.As< ::HWND >(), _show ? SW_SHOW : SW_HIDE );
}


void Win32::_ResizeWindow( const Handle & _wnd, const Dimension_ui & _dimension )
{
    auto hWnd{ _wnd.As< ::HWND >() };
    ::RECT rect{ 0, 0, static_cast< LONG >( _dimension.width ), static_cast< LONG >( _dimension.height ) };
    ::AdjustWindowRect( &rect, ::GetWindowLong( hWnd, GWL_STYLE ), FALSE );
    ::SetWindowPos( hWnd, nullptr, 0, 0, rect.right - rect.left, rect.bottom - rect.top, SWP_NOZORDER | SWP_NOMOVE );
}


void Win32::_SetWindowPos( const Handle & _wnd, const Position_i & _position )
{
    ::SetWindowPos( _wnd.As< ::HWND >(), nullptr, _position.x, _position.y, 0, 0, SWP_NOZORDER | SWP_NOSIZE );
}


Dimension_ui Win32::_GetRelatedMonitorDimension( const Handle & _wnd )
{
    ::MONITORINFO monitorInfo{ sizeof( ::MONITORINFO ) };
    ::GetMonitorInfoW( ::MonitorFromWindow( _wnd.As< ::HWND >(), MONITOR_DEFAULTTOPRIMARY ), &monitorInfo );
    return { static_cast< unsigned int >( monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left ),
        static_cast< unsigned int >( monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top ) };
}


// ----------------

Win32::Handle::~Handle()
{
    if( handle != nullptr && mustBeClosed )
        ::CloseHandle( handle );
}


// ----------------

Win32::Windows::Windows( const std::wstring & _name, const Dimension_ui & _dimension )
    : m_class{ _CreateClass( _name ) }
    , m_dimension{ _dimension }
    , m_wnd{ nullptr, false }
    , m_dc{ nullptr, false }
{   
    // create window:
    const_cast< Handle & >( m_wnd ).handle = ::CreateWindowExW( 0, _name.c_str(), _name.c_str(),
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX, 0, 0, 0, 0,
        nullptr, nullptr, m_class.instance.As< ::HINSTANCE >(), nullptr );
    _ResizeWindow( m_wnd, _dimension );
    
    // center by default:
    Center();

    // install keyboard hook:
    _InstallKeyboardHook( true, m_wnd, [ this ]( const unsigned long _key, const bool _pressed ){
            m_keyPressed.at( static_cast< unsigned char >( _key ) )  = _pressed;
        } );

    // setup device context:
    const_cast< Handle & >( m_dc ).handle = ::GetDC( m_wnd.As< ::HWND >() );

    // set pixel format:
	::PIXELFORMATDESCRIPTOR pfdesc{
		sizeof( ::PIXELFORMATDESCRIPTOR ), 1, PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_SUPPORT_COMPOSITION | PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA, 32, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 24, 8, 0, PFD_MAIN_PLANE, 0, 0, 0, 0
	};
	::SetPixelFormat( m_dc.As< ::HDC >(), ::ChoosePixelFormat( m_dc.As< ::HDC >(), &pfdesc ), &pfdesc );

    // show window:
    Show();
}


Win32::Windows::~Windows()
{
    _InstallKeyboardHook( false, m_wnd );
    ::ReleaseDC( m_wnd.As< ::HWND >(), m_dc.As< ::HDC >() );
    ::DestroyWindow( m_wnd.As< ::HWND >() );
}


bool Win32::Windows::Dispatch() const
{
    MSG msg;
    if( !::GetMessageW( &msg, nullptr, 0, 0 ) )
        return false;
    ::TranslateMessage( &msg );
    ::DispatchMessageW( &msg );
    return true;
}


void Win32::Windows::Show( const bool _show ) const
{
    _ShowWindow( m_wnd, _show );
}


void Win32::Windows::SetPos( const Position_i & _position ) const
{
    _SetWindowPos( m_wnd, _position );
}


void Win32::Windows::Resize( const Dimension_ui & _dimension ) const
{
    const_cast< Dimension_ui & >( m_dimension ) = _dimension;
    _ResizeWindow( m_wnd, m_dimension );
}


void Win32::Windows::Center() const
{
    const auto monitorDimension{ _GetRelatedMonitorDimension( m_wnd ) };
    SetPos( ( ( monitorDimension - m_dimension ) >> 1 ).As< Position_i, int >() );
}


// ----------------

Win32::Class::Class( const std::wstring & _name )
    : name{ _name }
    , instance{ _GetModuleHandle() }
{
    ::WNDCLASS wndClass{ 0 };
    wndClass.lpfnWndProc = []( ::HWND _hWnd, ::UINT _msg, ::WPARAM _wParam, ::LPARAM _lParam ) {
            if( _msg != WM_DESTROY )
                return ::DefWindowProc( _hWnd, _msg, _wParam, _lParam );
            ::PostQuitMessage( 0 );
            return ::LRESULT{ 0 };
        };
    wndClass.hInstance = instance.As< ::HINSTANCE >();
    wndClass.lpszClassName = name.c_str();
    ::RegisterClassW( &wndClass );
}

Win32::Class::~Class()
{
    ::UnregisterClassW( name.c_str(), instance.As< ::HINSTANCE >() );
}


Win32::Class Win32::_CreateClass( const std::wstring & _name )
{
    return { _name };
}