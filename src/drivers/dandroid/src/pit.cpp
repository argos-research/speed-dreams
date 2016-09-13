/***************************************************************************

    file                 : pit.cpp
    created              : Thu Aug 31 01:21:49 UTC 2006
    copyright            : (C) 2006 Daniel Schellhammer

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#include "pit.h"


Pit::Pit()
{
}


Pit::~Pit()
{
}


void Pit::init(PTrack t, PSituation s, PtCarElt c, int pitdamage, double pitentrymargin)
{
  if (pitdamage) {
    PIT_DAMAGE = pitdamage;
  } else {
    PIT_DAMAGE = 5000;
  }
  MAX_DAMAGE = 8000;              // [-]
  MAX_DAMAGE_DIST = 50000;        // [m]
  ENTRY_MARGIN = pitentrymargin;  // [m]
  SPEED_LIMIT_MARGIN = 0.1;      // [m/s] savety margin
  track = t;
  car = c;
  teamcar = NULL;
  mypit = car->_pit;
  pitinfo = &track->pits;
  pitstop = inpitlane = false;
  fuelchecked = false;
  maxfuelperlap = 0.0005 * t->length;
  totalfuel = 0.0;
  fuellapscounted = 0;
  avgfuelperlap = 0.0;
  lastpitfuel = 0.0;
  lastfuel = 0.0;
  // Get teammates car
  int i;
  for (i = 0; i < s->_ncars; i++) {
    if (s->cars[i] != car) {
      if (!strncmp(car->_teamname, s->cars[i]->_teamname, 10)) {
        teamcar = s->cars[i];
      }
    }
  }
  if (mypit != NULL) {
    speedlimit = pitinfo->speedLimit - SPEED_LIMIT_MARGIN;
    // compute pit spline points along the track
    p[3].x = mypit->pos.seg->lgfromstart + mypit->pos.toStart;
    p[2].x = p[3].x - pitinfo->len;
    p[4].x = p[3].x + pitinfo->len;
    p[0].x = pitinfo->pitEntry->lgfromstart;
    p[1].x = pitinfo->pitStart->lgfromstart - pitinfo->len;
    p[5].x = pitinfo->pitEnd->lgfromstart + pitinfo->pitEnd->length + pitinfo->len;
    p[6].x = pitinfo->pitExit->lgfromstart + pitinfo->pitExit->length;
    pitentry = p[0].x;
    pitexit = p[6].x;
    limitentry = p[1].x;
    limitexit = p[5].x;
    // normalizing spline segments to <= 0.0
    int i;
    for (i = 0; i < NPOINTS; i++) {
      p[i].s = 0.0;
      p[i].x = toSplineCoord(p[i].x);
    }
    if (p[1].x > p[2].x) p[1].x = p[2].x;
    if (p[4].x > p[5].x) p[5].x = p[4].x;
    double sign = (pitinfo->side == TR_LFT) ? 1.0 : -1.0;
    p[0].y = sign * (track->width / 2.0 - 2.0);
    p[6].y = sign * (track->width / 2.0 - 2.0);
    for (i = 1; i < NPOINTS - 1; i++) {
      p[i].y = fabs(pitinfo->driversPits->pos.toMiddle) - pitinfo->width - 1.0;
      p[i].y *= sign;
    }
    p[3].y = fabs(pitinfo->driversPits->pos.toMiddle) * sign;
    spline.newSpline(NPOINTS, p);
  }
}


// Transforms track coordinates to spline parameter coordinates
double Pit::toSplineCoord(double x)
{
  x -= pitentry;
  while (x < 0.0) {
    x += track->length;
  }
  return x;
}


// computes offset to track middle for trajectory
double Pit::getPitOffset(double fromstart)
{
  if (mypit != NULL) {
    if (getInPit() || (getPitstop() && isBetween(fromstart))) {
      fromstart = toSplineCoord(fromstart);
      return spline.evaluate(fromstart);
    } else if (getPitstop() && isBetween(fromstart + ENTRY_MARGIN)) {
      return p[0].y;
    }
  }
  return 0.0;
}


// Sets the pitstop flag if we are not in the pit range
void Pit::setPitstop(bool pitst)
{
  if (mypit == NULL) return;
  if (!isBetween(car->_distFromStartLine) && !isBetween(car->_distFromStartLine + ENTRY_MARGIN)) {
    if (teamcar != NULL && !(teamcar->_state & RM_CAR_STATE_OUT)) {
      if (teamcar->_raceCmd == RM_CMD_PIT_ASKED || teamcar->_state & RM_CAR_STATE_PIT) {
        return;
      }
    }
    car->_raceCmd = RM_CMD_PIT_ASKED;
    pitstop = pitst;
  } else if (pitst == false) {
    pitstop = pitst;
  }
}

// Check if the argument fromstart is in the range of the pit
bool Pit::isBetween(double fromstart)
{
  if (fromstart > track->length) {
    fromstart -= track->length;
  }
  if (pitentry <= pitexit) {
    if (fromstart >= pitentry && fromstart <= pitexit) {
      return true;
    } else {
      return false;
    }
  } else {
    if ((fromstart >= 0.0 && fromstart <= pitexit) ||
      (fromstart >= pitentry && fromstart <= track->length))
    {
      return true;
    } else {
      return false;
    }
  }
}


// Check if the argument fromstart is in the range of the pit limit
bool Pit::isPitlimit(double fromstart)
{
  if (limitentry <= limitexit) {
    if (fromstart >= limitentry && fromstart <= limitexit) {
      return true;
    } else {
      return false;
    }
  } else {
    if ((fromstart >= 0.0 && fromstart <= limitexit)
    || (fromstart >= limitentry && fromstart <= track->length)) {
      return true;
    } else {
      return false;
    }
  }
}


// update pit data and strategy
void Pit::update()
{
  if (mypit != NULL) {
    int remainingLaps = car->_remainingLaps - car->_lapsBehindLeader;
    if (isBetween(car->_distFromStartLine)) {
      if (getPitstop()) {
        setInPit(true);
      }
    } else {
      setInPit(false);
    }
    // fuel update
    int id = car->_trkPos.seg->id;
    if (id >= 0 && id <= 5 && !fuelchecked) {
      if (car->race.laps > 1) {
        maxfuelperlap = MAX(maxfuelperlap, (lastfuel + lastpitfuel - car->priv.fuel));
        totalfuel += lastfuel + lastpitfuel - car->priv.fuel;
        fuellapscounted++;
        avgfuelperlap = totalfuel / fuellapscounted;
        //GfOut("Car:%s fuelpermeter:%g\n", car->_name, avgfuelperlap / track->length);
      }
      lastfuel = car->priv.fuel;
      lastpitfuel = 0.0;
      fuelchecked = true;
    } else if (id > 5) {
      fuelchecked = false;
    }
    if (!getPitstop() && remainingLaps > 0) {
      // check for damage
      bool pitdamage = false;
      if ((car->_dammage > PIT_DAMAGE && remainingLaps * track->length > MAX_DAMAGE_DIST && lastfuel > 15.0)
      || car->_dammage > MAX_DAMAGE) {
        pitdamage = true;
      }
      double teamcarfuel;
      if (teamcar == NULL || teamcar->_state & RM_CAR_STATE_OUT) {
        teamcarfuel = 0.0;
      } else {
        teamcarfuel = teamcar->_fuel;
        if (teamcarfuel < 2.0 * maxfuelperlap) {
          pitdamage = false;
        }
      }
      if (pitdamage) {
        setPitstop(true);
      }
      double maxpittime = 15.0 + 0.007 * car->_dammage;
      double pitlapdiff = ceil((80.0 * maxpittime + 2000.0) / track->length);
      if (car->_fuel < 1.0 * maxfuelperlap) {
        setPitstop(true);
      } else if (car->_fuel < teamcarfuel
      && teamcarfuel < (1.1 + pitlapdiff) * maxfuelperlap
      && car->_fuel < remainingLaps * maxfuelperlap) {
          setPitstop(true);
      }
    }
  }
}


// Computes the amount of fuel
double Pit::getFuel()
{
  double laps = car->_remainingLaps + (track->length - car->_distFromStartLine) / track->length;
  double fueltoend = (laps - car->_lapsBehindLeader) * avgfuelperlap;
  int pitstops = int(floor(fueltoend / car->_tank));
  double stintfuel = fueltoend / (pitstops + 1) + 2.0;
  if (pitstops && (stintfuel / car->_tank > 0.95)) {
    stintfuel = car->_tank;
  }
  double fuel = MAX(MIN(stintfuel - car->_fuel, car->_tank - car->_fuel), 0.0);
  //GfOut("fromStart:%g laps:%g lapsBehindLeader:%d fueltoend:%g pitstops:%d stintfuel:%g fuel:%g\n", car->_distFromStartLine, laps, car->_lapsBehindLeader, fueltoend, pitstops, stintfuel, fuel);
  return fuel;
}


// Computes how much damage to repair
int Pit::getRepair()
{
  if ((car->_remainingLaps - car->_lapsBehindLeader) * track->length < MAX_DAMAGE_DIST) {
    return (int)(0.2 * car->_dammage);
  }
  return car->_dammage;
}


double Pit::getSpeedlimit()
{
  return speedlimit;
}

double Pit::getSpeedlimit(double fromstart)
{
//  if (car->_trkPos.seg->raceInfo & TR_SPEEDLIMIT) {    // Not yet working
  if (isPitlimit(fromstart)) {
    return speedlimit;
  }
  return DBL_MAX;
}

double Pit::getDist()
{
  if (getPitstop()) {
    float dl, dw;
    RtDistToPit(car, track, &dl, &dw);
    return dl;
  }
  return DBL_MAX;
}

double Pit::getSideDist()
{
  if (getPitstop()) {
    float dl, dw;
    RtDistToPit(car, track, &dl, &dw);
    return dw;
  }
  return DBL_MAX;
}

// callback from driver
void Pit::pitCommand()
{
  car->_pitRepair = getRepair();
  lastpitfuel = getFuel();
  car->_pitFuel = (tdble) lastpitfuel;
  setPitstop(false);
}
