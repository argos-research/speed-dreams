/***************************************************************************

    file        : racestartmenu.cpp
    created     : Sun Dec  8 13:01:47 CET 2002
    copyright   : (C) 2002 by Eric Espie
    email       : eric.espie@torcs.org
    version     : $Id: racestartmenu.cpp 5284 2013-03-10 10:49:04Z pouillot $

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
                The race start menu
    @author     <a href=mailto:eric.espie@torcs.org>Eric Espie</a>
    @version    $Id: racestartmenu.cpp 5284 2013-03-10 10:49:04Z pouillot $
*/

#include <cstdio>

#include <portability.h>
#include <tgfclient.h>

#include <robot.h>

#include <drivers.h> // GfDriver::getType
#include <race.h>
#include <racemanagers.h>

#include "legacymenu.h"
#include "racescreens.h"


// Abandon race hook ******************************************************
static void
rmAbandonRaceHookActivate(void * /* vforce */)
{
	LmRaceEngine().abandonRace();
}

static void *pvAbandonRaceHookHandle = 0;

static void *
rmAbandonRaceHookInit(void)
{
	if (!pvAbandonRaceHookHandle)
		pvAbandonRaceHookHandle = GfuiHookCreate(0, rmAbandonRaceHookActivate);

	return pvAbandonRaceHookHandle;
}

// Start race hook ******************************************************
static void
rmStartRaceHookActivate(void * /* dummy */)
{
	LmRaceEngine().startRace();
}

static void	*pvStartRaceHookHandle = 0;

static void *
rmStartRaceHookInit(void)
{
	if (!pvStartRaceHookHandle)
		pvStartRaceHookHandle = GfuiHookCreate(0, rmStartRaceHookActivate);

	return pvStartRaceHookHandle;
}

// The menu itself ******************************************************
typedef struct 
{
    void        *startScr;
    void        *abortScr;
    tRmInfo     *info;
    int         start;
} tStartRaceCall;

static tStartRaceCall   nextStartRace, prevStartRace;
static void             *rmScrHdle = 0;

static void rmStartRaceMenu(tRmInfo *info, void *startScr, void *abortScr, int start = 0);

static void
rmChgStartScreen(void *vpsrc)
{
    void                *prevScr = rmScrHdle;
    tStartRaceCall      *psrc = (tStartRaceCall*)vpsrc;
    
    rmStartRaceMenu(psrc->info, psrc->startScr, psrc->abortScr, psrc->start);
    GfuiScreenRelease(prevScr);
}

void
rmStartRaceMenu(tRmInfo *info, void *startScr, void *abortScr, int start)
{
    static char path[512];
    static char buf[128];
    void        *params = info->params;
    
	GfLogTrace("Entering Start Race menu\n");
	
    // Create screen, load menu XML descriptor and create static controls.
    rmScrHdle = GfuiScreenCreate();
    void *hmenu = GfuiMenuLoad("startracemenu.xml");
    GfuiMenuCreateStaticControls(rmScrHdle, hmenu);

    // Create variable title label.
    const int titleId = GfuiMenuCreateLabelControl(rmScrHdle, hmenu, "TitleLabel");
	if (LmRaceEngine().race()->getManager()->hasSubFiles())
	{
		const char* pszGroup = GfParmGetStr(info->params, RM_SECT_HEADER, RM_ATTR_NAME, "<no group>");
		snprintf(buf, sizeof(buf), "%s - %s", info->_reName, pszGroup);
	}
	else
		snprintf(buf, sizeof(buf), "%s", info->_reName);
    GfuiLabelSetText(rmScrHdle, titleId, buf);
	
    // Create background image if any.
    const char* img = GfParmGetStr(params, RM_SECT_HEADER, RM_ATTR_STARTIMG, 0);
    if (img)
        GfuiScreenAddBgImg(rmScrHdle, img);
        
    // Create starting grid labels if specified in race params.
    if (!strcmp(GfParmGetStr(params, info->_reRaceName, RM_ATTR_DISP_START_GRID, RM_VAL_YES), RM_VAL_YES))
	{
        // Create starting grid subtitle label.
        const int subTitleId = GfuiMenuCreateLabelControl(rmScrHdle, hmenu, "SubTitleLabel");
		snprintf(buf, sizeof(buf), "%s at %s", info->_reRaceName, info->track->name);
		GfuiLabelSetText(rmScrHdle, subTitleId, buf);

		// Get layout properties.
		const int nMaxLines = (int)GfuiMenuGetNumProperty(hmenu, "nMaxLines", 15);
		const int yTopLine = (int)GfuiMenuGetNumProperty(hmenu, "yTopLine", 400);
		const int yLineShift = (int)GfuiMenuGetNumProperty(hmenu, "yLineShift", 20);

        // Create drivers info table.
        //snprintf(path, sizeof(path), "%s/%s", info->_reRaceName, RM_SECT_STARTINGGRID);
        //const int rows = (int)GfParmGetNum(params, path, RM_ATTR_ROWS, (char*)NULL, 2);
        const int nCars = GfParmGetEltNb(params, RM_SECT_DRIVERS_RACING);
        int y = yTopLine;
		int i = start;
        for (; i < MIN(start + nMaxLines, nCars); i++)
		{
            // Find starting driver's name
            snprintf(path, sizeof(path), "%s/%d", RM_SECT_DRIVERS_RACING, i + 1);
            const char* modName = GfParmGetStr(info->params, path, RM_ATTR_MODULE, "");
            const int robotIdx = (int)GfParmGetNum(info->params, path, RM_ATTR_IDX, NULL, 0);
            const int extended = (int)GfParmGetNum(info->params, path, RM_ATTR_EXTENDED, NULL, 0);
			
            void* robhdle = 0;
			snprintf(path, sizeof(path), "%sdrivers/%s/%s.xml", GfLocalDir(), modName, modName);
			robhdle = GfParmReadFile(path, GFPARM_RMODE_STD);
			if (!robhdle)
			{
				snprintf(path, sizeof(path), "drivers/%s/%s.xml", modName, modName);
				robhdle = GfParmReadFile(path, GFPARM_RMODE_STD);
			}
  
            const char* name = modName;
 			if (robhdle)
			{
				snprintf(path, sizeof(path), "%s/%s/%d", ROB_SECT_ROBOTS, ROB_LIST_INDEX, robotIdx);
				name = GfParmGetStr(robhdle, path, ROB_ATTR_NAME, modName);
			}

			const char* carName = 0;
			if (extended)
			{
				snprintf(path, sizeof(path), "%s/%s/%d/%d", RM_SECT_DRIVERINFO, modName, extended, robotIdx);
				carName = GfParmGetStr(info->params, path, RM_ATTR_CARNAME, "<not found>");
				if (name == modName)
					name = GfParmGetStr(info->params, path, ROB_ATTR_NAME, "<not found>");
			}
			else if (robhdle)
			{
				carName = GfParmGetStr(robhdle, path, ROB_ATTR_CAR, "<not found>");
			}
			
			void* carHdle = 0;
            if (carName)
            {
                snprintf(path, sizeof(path), "cars/models/%s/%s.xml", carName, carName);
                carHdle = GfParmReadFile(path, GFPARM_RMODE_STD);
                carName = GfParmGetName(carHdle);
			}
			
			//Rank
			snprintf(buf, sizeof(buf), "%d", i+1);
			GfuiMenuCreateLabelControl(rmScrHdle, hmenu, "Rank", true, // From template.
									   buf, GFUI_TPL_X, y);
			
			//Driver name
			GfuiMenuCreateLabelControl(rmScrHdle, hmenu, "DriverName", true, // From template.
									   name, GFUI_TPL_X, y);
			
			//Driver type
			GfuiMenuCreateLabelControl(rmScrHdle, hmenu, "DriverType", true, // From template.
									   GfDriver::getType(modName).c_str(), GFUI_TPL_X, y);
			
			//Car model
			GfuiMenuCreateLabelControl(rmScrHdle, hmenu, "CarModel", true, // From template.
									   carName ? carName : "<not found>", GFUI_TPL_X, y);

            y -= yLineShift;

			// Release params handles only when no more need for read strings (like carname).
			if (carHdle)
				GfParmReleaseHandle(carHdle);
			if (robhdle)
				GfParmReleaseHandle(robhdle);
        }
                
        if (start > 0)
		{
            prevStartRace.startScr = startScr;
            prevStartRace.abortScr = abortScr;
            prevStartRace.info     = info;
            prevStartRace.start    = start - nMaxLines;

            // Create Previous page button and associated keyboard shortcut if needed.
            GfuiMenuCreateButtonControl(rmScrHdle, hmenu, "PreviousPageArrow",
										(void*)&prevStartRace, rmChgStartScreen);
            GfuiAddKey(rmScrHdle, GFUIK_PAGEUP, "Previous drivers", 
					   (void*)&prevStartRace, rmChgStartScreen, NULL);
        }
                
        if (i < nCars)
		{
            nextStartRace.startScr = startScr;
            nextStartRace.abortScr = abortScr;
            nextStartRace.info     = info;
            nextStartRace.start    = start + nMaxLines;

            // Create Next page button and associated keyboard shortcut if needed.
            GfuiMenuCreateButtonControl(rmScrHdle, hmenu, "NextPageArrow",
										(void*)&nextStartRace, rmChgStartScreen);
            GfuiAddKey(rmScrHdle, GFUIK_PAGEDOWN, "Next Drivers", 
					   (void*)&nextStartRace, rmChgStartScreen, NULL);
        }
    }
        
    // Create Start and Abandon buttons.
    GfuiMenuCreateButtonControl(rmScrHdle, hmenu, "StartButton", startScr, GfuiScreenReplace);
    GfuiMenuCreateButtonControl(rmScrHdle, hmenu, "AbandonButton", abortScr, GfuiScreenReplace);

    // Close menu XML descriptor.
    GfParmReleaseHandle(hmenu);
    
    // Register keyboard shortcuts.
    GfuiAddKey(rmScrHdle, GFUIK_RETURN, "Start", startScr, GfuiScreenReplace, NULL);
    GfuiAddKey(rmScrHdle, GFUIK_ESCAPE, "Abandon", abortScr, GfuiScreenReplace, NULL);
    GfuiAddKey(rmScrHdle, GFUIK_F1, "Help", rmScrHdle, GfuiHelpScreen, NULL);
    GfuiAddKey(rmScrHdle, GFUIK_F12, "Take a Screen Shot", NULL, GfuiScreenShot, NULL);
        
    // Activate the created screen.
    GfuiScreenActivate(rmScrHdle);
}

void
RmStartRaceMenu()
{
	rmStartRaceMenu(LmRaceEngine().inData(),
					rmStartRaceHookInit(), rmAbandonRaceHookInit());
}

void
RmStartRaceMenuShutdown()
{
	GfuiHookRelease(pvAbandonRaceHookHandle);
	pvAbandonRaceHookHandle = 0;
	
	GfuiHookRelease(pvStartRaceHookHandle);
	pvStartRaceHookHandle = 0;
}
