/***************************************************************************

    file                 : splash.h
    created              : Sat Mar 18 23:49:17 CET 2000
    copyright            : (C) 2000 by Eric Espie
    email                : torcs@free.fr
    version              : $Id: splash.h 4243 2011-12-03 15:14:26Z pouillot $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
 
 
#ifndef _SPLASH_H_
#define _SPLASH_H_

extern bool SplashScreen(bool (*fnBackWork)(void), bool (*fnOnClosed)(void),
						 bool bInteractive = true);


#endif /* _SPLASH_H_ */ 





