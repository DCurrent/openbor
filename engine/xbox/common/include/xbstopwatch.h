//-----------------------------------------------------------------------------
// File: XbStopWatch.h
//
// Desc: StopWatch object using QueryPerformanceCounter
//
// Hist: 01.19.01 - New for March XDK release
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef XBSTOPWATCH_H
#define XBSTOPWATCH_H

#include <xtl.h>




//-----------------------------------------------------------------------------
// Name: class CXBStopWatch
// Desc: Simple timing object using stopwatch metaphor
//-----------------------------------------------------------------------------
class CXBStopWatch
{
	FLOAT    m_fTimerPeriod;        // seconds per tick (1/Hz)
	LONGLONG m_nStartTick;          // time watch last started/reset
	LONGLONG m_nPrevElapsedTicks;   // time watch was previously running
	BOOL     m_bIsRunning;          // TRUE if watch is running

public:

	explicit CXBStopWatch( BOOL bStartWatch = TRUE );

	VOID  Start();
	VOID  StartZero();
	VOID  Stop();
	VOID  Reset();

	BOOL  IsRunning() const;
	FLOAT GetElapsedSeconds() const;
	FLOAT GetElapsedMilliseconds() const;

private:

	LONGLONG GetTicks() const;

};

#endif // XBSTOPWATCH_H
