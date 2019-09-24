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
std::chrono::time_point<std::chrono::system_clock> start, stop;
uint64_t mincounter = 0;
uint64_t avgcounter = 0;
uint64_t maxcounter = 0;


extern uint64_t RaceResume(std::chrono::time_point<std::chrono::system_clock> start);
extern std::chrono::time_point<std::chrono::system_clock> RacePause();
extern uint64_t maxcalc(uint64_t maxcounter, uint64_t duration);
extern uint64_t mincalc(uint64_t mincounter, uint64_t duration);
extern uint64_t avgcalc(uint64_t totalduration, uint64_t stopcounter);
extern uint64_t durationtotal(uint64_t duration);