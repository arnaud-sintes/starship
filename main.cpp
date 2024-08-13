#include "Renderer.h"


int main()
{
    // TODO rethink window creation code / priority management

    const std::wstring windowName{ L"Starship" };
    const size_t windowWidth{ 1920 };
    const size_t windowHeight{ 1000 };

    // windows init:
    ::HINSTANCE hInstance{ ::GetModuleHandleW( nullptr ) };
    WNDCLASS wndClass{ 0 };
    wndClass.lpfnWndProc = []( HWND _hWnd, UINT _msg, WPARAM _wParam, LPARAM _lParam )
    {
        if( _msg != WM_DESTROY )
            return ::DefWindowProc( _hWnd, _msg, _wParam, _lParam );
        ::PostQuitMessage( 0 );
        return LRESULT{ 0 };
    };
    wndClass.hInstance = hInstance;
    wndClass.lpszClassName = windowName.c_str();
    ::RegisterClassW( &wndClass );
    RECT windowRect{ 0, 0, windowWidth, windowHeight };
    DWORD windowStyle{ WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX };
    DWORD windowExStyle{ WS_EX_APPWINDOW | WS_EX_WINDOWEDGE };
    ::AdjustWindowRectEx( &windowRect, windowStyle, FALSE, windowExStyle );
    HWND hWnd{ ::CreateWindowExW( windowExStyle, windowName.c_str(), windowName.c_str(), windowStyle, 200, 200, windowRect.right - windowRect.left, windowRect.bottom - windowRect.top, nullptr, nullptr, hInstance, nullptr ) };
    ::ShowWindow( ::GetConsoleWindow(), SW_HIDE );
    MONITORINFO monitorInfo{ sizeof( MONITORINFO ) };
    ::GetMonitorInfo( MonitorFromWindow( hWnd, MONITOR_DEFAULTTOPRIMARY ), &monitorInfo );
    ::SetWindowPos( hWnd, nullptr, ( monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left - windowWidth ) >> 1,
        ( monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top - windowHeight ) >> 1, 0, 0, SWP_NOSIZE | SWP_NOZORDER );
    ::ShowWindow( hWnd, SW_SHOW );

    // cairo init:
    HDC hDc{ ::GetDC( hWnd ) };
    cairo_surface_t * pCairoSurface{ cairo_win32_surface_create( hDc ) };
    cairo_t * pCairo{ cairo_create( pCairoSurface ) };
    cairo_set_antialias( pCairo, CAIRO_ANTIALIAS_BEST );
        
    // rendering thread:
    Renderer renderer{ *pCairoSurface, *pCairo, windowWidth, windowHeight };
    std::jthread renderThread{ [ & ]( std::stop_token _token ){
            ::SetThreadPriority( ::GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL );
            renderer.Loop( _token );
        } };

    // keyboard hook:
    static const auto fnKeyPress{ [ & ]( const unsigned long _key, const bool _pressed ){
            if( ::GetForegroundWindow() == hWnd )
                renderer.KeyPress( _key, _pressed );
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

    // dispatch thread:
    ::SetThreadPriority( ::GetCurrentThread(), THREAD_PRIORITY_NORMAL );
    MSG msg;
    while( ::GetMessageW( &msg, nullptr, 0, 0 ) ) {
        ::TranslateMessage( &msg );
        ::DispatchMessageW( &msg );
    }

    // stop keyboard hook:
    ::UnhookWindowsHookEx( keyboardHook );

    // rendering thread stop:
    renderThread.request_stop();
    renderThread.join();

    // cairo release:
    cairo_destroy( pCairo );
    cairo_surface_destroy( pCairoSurface );
    ::ReleaseDC( hWnd, hDc );

    // windows release:
    ::DestroyWindow( hWnd );
    ::UnregisterClassW( windowName.c_str(), hInstance  );

    return 0;
}