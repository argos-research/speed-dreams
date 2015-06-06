/**************************************************************************

    file        : racemain.cpp
    created     : Sat Nov 16 12:13:31 CET 2002
    copyright   : (C) 2002 by Eric Espie
    email       : eric.espie@torcs.org   
    version     : $Id: racemain.cpp 5377 2013-04-01 12:27:42Z wdbee $
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

/** @file    racemain
    		
    @author	<a href=mailto:eric.espie@torcs.org>Eric Espie</a>
    @version	$Id: racemain.cpp 5377 2013-04-01 12:27:42Z wdbee $
*/

#include <sstream>

#include <portability.h>

#include <robot.h>
#include <time.h>
#include <car.h>

#include "genparoptv1.h"

#include "raceutil.h" // RmGetFeaturesList
#include "racesituation.h"
#include "raceinit.h"
#include "raceupdate.h"
#include "raceresults.h"
#include "racestate.h"
#include "racetrack.h" // ReTrackInit, ReTrackUpdate

#include "teammanager.h"
#include "genetic.h"

#include "racemain.h"

// DEBUG
/*
#define DIV 1024
#define WIDTH 7
DWORDLONG lastFreeMem = 0;
*/

// State flag for run once initialisation
bool genOptNeedInit = true;
int OptiCounter = 0;

// Buffer for short time use for filenames
char buf[FILENAME_MAX+1]; // BUFSIZE defined in genetic.h to be = MAX_PATH

typedef struct
{
	char *racename;
	int startpos;
	int endpos;
	int diffpos;
}tReGridPart;

int *ReStartingOrderIdx = NULL; //array to hold indexes of cars (in params/RM_SECT_DRIVERS) in starting order

//Utility

/** 
 * ReHumanInGroup
 * Checks if there is a human-driven car among the racing cars.
 * 
 * @return True if there is a human.
 */
bool
ReHumanInGroup()
{
	if (GfParmListSeekFirst(ReInfo->params, RM_SECT_DRIVERS) == 0) {
		do {
			if (strcmp (GfParmGetCurStr(ReInfo->params, RM_SECT_DRIVERS, RM_ATTR_MODULE, ""), "human") == 0)
				return true;
		} while (GfParmListSeekNext(ReInfo->params, RM_SECT_DRIVERS) == 0);
	}
	return false;
}//ReHumanInGroup


/***************************************************************/

int ReConfigure()
{
	ReUI().onRaceConfiguring();

	return RM_ASYNC | RM_NEXT_STEP;
}

void ReRaceAbandon()
{
	ReCleanupGeneticOptimisation();
	// Notify the UI that the race event is finishing now.
	ReUI().onRaceEventFinishing();

	// Shutdown track-physics-related stuff.
	ReTrackShutdown();

	// Cleanup needed stuff.
	FREEZ(ReInfo->_reCarInfo);

	if (ReInfo->params != ReInfo->mainParams)
	{
		GfParmReleaseHandle(ReInfo->params);
		ReInfo->params = ReInfo->mainParams;
	}

	// Return to race configuration step
	ReStateApply((void*)RE_STATE_CONFIG);
}

void ReRaceAbort()
{
	if (ReCleanupGeneticOptimisation())
		return;

	ReShutdownUpdaters();

	RePhysicsEngine().shutdown();
	GenParOptV1::self().unloadPhysicsEngine();

	ReUI().onRaceFinishing();

	ReRaceCleanDrivers();

	FREEZ(ReInfo->_reCarInfo);
	
	if (ReInfo->params != ReInfo->mainParams)
	{
		GfParmReleaseHandle(ReInfo->params);
		ReInfo->params = ReInfo->mainParams;
	}

	// Return to race configuration step
	ReStateApply((void*)RE_STATE_CONFIG);
}

void ReRaceSkipSession()
{
	ReStateApply((void*)RE_STATE_RACE_END);
}

int
ReRaceEventInit(void)
{
	// Initialize the race session name.
	ReInfo->_reRaceName = ReGetCurrentRaceName();
	GfLogInfo("Starting new event (%s session)\n", ReInfo->_reRaceName);

	ReUI().onRaceEventInitializing();
	
	ReInfo->s->_features = RmGetFeaturesList(ReInfo->params);

	ReTrackInit();
	
	ReEventInitResults();

	return RM_SYNC | RM_NEXT_STEP;
}

/* parse advanced starting order strings */
/* grid part format: "sessionname[startpos:endpos]" */
/* returns: 0, when failed to parse; 1, when successfully parsed */
int
ReParseStartingOrder(const char *StartingOrder, tReGridPart **pGridList, int nCars, int &nGridList)
{
	char path[128];
	char *tempstr;
	int curRaceIdx;
	int i, nGL;
	void  *params = ReInfo->params;
	void  *results = ReInfo->results;
	tReGridPart *GridList;
	
	//input sanity check
	if ((StartingOrder == NULL) || (nCars<1)){nGridList = 0; return 0;}
	//find the number of parts, that is the number of '[' characters
	nGL = 0;
	i = 0;
	while (StartingOrder[i] != '\0') {
		if (StartingOrder[i] == '['){nGL++;}
		i++;
	}
	
	curRaceIdx = (int)GfParmGetNum(results, RE_SECT_CURRENT, RE_ATTR_CUR_RACE, NULL, 1);
	// check whether it is a name of an earlier session
	if (nGL == 0) {
		for ( i = 1; i < curRaceIdx; i++ ) {
			snprintf(path, sizeof(path), "%s/%d", RM_SECT_RACES, i);
			tempstr = GfParmGetStrNC(params, path, RM_ATTR_NAME, 0);
			if (strcmp(tempstr, StartingOrder) == 0 ) {
				GridList = new tReGridPart[1];
				if (GridList == NULL){return 0;}
				GridList[0].racename = tempstr;
				GridList[0].startpos = 1;
				GridList[0].endpos = nCars;
				GridList[0].diffpos = 1;
				nGridList = 1;
				*pGridList = GridList;
				return 1;
			}
		}
		//badly formatted GridList
		nGridList = 0;
		*pGridList = NULL;
		return 0;
	}
	
	// now try to parse it
	char *tempstr2 = new char[strlen(StartingOrder)];
	int stri;
	int GLi = 0;
	GridList = new tReGridPart[nGL];

	for (i = 0; i < nGL; i++) {
		//search for session name
		stri = 0;
		while (StartingOrder[GLi] != '[') {
			tempstr2[stri] = StartingOrder[GLi];
			stri++;
			GLi++;
		}
		tempstr2[stri] = '\0';
		GLi++;
		GridList[i].racename = NULL;
		for ( int j = 1; j < curRaceIdx; j++ ) {
			snprintf(path, sizeof(path), "%s/%d", RM_SECT_RACES, j);
			tempstr = GfParmGetStrNC(params, path, RM_ATTR_NAME, 0);
			if (strcmp(tempstr, tempstr2) == 0 ) {
				GridList[i].racename = tempstr;
				break;
			}
		}
		if (GridList[i].racename == NULL) {
			// failed to find session
			nGridList = 0;
			delete[] GridList;
			delete[] tempstr2;
			*pGridList = NULL;
			return 0;
		}
		//find indexes
		stri = 0;
		while (StartingOrder[GLi] != ']') {
			tempstr2[stri] = StartingOrder[GLi];
			stri++;
			GLi++;
		}
		tempstr2[stri] = '\0';
		GLi++;
		GridList[i].startpos = GridList[i].endpos = -1;
		sscanf(tempstr2, "%d:%d", &(GridList[i].startpos), &(GridList[i].endpos));
		if (GridList[i].startpos <= 0) {
			nGridList = 0;
			delete[] GridList;
			delete[] tempstr2;
			*pGridList = NULL;
			return 0;
		} else if (GridList[i].endpos <= 0) {
			GridList[i].endpos = GridList[i].startpos;
		}
		if (GridList[i].endpos < GridList[i].startpos)
			{GridList[i].diffpos = -1;}
		else {GridList[i].diffpos = 1;}
	}
	
	delete[] tempstr2;
	nGridList = nGL;
	*pGridList = GridList;
	return 1;
}

// Find driver position in params/RM_SECT_DRIVERS based on module name and ID
// returns the driver position or -1 when not found or error
int
ReFindDriverIdx (const char *modulename, int idx)
{
	char path[128];
	void *params = ReInfo->params;
	
	for (int i = 1; i <= GfParmGetEltNb(params, RM_SECT_DRIVERS); i++) {
		snprintf(path, sizeof(path), "%s/%d", RM_SECT_DRIVERS, i);
		if (( (int)GfParmGetNum(params, path, RE_ATTR_IDX, NULL, 0) == idx ) && 
		     (strcmp(modulename,
			         GfParmGetStr(params, path, RE_ATTR_MODULE, "")) == 0) ) {
			//car found
			return i;
		}
	}
	//car not found
	return -1;
}


int
RePreRace(void)
{
	char path[128];
	const char *raceName;
	const char *raceType;
	void *params = ReInfo->params;
	void *results = ReInfo->results;
	int curRaceIdx;
	int timedLapsReplacement = 0;
	char *prevRaceName;
	
	raceName = ReInfo->_reRaceName = ReGetCurrentRaceName();
	
	GfParmRemoveVariable (params, "/", "humanInGroup");
	GfParmRemoveVariable (params, "/", "eventNb");
	GfParmSetVariable (params, "/", "humanInGroup", ReHumanInGroup() ? 1.0f : 0.0f);
	GfParmSetVariable (params, "/", "eventNb", GfParmGetNum (ReInfo->results, RE_SECT_CURRENT, RE_ATTR_CUR_TRACK, NULL, 1.0 ) );
	if (!raceName) {
		return RM_ERROR;
	}

	if (strcmp(GfParmGetStr(params, raceName, RM_ATTR_ENABLED, RM_VAL_YES), RM_VAL_NO) == 0) {
		GfLogTrace( "Race %s disabled\n",  raceName);
		curRaceIdx = (int)GfParmGetNum(results, RE_SECT_CURRENT, RE_ATTR_CUR_RACE, NULL, 1);
		if (curRaceIdx < GfParmGetEltNb(params, RM_SECT_RACES)) {
			curRaceIdx++;
			GfLogTrace( "Race %s is not the last one, but the #%d\n",  raceName, curRaceIdx);
			GfParmSetNum(results, RE_SECT_CURRENT, RE_ATTR_CUR_RACE, NULL, (tdble)curRaceIdx);
	
			return RM_SYNC | RM_NEXT_RACE;
		}
	
		GfParmSetNum(results, RE_SECT_CURRENT, RE_ATTR_CUR_RACE, NULL, 1);
		return RM_SYNC | RM_NEXT_RACE | RM_NEXT_STEP;
	}

	// Get session max dammages.
	ReInfo->s->_maxDammage = (int)GfParmGetNum(params, raceName, RM_ATTR_MAX_DMG, NULL, 10000);

	// Get session type (race, qualification or practice).
	raceType = GfParmGetStr(params, raceName, RM_ATTR_TYPE, RM_VAL_RACE);
	if (!strcmp(raceType, RM_VAL_RACE)) {
		ReInfo->s->_raceType = RM_TYPE_RACE;
	} else if (!strcmp(raceType, RM_VAL_QUALIF)) {
		ReInfo->s->_raceType = RM_TYPE_QUALIF;
	} else if (!strcmp(raceType, RM_VAL_PRACTICE)) {
		ReInfo->s->_raceType = RM_TYPE_PRACTICE;
	}

	// Get session duration (defaults to "All sessions" one, or else -60).
	ReInfo->s->_totTime = GfParmGetNum(params, raceName, RM_ATTR_SESSIONTIME, NULL, -1);
	if (ReInfo->s->_totTime < 0)
		ReInfo->s->_totTime = GfParmGetNum(params, RM_VAL_ANYRACE, RM_ATTR_SESSIONTIME, NULL, -60.0f);

	// Determine the actual session duration and/or number of laps.
	ReInfo->s->_extraLaps = 0; // TODO: Does this is ever needed ?
	ReInfo->s->_totLaps = 0; // Make sure it is initialized

	if (ReInfo->s->_totTime > 0 && !(ReInfo->s->_features & RM_FEATURE_TIMEDSESSION)) {
		// Timed session not supported: add 1 km for every minute in parctise or qualifying,
		// and 150 km for every hour (2.5 km for every minute) in race
		if (ReInfo->s->_raceType == RM_TYPE_RACE) {
			ReInfo->s->_totLaps = (int)floor(ReInfo->s->_totTime * 2500.0f / 60.0f / ReInfo->track->length + 0.5f);
		} else {
			ReInfo->s->_totLaps = (int)floor(ReInfo->s->_totTime * 1000.0f / 60.0f / ReInfo->track->length + 0.5f);
		}
		timedLapsReplacement = ReInfo->s->_totLaps;
		ReInfo->s->_totTime = -60.0f;
	}
	
	// Timed session doesn't exclude additional laps after the time finishes
	// Make sure that if no time set, we set far below zero
	if(ReInfo->s->_totTime <= 0.0f )
		ReInfo->s->_totTime = -60.0f;
		
	// Get session distance (defaults to "All sessions" one, or else 0).
	tdble dist = GfParmGetNum(params, raceName, RM_ATTR_DISTANCE, NULL, -1);
	if (dist < 0)
		dist = GfParmGetNum(params, RM_VAL_ANYRACE, RM_ATTR_DISTANCE, NULL, 0);
	
	// If a (> 0) session distance was specified, deduce the number of laps
	// in case the race settings don't specify it, and it is not a timed race.
	if ( (dist >= 0.001) && (ReInfo->s->_totTime < 0.0f) ) { // Why not 'if (dist > 0)' ???
		ReInfo->s->_totLaps = (int)(dist / ReInfo->track->length) + 1;
		ReInfo->s->_extraLaps = ReInfo->s->_totLaps; // Extralaps are used to find out how many laps there are after the time is up in timed sessions
	} else {dist = -1;}
	
	// Get the number of laps (defaults to "All sessions" one,
	// or else the already computed one from the session distance, or 0).
	int laps = (int)GfParmGetNum(params, raceName, RM_ATTR_LAPS, NULL, -1);
	if (laps < 0)
		laps = (int)GfParmGetNum(params, RM_VAL_ANYRACE, RM_ATTR_LAPS, NULL, 0);

	// Use lap number only when race distance is not in use.
	if ( (laps > 0) && (dist <= 0.0) && (timedLapsReplacement <= 0) ) {
		ReInfo->s->_totLaps = laps;
		ReInfo->s->_extraLaps = ReInfo->s->_totLaps; //Extralaps are used to find out how many laps there are after the time is up in timed sessions
	}

	// Make sure we have at least 1 lap race length.
	if ( (laps <= 0) && (dist <=0) && (ReInfo->s->_totTime < 0) ) {
		ReInfo->s->_totLaps = 1;
		ReInfo->s->_extraLaps = ReInfo->s->_totLaps; //Extralaps are used to find out how many laps there are after the time is up in timed sessions
	}

	// Correct extra laps (possible laps run after the winner arrived ?) :
	// during timed practice or qualification, there are none.
	if (ReInfo->s->_raceType != RM_TYPE_RACE && ReInfo->s->_totTime > 0) {
		ReInfo->s->_extraLaps = 0; //Extralaps are used to find out how many laps there are after the time is up in timed sessions
		ReInfo->s->_totLaps = 0;
	}

	GfLogInfo("Race length : time=%.0fs, laps=%d (extra=%d)\n",
			  ReInfo->s->_totTime, ReInfo->s->_totLaps, ReInfo->s->_extraLaps);
	
	// Initialize race state.
	ReInfo->s->_raceState = 0;

	// Cleanup results
	snprintf(path, sizeof(path), "%s/%s/%s", ReInfo->track->name, RE_SECT_RESULTS, raceName);
	GfParmListClean(results, path);

	// Drivers starting order
	// The starting order is decided here,
	// then car indexes are stored in ReStartingOrderIdx, in the starting order.
	// The actual grid is assembled in ReRaceStart().
	// In case of a race, when all cars start at the same time,
	// cars are simply added to the starting list in the order stored in ReStartingOrderIdx.
	// If only one car is at the track at a time (not timed session qualifying or practice),
	// the race is divided into many sub-races.
	// For a sub-race, only the results/RE_ATTR_CUR_DRIVER-th driver in ReStartingOrderIdx
	// is added to the starting grid.
	// RE_ATTR_CUR_DRIVER is refreshed after every sub-race in ReRaceEnd().
	int nCars = GfParmGetEltNb(params, RM_SECT_DRIVERS);
	GfParmListClean(params, RM_SECT_DRIVERS_RACING);
	if (nCars == 0)
	{
		// This may happen, when playing with the text-only mode,
		// and forgetting that human are automatically excluded then,
		// or when getting back to the GUI mode, and not reconfiguring the competitors list.
		GfLogError("No competitor in this race : cancelled.\n");
		return RM_ERROR;
	}
	else
	{
		ReUI().addLoadingMessage("Determining Starting Order ...");

		const char* gridType =
			GfParmGetStr(params, raceName, RM_ATTR_START_ORDER, RM_VAL_DRV_LIST_ORDER);
		
		int maxCars = (int)GfParmGetNum(params, raceName, RM_ATTR_MAX_DRV, NULL, 100);
		nCars = MIN(nCars, maxCars);
		
		tReGridPart *GridList = NULL;
		int nGridList = 0;
		
		// Initialize the array of car indexes for starting order
		if (ReStartingOrderIdx != NULL) {
			delete[] ReStartingOrderIdx;
			ReStartingOrderIdx = NULL;
		}
		ReStartingOrderIdx = new int[nCars];
		for (int i = 0; i < nCars; i++) {
			ReStartingOrderIdx[i] = -1;
		}
		
		// Starting grid in the arrival order of the previous race (or qualification session)
		if (!strcmp(gridType, RM_VAL_LAST_RACE_ORDER))
		{
			GfLogTrace("Starting grid in the order of the last race\n");
			
			prevRaceName = ReGetPrevRaceName(/* bLoop = */false);
			if (!prevRaceName) {
				return RM_ERROR;
			}

			for (int i = 1; i < nCars + 1; i++) {
				snprintf(path, sizeof(path), "%s/%s/%s/%s/%d",
						 ReInfo->track->name, RE_SECT_RESULTS, prevRaceName, RE_SECT_RANK, i);
				ReStartingOrderIdx[i-1] = 
					ReFindDriverIdx (GfParmGetStr(results, path, RE_ATTR_MODULE, ""),
									(int)GfParmGetNum(results, path, RE_ATTR_IDX, NULL, 0));
			}
		}
		
		// Starting grid in the reversed arrival order of the previous race
		else if (!strcmp(gridType, RM_VAL_LAST_RACE_RORDER))
		{
			GfLogTrace("Starting grid in the reverse order of the last race\n");

			prevRaceName = ReGetPrevRaceName(/* bLoop = */false);
			if (!prevRaceName) {
				return RM_ERROR;
			}

			for (int i = 1; i < nCars + 1; i++) {
				snprintf(path, sizeof(path), "%s/%s/%s/%s/%d",
						ReInfo->track->name, RE_SECT_RESULTS, prevRaceName, RE_SECT_RANK, nCars - i + 1);
				ReStartingOrderIdx[i-1] = 
					ReFindDriverIdx (GfParmGetStr(results, path, RE_ATTR_MODULE, ""),
									(int)GfParmGetNum(results, path, RE_ATTR_IDX, NULL, 0));
			}
		}

		// Starting grid as a mix from the results of earlier sessions
		else if (ReParseStartingOrder(gridType, &GridList, nCars, nGridList))
		{
			GfLogTrace("Starting grid as a mix from the results of earlier sessions\n");
			
			int idx;
			int gridpos = 1;
			int carnr;
			const char *modulename;
			for (int i = 0; i < nGridList; i++) {
				if (gridpos > nCars) {break;}
				if (GridList[i].diffpos == -1) {//reversed
					for ( int j = GridList[i].startpos; j >= GridList[i].endpos; j--) {
						if (gridpos > nCars) {break;}
						snprintf(path, sizeof(path), "%s/%s/%s/%s/%d",
								ReInfo->track->name, RE_SECT_RESULTS, GridList[i].racename, RE_SECT_RANK, j);
						idx = (int)GfParmGetNum(results, path, RE_ATTR_IDX, NULL, 0);
						modulename = GfParmGetStr(results, path, RE_ATTR_MODULE, "");
						carnr = ReFindDriverIdx(modulename, idx);
						for (int k = 0; k < gridpos-1; k++) {
							if ( carnr == ReStartingOrderIdx[k] ) {
								//oops: same car twice
								GfLogWarning("The same car appears twice in the advanced grid!\n");
								carnr = -1;
								break;
							}
						}
						//adding car to the list
						if (carnr != -1) {
							ReStartingOrderIdx[gridpos-1] = carnr;
							gridpos++;
						}
					}
				} else if (GridList[i].diffpos == 1){//straight order
					for ( int j = GridList[i].startpos; j <= GridList[i].endpos; j++) {
						if (gridpos > nCars) {break;}
						snprintf(path, sizeof(path), "%s/%s/%s/%s/%d",
								ReInfo->track->name, RE_SECT_RESULTS, GridList[i].racename, RE_SECT_RANK, j);
						idx = (int)GfParmGetNum(results, path, RE_ATTR_IDX, NULL, 0);
						modulename = GfParmGetStr(results, path, RE_ATTR_MODULE, "");
						carnr = ReFindDriverIdx(modulename, idx);
						for (int k = 0; k < gridpos-1; k++) {
							if ( carnr == ReStartingOrderIdx[k] ) {
								//oops: same car twice
								GfLogWarning("The same car appears twice in the advanced grid!\n");
								carnr = -1;
								break;
							}
						}
						//adding car to the list
						if (carnr != -1) {
							ReStartingOrderIdx[gridpos-1] = carnr;
							gridpos++;
						}
					}
				}
			}
			//cleaning up memory
			if (nGridList > 0){delete[] GridList;}
		}

		// Starting grid in the drivers list order
		else
		{
			GfLogTrace("Starting grid in the order of the driver list\n");

			for (int i = 1; i < nCars + 1; i++) {
				snprintf(path, sizeof(path), "%s/%d", RM_SECT_DRIVERS, i);
				ReStartingOrderIdx[i-1] = 
					ReFindDriverIdx (GfParmGetStr(params, path, RE_ATTR_MODULE, ""),
									(int)GfParmGetNum(params, path, RE_ATTR_IDX, NULL, 0));
			}
		}
	}
	
	GfParmSetNum(results, RE_SECT_CURRENT, RE_ATTR_CUR_DRIVER, NULL, 1.0);

	return RM_SYNC | RM_NEXT_STEP;
}

/* return state mode */
int
ReRaceRealStart(void)
{
	int i, j;
	tRobotItf *robot;
	tReCarInfo *carInfo;
	char buf[128];
	int foundHuman;
	void *params = ReInfo->params;
	tSituation *s = ReInfo->s;
	tMemoryPool oldPool = NULL;
	void* carHdle;

	// Load the physics engine
	if (!GenParOptV1::self().loadPhysicsEngine())
		return RM_ERROR;

	// Get the session display mode (default to "All sessions" ones, or else "normal").
	std::string strDispMode =
		GfParmGetStr(params, ReInfo->_reRaceName, RM_ATTR_DISPMODE, "");
	if (strDispMode.empty())
		strDispMode =
			GfParmGetStr(params, RM_VAL_ANYRACE, RM_ATTR_DISPMODE, RM_VAL_VISIBLE);

	if (strDispMode == RM_VAL_INVISIBLE)
		ReInfo->_displayMode = RM_DISP_MODE_NONE;
	else if (strDispMode == RM_VAL_VISIBLE)
		ReInfo->_displayMode = RM_DISP_MODE_NORMAL;
	else if (strDispMode == RM_VAL_SIMUSIMU)
		ReInfo->_displayMode = RM_DISP_MODE_SIMU_SIMU;
	else
	{
		GfLogError("Unsupported display mode '%s' loaded from race file ; "
				   "assuming 'normal'\n", strDispMode.c_str());
		ReInfo->_displayMode = RM_DISP_MODE_NORMAL;
	}

	//GfLogDebug("ReRaceRealStart: Loaded dispMode=0x%x\n", ReInfo->_displayMode);
	
	// Check if there is a human in the driver list
	foundHuman = ReHumanInGroup() ? 2 : 0;

	// Reset SimuSimu bit if any human in the race.
	// Note: Done here in order to make SimuSimu faster. 
	if ((ReInfo->_displayMode & RM_DISP_MODE_SIMU_SIMU) && foundHuman)
	{
		ReInfo->_displayMode &= ~RM_DISP_MODE_SIMU_SIMU;
	}
	
	// Initialize & place cars
	// Note: if SimuSimu display mode, robot->rbNewTrack isn't called. This is a lot faster.
	if (ReInitCars())
		return RM_ERROR;

	// Check if there is a human in the current race
	// Warning: Don't move this before ReInitCars (initializes s->cars).
	for (i = 0; i < s->_ncars; i++) {
		if (s->cars[i]->_driverType == RM_DRV_HUMAN) {
			foundHuman = 1;
			break;
		}//if human
	}//for i

	// Force "normal" display mode if any human in the session
	if (foundHuman == 1)
	{
		ReInfo->_displayMode = RM_DISP_MODE_NORMAL;
	}
	// Force "result only" mode in Practice / Qualif. sessions without any human,
	// but at least 1 in another session (why this ?), and SimuSimu bit on.
	else if (foundHuman == 2 && (ReInfo->_displayMode & RM_DISP_MODE_SIMU_SIMU)
		     && (ReInfo->s->_raceType == RM_TYPE_QUALIF
				 || ReInfo->s->_raceType == RM_TYPE_PRACTICE))
	{
		ReInfo->_displayMode = RM_DISP_MODE_NONE;
	}

	GfLogInfo("Display mode : %s\n",
			  (ReInfo->_displayMode & RM_DISP_MODE_SIMU_SIMU) ? "SimuSimu" :
			  ((ReInfo->_displayMode & RM_DISP_MODE_NORMAL) ? "Normal" : "Results-only"));

	// Notify the UI that it's "race loading time".
	ReUI().onRaceLoadingDrivers();

	// Load drivers for the race
	for (i = 0; i < s->_ncars; i++)
	{
		snprintf(buf, sizeof(buf), "cars/models/%s/%s.xml",
				 s->cars[i]->_carName, s->cars[i]->_carName);
		carHdle = GfParmReadFile(buf, GFPARM_RMODE_STD);
		snprintf(buf, sizeof(buf), "Loading %s driver (%s) ...",
				 s->cars[i]->_name, GfParmGetName(carHdle));
		
		ReUI().addLoadingMessage(buf);

		if (!(ReInfo->_displayMode & RM_DISP_MODE_SIMU_SIMU))
		{
			//Tell robots they are to start a new race
			robot = s->cars[i]->robot;
			GfPoolMove( &s->cars[i]->_newRaceMemPool, &oldPool );
			robot->rbNewRace(robot->index, s->cars[i], s);
			GfPoolFreePool( &oldPool );
		}//if ! simusimu
	}//for i
	
	RtTeamManagerStart();

	// Notify the UI that the drivers have been loaded now.
	ReUI().onRaceDriversLoaded();

	// Initialize the physics engine
	RePhysicsEngine().updateSituation(s, RCM_MAX_DT_SIMU);

	carInfo = ReInfo->_reCarInfo;
	for (i = 0; i < s->_ncars; i++) {
		carInfo[i].prevTrkPos = s->cars[i]->_trkPos;
	}

	// All cars start with max brakes on
	ReUI().addLoadingMessage("Running Prestart ...");
	
	for (i = 0; i < s->_ncars; i++)
	{
		memset(&(s->cars[i]->ctrl), 0, sizeof(tCarCtrl));
		s->cars[i]->ctrl.brakeCmd = 1.0;
	}

	for (j = 0; j < (int)(1.0 / RCM_MAX_DT_SIMU); j++)
		RePhysicsEngine().updateSituation(s, RCM_MAX_DT_SIMU);

	// Initialize current result manager.
	ReInitCurRes();

	// More initializations.
	ReInfo->_reTimeMult = 1.0;
	ReInfo->_reLastRobTime = -1.0;
	ReInfo->s->currentTime = -2.0;	// We start 2 seconds before the real race start
	ReInfo->s->deltaTime = RCM_MAX_DT_SIMU;
	ReInfo->s->_raceState = RM_RACE_STARTING;

	ReInfo->_rePitRequester = 0;
	ReInfo->_reMessage = 0;
	ReInfo->_reMessageEnd = 0.0;
	ReInfo->_reBigMessage = 0;
	ReInfo->_reBigMessageEnd = 0.0;
	
	ReInitUpdaters();

	// Notify the UI that the race simulation is ready now.
	ReUI().onRaceSimulationReady();

	// Notify the UI that the race is now started.
	ReUI().addLoadingMessage("Ready.");

	ReUI().onRaceStarted();

	// And go on looping the race state automaton.
	return RM_SYNC | RM_NEXT_STEP;
}//ReRaceRealStart

/* return state mode */
int
ReRaceStart(void)
{
	char path[128];
	char path2[128];
	const char *sessionName = ReInfo->_reRaceName;
	void *params = ReInfo->params;
	void *results = ReInfo->results;
	int mode = 0;

	// Trace race session identification (more to say for the Carer mode).
	char pszSessionId[128];
	if (!strcmp(GfParmGetStr(ReInfo->mainParams, RM_SECT_SUBFILES, RM_ATTR_HASSUBFILES, RM_VAL_NO), RM_VAL_YES))
	{
		const char* pszGroup = GfParmGetStr(params, RM_SECT_HEADER, RM_ATTR_NAME, "<no group>");
		snprintf(pszSessionId, sizeof(pszSessionId), "%s %s %s", ReInfo->_reName, pszGroup, sessionName);
	}
	else
		snprintf(pszSessionId, sizeof(pszSessionId), "%s %s", ReInfo->_reName, sessionName);
	
	GfLogInfo("Starting %s session at %s\n", pszSessionId, ReInfo->track->name);

	// Reallocate and reset car info for the race.
	FREEZ(ReInfo->_reCarInfo);
	ReInfo->_reCarInfo =
		(tReCarInfo*)calloc(GfParmGetEltNb(params, RM_SECT_DRIVERS), sizeof(tReCarInfo));

	ReUI().onRaceInitializing();
	
	// Drivers starting order
	int nCars = GfParmGetEltNb(params, RM_SECT_DRIVERS);
	GfParmListClean(params, RM_SECT_DRIVERS_RACING);
	if (nCars == 0)
	{
		// This may happen, when playing with the text-only mode,
		// and forgetting that human are automatically excluded then,
		// or when getting back to the GUI mode, and not reconfiguring the competitors list.
		GfLogError("No competitor in this race : cancelled.\n");
		mode = RM_ERROR;
	}
	else if ((ReInfo->s->_raceType == RM_TYPE_QUALIF || ReInfo->s->_raceType == RM_TYPE_PRACTICE)
		&& ReInfo->s->_totTime < 0.0f /* Timed session? */)
	//Checks if there is only one driver per session allowed, so practice, qualification without timed session. 
	{
		// non-timed Qualification or Practice session => 1 driver at a time = the "current" one.
		int nCurrDrvInd =
			(int)GfParmGetNum(results, RE_SECT_CURRENT, RE_ATTR_CUR_DRIVER, NULL, 1);
		if (nCurrDrvInd <= 0)
			return RM_ERROR;

		// Propagate competitor drivers info to the real race starting grid
		snprintf(path, sizeof(path), "%s/%d", RM_SECT_DRIVERS, ReStartingOrderIdx[nCurrDrvInd-1]);
		snprintf(path2, sizeof(path2), "%s/%d", RM_SECT_DRIVERS_RACING, 1);

		GfParmSetStr(params, path2, RM_ATTR_MODULE,
					 GfParmGetStr(params, path, RM_ATTR_MODULE, ""));
		GfParmSetNum(params, path2, RM_ATTR_IDX, NULL,
					 GfParmGetNum(params, path, RM_ATTR_IDX, NULL, 0));
		GfParmSetNum(params, path2, RM_ATTR_EXTENDED, NULL,
					 GfParmGetNum(params, path, RM_ATTR_EXTENDED, NULL, 0));
		GfParmSetNum(params, path2, RM_ATTR_SKINTARGETS, NULL,
					 GfParmGetNum(params, path, RM_ATTR_SKINTARGETS, NULL, 0));
		if (GfParmGetStr(params, path, RM_ATTR_SKINNAME, 0))
			GfParmSetStr(params, path2, RM_ATTR_SKINNAME,
						 GfParmGetStr(params, path, RM_ATTR_SKINNAME, ""));
	}
	else
	{
		// For a race, add cars to the starting grid in the order stored in ReStartingOrderIdx.
		ReUI().addLoadingMessage("Preparing Starting Grid ...");
		
		int maxCars = (int)GfParmGetNum(params, sessionName, RM_ATTR_MAX_DRV, NULL, 100);
		nCars = MIN(nCars, maxCars);
		int currDriver = -1;
		int aCars = 0;
		
		for (int i = 1; i < nCars + 1; i++)
		{
			currDriver = ReStartingOrderIdx[i-1];
			if (currDriver == -1)
				continue;
			aCars++;
			snprintf(path, sizeof(path), "%s/%d", RM_SECT_DRIVERS, currDriver);
			snprintf(path2, sizeof(path2), "%s/%d", RM_SECT_DRIVERS_RACING, i);
			GfParmSetStr(params, path2, RM_ATTR_MODULE,
						 GfParmGetStr(params, path, RE_ATTR_MODULE, ""));
			GfParmSetNum(params, path2, RM_ATTR_IDX, NULL,
						 GfParmGetNum(params, path, RE_ATTR_IDX, NULL, 0));
			GfParmSetNum(params, path2, RM_ATTR_EXTENDED, NULL,
						 GfParmGetNum(params, path, RM_ATTR_EXTENDED, NULL, 0));
			GfParmSetNum(params, path2, RM_ATTR_SKINTARGETS, NULL,
						 GfParmGetNum(params, path, RM_ATTR_SKINTARGETS, NULL, 0));
			if (GfParmGetStr(params, path, RM_ATTR_SKINNAME, 0))
				GfParmSetStr(params, path2, RM_ATTR_SKINNAME,
							 GfParmGetStr(params, path, RM_ATTR_SKINNAME, ""));
		}
		
		//no valid drivers present in the list
		if (aCars == 0)
		{
			GfLogError("No competitor in this race : cancelled.\n");
			mode = RM_ERROR;
		}
	}
	
	//ReTrackUpdate();

	if (!(mode & RM_ERROR))
	{
		// According to what the UI answers, start the race right now or not.
		mode = RM_ASYNC | RM_NEXT_STEP;
		const bool bGoOn = ReUI().onRaceStarting();
		if (bGoOn)
			mode = ReRaceRealStart();
	}
	
	return mode;
}

void ReRaceRestart()
{
	ReShutdownUpdaters();

	ReUI().onRaceFinishing();
	
	ReRaceCleanup();

	ReStateApply((void*)RE_STATE_PRE_RACE);
}

int
ReRaceStop(void)
{
	ReStop();

	ReUI().onRaceInterrupted();
	
	return RM_ASYNC | RM_NEXT_STEP;
}

int
ReRaceEnd(void)
{
	int curDrvIdx;
	int nCars;
	void *params = ReInfo->params;
	void *results = ReInfo->results;
	const char *sessionName = ReInfo->_reRaceName;
	tgenData *Data = &TGeneticParameter::Data;

	// Pick up optimization results ...
	Data->car = ReInfo->s->cars[0];

	Data->DamagesTotal = Data->car->_dammage;
	if (Data->car->_bestLapTime > 0)
		Data->BestLapTime = Data->car->_bestLapTime;
	else
		Data->BestLapTime = 99*60;

	Data->WeightedBestLapTime = Data->BestLapTime + Data->WeightOfDamages * Data->DamagesTotal * 0.007f;
	// ... pick up optimization results

	ReShutdownUpdaters();

	ReUI().onRaceFinishing();
	
	ReRaceCleanup();

	// If we are at the end of a qualification or practice session for a competitor,
	// select the next competitor : it is his turn for the same session.
	// If no more competitor, this is the end of the session for all the competitors.
	bool bEndOfSession = true;
	if ((ReInfo->s->_raceType == RM_TYPE_QUALIF || ReInfo->s->_raceType == RM_TYPE_PRACTICE)
		&& ReInfo->s->_totTime < 0.0f)
	{
		// Get the index of the current competitor (the one who just completed his race).
		curDrvIdx = (int)GfParmGetNum(results, RE_SECT_CURRENT, RE_ATTR_CUR_DRIVER, NULL, 1);

		// Up to the next competitor now, if not the last one.
		curDrvIdx++;
		nCars = MIN(GfParmGetEltNb(params, RM_SECT_DRIVERS),
				(int)GfParmGetNum(params, sessionName, RM_ATTR_MAX_DRV, NULL, 100));
		if (curDrvIdx <= nCars) 
			bEndOfSession = false;
		else
			curDrvIdx = 1; // Was the last one : end of session !

		GfParmSetNum(results, RE_SECT_CURRENT, RE_ATTR_CUR_DRIVER, NULL, (tdble)curDrvIdx);
	}

	// Calculate class points if we just finished a session.
	if (bEndOfSession)
	{
		ReCalculateClassPoints (ReInfo->_reRaceName);
	}
	
	// Determine the new race state automation mode.
//	const bool bGoOn = ReUI().onRaceFinished(bEndOfSession);
	const bool bGoOn = true; // Always set RM_SYNC
	bEndOfSession = true;    // Always set RM_NEXT_STEP
	
	return (bEndOfSession ? RM_NEXT_STEP : RM_NEXT_RACE) | (bGoOn ? RM_SYNC : RM_ASYNC);
}


int
RePostRace(void)
{
	int curRaceIdx;
	void *results = ReInfo->results;
	void *params = ReInfo->params;

	// Prepare for next session if any left in the event.
	curRaceIdx = (int)GfParmGetNum(results, RE_SECT_CURRENT, RE_ATTR_CUR_RACE, NULL, 1);
	if (curRaceIdx < GfParmGetEltNb(params, RM_SECT_RACES)) {

		// Next session.
		curRaceIdx++;
		GfLogInfo("Next session will be #%d\n", curRaceIdx);
		GfParmSetNum(results, RE_SECT_CURRENT, RE_ATTR_CUR_RACE, NULL, (tdble)curRaceIdx);
		
		// Update standings in the results file.
		ReUpdateStandings();
		
		return RM_SYNC | RM_NEXT_RACE;
	}

	// No more session in the event : update standings in the results file.
	ReUpdateStandings();

	// Next event if any will start with its first session.
	GfParmSetNum(results, RE_SECT_CURRENT, RE_ATTR_CUR_RACE, NULL, 1);
	
	return RM_SYNC | RM_NEXT_STEP;
}


//
// Shutdown race engine
//
int
ReRaceEventShutdown(void)
{
	int curTrkIdx;
	void *params = ReInfo->params;
	int nbTrk;
	void *results = ReInfo->results;
	int curRaceIdx;

	// Notify the UI that the race event is finishing now.
	ReUI().onRaceEventFinishing();

	// Shutdown track-physics-related stuff.
	ReTrackShutdown();

	// Determine the track of the next event to come, if not the last one
	nbTrk = GfParmGetEltNb(params, RM_SECT_TRACKS);
	curRaceIdx =(int)GfParmGetNum(results, RE_SECT_CURRENT, RE_ATTR_CUR_RACE, NULL, 1);
	curTrkIdx = (int)GfParmGetNum(results, RE_SECT_CURRENT, RE_ATTR_CUR_TRACK, NULL, 1);

	if (curRaceIdx == 1) {
		if (curTrkIdx < nbTrk) {
			// Next track.
			curTrkIdx++;
		} else if (curTrkIdx >= nbTrk) {
			// Back to the beginning.
			curTrkIdx = 1;
		}
	}

	GfParmSetNum(results, RE_SECT_CURRENT, RE_ATTR_CUR_TRACK, NULL, (tdble)curTrkIdx);

	// Determine new race state automaton mode.
	int mode = (curTrkIdx != 1) ? RM_NEXT_RACE : RM_NEXT_STEP;

	//mode |= ReUI().onRaceEventFinished(nbTrk != 1, careerNonHumanGroup) ? RM_SYNC : RM_ASYNC;;

	mode |= RM_SYNC;
	
	if (mode & RM_NEXT_STEP)
		FREEZ(ReInfo->_reCarInfo);

	return mode;
}

//
// Build the well known weather code from rain and water at surface
//
int GetWeather(tTrack* Track)
{
	return (Track->local.rain << 4) + Track->local.water;
};

//
// Prepare a buffer with the file name
//
const char* SetupGlobalFileName(char* buf, int size, tgenData* Data, const char* Ext)
{
	if (Data->WeatherCode == 0)
	{
		snprintf(buf,size,"%sdrivers/%s/%s/%s%s",
			GetLocalDir(),Data->RobotName,Data->CarType,Data->TrackName,Ext);
	}
	else
	{
		snprintf(buf,size,"%sdrivers/%s/%s/%s-%d%s",
			GetLocalDir(),Data->RobotName,Data->CarType,Data->TrackName,Data->WeatherCode,Ext);
	}
	return buf;
}

//
// Initialisation of Optimisation
//
void
ReInitialiseGeneticOptimisation()
{
	if (!genOptNeedInit)
		return;

	genOptNeedInit = false;

	tgenData *Data = &TGeneticParameter::Data;

	Data->TrackName = &(Data->TrackNameBuffer[0]);
	Data->CarType = &(Data->CarTypeBuffer[0]);
	Data->RobotName = &(Data->RobotNameBuffer[0]);
	Data->AuthorName = &(Data->AuthorNameBuffer[0]);
	Data->PrivateSection = &(Data->PrivateSectionBuffer[0]);

	Data->BestLapTime = FLT_MAX;

	Data->WeightedBestLapTime = FLT_MAX;
	Data->LastWeightedBestLapTime = FLT_MAX;

	Data->DamagesTotal = 0;
	Data->LastDamagesTotal = 0;

	// Setup pointer to car data and track, car type and robot name
	Data->car = &ReInfo->carList[0];
	snprintf(Data->TrackNameBuffer, sizeof(Data->TrackNameBuffer),
		"%s", ReInfo->track->internalname);
	snprintf(Data->CarTypeBuffer, sizeof(Data->CarTypeBuffer),
		"%s", Data->car->_carName);
	snprintf(Data->RobotNameBuffer, sizeof(Data->RobotNameBuffer),
		"%s", Data->car->_modName);

	// Check weather conditions
	Data->WeatherCode = GetWeather(ReInfo->track);
	if (Data->WeatherCode > 0)		// In case of rain, ...
		Data->WeightOfDamages = 100;// ... use a heigh weight for penalties

	// Setup path to car setup file (xml and opt)
	Data->XmlFileName = SetupGlobalFileName(Data->BufferXML, FILENAME_MAX, Data, ".xml");
	Data->OptFileName = SetupGlobalFileName(Data->BufferOPT, FILENAME_MAX, Data, ".opt");

	// Assume, we can read initial values
	Data->GetInitialVal = true;

	// Try to open XML file ...
	Data->Handle = GfParmReadFile(Data->XmlFileName, GFPARM_RMODE_REREAD);
	Data->GetInitialVal = true;

	// ... in case the car setup file does not exist ...
	if (!Data->Handle)
	{
		// ... use default setup file ...
		snprintf(buf,FILENAME_MAX,"%sdrivers/%s/%s/default.xml",
			GetLocalDir(),Data->RobotName,Data->CarType);
		void* Handle = GfParmReadFile(buf, GFPARM_RMODE_REREAD);

		if (Handle) // ... if existing ...
		{			// ... use it to create a track specific setup file.
			GfParmWriteFileSDHeader (Data->XmlFileName, Handle, Data->CarType, Data->AuthorName);
		}
		else		// ... else create an empty setup file.
		{			
			void* Handle = GfParmReadFile(Data->XmlFileName, GFPARM_RMODE_REREAD | GFPARM_RMODE_CREAT);
			GfParmWriteFileSDHeader (Data->XmlFileName, Handle, Data->CarType, Data->AuthorName);
			Data->GetInitialVal = false; // No initial setup, we cannot read it
		}
		GfParmReleaseHandle(Handle);

		// Open created car type track setup file
		Data->Handle = GfParmReadFile(Data->XmlFileName, GFPARM_RMODE_REREAD);
	}

	ReImportGeneticParameters();

	OptiCounter = 0;
}

//
// Before we can start, we have to pick up the configuration
//
void
ReImportGeneticParameters()
{
	ReLogOptim.info("\n\nImport Genetic Parameters ...\n\n");

	// Setup pointer to structure
	tgenData *Data = &TGeneticParameter::Data;

	// Initialize flags
	Data->First = true;

	// Initialize values
	Data->TotalWeight = 0.0;
	int NextIdx = 0;    // Index to next parameter

	// For future use get race type
	// 0: Race; 1: Qualifying
    //Data->Type = (int) GfParmGetNum(Data->Handle, 
	//	"simplix private", "qualification", 0, 0);

	// Build path to meta data file
    snprintf(buf,sizeof(buf),"%sdrivers/%s/%s/genetic-%s.xml",
		GetLocalDir(),Data->RobotName,Data->CarType,Data->TrackName);

	// Read meta data file
	void* MetaDataFile = GfParmReadFile(buf, GFPARM_RMODE_REREAD);
	if (!MetaDataFile)
	{
		// Build path to meta data file
		snprintf(buf,sizeof(buf),"%sdrivers/%s/%s/genetic-template.xml",
		GetLocalDir(),Data->RobotName,Data->CarType);

		// Read meta data file
		MetaDataFile = GfParmReadFile(buf, GFPARM_RMODE_REREAD);
		if (!MetaDataFile)
			assert( 0 );
	}

	// Read table of content of meta data file
	TGeneticParameterTOC* TOC = new TGeneticParameterTOC(MetaDataFile);
	TOC->Get();
	Data->Loops = TOC->OptimisationLoops;
	Data->Scale = (float) Data->Loops * Data->Loops;

	// Update author name
	snprintf(Data->AuthorName,sizeof(Data->AuthorNameBuffer),"%s",TOC->Author);
	// Update author name
	snprintf(Data->PrivateSection,sizeof(Data->PrivateSectionBuffer),"%s",TOC->Private);

	// How to handle damage as time penalty
	Data->WeightOfDamages = TOC->WeightOfDamages;

	// If switched off in meta data, disable reading of initial values
	if (!TOC->GetInitialVal)
		Data->GetInitialVal = false;

	// Get tank capacity from car type setup file
	snprintf(buf,sizeof(buf),"%scars/models/%s/%s.xml",
		GetDataDir(),Data->CarType,Data->CarType);
	void* Handle = GfParmReadFile(buf, GFPARM_RMODE_REREAD);
	Data->MaxFuel = (float) GfParmGetNum(Handle, 
		SECT_CAR, PRM_TANK, "l", (float) 60.0);
	GfParmReleaseHandle(Handle);

	// Store tank capacity as initial fuel
	GfParmSetNum(Data->Handle, Data->PrivateSection, PRM_FUEL,    
		(char*) NULL, Data->MaxFuel, -1.0, Data->MaxFuel);

	// Set optimisation flag for robot
	GfParmSetNum(Data->Handle, Data->PrivateSection, PRV_OPTI,    
		(char*) NULL, 1, 0, 1);

	Data->NbrOfParam = GfParmGetEltNb(MetaDataFile,SECT_GLOBAL);
	Data->NbrOfParts = GfParmGetEltNb(MetaDataFile,SECT_LOCAL);

	// We need at least one part to store the offset 
	// as index to the last global parameter
	// and to keep the number of global parameters!
	// Allocate memory for list of parts
	if (Data->NbrOfParts < 1)
  	  Data->Part = new tgenPart[1];
	else
	  Data->Part = new tgenPart[Data->NbrOfParts];

	// Keep the number of global parameters
	Data->Part[0].Offset = Data->NbrOfParam;

	// Loop over all local parameter groups
	for (int I = 0; I < Data->NbrOfParts; I++)
	{
		// Read parameters defined in the current group
	    snprintf(buf,sizeof(buf),"%s/%d/%s",SECT_LOCAL,I+1,SECT_DEFINE);
		TGeneticParameterPart* GroupParam = new TGeneticParameterPart(
			MetaDataFile,"Group Params", buf);
		GroupParam->Get(1+I);
		Data->Part[I].Active = GroupParam->Active;
		// Initialise pointers here in case the part is not active
		Data->Part[I].Parameter = NULL;
		Data->Part[I].Subsection = NULL;

		// If part is set to active
		if (Data->Part[I].Active)
		{
			// Read section parameters form car setup file 
			TGeneticParameterPart* TrackParam = new TGeneticParameterPart(
				Data->Handle, "Track Params", Data->PrivateSection, GroupParam->Parameter);

			// Get number of parts defined in the meta data configuration
			snprintf(buf,sizeof(buf),"%s/%d/%s",SECT_LOCAL,I+1,SECT_PARAM);
			Data->Part[I].Count = GfParmGetEltNb(MetaDataFile,buf);

			// Store the data to the list of parts
			if (TrackParam->Parameter)
				Data->Part[I].Parameter = strdup(TrackParam->Parameter);
			if (GroupParam->Subsection)
				Data->Part[I].Subsection = strdup(GroupParam->Subsection);

			// Get number of local sections defined in the car setup
			snprintf(buf,sizeof(buf),"%s/%s",Data->PrivateSection,Data->Part[I].Subsection);
			Data->Part[I].NbrOfSect = GfParmGetEltNb(Data->Handle,buf);
			// Update number of parameters
			Data->NbrOfParam += Data->Part[I].Count * Data->Part[I].NbrOfSect;

			delete TrackParam;
		}
		delete GroupParam;
	}

	// NbrOfParam now defines the total number of parameters
	// Allocate a list of pointers, one for each parameter
	// GP is the owner of the allocated memory
	Data->GP = (TGeneticParameter**) new TGeneticParameter* [Data->NbrOfParam];
	for (int I = 0; I < Data->NbrOfParam; I++)
		Data->GP[I] = NULL;
	TGeneticParameter* NewGP = NULL;

	//
	// Import global parameters data
	//

	// Initialize robot and car parameters
    for (int I = 0; I < Data->Part[0].Offset; I++)
	{
		NewGP = new TGeneticParameter();
		NewGP->Handle = MetaDataFile;
		// Read meta data from mete data file
		NewGP->Get(I == 0);  
		if (NewGP->Active)			// if parameter is set active
		{
			if (Data->GetInitialVal)// and the flag is set
			{						// we read the starting value from the opened car setup file
				NewGP->GetVal(Data->Handle, (I == 0));
			}
			//NewGP->DisplayParameter();
			// Calculate the total of the individual parameter weights to define 100% probability
			Data->TotalWeight = Data->TotalWeight + NewGP->Weight;
			// Store parameter at the owner
			Data->GP[NextIdx++] = NewGP;
		}
		else 
		  delete NewGP; // If not active free memory

	}

	// This is the true offset to the first local parameter 
	// (may be different caused by inactive parameters)
	Data->Part[0].Offset = NextIdx;

	//
	// Import local parameters data
	//

	// Loop over all parts
	for (int I = 0; I < Data->NbrOfParts; I++)
	{
		// If the part is set to active
		if (Data->Part[I].Active) 
		{	// we look for the details
			Data->Part[I].Offset = NextIdx;

			// Prepare the section depending on the part number
			snprintf(buf,sizeof(buf),"%s/%d/%s",SECT_LOCAL,I+1,SECT_PARAM);

			// Loop over all sections in the group
			for (int J = 0; J < Data->Part[I].NbrOfSect; J++)
			{
				// Loop over all parameters in the part
				for (int K = 0; K < Data->Part[I].Count; K++)
				{
					NewGP = new TGeneticParameter;
					NewGP->Handle = MetaDataFile;
					NewGP->Get((K == 0),buf);
					
					if (!NewGP->Active)		// If parameter is not active ...
						NewGP->Weight = 0;	// ... do not use the weight

					if (Data->GetInitialVal)// if possible ...
					{						// ... read the starting value from car setup file
						NewGP->GetVal(Data->Handle,(K == 0),true);
					}
					//NewGP->DisplayParameter();
					Data->TotalWeight = Data->TotalWeight + NewGP->Weight;
					Data->GP[NextIdx++] = NewGP;
				}
			}
		}
	}

	Data->NbrOfParam = NextIdx;

	delete TOC;

	GfParmReleaseHandle(MetaDataFile);

	ReLogOptim.info("Write parameters to initial xml file\n");
	GfParmWriteFileSDHeader (Data->XmlFileName, Data->Handle, Data->CarType, Data->AuthorName);

	ReLogOptim.info("\n... Import Genetic Parameters\n\n");
}

//
// We did it! Clean up
//
bool
ReCleanupGeneticOptimisation()
{
	if (genOptNeedInit)	// If still needed, ...
		return true;	// ... we do not have to cleanup 

	ReLogOptim.info("============================\n");
	ReLogOptim.info("Cleanup\n");

	// Setup pointer to structure
	tgenData *Data = &TGeneticParameter::Data;

	ReLogOptim.info("Delete parameters\n");
	// Free all parameters allocated
	for (int I = 0; I < Data->NbrOfParam; I++)
	{
		if (Data->GP[I])
		{
		    Data->GP[I]->DisplayStatistik();
			delete Data->GP[I];
		}
	}

	ReLogOptim.info("Delete list of parameters\n");
	// Free list of pointers allocated
	delete [] Data->GP;

	ReLogOptim.info("Delete strings of parts\n");
	// Free all strings allocated
	for (int I = 0; I < Data->NbrOfParts; I++)
	{
		if (Data->Part[I].Parameter)
			free(Data->Part[I].Parameter);
		if (Data->Part[I].Subsection)
			free(Data->Part[I].Subsection);
	}

	ReLogOptim.info("Delete list of parts\n");
	delete [] Data->Part;

	ReLogOptim.info("Release file handle\n");
	GfParmReleaseHandle(Data->Handle);

	ReLogOptim.info("Reset need initialisation flag\n");
	genOptNeedInit = true;

	ReLogOptim.info("Setup path to best setup found\n");
	void* Handle = GfParmReadFile(Data->OptFileName, GFPARM_RMODE_REREAD);

	ReLogOptim.info("Reset fuel control\n");
	GfParmSetNum(Handle, Data->PrivateSection, PRM_FUEL,    
		(char*) NULL, -1, -1.0, Data->MaxFuel);

	ReLogOptim.info("Reset optimisation flag for robot\n");
	GfParmSetNum(Handle, Data->PrivateSection, PRV_OPTI,    
		(char*) NULL, 0, 0, 1);

	ReLogOptim.info("Write parameters to opt file\n");
	GfParmWriteFileSDHeader (Data->OptFileName, Handle, Data->CarType, Data->AuthorName);

	ReLogOptim.info("Write parameters to xml file\n");
	GfParmWriteFileSDHeader (Data->XmlFileName, Handle, Data->CarType, Data->AuthorName);

	ReLogOptim.info("Release file handle\n");
	GfParmReleaseHandle(Handle);

	ReLogOptim.info("============================\n");

	return false;
}

//
// Idetification of parameter to select
//
int
ParameterIndex(tgenData *Data, float Parameter)
{
	// Identify parameter index from total of parameter weight up to here
	float Total = 0.0;

	for (int I = 0; I < Data->NbrOfParam; I++)
	{
		Total += Data->GP[I]->Weight;

		// If we are in the allowed range and the parameter is not selected already
		if ((Total > Parameter) && (!Data->GP[I]->Selected))
			return I; // we will select it
	}

	return -1; // Parameter already selected or out of range
}

//
// Selection and mutation
//
void
SelectParameterAndMutation(tgenData *Data)
{
	// Reset selection flags
	for (int I = 0; I < Data->NbrOfParam; I++)
		Data->GP[I]->Selected = false;

	// Select random number of parameters
	double RandomFloat = (Data->MaxSelected * rand())/RAND_MAX;
	int N = (int) (1 + RandomFloat);

	TGeneticParameter* Param = Data->GP[0];
	double Change; 
	double OldValue;
	int P = 0;

	double Scale = Data->Loops * Data->Loops / Data->Scale;
	ReLogOptim.info("\nRandom parameter variation scale: %g\n",Scale); 

	// Loop over wanted selections
	for (int I = 0; I < N; I++)
	{
		do // Repeat until number of distinct parameters is selected
		{
			// Initialize
			Change = 0.0; 

			//
			// Selection:
			// Generate random parameter index
			RandomFloat = (1.0 * rand())/RAND_MAX;
			float Parameter = (float)((Data->TotalWeight - 0.00001) * RandomFloat); 

			//
			// Mutation:
			// Generate random variation factor 
			// TODO: use generator for normal distributed random numbers
			RandomFloat = (1.0 * rand())/RAND_MAX - 0.5;
			double factor = MIN(1.0,1.1 * Scale) * RandomFloat;

			// While first races only use global parameters
			if ((Parameter > Data->Part[0].Offset) && (OptiCounter < 10))
				continue;

			//
			// Combined selection and mutation loop
			// Check allowed range
			if (Parameter < Data->TotalWeight)
			{
				// Select parameter based on probability weight
				P = ParameterIndex(Data,Parameter);

				// If parameter cannot be selected (to always get distinct seleted parameters)
				if (P == -1)
				{
					do // Repeat until valid selection
					{
						// Try next parameter instead
						Parameter = Parameter + 1;
						// If last was taken restart search at first
						if (Parameter > Data->TotalWeight)
							Parameter = Parameter - Data->TotalWeight;
						P = ParameterIndex(Data,Parameter);

						// TODO: Avoid dead lock
					} while (P == -1); // Repeat until valid selection
				}

//				ReLogOptim.info("\nParameter: %g (Factor: %g) P: %d\n\n",Parameter,factor,P);

				// Get parameter from index
				Param = Data->GP[P];

				// Statistics
				Param->Tries += 1;

				// Calculate a variation that can be stored to the xml files
				double Change0 = Param->Scale * factor; 
				Change = ((int) (Param->Round * Change0)/Param->Round);

//				ReLogOptim.info("%s: (%g<%g<%g): %g * %g = %g -> %g\n",Param->oLabel,Param->Min,Param->Val,Param->Max,Param->Scale,factor,Change0,Change);

				// Check allowed parameter min-max-range
				OldValue = Param->Val;
				Param->Val += (float) Change;
				if (Param->Val < Param->Min)
				{	// Use min instead
//					ReLogOptim.info("%s: = Min (%g)\n",Param->oLabel,Param->Val);
					Param->Val = Param->Min;
				}
				else if (Param->Val > Param->Max)
				{	// Use max instead
//					ReLogOptim.info("%s: = Max (%g)\n",Param->oLabel,Param->Val);
					Param->Val = Param->Max;
				}
				if (fabs(OldValue - Param->Val) < 0.00000001) 
				{	// No change after reading from xml file
//					ReLogOptim.info("%s: Change too small %g\n",Param->oLabel,fabs(OldValue - Param->Val));
					Change = 0.0;
				}
				else
				{	// Successfully changed parameter
					ReLogOptim.info("%s: Val: %g (Change: %g)\n",Param->Label,Param->Val,Change);
					Param->Selected = true;
					Param->Changed += 1;
					Param->DisplayParameter();
				}

				// Rescale
				Scale += 0.0001;
				Scale *= 1.1;
			}
			else
				Scale += 0.0001;

		} while (fabs(Change) < 0.0000001); // repeat if no change
	} // Loop until all selections are done with a rounded change > 0

/*	// For Debug
	//
	// Report static of parameter usage
	//
	for (int I = 0; I < Data->Part[0].Offset; I++)
	{
		if (Data->GP[I]->Active)
			Data->GP[I]->rDisplayStatistik();
	}
*/
/*  // For Debug
	//
	// Report parameter state
	//
	for (int I = 0; I < Data->Part[0].Offset; I++)
	{
		if (Data->GP[I]->Active)
			Data->GP[I]->DisplayParameter();
	}
*/

}

//
// Optimisation Control
//
int
ReEvolution()
{
	ReLogOptim.info(">>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");

	/* DEBUG, ONLY FOR WINDOWS
	// Check memory consumption
	MEMORYSTATUSEX statex;
	statex.dwLength = sizeof (statex);
	GlobalMemoryStatusEx (&statex);
	printf (TEXT("0. There are  %*I64d KB\n"), WIDTH, lastFreeMem);
	DWORDLONG FreeMem = statex.ullAvailPhys/DIV;
	printf (TEXT("1. There are  %*I64d KB\n"), WIDTH, FreeMem);
	DWORDLONG diff = lastFreeMem - statex.ullAvailPhys/DIV;
	printf (TEXT("=. Difference %*I64d KB\n"), WIDTH, diff);
	lastFreeMem = statex.ullAvailPhys/DIV;
	_tgf_mallocBalance(); // For Debug check allocation/free balance
	*/

	// Setup shortcuts
	tgenData *Data = &TGeneticParameter::Data;
	void* Handle = Data->Handle; 
	double TotalLapTime = Data->WeightedBestLapTime;

	if (Data->First) 
	{
		// Run once ...
		
		// For parameter selection we use the default generator 
		// for equally distributed random numbers
		// and it is initialized already by the SD code

		// TODO: 
		// Initialize a "private" random number generator for mutation
		// This should be a generator for normal distributed random numbers
		// srand_normal((unsigned)time(NULL));

		// First race was done with the initial parameters to get the reference laptime
		ReLogOptim.info("Initial Lap Time : %g\n",TotalLapTime);

		// Get range for number of parameters to select for variation
		Data->MaxSelected = MIN(8,1 + Data->NbrOfParam / 2);
		ReLogOptim.info("Nbr. of selected : %d\n",Data->MaxSelected);
		if (Data->MaxSelected < 1)
			assert( 0 );

		// ... run once
	}
	else
	{
		// Run always ...

		// Count the loops and show the status
		OptiCounter++;
		ReLogOptim.info("Loop %d (Still to do %d loops)\n",OptiCounter,Data->Loops);

		// ... run always
	}

	//
	// Check results of last race
	//
	// TODO:
	// Use additional results like 
	// min speed, max yaw, fuel consumption, ...
	if (TotalLapTime < Data->LastWeightedBestLapTime)
	{
		if (!Data->First)
		{
			// Wow, we got a new best lap time
			ReLogOptim.info("New best Lap Time: %g\n",TotalLapTime);
		}

		// Push the result data to be reused in case of a back step later
		Data->LastWeightedBestLapTime = Data->WeightedBestLapTime;
		Data->LastDamagesTotal = Data->DamagesTotal;	

		// Store current parameters to be reused in case of a back step later
		for (int I = 0; I < Data->NbrOfParam; I++)
			Data->GP[I]->LastVal = Data->GP[I]->OptVal = Data->GP[I]->Val;

		// Write the best so far setup to the opt-file
		GfParmWriteFileSDHeader (Data->OptFileName, Handle, Data->CarType, Data->AuthorName);
		ReLogOptim.info("Stored to .opt\n");
	}
	else if (0.99 * TotalLapTime < Data->LastWeightedBestLapTime)
	{
		// We did not find a better set of parameters but we are not bad.
		// Let's use the child to start the next search step of optimisation
		ReLogOptim.info("Total Lap Time   : %g (Best so far: %g)\n",TotalLapTime,Data->LastWeightedBestLapTime);
	}            
	else
	{
		// We got a bad result.
		// Let's use the remote anchestor to start the next search step of optimisation
		ReLogOptim.info("Total Lap Time   : %g (Bad!)\n",TotalLapTime);

		// Pop the stored result data
		Data->WeightedBestLapTime = Data->LastWeightedBestLapTime;
		Data->DamagesTotal = Data->LastDamagesTotal;	

		// Pop the stored genetic parameter data
		for (int I = 0; I < Data->NbrOfParam; I++)
			Data->GP[I]->Val = Data->GP[I]->OptVal;

		ReLogOptim.info("Back to last .opt\n");
		ReLogOptim.info("Old Best Lap Time: %g\n",Data->WeightedBestLapTime);
	}

	if (Data->First)
	{
		// Run once ...
		ReLogOptim.info("\nStart Optimisation\n");
		// ... run once
	}

	//
	// Next Race -> Selection and mutation of arameters
	//
	SelectParameterAndMutation(Data);

	//
	// Export global parameters to xml data
	//
	for (int I = 0; I < Data->Part[0].Offset; I++)
	{
		if (Data->GP[I]->Active)
			Data->GP[I]->SetVal(Handle);
	}

	//
	// Export local parameters to xml data
	//
	for (int I = 0; I < Data->NbrOfParts; I++)
	{
		if (Data->Part[I].Active)
		{
			for (int J = 0; J < Data->Part[I].NbrOfSect; J++)
			{
				for (int K = 0; K < Data->Part[I].Count; K++)
				{
					int Index = Data->Part[I].Offset + Data->Part[I].Count * J + K;
					if (Data->GP[Index]->Active)
					  Data->GP[Index]->SetVal(Handle,J+1);
				}
			}
		}
	}

	// Write xml data to file
	GfParmWriteFileSDHeader (Data->XmlFileName, Handle, Data->CarType, Data->AuthorName);

	ReLogOptim.info("<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n");

	// Reset run once flag
	Data->First = false;

	// Check number of loops still to do
	if (Data->Loops--)
		return RM_SYNC | RM_NEXT_STEP;	// Next optimisation loop
	else
		return RM_SYNC;					// All planned loops are done!
}


