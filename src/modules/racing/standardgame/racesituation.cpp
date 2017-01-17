/***************************************************************************

    file        : racesituation.cpp
    copyright   : (C) 2010 by Jean-Philippe Meuret
    web         : www.speed-dreams.org 
    version     : $Id: racesituation.cpp 6294 2015-11-30 00:20:46Z kakukri $

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
    		The central raceman data structure (situation + other race infos)
    @author	    Jean-Philippe Meuret
    @version	$Id: racesituation.cpp 6294 2015-11-30 00:20:46Z kakukri $
*/

#include <cstdlib>
#include <sstream>
#include <iomanip>

#include <SDL.h>
#include <SDL_thread.h>

#ifdef THIRD_PARTY_SQLITE3
#include <sqlite3.h>
#endif

#include <portability.h>
#include <network.h>
#include <robot.h>
#include <raceman.h>
#include <replay.h>
#ifdef WEBSERVER
#include <webserver.h>
#endif //WEBSERVER

#include "standardgame.h"

#include "racecars.h"
#include "racesituation.h"

#include "raceresults.h"
#include "racemessage.h"
#include "racenetwork.h"



// The singleton.
ReSituation* ReSituation::_pSelf = 0;

ReSituation& ReSituation::self()
{
	if (!_pSelf)
		_pSelf = new ReSituation;
	
	return *_pSelf;
}

ReSituation::ReSituation()
: _pMutex(0)
{
	// Allocate race engine info structures.
	_pReInfo = (tRmInfo*)calloc(1, sizeof(tRmInfo));
	_pReInfo->s = (tSituation*)calloc(1, sizeof(tSituation));

	// Singleton initialized.
	_pSelf = this;
}

ReSituation::~ReSituation()
{
	// Free ReInfo memory.
	if (_pReInfo->results) 
    {
		if (_pReInfo->mainResults != _pReInfo->results)
			GfParmReleaseHandle(_pReInfo->mainResults);
		GfParmReleaseHandle(_pReInfo->results);
    }
    if (_pReInfo->_reParam)
		GfParmReleaseHandle(_pReInfo->_reParam);
    if (_pReInfo->params != _pReInfo->mainParams) 
    {
		GfParmReleaseHandle(_pReInfo->params);
		_pReInfo->params = _pReInfo->mainParams;
    }
    // if (_pReInfo->movieCapture.outputBase)
	// 	free(_pReInfo->movieCapture.outputBase);
    free(_pReInfo->s);
    free(_pReInfo->carList);
    free(_pReInfo->rules);
    
    FREEZ(_pReInfo);

	// Prepare the singleton for next use.
	_pSelf = 0;
}

void ReSituation::terminate()
{
	delete _pSelf;
}

void ReSituation::setThreadSafe(bool bOn)
{
	if (bOn)
	{
		if (!_pMutex)
			_pMutex = SDL_CreateMutex(); // Thread-safe On.
	}
	else if (_pMutex)
	{
		SDL_DestroyMutex(_pMutex);
		_pMutex = 0; // Thread-safe Off.
	}
}

bool ReSituation::lock(const char* pszCallerName)
{
	if (!_pMutex)
		return true;
	
	const bool bStatus = (SDL_mutexP(_pMutex) == 0);
	if (!bStatus)
		GfLogWarning("%s : Failed to lock situation mutex\n", pszCallerName);

	return bStatus;
}
	
bool ReSituation::unlock(const char* pszCallerName)
{
	if (!_pMutex)
		return true;
	
	const bool bStatus = SDL_mutexV(_pMutex) == 0;
	if (!bStatus)
		GfLogWarning("%s : Failed to unlock situation mutex\n", pszCallerName);

	return bStatus;
}

// TODO: Remove when safe accessors ready.
tRmInfo* ReSituation::data()
{
	return _pReInfo;
}

// Safe accessors.
void ReSituation::setDisplayMode(unsigned bfDispMode)
{
	lock("setDisplayMode");

	_pReInfo->_displayMode = bfDispMode;
	
	unlock("setDisplayMode");
}

void ReSituation::accelerateTime(double fMultFactor)
{
	lock("accelerateTime");

	if (_pReInfo->_reTimeMult > 0.0)
	    _pReInfo->_reTimeMult *= fMultFactor;
	else
	    _pReInfo->_reTimeMult /= fMultFactor;

	if (fMultFactor == 0.0)
	    _pReInfo->_reTimeMult = 1.0;
	else if (replayReplay) {
            // allow for the reversal of time on replays (note this is divider not multipler)
            if (_pReInfo->_reTimeMult > 4.0) {
                GfLogInfo("Reversing Time %f\n", _pReInfo->_reCurTime);
                _pReInfo->_reTimeMult = -4.0;
            } else if (_pReInfo->_reTimeMult < -4.0) {
                GfLogInfo("Correcting Time at %f\n", _pReInfo->_reCurTime);
                _pReInfo->_reTimeMult = 4.0;
            } else if (_pReInfo->_reTimeMult > -0.0625 && _pReInfo->_reTimeMult < 0.0)
                _pReInfo->_reTimeMult = -0.0625;
            else if (_pReInfo->_reTimeMult < 0.0625 && _pReInfo->_reTimeMult > 0.0)
                _pReInfo->_reTimeMult = 0.0625;
	} else if (_pReInfo->_reTimeMult > 64.0)
	    _pReInfo->_reTimeMult = 64.0;
	else if (_pReInfo->_reTimeMult < 0.0625)
	    _pReInfo->_reTimeMult = 0.0625;

	std::ostringstream ossTimeMult;
	ossTimeMult << "Time x" << std::setprecision(2) << 1.0 / _pReInfo->_reTimeMult;
    ReRaceMsgSet(_pReInfo, ossTimeMult.str().c_str(), 5);

	unlock("accelerateTime");
}

void ReSituation::setRaceMessage(const std::string& strMsg, double fLifeTime, bool bBig)
{
	lock("setRaceMessage");

	if (bBig)
		ReRaceMsgSetBig(_pReInfo, strMsg.c_str(), fLifeTime);
	else
		ReRaceMsgSet(_pReInfo, strMsg.c_str(), fLifeTime);
	
	unlock("setRaceMessage");
}

void ReSituation::setPitCommand(int nCarIndex, const tCarPitCmd *pPitCmd)
{
	lock("updateCarPitCmd");

	// Retrieve the car in situation with 'nCarIndex' index.
	//GfLogDebug("ReSituation::updateCarPitCmd(i=%d)\n", nCarIndex);
	tCarElt* pCurrCar = 0;
	for (int nCarInd = 0; nCarInd < _pReInfo->s->_ncars; nCarInd++)
	{
		if (_pReInfo->s->cars[nCarInd]->index == nCarIndex)
		{
			// Found : update its pit command information from pit menu data.
			pCurrCar = _pReInfo->s->cars[nCarInd];
			pCurrCar->_pitFuel = pPitCmd->fuel;
			pCurrCar->_pitRepair = pPitCmd->repair;
			pCurrCar->_pitStopType = pPitCmd->stopType;
			break;
		}
	}
	
	// Compute and set pit time.
	if (pCurrCar)
		ReCarsUpdateCarPitTime(pCurrCar);
	else
		GfLogError("Failed to retrieve car with index %d when computing pit time\n", nCarIndex);
	
	unlock("setRaceMessage");
}

// The race situation updater class ==================================================

// Index of the CPU to use for thread affinity if any and if there are at least 2 ones.
static const int NUserInterfaceCPUId = 0;
static const int NSituationUpdaterCPUId = 1;

void ReSituationUpdater::runOneStep(double deltaTimeIncrement)
{
	tRmInfo* pCurrReInfo = ReSituation::self().data();
	tSituation *s = pCurrReInfo->s;

	// Race messages life cycle management.
	ReRaceMsgManage(pCurrReInfo);
	
	if (NetGetNetwork())
	{
		// Resync clock in case computer falls behind
		if (s->currentTime < 0.0)
		{
			s->currentTime = GfTimeClock() - NetGetNetwork()->GetRaceStartTime();
		}
	}

	if (s->currentTime < -2.0)
	{
		std::ostringstream ossMsg;
		ossMsg << "Race will start in " << -(int)s->currentTime << " seconds";
		ReRaceMsgSetBig(pCurrReInfo, ossMsg.str().c_str());
	}

	//GfLogDebug("ReSituationUpdater::runOneStep: currTime=%.3f\n", s->currentTime);
	if (s->currentTime >= -2.0 && s->currentTime < deltaTimeIncrement - 2.0) {
		ReRaceMsgSetBig(pCurrReInfo, "Ready", 1.0);
		GfLogInfo("Ready.\n");
	} else if (s->currentTime >= -1.0 && s->currentTime < deltaTimeIncrement - 1.0) {
		ReRaceMsgSetBig(pCurrReInfo, "Set", 1.0);
		GfLogInfo("Set.\n");
	} else if (s->currentTime >= 0.0 && s->currentTime < deltaTimeIncrement) {
		ReRaceMsgSetBig(pCurrReInfo, "Go", 1.0);
		GfLogInfo("Go.\n");
	}

	// Update times.
	pCurrReInfo->_reCurTime += deltaTimeIncrement * fabs(pCurrReInfo->_reTimeMult); /* "Real" time */

	if (pCurrReInfo->_reTimeMult > 0)
		s->currentTime += deltaTimeIncrement;
	else
		s->currentTime -= deltaTimeIncrement;

	if (s->currentTime < 0) {
		if (pCurrReInfo->_reTimeMult < 0)
			/* Revert to forward time x1 */
			pCurrReInfo->_reTimeMult = 1;
		else
			/* no simu yet */
			pCurrReInfo->s->_raceState = RM_RACE_PRESTART;
	} else if (pCurrReInfo->s->_raceState == RM_RACE_PRESTART) {
		pCurrReInfo->s->_raceState = RM_RACE_RUNNING;
		s->currentTime = 0.0; /* resynchronize */
		pCurrReInfo->_reLastRobTime = 0.0;
	}

	tTrackLocalInfo *trackLocal = &ReInfo->track->local;
	if (s->currentTime > 0 && trackLocal->timeofdayindex == 9) { //RM_VAL_TIME_24HR
		if (s->_totTime > 0) {
			// Scaled on Total Time
			s->accelTime = 24 * 3600 * s->currentTime / s->_totTime;
		} else {
			// Scaled on Number of Laps that the lead driver has completed
			if (s->cars[0]->_laps > 0 && s->cars[0]->_laps <= s->_totLaps) {
				// prevent issues if lead driver crosses line the wrong way
				if (pCurrReInfo->raceEngineInfo.carInfo->lapFlag)
					s->accelTime = s->cars[0]->_laps - 1;
				else
					s->accelTime = s->cars[0]->_laps - 1 + (s->cars[0]->_distFromStartLine / pCurrReInfo->track->length);

				s->accelTime = 24 * 3600 * s->accelTime / s->_totLaps;
			} else
				s->accelTime = 0;
		}
	} else
		s->accelTime = s->currentTime;

	GfProfStartProfile("rbDrive*");
	GfSchedBeginEvent("raceupdate", "robots");
	if ((s->currentTime - pCurrReInfo->_reLastRobTime) >= RCM_MAX_DT_ROBOTS) {
		s->deltaTime = s->currentTime - pCurrReInfo->_reLastRobTime;
		tRobotItf *robot;
		for (int i = 0; i < s->_ncars; i++) {
			if ((s->cars[i]->_state & RM_CAR_STATE_NO_SIMU) == 0) {
				robot = s->cars[i]->robot;
				if (replayReplay == 0)
					robot->rbDrive(robot->index, s->cars[i], s);
			}
			else if (! (s->cars[i]->_state & RM_CAR_STATE_ENDRACE_CALLED ) && ( s->cars[i]->_state & RM_CAR_STATE_OUT ) == RM_CAR_STATE_OUT )
			{ // No simu, look if it is out
				robot = s->cars[i]->robot;
				if (robot->rbEndRace)
					robot->rbEndRace(robot->index, s->cars[i], s);
				s->cars[i]->_state |= RM_CAR_STATE_ENDRACE_CALLED;
			}
		}
		pCurrReInfo->_reLastRobTime = s->currentTime;
	}
	GfSchedEndEvent("raceupdate", "robots");
	GfProfStopProfile("rbDrive*");


	if (NetGetNetwork())
		ReNetworkOneStep();

	GfProfStartProfile("physicsEngine.update*");
	GfSchedBeginEvent("raceupdate", "physics");
	RePhysicsEngine().updateSituation(s, deltaTimeIncrement);
	bool bestLapChanged = false;
	for (int i = 0; i < s->_ncars; i++)
		ReCarsManageCar(s->cars[i], bestLapChanged);

	GfSchedEndEvent("raceupdate", "physics");
	GfProfStopProfile("physicsEngine.update*");
	
	ReCarsSortCars();

	// Update results if a best lap changed
	if (pCurrReInfo->_displayMode == RM_DISP_MODE_NONE && s->_ncars > 1 && bestLapChanged)
	{
		if (pCurrReInfo->s->_raceType == RM_TYPE_PRACTICE)
			ReUpdatePracticeCurRes(pCurrReInfo->s->cars[0]);
		else if (pCurrReInfo->s->_raceType == RM_TYPE_QUALIF)
			ReUpdateQualifCurRes(pCurrReInfo->s->cars[0]);
	}

	if (replayRecord && pCurrReInfo->s->currentTime >= replayTimestamp) {
		replaySituation(pCurrReInfo);
	}

	#ifdef WEBSERVER
	// webServer lap logger.
	extern TGFCLIENT_API WebServer webServer;
	//Find human cars
	for (int i = 0; i < pCurrReInfo->s->_ncars; i++) {
		if(pCurrReInfo->s->cars[i]->_driverType == RM_DRV_HUMAN){
			//if: 
			//- at least a lap has been done 
			//- a lap is passed 
			//- we have not done the final lap
			// then log it to the webServer
			if(pCurrReInfo->s->cars[i]->_laps > 1 && pCurrReInfo->s->cars[i]->_laps > webServer.previousLaps && webServer.raceEndSent==false){
				
				//remember the current number of laps for next cicle
				webServer.previousLaps = pCurrReInfo->s->cars[i]->_laps;

				//GfLogInfo("############rain: %i, ",trackLocal->rain); //0=no 1=little 2=medium 3=heawy
				//GfLogInfo("############WATER: %i, ",trackLocal->water); //0=no 1=little  2=medium  3=heawy


				//send the lap info to the server
				webServer.sendLap(
					webServer.raceId,						//race_id
					pCurrReInfo->s->cars[i]->_lastLapTime,	//laptime
					pCurrReInfo->s->cars[i]->_fuel,			//car remaining fuel at the end of the lap
					pCurrReInfo->s->cars[i]->_pos,			//car position
					trackLocal->water						//level of water on track
					//VERSION_LONG, 						//speed dreams version
					//pCurrReInfo->s->cars[i]->_name, 		//player name
					//pCurrReInfo->s->cars[i]->_skillLevel,	//player skill level: 0 rokie/ 1 amateour/ 2 semi-pro/ 3 pro
					//pCurrReInfo->track->name,				//track name
					//strName,								//os info
					//pCurrReInfo->s->_raceType,			//type of race: 0 practice/ 1 qualify/ 2 race
					//pCurrReInfo->s->cars[i]->_carName,	//car name
					//pCurrReInfo->s->cars[i]->_category,	//car category
				);
			}
			
			//if we have already done the last lap but we have not yet sent the raceEnd comunication to the webserver: do it!
			if(pCurrReInfo->s->cars[i]->_remainingLaps < 0 && webServer.raceEndSent == false){
				//send race data
				webServer.sendRaceEnd (
					webServer.raceId,
					ReInfo->s->cars[i]->_pos				//car end position,
				);
			}
		}
	}
	webServer.updateAsyncStatus();
	#endif //WEBSERVER	

}

int ReSituationUpdater::threadLoop(void* pUpdater)
{
	return static_cast<ReSituationUpdater*>(pUpdater)->threadLoop();
}

int ReSituationUpdater::threadLoop()
{
	// Wait delay for each loop, from bRunning value (index 0 = false, 1 = true).
	static const unsigned KWaitDelayMS[2] = { 1, (unsigned)(RCM_MAX_DT_SIMU * 1000 / 10) };

	// Termination flag.
	bool bEnd = false;

	// Local state (false = paused, true = simulating).
	bool bRunning = false;

	// Current real time.
	double realTime;
	
	// Apply thread affinity to the current = situation updater thread if specified.
	// Note: No need to reset the affinity, as the thread is just born.
	if (_bThreadAffinity)
		GfSetThreadAffinity(NSituationUpdaterCPUId);

	tRmInfo* pCurrReInfo = ReSituation::self().data();

	GfLogInfo("SituationUpdater thread is started.\n");
	
	do
	{
		// Let's make current step the next one (update).
		// 1) Lock the race engine data.
		ReSituation::self().lock("ReSituationUpdater::threadLoop");

		// 2) Check if time to terminate has come.
		if (_bTerminate)
			
			bEnd = true;
		
		// 3) If not time to terminate, and running, do the update job.
		else if (pCurrReInfo->_reRunning)
		{
			if (!bRunning)
			{
				bRunning = true;
				GfLogInfo("SituationUpdater thread is running.\n");
			}
			
			realTime = GfTimeClock();
		
			GfProfStartProfile("reOneStep*");
		
			while (pCurrReInfo->_reRunning
				   && ((realTime - pCurrReInfo->_reCurTime) > RCM_MAX_DT_SIMU))
			{
				// One simu + robots (if any) step
				runOneStep(RCM_MAX_DT_SIMU);
			}
		
			GfProfStopProfile("reOneStep*");
		
			// Send car physics to network if needed
			if (NetGetNetwork())
				NetGetNetwork()->SendCarControlsPacket(pCurrReInfo->s);
		}
		
		// 3) If not time to terminate, and not running, do nothing.
		else
		{
			if (bRunning)
			{
				bRunning = false;
				GfLogInfo("SituationUpdater thread is paused.\n");
			}
		}
			
		// 4) Unlock the race engine data.
		ReSituation::self().unlock("ReSituationUpdater::threadLoop");
		
		// 5) Let the CPU take breath if possible (but after unlocking data !).
		SDL_Delay(KWaitDelayMS[(int)bRunning]);
	}
	while (!bEnd);

	GfLogInfo("SituationUpdater thread has been terminated.\n");
	
	return 0;
}

ReSituationUpdater::ReSituationUpdater()
: _fSimuTick(RCM_MAX_DT_SIMU), _fOutputTick(0), _fLastOutputTime(0)

{
	// Save the race engine info (state + situation) pointer for the current step.
	tRmInfo* pCurrReInfo = ReSituation::self().data();
	_nInitDrivers = pCurrReInfo->s->_ncars;

	// Determine if we have a dedicated separate thread or not
	// (according to the user settings, and the actual number of CPUs).
	std::ostringstream ossConfFile;
	ossConfFile << GfLocalDir() << RACE_ENG_CFG;
	void *paramHandle =
		GfParmReadFile(ossConfFile.str().c_str(), GFPARM_RMODE_REREAD | GFPARM_RMODE_CREAT);
	const char* pszMultiThreadScheme =
		GfParmGetStr(paramHandle, RM_SECT_RACE_ENGINE, RM_ATTR_MULTI_THREADING, RM_VAL_AUTO);

	if (!strcmp(pszMultiThreadScheme, RM_VAL_OFF))
		_bThreaded = false;
	else if (!strcmp(pszMultiThreadScheme, RM_VAL_ON))
		_bThreaded = true;
	else // Can't be anything else than RM_VAL_AUTO
		_bThreaded = GfGetNumberOfCPUs() > 1;

	// Determine if we apply some thread affinity or not (according to the user settings).
	const char* pszThreadAffinityScheme =
		GfParmGetStr(paramHandle, RM_SECT_RACE_ENGINE, RM_ATTR_THREAD_AFFINITY, RM_VAL_OFF);
	_bThreadAffinity = strcmp(pszThreadAffinityScheme, RM_VAL_ON) == 0;

	GfParmReleaseHandle(paramHandle);

	// Apply thread affinity to the current = main = graphics thread
	// (and don't forget to reset it when specified :
	//  user settings may have changed since last race).
	GfSetThreadAffinity(_bThreadAffinity ? NUserInterfaceCPUId : GfAffinityAnyCPU);

	// Disable movie capture feature in dual-threaded mode
	// (only available in one-threaded mode, because of a non-"real-time" behaviour).
	// if (_bThreaded && pCurrReInfo->movieCapture.enabled)
	// {
	// 	pCurrReInfo->movieCapture.enabled = 0;
	// 	GfLogInfo("Movie capture disabled (not implemented in multi-threaded mode)\n");
	// }
	
	// Initialize termination flag.
	_bTerminate = false;


	if (_bThreaded)
	{
		// Initialize the race engine info (state + situation) pointer for the previous step.
		_pPrevReInfo = initSituation(pCurrReInfo);

		// Activate the thread-safe mode for the the race engine info.
		ReSituation::self().setThreadSafe(true);
		
		// Create and start the updater thread.
#if SDL_MAJOR_VERSION >= 2
		_pUpdateThread = SDL_CreateThread(ReSituationUpdater::threadLoop,"Update_thread",this);
#else
		_pUpdateThread = SDL_CreateThread(ReSituationUpdater::threadLoop, this);
#endif
	}
	else
	{
		_pPrevReInfo = 0;
		_pUpdateThread = 0;
	}

	GfLogInfo("SituationUpdater initialized (%sseparate thread, CPU affinity %s).\n",
	      (_bThreaded ? "" : "no "), (_bThreadAffinity ? "On" : "Off"));
}

ReSituationUpdater::~ReSituationUpdater()
{
	terminate(); // In case not already done.

	if (_bThreaded)
	{
		// Deactivate the thread-safe mode for the the race engine info.
		ReSituation::self().setThreadSafe(false);
		
		if (_pPrevReInfo)
			freezSituation(_pPrevReInfo);
	}
}

void ReSituationUpdater::start()
{
	int i;
	tRobotItf *robot;
	tSituation *s = ReInfo->s;

	GfLogInfo("Starting race engine.\n");

	// Lock the race engine data.
	ReSituation::self().lock("ReSituationUpdater::start");

	// Allow robots to run their start function
	for (i = 0; i < s->_ncars; i++) {
		robot = s->cars[i]->robot;
		if (robot->rbResumeRace)
			robot->rbResumeRace(robot->index, s->cars[i], s);
	}

	// Set the running flags.
    ReSituation::self().data()->_reRunning = 1;
    ReSituation::self().data()->s->_raceState &= ~RM_RACE_PAUSED;
	ReSituation::self().data()->_reState = RE_STATE_RACE;

	// Resynchronize simulation time.
    ReSituation::self().data()->_reCurTime = GfTimeClock() - RCM_MAX_DT_SIMU;
	
	// Unlock the race engine data.
	ReSituation::self().unlock("ReSituationUpdater::start");
}

void ReSituationUpdater::stop()
{
	GfLogInfo("Stopping race engine.\n");

	// Lock the race engine data.
	ReSituation::self().lock("ReSituationUpdater::stop");

	// Reset the running flags.
	ReSituation::self().data()->_reRunning = 0;
	ReSituation::self().data()->s->_raceState |= RM_RACE_PAUSED;
		
	// Unlock the race engine data.
	ReSituation::self().unlock("ReSituationUpdater::stop");
}

int ReSituationUpdater::terminate()
{
	int status = 0;
	
	GfLogInfo("Terminating situation updater.\n");

	/* need to ensure the last record gets writeen */
	tRmInfo* pCurrReInfo = ReSituation::self().data();
	if (replayRecord) {
		replaySituation(pCurrReInfo);
		GfLogInfo("Last replay entry done.\n");
	}

	// Lock the race engine data.
	ReSituation::self().lock("ReSituationUpdater::terminate");

	// Set the death flag.
	_bTerminate = true;
	
	// Unlock the race engine data.
	ReSituation::self().unlock("ReSituationUpdater::terminate");
	
	// Wait for the thread to gracefully terminate if any.
	if (_bThreaded)
	{
		SDL_WaitThread(_pUpdateThread, &status);
		_pUpdateThread = 0;
 	}

	return status;
}

tRmInfo* ReSituationUpdater::initSituation(const tRmInfo* pSource)
{
	tRmInfo* pTarget;

	// Allocate main structure (level 0).
	pTarget = (tRmInfo *)calloc(1, sizeof(tRmInfo));

	// Allocate variable level 1 structures.
	pTarget->carList = (tCarElt*)calloc(_nInitDrivers, sizeof(tCarElt));
	pTarget->s = (tSituation *)calloc(1, sizeof(tSituation));
	pTarget->rules = (tRmCarRules*)calloc(_nInitDrivers, sizeof(tRmCarRules));

	// Assign level 1 constants.
	pTarget->track = pSource->track; // Only read during race.
	pTarget->params = pSource->params; // Never read/written during race.
	pTarget->mainParams = pSource->mainParams; // Never read/written during race.
	pTarget->results = pSource->results; // Never read/written during race.
	pTarget->mainResults = pSource->mainResults; // Never read/written during race.
	pTarget->robModList = pSource->robModList; // Not used / written by updater.

	// Assign level 2 constants and initialize lists in carList field.
	for (int nCarInd = 0; nCarInd < _nInitDrivers; nCarInd++)
	{
		tCarElt* pTgtCar = &pTarget->carList[nCarInd];
		tCarElt* pSrcCar = &pSource->carList[nCarInd];

		pTgtCar->_curSplitTime = (double*)malloc(sizeof(double) * (pSource->track->numberOfSectors - 1));
		pTgtCar->_bestSplitTime = (double*)malloc(sizeof(double) * (pSource->track->numberOfSectors - 1));

		GF_TAILQ_INIT(&(pTgtCar->_penaltyList)); // Not used by the graphics engine.

		memcpy(&pTgtCar->info, &pSrcCar->info, sizeof(tInitCar)); // Not changed + only read during race.
		memcpy(&pTgtCar->priv, &pSrcCar->priv, sizeof(tPrivCar)); // Partly only read during race ; other copied in vars below.
		pTgtCar->robot = pSrcCar->robot; // Not changed + only read during race.
	}

	// Allocate level 2 structures in s field.
	pTarget->s->cars = (tCarElt **)calloc(_nInitDrivers, sizeof(tCarElt *));

	// Allocate level 2 structures in raceEngineInfo field.
	pTarget->_reCarInfo = (tReCarInfo*)calloc(_nInitDrivers, sizeof(tReCarInfo));
		
	// Assign level 2 constants in raceEngineInfo field.
	pTarget->_reParam = pSource->_reParam; // Not used / written by updater.
	pTarget->_reFilename = pSource->_reFilename; // Not used during race.
	pTarget->_reName = pSource->_reName; // Not changed + only read during race.
	pTarget->_reRaceName = pSource->_reRaceName; // Not changed + only read during race.

	return pTarget;
}

tRmInfo* ReSituationUpdater::copySituation(tRmInfo*& pTarget, const tRmInfo* pSource)
{
	tCarElt* pTgtCar;
	tCarElt* pSrcCar;

	// Copy variable data from source to target.
	// I) pSource->carList
	for (int nCarInd = 0; nCarInd < _nInitDrivers; nCarInd++)
	{
		pTgtCar = &pTarget->carList[nCarInd];
		pSrcCar = &pSource->carList[nCarInd];

		// 1) index
		pTgtCar->index = pSrcCar->index;

		// 2) pub (raw mem copy)
		memcpy(&pTgtCar->pub, &pSrcCar->pub, sizeof(tPublicCar));

		// 3) race (field by field copy)
		// 3a) all fields except _penaltyList and _pit.
		pTgtCar->_bestLapTime = pSrcCar->_bestLapTime;
		memcpy(pTgtCar->_bestSplitTime, pSrcCar->_bestSplitTime,
			   sizeof(double) * (pSource->track->numberOfSectors - 1));
		pTgtCar->_deltaBestLapTime = pSrcCar->_deltaBestLapTime;
		pTgtCar->_curLapTime = pSrcCar->_curLapTime;
		memcpy(pTgtCar->_curSplitTime, pSrcCar->_curSplitTime,
			   sizeof(double) * (pSource->track->numberOfSectors - 1));
		pTgtCar->_lastLapTime = pSrcCar->_lastLapTime;
		pTgtCar->_curTime = pSrcCar->_curTime;
		pTgtCar->_topSpeed = pSrcCar->_topSpeed;
		pTgtCar->_laps = pSrcCar->_laps;
		pTgtCar->_bestLap = pSrcCar->_bestLap;
		pTgtCar->_nbPitStops = pSrcCar->_nbPitStops;
		pTgtCar->_remainingLaps = pSrcCar->_remainingLaps;
		pTgtCar->_pos = pSrcCar->_pos;
		pTgtCar->_timeBehindLeader = pSrcCar->_timeBehindLeader;
		pTgtCar->_lapsBehindLeader = pSrcCar->_lapsBehindLeader;
		pTgtCar->_timeBehindPrev = pSrcCar->_timeBehindPrev;
		pTgtCar->_timeBeforeNext = pSrcCar->_timeBeforeNext;
		pTgtCar->_distRaced = pSrcCar->_distRaced;
		pTgtCar->_distFromStartLine = pSrcCar->_distFromStartLine;
		pTgtCar->_currentSector = pSrcCar->_currentSector;
		pTgtCar->_scheduledEventTime = pSrcCar->_scheduledEventTime;
		//pTgtCar->_pit ... // Not used by the graphics engine (robots and situ. updater only).
		pTgtCar->_event = pSrcCar->_event;

		// Note: Commented-out because not used by the graphics engine (situ. updater only).
		// 3b) Clear target penalty list, and then copy the source one into it.
		//     TODO if profiling shows its usefull : optimize (reuse already allocated entries
		//       to minimize mallocs / frees).
		//tCarPenalty *penalty;
		//while ((penalty = GF_TAILQ_FIRST(&(pTgtCar->_penaltyList))))
		//{
		//	GfLogDebug("ReSituationCopy(car #%d) : Clearing penalty %p\n",
		//			   pSrcCar->index, penalty);
		//	GF_TAILQ_REMOVE (&(pTgtCar->_penaltyList), penalty, link);
		//	free(penalty);
        //}
		//GF_TAILQ_INIT(&(pTgtCar->_penaltyList));
		//penalty = GF_TAILQ_FIRST(&(pSrcCar->_penaltyList));
		//while (penalty)
		//{
		//	tCarPenalty *newPenalty = (tCarPenalty*)malloc(sizeof(tCarPenalty));
		//	newPenalty->penalty = penalty->penalty;
		//      newPenalty->lapToClear = penalty->lapToClear;
		//      GfLogDebug("ReSituationCopy(car #%d) : Copying penalty %p to %p\n",
		//                         pSrcCar->index, penalty, newPenalty);
		//	GF_TAILQ_INSERT_TAIL(&(pTgtCar->_penaltyList), newPenalty, link);
		//	penalty = GF_TAILQ_NEXT(penalty, link);
		//}

		// 4) priv (field by field copy)
		memcpy(&pTgtCar->priv.wheel[0], &pSrcCar->priv.wheel[0], 4*sizeof(tWheelState));
		memcpy(&pTgtCar->priv.corner[0], &pSrcCar->priv.corner[0], 4*sizeof(tPosd));
		pTgtCar->_gear = pSrcCar->_gear;
		pTgtCar->_fuel = pSrcCar->_fuel;
		pTgtCar->_fuelTotal = pSrcCar->_fuelTotal;
		pTgtCar->_fuelInstant = pSrcCar->_fuelInstant;
		pTgtCar->_enginerpm = pSrcCar->_enginerpm;
		memcpy(&pTgtCar->priv.skid[0], &pSrcCar->priv.skid[0], 4*sizeof(tdble));
		memcpy(&pTgtCar->priv.reaction[0], &pSrcCar->priv.reaction[0], 4*sizeof(tdble));
		pTgtCar->_collision = pSrcCar->_collision;
		pTgtCar->_smoke = pSrcCar->_smoke;
		pTgtCar->_normal = pSrcCar->_normal;
		pTgtCar->_coll2Pos = pSrcCar->_coll2Pos;
		pTgtCar->_dammage = pSrcCar->_dammage;
		//pTgtCar->_debug = pSrcCar->_debug; // Ever used anywhere ?
		pTgtCar->priv.collision_state = pSrcCar->priv.collision_state;
		//pTgtCar->_memoryPool ...; // ???? Memory pool copy ??????
		pTgtCar->_dashboardActiveItem = pSrcCar->_dashboardActiveItem;
		/* NOTE: dashboardInstant, dashboardInstantNb, dashboardRequest, dashboardRequestNb
		 * are not copied, because they are initialized once and never changed again.
		 * An dashboardInstant and dashboardRequest's tCarSetupItem pointers
		 * are not updated, so they always point to the targets setup field.
		 * This is safe until the graphics engine does not start to change car setups.
		 */ 

		// 5) ctrl (raw mem copy)
		memcpy(&pTgtCar->ctrl, &pSrcCar->ctrl, sizeof(tCarCtrl));

		// 6) pitcmd (raw mem copy)
		memcpy(&pTgtCar->pitcmd, &pSrcCar->pitcmd, sizeof(tCarPitCmd));

		// 7) next : ever used anywhere ? Seems not.
		//struct CarElt	*next;
	}
	
	// II) pSource->s
	pTarget->s->raceInfo = pSource->s->raceInfo;
	pTarget->s->deltaTime = pSource->s->deltaTime;
	pTarget->s->currentTime = pSource->s->currentTime;
	pTarget->s->accelTime = pSource->s->accelTime;
	pTarget->s->nbPlayers = pSource->s->nbPlayers;
	for (int nCarInd = 0; nCarInd < _nInitDrivers; nCarInd++)
		pTarget->s->cars[nCarInd] =
			pTarget->carList + (pSource->s->cars[nCarInd] - pSource->carList);
	
	// III) pSource->rules (1 int per driver) // Not used by the graphics engine (situ. updater only).
	//memcpy(pTarget->rules, pSource->rules, _nInitDrivers*sizeof(tRmCarRules));
	
	// IV) pSource->raceEngineInfo
	//     TODO: Make _reMessage and _reBigMessage inline arrays inside raceEngineInfo
	//           to avoid strdups (optimization) ?
	pTarget->_reState = pSource->_reState;
	memcpy(pTarget->_reCarInfo, pSource->_reCarInfo, _nInitDrivers*sizeof(tReCarInfo));
	pTarget->_reCurTime = pSource->_reCurTime;
	pTarget->_reTimeMult = pSource->_reTimeMult;
	pTarget->_reRunning = pSource->_reRunning;
	pTarget->_reLastRobTime = pSource->_reLastRobTime;
	pTarget->_displayMode = pSource->_displayMode;
	if (pTarget->_reMessage)
	{
		free(pTarget->_reMessage);
		pTarget->_reMessage = 0;
	}
	if (pSource->_reMessage)
	{
		free(pTarget->_reMessage);
		pTarget->_reMessage = strdup(pSource->_reMessage);
	}
	pTarget->_reMessageEnd = pSource->_reMessageEnd;
	if (pTarget->_reBigMessage)
	{
		free(pTarget->_reBigMessage);
		pTarget->_reBigMessage = 0;
	}
	if (pSource->_reBigMessage)
	{
		free(pTarget->_reBigMessage);
		pTarget->_reBigMessage = strdup(pSource->_reBigMessage);
	}
	pTarget->_reBigMessageEnd = pSource->_reBigMessageEnd;

	if (pSource->_rePitRequester)
	{
		//GfLogDebug("ReSituationCopy: Pit menu request forwarded.\n");
		pTarget->_rePitRequester =
			pTarget->carList + (pSource->_rePitRequester - pSource->carList);
	}
	else
		pTarget->_rePitRequester = 0;

	return pTarget;
}

void ReSituationUpdater::replaySituation(tRmInfo*& pSource)
{
#ifdef THIRD_PARTY_SQLITE3
	char command[200];
	int result;

	tReplayElt dummyTarget;
	tReplayElt* pTgtCar;
	tCarElt* pSrcCar;

	if (!replayDB) return;
	
	// Do everything in 1 transaction for speed
	sqlite3_exec(replayDB, "BEGIN TRANSACTION", NULL, NULL, NULL);

	for (int nCarInd = 0; nCarInd < _nInitDrivers; nCarInd++)
	{
		pTgtCar = &dummyTarget;
		pSrcCar = &pSource->carList[nCarInd];

		// Assemble the data we record
		pTgtCar->currentTime = pSource->s->currentTime;

		memcpy(&pTgtCar->info, &pSrcCar->info, sizeof(tInitCar));
		memcpy(&pTgtCar->pub, &pSrcCar->pub, sizeof(tPublicCar));
		memcpy(&pTgtCar->race, &pSrcCar->race, sizeof(tCarRaceInfo));
		memcpy(&pTgtCar->priv, &pSrcCar->priv, sizeof(tPrivCar));
		memcpy(&pTgtCar->ctrl, &pSrcCar->ctrl, sizeof(tCarCtrl));
		memcpy(&pTgtCar->pitcmd, &pSrcCar->pitcmd, sizeof(tCarPitCmd));

		// and write to database
		sprintf(command, "INSERT INTO car%d (timestamp, lap, datablob) VALUES (%f, %d, ?)", nCarInd, 
			pSource->s->currentTime, pSrcCar->_laps);

 		result = sqlite3_prepare_v2(replayDB, command, -1, &replayBlobs[nCarInd], 0);
		if (result) {
			GfLogInfo("Replay: Unable to instert into table car%d: %s\n", nCarInd, sqlite3_errmsg(replayDB));
		} else {
			/* push binary blob into database */
			result = sqlite3_bind_blob(replayBlobs[nCarInd], 1, (void *) &dummyTarget, sizeof(dummyTarget), SQLITE_STATIC);
			result = sqlite3_step(replayBlobs[nCarInd]);
		}
		//GfLogInfo("Replay wrote car%d = time %f, lap %d\n", nCarInd, pSource->s->currentTime, pSrcCar->_laps);
	}

	sqlite3_exec(replayDB, "END TRANSACTION", NULL, NULL, NULL);
	replayTimestamp = pSource->s->currentTime + (1/(float)replayRecord);
#endif
}

void ReSituationUpdater::ghostcarSituation(tRmInfo*& pTarget)
{
#ifdef THIRD_PARTY_SQLITE3
	tCarElt *pTgtCar;
	tReplayElt *pSrcCar, *pSrc2Car;
	int result;

	if (!replayDB) return;

	if (ghostcarActive) {
		if (pTarget->s->currentTime - ghostcarTimeOffset >= nextGhostcarData.currentTime) {
			result = sqlite3_step(ghostcarBlob);
			if (result == SQLITE_ROW) {
				curGhostcarData = nextGhostcarData;
				memcpy(&nextGhostcarData, sqlite3_column_blob(ghostcarBlob, 0), sizeof(tReplayElt));
			} else {
				// don't do anything untill next lap is started
				// GfLogInfo("Ghostcar completed lap\n");
				ghostcarActive = 0;
				return;
			}

			// Ghostcar uses the last carElt
			pTgtCar = &pTarget->carList[_nInitDrivers];
			pSrcCar = &curGhostcarData;

			// GfLogInfo("Read ghostcar data: time %f %f, lap %d\n", pTarget->s->currentTime, pSrcCar->currentTime, pSrcCar->_laps);

			// Really this should only be read once at start of race
			memcpy(&pTgtCar->race, &pSrcCar->race, sizeof(tCarRaceInfo));
			pTgtCar->race.pit = NULL;

			// hack to fix trkpos
			pSrcCar->pub.trkPos = pTgtCar->pub.trkPos;

			memcpy(&pTgtCar->pub, &pSrcCar->pub, sizeof(tPublicCar));
			memcpy(&pTgtCar->info, &pSrcCar->info, sizeof(tInitCar));

			for(int i=0; i < 4; i++) {
				pTgtCar->priv.wheel[i] = pSrcCar->priv.wheel[i];
				pTgtCar->priv.wheel[i].seg = NULL;
			}
			pTgtCar->priv.gear = pSrcCar->priv.gear;
			pTgtCar->priv.fuel = pSrcCar->priv.fuel;
			pTgtCar->priv.enginerpm = pSrcCar->priv.enginerpm;
			pTgtCar->priv.dammage = pSrcCar->priv.dammage;
		}

		if (pTarget->s->currentTime - ghostcarTimeOffset < nextGhostcarData.currentTime) {
			// Interpolate position in between records
			double timeFrac;
			double yaw, roll, pitch;

			pTgtCar = &pTarget->carList[_nInitDrivers];
			pSrcCar = &curGhostcarData;
			pSrc2Car = &nextGhostcarData;

			timeFrac = (pTarget->s->currentTime - ghostcarTimeOffset - curGhostcarData.currentTime) /
					(nextGhostcarData.currentTime - curGhostcarData.currentTime);

			pTgtCar->_pos_X = pSrcCar->_pos_X + (pSrc2Car->_pos_X - pSrcCar->_pos_X) * timeFrac;
			pTgtCar->_pos_Y = pSrcCar->_pos_Y + (pSrc2Car->_pos_Y - pSrcCar->_pos_Y) * timeFrac;
			pTgtCar->_pos_Z = pSrcCar->_pos_Z + (pSrc2Car->_pos_Z - pSrcCar->_pos_Z) * timeFrac - pTgtCar->_statGC_z;

			yaw = pSrc2Car->_yaw;
			roll = pSrc2Car->_roll;
			pitch = pSrc2Car->_pitch;

			// assumes that these can't change at high rate
			if (yaw < pSrcCar->_yaw - PI)
				yaw += 2 * PI;
			else if (yaw > pSrcCar->_yaw + PI)
				yaw -= 2 * PI;

			if (roll < pSrcCar->_roll - PI)
				roll += 2 * PI;
			else if (roll > pSrcCar->_roll + PI)
				roll -= 2 * PI;

			if (pitch < pSrcCar->_pitch - PI)
				pitch += 2 * PI;
			else if (pitch > pSrcCar->_pitch + PI)
				pitch -= 2 * PI;

			pTgtCar->_yaw = pSrcCar->_yaw + (yaw - pSrcCar->_yaw) * timeFrac;
			pTgtCar->_roll = pSrcCar->_roll + (roll - pSrcCar->_roll) * timeFrac;
			pTgtCar->_pitch = pSrcCar->_pitch + (pitch - pSrcCar->_pitch) * timeFrac;

			sgMakeCoordMat4(pTgtCar->pub.posMat, pTgtCar->_pos_X, pTgtCar->_pos_Y, pTgtCar->_pos_Z,
					(tdble) RAD2DEG(pTgtCar->_yaw), (tdble) RAD2DEG(pTgtCar->_roll),
					(tdble) RAD2DEG(pTgtCar->_pitch));
		}
	}
#endif
}

void ReSituationUpdater::freezSituation(tRmInfo*& pSituation)
{
	if (pSituation)
	{
		// carList
		if (pSituation->carList)
		{
			for (int nCarInd = 0; nCarInd < _nInitDrivers; nCarInd++)
			{
				tCarElt* pTgtCar = &pSituation->carList[nCarInd];
		
				tCarPenalty *penalty;
				while ((penalty = GF_TAILQ_FIRST(&(pTgtCar->_penaltyList)))
					   != GF_TAILQ_END(&(pTgtCar->_penaltyList)))
				{
					GF_TAILQ_REMOVE (&(pTgtCar->_penaltyList), penalty, link);
					free(penalty);
				}
				free(pTgtCar->_curSplitTime);
				free(pTgtCar->_bestSplitTime);
			}
		
			free(pSituation->carList);
		}
		
		// s
		if (pSituation->s)
			free(pSituation->s);

		// rules
		if (pSituation->rules)
			free(pSituation->rules);

		// raceEngineInfo
		if (pSituation->_reMessage)
			free(pSituation->_reMessage);
		if (pSituation->_reBigMessage)
			free(pSituation->_reBigMessage);
		if (pSituation->_reCarInfo)
			free(pSituation->_reCarInfo);
		
		free(pSituation);
		pSituation = 0;
	}
}

void ReSituationUpdater::acknowledgeEvents()
{
	tRmInfo* pCurrReInfo = ReSituation::self().data();

	// Acknowlegde collision events for each car.
	for (int nCarInd = 0; nCarInd < pCurrReInfo->s->_ncars; nCarInd++)
	{
		tCarElt* pCar = pCurrReInfo->s->cars[nCarInd];
		
		//if (pCar->priv.collision)
		//	GfLogDebug("Reset collision state of car #%d (was 0x%X)\n",
		//			   nCarInd, pCar->priv.collision);
		pCar->priv.collision = 0;
		
		// Note: This one is only for SimuV3, and not yet used actually
		// (WIP on collision code issues ; see simuv3/collide.cpp).
		pCar->priv.collision_state.collision_count = 0;
	}

	// Acknowlegde human pit event if any (update the car pit command in current situation
	// with the one modified by the Pit menu in previous situation).
	if (_pPrevReInfo && _pPrevReInfo->_rePitRequester)
	{
		//GfLogDebug("ReSituationAcknowlegdeEvents: Pit menu request cleared.\n");
		pCurrReInfo->_rePitRequester = 0;
	}
}

tRmInfo* ReSituationUpdater::getPreviousStep()
{
	if (!_bThreaded)
	{
		// No multi-threading : no need to really copy.
		_pPrevReInfo = ReSituation::self().data();

		// Acknowledge the collision and human pit events occurred
		// since the last graphics update : we know now that they all have been processed
		// or at least being processed by the graphics engine / menu system
		// (the main thread's job).
		acknowledgeEvents();
	}
	else
	{
		// Lock the race engine data.
		if (!ReSituation::self().lock("ReSituationUpdater::getPreviousStep"))
			return 0;

		// Get the situation data.
		copySituation(_pPrevReInfo, ReSituation::self().data());

		// Acknowledge the collision and human pit events (see above).
		acknowledgeEvents();

		// Unlock the race engine data.
		if (!ReSituation::self().unlock("ReSituationUpdater::getPreviousStep"))
			return 0;
	}

	if (replayRecord && _pPrevReInfo->s->currentTime >= replayTimestamp) {
		replaySituation(_pPrevReInfo);
	}

	if (replayRecord)
		ghostcarSituation(_pPrevReInfo);

	return _pPrevReInfo;
}

// This member function decorates the situation updater as a normal function,
// thus hiding the possible separate thread behind to the main updater. 
void ReSituationUpdater::computeCurrentStep()
{
	// Nothing to do if actually threaded :
	// the updater thread is already doing the job on his side.
	if (_bThreaded)
		return;

	// Non-threaded mode.
	GfProfStartProfile("reOneStep*");
			
	tRmInfo* pCurrReInfo = ReSituation::self().data();

	// Stable but slowed-down frame rate mode.
	if (_fOutputTick > 0)
	{
		while ((pCurrReInfo->_reCurTime - _fLastOutputTime) < _fOutputTick)

			runOneStep(_fSimuTick);

		_fLastOutputTime = pCurrReInfo->_reCurTime;
	}

	// Real-time but variable frame rate mode.
	else
	{
		const double t = GfTimeClock();
		
		while (pCurrReInfo->_reRunning && ((t - pCurrReInfo->_reCurTime) > RCM_MAX_DT_SIMU))
				
			runOneStep(_fSimuTick);
	}

	GfProfStopProfile("reOneStep*");
		
	// Send car physics to network if needed
	if (NetGetNetwork())
		NetGetNetwork()->SendCarControlsPacket(pCurrReInfo->s);
}

bool ReSituationUpdater::setSchedulingSpecs(double fSimuRate, double fOutputRate)
{
	// Not supported if actually threaded : we can't control the updater thread output rate.
	if (_bThreaded && fOutputRate > 0)
		return false; // Specs not changed.

	// Can't output faster than the simu, even in non-real time !
	if (fOutputRate > fSimuRate)
		fOutputRate = fSimuRate;

	if (fOutputRate > 0)
	{
		_fOutputTick = 1.0 / fOutputRate;
		_fLastOutputTime = GfTimeClock() - _fOutputTick;
	}
	else
		_fOutputTick = 0;

	_fSimuTick = 1.0 / fSimuRate;

	return true;
}
