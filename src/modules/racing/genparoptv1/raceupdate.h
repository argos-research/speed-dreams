/***************************************************************************

    file        : raceupdate.h
    created     : Sat Nov 23 09:35:21 CET 2002
    copyright   : (C) 2002 by Eric Espie
    email       : eric.espie@torcs.org   
    version     : $Id: raceupdate.h 4983 2012-10-07 13:53:17Z pouillot $                                  

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
    @version	$Id: raceupdate.h 4983 2012-10-07 13:53:17Z pouillot $
*/

#ifndef _RACEUPDATE_H_
#define _RACEUPDATE_H_

extern void ReInitUpdaters();
extern void ReShutdownUpdaters();

extern bool ReSetSchedulingSpecs(double fSimuRate, double fOutputRate = 0);

extern void ReStart();
extern void ReStop();
extern int  ReUpdate();

#ifdef SD_DEBUG
extern void ReOneStep(double dt);
#endif

struct RmInfo;
extern const struct RmInfo* ReOutputSituation();

#endif /* _RACEUPDATE_H_ */ 



