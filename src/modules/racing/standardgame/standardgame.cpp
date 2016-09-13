/***************************************************************************

    file        : standardgame.cpp
    copyright   : (C) 2010 by Jean-Philippe Meuret                        
    email       : pouillot@users.sourceforge.net   
    version     : $Id: standardgame.cpp 6097 2015-08-30 23:12:09Z beaglejoe $                                  

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
    		The standard game race engine module
    @version    $Id: standardgame.cpp 6097 2015-08-30 23:12:09Z beaglejoe $
*/

#include <sstream>

#include <tgf.hpp>

#include <car.h> // tCarPitCmd.
#include <raceman.h>

#include <tgfdata.h>
#include <race.h>
#include <tracks.h>
#include <replay.h>

#include "racesituation.h"
#include "racemain.h"
#include "raceinit.h"
#include "racestate.h"
#include "raceupdate.h"

#include "standardgame.h"

int replayReplay;

// The singleton.
StandardGame* StandardGame::_pSelf = 0;

int openGfModule(const char* pszShLibName, void* hShLibHandle)
{
	// Instanciate the (only) module instance.
	StandardGame::_pSelf = new StandardGame(pszShLibName, hShLibHandle);

	// Register it to the GfModule module manager if OK.
	if (StandardGame::_pSelf)
		GfModule::register_(StandardGame::_pSelf);

	// Report about success or error.
	return StandardGame::_pSelf ? 0 : 1;
}

int closeGfModule()
{
	// Unregister it from the GfModule module manager.
	if (StandardGame::_pSelf)
		StandardGame::unregister(StandardGame::_pSelf);

	// Delete the (only) module instance.
	delete StandardGame::_pSelf;
	StandardGame::_pSelf = 0;

	// Report about success or error.
	return 0;
}

StandardGame& StandardGame::self()
{
	// Pre-condition : 1 successfull openGfModule call.
	return *_pSelf;
}

StandardGame::StandardGame(const std::string& strShLibName, void* hShLibHandle)
: GfModule(strShLibName, hShLibHandle),
  _piUserItf(0), _piTrkLoader(0), _piPhysEngine(0), _pRace(new GfRace())
{
}

// Implementation of IRaceEngine.
void StandardGame::reset(void)
{
	GfLogInfo("Resetting StandardGame race engine.\n");

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

void StandardGame::cleanup(void)
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

void StandardGame::shutdown(void)
{
	GfLogInfo("Shutting down StandardGame race engine.\n");

	cleanup();

	delete _pRace;
}

StandardGame::~StandardGame()
{
}

void StandardGame::setUserInterface(IUserInterface& userItf)
{
	_piUserItf = &userItf;
}

void StandardGame::initializeState(void *prevMenu)
{
	::ReStateInit(prevMenu);
}

void StandardGame::updateState(void)
{
	::ReStateManage();
}

void StandardGame::applyState(int state)
{
	::ReStateApply((void*)(long)state);
}

void StandardGame::selectRaceman(GfRaceManager* pRaceMan, bool bKeepHumans)
{
	::ReRaceSelectRaceman(pRaceMan, bKeepHumans);
}

void StandardGame::restoreRace(void* hparmResults)
{
	::ReRaceRestore(hparmResults);
}

void StandardGame::configureRace(bool bInteractive)
{
	::ReRaceConfigure(bInteractive);
}

//************************************************************
void StandardGame::startNewRace()
{
	::ReStartNewRace();
}

void StandardGame::resumeRace()
{
	::ReResumeRace();
}

//************************************************************
void StandardGame::startRace()
{
	int mode = ::ReRaceRealStart();
	if(mode & RM_ERROR)
	{
		GfLogError("ReRaceRealStart() ERROR in RaceEngine::startRace() \n");
		ReInfo->_reState = RE_STATE_ERROR;
	}
	else
	{
		// Maybe should be RE_STATE_NETWORK_WAIT
		ReInfo->_reState = RE_STATE_PRE_RACE_PAUSE;
	}
}

void StandardGame::stopCooldown()
{
	::ReStopCooldown();
}

void StandardGame::abandonRace()
{
	::ReRaceAbandon();
}

void StandardGame::abortRace()
{
	::ReRaceAbort();
}

void StandardGame::skipRaceSession()
{
	::ReRaceSkipSession();
}

void StandardGame::restartRace()
{
	::ReRaceRestart();
}

//************************************************************
void StandardGame::start(void)
{
	::ReStart();
}

void StandardGame::stop(void)
{
	::ReStop();
}

bool StandardGame::supportsHumanDrivers()
{
	return true;
}

#ifdef SD_DEBUG
void StandardGame::step(double dt)
{
	::ReOneStep(dt);
}
#endif

void StandardGame::stopPreracePause()
{
   ::ReStopPreracePause();
}

//************************************************************
GfRace* StandardGame::race()
{
	return _pRace;
}

const GfRace* StandardGame::race() const
{
	return _pRace;
}

// TODO: Remove when safe dedicated setters ready.
tRmInfo* StandardGame::inData()
{
	return ReSituation::self().data(); // => ReInfo
}

const tRmInfo* StandardGame::outData() const
{
	return ::ReOutputSituation();
}

// Accessor to the user interface.
IUserInterface& StandardGame::userInterface()
{
	return *_piUserItf;
}

// Physics engine management.
bool StandardGame::loadPhysicsEngine()
{
    // Load the Physics engine module if not already done.
	if (_piPhysEngine)
		return true;

	// 1) Get the physics engine name from user settings (default: Simu V4.0)
	static const char* pszDefaultModName = RM_VAL_MOD_SIMU_V4;
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
	std::ostringstream ossLoadMsg;
	ossLoadMsg << "Loading physics engine (" << strModName << ") ...";
	if (_piUserItf)
		_piUserItf->addLoadingMessage(ossLoadMsg.str().c_str());

	GfModule* pmodPhysEngine = GfModule::load("modules/simu", strModName.c_str());
	if (pmodPhysEngine)
		_piPhysEngine = pmodPhysEngine->getInterface<IPhysicsEngine>();
	if (pmodPhysEngine && !_piPhysEngine)
		GfModule::unload(pmodPhysEngine);

	// don't record if we're 'replaying'
	printf("Checking type of SIMU\n");
	if (strcmp(RM_VAL_MOD_SIMU_REPLAY, strModName.c_str()) == 0)
		replayReplay = 1;
	else
		replayReplay = 0;

	return _piPhysEngine ? true : false;
}

void StandardGame::unloadPhysicsEngine()
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
ITrackLoader& StandardGame::trackLoader()
{
	return *_piTrkLoader;
}

// Accessor to the physics engine.
IPhysicsEngine& StandardGame::physicsEngine()
{
	return *_piPhysEngine;
}


//************************************************************
// WIP : dedicated situation setters.

bool StandardGame::setSchedulingSpecs(double fSimuRate, double fOutputRate)
{
	return ::ReSetSchedulingSpecs(fSimuRate, fOutputRate);
}

void StandardGame::accelerateTime(double fMultFactor)
{
	ReSituation::self().accelerateTime(fMultFactor);
}

void StandardGame::setPitCommand(int nCarIndex, const struct CarPitCmd* pPitCmd)
{
	ReSituation::self().setPitCommand(nCarIndex, pPitCmd);
}
