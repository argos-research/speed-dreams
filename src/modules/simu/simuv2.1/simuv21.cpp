/***************************************************************************

    file                 : simuv21.cpp
    created              : Sun Mar 19 00:08:04 CET 2000
    copyright            : (C) 2000 by Eric Espie
    email                : torcs@free.fr
    version              : $Id: simuv21.cpp 6097 2015-08-30 23:12:09Z beaglejoe $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "simuv21.h"

#include "sim.h"


// The Simuv21: singleton.
Simuv21* Simuv21::_pSelf = 0;

int openGfModule(const char* pszShLibName, void* hShLibHandle)
{
	// Instanciate the (only) module instance.
	Simuv21::_pSelf = new Simuv21(pszShLibName, hShLibHandle);

	// Register it to the GfModule module manager if OK.
	if (Simuv21::_pSelf)
		GfModule::register_(Simuv21::_pSelf);

	// Report about success or error.
	return Simuv21::_pSelf ? 0 : 1;
}

int closeGfModule()
{
	// Unregister it from the GfModule module manager.
	if (Simuv21::_pSelf)
		Simuv21::unregister(Simuv21::_pSelf);
	
	// Delete the (only) module instance.
	delete Simuv21::_pSelf;
	Simuv21::_pSelf = 0;

	// Report about success or error.
	return 0;
}

Simuv21& Simuv21::self()
{
	// Pre-condition : 1 successfull openGfModule call.
	return *_pSelf;
}

Simuv21::Simuv21(const std::string& strShLibName, void* hShLibHandle)
: GfModule(strShLibName, hShLibHandle)
{
}

Simuv21::~Simuv21()
{
}

// Implementation of IPhysicsEngine.
void Simuv21::initialize(int nCars, struct Track* pTrack)
{
	::SimInit(nCars, pTrack);
}

void Simuv21::configureCar(struct CarElt* pCar)
{
	::SimConfig(pCar);
}

void Simuv21::reconfigureCar(struct CarElt* pCar)
{
	::SimReConfig(pCar);
}

void Simuv21::toggleCarTelemetry(int nCarIndex, bool bOn)
{
	::SimCarTelemetry(nCarIndex, bOn);
}

void Simuv21::updateSituation(struct Situation *pSituation, double fDeltaTime)
{
	::SimUpdate(pSituation, fDeltaTime);
}

void Simuv21::updateCar(struct Situation *pSituation, double fDeltaTime, int nCarIndex)
{
	::SimUpdateSingleCar(nCarIndex, fDeltaTime, pSituation);
}

void Simuv21::setCar(const struct DynPt& dynGCG, int nCarIndex)
{
	::UpdateSimCarTable(dynGCG, nCarIndex);
}

tDynPt* Simuv21::getCar(int nCarIndex)
{
	return ::GetSimCarTable(nCarIndex);
}

void Simuv21::shutdown()
{
	::SimShutdown();
}
