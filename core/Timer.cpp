#include "Timer.h"

#include <windows.h>


// ----------------

Timer::Timer()
	: m_ntDll{ ::GetModuleHandleW( L"Ntdll" ), false }
    , m_waitEvent{ ::CreateEventA( nullptr, TRUE, FALSE, "" ) }
{
	// map undocumented Nt dll timer functions:
	typedef NTSTATUS( CALLBACK * LPFN_NtQueryTimerResolution ) ( ::PULONG, ::PULONG, ::PULONG );
	m_queryResolution = ( LPFN_NtQueryTimerResolution ) ::GetProcAddress( m_ntDll.As< ::HMODULE >(), "NtQueryTimerResolution" );
    typedef NTSTATUS( CALLBACK * LPFN_NtSetTimerResolution ) ( ::ULONG, ::BOOLEAN, ::PULONG );
	m_setResolution = ( LPFN_NtSetTimerResolution ) ::GetProcAddress( m_ntDll.As< ::HMODULE >(), "NtSetTimerResolution" );
    typedef NTSTATUS( CALLBACK * LPFN_NtWaitForSingleObject ) ( ::HANDLE, ::BOOLEAN, ::PLARGE_INTEGER );
	m_waitForSingleObject = ( LPFN_NtWaitForSingleObject ) ::GetProcAddress( m_ntDll.As< ::HMODULE >(), "NtWaitForSingleObject" );

	// switch to maximum precision:
	::ULONG nMinRes{ 0 }, nMaxRes{ 0 }, nCurRes{ 0 };
	m_queryResolution( &nMinRes, &nMaxRes, &nCurRes );
	::ULONG dummy{ 0 };
    m_setResolution( nMaxRes, TRUE, &dummy );
}


Timer::~Timer()
{
	::FreeLibrary( m_ntDll.As< ::HMODULE >() );
}


Timer & Timer::GetInstance()
{
	static Timer instance;
	return instance;
}


unsigned long long Timer::Get() const
{
    ::LARGE_INTEGER counts{ 0 };
    static ::LARGE_INTEGER countsPerSec{ 0 };
    static ::BOOL b{ ::QueryPerformanceFrequency( &countsPerSec ) };
    ::QueryPerformanceCounter( &counts );
    return static_cast< unsigned long long >( counts.QuadPart ) * 1'000'000'000 / countsPerSec.QuadPart;
}


void Timer::Sleep( unsigned long long _ns ) const
{
    ::LARGE_INTEGER dueTime;
	dueTime.QuadPart = -( static_cast< long long >( _ns / 100 ) );
	m_waitForSingleObject( m_waitEvent.As< ::HANDLE >(), TRUE, &dueTime );
}


Timer::TemperContext Timer::Temper( FpsContext & _fpsContext ) const
{
    return { _fpsContext };
}


// --------------

Timer::FpsContext::FpsContext( const unsigned long long _targetFrameRate )
    : m_targetFrameRate{ _targetFrameRate }
    , m_updateTime{ GetInstance().Get() }
{}


const Timer::FpsContext::State & Timer::FpsContext::Update()
{
    const auto currentTime{ GetInstance().Get() };
    if( currentTime >= m_updateTime + 500'000'000 ) { // refresh every 500ms
        // update states:
        m_state.avgFrameRate = static_cast< double >( m_frameCount * 1'000'000'000 ) / ( currentTime - m_updateTime );
        m_state.avgConsumption = m_consumption / m_frameCount;
        // reset:
        m_state.frameDropped = false;
        m_updateTime = currentTime;
        m_frameCount = 0;
        m_consumption = 0;
    }
    return m_state;
}


// --------------

Timer::TemperContext::TemperContext( FpsContext & _fpsContext )
    : m_fpsContext{ _fpsContext }
    , m_instance{ GetInstance() }
    , m_loopStartTime{ m_instance.Get() }
    , m_maxLoopDuration{ 1'000'000'000 / _fpsContext.m_targetFrameRate }
{}


Timer::TemperContext::~TemperContext()
{
    const auto loopElapsedTime( m_instance.Get() - m_loopStartTime );
    if( loopElapsedTime < m_maxLoopDuration )
        m_instance.Sleep( m_maxLoopDuration - loopElapsedTime );
}

void Timer::TemperContext::Update() const
{
    m_fpsContext.m_frameCount++;
    const auto loopElapsedTime( m_instance.Get() - m_loopStartTime );
    m_fpsContext.m_consumption += static_cast< double >( loopElapsedTime ) * 100 / m_maxLoopDuration;
    if( loopElapsedTime > m_maxLoopDuration )
        m_fpsContext.m_state.frameDropped = true;
}