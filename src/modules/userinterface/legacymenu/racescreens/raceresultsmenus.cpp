/***************************************************************************

    file                 : raceresultsmenus.cpp
    created              : Fri Apr 14 22:36:36 CEST 2000
    copyright            : (C) 2000 by Eric Espie
    email                : torcs@free.fr
    version              : $Id: raceresultsmenus.cpp 5803 2014-07-30 03:19:34Z mungewell $

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
    		This is a set of tools useful for race managers to display results.
    @ingroup	racemantools
    @author	<a href=mailto:torcs@free.fr>Eric Espie</a>
    @version	$Id: raceresultsmenus.cpp 5803 2014-07-30 03:19:34Z mungewell $
*/

#include <portability.h>
#include <tgfclient.h>

#include <drivers.h>
#include <tracks.h>
#include <race.h>
#include <racemanagers.h>

#include "legacymenu.h"
#include "racescreens.h"


static int	rmSaveButtonId;
static int	rmReplayButtonId;
static void	*rmScrHdle = NULL;

static void rmPracticeResults(void *prevHdle, tRmInfo *info, int start);
static void rmRaceResults(void *prevHdle, tRmInfo *info, int start);
static void rmQualifResults(void *prevHdle, tRmInfo *info, const char*pszTitle, int start);

static const int DefaultSimuVersion = 1;
static const char *SimuVersionList[] =
	{RM_VAL_MOD_SIMU_V2, RM_VAL_MOD_SIMU_V2_1, RM_VAL_MOD_SIMU_V3, RM_VAL_MOD_SIMU_V4, RM_VAL_MOD_SIMU_REPLAY};
static const int NbSimuVersions = sizeof(SimuVersionList) / sizeof(SimuVersionList[0]);
static int CurSimuVersion = DefaultSimuVersion;

typedef struct
{
    void	*prevHdle;
    tRmInfo	*info;
    int		start;
	const char	*title;
} tRaceCall;

tRaceCall	RmNextRace;
tRaceCall	RmPrevRace;


static void 
rmReplayRace(void * /* dummy */) 
{ 
    const char *simuVersionName;
    int i;
    char buf[1024];
    snprintf(buf, sizeof(buf), "%s%s", GfLocalDir(), RACE_ENG_CFG);

    void *paramHandle = GfParmReadFile(buf, GFPARM_RMODE_REREAD | GFPARM_RMODE_CREAT);

    // Temporarily overwrite Simulation Type
    CurSimuVersion = DefaultSimuVersion;
    simuVersionName = GfParmGetStr(paramHandle, RM_SECT_MODULES, RM_ATTR_MOD_SIMU, SimuVersionList[DefaultSimuVersion]);
    for (i = 0; i < NbSimuVersions; i++) {
        if (strcmp(simuVersionName, SimuVersionList[i]) == 0) {
            CurSimuVersion = i;
            break;
        }
    }

    GfParmSetStr(paramHandle, RM_SECT_MODULES, RM_ATTR_MOD_SIMU, SimuVersionList[4]);
    GfParmWriteFile(NULL, paramHandle, "raceengine");

    LmRaceEngine().startNewRace(); 

    // Restore original Simulation type
    GfParmSetStr(paramHandle, RM_SECT_MODULES, RM_ATTR_MOD_SIMU, SimuVersionList[CurSimuVersion]);
    GfParmWriteFile(NULL, paramHandle, "raceengine");
    GfParmReleaseHandle(paramHandle);
} 

static void
rmSaveRes(void *vInfo)
{
    tRmInfo *info = (tRmInfo *)vInfo;

    GfParmWriteFile(0, info->results, "Results");

    GfuiVisibilitySet(rmScrHdle, rmSaveButtonId, GFUI_INVISIBLE);
}

static void
rmChgPracticeScreen(void *vprc)
{
    void		*prevScr = rmScrHdle;
    tRaceCall 	*prc = (tRaceCall*)vprc;

    rmPracticeResults(prc->prevHdle, prc->info, prc->start);
    GfuiScreenRelease(prevScr);
}

static void
rmPracticeResults(void *prevHdle, tRmInfo *info, int start)
{
	// Used across rmPracticeResults calls when multiple pages.
	static int NLastLapDamages = 0;

    void		*results = info->results;
    const char		*race = info->_reRaceName;
    int			i;
    int			y;
    static char		buf[256];
    static char		path[1024];
    char		*str;
    int 		damages; 
    void		*paramHandle;
    const char		*replayRateSchemeName;

    // Create screen, load menu XML descriptor and create static controls.
    rmScrHdle = GfuiScreenCreate();

	GfLogTrace("Entering Practice Results menu\n");

    void *hmenu = GfuiMenuLoad("practiceresultsmenu.xml");
    GfuiMenuCreateStaticControls(rmScrHdle, hmenu);

    // Create variable title labels.
    snprintf(buf, sizeof(buf), "%s at %s", race, info->track->name);
    const int titleId = GfuiMenuCreateLabelControl(rmScrHdle, hmenu, "Title");
    GfuiLabelSetText(rmScrHdle, titleId, buf);
 
    snprintf(path, sizeof(path), "%s/%s/%s", info->track->name, RE_SECT_RESULTS, race);
    snprintf(buf, sizeof(buf), "%s (%s)", GfParmGetStr(results, path, RM_ATTR_DRVNAME, NULL),
			 GfParmGetStr(results, path, RM_ATTR_CAR, NULL));

    const int subTitleId = GfuiMenuCreateLabelControl(rmScrHdle, hmenu, "SubTitle");
    GfuiLabelSetText(rmScrHdle, subTitleId, buf);
 
	// Get layout properties.
    const int nMaxLines = (int)GfuiMenuGetNumProperty(hmenu, "nMaxResultLines", 15);
    const int yTopLine = (int)GfuiMenuGetNumProperty(hmenu, "yTopLine", 400);
    const int yLineShift = (int)GfuiMenuGetNumProperty(hmenu, "yLineShift", 20);

	
	// Display the result table.
    y = yTopLine;
    
    snprintf(path, sizeof(path), "%s/%s/%s", info->track->name, RE_SECT_RESULTS, race);
    const int totLaps = (int)GfParmGetEltNb(results, path);

	// Reset last damage value if top of the table.
	if (start == 0)
		NLastLapDamages = 0; 
	else {
		snprintf(path, sizeof(path), "%s/%s/%s/%d", info->track->name, RE_SECT_RESULTS, race, start - 1);
		NLastLapDamages =  (int)(GfParmGetNum(results, path, RE_ATTR_DAMMAGES, NULL, 0)); 
	}

    for (i = 0 + start; i < MIN(start + nMaxLines, totLaps); i++) {
		snprintf(path, sizeof(path), "%s/%s/%s/%d", info->track->name, RE_SECT_RESULTS, race, i + 1);

		/* Lap */
		snprintf(buf, sizeof(buf), "%d", i+1);
		GfuiMenuCreateLabelControl(rmScrHdle, hmenu, "LapNumber", true, // From template.
								   buf, GFUI_TPL_X, y);

		/* Time */
		str = GfTime2Str(GfParmGetNum(results, path, RE_ATTR_TIME, NULL, 0), "  ", false, 3);
		GfuiMenuCreateLabelControl(rmScrHdle, hmenu, "LapTime", true, // From template.
								   str, GFUI_TPL_X, y);
		free(str);

		/* Best Lap Time */
		str = GfTime2Str(GfParmGetNum(results, path, RE_ATTR_BEST_LAP_TIME, NULL, 0), "  ", false, 3);
		GfuiMenuCreateLabelControl(rmScrHdle, hmenu, "BestTime", true, // From template.
								   str, GFUI_TPL_X, y);
		free(str);

		/* Top Spd */
		snprintf(buf, sizeof(buf), "%3.1f", (GfParmGetNum(results, path, RE_ATTR_TOP_SPEED, NULL, 0) * 3.6));
		GfuiMenuCreateLabelControl(rmScrHdle, hmenu, "TopSpeed", true, // From template.
								   buf, GFUI_TPL_X, y);

		/* Min Spd */
		snprintf(buf, sizeof(buf), "%3.1f", (GfParmGetNum(results, path, RE_ATTR_BOT_SPEED, NULL, 0) * 3.6));
		GfuiMenuCreateLabelControl(rmScrHdle, hmenu, "MinSpeed", true, // From template.
								   buf, GFUI_TPL_X, y);

		/* Damages in current lap + (total so far) */
		damages =  (int)(GfParmGetNum(results, path, RE_ATTR_DAMMAGES, NULL, 0)); 
		snprintf(buf, sizeof(buf), "%d (%d)", damages ? damages - NLastLapDamages : 0, damages); 
		GfuiMenuCreateLabelControl(rmScrHdle, hmenu, "Damages", true, // From template.
								   buf, GFUI_TPL_X, y);
		NLastLapDamages = damages; 

		y -= yLineShift;
    }

    if (start > 0) {
		RmPrevRace.prevHdle = prevHdle;
		RmPrevRace.info     = info;
		RmPrevRace.start    = start - nMaxLines;
		GfuiMenuCreateButtonControl(rmScrHdle, hmenu, "PreviousPageArrow",
									(void*)&RmPrevRace, rmChgPracticeScreen);
		GfuiAddKey(rmScrHdle, GFUIK_PAGEUP,   "Previous Results", (void*)&RmPrevRace, rmChgPracticeScreen, NULL);
    }
    
    // Add "Continue" button
    GfuiMenuCreateButtonControl(rmScrHdle, hmenu, "ContinueButton", prevHdle, GfuiScreenReplace);
    
    // Add "Replay" button (if available)
    snprintf(buf, sizeof(buf), "%s%s", GfLocalDir(), RACE_ENG_CFG);
    paramHandle = GfParmReadFile(buf, GFPARM_RMODE_REREAD | GFPARM_RMODE_CREAT);
    replayRateSchemeName = GfParmGetStr(paramHandle, RM_SECT_RACE_ENGINE, RM_ATTR_REPLAY_RATE, RM_VAL_REPLAY_OFF);

    rmReplayButtonId = GfuiMenuCreateButtonControl(rmScrHdle, hmenu, "ReplayButton", prevHdle, rmReplayRace);
    if (strcmp(replayRateSchemeName, RM_VAL_REPLAY_OFF) == 0)
        GfuiEnable(rmScrHdle, rmReplayButtonId, GFUI_DISABLE);
    GfParmReleaseHandle(paramHandle);

    //Create 'save' button in the bottom right
    //rmSaveButtonId = GfuiMenuCreateButtonControl(rmScrHdle, hmenu, "SaveButton", info, rmSaveRes);
    
    if (i < totLaps) {
		RmNextRace.prevHdle = prevHdle;
		RmNextRace.info     = info;
		RmNextRace.start    = start + nMaxLines;
		GfuiMenuCreateButtonControl(rmScrHdle, hmenu, "NextPageArrow",
									(void*)&RmNextRace, rmChgPracticeScreen);
		GfuiAddKey(rmScrHdle, GFUIK_PAGEDOWN, "Next Results", (void*)&RmNextRace, rmChgPracticeScreen, NULL);
    }

    GfuiAddKey(rmScrHdle, GFUIK_ESCAPE, "Continue", prevHdle, GfuiScreenReplace, NULL);
    GfuiAddKey(rmScrHdle, GFUIK_RETURN, "Continue", prevHdle, GfuiScreenReplace, NULL);
    GfuiAddKey(rmScrHdle, GFUIK_F12, "Take a Screen Shot", NULL, GfuiScreenShot, NULL);
    GfuiAddKey(rmScrHdle, GFUIK_F1, "Help", rmScrHdle, GfuiHelpScreen, NULL);

    GfuiScreenActivate(rmScrHdle);
}


static void
rmChgRaceScreen(void *vprc)
{
    void		*prevScr = rmScrHdle;
    tRaceCall 	*prc = (tRaceCall*)vprc;

    rmRaceResults(prc->prevHdle, prc->info, prc->start);
    GfuiScreenRelease(prevScr);
}

static void
rmRaceResults(void *prevHdle, tRmInfo *info, int start)
{
    void		*results = info->results;
    const char		*race = info->_reRaceName;
    static char		buf[256];
    static char		path[512];
    char		*str;
    void		*paramHandle;
    const char		*replayRateSchemeName;
    
	GfLogTrace("Entering Race Results menu\n");

    // Create screen, load menu XML descriptor and create static controls.
    rmScrHdle = GfuiScreenCreate();
    void *hmenu = GfuiMenuLoad("raceresultsmenu.xml");
    GfuiMenuCreateStaticControls(rmScrHdle, hmenu);

    // Create variable title label.
    snprintf(buf, sizeof(buf), "%s at %s", race, info->track->name);
    const int titleId = GfuiMenuCreateLabelControl(rmScrHdle, hmenu, "Title");
    GfuiLabelSetText(rmScrHdle, titleId, buf);
  
	// Get layout properties.
    const int nMaxLines = (int)GfuiMenuGetNumProperty(hmenu, "nMaxResultLines", 15);
    const int yTopLine = (int)GfuiMenuGetNumProperty(hmenu, "yTopLine", 400);
    const int yLineShift = (int)GfuiMenuGetNumProperty(hmenu, "yLineShift", 20);
	const GfuiColor cPlaceGain =
		GfuiColor::build(GfuiMenuGetStrProperty(hmenu, "colorGainedPlaces", "0x32CD32"));
	const float* acPlaceGain = cPlaceGain.toFloatRGBA();
	const GfuiColor cPlaceLoss =
		GfuiColor::build(GfuiMenuGetStrProperty(hmenu, "colorLostPlaces", "0xF28500"));
	const float* acPlaceLoss = cPlaceLoss.toFloatRGBA();

	// Never used : remove ?
    //Get total laps, winner time
    //snprintf(path, sizeof(path), "%s/%s/%s", info->track->name, RE_SECT_RESULTS, race);
    //int totLaps = (int)GfParmGetNum(results, path, RE_ATTR_LAPS, NULL, 0);
    //snprintf(path, sizeof(path), "%s/%s/%s/%s/%d", info->track->name, RE_SECT_RESULTS, race, RE_SECT_RANK, 1);
    //tdble refTime = GfParmGetNum(results, path, RE_ATTR_TIME, NULL, 0);

    //Get number of cars
    snprintf(path, sizeof(path), "%s/%s/%s/%s", info->track->name, RE_SECT_RESULTS, race, RE_SECT_RANK);
    int nbCars = (int)GfParmGetEltNb(results, path);
    
	// Display the result table.
    int y = yTopLine;
	int i;
    for (i = start; i < MIN(start + nMaxLines, nbCars); i++) {
        snprintf(path, sizeof(path), "%s/%s/%s/%s/%d", info->track->name, RE_SECT_RESULTS, race, RE_SECT_RANK, i + 1);
        int laps = (int)GfParmGetNum(results, path, RE_ATTR_LAPS, NULL, 0);//Laps covered

        //Rank
        snprintf(buf, sizeof(buf), "%d", i+1);
		GfuiMenuCreateLabelControl(rmScrHdle, hmenu, "Rank", true, // From template.
								   buf, GFUI_TPL_X, y);

        //Advance (The num.attrib 'index' holds the starting position)
        int advance = (int)(GfParmGetNum(results, path, RE_ATTR_INDEX, NULL, 0)) - i;
        snprintf(buf, sizeof(buf), "%d", advance);
        const float *aColor =
			advance > 0 ? acPlaceGain : (advance < 0 ? acPlaceLoss : GFUI_TPL_COLOR);
		GfuiMenuCreateLabelControl(rmScrHdle, hmenu, "Advance", true, // From template.
								   buf, GFUI_TPL_X, y, GFUI_TPL_FONTID, GFUI_TPL_WIDTH,
								   GFUI_TPL_ALIGN, GFUI_TPL_MAXLEN, aColor);

        //Driver short name
		GfuiMenuCreateLabelControl(rmScrHdle, hmenu, "DriverName", true, // From template.
								   GfParmGetStr(results, path, RE_ATTR_SNAME, ""), GFUI_TPL_X, y);

        //Driver type
        const std::string strModName = GfParmGetStr(results, path, RE_ATTR_MODULE, "");
		GfuiMenuCreateLabelControl(rmScrHdle, hmenu, "DriverType", true, // From template.
								   GfDriver::getType(strModName).c_str(), GFUI_TPL_X, y);

        //Car
		GfuiMenuCreateLabelControl(rmScrHdle, hmenu, "CarModel", true, // From template.
								   GfParmGetStr(results, path, RE_ATTR_CAR, ""), GFUI_TPL_X, y);

        //Total Time 
        str = GfTime2Str(GfParmGetNum(results, path, RE_ATTR_TIME, NULL, 0), 0, false, 3); 
		GfuiMenuCreateLabelControl(rmScrHdle, hmenu, "TotalTime", true, // From template.
								   str, GFUI_TPL_X, y);
        free(str);
        
        //Best lap
        str = GfTime2Str(GfParmGetNum(results, path, RE_ATTR_BEST_LAP_TIME, NULL, 0), 0, false, 3);
		GfuiMenuCreateLabelControl(rmScrHdle, hmenu, "BestLapTime", true, // From template.
								   str, GFUI_TPL_X, y);
        free(str);
        
        //Laps covered
        snprintf(buf, sizeof(buf), "%d", laps);
		GfuiMenuCreateLabelControl(rmScrHdle, hmenu, "Laps", true, // From template.
								   buf, GFUI_TPL_X, y);
        
        //Top speed
        snprintf(buf, sizeof(buf), "%3.1f", (GfParmGetNum(results, path, RE_ATTR_TOP_SPEED, NULL, 0) * 3.6));
		GfuiMenuCreateLabelControl(rmScrHdle, hmenu, "TopSpeed", true, // From template.
								   buf, GFUI_TPL_X, y);
        
        //Damage
        snprintf(buf, sizeof(buf), "%d", (int)(GfParmGetNum(results, path, RE_ATTR_DAMMAGES, NULL, 0)));
		GfuiMenuCreateLabelControl(rmScrHdle, hmenu, "Damages", true, // From template.
								   buf, GFUI_TPL_X, y);
        
        //Pitstops
        snprintf(buf, sizeof(buf), "%d", (int)(GfParmGetNum(results, path, RE_ATTR_NB_PIT_STOPS, NULL, 0)));
		GfuiMenuCreateLabelControl(rmScrHdle, hmenu, "Pits", true, // From template.
								   buf, GFUI_TPL_X, y);

        y -= yLineShift;  //Line feed
    }//for i

    //If it is not the first screen of the results, show a 'Prev' button
    if (start > 0) {
        RmPrevRace.prevHdle = prevHdle;
        RmPrevRace.info     = info;
        RmPrevRace.start    = start - nMaxLines;
        GfuiMenuCreateButtonControl(rmScrHdle, hmenu, "PreviousPageArrow",
          (void*)&RmPrevRace, rmChgRaceScreen);
        GfuiAddKey(rmScrHdle, GFUIK_PAGEUP,   "Previous Results", (void*)&RmPrevRace, rmChgRaceScreen, NULL);
    }//if start

    // Add "Continue" button
    GfuiMenuCreateButtonControl(rmScrHdle, hmenu, "ContinueButton", prevHdle, GfuiScreenReplace);

    // Add "Replay" button (if available)
    snprintf(buf, sizeof(buf), "%s%s", GfLocalDir(), RACE_ENG_CFG);
    paramHandle = GfParmReadFile(buf, GFPARM_RMODE_REREAD | GFPARM_RMODE_CREAT);
    replayRateSchemeName = GfParmGetStr(paramHandle, RM_SECT_RACE_ENGINE, RM_ATTR_REPLAY_RATE, RM_VAL_REPLAY_OFF);

    rmReplayButtonId = GfuiMenuCreateButtonControl(rmScrHdle, hmenu, "ReplayButton", prevHdle, rmReplayRace);
    if (strcmp(replayRateSchemeName, RM_VAL_REPLAY_OFF) == 0)
        GfuiEnable(rmScrHdle, rmReplayButtonId, GFUI_DISABLE);
    GfParmReleaseHandle(paramHandle);

    //Create 'save' button in the bottom right
    //rmSaveButtonId = GfuiMenuCreateButtonControl(rmScrHdle, hmenu, "SaveButton", info, rmSaveRes);

    //If we did not display all the results yet, let's show a 'Next' button
    if (i < nbCars) {
        RmNextRace.prevHdle = prevHdle;
        RmNextRace.info     = info;
        RmNextRace.start    = start + nMaxLines;
        GfuiMenuCreateButtonControl(rmScrHdle, hmenu, "NextPageArrow", (void*)&RmNextRace, rmChgRaceScreen);
        GfuiAddKey(rmScrHdle, GFUIK_PAGEDOWN, "Next Results", (void*)&RmNextRace, rmChgRaceScreen, NULL);
    }//if i

    //Link key handlers
    GfuiAddKey(rmScrHdle, GFUIK_ESCAPE, "Continue", prevHdle, GfuiScreenReplace, NULL);
    GfuiAddKey(rmScrHdle, GFUIK_RETURN, "Continue", prevHdle, GfuiScreenReplace, NULL);
    GfuiAddKey(rmScrHdle, GFUIK_F12, "Take a Screen Shot", NULL, GfuiScreenShot, NULL);
    GfuiAddKey(rmScrHdle, GFUIK_F1, "Help", rmScrHdle, GfuiHelpScreen, NULL);

	//Show!
    GfuiScreenActivate(rmScrHdle);
}//rmRaceResults


static void
rmChgQualifScreen(void *vprc)
{
    void		*prevScr = rmScrHdle;
    tRaceCall 	*prc = (tRaceCall*)vprc;

    rmQualifResults(prc->prevHdle, prc->info, prc->title, prc->start);
    GfuiScreenRelease(prevScr);
}

static void
rmQualifResults(void *prevHdle, tRmInfo *info, const char* pszTitle, int start)
{
    void		*results = info->results;
    const char		*race = info->_reRaceName;
    int			i;
    static char		buf[256];
    static char		path[512];
    char		*str;

	GfLogTrace("Entering %s Results menu\n", pszTitle);

    // Create screen, load menu XML descriptor and create static controls.
    rmScrHdle = GfuiScreenCreate();
    void *hmenu = GfuiMenuLoad("qualifsresultsmenu.xml");
    GfuiMenuCreateStaticControls(rmScrHdle, hmenu);

    // Create variable title label.
    const int titleId = GfuiMenuCreateLabelControl(rmScrHdle, hmenu, "Title");
    snprintf(buf, sizeof(buf), "%s at %s", race, info->track->name);
    GfuiLabelSetText(rmScrHdle, titleId, buf);

	// Get layout properties.
    const int nMaxLines = (int)GfuiMenuGetNumProperty(hmenu, "nMaxResultLines", 15);
    const int yTopLine = (int)GfuiMenuGetNumProperty(hmenu, "yTopLine", 400);
    const int yLineShift = (int)GfuiMenuGetNumProperty(hmenu, "yLineShift", 20);

	// Never used : remove ?
    //snprintf(path, sizeof(path), "%s/%s/%s/%s/%d", info->track->name, RE_SECT_RESULTS, race, RE_SECT_RANK, 1);
    //tdble refTime = GfParmGetNum(results, path, RE_ATTR_TIME, NULL, 0);
    //snprintf(path, sizeof(path), "%s/%s/%s", info->track->name, RE_SECT_RESULTS, race);
    //const int totLaps = (int)GfParmGetNum(results, path, RE_ATTR_LAPS, NULL, 0);
    snprintf(path, sizeof(path), "%s/%s/%s/%s", info->track->name, RE_SECT_RESULTS, race, RE_SECT_RANK);
    const int nbCars = (int)GfParmGetEltNb(results, path);
	GfLogDebug("rmQualifResults: path=%s, file=%s\n", path, GfParmGetFileName(results));
	GfLogDebug("rmQualifResults: start=%d, nbCars=%d, nMaxLines=%d\n", start, nbCars, nMaxLines);
	
	int y = yTopLine;
    for (i = start; i < MIN(start + nMaxLines, nbCars); i++) {
		snprintf(path, sizeof(path), "%s/%s/%s/%s/%d", info->track->name, RE_SECT_RESULTS, race, RE_SECT_RANK, i + 1);
		// Never used : remove ?
		//const int laps = (int)GfParmGetNum(results, path, RE_ATTR_LAPS, NULL, 0);

        //Rank
        snprintf(buf, sizeof(buf), "%d", i+1);
		GfuiMenuCreateLabelControl(rmScrHdle, hmenu, "Rank", true, // From template.
								   buf, GFUI_TPL_X, y);

        //Driver short name
		GfuiMenuCreateLabelControl(rmScrHdle, hmenu, "DriverName", true, // From template.
								   GfParmGetStr(results, path, RE_ATTR_SNAME, ""), GFUI_TPL_X, y);

        //Driver type
        const std::string strModName = GfParmGetStr(results, path, RE_ATTR_MODULE, "");
		GfuiMenuCreateLabelControl(rmScrHdle, hmenu, "DriverType", true, // From template.
								   GfDriver::getType(strModName).c_str(), GFUI_TPL_X, y);

        //Car
		GfuiMenuCreateLabelControl(rmScrHdle, hmenu, "CarModel", true, // From template.
								   GfParmGetStr(results, path, RE_ATTR_CAR, ""), GFUI_TPL_X, y);


        //Best lap
        str = GfTime2Str(GfParmGetNum(results, path, RE_ATTR_BEST_LAP_TIME, NULL, 0), 0, false, 3);
		GfuiMenuCreateLabelControl(rmScrHdle, hmenu, "BestLapTime", true, // From template.
								   str, GFUI_TPL_X, y);
        free(str);

		// Next line.
		y -= yLineShift;
    }//for i


    if (start > 0) {
		RmPrevRace.prevHdle = prevHdle;
		RmPrevRace.info     = info;
		RmPrevRace.start    = start - nMaxLines;
		RmPrevRace.title    = pszTitle;
		GfuiMenuCreateButtonControl(rmScrHdle, hmenu, "PreviousPageArrow",
									(void*)&RmPrevRace, rmChgQualifScreen);
		GfuiAddKey(rmScrHdle, GFUIK_PAGEUP,   "Previous Results", (void*)&RmPrevRace, rmChgQualifScreen, NULL);
    }

    // Add "Continue" button 
    GfuiMenuCreateButtonControl(rmScrHdle, hmenu, "ContinueButton", prevHdle, GfuiScreenReplace);
    
    //Create 'save' button in the bottom right
    //rmSaveButtonId = GfuiMenuCreateButtonControl(rmScrHdle, hmenu, "savebutton", info, rmSaveRes);

    if (i < nbCars) {
		RmNextRace.prevHdle = prevHdle;
		RmNextRace.info     = info;
		RmNextRace.start    = start + nMaxLines;
		RmNextRace.title    = pszTitle;
		GfuiMenuCreateButtonControl(rmScrHdle, hmenu, "NextPageArrow",
									(void*)&RmNextRace, rmChgQualifScreen);
		GfuiAddKey(rmScrHdle, GFUIK_PAGEDOWN, "Next Results", (void*)&RmNextRace, rmChgQualifScreen, NULL);
    }

    GfuiAddKey(rmScrHdle, GFUIK_ESCAPE, "Continue", prevHdle, GfuiScreenReplace, NULL);
    GfuiAddKey(rmScrHdle, GFUIK_RETURN, "Continue", prevHdle, GfuiScreenReplace, NULL);
    GfuiAddKey(rmScrHdle, GFUIK_F12, "Take a Screen Shot", NULL, GfuiScreenShot, NULL);
    GfuiAddKey(rmScrHdle, GFUIK_F1, "Help", rmScrHdle, GfuiHelpScreen, NULL);

    GfuiScreenActivate(rmScrHdle);
}

static void
rmChgStandingScreen(void *vprc)
{
    void		*prevScr = rmScrHdle;
    tRaceCall 	*prc = (tRaceCall*)vprc;

    RmShowStandings(prc->prevHdle, prc->info, prc->start);
    GfuiScreenRelease(prevScr);
}

/** 
 * RmShowStandings
 * 
 * Shows a results page, with optional prev/next results page buttons
 * 
 * @param prevHdle	handle for previous results page
 * @param info	race results information
 * @param start	page number
*/
void
RmShowStandings(void *prevHdle, tRmInfo *info, int start)
{
	int			i;
	static char		buf[256];
	static char		path[512];
	void		*results = info->results;

	GfLogTrace("Entering Standings menu\n");

    // Create screen, load menu XML descriptor and create static controls.
	rmScrHdle = GfuiScreenCreate();
	void *hmenu = GfuiMenuLoad("standingsmenu.xml");
	GfuiMenuCreateStaticControls(rmScrHdle, hmenu);

    // Create variable title label (with group info for the Career mode).
	const int titleId = GfuiMenuCreateLabelControl(rmScrHdle, hmenu, "Title");
	GfRaceManager* pRaceMan = LmRaceEngine().race()->getManager();
	if (pRaceMan->hasSubFiles())
	{
		const char* pszGroup = GfParmGetStr(info->params, RM_SECT_HEADER, RM_ATTR_NAME, "<no group>");
		snprintf(buf, sizeof(buf), "%s - %s", info->_reName, pszGroup);
	}
	else
		snprintf(buf, sizeof(buf), "%s", info->_reName);
	GfuiLabelSetText(rmScrHdle, titleId, buf);

    // Create variable subtitle label.
	const char* pszSessionName;
	const char* pszTrackName;
	if (pRaceMan->hasSubFiles())
	{
		// Career mode : Can't rely on GfRaceManager/GfRace, they don't support Career mode yet.
		pszSessionName = info->_reRaceName;
		const int curTrackIdx =
			(int)GfParmGetNum(results, RE_SECT_CURRENT, RE_ATTR_CUR_TRACK, NULL, 1) - 1;
		snprintf(path, sizeof(path), "%s/%d", RM_SECT_TRACKS, curTrackIdx);
		pszTrackName = GfParmGetStr(info->params, path, RM_ATTR_NAME, "<unkown track>");
	}
	else
	{
		// Non-Career mode : The session is the _last_ one ; the track is the _previous_ one.
		const unsigned nCurrEventIndex =
			(unsigned)GfParmGetNum(results, RE_SECT_CURRENT, RE_ATTR_CUR_TRACK, NULL, 1);
		pszSessionName =
			pRaceMan->getSessionName(pRaceMan->getSessionCount() - 1).c_str();
		pszTrackName =
			pRaceMan->getPreviousEventTrack(nCurrEventIndex - 1)->getName().c_str();
	}
	snprintf(buf, sizeof(buf), "%s at %s", pszSessionName, pszTrackName);
	const int subTitleId = GfuiMenuCreateLabelControl(rmScrHdle, hmenu, "SubTitle");
	GfuiLabelSetText(rmScrHdle, subTitleId, buf);

	// Get layout properties.
    const int nMaxLines = (int)GfuiMenuGetNumProperty(hmenu, "nMaxResultLines", 15);
    const int yTopLine = (int)GfuiMenuGetNumProperty(hmenu, "yTopLine", 400);
    const int yLineShift = (int)GfuiMenuGetNumProperty(hmenu, "yLineShift", 20);

	// List results line by line, paginated
	int y = yTopLine;
	const int nbCars = (int)GfParmGetEltNb(results, RE_SECT_STANDINGS);
	for (i = start; i < MIN(start + nMaxLines, nbCars); i++) {
		snprintf(path, sizeof(path), "%s/%d", RE_SECT_STANDINGS, i + 1);

        //Rank
        snprintf(buf, sizeof(buf), "%d", i+1);
		GfuiMenuCreateLabelControl(rmScrHdle, hmenu, "Rank", true, // From template.
								   buf, GFUI_TPL_X, y);

        //Driver short name
		GfuiMenuCreateLabelControl(rmScrHdle, hmenu, "DriverName", true, // From template.
								   GfParmGetStr(results, path, RE_ATTR_SNAME, ""), GFUI_TPL_X, y);

        //Driver type
        const std::string strModName = GfParmGetStr(results, path, RE_ATTR_MODULE, "");
		GfuiMenuCreateLabelControl(rmScrHdle, hmenu, "DriverType", true, // From template.
								   GfDriver::getType(strModName).c_str(), GFUI_TPL_X, y);

        //Car
		GfuiMenuCreateLabelControl(rmScrHdle, hmenu, "CarModel", true, // From template.
								   GfParmGetStr(results, path, RE_ATTR_CAR, ""), GFUI_TPL_X, y);


		//Points
		snprintf(buf, sizeof(buf), "%d", (int)GfParmGetNum(results, path, RE_ATTR_POINTS, NULL, 0));
		GfuiMenuCreateLabelControl(rmScrHdle, hmenu, "Points", true, // From template.
								   buf, GFUI_TPL_X, y);

		// Next line.
		y -= yLineShift;	//Next line
	}//for i

	// If not on first page, show 'previous results' button on the bottom left
	if (start > 0) {
		RmPrevRace.prevHdle = prevHdle;
		RmPrevRace.info     = info;
		RmPrevRace.start    = start - nMaxLines;
		GfuiMenuCreateButtonControl(rmScrHdle, hmenu, "PreviousPageArrow",
				    (void*)&RmPrevRace, rmChgStandingScreen);
		GfuiAddKey(rmScrHdle, GFUIK_PAGEUP, "Previous Results", (void*)&RmPrevRace, rmChgStandingScreen, NULL);
	}//if start

	// Add "Continue" button in the bottom left
	GfuiMenuCreateButtonControl(rmScrHdle, hmenu, "ContinueButton", prevHdle, GfuiScreenReplace);
    
	// Add "save" button in the bottom right, but disable it when Career mode.
	rmSaveButtonId = GfuiMenuCreateButtonControl(rmScrHdle, hmenu, "SaveButton", info, rmSaveRes);
	if (LmRaceEngine().race()->getManager()->hasSubFiles())
		GfuiEnable(rmScrHdle, rmSaveButtonId, GFUI_DISABLE);

	// If there is a next page, show 'next results' button on the bottom extreme right
	if (i < nbCars) {
		RmNextRace.prevHdle = prevHdle;
		RmNextRace.info     = info;
		RmNextRace.start    = start + nMaxLines;
		GfuiMenuCreateButtonControl(rmScrHdle, hmenu, "NextPageArrow",
				    (void*)&RmNextRace, rmChgStandingScreen);
		GfuiAddKey(rmScrHdle, GFUIK_PAGEDOWN, "Next Results", (void*)&RmNextRace, rmChgStandingScreen, NULL);
	}//if i

    GfuiAddKey(rmScrHdle, GFUIK_ESCAPE, "Continue", prevHdle, GfuiScreenReplace, NULL);
    GfuiAddKey(rmScrHdle, GFUIK_RETURN, "Continue", prevHdle, GfuiScreenReplace, NULL);
    GfuiAddKey(rmScrHdle, GFUIK_F1, "Help", rmScrHdle, GfuiHelpScreen, NULL);
    GfuiAddKey(rmScrHdle, GFUIK_F12, "Take a Screen Shot", NULL, GfuiScreenShot, NULL);

    GfuiScreenActivate(rmScrHdle);
}//RmShowStandings


void
RmShowResults(void *prevHdle, tRmInfo *info)
{
    switch (info->s->_raceType)
	{
		case RM_TYPE_PRACTICE:
		{
			char buffer[128];
			snprintf(buffer, sizeof(buffer), "%s/%s", info->track->name, RE_SECT_DRIVERS);
			int nCars = GfParmGetEltNb(info->results, buffer);
			bool bQualif = (nCars != 1);

			// Career special case : Practice results show multiple cars,
			// but only 1 driver, so no 'rank' section.
			// TODO: Rather fix the Career code ?
			if (bQualif)
			{
				snprintf(buffer, sizeof(buffer), "%s/%s/%s/%s", info->track->name, RE_SECT_RESULTS, info->_reRaceName, RE_SECT_RANK);
				nCars = (int)GfParmGetEltNb(info->results, buffer);
				GfLogDebug("RmShowResults: %d elements in %s\n", nCars, buffer);
				bQualif = bQualif && (nCars != 0);
			}
			
			if (bQualif)
				rmQualifResults(prevHdle, info, "Practice", 0);
			else
				rmPracticeResults(prevHdle, info, 0);
			break;
		}

		case RM_TYPE_RACE:
			rmRaceResults(prevHdle, info, 0);
			break;
			
		case RM_TYPE_QUALIF:
			rmQualifResults(prevHdle, info, "Qualification", 0);
			break;
    }//switch raceType
}//RmShowResults
