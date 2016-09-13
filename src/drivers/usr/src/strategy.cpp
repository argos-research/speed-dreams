/***************************************************************************

    file                 : strategy.cpp
    created              : Wed Sep 22 15:32:21 CET 2004
    copyright            : (C) 2004 Bernhard Wymann
    email                : berniw@bluewin.ch
    version              : $Id: strategy.cpp 6065 2015-08-09 16:59:15Z torcs-ng $

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
    Very simple stategy sample implementation.
*/


#include "strategy.h"
#include "globaldefs.h"

const float SimpleStrategy::MAX_FUEL_PER_METER = 0.0008f;	// [kg/m] fuel consumtion.


SimpleStrategy::SimpleStrategy() :
    m_fuelchecked(false),
    m_fuelperlap(0.0f),
    m_lastpitfuel(0.0f),
    m_lastfuel(0.0f),
    m_expectedfuelperlap(0.0f),
    m_fuelsum(0.0f),
    PitDamage(5000),
    pit_damage(0),
    min_damage(0),
    is_pitting(0),
    remainlaps(0),
    pit_reason(0),
    track(NULL),
    m_Driver(NULL)
{
#ifdef SPEED_DREAMS
    teamIndex = 0;
    releasePit = false;
#endif
    strategy = STRATEGY_DESPERATE;
}


SimpleStrategy::~SimpleStrategy()
{
    // Nothing so far.
}

void SimpleStrategy::Init(Driver *driver)
{
  m_Driver = driver;

  m_TireLimitFront = m_Driver->TyreTreadDepthFront();
  m_TireLimitRear = m_Driver->TyreTreadDepthRear();
  m_DegradationPerLap = 0.0;
  m_Laps = 0;
}

// Trivial strategy: fill in as much fuel as required for the whole race, or if the tank is
// too small fill the tank completely.
void SimpleStrategy::setFuelAtRaceStart(tTrack* t, void **carParmHandle, tSituation *s, int index)
{
    // Load and set parameters.
    float fuel = GfParmGetNum(*carParmHandle, BT_SECT_PRIV, BT_ATT_FUELPERLAP, (char*) NULL, t->length*MAX_FUEL_PER_METER);
    m_expectedfuelperlap = fuel;
    float maxfuel = GfParmGetNum(*carParmHandle, SECT_CAR, PRM_TANK, (char*) NULL, 100.0f);
    fuel *= (s->_totLaps + 1.0f);
    float ifuel = GfParmGetNum(*carParmHandle, SECT_PRIVATE, PRV_MAX_FUEL, (char *)NULL, 0.0f);
    m_fuelperlap = GfParmGetNum(*carParmHandle, SECT_PRIVATE, PRV_FUEL_PER_LAP, (char *)NULL, 0.0f);
    if (ifuel)
        fuel = ifuel;
    m_lastfuel = MIN(fuel, maxfuel);
    GfParmSetNum(*carParmHandle, SECT_CAR, PRM_FUEL, (char*) NULL, m_lastfuel);
    PitDamage = (int) GfParmGetNum(*carParmHandle, SECT_PRIVATE, PRV_PIT_DAMAGE, (char *)NULL, 5000.0f);
}


void SimpleStrategy::update(tCarElt* car, tSituation *s)
{
    // Fuel statistics update.
    int id = car->_trkPos.seg->id;
    // Range must include enough segments to be executed once guaranteed.
    if (id >= 0 && id < 5 && !m_fuelchecked)
    {
        if (car->race.laps > 1)
        {
            m_fuelperlap = MAX(m_fuelperlap, (m_lastfuel+m_lastpitfuel-car->priv.fuel));
            m_fuelsum += (m_lastfuel+m_lastpitfuel-car->priv.fuel);
        }
        m_lastfuel = car->priv.fuel;
        m_lastpitfuel = 0.0;
        m_fuelchecked = true;
    } else if (id > 5)
    {
        m_fuelchecked = false;
    }
}

int SimpleStrategy::calcRepair(tCarElt* car, tSituation *s, Opponents *opp, int inpit)
{
    // find out what our lead over next car is.
    float lead = FLT_MAX;
    int pos = 1000;
    Opponent *O = NULL;
    tCarElt *Ocar = NULL;
    //int dammage = MIN(car->_dammage, PIT_DAMMAGE);

    // which car is behind me, not including team members?
    //if (car->_dammage < PIT_DAMMAGE)
    {
        if (car->_state == RM_CAR_STATE_PIT && pit_damage)
        {
            if (car->_remainingLaps-car->_lapsBehindLeader > 40)
                return car->_dammage;
            return MIN(car->_dammage, pit_damage);
        }

        for (int i = 0; i < opp->getNOpponents(); i++)
        {
            Opponent *o = opp->getOpponentPtr() + i;
            tCarElt *ocar = o->getCarPtr();
            if (o->getTeam() == TEAM_FRIEND)
                continue;
            if (ocar->_state >= RM_CAR_STATE_PIT) continue;

            if (ocar->_pos < pos && ocar->_pos > car->_pos)
            {
                if (inpit)
                {
                    float mytime = (float)((car->_distFromStartLine / track->length) * car->_lastLapTime + (car->_laps - ocar->_laps) * car->_bestLapTime);
                    float othertime = float((ocar->_distFromStartLine / track->length) * ocar->_bestLapTime);
                    lead = mytime - othertime;
                    if (lead < 25.0)
                        // lets accept that this car is past us & calculate vs the next one
                        continue;
                }

                // base damage repair off this car unless we find a better one
                O = o;
                Ocar = ocar;
                pos = ocar->_pos;
            }
        }
        if (O)
        {
            // how far behind is it?
            float mytime = float((car->_distFromStartLine / track->length) * car->_lastLapTime + (car->_laps - Ocar->_laps) * car->_bestLapTime);
            float othertime = float((Ocar->_distFromStartLine / track->length) * Ocar->_bestLapTime);

            lead = mytime - othertime;

            // how much damage is it safe to fix?
            int safe_damage = 0;
            if (car->_state == RM_CAR_STATE_PIT)
                lead -= float(15.0 + ((track->pits.len * track->pits.nPitSeg) / 20.0) * 0.30);
            else
                lead -= float(15.0 + (track->pits.len * track->pits.nPitSeg) / 20.0);

            if (pit_reason == REASON_NONE)
                lead -= 20.0f;

            if (lead > 10.0f)
                safe_damage = (int) (lead / 0.007);

            if (pit_reason == REASON_DAMAGE)
            {
                if (car->_remainingLaps-car->_lapsBehindLeader > 40)
                    safe_damage = car->_dammage;
                else
                    safe_damage = MIN(car->_dammage, safe_damage);
            }

            return MIN(car->_dammage, safe_damage);
        }
        else
            return car->_dammage;
    }
}

bool SimpleStrategy::needPitstop(tCarElt* car, tSituation *s, Opponents *opp)
{
    // Do we need to refuel?
    int remainlaps = car->_remainingLaps;//-car->_lapsBehindLeader;
    //int this_pit_dammage = PitDamage;

    if (!car->_pit)
        return false;

    int forcepit = (int) GfParmGetNum( car->_carHandle, SECT_PRIVATE, PRV_FORCE_PIT, (char *)NULL, 0.0 );
    if (forcepit)
        return true;

#ifdef SPEED_DREAMS
    int repairWanted = 10000;

    if ((remainlaps > 0) && (remainlaps < 20))
    {
        repairWanted = MIN(8000, PitDamage + (20-remainlaps)*200);
    }

    if (car->_dammage < 9000 && (remainlaps <= 2 || strategy == STRATEGY_DESPERATE))
        repairWanted = 0;

    if (car->_dammage < MIN(3000, PitDamage/2))
        repairWanted = 0;

    float cmpfuel = (m_fuelperlap == 0.0) ? m_expectedfuelperlap : m_fuelperlap;

    float fuelPerM = cmpfuel / track->length;
    bool GotoPit = RtTeamNeedPitStop(teamIndex,fuelPerM,repairWanted);

    if (m_Driver->HasTYC)
    {
      double TdF = m_Driver->TyreTreadDepthFront(); // Check tyre condition
      double TdR = m_Driver->TyreTreadDepthRear();  // Pit stop needed if
      m_DegradationPerLap = (m_Laps * m_DegradationPerLap
        + MAX(m_TireLimitFront - TdF, m_TireLimitRear - TdR));
      m_DegradationPerLap /= ++m_Laps;

      if (MIN(TdF,TdR) < 1.5 * m_DegradationPerLap) // tyres become critical
      {
          /*LogUSR.warning("Tyre condition D: %.1f%% F: %.1f%% R: %.1f%%\n",
          m_DegradationPerLap, TdF, TdR);*/

        if ((TdF < 1.1 * m_DegradationPerLap)
          || (TdR < 1.1 * m_DegradationPerLap))
        {
          GotoPit = true;                           //   to stop in pit
        }
      }

      m_TireLimitFront = TdF;
      m_TireLimitRear = TdR;
    }

    if (GotoPit)
        is_pitting = 1;
    else
        is_pitting = 0;

    return GotoPit;
#else
    if (remainlaps > 0)
    {
        float cmpfuel = (m_fuelperlap == 0.0) ? m_expectedfuelperlap : m_fuelperlap;
        if (car->_fuel < 2.5*cmpfuel &&
                car->_fuel < remainlaps*cmpfuel)
        {
            is_pitting = 1;
            pit_reason = REASON_FUEL;
            return true;
        }
        else if (remainlaps < 20)
            this_pit_dammage = MIN(8000, PitDamage + (20-remainlaps)*200);
    }

    if (isPitFree(car))
    {
        // don't pit for damage if getting close to end
        if (car->_dammage < MAX(PitDamage/2, 9500 - remainlaps*1000))
        {
            is_pitting = 0;
            return false;
        }

        // Ok, otherwise do we need to repair?
        if (car->_dammage >= PitDamage)
        {
            is_pitting = 1;
            pit_reason = REASON_DAMAGE;
            return true;
        }

        // Can we safely repair a lesser amount of damage?
        int canrepair_damage;
        pit_reason = REASON_NONE;

        if ((canrepair_damage = calcRepair(car, s, opp, 0)) >= PitDamage/2)
        {
            if (car->_pos < 6)
            {
                // if there's a chance of overtaking an opponent that's
                // not far in front, avoid going in to fix optional damage.
                for (int i = 0; i < opp->getNOpponents(); i++)
                {
                    Opponent *o = opp->getOpponentPtr() + i;
                    tCarElt *ocar = o->getCarPtr();
                    if (ocar->_pos >= car->_pos) continue;
                    if (o->getTeam() == TEAM_FRIEND) continue;

                    if (o->getDistance() < 200.0 && car->_dammage < ocar->_dammage + 1500 && car->_dammage < PitDamage)
                    {
                        // close behind opponent, so lets not pit for damage purposes
                        return false;
                    }
                }
            }

            if (is_pitting)
                pit_damage = MIN(car->_dammage, MAX(pit_damage, canrepair_damage));
            else
                pit_damage = MIN(car->_dammage, MIN(pit_damage, canrepair_damage));
            is_pitting = 1;
            pit_reason = REASON_DAMAGE;
            return true;
        }
    }

    is_pitting = 0;
    return false;
#endif
}


bool SimpleStrategy::isPitFree(tCarElt* car)
{
#ifdef SPEED_DREAMS
    bool IsFree = RtTeamIsPitFree(teamIndex);
    if (IsFree)
        GfOut("#%s pit is free (%d)\n",car->_name,teamIndex);
    else
        GfOut("#%s pit is locked (%d)\n",car->_name,teamIndex);
    return IsFree;
#else
    if (car->_pit != NULL) {
        if (car->_pit->pitCarIndex == TR_PIT_STATE_FREE) {
            return true;
        }
    }
    return false;
#endif
}


float SimpleStrategy::pitRefuel(tCarElt* car, tSituation *s)
{
    float fuel;
    float cmpfuel = (m_fuelperlap == 0.0f) ? m_expectedfuelperlap : m_fuelperlap;
    fuel = MAX(MIN((car->_remainingLaps + 1.0f)*cmpfuel - car->_fuel,
                   car->_tank - car->_fuel),
               0.0f);
    float maxfuel = GfParmGetNum(car->_carHandle, "private", "MaxFuel", (char *)NULL, 0.0);
    if (maxfuel)
        fuel = maxfuel;
    m_lastpitfuel = fuel;
    return fuel;
}


int SimpleStrategy::pitRepair(tCarElt* car, tSituation *s)
{
    return car->_dammage;
}


void SimpleStrategy2::update(tCarElt* car, tSituation *s)
{
    // Fuel statistics update.
    int id = car->_trkPos.seg->id;
    // Range must include enough segments to be executed once guaranteed.
    if (id >= 0 && id < 5 && !m_fuelchecked)
    {
        if (car->race.laps > 1)
        {
            //m_fuelperlap = MAX(m_fuelperlap, (m_lastfuel + m_lastpitfuel - car->priv.fuel));
            m_fuelsum += (m_lastfuel + m_lastpitfuel - car->priv.fuel);
            m_fuelperlap = (m_fuelsum/(car->race.laps - 1));
            // This is here for adding strategy decisions, otherwise it could be moved to pitRefuel
            // for efficiency.
            updateFuelStrategy(car, s);
        }
        m_lastfuel = car->priv.fuel;
        m_lastpitfuel = 0.0;
        m_fuelchecked = true;
    } else if (id > 5)
    {
        m_fuelchecked = false;
    }

#ifdef SPEED_DREAMS
    if (releasePit)
        RtTeamReleasePit(teamIndex);
    releasePit = false;
#endif
}


void SimpleStrategy2::setFuelAtRaceStart(tTrack* t, void **carParmHandle, tSituation *s, int index)
{
    // Load and set parameters.
    float consfactor = GfParmGetNum(*carParmHandle, SECT_CAR, PRM_FUELCONS, (char*) NULL, 1.0f);
    float cons2 = GfParmGetNum(*carParmHandle, BT_SECT_PRIV, "FuelCons", (char*) NULL, 1.0f);
    float fuel = GfParmGetNum(*carParmHandle, BT_SECT_PRIV, BT_ATT_FUELPERLAP, (char*) NULL, t->length*MAX_FUEL_PER_METER*consfactor*cons2);
    m_expectedfuelperlap = fuel;
    // Pittime is pittime without refuel.
    m_pittime = GfParmGetNum(*carParmHandle, BT_SECT_PRIV, BT_ATT_PITTIME, (char*) NULL, 25.0f);
    m_bestlap = GfParmGetNum(*carParmHandle, BT_SECT_PRIV, BT_ATT_BESTLAP, (char*) NULL, 87.0f);
    m_worstlap = GfParmGetNum(*carParmHandle, BT_SECT_PRIV, BT_ATT_WORSTLAP, (char*) NULL, 87.0f);
    float maxfuel = GfParmGetNum(*carParmHandle, SECT_CAR, PRM_TANK, (char*) NULL, 100.0f);
    PitDamage = (int) GfParmGetNum(*carParmHandle, "private", "PitDamage", (char *)NULL, 5000.0f);

    // Fuel for the whole race.
    float fuelforrace = (s->_totLaps + 1.0f)*fuel;
    // Estimate minimum number of pit stops, -1 because the tank can be filled at the start.
    int pitstopmin = int(ceil(fuelforrace/maxfuel) - 1.0f);
    // Compute race times for min to min + 9 pit stops.
    int i;
    float mintime = FLT_MAX;
    int beststops = pitstopmin;
    m_lastfuel = maxfuel;
    for (i = 0; i < 10; i++)
    {
        float stintfuel = fuelforrace/(pitstopmin + i + 1);
        float fillratio = stintfuel/maxfuel;
        float avglapest = m_bestlap + (m_worstlap - m_bestlap)*fillratio;
        float racetime = (pitstopmin + i)*(m_pittime + stintfuel/8.0f) + s->_totLaps*avglapest;

        if (mintime > racetime)
        {
            mintime = racetime;
            beststops = i + pitstopmin;
            m_lastfuel = stintfuel;
            m_fuelperstint = stintfuel;
        }
    }

    m_remainingstops = beststops;

    fuel = m_lastfuel + m_expectedfuelperlap;
    float ifuel = GfParmGetNum(*carParmHandle, "private", "MaxFuel", (char *)NULL, 0.0f);
    if (ifuel)
        fuel = ifuel;
    ifuel = GfParmGetNum(*carParmHandle, "private", "InitFuel", (char *)NULL, 0.0f);
    if (ifuel)
        fuel = ifuel;
    // Add fuel dependent on index to avoid fuel stop in the same lap.
    GfParmSetNum(*carParmHandle, SECT_CAR, PRM_FUEL, (char*) NULL, fuel);
}


void SimpleStrategy2::updateFuelStrategy(tCarElt* car, tSituation *s)
{
    // Required additional fuel for the rest of the race. +1 because the computation happens right after
    // crossing the start line.
    float requiredfuel = ((car->_remainingLaps + 1) - ceil(car->_fuel/m_fuelperlap))*m_fuelperlap;
    if (requiredfuel < 0.0f)
    {
        // We have enough fuel to end the race, no further stop required.
        return;
    }

    // Estimate minimum number of minimum remaining pit stops.
    int pitstopmin = int(ceil(requiredfuel/car->_tank));
    if (pitstopmin < 1)
    {
        // Should never come here becuase of the above test, leave it anyway.
        return;
    }

    // Compute race times for min to min + 8 pit stops.
    int i;
    float mintime = FLT_MAX;
    int beststops = pitstopmin;
    for (i = 0; i < 9; i++)
    {
        float stintfuel = requiredfuel/(pitstopmin + i);
        float fillratio = stintfuel/car->_tank;
        float avglapest = m_bestlap + (m_worstlap - m_bestlap)*fillratio;
        float racetime = (pitstopmin + i)*(m_pittime + stintfuel/8.0f) + car->_remainingLaps*avglapest;
        if (mintime > racetime)
        {
            mintime = racetime;
            beststops = i + pitstopmin;
            m_fuelperstint = stintfuel;
        }
    }

    m_remainingstops = beststops;
}


SimpleStrategy2::~SimpleStrategy2()
{
    // Nothing so far.
}


float SimpleStrategy2::pitRefuel(tCarElt* car, tSituation *s)
{
    float fuel;
#if 0
    if (m_remainingstops > 1)
    {
        fuel = MIN(m_fuelperstint, car->_tank - car->_fuel);
        m_remainingstops--;
    }
    else
#endif
    {
        float cmpfuel = (m_fuelperlap == 0.0f) ? m_expectedfuelperlap : m_fuelperlap;
        fuel = MAX(MIN((car->_remainingLaps + 1.0f)*cmpfuel - car->_fuel,
                       car->_tank - car->_fuel),
                   0.0f);
    }

    float maxfuel = GfParmGetNum(car->_carHandle, "private", "MaxFuel", (char *)NULL, 0.0);
    if (maxfuel)
        fuel = maxfuel;
    m_lastpitfuel = fuel;

#ifdef SPEED_DREAMS
    releasePit = true;
#endif

    return fuel;
}
