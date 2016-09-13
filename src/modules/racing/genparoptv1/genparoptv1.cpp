/***************************************************************************

    file        : genparoptv1.cpp
    copyright   : (C) 2012 by Wolf-Dieter Beelitz
    email       : pouillot@users.sourceforge.net
    version     : $Id: genparoptv1.cpp 6097 2015-08-30 23:12:09Z beaglejoe $

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
  		A race engine module designed for optimising car and AI driver setups
		(parameters) through a genetic algorithm
    @version    $Id: genparoptv1.cpp 6097 2015-08-30 23:12:09Z beaglejoe $
*/

#include <sstream>

#include <tgf.hpp>

#include <car.h> // tCarPitCmd.
#include <raceman.h>

#include <tgfdata.h>
#include <race.h>
#include <tracks.h>

#include "racesituation.h"
#include "racemain.h"
#include "raceinit.h"
#include "racestate.h"
#include "raceupdate.h"

#include "genparoptv1.h"


// The "Optim" logger instance (really initialised in GenParOptV1 constructor).
GfLogger* RePLogOptim = 0;

// The module singleton.
GenParOptV1* GenParOptV1::_pSelf = 0;

int openGfModule(const char* pszShLibName, void* hShLibHandle)
{
	// Instanciate the (only) module instance.
	GenParOptV1::_pSelf = new GenParOptV1(pszShLibName, hShLibHandle);

	// Register it to the GfModule module manager if OK.
	if (GenParOptV1::_pSelf)
		GfModule::register_(GenParOptV1::_pSelf);

	// Report about success or error.
	return GenParOptV1::_pSelf ? 0 : 1;
}

int closeGfModule()
{
	// Unregister it from the GfModule module manager.
	if (GenParOptV1::_pSelf)
		GenParOptV1::unregister(GenParOptV1::_pSelf);

	// Delete the (only) module instance.
	delete GenParOptV1::_pSelf;
	GenParOptV1::_pSelf = 0;

	// Report about success or error.
	return 0;
}

GenParOptV1& GenParOptV1::self()
{
	// Pre-condition : 1 successfull openGfModule call.
	return *_pSelf;
}

GenParOptV1::GenParOptV1(const std::string& strShLibName, void* hShLibHandle)
: GfModule(strShLibName, hShLibHandle),
  _piUserItf(0), _piTrkLoader(0), _piPhysEngine(0), _pRace(new GfRace())
{
	// Initialise the "Optim" logger instance.
	RePLogOptim = GfLogger::instance("Optim");
}

// Implementation of IRaceEngine.
void GenParOptV1::reset(void)
{
	GfLogInfo("Resetting GenParOptV1 race engine.\n");

	// Cleanup everything in case no yet done.
	cleanup();
	
	// Internal init.
	::ReReset();

	// Load and initialize the track loader module.
	const char* pszModName =
		GfParmGetStr(ReSituation::self().data()->_reParam, "Modules", "track", "track");
	GfLogInfo("Loading '%s' track loader ...\n", pszModName);
	GfModule* pmodTrkLoader = GfModule::load("modules/track", pszModName);

	// Check that it implements ITrackLoader.
	if (pmodTrkLoader)
		_piTrkLoader = pmodTrkLoader->getInterface<ITrackLoader>();
	if (pmodTrkLoader && !_piTrkLoader)
	{
		GfModule::unload(pmodTrkLoader);
		return;
	}

	// Initialize GfTracks' track module interface (needed for some track infos).
	GfTracks::self()->setTrackLoader(_piTrkLoader);
}

void GenParOptV1::cleanup(void)
{
	// Internal cleanup.
	::ReCleanup();

	// Unload the track and then the Track loader module if not already done.
	if (_piTrkLoader)
	{
		// Unload the track.
		_piTrkLoader->unload();

		// Unload the Track loader module.
		GfModule* pmodTrkLoader = dynamic_cast<GfModule*>(_piTrkLoader);
		if (pmodTrkLoader)
		{
			GfModule::unload(pmodTrkLoader);
			_piTrkLoader = 0;
			GfTracks::self()->setTrackLoader(0);
		}
	}

    // Unload the Physics engine module if not already done.
	if (_piPhysEngine)
	{
		GfModule* pmodPhysEngine = dynamic_cast<GfModule*>(_piPhysEngine);
		if (pmodPhysEngine)
		{
			GfModule::unload(pmodPhysEngine);
			_piPhysEngine = 0;
		}
	}
}

void GenParOptV1::shutdown(void)
{
	GfLogInfo("Shutting down GenParOptV1 race engine.\n");

	cleanup();

	delete _pRace;
}

GenParOptV1::~GenParOptV1()
{
}

void GenParOptV1::setUserInterface(IUserInterface& userItf)
{
	_piUserItf = &userItf;
}

void GenParOptV1::initializeState(void *prevMenu)
{
	::ReStateInit(prevMenu);
}

void GenParOptV1::updateState(void)
{
	::ReStateManage();
}

void GenParOptV1::applyState(int state)
{
	::ReStateApply((void*)(long)state);
}

void GenParOptV1::selectRaceman(GfRaceManager* pRaceMan, bool bKeepHumans)
{
	::ReRaceSelectRaceman(pRaceMan, bKeepHumans);
}

void GenParOptV1::restoreRace(void* hparmResults)
{
	::ReRaceRestore(hparmResults);
}

void GenParOptV1::configureRace(bool bInteractive)
{
	::ReRaceConfigure(bInteractive);
}

//************************************************************
void GenParOptV1::startNewRace()
{
	::ReStartNewRace();
}

void GenParOptV1::resumeRace()
{
	::ReResumeRace();
}

//************************************************************
void GenParOptV1::startRace()
{
	// TODO: Process error status ?
	(void)::ReRaceRealStart();
}

void GenParOptV1::abandonRace()
{
	::ReRaceAbandon();
}

void GenParOptV1::abortRace()
{
	::ReRaceAbort();
}

void GenParOptV1::skipRaceSession()
{
	::ReRaceSkipSession();
}

void GenParOptV1::restartRace()
{
	::ReRaceRestart();
}

//************************************************************
void GenParOptV1::start(void)
{
	::ReStart();
}

void GenParOptV1::stop(void)
{
	::ReStop();
}

bool GenParOptV1::supportsHumanDrivers()
{
	return false;
}

#ifdef SD_DEBUG
void GenParOptV1::step(double dt)
{
	::ReOneStep(dt);
}
#endif

//************************************************************
GfRace* GenParOptV1::race()
{
	return _pRace;
}

const GfRace* GenParOptV1::race() const
{
	return _pRace;
}

// TODO: Remove when safe dedicated setters ready.
tRmInfo* GenParOptV1::inData()
{
	return ReSituation::self().data(); // => ReInfo
}

const tRmInfo* GenParOptV1::outData() const
{
	return ::ReOutputSituation();
}

// Accessor to the user interface.
IUserInterface& GenParOptV1::userInterface()
{
	return *_piUserItf;
}

// Physics engine management.
bool GenParOptV1::loadPhysicsEngine()
{
    // Load the Physics engine module if not already done.
	if (_piPhysEngine)
		return true;

	// 1) Get the physics engine name from user settings (default: Simu V2.1)
	static const char* pszDefaultModName = RM_VAL_MOD_SIMU_V2_1;
	std::string strModName =
		GfParmGetStr(ReSituation::self().data()->_reParam, "Modules", "simu", pszDefaultModName);

	// 2) Check if the module is really there, and fall back to the default one if not
	//    Note : The default module is supposed to be always there.
	if (!GfModule::isPresent("simu", strModName.c_str()))
	{
		GfLogWarning("User settings %s physics engine module not found ; "
					 "falling back to %s\n", strModName.c_str(), pszDefaultModName);
		strModName = pszDefaultModName;
	}

	// 3) Load it.
/*
	std::ostringstream ossLoadMsg;
	ossLoadMsg << "Loading physics engine (" << strModName << ") ...";
	if (_piUserItf)
		_piUserItf->addOptimizationMessage(ossLoadMsg.str().c_str());
*/
	GfModule* pmodPhysEngine = GfModule::load("modules/simu", strModName.c_str());
	if (pmodPhysEngine)
		_piPhysEngine = pmodPhysEngine->getInterface<IPhysicsEngine>();
	if (pmodPhysEngine && !_piPhysEngine)
		GfModule::unload(pmodPhysEngine);

	return _piPhysEngine ? true : false;
}

void GenParOptV1::unloadPhysicsEngine()
{
    // Unload the Physics engine module if not already done.
	if (!_piPhysEngine)
		return;
	
	GfModule* pmodPhysEngine = dynamic_cast<GfModule*>(_piPhysEngine);
	if (pmodPhysEngine)
		GfModule::unload(pmodPhysEngine);
	_piPhysEngine = 0;
}

// Accessor to the track loader.
ITrackLoader& GenParOptV1::trackLoader()
{
	return *_piTrkLoader;
}

// Accessor to the physics engine.
IPhysicsEngine& GenParOptV1::physicsEngine()
{
	return *_piPhysEngine;
}


//************************************************************
// WIP : dedicated situation setters.

bool GenParOptV1::setSchedulingSpecs(double fSimuRate, double fOutputRate)
{
	return ::ReSetSchedulingSpecs(fSimuRate, fOutputRate);
}

void GenParOptV1::accelerateTime(double fMultFactor)
{
	ReSituation::self().accelerateTime(fMultFactor);
}

void GenParOptV1::setPitCommand(int nCarIndex, const struct CarPitCmd* pPitCmd)
{
	ReSituation::self().setPitCommand(nCarIndex, pPitCmd);
}
