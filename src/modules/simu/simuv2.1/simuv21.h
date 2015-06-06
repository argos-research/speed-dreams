/***************************************************************************

    file        : simuv21.h
    copyright   : (C) 2011 by Jean-Philippe Meuret
    email       : pouillot@users.sourceforge.net
    version     : $Id: simuv21.h 4903 2012-08-27 11:31:33Z kmetykog $

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
    		The "Simu V2.1" physics engine module
    @version    $Id: simuv21.h 4903 2012-08-27 11:31:33Z kmetykog $
*/

#ifndef _SIMUV21_H_
#define _SIMUV21_H_

#include <iphysicsengine.h>

#include <tgf.hpp>


// DLL exported symbols declarator for Windows.
#ifdef WIN32
# ifdef SIMUV21_DLL
#  define SIMUV21_API __declspec(dllexport)
# else
#  define SIMUV21_API __declspec(dllimport)
# endif
#else
# define SIMUV21_API
#endif


// The C interface of the module.
extern "C" int SIMUV21_API openGfModule(const char* pszShLibName, void* hShLibHandle);
extern "C" int SIMUV21_API closeGfModule();

// The module main class (Singleton, inherits GfModule, and implements IPhysicsEngine).
class SIMUV21_API Simuv21 : public GfModule, public IPhysicsEngine
{
 public:

	// Implementation of IPhysicsEngine.
	virtual void initialize(int nCars, struct Track* pTrack);
	virtual void configureCar(struct CarElt* pCar);
	virtual void reconfigureCar(struct CarElt* pCar);
	virtual void toggleCarTelemetry(int nCarIndex, bool bOn = true);
	virtual void updateSituation(struct Situation *pSituation, double fDeltaTime);
	virtual void updateCar(struct Situation *pSituation, double fDeltaTime, int nCarIndex);
	virtual void setCar(const struct DynPt& dynGCG, int nCarIndex);
	virtual struct DynPt* getCar(int nCarIndex);
	virtual void shutdown();

	// Accessor to the singleton.
	static Simuv21& self();

	// Destructor.
	virtual ~Simuv21();

 protected:

	// Protected constructor to avoid instanciation outside (but friends).
	Simuv21(const std::string& strShLibName, void* hShLibHandle);

	// Make the C interface functions nearly member functions.
	friend int openGfModule(const char* pszShLibName, void* hShLibHandle);
	friend int closeGfModule();

 protected:

	// The singleton.
	static Simuv21* _pSelf;
};

#endif /* _SIMUV21_H_ */
