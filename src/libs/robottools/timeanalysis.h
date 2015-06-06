/***************************************************************************

    file                 : timeanalysis.h
    created              : Sun Jun 07 11:15:00 CET 2009
    copyright            : (C) 2009 by Wolf-Dieter Beelitz
    email                : wdb@wdbee.de
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

    @author	<a href=mailto:wdb@wdbee.de>Wolf-Dieter Beelitz</a>
    @version	
    @ingroup	robottools
*/

#ifndef _TIMEANALYSIS_H_
#define _TIMEANALYSIS_H_

#ifdef WIN32
#include <windows.h>
#include <time.h>
#else
#include <sys/time.h>
#endif

#include "robottools.h"

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// Time Analysis
// Robot developer API:
//
ROBOTTOOLS_API bool RtInitTimer();						// Check performance counter hardware
ROBOTTOOLS_API double RtTimerFrequency();				// Get timer frequency in Hz
ROBOTTOOLS_API double RtTimeStamp();					// Get time stamp in msec
ROBOTTOOLS_API double RtDuration(double StartTimeStamp);// Calculate duration between time stamps
//
// End of robot developer API
//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
#endif /* _TIMEANALYSIS_H_ */ 
