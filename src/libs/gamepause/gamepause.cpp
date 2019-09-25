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


TimeMeasurement::TimeMeasurement()
{
    stopcounter,avgcounter,mincounter,maxcounter,duration,totalduration = 0;
    
}

TimeMeasurement::~TimeMeasurement()
{

}





void
TimeMeasurement::TimedRaceResume()
{

        //resetting the duration
        duration = 0;

        RaceResume();

         //Taking the stopvalue time
        stopvalue = std::chrono::system_clock::now();


        //calculating duration between the given startvalue and the taken stopvalue time
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(stopvalue - startvalue).count();
        

        //adding of the actual calculated duration to the total amount of the game being paused
        totalduration += duration;


        //updating the stopcounter, which indicates how often the game was stopvalueped.
        stopcounter++;


}//Resuming the race for the robots

void
TimeMeasurement::TimedRacePause()
{
        //Taking the startvalue time
        startvalue = std::chrono::system_clock::now();

        RacePause();

}//Pausing the race for the robots

void
TimeMeasurement::maxcalc()
{

    //calculating, returning and writing the maximum value to the globally available variable
    maxcounter = std::max(maxcounter,duration);


}//calculating maximum duration time taken for one step


void
TimeMeasurement::mincalc()
{

    //if it is the first simulation step set minval as totest
    //otherwise minval will always be zero
    if (stopcounter == 1) mincounter = duration;

    //calculating and writing the minimum value to the globally available variable
    mincounter = std::min(mincounter,duration);


}//calculating minimum duration time taken for one step

void
TimeMeasurement::avgcalc()
{
    //calculating, returning and writing the average value to the globally available variable
    avgcounter = totalduration / stopcounter;

}//calculating average duration time taken all previous steps and durations



void
RaceResume()
{

        //see whether the game is started with a gui and sound and resume the sound
        if (LegacyMenu::self().soundEngine())
            LegacyMenu::self().soundEngine()->mute(false);


        //Resuming the RaceEngine
		LmRaceEngine().start();
}

void
RacePause()
{ 
        //see whether the game is started with a gui and sound and resume the sound
		if (LegacyMenu::self().soundEngine())
			LegacyMenu::self().soundEngine()->mute(true);

        //stopvalueping the RaceEngine
		LmRaceEngine().stop();
}

}
