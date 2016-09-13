/***************************************************************************

    file        : raceinit.h
    created     : Sat Nov 16 12:24:26 CET 2002
    copyright   : (C) 2002 by Eric Espie
    email       : eric.espie@torcs.org   
    version     : $Id: raceinit.h 6270 2015-11-23 19:44:40Z madbad $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
 
/** @file    
    		
    @author	<a href=mailto:torcs@free.fr>Eric Espie</a>
    @version	$Id: raceinit.h 6270 2015-11-23 19:44:40Z madbad $
*/

#ifndef _RACEINIT_H_
#define _RACEINIT_H_

class GfRaceManager;


extern void ReStartNewRace(void);
extern void ReResumeRace(void);

extern void ReReset(void);
extern void ReCleanup(void);
extern int  ReExit();

extern void ReRaceSelectRaceman(GfRaceManager* pRaceMan, bool bKeepHumans = true);
extern void ReRaceRestore(void* hparmResults);
extern void ReRaceConfigure(bool bInteractive = true);

extern int  ReInitCars(void);

extern void ReRaceCleanup(void);
extern void ReRaceCleanDrivers(void);

extern char *ReGetCurrentRaceName(void);

extern char *ReGetPrevRaceName(bool bLoop);

extern tModList *ReRacingRobotsModList;

// The race situation data structure.
// WIP: Remove this global variable that anyone can wildly change
//      and replace the read/write instruction everywhere
//      by calls to the functions of ReSituation::self().
struct RmInfo;
extern struct RmInfo *ReInfo;

#endif /* _RACEINIT_H_ */ 



