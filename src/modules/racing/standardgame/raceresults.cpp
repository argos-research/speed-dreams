/***************************************************************************

    file        : raceresults.cpp
    created     : Thu Jan  2 12:43:10 CET 2003
    copyright   : (C) 2002 by Eric Espie
    email       : eric.espie@torcs.org
    version     : $Id: raceresults.cpp 6270 2015-11-23 19:44:40Z madbad $                                  

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
    		Results managment for all race types
    @author	<a href=mailto:eric.espie@torcs.org>Eric Espie</a>
    @version	$Id: raceresults.cpp 6270 2015-11-23 19:44:40Z madbad $
*/

#include <ctime>
#include <vector>
#include <algorithm>
#include <string>

#include <portability.h>
#include <robot.h>
#ifdef WEBSERVER
#include <webserver.h>
#endif //WEBSERVER

#include "standardgame.h"

#include "racesituation.h"
#include "racestate.h"
#include "raceresults.h"


//TODO: is it still necessary?
//static const char *aSessionTypeNames[3] = {"Practice", "Qualifications", "Race"};

static char buf[1024];
static char path[1024];
static char path2[1024];


typedef struct
{
	std::string drvName;
	std::string shortname;
	std::string modName;
	std::string carName;
	int         extended;
	int         drvIdx;
	int         points;
} tReStandings;


void
ReInitResults(void)
{
	struct tm	*stm;
	time_t	t;
	void	*results;
	
	t = time(NULL);
	stm = localtime(&t);
	snprintf(buf, sizeof(buf), "%sresults/%s/results-%4d-%02d-%02d-%02d-%02d.xml",
		GfLocalDir(),
		ReInfo->_reFilename,
		stm->tm_year+1900,
		stm->tm_mon+1,
		stm->tm_mday,
		stm->tm_hour,
		stm->tm_min);

	ReInfo->results = GfParmReadFile(buf, GFPARM_RMODE_STD | GFPARM_RMODE_CREAT);
	ReInfo->mainResults = ReInfo->results;
	results = ReInfo->results;
	GfParmSetNum(results, RE_SECT_HEADER, RE_ATTR_DATE, NULL, (tdble)t);
	GfParmSetNum(results, RE_SECT_CURRENT, RE_ATTR_CUR_TRACK, NULL, 1);
	GfParmSetNum(results, RE_SECT_CURRENT, RE_ATTR_CUR_RACE, NULL, 1);
	GfParmSetNum(results, RE_SECT_CURRENT, RE_ATTR_CUR_DRIVER, NULL, 1);
}

void
ReEventInitResults(void)
{
	void	*results = ReInfo->results;
	void	*params = ReInfo->params;
	
	const int nCars = GfParmGetEltNb(params, RM_SECT_DRIVERS);
	for (int i = 1; i < nCars + 1; i++) 
	{
		snprintf(path, sizeof(path), "%s/%s/%d", ReInfo->track->name, RM_SECT_DRIVERS, i);
		snprintf(path2, sizeof(path2), "%s/%d", RM_SECT_DRIVERS, i);
		GfParmSetStr(results, path, RE_ATTR_DLL_NAME,
					 GfParmGetStr(params, path2, RM_ATTR_MODULE, ""));
		GfParmSetNum(results, path, RE_ATTR_INDEX, NULL,
					 GfParmGetNum(params, path2, RM_ATTR_IDX, (char*)NULL, 0));
		GfParmSetNum(results, path, RM_ATTR_EXTENDED, NULL,
					 GfParmGetNum(params, path2, RM_ATTR_EXTENDED, (char*)NULL, 0));
		//GfParmSetStr(results, path, ROB_ATTR_NAME,
		//			 GfParmGetStr(params, path2, ROB_ATTR_NAME, ""));
		//GfParmSetStr(results, path, ROB_ATTR_CAR,
		//			 GfParmGetStr(params, path2, ROB_ATTR_CAR, ""));
	}
}


//for sort()
inline bool sortByScore(const tReStandings& a, const tReStandings& b)
{
	return (a.points > b.points);
}
	
//for find()
inline bool operator ==(const tReStandings& a, const std::string b)
{
	return !a.drvName.compare(b);
}

void
ReUpdateStandings(void)
{
	tReStandings st;
	std::string drvName;
	std::vector<tReStandings> *standings;
	std::vector<tReStandings>::iterator found;
	std::vector<tReStandings>::iterator it;
	int runDrv, curDrv;
	int i;
	void *results = ReInfo->results;

	snprintf(path, sizeof(path), "%s/%s/%s/%s", ReInfo->track->name, RE_SECT_RESULTS, ReInfo->_reRaceName, RE_SECT_RANK);
	runDrv = GfParmGetEltNb(results, path);
	curDrv = GfParmGetEltNb(results, RE_SECT_STANDINGS);
	
	standings = new std::vector<tReStandings>;

	standings->reserve(curDrv);

	/* Read the current standings */
	for (i = 0; i < curDrv; i++) 
	{
		snprintf(path2, sizeof(path2), "%s/%d", RE_SECT_STANDINGS, i + 1);
		st.drvName = GfParmGetStr(results, path2, RE_ATTR_NAME, 0);
		st.shortname = GfParmGetStr(results, path2, RE_ATTR_SNAME, 0);
		st.modName = GfParmGetStr(results, path2, RE_ATTR_MODULE, 0);
		st.carName = GfParmGetStr(results, path2, RE_ATTR_CAR, 0);
		st.extended = (int)GfParmGetNum(results, path2, RM_ATTR_EXTENDED, NULL, 0);
		st.drvIdx  = (int)GfParmGetNum(results, path2, RE_ATTR_IDX, NULL, 0);
		st.points  = (int)GfParmGetNum(results, path2, RE_ATTR_POINTS, NULL, 0);
		standings->push_back(st);
	}//for i

	//Void the stored results
	GfParmListClean(results, RE_SECT_STANDINGS);
	
	//Check last races' drivers and search their name in the results.
	//If found there, adds recent points.
	//If not found, adds the driver
	for (i = 0; i < runDrv; i++) {
		//Search the driver name in the standings
		snprintf(path, sizeof(path), "%s/%s/%s/%s/%d", ReInfo->track->name, RE_SECT_RESULTS, ReInfo->_reRaceName, RE_SECT_RANK, i + 1);
		drvName = GfParmGetStr(results, path, RE_ATTR_NAME, 0);
		found = std::find(standings->begin(), standings->end(), drvName);
		
		if(found == standings->end()) {
			//No such driver in the standings, let's add it
			st.drvName = drvName;
			st.shortname = GfParmGetStr(results, path, RE_ATTR_SNAME, 0);
			st.modName = GfParmGetStr(results, path, RE_ATTR_MODULE, 0);
			st.carName = GfParmGetStr(results, path, RE_ATTR_CAR, 0);
			st.extended = (int)GfParmGetNum(results, path, RM_ATTR_EXTENDED, NULL, 0);
			st.drvIdx  = (int)GfParmGetNum(results, path, RE_ATTR_IDX, NULL, 0);
			st.points  = (int)GfParmGetNum(results, path, RE_ATTR_POINTS, NULL, 0);
			standings->push_back(st);
		} else {
			//Driver found, add recent points
			found->points += (int)GfParmGetNum(results, path, RE_ATTR_POINTS, NULL, 0);
		}//if found
	}//for i
	
	//sort standings by score
	std::sort(standings->begin(), standings->end(), sortByScore);
	
	//Store the standing back
	for(it = standings->begin(), i = 0; it != standings->end(); ++it, ++i) {
		snprintf(path, sizeof(path), "%s/%d", RE_SECT_STANDINGS, i + 1);
		GfParmSetStr(results, path, RE_ATTR_NAME, it->drvName.c_str());
		GfParmSetStr(results, path, RE_ATTR_SNAME, it->shortname.c_str());
		GfParmSetStr(results, path, RE_ATTR_MODULE, it->modName.c_str());
		GfParmSetStr(results, path, RE_ATTR_CAR, it->carName.c_str());
		GfParmSetNum(results, path, RE_ATTR_IDX, NULL, (tdble)it->drvIdx);
		GfParmSetNum(results, path, RE_ATTR_POINTS, NULL, (tdble)it->points);
	}//for it
	delete standings;
	
	char str1[512], str2[512];
	snprintf(str1, sizeof(str1), "%sconfig/params.dtd", GfDataDir());
	snprintf(str2, sizeof(str2), "<?xml-stylesheet type=\"text/xsl\" href=\"file:///%sconfig/raceresults.xsl\"?>", GfDataDir());
	
	GfParmSetDTD (results, str1, str2);
	GfParmWriteFile(0, results, "Results");
}//ReUpdateStandings

void ReCalculateClassPoints(char const *race)
{
	double points;
	char *path3;
	int rank = 1;
	int count;

	snprintf(buf, sizeof(buf), "%s/%s/%s/%s", ReInfo->track->name, RE_SECT_RESULTS, ReInfo->_reRaceName, RE_SECT_RANK);
	path3 = strdup(buf);
	if (GfParmListSeekFirst(ReInfo->results, path3) != 0)
	{
		free(path3);
		return; /* No result found */
	}
	count = GfParmGetEltNb(ReInfo->results, path3);
	do {
		snprintf( path2, sizeof(path2), "%s/%s", race, RM_SECT_CLASSPOINTS );
		if (GfParmListSeekFirst( ReInfo->params, path2 ) != 0) {
			GfLogDebug( "ReCalculateClassPoints: First not found in %s)\n", path2 );
			continue;
		}
		do {
			snprintf( buf, sizeof(buf), "%s/%s", path2, GfParmListGetCurEltName( ReInfo->params, path2 ) );
			snprintf( path, sizeof(path), "%s/%s/%d/%d/%s", RE_SECT_CLASSPOINTS,
			          GfParmGetCurStr (ReInfo->results, path3, RE_ATTR_MODULE, ""),
			          (int)GfParmGetCurNum (ReInfo->results, path3, RM_ATTR_EXTENDED, NULL, 0),
			          (int)GfParmGetCurNum (ReInfo->results, path3, RE_ATTR_IDX, NULL, 0),
			          GfParmGetStr( ReInfo->params, buf, RM_ATTR_SUFFIX, "" ) );
			points = GfParmGetNum (ReInfo->results, path, RE_ATTR_POINTS, NULL, 0);
			GfParmSetVariable (ReInfo->params, buf, "pos", (tdble)rank);
			GfParmSetVariable (ReInfo->params, buf, "cars", (tdble)count);
			//GfLogDebug( "ReCalculateClassPoints: pos = %d; count = %d\n", rank, count);
			//GfLogDebug( "ReCalculateClassPoints: GfParmGetNum (..., %s, %s, NULL, 0)\n", buf, RM_ATTR_POINTS );
			points += ( GfParmGetNum (ReInfo->params, buf, RM_ATTR_POINTS, NULL, 0) /
			            GfParmGetNum (ReInfo->params, RM_SECT_TRACKS, RM_ATTR_NUMBER, NULL, 1) );
			GfParmRemoveVariable (ReInfo->params, buf, "pos");
			GfParmRemoveVariable (ReInfo->params, buf, "cars");
			GfParmSetNum (ReInfo->results, path, RE_ATTR_POINTS, NULL, (tdble)points);
		} while (GfParmListSeekNext( ReInfo->params, path2 ) == 0);
		++rank;
	} while (GfParmListSeekNext (ReInfo->results, path3) == 0);
	free(path3);
}

void
ReStoreRaceResults(const char *race)
{
	int		i;
	int		nCars;
	tCarElt	*car;
	tSituation 	*s = ReInfo->s;
	char	*carName;
	void	*carparam;
	void	*results = ReInfo->results;
	void	*params = ReInfo->params;
	
	/* Store the number of laps of the race */
	switch (ReInfo->s->_raceType) {
		case RM_TYPE_RACE:
			car = s->cars[0];
			if (car->_laps > s->_totLaps) car->_laps = s->_totLaps + 1;

			snprintf(path, sizeof(path), "%s/%s/%s", ReInfo->track->name, RE_SECT_RESULTS, race);
			GfParmListClean(results, path);
			GfParmSetNum(results, path, RE_ATTR_LAPS, NULL, (tdble)(car->_laps - 1));
			
			for (i = 0; i < s->_ncars; i++) {
				snprintf(path, sizeof(path), "%s/%s/%s/%s/%d", ReInfo->track->name, RE_SECT_RESULTS, race, RE_SECT_RANK, i + 1);
				car = s->cars[i];
				if (car->_laps > s->_totLaps)
					car->_laps = s->_totLaps + 1;
			
				GfParmSetStr(results, path, RE_ATTR_NAME, car->_name);
				GfParmSetStr(results, path, RE_ATTR_SNAME, car->_sname);

				snprintf(buf, sizeof(buf), "cars/models/%s/%s.xml", car->_carName, car->_carName);
				carparam = GfParmReadFile(buf, GFPARM_RMODE_STD);
				carName = GfParmGetName(carparam);
			
				GfParmSetStr(results, path, RE_ATTR_CAR, carName);
				GfParmSetNum(results, path, RE_ATTR_INDEX, NULL, (tdble)car->index);
			
				GfParmSetNum(results, path, RE_ATTR_LAPS, NULL, (tdble)(car->_laps - 1));
				GfParmSetNum(results, path, RE_ATTR_TIME, NULL, (tdble)car->_curTime);
				GfParmSetNum(results, path, RE_ATTR_BEST_LAP_TIME, NULL, (tdble)car->_bestLapTime);
				GfParmSetNum(results, path, RE_ATTR_TOP_SPEED, NULL, car->_topSpeed);
				GfParmSetNum(results, path, RE_ATTR_DAMMAGES, NULL, (tdble)car->_dammage);
				GfParmSetNum(results, path, RE_ATTR_NB_PIT_STOPS, NULL, (tdble)car->_nbPitStops);
			
				GfParmSetStr(results, path, RE_ATTR_MODULE, car->_modName);
				GfParmSetNum(results, path, RE_ATTR_IDX, NULL, (tdble)car->_moduleIndex);
				snprintf(path2, sizeof(path2), "%s/%d", RM_SECT_DRIVERS_RACING, car->index + 1 );
				GfParmSetNum(results, path, RM_ATTR_EXTENDED, NULL,
							 GfParmGetNum(params, path2, RM_ATTR_EXTENDED, NULL, 0));
				GfParmSetStr(results, path, ROB_ATTR_CAR, car->_carName);
				snprintf(path2, sizeof(path2), "%s/%s/%d", race, RM_SECT_POINTS, i + 1);
				GfParmSetNum(results, path, RE_ATTR_POINTS, NULL,
							 GfParmGetNum(params, path2, RE_ATTR_POINTS, NULL, 0));
				if (strlen(car->_skinName) > 0)
					GfParmSetStr(results, path, RM_ATTR_SKINNAME, car->_skinName);
				GfParmSetNum(results, path, RM_ATTR_SKINTARGETS, NULL, (tdble)car->_skinTargets);

				GfParmReleaseHandle(carparam);
			}
			break;
			
		case RM_TYPE_PRACTICE:
			if (s->_ncars == 1)
			{
				car = s->cars[0];
				snprintf(path, sizeof(path), "%s/%s/%s", ReInfo->track->name, RE_SECT_RESULTS, race);
				GfParmSetStr(results, path, RM_ATTR_DRVNAME, car->_name);
				snprintf(buf, sizeof(buf), "cars/models/%s/%s.xml", car->_carName, car->_carName);
				carparam = GfParmReadFile(buf, GFPARM_RMODE_STD);
				carName = GfParmGetName(carparam);
				GfParmSetStr(results, path, RE_ATTR_CAR, carName);
				GfParmReleaseHandle(carparam);
				break;
			}
			/* Otherwise, fall through */
			
		case RM_TYPE_QUALIF:
			if (s->_ncars == 1)
			{
				car = s->cars[0];
				snprintf(path, sizeof(path), "%s/%s/%s/%s", ReInfo->track->name, RE_SECT_RESULTS, race, RE_SECT_RANK);
				nCars = GfParmGetEltNb(results, path);
				for (i = nCars; i > 0; i--) {
					snprintf(path, sizeof(path), "%s/%s/%s/%s/%d", ReInfo->track->name, RE_SECT_RESULTS, race, RE_SECT_RANK, i);
					float opponentBestLapTime = GfParmGetNum(results, path, RE_ATTR_BEST_LAP_TIME, NULL, 0);
				
					if (car->_bestLapTime != 0.0 
						&& (car->_bestLapTime < opponentBestLapTime || opponentBestLapTime == 0.0))
					{
						/* shift */
						snprintf(path2, sizeof(path2), "%s/%s/%s/%s/%d",
								ReInfo->track->name, RE_SECT_RESULTS, race, RE_SECT_RANK, i + 1);
						GfParmSetStr(results, path2, RE_ATTR_NAME,
									 GfParmGetStr(results, path, RE_ATTR_NAME, ""));
						GfParmSetStr(results, path2, RE_ATTR_SNAME,
									 GfParmGetStr(results, path, RE_ATTR_SNAME, ""));
						GfParmSetStr(results, path2, RE_ATTR_CAR,
									 GfParmGetStr(results, path, RE_ATTR_CAR, ""));
						GfParmSetNum(results, path2, RE_ATTR_BEST_LAP_TIME, NULL,
									 GfParmGetNum(results, path, RE_ATTR_BEST_LAP_TIME, NULL, 0));
						GfParmSetStr(results, path2, RE_ATTR_MODULE,
									 GfParmGetStr(results, path, RM_ATTR_MODULE, ""));
						GfParmSetNum(results, path2, RE_ATTR_IDX, NULL,
									 GfParmGetNum(results, path, RM_ATTR_IDX, NULL, 0));
						GfParmSetNum(results, path2, RM_ATTR_EXTENDED, NULL,
									 GfParmGetNum(results, path, RM_ATTR_EXTENDED, NULL, 0));
						GfParmSetStr(results, path2, ROB_ATTR_CAR,
									 GfParmGetStr(results, path, ROB_ATTR_CAR, ""));
						GfParmSetStr(results, path2, ROB_ATTR_NAME,
									 GfParmGetStr(results, path, ROB_ATTR_NAME, ""));
						snprintf(path, sizeof(path), "%s/%s/%d", race, RM_SECT_POINTS, i + 1);
						GfParmSetNum(results, path2, RE_ATTR_POINTS, NULL,
									 GfParmGetNum(params, path, RE_ATTR_POINTS, NULL, 0));
						if (GfParmGetStr(results, path, RM_ATTR_SKINNAME, 0))
							GfParmSetStr(results, path2, RM_ATTR_SKINNAME,
										 GfParmGetStr(results, path, RM_ATTR_SKINNAME, 0));
						GfParmSetNum(results, path2, RM_ATTR_SKINTARGETS, NULL,
									 GfParmGetNum(results, path, RM_ATTR_SKINTARGETS, NULL, 0));
					} else {
						break;
					}
				}
				/* insert after */
				snprintf(path, sizeof(path), "%s/%s/%s/%s/%d", ReInfo->track->name, RE_SECT_RESULTS, race, RE_SECT_RANK, i + 1);
				GfParmSetStr(results, path, RE_ATTR_NAME, car->_name);
				GfParmSetStr(results, path, RE_ATTR_SNAME, car->_sname);

				snprintf(buf, sizeof(buf), "cars/models/%s/%s.xml", car->_carName, car->_carName);
				carparam = GfParmReadFile(buf, GFPARM_RMODE_STD);
				carName = GfParmGetName(carparam);

				GfParmSetStr(results, path, RE_ATTR_CAR, carName);
				GfParmSetNum(results, path, RE_ATTR_BEST_LAP_TIME, NULL, (tdble)car->_bestLapTime);
				GfParmSetStr(results, path, RE_ATTR_MODULE, car->_modName);
				GfParmSetNum(results, path, RE_ATTR_IDX, NULL, (tdble)car->_moduleIndex);
				GfParmSetStr(results, path, ROB_ATTR_CAR, car->_carName);
				GfParmSetStr(results, path, ROB_ATTR_NAME, car->_name);
				snprintf(path2, sizeof(path2), "%s/%d", RM_SECT_DRIVERS_RACING, car->index + 1 );
				GfParmSetNum(results, path, RM_ATTR_EXTENDED, NULL,
							 GfParmGetNum(params, path2, RM_ATTR_EXTENDED, NULL, 0));
				snprintf(path2, sizeof(path2), "%s/%s/%d", race, RM_SECT_POINTS, i + 1);
				GfParmSetNum(results, path, RE_ATTR_POINTS, NULL,
							 GfParmGetNum(params, path2, RE_ATTR_POINTS, NULL, 0));
				if (strlen(car->_skinName) > 0)
					GfParmSetStr(results, path, RM_ATTR_SKINNAME, car->_skinName);
				GfParmSetNum(results, path, RM_ATTR_SKINTARGETS, NULL, (tdble)car->_skinTargets);
			
				GfParmReleaseHandle(carparam);
				break;
			} else {
				car = s->cars[0];
	
				if (s->_totTime < 0.0f)
					GfLogWarning("Saving results of multicar non-race session, but it was not timed!\n" );
				snprintf(path, sizeof(path), "%s/%s/%s", ReInfo->track->name, RE_SECT_RESULTS, race);
				GfParmListClean(results, path);
				GfParmSetNum(results, path, RE_ATTR_SESSIONTIME, NULL, (tdble)s->_totTime);
				
				for (i = 0; i < s->_ncars; i++) {
					snprintf(path, sizeof(path), "%s/%s/%s/%s/%d", ReInfo->track->name, RE_SECT_RESULTS, race, RE_SECT_RANK, i + 1);
					car = s->cars[i];
				
					GfParmSetStr(results, path, RE_ATTR_NAME, car->_name);
				
					GfParmSetStr(results, path, RE_ATTR_SNAME, car->_sname);
					snprintf(buf, sizeof(buf), "cars/models/%s/%s.xml", car->_carName, car->_carName);
					carparam = GfParmReadFile(buf, GFPARM_RMODE_STD);
					carName = GfParmGetName(carparam);
				
					GfParmSetStr(results, path, RE_ATTR_CAR, carName);
					GfParmSetNum(results, path, RE_ATTR_INDEX, NULL, (tdble)car->index);
				
					GfParmSetNum(results, path, RE_ATTR_LAPS, NULL, (tdble)(car->_laps - 1));
					GfParmSetNum(results, path, RE_ATTR_TIME, NULL, (tdble)car->_curTime);
					GfParmSetNum(results, path, RE_ATTR_BEST_LAP_TIME, NULL, (tdble)car->_bestLapTime);
					GfParmSetNum(results, path, RE_ATTR_TOP_SPEED, NULL, car->_topSpeed);
					GfParmSetNum(results, path, RE_ATTR_DAMMAGES, NULL, (tdble)car->_dammage);
					GfParmSetNum(results, path, RE_ATTR_NB_PIT_STOPS, NULL, (tdble)car->_nbPitStops);
				
					GfParmSetStr(results, path, RE_ATTR_MODULE, car->_modName);
					GfParmSetNum(results, path, RE_ATTR_IDX, NULL, (tdble)car->_moduleIndex);
					snprintf(path2, sizeof(path2), "%s/%d", RM_SECT_DRIVERS_RACING, car->index + 1 );
					GfParmSetNum(results, path, RM_ATTR_EXTENDED, NULL,
								 GfParmGetNum(params, path2, RM_ATTR_EXTENDED, NULL, 0));
					GfParmSetStr(results, path, ROB_ATTR_CAR, car->_carName);
					snprintf(path2, sizeof(path2), "%s/%s/%d", race, RM_SECT_POINTS, i + 1);
					GfParmSetNum(results, path, RE_ATTR_POINTS, NULL,
								 GfParmGetNum(params, path2, RE_ATTR_POINTS, NULL, 0));
					if (strlen(car->_skinName) > 0)
						GfParmSetStr(results, path, RM_ATTR_SKINNAME, car->_skinName);
					GfParmSetNum(results, path, RM_ATTR_SKINTARGETS, NULL, (tdble)car->_skinTargets);
			
					GfParmReleaseHandle(carparam);
				}
				break;
			}
	}
}

void
ReInitCurRes()
{
	if (ReInfo->_displayMode != RM_DISP_MODE_NORMAL)
	{
		if (ReInfo->s->_raceType == RM_TYPE_QUALIF)
		{
			ReUpdateQualifCurRes(ReInfo->s->cars[0]);
		}
		else if (ReInfo->s->_raceType == RM_TYPE_PRACTICE && ReInfo->s->_ncars > 1)
		{
			ReUpdatePracticeCurRes(ReInfo->s->cars[0]);
		}
		else
		{
			static const char* pszTableHeader = "Rank    Time     Driver               Car";
			char pszTitle[128];
			snprintf(pszTitle, sizeof(pszTitle), "%s at %s", 
					 ReInfo->_reRaceName, ReInfo->track->name);
			char pszSubTitle[128];
			snprintf(pszSubTitle, sizeof(pszSubTitle), "%s (%s)",
					 ReInfo->s->cars[0]->_name, ReInfo->s->cars[0]->_carName);
			ReUI().setResultsTableTitles(pszTitle, pszSubTitle);
			ReUI().setResultsTableHeader(pszTableHeader);
		}
	}//if displayMode != normal
}

void
ReUpdatePracticeCurRes(tCarElt *car, bool bForceNew)
{
	if (bForceNew)
	{
		static const char* pszTableHeader =
			"Lap     \tTime          \tBest      \tTop spd  \tMin spd  \tDamages";
		ReUI().setResultsTableHeader(pszTableHeader);
		char* t1 = GfTime2Str(car->_lastLapTime, 0, false, 3);
		char* t2 = GfTime2Str(car->_bestLapTime, 0, false, 3);
		char buf[128];

		// Cancel hightlight on first line
		if (car->_laps == 2) ReUI().setResultsTableRow(0, "");

		tReCarInfo *info = &(ReInfo->_reCarInfo[car->index]);
		static int nLastLapDamages = 0;
		if (car->_laps <= 2)
			nLastLapDamages = 0;
		snprintf(buf, sizeof(buf), "%.3d  \t%-12s \t%-12s    \t%5.1f   \t%5.1f \t %.5d (%d)",
				 car->_laps - 1, t1, t2, info->topSpd * 3.6, info->botSpd * 3.6,
				 car->_dammage ? car->_dammage - nLastLapDamages : 0, car->_dammage);
		nLastLapDamages = car->_dammage;
		free(t1);
		free(t2);
		
		ReUI().addResultsTableRow(buf);
	}
	else
	{
		ReUpdateQualifCurRes(car);
	}
}

void
ReUpdateQualifCurRes(tCarElt *car)
{
	static const char* pszTableHeader = "Rank    \tTime          \tDriver                     \tCar";
	int		i;
	int		xx;
	int		nCars;
	int		nCarsReal;
	int		printed;
	int		maxLines;
	void	*carparam;
	char	*carName;
	const char	*race = ReInfo->_reRaceName;
	void	*results = ReInfo->results;
	char	*tmp_str;
	double		time_left;
	
	if (ReInfo->s->_ncars == 1)
	{
		ReUI().eraseResultsTable();
		maxLines = ReUI().getResultsTableRowCount();
		
		snprintf(buf, sizeof(buf), "cars/models/%s/%s.xml", car->_carName, car->_carName);
		carparam = GfParmReadFile(buf, GFPARM_RMODE_STD);
		carName = GfParmGetName(carparam);

		char pszTitle[128];
		snprintf(pszTitle, sizeof(pszTitle), "%s at %s", 
				 race, ReInfo->track->name);
		if (ReInfo->s->_raceType == RM_TYPE_PRACTICE || car->_laps < 1 || car->_laps > ReInfo->s->_totLaps)
			snprintf(buf, sizeof(buf), "%s (%s)", car->_name, carName);
		else
			snprintf(buf, sizeof(buf), "%s (%s) - Lap %d", car->_name, carName, car->_laps);
		ReUI().setResultsTableTitles(pszTitle, buf);
		ReUI().setResultsTableHeader(pszTableHeader);

		printed = 0;
		snprintf(path, sizeof(path), "%s/%s/%s/%s", ReInfo->track->name, RE_SECT_RESULTS, race, RE_SECT_RANK);
		nCarsReal = GfParmGetEltNb(results, path);
		nCars = MIN(nCarsReal + 1, maxLines); // limit display to only those on 1st page
		for (i = 1; i < nCars; i++) {
			snprintf(path, sizeof(path), "%s/%s/%s/%s/%d", ReInfo->track->name, RE_SECT_RESULTS, race, RE_SECT_RANK, i);
			if (!printed && car->_bestLapTime != 0.0
				&& car->_bestLapTime < GfParmGetNum(results, path, RE_ATTR_BEST_LAP_TIME, NULL, 0)) {
				tmp_str = GfTime2Str(car->_bestLapTime, "  ", false, 3);
				snprintf(buf, sizeof(buf), " %2d \t%-12s  \t%-25s \t%-20s", i, tmp_str, car->_name, carName);
				free(tmp_str);
				ReUI().setResultsTableRow(i - 1, buf, /*highlight=*/true);
				printed = 1;
			}
			tmp_str = GfTime2Str(GfParmGetNum(results, path, RE_ATTR_BEST_LAP_TIME, NULL, 0), "  ", false, 3);
			snprintf(buf, sizeof(buf), " %2d \t%-12s  \t%-25s \t%-20s",
					 i + printed, tmp_str, GfParmGetStr(results, path, RE_ATTR_NAME, ""),
					 GfParmGetStr(results, path, RE_ATTR_CAR, ""));
			free (tmp_str);
			ReUI().setResultsTableRow(i - 1 + printed, buf);
		}
	
		if (!printed) {
			tmp_str = GfTime2Str(car->_bestLapTime, "  ", false, 3);
			snprintf(buf, sizeof(buf), " %2d \t%-12s  \t%-25s \t%-20s", nCarsReal + 1, tmp_str, car->_name, carName);
			free(tmp_str);
			ReUI().setResultsTableRow(i - 1, buf, /*highlight=*/true);
		}
	
		GfParmReleaseHandle(carparam);
	}
	else
	{
		nCars = ReInfo->s->_ncars;
		if (nCars > ReUI().getResultsTableRowCount())
			nCars = ReUI().getResultsTableRowCount();

		char pszTitle[128];
		snprintf(pszTitle, sizeof(pszTitle), "%s at %s", 
				 race, ReInfo->track->name);
		if (ReInfo->s->_totTime > ReInfo->s->currentTime)
		{
			time_left = ReInfo->s->_totTime - ReInfo->s->currentTime;
			snprintf( buf, sizeof(buf), "%d:%02d:%02d",
					  (int)floor( time_left / 3600.0f ), (int)floor( time_left / 60.0f ) % 60,
					  (int)floor( time_left ) % 60 );
		}
		else
		{
			snprintf( buf, sizeof(buf), "%d laps", ReInfo->s->_totLaps );
		}
		ReUI().setResultsTableTitles(pszTitle, buf);
		ReUI().setResultsTableHeader(pszTableHeader);
		
		for (xx = 0; xx < nCars; ++xx) {
			car = ReInfo->s->cars[ xx ];
			snprintf(buf, sizeof(buf), "cars/models/%s/%s.xml", car->_carName, car->_carName);
			carparam = GfParmReadFile(buf, GFPARM_RMODE_STD);
			carName = strdup(GfParmGetName(carparam));
			GfParmReleaseHandle(carparam);
			
			if (car->_state & RM_CAR_STATE_DNF) {
				snprintf(buf, sizeof(buf), "out \t               \t%-25s \t%-20s", car->_name, carName);
			} else if (car->_bestLapTime <= 0.0f) {
				snprintf(buf, sizeof(buf), " %2d \t      --:---   \t%-25s \t%-20s",
						 xx + 1, car->_name, carName);
			} else {
				if (xx == 0)
					tmp_str = GfTime2Str(car->_bestLapTime, " ", false, 3);
				else
					tmp_str = GfTime2Str(car->_bestLapTime - ReInfo->s->cars[0]->_bestLapTime,
										 "+", false, 3);
				snprintf(buf, sizeof(buf), " %2d \t%-12s  \t%-25s \t%-20s",
						 xx + 1, tmp_str, car->_name, carName);
				free(tmp_str);
			}
			ReUI().setResultsTableRow(xx, buf);
			FREEZ(carName);
		}
	}
}

void
ReUpdateRaceCurRes()
{
	static const char* pszTableHeader = "Rank    \tTime          \tDriver                   \tCar";
    int ncars;
    int xx;
    void *carparam;
    char *carName;
    tCarElt *car;
    char *tmp_str;
    double time_left;

    ncars = ReInfo->s->_ncars;
    if (ncars > ReUI().getResultsTableRowCount())
    	ncars = ReUI().getResultsTableRowCount();

	char pszTitle[128];
	snprintf(pszTitle, sizeof(pszTitle), "%s at %s",
			 ReInfo->_reRaceName, ReInfo->track->name);

    if (ReInfo->s->_totTime > ReInfo->s->currentTime)
    {
    	time_left = ReInfo->s->_totTime - ReInfo->s->currentTime;
    	snprintf( buf, sizeof(buf), "%d:%02d:%02d",
				  (int)floor( time_left / 3600.0f ),
				  (int)floor( time_left / 60.0f ) % 60, (int)floor( time_left ) % 60 );
    }
    else
    {
    	snprintf( buf, sizeof(buf), "%d laps", ReInfo->s->_totLaps );
    }
	ReUI().setResultsTableTitles(pszTitle, buf);
	ReUI().setResultsTableHeader(pszTableHeader);

    for (xx = 0; xx < ncars; ++xx) {
    	car = ReInfo->s->cars[ xx ];
        snprintf(buf, sizeof(buf), "cars/models/%s/%s.xml", car->_carName, car->_carName);
        carparam = GfParmReadFile(buf, GFPARM_RMODE_STD);
        carName = strdup(GfParmGetName(carparam));
        GfParmReleaseHandle(carparam);
	
		if (car->_state & RM_CAR_STATE_DNF) {
			snprintf(buf, sizeof(buf), "out               %-20s %-20s", car->_name, carName);
		} else if (car->_timeBehindLeader == 0.0f) {
			if (xx != 0)
				snprintf(buf, sizeof(buf), " %2d     \t   --:--- \t%-25s \t%-20s",
						 xx + 1, car->_name, carName);
			else
				snprintf(buf, sizeof(buf), " %2d     \t%3d laps  \t%-25s \t%-20s",
						 xx + 1, car->_laps - 1, car->_name, carName);
		} else {
			if (xx == 0) {
				snprintf(buf, sizeof(buf), " %2d     \t%3d laps  \t%-25s \t%-20s",
						 xx + 1, car->_laps - 1, car->_name, carName);
			} else {
				if (car->_lapsBehindLeader == 0)
				{
					tmp_str = GfTime2Str(car->_timeBehindLeader, "  ", false, 3);
					snprintf(buf, sizeof(buf), " %2d \t%-12s\t%-25s \t%-20s",
							 xx + 1, tmp_str, car->_name, carName);
					free(tmp_str);
				}
				else if (car->_lapsBehindLeader == 1)
					snprintf(buf, sizeof(buf), " %2d \t       1 lap  \t%-25s \t%-20s",
							 xx + 1, car->_name, carName);
				else
					snprintf(buf, sizeof(buf), " %2d \t    %3d laps  \t%-25s \t%-20s",
							 xx + 1, car->_lapsBehindLeader, car->_name, carName);
			}
		}
		ReUI().setResultsTableRow(xx, buf);
		FREEZ(carName);
    }
}

void
ReSavePracticeLap(tCarElt *car)
{
    void	*results = ReInfo->results;
    tReCarInfo	*info = &(ReInfo->_reCarInfo[car->index]);

    if (car->_laps == 1) {
        /* hack to allow results from practice hillclimb to be recorded (as lap1) */
        snprintf(path, sizeof(path), "%s/%s/%s/%d", ReInfo->track->name, RE_SECT_RESULTS, ReInfo->_reRaceName, 1);
        GfParmSetNum(results, path, RE_ATTR_TIME, NULL, (tdble)car->_curTime);
    } else {	
        snprintf(path, sizeof(path), "%s/%s/%s/%d", ReInfo->track->name, RE_SECT_RESULTS, ReInfo->_reRaceName, car->_laps - 1);
        GfParmSetNum(results, path, RE_ATTR_TIME, NULL, (tdble)car->_lastLapTime);
    }
    GfParmSetNum(results, path, RE_ATTR_BEST_LAP_TIME, NULL, (tdble)car->_bestLapTime);
    GfParmSetNum(results, path, RE_ATTR_TOP_SPEED, NULL, info->topSpd);
    GfParmSetNum(results, path, RE_ATTR_BOT_SPEED, NULL, info->botSpd);
    GfParmSetNum(results, path, RE_ATTR_DAMMAGES, NULL, (tdble)car->_dammage);
}
