/***************************************************************************

    file                 : pit.h
    created              : Thu Mai 15 2:41:00 CET 2003
    copyright            : (C) 2003-2004 by Bernhard Wymann
    email                : berniw@bluewin.ch
    version              : $Id: pit.h 5950 2015-04-05 19:34:04Z torcs-ng $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _PIT_H_
#define _PIT_H_

#include "driver.h"
#include "spline.h"

enum { PIT_NONE=0, PIT_MID, PIT_FRONT, PIT_BACK };

class Driver;

class Pit
{
public:
    Pit(tSituation *s, Driver *driver, float PitOffset);
    ~Pit();

    void setPitstop(bool pitstop);
    bool getPitstop() { return pitstop; }
    void needPitstop(bool npitstop) { needpitstop = npitstop; }
    bool needPitstop() { return needpitstop; }

    void setInPit(bool inpitlane) { this->inpitlane = inpitlane; }
    bool getInPit() { return inpitlane; }

    float getPitOffset(float offset, float fromstart, int which);

    bool isBetween(float fromstart, int pitonly);
    bool isTimeout(float distance);

    float getNPitStart() { return pMID[1].x; }
    float getNPitLoc(int which) { return (which==PIT_MID ? pMID[3].x : (which==PIT_BACK ? pBACK[3].x : pFRONT[3].x)); }
    float getNPitEnd() { return pMID[5].x; }
    float getNPitEntry() { return pMID[0].x; }

    float toSplineCoord(float x);

    float getSpeedlimitSqr() { return speedlimitsqr; }
    float getSpeedlimit() { return speedlimit; }
    float getSpeedLimitBrake(float speedsqr);

    void update();

private:
    tTrack *track;
    tCarElt *car;
    tTrackOwnPit *mypit;			// Pointer to my pit.
    tTrackPitInfo *pitinfo;			// General pit info.

    enum { NPOINTS = 7 };
    SplinePoint pMID[NPOINTS];			// Spline points.
    SplinePoint pFRONT[NPOINTS];			// Spline points.
    SplinePoint pBACK[NPOINTS];			// Spline points.
    Spline *splineMID;					// Spline.
    Spline *splineFRONT;					// Spline.
    Spline *splineBACK;					// Spline.

    bool needpitstop;				// Pitstop planned.
    bool pitstop;					// Pitstop planned.
    bool inpitlane;					// We are still in the pit lane.
    float pitentry;					// Distance to start line of the pit entry.
    float pitexit;					// Distance to the start line of the pit exit.
    float pitstart;
    float pitend;

    float speedlimitsqr;			// Pit speed limit squared.
    float speedlimit;				// Pit speed limit.
    float pitspeedlimitsqr;			// The original speedlimit squared.

    float pittimer;					// Timer for pit timeouts.
    int side;

    static const float SPEED_LIMIT_MARGIN;
};

#endif // _PIT_H_
