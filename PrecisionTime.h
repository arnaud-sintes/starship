#pragma once

#include <windows.h>

#include "std.h"


// --------------
class PrecisionTime
{
private:
    typedef NTSTATUS( CALLBACK * LPFN_NtQueryTimerResolution ) ( PULONG, PULONG, PULONG );
    typedef NTSTATUS( CALLBACK * LPFN_NtSetTimerResolution ) ( ULONG, BOOLEAN, PULONG );
    typedef NTSTATUS( CALLBACK * LPFN_NtWaitForSingleObject ) ( HANDLE, BOOLEAN, PLARGE_INTEGER );

public:
    PrecisionTime();
    ~PrecisionTime();

public:
    unsigned long long Get(); // nano precision
    void Sleep( unsigned long long _ns );

public:
    class TemperObj;
    TemperObj Temper( const unsigned long long _frameRate, std::function< void( const int _consumptionPercent ) > && _fnConsumption, std::function< void() > && _fnReport );

private:
    HMODULE m_hNtDll{ nullptr };
	LPFN_NtQueryTimerResolution m_pQueryResolution{ nullptr };
	LPFN_NtSetTimerResolution m_pSetResolution{ nullptr };
	LPFN_NtWaitForSingleObject m_NtWaitForSingleObject{ nullptr };
    HANDLE m_waitEvent{ nullptr };
};


// --------------
class PrecisionTime::TemperObj
{
public:
    TemperObj( PrecisionTime & _precisionTime, const unsigned long long _frameRate, std::function< void( const int _consumptionPercent ) > && _fnConsumption, std::function< void() > && _fnReport );
    ~TemperObj();

public:
    const auto GetStartTime() const { return m_loopStartTime; }

private:
    PrecisionTime & m_precisionTime;
    const unsigned long long m_loopStartTime;
    const unsigned long long m_frameRate;
    const std::function< void( const int _consumptionPercent ) > m_fnConsumption;
    const std::function< void() > m_fnReport;
};