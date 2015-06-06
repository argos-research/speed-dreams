/***************************************************************************

    file        : racenetwork.h
    copyright   : (C) 2009 by Brian Gavin 
    web         : www.speed-dreams.org 
    version     : $Id: racenetwork.h 2917 2010-10-17 19:03:40Z pouillot $

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
    		
    @author	    Brian Gavin
    @version	$Id: racenetwork.h 2917 2010-10-17 19:03:40Z pouillot $
*/

#ifndef _RACENETWORK_H_
#define _RACENETWORK_H_

extern void ReNetworkOneStep();
extern int ReNetworkWaitReady();
extern void ReNetworkCheckEndOfRace();


#endif /* _RACENETWORK_H_ */ 
