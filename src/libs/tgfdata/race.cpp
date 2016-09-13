/***************************************************************************

    file                 : competitors.cpp
    created              : Sun Nov 21 19:00:00 CET 2010
    copyright            : (C) 2010 by Jean-Philippe MEURET
    web                  : speed-dreams.sourceforge.net
    version              : $Id: race.cpp 5899 2014-12-17 21:00:23Z wdbee $
                      
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <cstdlib>
#include <sstream>
#include <map>
#include <algorithm>

#include <raceman.h>
#include <robot.h>
#include <car.h>

#include "cars.h"
#include "drivers.h"
#include "tracks.h"
#include "racemanagers.h"
#include "race.h"


// Constants.
static const char* DisplayModeNames[RM_DISP_MODE_NUMBER] =
	{ RM_VAL_INVISIBLE, RM_VAL_VISIBLE, RM_VAL_SIMUSIMU, RM_VAL_SIMUSIMU };
static const char* TimeOfDaySpecNames[GfRace::nTimeSpecNumber] = RM_VALS_TIME;
static const char* CloudsSpecNames[GfRace::nCloudsSpecNumber] = RM_VALS_CLOUDS;
static const char* RainSpecNames[GfRace::nRainSpecNumber] = RM_VALS_RAIN;


// Private data for GfRace
class GfRace::Private
{
public:

	Private() : bIsDirty(false), pRaceMan(0), nMaxCompetitors(0), nFocusedItfIndex(-1),
				nEventInd(0), nSessionInd(0), hparmResults(0) {};
	
public:

	// True if no change occurred since last reset().
	bool bIsDirty;

	// The "parent" race manager.
	GfRaceManager* pRaceMan;

	// Race parameters, for each configured session.
	std::map<std::string, Parameters*> mapParametersBySession;
	
	// Max authorized number of competitors.
	unsigned nMaxCompetitors;
	
	// One GfDriver pointer for each competitor (order = race starting grid).
	std::vector<GfDriver*> vecCompetitors;

	// Map for quick access to GfDriver by { module name, interface index }
	typedef std::map<std::pair<std::string, int>, GfDriver*> TMapCompetitorsByKey;
	TMapCompetitorsByKey mapCompetitorsByKey;

	// Focused competitor (what for ?).
	std::string strFocusedModuleName;
	int nFocusedItfIndex;

	// Index of the current event in the race manager event list (event ~ race day on 1 track).
	unsigned nEventInd;

	// Index of the current session in the race manager session list (sessions inside each event).
	unsigned nSessionInd;

	// Results params.
	void* hparmResults;
	
	// An empty string.
	static const std::string strEmpty;

	// An empty string vector.
	static const std::vector<std::string> vecstrEmpty;
};

const std::string GfRace::Private::strEmpty;
const std::vector<std::string> GfRace::Private::vecstrEmpty;


// GfRace class.
GfRace::GfRace()
{
	_pPrivate = new GfRace::Private;
}

GfRace::~GfRace()
{
	clear();
	
	delete _pPrivate;
	_pPrivate = 0;
}

void GfRace::clear()
{
	_pPrivate->pRaceMan = 0;
	
	std::map<std::string, Parameters*>::const_iterator itSessionParams;
	for (itSessionParams = _pPrivate->mapParametersBySession.begin();
		 itSessionParams != _pPrivate->mapParametersBySession.end(); itSessionParams++)
		delete itSessionParams->second;
	_pPrivate->mapParametersBySession.clear();
	_pPrivate->nMaxCompetitors = 0;
	_pPrivate->mapCompetitorsByKey.clear();
	_pPrivate->vecCompetitors.clear();
	_pPrivate->strFocusedModuleName.clear();
	_pPrivate->nFocusedItfIndex = 0;
	_pPrivate->nEventInd = 0;
	_pPrivate->nSessionInd = 0;
	_pPrivate->hparmResults = 0;
}

void GfRace::load(GfRaceManager* pRaceMan, bool bKeepHumans, void* hparmResults)
{
//  	GfLogDebug("GfRace::load(mgr='%s', humans=%d, hRes=%s)\n", pRaceMan->getName().c_str(),
//  			   (int)bKeepHumans, hparmResults ? GfParmGetFileName(hparmResults) : "none");

	// Clear the race.
	clear();

	// Save the new race manager.
	_pPrivate->pRaceMan = pRaceMan;

	// Now we should be consistent with the race params (in memory).
	_pPrivate->bIsDirty = false;

	// Check if usable, and exit if not.
	if (!_pPrivate->pRaceMan)
		return;

	void* hparmRaceMan = pRaceMan->getDescriptorHandle();

	if (!hparmRaceMan)
		return;

	_pPrivate->hparmResults = hparmResults;
	
	// Load the current event index from the results file (or re-initialize it to 0)
	// (the track on which the race will take place).
	_pPrivate->nEventInd = 0;
	if (hparmResults)
	{
		_pPrivate->nEventInd =
			(unsigned)GfParmGetNum(hparmResults, RE_SECT_CURRENT, RE_ATTR_CUR_TRACK, NULL, 1) - 1;
	}
	
	// Load the current session index from the results file (or re-initialize it to 0).
	_pPrivate->nSessionInd = 0;
	if (hparmResults)
	{
		_pPrivate->nSessionInd =
			(unsigned)GfParmGetNum(hparmResults, RE_SECT_CURRENT, RE_ATTR_CUR_RACE, NULL, 1) - 1;
	}

	// Load the parameters for all the referenced sessions, even if not user-configurable
	// ("All Sessions" included, if any).
	// 1) Pre-load the parameters for user-configurable sessions.
 	const std::vector<std::string>& vecRefSessionNames = _pPrivate->pRaceMan->getSessionNames();
	for (int nConfInd = 1; nConfInd <= GfParmGetEltNb(hparmRaceMan, RM_SECT_CONF); nConfInd++)
	{
		std::ostringstream ossSecPath;
		ossSecPath << RM_SECT_CONF << '/' << nConfInd;

		// Ignore non "race config" config. types.
		const std::string strConfType =
			GfParmGetStr(hparmRaceMan, ossSecPath.str().c_str(), RM_ATTR_TYPE, "");
		if (strConfType != RM_VAL_RACECONF)
			continue;
		
		// Ignore configs with no actual option.
		const std::string strOptionsSecPath = ossSecPath.str() + '/' + RM_SECT_OPTIONS;
		const int nOptions = GfParmGetEltNb(hparmRaceMan, strOptionsSecPath.c_str());
		if (nOptions == 0) 
		{
			GfLogWarning("Empty %s section in %s\n",
						 strOptionsSecPath.c_str(), GfParmGetFileName(hparmRaceMan));
			continue;
		}

		// Ignore non members of the raceman session name list, but "All Sessions".
		const std::string strSessionName =
			GfParmGetStr(hparmRaceMan, ossSecPath.str().c_str(), RM_ATTR_RACE, "Race");
		if (std::find(vecRefSessionNames.begin(), vecRefSessionNames.end(),
					  strSessionName) == vecRefSessionNames.end()
			&& strSessionName != RM_VAL_ANYRACE)
			continue;

		// Create and register a new Parameters instance for this session.
		Parameters* pSessionParams = new Parameters;
		_pPrivate->mapParametersBySession[strSessionName] = pSessionParams;

		// Load the list (actually a bit field) of configurable options.
		pSessionParams->bfOptions = 0;
		for (int nOptInd = 1; nOptInd <= nOptions; nOptInd++)
		{
			ossSecPath.str("");
			ossSecPath << strOptionsSecPath << '/' << nOptInd;
			const std::string strOption =
				GfParmGetStr(hparmRaceMan, ossSecPath.str().c_str(), RM_ATTR_TYPE, "");
			if (strOption == RM_VAL_CONFRACELEN)
				pSessionParams->bfOptions |= RM_CONF_RACE_LEN;
			else if (strOption == RM_VAL_CONFDISPMODE)
				pSessionParams->bfOptions |= RM_CONF_DISP_MODE;
			else if (strOption == RM_VAL_CONFTIMEOFDAY)
				pSessionParams->bfOptions |= RM_CONF_TIME_OF_DAY;
			else if (strOption == RM_VAL_CONFCLOUDCOVER)
				pSessionParams->bfOptions |= RM_CONF_CLOUD_COVER;
			else if (strOption == RM_VAL_CONFRAINFALL)
				pSessionParams->bfOptions |= RM_CONF_RAIN_FALL;
		}
	}

	// 2) Pre-load the parameters for other sessions.
	std::vector<std::string>::const_iterator itRefSessionName;
	for (itRefSessionName = vecRefSessionNames.begin();
		 itRefSessionName != vecRefSessionNames.end(); itRefSessionName++)
	{
		// Only non-configurable sessions actually.
		if (_pPrivate->mapParametersBySession.find(*itRefSessionName)
			== _pPrivate->mapParametersBySession.end())
		{
			Parameters* pSessionParams = new Parameters;
			_pPrivate->mapParametersBySession[*itRefSessionName] = pSessionParams;
			pSessionParams->bfOptions = 0;
		}
	}
	
	// 3) Load the actual parameter values for each referenced session
	//    (user-configurable or not, the "All Sessions" one included if needed).
	std::map<std::string, Parameters*>::iterator itSessionParams;
	for (itSessionParams = _pPrivate->mapParametersBySession.begin();
		 itSessionParams != _pPrivate->mapParametersBySession.end(); itSessionParams++)
	{
		const char* pszSessionName = itSessionParams->first.c_str();
		Parameters* pSessionParams = itSessionParams->second;

		// Use invalid values as default, in order to remember it at store time.
		pSessionParams->nLaps =
			(int)GfParmGetNum(hparmRaceMan, pszSessionName, RM_ATTR_LAPS, NULL, -1);
		pSessionParams->nDistance =
			(int)GfParmGetNum(hparmRaceMan, pszSessionName, RM_ATTR_DISTANCE, "km", -1);
		pSessionParams->nDuration =
			(int)GfParmGetNum(hparmRaceMan, pszSessionName, RM_ATTR_SESSIONTIME, "s", -1);

		const std::string strDispMode =
			GfParmGetStr(hparmRaceMan, pszSessionName, RM_ATTR_DISPMODE, "");
		if (strDispMode.empty())
			pSessionParams->bfDisplayMode = RM_DISP_MODE_UNDEFINED;
		else if (strDispMode == RM_VAL_INVISIBLE)
			pSessionParams->bfDisplayMode = RM_DISP_MODE_NONE;
		else if (strDispMode == RM_VAL_VISIBLE)
			pSessionParams->bfDisplayMode = RM_DISP_MODE_NORMAL;
		else if (strDispMode == RM_VAL_SIMUSIMU)
			pSessionParams->bfDisplayMode = RM_DISP_MODE_SIMU_SIMU;
		else
		{
			GfLogError("Unsupported display mode '%s' loaded from race file ; "
					   "assuming 'normal' mode\n", strDispMode.c_str());
			pSessionParams->bfDisplayMode = RM_DISP_MODE_NORMAL;
		}

		const std::string strTimeOfDaySpec =
			GfParmGetStr(hparmRaceMan, pszSessionName, RM_ATTR_TIME_OF_DAY, "");
		pSessionParams->eTimeOfDaySpec = nTimeSpecNumber;
		for (int i = 0; i < nTimeSpecNumber; i++)
			if (strTimeOfDaySpec == TimeOfDaySpecNames[i])
			{
				pSessionParams->eTimeOfDaySpec = (ETimeOfDaySpec)i;
				break;
			}

		const std::string strCloudsSpec =
			GfParmGetStr(hparmRaceMan, pszSessionName, RM_ATTR_CLOUDS, "");
		pSessionParams->eCloudsSpec = nCloudsSpecNumber;
		for (int i = 0; i < nCloudsSpecNumber; i++)
			if (strCloudsSpec == CloudsSpecNames[i])
			{
				pSessionParams->eCloudsSpec = (ECloudsSpec)i;
				break;
			}

		const std::string strRainSpec =
			GfParmGetStr(hparmRaceMan, pszSessionName, RM_ATTR_RAIN, "");
		pSessionParams->eRainSpec = nRainSpecNumber;
		for (int i = 0; i < nRainSpecNumber; i++)
			if (strRainSpec == RainSpecNames[i])
			{
				pSessionParams->eRainSpec = (ERainSpec)i;
				break;
			}

 		// GfLogDebug("GfRace::load(...) : %s : opts=%02x, "
 		// 		   "laps=%d, dist=%d, dur=%d, disp=0x%x, tod=%d, clds=%d, rain=%d\n",
 		// 		   pszSessionName, pSessionParams->bfOptions,
 		// 		   pSessionParams->nLaps, pSessionParams->nDistance, pSessionParams->nDuration,
 		// 		   pSessionParams->bfDisplayMode, pSessionParams->eTimeOfDaySpec,
 		// 		   pSessionParams->eCloudsSpec, pSessionParams->eRainSpec);
	}

	// Load the max number of competitors from the raceman params and / or the results file.
	// TODO: Make nMaxCompetitors value consistent with racemain::ReRaceStart.
    _pPrivate->nMaxCompetitors =
		(unsigned)GfParmGetNum(hparmRaceMan, RM_SECT_DRIVERS, RM_ATTR_MAXNUM, NULL, 0);

//  	GfLogDebug("GfRace::load(...) : maxComp=%u\n", _pPrivate->nMaxCompetitors);

	// Load the starting grid type from the raceman params.
	std::string strGridType =
		GfParmGetStr(hparmRaceMan, pRaceMan->getSessionName(_pPrivate->nSessionInd).c_str(),
					 RM_ATTR_START_ORDER, RM_VAL_DRV_LIST_ORDER);

	// Determine the previous session index and track (may refer to the previous event).
	// Note: Will only be used if the used starting grid is not the initial competitors list.
	GfTrack* pPrevSessionTrack = getTrack();
//  	GfLogDebug("GfRace::load(...) : curTrk=%s, curSess=%s, gridType=%s\n",
// 			   pPrevSessionTrack->getName().c_str(),
//  			   pRaceMan->getSessionName(_pPrivate->nSessionInd).c_str(), strGridType.c_str());

	int nPrevSessionInd = (int)_pPrivate->nSessionInd - 1;
	if (strGridType != RM_VAL_DRV_LIST_ORDER)
	{
		// Fix previous session if it pushes us back to the previous event
		// Note: This is not supported yet by the race engine : see racemain::ReRaceStart.
		if (nPrevSessionInd < 0)
		{
			nPrevSessionInd = pRaceMan->getSessionCount() - 1;
			pPrevSessionTrack = pRaceMan->getPreviousEventTrack(_pPrivate->nEventInd);
			if (!pPrevSessionTrack)
			{
				strGridType = RM_VAL_DRV_LIST_ORDER;
				pPrevSessionTrack = getTrack();
				GfLogWarning("No previous session for %s / %s"
							 "=> starting grid = initial competitors list (%s)\n",
							 pPrevSessionTrack->getName().c_str(),
							 pRaceMan->getSessionName(_pPrivate->nSessionInd).c_str(),
							 GfParmGetFileName(hparmResults));
			}
		}
		
//  		GfLogDebug("GfRace::load(...) : prevTrk=%s, prevSess=%s, gridType=%s\n",
//  				   pPrevSessionTrack->getName().c_str(),
//  				   pRaceMan->getSessionName(nPrevSessionInd).c_str(), strGridType.c_str());
	}

	// Determine how to load the starting grid from the raceman params / results.
	void* hparmStartGrid;
	bool bReversedGrid;
	std::string strDrvSec;
	const char* pszModulePropName;
	const char* pszItfIndexPropName;
	if (strGridType != RM_VAL_DRV_LIST_ORDER)
	{
		// Params to read the starting grid from.
		hparmStartGrid = hparmResults;

		// Reversed starting grid.
		bReversedGrid = strGridType == RM_VAL_LAST_RACE_RORDER;

		// Params section to read the starting grid from.
		strDrvSec = pPrevSessionTrack->getName() + '/' + RE_SECT_RESULTS
			+ '/' + pRaceMan->getSessionName(nPrevSessionInd) + '/' + RE_SECT_RANK;
		

		// Module and interface index property names.
		pszModulePropName = RE_ATTR_MODULE;
		pszItfIndexPropName = RE_ATTR_IDX;
	}
	
	else
	{
		// Params to read the starting grid from.
		hparmStartGrid = hparmRaceMan;
		
		// Driver starting order.
		bReversedGrid = false;

		// Params section to read the starting grid from.
		strDrvSec = RM_SECT_DRIVERS;
		
		// Module and interface index property names.
		pszModulePropName = RM_ATTR_MODULE;
		pszItfIndexPropName = RM_ATTR_IDX;
	}

	// Finally load the competitors in the specified starting grid order.
	const int nCompetitors = GfParmGetEltNb(hparmStartGrid, strDrvSec.c_str());

//  	GfLogDebug("GfRace::load(...) : drvSec=%s, revGrid=%s, nComps=%d (max=%u), hGrid=%s\n",
// 			   strDrvSec.c_str(), bReversedGrid ? "true" : "false",
//  			   nCompetitors, _pPrivate->nMaxCompetitors,
//  			   hparmStartGrid == hparmResults ? "hRes" : GfParmGetFileName(hparmStartGrid));

	std::ostringstream ossDrvSecPath;
	const int nCompIndexDelta = bReversedGrid ? -1 : +1;
	const int nEndCompIndex = bReversedGrid ? 0 : nCompetitors + 1;
	int nCompIndex = bReversedGrid ? nCompetitors : 1;
	nCompIndex -= nCompIndexDelta;
    while ((nCompIndex += nCompIndexDelta) != nEndCompIndex)
	{
		// Get driver infos from the the starting grid.
		ossDrvSecPath.str("");
		ossDrvSecPath << strDrvSec << '/' << nCompIndex;
		const char* pszModName =
			GfParmGetStr(hparmStartGrid, ossDrvSecPath.str().c_str(), pszModulePropName, "");
		const int nItfIndex =
			(int)GfParmGetNum(hparmStartGrid, ossDrvSecPath.str().c_str(), pszItfIndexPropName, NULL, 0);

//  		GfLogDebug("Competitor #%d : %s#%d\n", nCompIndex, pszModName, nItfIndex);

		// Try and retrieve this driver among all the available ones.
		GfDriver* pCompetitor = GfDrivers::self()->getDriver(pszModName, nItfIndex);
		if (!pCompetitor)
		{
			GfLogInfo("Ignoring '%s' driver #%d : not found in available drivers\n",
						 pszModName, nItfIndex);
			
			// We are no more consistent with the race params (in memory).
			_pPrivate->bIsDirty = true;
			
			continue;
		}

		// We've got it but can't keep it for the race,
		// because there is a threshold on the number of competitors.
		if (!acceptsMoreCompetitors())
		{
			GfLogInfo("Ignoring subsequent competitors (max=%u)\n",
						 _pPrivate->nMaxCompetitors);
			
			// We are no more consistent with the race params (in memory).
			_pPrivate->bIsDirty = true;
			
			break;
		}

		// We've got it but can't keep it for the race,
		// because we are requested to exclude humans from the race.
		if (!bKeepHumans && pCompetitor->isHuman())
		{
			GfLogInfo("Ignoring '%s' (%s' #%d) : humans excluded from this race\n",
						 pCompetitor->getName().c_str(), pszModName, nItfIndex);
			
			// We are no more consistent with the race params (in memory).
			_pPrivate->bIsDirty = true;
			
			break;
		}
		
		// We've got it and can keep it for the race => make it a competitor
		const char* pszSkinName =
			GfParmGetStr(hparmStartGrid, ossDrvSecPath.str().c_str(), RM_ATTR_SKINNAME, "");
		const int nSkinTargets =
			(int)GfParmGetNum(hparmStartGrid, ossDrvSecPath.str().c_str(), RM_ATTR_SKINTARGETS, NULL, 0);
		const bool bExtended =
			GfParmGetNum(hparmStartGrid, ossDrvSecPath.str().c_str(), RM_ATTR_EXTENDED, NULL, 0)
			? true : false;

		// Get the chosen car for the race if any specified (extended drivers only).
		const GfCar* pCarForRace = 0;
		if (bExtended)
		{
			ossDrvSecPath.str("");
			ossDrvSecPath << RM_SECT_DRIVERINFO << '/' << pszModName
						  << '/' << (bExtended ? 1 : 0) << '/' << nItfIndex;
			const char* pszCarId =
				GfParmGetStr(hparmRaceMan, ossDrvSecPath.str().c_str(), RM_ATTR_CARNAME, "<none>");
			pCarForRace = GfCars::self()->getCar(pszCarId);
		}
		//  			GfLogDebug("GfRace::load(...) : car=%s (%s)\n",
		//  					   pCarForRace ? pCarForRace->getName().c_str() : pCompetitor->getCar()->getName().c_str(),
		//  					   pCarForRace ? "extended" : "standard");

		// Update the driver.
		GfDriverSkin skin(pszSkinName);
		skin.setTargets(nSkinTargets);
		pCompetitor->setSkin(skin);
		if (pCarForRace) // Override default car with the one chosen for the race if any.
			pCompetitor->setCar(pCarForRace);
			
		// Check if this driver can compete in this race.
		if (acceptsDriverType(pCompetitor->getType())
			&& acceptsCarCategory(pCompetitor->getCar()->getCategoryId()))
		{
			// Update the GfRace.
			appendCompetitor(pCompetitor);
		}
		else
		{
			GfLogInfo("Ignoring '%s' (%s' #%d) : Type or car category not accepted by the race\n",
						 pCompetitor->getName().c_str(), pszModName, nItfIndex);
			
			// We are no more consistent with the race params (in memory).
			_pPrivate->bIsDirty = true;
		}				
	}
	
	// Load focused competitor data from the raceman params.
    _pPrivate->strFocusedModuleName =
		GfParmGetStr(hparmRaceMan, RM_SECT_DRIVERS, RM_ATTR_FOCUSED, "");
    _pPrivate->nFocusedItfIndex =
		(int)GfParmGetNum(hparmRaceMan, RM_SECT_DRIVERS, RM_ATTR_FOCUSEDIDX, NULL, 0);
}

void GfRace::store()
{
	// Warning: For the moment, only saves to the race config. params,
	//          never to the results params, even if some data was loaded from it ...
	
	if (!_pPrivate->pRaceMan)
		return;

	void* hparmRaceMan = _pPrivate->pRaceMan->getDescriptorHandle();

	if (!hparmRaceMan)
		return;

	// Save race manager level data.
	_pPrivate->pRaceMan->store();
	
	// Save the parameters for all the sessions :
	// for a given parameter of a given non-"All Sessions" session,
	// save the parameter value if valid, otherwise use the coresponding one's value
	// from the "All Sessions" session if valid, otherwise don't save.
	// For the parameters of the "All Sessions" session, save only if valid.
	std::map<std::string, Parameters*>::const_iterator itSessionParams;
	for (itSessionParams = _pPrivate->mapParametersBySession.begin();
		 itSessionParams != _pPrivate->mapParametersBySession.end(); itSessionParams++)
	{
		const char* pszSessionName = itSessionParams->first.c_str();
		const Parameters* pSessionParams = itSessionParams->second;
		
		// Write valid param. value for the current session,
		// or remove the parameter if invalid value.
		if (pSessionParams->nLaps >= 0)
			GfParmSetNum(hparmRaceMan, pszSessionName, RM_ATTR_LAPS,
						 (char*)NULL, (tdble)pSessionParams->nLaps);
		else
			GfParmRemove(hparmRaceMan, pszSessionName, RM_ATTR_LAPS);

		if (pSessionParams->nDistance >= 0)
			GfParmSetNum(hparmRaceMan, pszSessionName, RM_ATTR_DISTANCE,
						 "km", (tdble)pSessionParams->nDistance);
		else
			GfParmRemove(hparmRaceMan, pszSessionName, RM_ATTR_DISTANCE);
		
		if (pSessionParams->nDuration >= 0)
			GfParmSetNum(hparmRaceMan, pszSessionName, RM_ATTR_SESSIONTIME,
						 "s", (tdble)pSessionParams->nDuration);
		else
			GfParmRemove(hparmRaceMan, pszSessionName, RM_ATTR_SESSIONTIME);
		
		if (pSessionParams->bfDisplayMode != RM_DISP_MODE_UNDEFINED)
			GfParmSetStr(hparmRaceMan, pszSessionName, RM_ATTR_DISPMODE,
						 DisplayModeNames[pSessionParams->bfDisplayMode]);
		else
			GfParmRemove(hparmRaceMan, pszSessionName, RM_ATTR_DISPMODE);
		
		if (pSessionParams->eTimeOfDaySpec != nTimeSpecNumber)
			GfParmSetStr(hparmRaceMan, pszSessionName, RM_ATTR_TIME_OF_DAY,
						 TimeOfDaySpecNames[pSessionParams->eTimeOfDaySpec]);
		else
			GfParmRemove(hparmRaceMan, pszSessionName, RM_ATTR_TIME_OF_DAY);
		
		if (pSessionParams->eCloudsSpec != nCloudsSpecNumber)
			GfParmSetStr(hparmRaceMan, pszSessionName, RM_ATTR_CLOUDS,
						 CloudsSpecNames[pSessionParams->eCloudsSpec]);
		else
			GfParmRemove(hparmRaceMan, pszSessionName, RM_ATTR_CLOUDS);
		
		if (pSessionParams->eRainSpec != nRainSpecNumber)
			GfParmSetStr(hparmRaceMan, pszSessionName, RM_ATTR_RAIN,
						 RainSpecNames[pSessionParams->eRainSpec]);
		else
			GfParmRemove(hparmRaceMan, pszSessionName, RM_ATTR_RAIN);
		
 		// GfLogDebug("GfRace::store(...) : %s params : "
 		// 		   "laps=%d, dist=%d, dur=%d, disp=0x%x, tod=%d, clds=%d, rain=%d\n",
 		// 		   pszSessionName, pSessionParams->nLaps,
 		// 		   pSessionParams->nDistance, pSessionParams->nDuration,
 		// 		   pSessionParams->bfDisplayMode, pSessionParams->eTimeOfDaySpec,
 		// 		   pSessionParams->eCloudsSpec, pSessionParams->eRainSpec);
	}
	
	// Clear the drivers list and race starting grid.
    GfParmListClean(hparmRaceMan, RM_SECT_DRIVERS);
    GfParmListClean(hparmRaceMan, RM_SECT_DRIVERS_RACING);

	// And then rebuild it from the current Competitors list state
	// (for each competitor, module name, interface index, car name if human,
	//  skin name and targets if needed).
	std::vector<GfDriver*>::const_iterator itComp;
	for (itComp = _pPrivate->vecCompetitors.begin();
		 itComp != _pPrivate->vecCompetitors.end(); itComp++)
	{
		std::ostringstream ossDrvSecPath;
		ossDrvSecPath << RM_SECT_DRIVERS
					   << '/' << (unsigned)(itComp - _pPrivate->vecCompetitors.begin() + 1);
		const std::string strDrvSec(ossDrvSecPath.str());
		
		GfParmSetNum(hparmRaceMan, strDrvSec.c_str(), RM_ATTR_IDX, (char*)NULL,
					 (tdble)(*itComp)->getInterfaceIndex());
		GfParmSetStr(hparmRaceMan, strDrvSec.c_str(), RM_ATTR_MODULE,
					 (*itComp)->getModuleName().c_str());
		
		const GfCar* pCar = (*itComp)->getCar();
		if (pCar && (*itComp)->isHuman())
		{
			/* Set extended */
			GfParmSetNum(hparmRaceMan, strDrvSec.c_str(), RM_ATTR_EXTENDED, NULL, 1);
			
			ossDrvSecPath.str("");
			ossDrvSecPath << RM_SECT_DRIVERINFO << '/' << (*itComp)->getModuleName()
						   << '/' << 1 /*extended*/ << '/' << (*itComp)->getInterfaceIndex();

			GfParmSetStr(hparmRaceMan, ossDrvSecPath.str().c_str(), RM_ATTR_CARNAME,
						 pCar->getId().c_str());

			// Save also the chosen car as the default one for this human driver
			// (may be needed later for races where it is not specified in <race>.xml)
			std::ostringstream ossFilePath;
			ossFilePath << GetLocalDir() << "drivers/" << (*itComp)->getModuleName()
						   << '/' << (*itComp)->getModuleName() << PARAMEXT;
			void* hparmRobot = GfParmReadFile(ossFilePath.str().c_str(), GFPARM_RMODE_STD);
			ossDrvSecPath.str("");
			ossDrvSecPath << ROB_SECT_ROBOTS << '/' << ROB_LIST_INDEX
						   << '/' << (*itComp)->getInterfaceIndex();
			GfParmSetStr(hparmRobot, ossDrvSecPath.str().c_str(), ROB_ATTR_CAR,
						 pCar->getId().c_str());
			GfParmWriteFile(NULL, hparmRobot, (*itComp)->getModuleName().c_str());
			GfParmReleaseHandle(hparmRobot);
		}
		else
		{
			/* Not extended for robots yet in driverconfig */
			GfParmSetNum(hparmRaceMan, strDrvSec.c_str(), RM_ATTR_EXTENDED, NULL, 0);
		}

		// Skin and skin targets.
		const GfDriverSkin& skin = (*itComp)->getSkin();
		GfParmSetNum(hparmRaceMan, strDrvSec.c_str(), RM_ATTR_SKINTARGETS, NULL,
					 (tdble)skin.getTargets());
		if ((!skin.getName().empty())
			|| GfParmGetStr(hparmRaceMan, strDrvSec.c_str(), RM_ATTR_SKINNAME, 0))
			GfParmSetStr(hparmRaceMan, strDrvSec.c_str(), RM_ATTR_SKINNAME,
						 skin.getName().c_str());
	}
	
	// Save focused competitor data to the raceman params.
	GfParmSetStr(hparmRaceMan, RM_SECT_DRIVERS, RM_ATTR_FOCUSED,
				 _pPrivate->strFocusedModuleName.c_str());
	GfParmSetNum(hparmRaceMan, RM_SECT_DRIVERS, RM_ATTR_FOCUSEDIDX, NULL,
				 (tdble)_pPrivate->nFocusedItfIndex);

	// Now we are consistent with the race params (in memory).
	_pPrivate->bIsDirty = false;
}

bool GfRace::isDirty() const
{
	return _pPrivate->bIsDirty || (_pPrivate->pRaceMan && _pPrivate->pRaceMan->isDirty());
}

void GfRace::setDirty(bool bIsDirty)
{
	_pPrivate->bIsDirty = bIsDirty;
}

GfRaceManager* GfRace::getManager() const
{
	return _pPrivate->pRaceMan;
}

GfRace::Parameters* GfRace::getParameters(const std::string& strSessionName) const
{
	Parameters* pParams = 0;
	
	std::map<std::string, Parameters*>::const_iterator itParams =
		_pPrivate->mapParametersBySession.find(strSessionName);
	if (itParams != _pPrivate->mapParametersBySession.end())
		pParams = itParams->second;
	
	return pParams;
}

int GfRace::getSupportedFeatures() const
{
	int nFeatures = 0;

	std::vector<GfDriver*>::const_iterator itComp;
	for (itComp = _pPrivate->vecCompetitors.begin();
		 itComp != _pPrivate->vecCompetitors.end(); itComp++)
	{
		if (itComp == _pPrivate->vecCompetitors.begin())
			nFeatures = (*itComp)->getSupportedFeatures();
		else
			nFeatures &= (*itComp)->getSupportedFeatures();
	}
	
// 	GfLogDebug("GfRace::getSupportedFeatures() : %02x\n", nFeatures);
			   
	return nFeatures;
}

bool GfRace::acceptsDriverType(const std::string& strType) const
{
	if (!_pPrivate->pRaceMan)
		return false;

	return _pPrivate->pRaceMan->acceptsDriverType(strType);
}

const std::vector<std::string>& GfRace::getAcceptedDriverTypes() const
{
	if (!_pPrivate->pRaceMan)
		return _pPrivate->vecstrEmpty;

	return _pPrivate->pRaceMan->getAcceptedDriverTypes();
}
	
bool GfRace::acceptsCarCategory(const std::string& strCatId) const
{
	if (!_pPrivate->pRaceMan)
		return false;

	return _pPrivate->pRaceMan->acceptsCarCategory(strCatId);
}

const std::vector<std::string>& GfRace::getAcceptedCarCategoryIds() const
{
	if (!_pPrivate->pRaceMan)
		return _pPrivate->vecstrEmpty;

	return _pPrivate->pRaceMan->getAcceptedCarCategoryIds();
}

void GfRace::forceResultsOnly()
{
	std::map<std::string, Parameters*>::iterator itSesParams;
	for (itSesParams = _pPrivate->mapParametersBySession.begin();
		 itSesParams != _pPrivate->mapParametersBySession.end(); itSesParams++)
	{
		GfRace::Parameters* pSesParams = itSesParams->second;
		if (pSesParams->bfDisplayMode != RM_DISP_MODE_UNDEFINED)
			pSesParams->bfDisplayMode &= ~RM_DISP_MODE_NORMAL;
		else
			pSesParams->bfDisplayMode = RM_DISP_MODE_NONE;
	}
	
	// Now we are no more consistent with the race params (in memory).
	_pPrivate->bIsDirty = true;
}

const std::vector<GfDriver*>& GfRace::getCompetitors() const
{
	return _pPrivate->vecCompetitors;
}

unsigned GfRace::getCompetitorsCount() const
{
	return _pPrivate->vecCompetitors.size();
}

bool GfRace::acceptsMoreCompetitors() const
{
	return _pPrivate->vecCompetitors.size() < _pPrivate->nMaxCompetitors;
}

GfDriver* GfRace::getCompetitor(const std::string& strModName, int nItfIndex) const
{
	const std::pair<std::string, int> compKey(strModName, nItfIndex);
	Private::TMapCompetitorsByKey::iterator itComp =
		_pPrivate->mapCompetitorsByKey.find(compKey);
	if (itComp != _pPrivate->mapCompetitorsByKey.end())
		return itComp->second;

	return 0;
}

bool GfRace::hasHumanCompetitors() const
{
	bool bAnswer = false;
	
	std::vector<GfDriver*>::const_iterator itComp;
	for (itComp = _pPrivate->vecCompetitors.begin();
		 itComp != _pPrivate->vecCompetitors.end(); itComp++)
	{
		if ((*itComp)->isHuman())
		{
			bAnswer = true;
			break;
		}
	}

	return bAnswer;
}

bool GfRace::isCompetitorFocused(const GfDriver* pComp) const
{
	return _pPrivate->strFocusedModuleName == pComp->getModuleName()
		   && _pPrivate->nFocusedItfIndex == pComp->getInterfaceIndex();
}

GfDriver* GfRace::getFocusedCompetitor() const
{
	return getCompetitor(_pPrivate->strFocusedModuleName, _pPrivate->nFocusedItfIndex);
}

void GfRace::setFocusedCompetitor(const GfDriver* pComp)
{
	_pPrivate->strFocusedModuleName = pComp ? pComp->getModuleName() : "";
	_pPrivate->nFocusedItfIndex = pComp ? pComp->getInterfaceIndex() : -1;
}

bool GfRace::appendCompetitor(GfDriver* pComp)
{
	const bool bAppended = acceptsMoreCompetitors();
	
	if (bAppended)
	{
		_pPrivate->vecCompetitors.push_back(pComp);
		const std::pair<std::string, int> compKey(pComp->getModuleName(),
												  pComp->getInterfaceIndex());
		_pPrivate->mapCompetitorsByKey[compKey] = pComp;

		// Now we are no more consistent with the race params (in memory).
		_pPrivate->bIsDirty = true;
	}

	return bAppended;
}

bool GfRace::removeCompetitor(GfDriver* pComp)
{
	bool bRemoved = true;
	
	// Remove it from the vector.
	std::vector<GfDriver*>::iterator itVComp =
		std::find(_pPrivate->vecCompetitors.begin(), _pPrivate->vecCompetitors.end(), pComp);
	if (itVComp != _pPrivate->vecCompetitors.end())
	{
		_pPrivate->vecCompetitors.erase(itVComp);

		// Now we are no more consistent with the race params (in memory).
		_pPrivate->bIsDirty = true;
	}
	else
		bRemoved = false;
	
	// Remove it from the map.
	const std::pair<std::string, int> compKey(pComp->getModuleName(), pComp->getInterfaceIndex());
	Private::TMapCompetitorsByKey::iterator itMComp =
		_pPrivate->mapCompetitorsByKey.find(compKey);
	if (itMComp != _pPrivate->mapCompetitorsByKey.end())
	{
		_pPrivate->mapCompetitorsByKey.erase(itMComp);

		// Now we are no more consistent with the race params (in memory).
		_pPrivate->bIsDirty = true;
	}
	else
		bRemoved = false;

	return bRemoved;
}

bool GfRace::moveCompetitor(GfDriver* pComp, int nDeltaPlace)
{
	// Nothing to do if no real move.
	if (nDeltaPlace == 0)
		return false;

	// Neither if competitor not found in race.
	std::vector<GfDriver*>::iterator itComp =
		std::find(_pPrivate->vecCompetitors.begin(), _pPrivate->vecCompetitors.end(), pComp);
	if (itComp == _pPrivate->vecCompetitors.end())
		return false;

	// Remove the competitor from his place.
	const int nOldIndex = itComp - _pPrivate->vecCompetitors.begin();
	_pPrivate->vecCompetitors.erase(itComp);
	
	// Determine his new place.
	const int nNewIndex = nOldIndex + nDeltaPlace;
	if (nNewIndex < 0)
		itComp = _pPrivate->vecCompetitors.begin();
	else if (nNewIndex >= (int)_pPrivate->vecCompetitors.size())
		itComp = _pPrivate->vecCompetitors.end();
	else
		itComp = _pPrivate->vecCompetitors.begin() + nNewIndex;

	// Insert it at his new place.
	_pPrivate->vecCompetitors.insert(itComp, pComp);

	// Now we are no more consistent with the race params (in memory).
	_pPrivate->bIsDirty = true;

	return true;
}


bool GfRace::removeAllCompetitors()
{
	_pPrivate->vecCompetitors.clear();

	// Now we are no more consistent with the race params (in memory).
	_pPrivate->bIsDirty = true;

	return true;
}

bool GfRace::shuffleCompetitors()
{
	// Get the number of competitors ('cause nothing to do if less than 2).
	const unsigned nCompetitors = _pPrivate->vecCompetitors.size();
	if (nCompetitors < 2)
		return false; // Didn't change anything.

	// Make a copy of the competitors vector, and clear it.
	std::vector<GfDriver*> vecCompetitors = _pPrivate->vecCompetitors;
	_pPrivate->vecCompetitors.clear();

	// Pickup a random competitor from the old vector, and add it at the end o fthe new one.
	for (unsigned nCount = 1; nCount < nCompetitors; nCount++)
	{
		// Get a random competitor index in the remaining list.
		const unsigned nPickedCompInd = rand() % vecCompetitors.size();

		// Put this competitor at the end of the new list.
		_pPrivate->vecCompetitors.push_back(vecCompetitors[nPickedCompInd]);

		// Remove it from the old list.
		vecCompetitors.erase(vecCompetitors.begin() + nPickedCompInd);
	}

	// Put the last competitor at the end of the new list.
	_pPrivate->vecCompetitors.push_back(vecCompetitors[0]);
	
	// Now we are no more consistent with the race params (in memory).
	_pPrivate->bIsDirty = true;

	return true;
}

GfTrack* GfRace::getTrack() const
{
	if (!_pPrivate->pRaceMan)
		return 0;

	return _pPrivate->pRaceMan->getEventTrack(_pPrivate->nEventInd);
}

const std::string& GfRace::getSessionName() const
{
	if (!_pPrivate->pRaceMan)
		return _pPrivate->strEmpty;

	return _pPrivate->pRaceMan->getSessionName(_pPrivate->nSessionInd);
}

void* GfRace::getResultsDescriptorHandle() const
{
	return _pPrivate->hparmResults;
}

void GfRace::print() const
{
	// TODO.
	GfLogWarning("GfTrack::print() not yet implemented\n");
}
