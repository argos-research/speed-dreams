/***************************************************************************

    file        : categories.cpp
    created     : Sun Dec 15 11:12:56 CET 2002
    copyright   : (C) 2002 by Eric Espi�                        
    email       : eric.espie@torcs.org   
    version     : $Id: categories.cpp 6144 2015-09-24 19:05:15Z torcs-ng $                                  

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
    		
    @author	<a href=mailto:eric.espie@torcs.org>Eric Espie</a>
    @version	$Id: categories.cpp 6144 2015-09-24 19:05:15Z torcs-ng $
*/

#include <stdio.h>
#include "sim.h"


tdble simDammageFactor[] = {0.0f, 0.1f, 0.3f, 0.5f, 0.8f, 1.0f};

//tdble simSkidFactor[] = {0.40f, 0.35f, 0.3f, 0.0f, 0.0f};
tdble simSkidFactor[] = {0.4f, 0.3f, 0.2f, 0.1f, 0.0f, 0.0f};
