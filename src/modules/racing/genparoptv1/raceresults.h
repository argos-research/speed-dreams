/***************************************************************************

    file        : raceresults.h
    created     : Thu Jan  2 12:43:28 CET 2003
    copyright   : (C) 2002 by Eric Espié                        
    email       : eric.espie@torcs.org   
    version     : $Id: raceresults.h 4304 2011-12-27 13:45:00Z pouillot $                                  

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
    @version	$Id: raceresults.h 4304 2011-12-27 13:45:00Z pouillot $
*/

#ifndef _RACERESULTS_H_
#define _RACERESULTS_H_


extern void ReInitResults(void);

extern void ReStoreRaceResults(const char *race);

extern void ReCalculateClassPoints(char const *race);

extern void ReSavePracticeLap(tCarElt *car);

extern void ReInitCurRes(void);
extern void ReUpdatePracticeCurRes(tCarElt *car, bool bForceNew = false);
extern void ReUpdateQualifCurRes(tCarElt *car);
extern void ReUpdateRaceCurRes();

extern void ReEventInitResults(void);

extern void ReUpdateStandings();

#endif /* _RACERESULTS_H_ */ 



