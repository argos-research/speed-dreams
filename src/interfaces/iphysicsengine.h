/***************************************************************************
                 iphysicsengine.h -- Interface for physics engines

    created              : Tue May 10 22:40:04 CEST 2011
    copyright            : (C) 2011 by Jean-Philippe Meuret
    web                  : http://www.speed-dreams.org
    version              : $Id: iphysicsengine.h 4903 2012-08-27 11:31:33Z kmetykog $
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
    	Interface for physics engines
    @version	$Id: iphysicsengine.h 4903 2012-08-27 11:31:33Z kmetykog $
*/

#ifndef __IPHYSICSENGINE__H__
#define __IPHYSICSENGINE__H__

struct DynPt;


class IPhysicsEngine
{
public:

	//! Initial setup
	virtual void initialize(int nCars, struct Track* pTrack) = 0;

	//! Initial configuration
	virtual void configureCar(struct CarElt* pCar) = 0;

	//! After pit stop
	virtual void reconfigureCar(struct CarElt* pCar) = 0;

	//! Activate / Deactivate telemetry for a given car (Limitation: only one at a time).
	virtual void toggleCarTelemetry(int nCarIndex, bool bOn = true) = 0;

	//! Update the situation (1 simulation step)
	virtual void updateSituation(struct Situation *pSituation, double fDeltaTime) = 0;

	//! Update only a given car (1 simulation step)
	virtual void updateCar(struct Situation *pSituation, double fDeltaTime, int nCarIndex) = 0;

	//! For network races, force the DynGCG of a given car (computed by a remote instance).
	virtual void setCar(const struct DynPt& dynGCG, int nCarIndex) = 0;

	//! For network races, read the DynGCG of a given car.
	virtual struct DynPt* getCar(int nCarIndex) = 0;

	//! Reset => ready for a new initialize
	virtual void shutdown() = 0;
};

#endif // __IPHYSICSENGINE__H__
