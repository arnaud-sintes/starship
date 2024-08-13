#include "PrecisionTime.h"


// --------------
PrecisionTime::PrecisionTime()
    : m_waitEvent{ ::CreateEventA( nullptr, TRUE, FALSE, "" ) }
{
    // map undocumented Nt dll timer functions:
    m_hNtDll = ::GetModuleHandle( L"Ntdll" );
	m_pQueryResolution = ( LPFN_NtQueryTimerResolution ) ::GetProcAddress( m_hNtDll, "NtQueryTimerResolution" );
	m_pSetResolution = ( LPFN_NtSetTimerResolution ) ::GetProcAddress( m_hNtDll, "NtSetTimerResolution" );
	m_NtWaitForSingleObject = ( LPFN_NtWaitForSingleObject ) ::GetProcAddress( ( HMODULE ) m_hNtDll, "NtWaitForSingleObject" );

    // switch to maximum precision:
    ULONG nMinRes = 0, nMaxRes = 0, nCurRes = 0;
	m_pQueryResolution( &nMinRes, &nMaxRes, &nCurRes );
    ULONG dummy = 0;
    m_pSetResolution( nMaxRes, TRUE, &dummy );

    // increase priorities:
    ::SetPriorityClass( ::GetCurrentProcess(), REALTIME_PRIORITY_CLASS );
}


PrecisionTime::~PrecisionTime()
{
    ::FreeLibrary( m_hNtDll );
    ::CloseHandle( m_waitEvent );
}


unsigned long long PrecisionTime::Get()
{
    LARGE_INTEGER counts{ 0 };
    static LARGE_INTEGER countsPerSec{ 0 };
    static BOOL b{ ::QueryPerformanceFrequency( &countsPerSec ) };
    ::QueryPerformanceCounter( &counts );
    return static_cast< unsigned long long >( counts.QuadPart ) * 1'000'000'000 / countsPerSec.QuadPart;
}


void PrecisionTime::Sleep( unsigned long long _ns )
{
    LARGE_INTEGER dueTime;
	dueTime.QuadPart = -( static_cast< long long >( _ns / 100 ) );
	m_NtWaitForSingleObject( m_waitEvent, TRUE, &dueTime );
}


PrecisionTime::TemperObj PrecisionTime::Temper( const unsigned long long _frameRate, std::function< void( const int _consumptionPercent ) > && _fnConsumption, std::function< void() > && _fnReport )
{
    return { *this, _frameRate, std::move( _fnConsumption ), std::move( _fnReport ) };
}


// --------------
PrecisionTime::TemperObj::TemperObj( PrecisionTime & _precisionTime, const unsigned long long _frameRate, std::function< void( const int _consumptionPercent ) > && _fnConsumption, std::function< void() > && _fnReport )
    : m_precisionTime{ _precisionTime }
    , m_loopStartTime{ m_precisionTime.Get() }
    , m_frameRate{ _frameRate }
    , m_fnConsumption{ std::move( _fnConsumption ) }
    , m_fnReport{ std::move( _fnReport ) }
{}


PrecisionTime::TemperObj::~TemperObj()
{
    const auto loopElapsedTime( m_precisionTime.Get() - m_loopStartTime );
    static const auto maxLoopDuration{ 1'000'000'000 / m_frameRate };
    if( loopElapsedTime < maxLoopDuration ) {
        m_fnConsumption( static_cast< int >( loopElapsedTime * 100 / maxLoopDuration ) );
        m_precisionTime.Sleep( maxLoopDuration - loopElapsedTime );
    }
    else
        m_fnReport();
}