/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
 
/**
    @defgroup	GamePause.
    Pausing the game engine within a robot.
*/
#include <algorithm> 
#include <chrono>
typedef std::chrono::high_resolution_clock Clock;
uint64_t stopcounter = 0;
uint64_t totalduration = 0;
uint64_t duration = 0;
std::chrono::system_clock::time_point start;
uint64_t mincounter = 0;
uint64_t avgcounter = 0;
uint64_t maxcounter = 0;


extern uint64_t RaceResume(std::chrono::system_clock::time_point start);
extern std::chrono::system_clock::time_point RacePause();