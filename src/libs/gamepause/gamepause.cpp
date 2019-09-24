/***************************************************************************

    created              : Sat Mar 18 23:16:38 CET 2000
    copyright            : (C) 2000 by Eric Espie
    email                : torcs@free.fr
    version              : $Id: human.cpp 5522 2013-06-17 21:03:25Z torcs-ng $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "legacymenu.h"
#include "gamepause.h"



uint64_t
RaceResume(std::chrono::time_point<std::chrono::system_clock> start)
{

        duration = 0;
        if (LegacyMenu::self().soundEngine())
            LegacyMenu::self().soundEngine()->mute(false);

		LmRaceEngine().start();
         //Taking the time
        stop = std::chrono::system_clock::now();
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
        totalduration += duration;
        stopcounter++;
        return duration;
}

std::chrono::time_point<std::chrono::system_clock>
RacePause()
{
        
        start = std::chrono::system_clock::now();
		if (LegacyMenu::self().soundEngine())
			LegacyMenu::self().soundEngine()->mute(true);

		LmRaceEngine().stop();
        return start;

}


uint64_t maxcalc(uint64_t maxval, uint64_t totest)
{
    return maxcounter = std::max(maxval,totest);
}

uint64_t mincalc(uint64_t minval, uint64_t totest)
{
    if (stopcounter == 1) minval = totest;
    return mincounter = std::min(minval,totest);
}

uint64_t avgcalc(uint64_t totduration, uint64_t counter)
{
    return avgcounter = totduration / counter;
}
