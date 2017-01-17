/***************************************************************************

    file                 : rttimeanalysis.cpp
    created              : Sun Jun 07 11:15:00 CET 2009
    copyright            : (C) 2009 by Wolf-Dieter Beelitz
    email                : wdbee@users.sourceforge.net
    version              : 

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

/** @file	
    This is a collection of useful functions for time analysis

    @author	<a href=mailto:wdbee@users.sourceforge.net>Wolf-Dieter Beelitz</a>
    @version	
    @ingroup	robottools
*/

/** @defgroup timeanalysis		Time analysis for robots.
    @ingroup	robottools
*/
    
#include "timeanalysis.h"

// Private variables
static double RtTicksPerSec = 1000.0;			// Ticks per second

// Public functions

// Init Timer
bool RtInitTimer()
{
#ifdef WIN32
  ULONGLONG TicksPerSec;
  if (!QueryPerformanceFrequency((LARGE_INTEGER*)&TicksPerSec)) 
  {
	  //printf("\n\n\nPerformance Counter not found\n\n\n"); 
	  return false;
  }
  else
  {
	  RtTicksPerSec = (double) TicksPerSec;
	  //printf("\n\n\nFrequency for Performance Counter: %g GHz (= 1 / %u)\n",(1000000000.0/RtTicksPerSec),TicksPerSec); 
	  //printf("Resolution for Performance Counter: %g nanosec\n\n\n",TicksPerSec/1000000000.0); 
	  return true;
  }
#else
  return false;
#endif
}

// Get timer frequency [Hz]
double RtTimerFrequency()
{
	return 1.0 / RtTicksPerSec;
}

// Get timestamp [msec]
double RtTimeStamp()
{
#ifdef WIN32
	ULONGLONG TickCount; 
	DWORD_PTR oldmask = ::SetThreadAffinityMask(::GetCurrentThread(), 0);
	QueryPerformanceCounter((LARGE_INTEGER*)&TickCount); 
	::SetThreadAffinityMask(::GetCurrentThread(), oldmask);
	return (1000.0 * TickCount)/RtTicksPerSec;
#else
	struct timeval tv;
	gettimeofday(&tv, 0); 
	return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
#endif
}

// Get Duration [msec]
// ATTENTION:
// With Linux the duration has to be smaller than 60 sec!
double RtDuration(double StartTimeStamp)
{
	return RtTimeStamp() - StartTimeStamp;
}
