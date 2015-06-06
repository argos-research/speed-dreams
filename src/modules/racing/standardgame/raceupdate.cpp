/***************************************************************************

    file        : raceupdate.cpp
    created     : Sat Nov 23 09:05:23 CET 2002
    copyright   : (C) 2002 by Eric Espie
    email       : eric.espie@torcs.org 
    version     : $Id: raceupdate.cpp 5081 2012-12-30 18:24:16Z pouillot $

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
    		Race engine core : physics engine, drivers and graphics direction
    @author	<a href=mailto:eric.espie@torcs.org>Eric Espie</a>
    @version	$Id: raceupdate.cpp 5081 2012-12-30 18:24:16Z pouillot $
*/

#include <cstdlib>
#include <sstream>

#include <portability.h>
#include <robot.h>
#include <raceman.h>

#include "standardgame.h"

#include "raceresults.h"
#include "racesimusimu.h"
#include "racesituation.h"
#include "racenetwork.h"

#include "raceupdate.h"


// The situation updater instance (initialized in ReInitUpdaters).
static ReSituationUpdater* situationUpdater = 0;


// The main updater class ============================================================
class reMainUpdater
{
public:
	
	//! Constructor.
	reMainUpdater(ReSituationUpdater* pSituUpdater);

	//! The real updating funtion.
	int operator()(void);

	//! Accessor to the output situation (read-only, as always overwritten).
	const tRmInfo* data() const { return _pReInfo; };

private:

	//! The last step situation got from the situationUpdater.
	tRmInfo* _pReInfo;

	//! The associated situation updater.
	ReSituationUpdater* _pSituationUpdater;
};

// The main updater instance (intialized in ReInitUpdaters).
static reMainUpdater* mainUpdater = 0;


reMainUpdater::reMainUpdater(ReSituationUpdater* pSituUpdater)
: _pReInfo(pSituUpdater->getPreviousStep()), _pSituationUpdater(pSituUpdater)
{
}

int reMainUpdater::operator()(void)
{
	GfProfStartProfile("ReUpdate");

	if (_pReInfo->_displayMode & RM_DISP_MODE_SIMU_SIMU)
	{
		// Simulate the simulation.
		ReSimuSimu();
	}
	else if (_pReInfo->_displayMode & RM_DISP_MODE_NORMAL)
	{
		// Get the situation for the previous step.
		GfSchedBeginEvent("raceupdate", "situCopy");
		_pReInfo = situationUpdater->getPreviousStep();
		GfSchedEndEvent("raceupdate", "situCopy");
		
		// Compute the situation for the current step (mono-threaded race engine)
		// or do nothing (dual-threaded race engine : the updater thread does the job itself).
		_pSituationUpdater->computeCurrentStep();
	}
	else // No other choice than RM_DISP_MODE_NONE
	{
		_pSituationUpdater->runOneStep(RCM_MAX_DT_SIMU);
    }

	ReNetworkCheckEndOfRace();

	GfProfStopProfile("ReUpdate");

    return RM_ASYNC;
}

// Public C API of raceupdate =========================================================

void ReInitUpdaters()
{
	ReInfo->_reRunning = 0;

 	if (!situationUpdater)
 		situationUpdater = new ReSituationUpdater();

 	if (!mainUpdater)
 		mainUpdater = new reMainUpdater(situationUpdater);

	// TODO: Rework, as Currently broken, after the race engine / user interface separation.
	// Configure schedule spy                        (nMaxEv, ignoreDelay)
	GfSchedConfigureEventLog("raceupdate", "graphics", 10000, 0.0);
	GfSchedConfigureEventLog("raceupdate", "situCopy", 10000, 0.0);
	GfSchedConfigureEventLog("raceupdate", "robots",   10000, 0.0);
	GfSchedConfigureEventLog("raceupdate", "physics",  10000, 0.0);
}

bool ReSetSchedulingSpecs(double fSimuRate, double fOutputRate)
{
	return situationUpdater->setSchedulingSpecs(fSimuRate, fOutputRate);
}

void ReStart(void)
{
	GfSchedBeginSession("raceupdate");
	situationUpdater->start();
}

#ifdef SD_DEBUG
void ReOneStep(double dt)
{
	situationUpdater->runOneStep(dt);
}
#endif

int ReUpdate(void)
{
	return (*mainUpdater)();
}

void ReStop(void)
{
	situationUpdater->stop();
	GfSchedEndSession("raceupdate");
}

void ReShutdownUpdaters()
{
	// Destroy the situation updater.
	delete situationUpdater;
	situationUpdater = 0;
	
	// Destroy the main updater.
	delete mainUpdater;
	mainUpdater = 0;

	// TODO: Rework, as Currently broken, after the race engine / user interface separation.
	// Close schedule spy session and print the report.
	GfSchedEndSession("raceupdate");
	//                                          (timeResol, durUnit, durResol)
	GfSchedPrintReport("raceupdate", "schedule.csv", 1.0e-3, 1.0e-3, 1.0e-6);
}

const tRmInfo* ReOutputSituation()
{
	return mainUpdater->data();
}
