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
#include <chrono>

namespace gamepause
{

class TimeMeasurement
{

    typedef std::chrono::high_resolution_clock Clock;

public:
    TimeMeasurement();
    ~TimeMeasurement();
    void TimedRacePause();
    void TimedRaceResume();
    void maxcalc();
    void mincalc();
    void avgcalc();
    void durationtotal();

    uint64_t getStopcounter()
    {
        return stopcounter;
    }

    uint64_t getTotalduration()
    {
        return totalduration;
    }

    uint64_t getDuration()
    {
        return duration;
    }

    uint64_t getMincounter()
    {
        return mincounter;
    }

    uint64_t getAvgcounter()
    {
        return avgcounter;
    }

    uint64_t getMaxcounter()
    {
        return maxcounter;
    }

private:
    uint64_t stopcounter;
    uint64_t totalduration;
    uint64_t duration;
    uint64_t mincounter;
    uint64_t avgcounter;
    uint64_t maxcounter;
    std::chrono::time_point<std::chrono::system_clock> startvalue, stopvalue;

} timebox;

void RaceResume();
void RacePause();

} // namespace gamepause
