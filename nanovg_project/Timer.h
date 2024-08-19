#pragma once

#include "Win32.h"


// ----------------
class Timer
{
private:
	Timer();
	~Timer();

public:
	static Timer & GetInstance();

public:
    unsigned long long Get() const; // nano precision
    void Sleep( unsigned long long _ns ) const;

public:
    class FpsContext;
    class TemperContext;
    TemperContext Temper( FpsContext & _fpsContext ) const;

private:
	Win32::Handle m_ntDll;
	Win32::Handle m_waitEvent;
	std::function< long( unsigned long *, unsigned long *, unsigned long * ) > m_queryResolution;
	std::function< long( unsigned long, unsigned char, unsigned long * ) > m_setResolution;
	std::function< long( void *, unsigned char, union _LARGE_INTEGER * ) > m_waitForSingleObject;
};


// ----------------
class Timer::FpsContext
{
    friend TemperContext;

public:
    FpsContext( const unsigned long long _targetFrameRate );
    ~FpsContext() = default;

public:
    void Update();
    double Fps() const { return m_avgFrameRate; }
    double Consumption() const { return m_avgConsumption; }
    bool FrameDropped() const { return m_frameDropped; }

private:
    const unsigned long long m_targetFrameRate;
    unsigned long long m_updateTime;
    unsigned long long m_frameCount{ 0 };
    bool m_frameDropped{ false };
    double m_avgFrameRate{ 0 };
    double m_consumption{ 0 };
    double m_avgConsumption{ 0 };
};


// ----------------
class Timer::TemperContext
{
public:
    TemperContext( FpsContext & _fpsContext );
    ~TemperContext();

private:
    FpsContext & m_fpsContext;
    const unsigned long long m_loopStartTime;
    const unsigned long long m_maxLoopDuration;
};