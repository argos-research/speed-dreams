/***************************************************************************

    file                 : pit.cpp
    created              : Thu Mai 15 2:43:00 CET 2003
    copyright            : (C) 2003-2004 by Bernhard Wymann
    email                : berniw@bluewin.ch
    version              : $Id: pit.cpp 6065 2015-08-09 16:59:15Z torcs-ng $

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
#include "globaldefs.h"

const float Pit::SPEED_LIMIT_MARGIN = 0.5;		// [m/s] savety margin to avoid pit speeding.


Pit::Pit(tSituation *s, Driver *driver, float pitoffset)
{
    track = driver->getTrackPtr();
    car = driver->getCarPtr();
    mypit = driver->getCarPtr()->_pit;
    pitinfo = &track->pits;
    pitstop = inpitlane = false;
    pittimer = 0.0;

    if (mypit != NULL)
    {
        speedlimit = pitinfo->speedLimit - SPEED_LIMIT_MARGIN;
        speedlimitsqr = speedlimit*speedlimit;
        pitspeedlimitsqr = pitinfo->speedLimit*pitinfo->speedLimit;

        // Compute pit spline points along the track.
        pMID[3].x = mypit->pos.seg->lgfromstart + mypit->pos.toStart;
        pMID[2].x = pMID[3].x - pitinfo->len;
        pMID[4].x = pMID[3].x + pitinfo->len;
        pMID[0].x = pitinfo->pitEntry->lgfromstart + pitoffset;
        pMID[1].x = pitinfo->pitEntry->lgfromstart + pitinfo->pitEntry->length;
        pMID[5].x = pitinfo->pitStart->lgfromstart + pitinfo->nPitSeg * pitinfo->len; // Use nPitSeg to respect the pit speed limit on Migrants e.a.
        pMID[6].x = pitinfo->pitExit->lgfromstart;

        double PitEndOffset = GfParmGetNum( car->_carHandle, SECT_PRIVATE, PRV_PIT_END_OFFSET, (char *)NULL, 0.0 );
        pMID[6].x += (float)PitEndOffset;

        pitentry = pMID[0].x;
        pitexit = pMID[6].x;
        pitstart = pMID[1].x;
        pitend = pMID[5].x;

        // Normalizing spline segments to >= 0.0.
        int i;
        for (i = 0; i < NPOINTS; i++)
        {
            pMID[i].s = 0.0;
            pMID[i].x = toSplineCoord(pMID[i].x);
        }

        // Fix broken pit exit.
        if (pMID[6].x < pMID[5].x)
        {
            //fprintf(stderr,"bt: Pitexit broken on track %s.\n", track->name);fflush(stderr);
            pMID[6].x = pMID[5].x + 50.0f;
        }

        // Fix point for first pit if necessary.
        if (pMID[1].x > pMID[2].x)
        {
            pMID[1].x = pMID[2].x;
        }

        // Fix point for last pit if necessary.
        if (pMID[4].x > pMID[5].x)
        {
            pMID[5].x = pMID[4].x;
        }

        side = pitinfo->side;
        float sign = (side == TR_LFT) ? 1.0f : -1.0f;
        pMID[0].y = 0.0;
        pMID[6].y = 0.0;

        for (i = 1; i < NPOINTS - 1; i++)
        {
            pMID[i].y = fabs(pitinfo->driversPits->pos.toMiddle) - pitinfo->width;
            pMID[i].y *= sign;
        }

        double PitShift = GfParmGetNum( car->_carHandle, SECT_PRIVATE, "pit shift", (char *)NULL, 0.0 );
        pMID[3].y = (float)((fabs(pitinfo->driversPits->pos.toMiddle)+PitShift+1.0)*sign);
        splineMID = new Spline(NPOINTS, pMID);

        memcpy(pFRONT, pMID, sizeof(pMID));
        memcpy(pBACK, pMID, sizeof(pMID));

        pBACK[3].x -= (float)(car->_dimension_x/2 + 0.1);
        pBACK[2].x -= 1.2f;

        pFRONT[3].x += (float)(car->_dimension_x/2 + 0.1);
        pFRONT[4].x += 1.0f;

        splineFRONT = new Spline(NPOINTS, pFRONT);
        splineBACK = new Spline(NPOINTS, pBACK);
    }
}


Pit::~Pit()
{
    if (mypit != NULL)
    {
        delete splineMID;
        delete splineFRONT;
        delete splineBACK;
    }
}


// Transforms track coordinates to spline parameter coordinates.
float Pit::toSplineCoord(float x)
{
    x -= pitentry;
    while (x < 0.0f)
    {
        x += track->length;
    }

    return x;
}


// Computes offset to track middle for trajectory.
float Pit::getPitOffset(float offset, float fromstart, int which)
{
    if (mypit != NULL)
    {
        if (getInPit() || (getPitstop() && isBetween(fromstart, 0)))
        {
            fromstart = toSplineCoord(fromstart);
            //double newoffset;

            switch (which)
            {
            case PIT_MID:
                return splineMID->evaluate(fromstart);
            case PIT_FRONT:
                return splineFRONT->evaluate(fromstart);
            case PIT_BACK:
                return splineBACK->evaluate(fromstart);
                break;
            }
        }
    }
    return offset;
}


// Sets the pitstop flag if we are not in the pit range.
void Pit::setPitstop(bool pitstop)
{
    if (mypit == NULL)
    {
        return;
    }

    float fromstart = car->_distFromStartLine;

    if (!isBetween(fromstart, 0))
    {
        this->pitstop = pitstop;
    } else if (!pitstop)
    {
        this->pitstop = pitstop;
        pittimer = 0.0f;
    }
}


// Check if the argument fromstart is in the range of the pit.
bool Pit::isBetween(float fromstart, int pitonly)
{
    if (pitonly)
    {
        if (fromstart > pMID[4].x)
            needpitstop = false;

        if (pitstart <= pitend)
        {
            if (fromstart >= pitstart && fromstart <= pitend)
            {
                return true;
            } else
            {
                return false;
            }
        } else {
            // Warning: TORCS reports sometimes negative values for "fromstart"!
            if (fromstart <= pitend || fromstart >= pitstart)
            {
                return true;
            } else
            {
                return false;
            }
        }
    }
    else
    {
        if (pitentry <= pitexit)
        {
            if (fromstart >= pitentry && fromstart <= pitexit)
            {
                return true;
            } else
            {
                return false;
            }
        } else
        {
            // Warning: TORCS reports sometimes negative values for "fromstart"!
            if (fromstart <= pitexit || fromstart >= pitentry)
            {
                return true;
            } else
            {
                return false;
            }
        }
    }
}


// Checks if we stay too long without getting captured by the pit.
// Distance is the distance to the pit along the track, when the pit is
// ahead it is > 0, if we overshoot the pit it is < 0.
bool Pit::isTimeout(float distance)
{
    if (car->_speed_x > 1.0f || distance > 3.0f || !getPitstop())
    {
        pittimer = 0.0f;
        return false;
    } else
    {
        pittimer += (float) RCM_MAX_DT_ROBOTS;
        if (pittimer > 3.0f)
        {
            pittimer = 0.0f;
            return true;
        } else
        {
            return false;
        }
    }
}


// Update pit data and strategy.
void Pit::update()
{
    if (mypit != NULL)
    {
        if (isBetween(car->_distFromStartLine, 0))
        {
            if (getPitstop())
            {
                /*
                if (!isBetween(car->_distFromStartLine, 1) ||
                    (side == TR_LFT && car->_trkPos.toLeft < 0.0) ||
                    (side == TR_RGT && car->_trkPos.toRight < 0.0))
                {
                */
                setInPit(true);
                /*
                }
                else
                {
                    setInPit(false);
                    setPitstop(false);
                }
                */
            }
        } else
        {
            setInPit(false);
        }

        if (getPitstop())
        {
            car->_raceCmd = RM_CMD_PIT_ASKED;
        }
    }
}


float Pit::getSpeedLimitBrake(float speedsqr)
{
    return (speedsqr-speedlimitsqr)/(pitspeedlimitsqr-speedlimitsqr);
}
