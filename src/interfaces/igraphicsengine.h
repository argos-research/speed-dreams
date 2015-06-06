/***************************************************************************
                 igraphicsengine.h -- Interface for graphics engines

    created              : Mon Mar 28 19:48:14 CEST 2011
    copyright            : (C) 2011 by Jean-Philippe Meuret                         
    web                  : http://www.speed-dreams.org
    version              : $Id: igraphicsengine.h 5095 2013-01-12 17:57:47Z pouillot $
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
    	Interface for graphics engines
    @version	$Id: igraphicsengine.h 5095 2013-01-12 17:57:47Z pouillot $
*/

#ifndef __IGRAPHICSENGINE__H__
#define __IGRAPHICSENGINE__H__

#include "camera.h"

class IGraphicsEngine
{
public:

	virtual bool loadTrack(struct Track* pTrack) = 0;
	virtual bool loadCars(struct Situation *pSituation) = 0;
	virtual bool setupView(int x, int y, int width, int height, void* pMenuScreen) = 0;
	virtual void redrawView(struct Situation *pSituation) = 0;
	virtual void shutdownView() = 0;
	virtual void unloadCars() = 0;
	virtual void unloadTrack() = 0;

	// Return the current (end-user) camera, as a new instance (caller must delete after use).
	virtual Camera *getCurCam() = 0;

	//virtual void bendCar(int index, sgVec3 poc, sgVec3 force, int count = 0) = 0;
};

#endif // __IGRAPHICSENGINE__H__
