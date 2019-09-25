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
#include <algorithm>
#include <chrono>

namespace gamepause{

uint64_t
RaceResume(std::chrono::time_point<std::chrono::system_clock> startvalue)
{

        //resetting the duration
        duration = 0;


        //see whether the game is started with a gui and sound and resume the sound
        if (LegacyMenu::self().soundEngine())
            LegacyMenu::self().soundEngine()->mute(false);


        //Resuming the RaceEngine
		LmRaceEngine().start();

         //Taking the stopvalue time
        stopvalue = std::chrono::system_clock::now();


        //calculating duration between the given startvalue and the taken stopvalue time
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(stopvalue - startvalue).count();
        

        //adding of the actual calculated duration to the total amount of the game being paused
        totalduration += duration;


        //updating the stopcounter, which indicates how often the game was stopvalueped.
        stopcounter++;


        //return value of duration, as this is also globally available it does not have to be caught
        return duration;


}//Resuming the race for the robots

std::chrono::time_point<std::chrono::system_clock>
RacePause()
{
        //Taking the startvalue time
        startvalue = std::chrono::system_clock::now();


        //see whether the game is started with a gui and sound and resume the sound
		if (LegacyMenu::self().soundEngine())
			LegacyMenu::self().soundEngine()->mute(true);

        //stopvalueping the RaceEngine
		LmRaceEngine().stop();


        //return value of startvalue, as this is also globally available it does not have to be caught
        return startvalue;

}//Pausing the race for the robots


uint64_t maxcalc(uint64_t maxval, uint64_t totest)
{

    //calculating, returning and writing the maximum value to the globally available variable
    return maxcounter = std::max(maxval,totest);


}//calculating maximum duration time taken for one step



uint64_t mincalc(uint64_t minval, uint64_t totest)
{

    //if it is the first simulation step set minval as totest
    //otherwise minval will always be zero
    if (stopcounter == 1) minval = totest;

    //calculating, returning and writing the minimum value to the globally available variable
    return mincounter = std::min(minval,totest);


}//calculating minimum duration time taken for one step

uint64_t avgcalc(uint64_t totduration, uint64_t counter)
{

    //calculating, returning and writing the average value to the globally available variable
    return avgcounter = totduration / counter;


}//calculating average duration time taken all previous steps and durations


}