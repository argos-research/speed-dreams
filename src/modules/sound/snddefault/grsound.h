// -*- Mode: c++ -*-
/***************************************************************************

    file                 : grsound.h
    created              : Thu Aug 17 23:57:35 CEST 2000
    copyright            : (C) 2000-2004 by Eric Espie, Christos Dimitrakakis
    email                : torcs@free.fr
    version              : $Id: grsound.h 5095 2013-01-12 17:57:47Z pouillot $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _GRSOUND_H_
#define _GRSOUND_H_

#include <isoundengine.h>
#include <raceman.h>	//tSituation

extern void grInitSound(tSituation* s, int ncars);
extern void grShutdownSound();
extern void grRefreshSound(tSituation *s, Camera *camera);
extern void grMuteSound(bool bOn = true);

#endif /* _GRSOUND_H_ */ 
