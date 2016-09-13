/***************************************************************************

    file                 : replay.h
    created              : Sun Jan 30 22:59:28 CET 2000
    copyright            : (C) 2000 by Eric Espie
    email                : torcs@free.fr
    version              : $Id: replay.h 5803 2014-07-30 03:19:34Z mungewell $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
 
 
#ifndef __REPLAYV1_H__
#define __REPLAYV1_H__

#define RPL_IDENT	0

#include <car.h>

#define REPLAY_TIMESTEP 2	// 2Hz Data logging
#define REPLAY_CHUNK	100

typedef struct ReplayElt
{
    double		currentTime;
    tInitCar		info;	/**< public */
    tPublicCar		pub;	/**< public */
    tCarRaceInfo	race;	/**< public */
    tPrivCar		priv;	/**< private */
    tCarCtrl		ctrl;	/**< private */
    tCarPitCmd		pitcmd;	/**< private */
} tReplayElt;

extern int replayRecord;
extern int replayReplay;
extern double replayTimestamp;

extern int ghostcarActive;
extern double ghostcarTimeOffset;
extern tReplayElt curGhostcarData, nextGhostcarData;

#ifdef THIRD_PARTY_SQLITE3
#include <sqlite3.h>

extern sqlite3 *replayDB;
extern sqlite3_stmt *replayBlobs[50];	// Hard coded 50 car limit to race
extern sqlite3_stmt *ghostcarBlob;
#endif

#endif /* __REPLAYV1_H__ */ 



