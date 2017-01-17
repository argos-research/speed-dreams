/***************************************************************************

    file                 : strategy.h
    created              : Wed Sep 22 15:31:51 CET 2004
    copyright            : (C) 2004 Bernhard Wymann
    email                : berniw@bluewin.ch
    version              : $Id: strategy.h 6009 2015-05-17 19:54:37Z torcs-ng $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

/*
    Pit strategy for drivers. It defines an abstract base class, such that one can easily plug in
    different strategies.
*/

#ifndef _STRATEGY_H_
#define _STRATEGY_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <tgf.h>
#include <track.h>
#include <car.h>
#include <raceman.h>
#include <robottools.h>
#include <robot.h>

#ifndef SPEED_DREAMS
#define SPEED_DREAMS
#endif //SPEED_DREAMS

#include <teammanager.h>

#include "driver.h"

enum { STRATEGY_DESPERATE=0, STRATEGY_NORMAL, STRATEGY_CAREFUL, STRATEGY_PASSIVE };
enum { REASON_NONE=0, REASON_DAMAGE=1, REASON_FUEL=2 };
class Opponents;

class AbstractStrategy
{
public:
    int teamIndex;
    bool releasePit;

    // Need this empty constructor... do not remove.
    virtual ~AbstractStrategy() {}
    // Set Initial fuel at race start.
    virtual void setFuelAtRaceStart(tTrack* t, void **carParmHandle, tSituation *s, int index) = 0;

    inline void setTeamIndex(int theTeamIndex) { teamIndex = theTeamIndex; }

    // Update internal data at every timestep.
    virtual void update(tCarElt* car, tSituation *s) = 0;
    // Do we need a pit stop? Can be called less frequently.
    virtual bool needPitstop(tCarElt* car, tSituation *s, Opponents *opp) = 0;
    // How much to refuel at pit stop.
    virtual float pitRefuel(tCarElt* car, tSituation *s) = 0;
    // How much repair at pit stop.
    virtual int pitRepair(tCarElt* car, tSituation *s) = 0;
    // Pit Free?
    virtual bool isPitFree(tCarElt* car) = 0;
    virtual void setTrack(tTrack *thetrack) = 0;

    double  m_DistToSwitch;                        // Dist to Pit
    double  m_StartFuel;                           // Fuel at start
    double  m_TireLimitFront;
    double  m_TireLimitRear;
    double  m_DegradationPerLap;
    int     m_Laps;

protected:
    int strategy;
};

class SimpleStrategy : public AbstractStrategy
{
public:
    SimpleStrategy();
    ~SimpleStrategy();

    void    setFuelAtRaceStart(tTrack* t, void **carParmHandle, tSituation *s, int index);
    void    Init(Driver *driver);
    void    update(tCarElt* car, tSituation *s);
    bool    needPitstop(tCarElt* car, tSituation *s, Opponents *opp);
    float   pitRefuel(tCarElt* car, tSituation *s);
    int     pitRepair(tCarElt* car, tSituation *s);
    bool    isPitFree(tCarElt* car);
    void    setTrack(tTrack *thetrack) { track = thetrack; }

protected:
    int     calcRepair(tCarElt *car, tSituation *s, Opponents *opp, int inpit);
    bool    m_fuelchecked;			// Fuel statistics updated.
    float   m_fuelperlap;           // The maximum amount of fuel we needed for a lap.
    float   m_lastpitfuel;          // Amount refueled, special case when we refuel.
    float   m_lastfuel;				// the fuel available when we cross the start lane.
    float   m_expectedfuelperlap;	// Expected fuel per lap (may be very inaccurate).
    float   m_fuelsum;				// all the fuel used.
    int     PitDamage;

    static const float MAX_FUEL_PER_METER;	// [kg/m] fuel consumtion.
    int     pit_damage;
    int     min_damage;
    int     is_pitting;
    int     remainlaps;
    int     pit_reason;
    tTrack  *track;
    Driver  *m_Driver;

};


class SimpleStrategy2 : public SimpleStrategy
{
public:
    ~SimpleStrategy2();
    void setFuelAtRaceStart(tTrack* t, void **carParmHandle, tSituation *s, int index);
    float pitRefuel(tCarElt* car, tSituation *s);
    void update(tCarElt* car, tSituation *s);


protected:
    int     m_remainingstops;
    float   m_fuelperstint;
    float   m_pittime;				// Expected additional time for pit stop.
    float   m_bestlap;				// Best possible lap, empty tank and alone.
    float   m_worstlap;				// Worst possible lap, full tank and alone.

    virtual void updateFuelStrategy(tCarElt* car, tSituation *s);
};


#endif // _STRATEGY_H_
