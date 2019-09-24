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
RaceResume(std::chrono::system_clock::time_point start)
{

       
        if (LegacyMenu::self().soundEngine())
            LegacyMenu::self().soundEngine()->mute(false);

		LmRaceEngine().start();
         //Taking the time
        std::chrono::system_clock::time_point stop = Clock::now();
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
        stopcounter++;
        return duration;
}

std::chrono::system_clock::time_point
RacePause()
{
        
        std::chrono::system_clock::time_point start = Clock::now();
		if (LegacyMenu::self().soundEngine())
			LegacyMenu::self().soundEngine()->mute(true);

		LmRaceEngine().stop();
        return start;

}
