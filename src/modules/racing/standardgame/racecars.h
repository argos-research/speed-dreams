/***************************************************************************

    file        : racecars.h
    created     : Sat Nov 23 09:35:21 CET 2002
    copyright   : (C) 2002 by Eric Espie
    email       : eric.espie@torcs.org   
    version     : $Id: racecars.h 2917 2010-10-17 19:03:40Z pouillot $                                  

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
    @version	$Id: racecars.h 2917 2010-10-17 19:03:40Z pouillot $
*/

#ifndef _RACECARS_H_
#define _RACECARS_H_

#include <car.h>

extern void ReCarsUpdateCarPitTime(tCarElt *car);
extern void ReCarsManageCar(tCarElt *car, bool& bestLapChanged);
extern void ReCarsSortCars(void);

#endif /* _RACECARS_H_ */ 



