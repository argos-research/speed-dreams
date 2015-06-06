/***************************************************************************

    file        : grMoonpos.cpp
    copyright   : (C) 2012 by Xavier Bertaux (based on simgear code)
    web         : http://www.speed-dreams.org
    version     : $Id: grSphere.h 3162 2012-07-05 13:11:22Z torcs-ng $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <ctime>
#include "grMoonpos.h"

// Position of the Moon first month 2012
static const double MoonPositionDay[31] = { 40020, 41400, 42960, 44760, 46980, 49680, 52860, 56520, 60600, 64860, 69300, 73740,
										78240, 82800, 0, 960, 5580, 10080, 14400, 18180, 21360, 23940, 26040, 27780, 29220,
										30540, 31800, 33060, 34380, 35880, 37560 };

double grUpdateMoonPos(double timeOfDay)
{
	double moonpos, actual = 0;
	time_t Time;
	struct tm *Date;
	
	time(&Time);
	Date = localtime(&Time);

	int hour = Date->tm_hour;
	int minute = Date->tm_min;
	
	int day = Date->tm_mday;
	int month = Date->tm_mon +1;
	int year = Date->tm_year + 1900;

	year = (2012 - year) +1;
	double diff = 420 * month * year;
	if (diff > 86340)
		diff = diff - 86340;

	actual = (hour*3600)+(minute*60);
	moonpos = (MoonPositionDay[day+1] - diff);
	if (actual > moonpos)
		moonpos = actual - moonpos;
	else
		moonpos = moonpos - actual;

	return moonpos;
	
}



