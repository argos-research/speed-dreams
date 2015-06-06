/***************************************************************************

    file                 : simuv4.cpp
    created              : Sun Mar 19 00:08:04 CET 2000
    copyright            : (C) 2000 by Eric Espie
    email                : torcs@free.fr
    version              : $Id: simuv4.cpp 3568 2011-05-15 15:55:24Z pouillot $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "simuv4.h"

#include "sim.h"


// The Simuv4: singleton.
Simuv4* Simuv4::_pSelf = 0;

int openGfModule(const char* pszShLibName, void* hShLibHandle)
{
	// Instanciate the (only) module instance.
	Simuv4::_pSelf = new Simuv4(pszShLibName, hShLibHandle);

	// Register it to the GfModule module manager if OK.
	if (Simuv4::_pSelf)
		GfModule::register_(Simuv4::_pSelf);

	// Report about success or error.
	return Simuv4::_pSelf ? 0 : 1;
}

int closeGfModule()
{
	// Unregister it from the GfModule module manager.
	if (Simuv4::_pSelf)
		GfModule::unregister(Simuv4::_pSelf);
	
	// Delete the (only) module instance.
	delete Simuv4::_pSelf;
	Simuv4::_pSelf = 0;

	// Report about success or error.
	return 0;
}

Simuv4& Simuv4::self()
{
	// Pre-condition : 1 successfull openGfModule call.
	return *_pSelf;
}

Simuv4::Simuv4(const std::string& strShLibName, void* hShLibHandle)
: GfModule(strShLibName, hShLibHandle)
{
}

Simuv4::~Simuv4()
{
}

// Implementation of IPhysicsEngine.
void Simuv4::initialize(int nCars, struct Track* pTrack)
{
	::SimInit(nCars, pTrack);
}

void Simuv4::configureCar(struct CarElt* pCar)
{
	::SimConfig(pCar);
}

void Simuv4::reconfigureCar(struct CarElt* pCar)
{
	::SimReConfig(pCar);
}

void Simuv4::toggleCarTelemetry(int nCarIndex, bool bOn)
{
	::SimCarTelemetry(nCarIndex, bOn);
}

void Simuv4::updateSituation(struct Situation *pSituation, double fDeltaTime)
{
	::SimUpdate(pSituation, fDeltaTime);
}

void Simuv4::updateCar(struct Situation *pSituation, double fDeltaTime, int nCarIndex)
{
	::SimUpdateSingleCar(nCarIndex, fDeltaTime, pSituation);
}

void Simuv4::setCar(const struct DynPt& dynGCG, int nCarIndex)
{
	::UpdateSimCarTable(dynGCG, nCarIndex);
}

tDynPt* Simuv4::getCar(int nCarIndex)
{
	return ::GetSimCarTable(nCarIndex);
}

void Simuv4::shutdown()
{
	::SimShutdown();
}
