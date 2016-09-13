/***************************************************************************

    file        : racemanmenu.cpp
    created     : Fri Jan  3 22:24:41 CET 2003
    copyright   : (C) 2003 by Eric Espie                        
    email       : eric.espie@torcs.org   
    version     : $Id: racemanmenu.cpp 6353 2016-01-31 16:22:09Z beaglejoe $

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
    		The race manager menu (where you can configure, load, save, start a race)
    @author	<a href=mailto:eric.espie@torcs.org>Eric Espie</a>
    @version	$Id: racemanmenu.cpp 6353 2016-01-31 16:22:09Z beaglejoe $
*/

#include <vector>
#include <string>
#include <sstream>

#include <portability.h>
#include <tgfclient.h>

#include <raceman.h>

#include <racemanagers.h>
#include <race.h>
#include <tracks.h>
#include <drivers.h>
#include <cars.h>

#include <playerconfig.h>
#include <network.h>

#include "legacymenu.h"
#include "racescreens.h"


// Raceman menu.
static void	*ScrHandle = 0;

// Data for the race configuration menus.
static tRmFileSelect FileSelectData;

// Flag to know if the Player Config menu has been open from here.
static bool PlayerConfigOpen = false;

// Menu control Ids
static int TrackTitleLabelId;
static int SaveRaceConfigButtonId;
static int LoadRaceConfigButtonId;
static int LoadRaceResultsButtonId;
static int ResumeRaceButtonId;
static int StartNewRaceButtonId;
static int TrackOutlineImageId;
static int CompetitorsScrollListId;

// Vector to hold competitors scroll-list elements.
static std::vector<std::string> VecCompetitorsInfo;

// Pre-declarations of local functions.
static void rmOnRaceDataChanged();

// Accessors to the menu handle -------------------------------------------------------------
void RmSetRacemanMenuHandle(void * handle)
{
	ScrHandle = handle;
}
void* RmGetRacemanMenuHandle()
{
	return ScrHandle;
}

void
RmConfigureRace(void * /* dummy */)
{
	RmConfigRunState(/*bStart=*/true);
}

// Callbacks for the File Select menu --------------------------------------------------------
static void
rmSaveRaceToConfigFile(const char *filename)
{
	// Note: No need to write the main file here, already done at the end of race configuration.
	const GfRaceManager* pRaceMan = LmRaceEngine().race()->getManager();

	// Determine the full path-name of the target race config file (add .xml ext. if not there).
	std::ostringstream ossTgtFileName;
	ossTgtFileName << GfLocalDir() << "config/raceman/" << pRaceMan->getId() << '/' << filename;
	if (ossTgtFileName.str().rfind(PARAMEXT) != ossTgtFileName.str().length() - strlen(PARAMEXT))
		ossTgtFileName << PARAMEXT;

	// Copy the main file to the selected one (overwrite if already there).
	const std::string strMainFileName = pRaceMan->getDescriptorFileName();
	GfLogInfo("Saving race config to %s ...\n", strMainFileName.c_str());
	if (!GfFileCopy(strMainFileName.c_str(), ossTgtFileName.str().c_str()))
		GfLogError("Failed to save race to selected config file %s", ossTgtFileName.str().c_str());
}

static void
rmLoadRaceFromConfigFile(const char *filename)
{
	GfRaceManager* pRaceMan = LmRaceEngine().race()->getManager();

	// Determine the full path-name of the selected race config file.
	std::ostringstream ossSelFileName;
	ossSelFileName << GfLocalDir() << "config/raceman/" << pRaceMan->getId() << '/' << filename;

	GfLogInfo("Loading saved race from config %s ...\n", ossSelFileName.str().c_str());

	// Replace the main race config file by the selected one.
	const std::string strMainFileName = pRaceMan->getDescriptorFileName();
	if (!GfFileCopy(ossSelFileName.str().c_str(), strMainFileName.c_str()))
	{
		GfLogError("Failed to load selected race config file %s", strMainFileName.c_str());
		return;
	}
	
	// Update the race manager.
	void* hparmRaceMan =
		GfParmReadFile(strMainFileName.c_str(), GFPARM_RMODE_STD | GFPARM_RMODE_REREAD);
	if (hparmRaceMan)
	{
		pRaceMan->reset(hparmRaceMan, /* bClosePrevHdle= */ true);

		// (Re-)initialize the race from the selected race manager.
		LmRaceEngine().race()->load(pRaceMan);

		// Notify the race engine of the changes (this is a non-interactive config., actually).
		LmRaceEngine().configureRace(/* bInteractive */ false);
	}
	
	// Update GUI.
	rmOnRaceDataChanged();
}

static void
rmLoadRaceFromResultsFile(const char *filename)
{
	GfRaceManager* pRaceMan = LmRaceEngine().race()->getManager();

	// Determine the full path-name of the result file.
	std::ostringstream ossResFileName;
	ossResFileName << GfLocalDir() << "results/" << pRaceMan->getId() << '/' << filename;

 	GfLogInfo("Restoring race from results %s ...\n", ossResFileName.str().c_str());

	// (Re-)initialize the race from the selected race manager and results params.
	void* hparmResults =
		GfParmReadFile(ossResFileName.str().c_str(), GFPARM_RMODE_STD | GFPARM_RMODE_REREAD);
	if (hparmResults)
	{
		LmRaceEngine().race()->load(pRaceMan, /*bKeepHumans=*/true, hparmResults);

		// Restore the race from the result file.
		LmRaceEngine().restoreRace(hparmResults);
	}
	
	// Update GUI.
	rmOnRaceDataChanged();
}

// Callbacks for the current menu ------------------------------------------------------------
static void
rmOnActivate(void * /* dummy */)
{
	GfLogTrace("Entering Race Manager menu\n");

	// If we are coming back from the Player Config menu, update the race data
	// (some human drivers = players may have appeared, disappeared or been renamed).
	if (PlayerConfigOpen)
	{
		// Reload the race from disk.
		GfRace* pRace = LmRaceEngine().race();
		GfRaceManager* pRaceMan = pRace->getManager();
		void* hparmResults = pRace->getResultsDescriptorHandle();
		pRace->load(pRaceMan, /*bKeepHumans=*/true, hparmResults);

		// End of "back from Player Config menu" in any case.
		PlayerConfigOpen = false;
	}

	// Update GUI.
	rmOnRaceDataChanged();
}

static void
rmOnRaceDataChanged()
{
	GfRace* pRace = LmRaceEngine().race();
	const GfRaceManager* pRaceMan = pRace->getManager();

	// Get the currently selected track for the race (should never fail, unless no track at all).
	const GfTrack* pTrack = pRace->getTrack();

	// Set title (race type + track name).
	std::ostringstream ossText;
	ossText << "at " << pTrack->getName();
	GfuiLabelSetText(ScrHandle, TrackTitleLabelId, ossText.str().c_str());

	// Display track name, outline image and preview image
	GfuiScreenAddBgImg(ScrHandle, pTrack->getPreviewFile().c_str());
	GfuiStaticImageSet(ScrHandle, TrackOutlineImageId, pTrack->getOutlineFile().c_str());

	// Show/Hide "Load race" buttons as needed.
	const bool bIsMultiEvent = pRaceMan->isMultiEvent();
	GfuiVisibilitySet(ScrHandle, LoadRaceConfigButtonId,
					  !bIsMultiEvent ? GFUI_VISIBLE : GFUI_INVISIBLE);
	GfuiVisibilitySet(ScrHandle, LoadRaceResultsButtonId,
					  bIsMultiEvent ? GFUI_VISIBLE : GFUI_INVISIBLE);

	// Enable/Disable "Load/Save race" buttons as needed.
	GfuiEnable(ScrHandle, SaveRaceConfigButtonId, 
			   !bIsMultiEvent ? GFUI_ENABLE : GFUI_DISABLE);
	GfuiEnable(ScrHandle, LoadRaceConfigButtonId, 
			   !bIsMultiEvent && pRaceMan->hasSavedConfigsFiles() ? GFUI_ENABLE : GFUI_DISABLE);
	GfuiEnable(ScrHandle, LoadRaceResultsButtonId, 
			   bIsMultiEvent && pRaceMan->hasResultsFiles() ? GFUI_ENABLE : GFUI_DISABLE);

	// Show/Hide "Start / Resume race" buttons as needed.
	const std::vector<GfDriver*>& vecCompetitors = pRace->getCompetitors();
	const bool bWasLoadedFromResults = pRace->getResultsDescriptorHandle() != 0;
	GfuiVisibilitySet(ScrHandle, StartNewRaceButtonId,
					  !vecCompetitors.empty() && !bWasLoadedFromResults
					  ? GFUI_VISIBLE : GFUI_INVISIBLE);
	GfuiVisibilitySet(ScrHandle, ResumeRaceButtonId,
					  !vecCompetitors.empty() && bWasLoadedFromResults
					  ? GFUI_VISIBLE : GFUI_INVISIBLE);

	// Re-load competitors scroll list from the race.
	GfuiScrollListClear(ScrHandle, CompetitorsScrollListId);
	VecCompetitorsInfo.clear();
    VecCompetitorsInfo.reserve(vecCompetitors.size());
    for (int nCompIndex = 0; nCompIndex < (int)vecCompetitors.size(); nCompIndex++)
	{
		const GfDriver* pComp = vecCompetitors[nCompIndex];
		ossText.str("");
		ossText << pComp->getName();
		if (!pRaceMan->hasSubFiles()) // Don't show car name if Career mode (N/A here).
			ossText << " (" << pComp->getCar()->getName() << ')';
		VecCompetitorsInfo.push_back(ossText.str());
	}
	for (int nCompIndex = 0; nCompIndex < (int)vecCompetitors.size(); nCompIndex++)
	{
		const GfDriver* pComp = vecCompetitors[nCompIndex];
		GfuiScrollListInsertElement(ScrHandle, CompetitorsScrollListId,
									VecCompetitorsInfo[nCompIndex].c_str(), nCompIndex+1, (void*)pComp);
		//GfLogDebug("Added competitor %s (%s#%d)\n", ossText.str().c_str(),
		//		   pComp->getModuleName().c_str(),  pComp->getInterfaceIndex());
	}

	// Show the driver at the pole position.
	if (!vecCompetitors.empty())
		GfuiScrollListShowElement(ScrHandle, CompetitorsScrollListId, 0);
}

static void
rmOnSelectCompetitor(void * /* dummy */)
{
	// TODO: Display some details somewhere about the selected competitor ?
	GfDriver* pComp = 0;
    const char *pszElementText =
		GfuiScrollListGetSelectedElement(ScrHandle, CompetitorsScrollListId, (void**)&pComp);
	if (pszElementText && pComp)
		GfLogDebug("Selecting %s\n", pComp->getName().c_str());
}

static void
rmOnPlayerConfig(void * /* dummy */)
{
	/* Here, we need to call OptionOptionInit each time the firing button
	   is pressed, and not only once at the Raceman menu initialization,
	   because the previous menu has to be saved (ESC, Back) and because it can be this menu,
	   as well as the Main menu */
	GfuiScreenActivate(PlayerConfigMenuInit(ScrHandle));

	// Keep this in mind for when we go back here.
	PlayerConfigOpen = true;
}

static void
rmOnLoadRaceFromConfigFile(void *pPrevMenu)
{
	GfRaceManager* pRaceMan = LmRaceEngine().race()->getManager();
	
	FileSelectData.title = pRaceMan->getName();
	FileSelectData.prevScreen = pPrevMenu;
	FileSelectData.mode = RmFSModeLoad;

	FileSelectData.dirPath = pRaceMan->getSavedConfigsDir();
	FileSelectData.namePrefix = "";
	FileSelectData.nameSuffix = RESULTEXT;

	FileSelectData.select = rmLoadRaceFromConfigFile;

	// Fire the file selection menu.
	GfuiScreenActivate(RmFileSelect(&FileSelectData));
}

static void
rmOnLoadRaceFromResultsFile(void *pPrevMenu)
{
	GfRaceManager* pRaceMan = LmRaceEngine().race()->getManager();
	
	FileSelectData.title = pRaceMan->getName();
	FileSelectData.prevScreen = pPrevMenu;
	FileSelectData.mode = RmFSModeLoad;

	FileSelectData.dirPath = pRaceMan->getResultsDir();
	FileSelectData.namePrefix = "";
	FileSelectData.nameSuffix = RESULTEXT;

	FileSelectData.select = rmLoadRaceFromResultsFile;

	// Fire the file selection menu.
	GfuiScreenActivate(RmFileSelect(&FileSelectData));
}

static void
rmOnSaveRaceToConfigFile(void *pPrevMenu)
{
	const GfRaceManager* pRaceMan = LmRaceEngine().race()->getManager();
	
	// Fill-in file selection descriptor
	FileSelectData.title = pRaceMan->getName();
	FileSelectData.prevScreen = pPrevMenu;
	FileSelectData.mode = RmFSModeSave;

	FileSelectData.dirPath = GfLocalDir();
	FileSelectData.dirPath += "config/raceman/";
	FileSelectData.dirPath += pRaceMan->getId();
	FileSelectData.namePrefix = "";
	FileSelectData.nameSuffix = RESULTEXT;

	FileSelectData.select = rmSaveRaceToConfigFile;

	// Fire the file selection menu.
	GfuiScreenActivate(RmFileSelect(&FileSelectData));
}

static void
rmStartNewRace(void * /* dummy */)
{
	LmRaceEngine().startNewRace();
}

static void
rmResumeRace(void * /* dummy */)
{
	LmRaceEngine().resumeRace();
}

// Init. function for the current menu -----------------------------------------------------
void
RmRacemanMenu()
{
	// Special case of the online race, not yet migrated to using tgfdata.
	// TODO: Integrate better the networking menu system in the race config. menu system
	//       (merge the RmNetworkClientMenu and RmNetworkHostMenu into this race man menu,
	//        after adding some more features / controls ? because they look similar).
	tRmInfo* reInfo = LmRaceEngine().inData();
	if (!strcmp(reInfo->_reName, "Online Race"))
	{
		// Temporary, as long as the networking menu are not ported to tgfdata.
		
		// Force any needed fix on the specified track for the race (may not exist)
		const GfTrack* pTrack = LmRaceEngine().race()->getTrack();
		GfLogTrace("Using track %s for Online Race", pTrack->getName().c_str());

		// Synchronize reInfo->params with LmRaceEngine().race() state,
		// in case the track was fixed.
		if (LmRaceEngine().race()->isDirty())
			LmRaceEngine().race()->store(); // Save data to params.
		
		// End of temporary.

		if (NetGetNetwork())
		{
			if (NetGetNetwork()->IsConnected())
			{
				if (NetIsClient())
				{
					RmNetworkClientMenu(NULL);
					return;
				}
				else if (NetIsServer())
				{
					RmNetworkHostMenu(NULL);
					return;
				}
			}
		}
		else
		{
			RmNetworkMenu(NULL);
			return;
		}
	}

	// Don't do this twice.
	if (ScrHandle)
		GfuiScreenRelease(ScrHandle);

	const GfRaceManager* pRaceMan = LmRaceEngine().race()->getManager();
	// Ask the RaceEngine what types of races should be allowed here
	bool SupportsHumanDrivers = LmRaceEngine().supportsHumanDrivers();


	// Create screen, load menu XML descriptor and create static controls.
	ScrHandle = GfuiScreenCreate(NULL, NULL, rmOnActivate, 
										 NULL, (tfuiCallback)NULL, 1);
	void *menuXMLDescHdle = GfuiMenuLoad("racemanmenu.xml");
	
	GfuiMenuCreateStaticControls(ScrHandle, menuXMLDescHdle);

	// Create and initialize static title label (race mode name).
	const int nRaceModeTitleLabelId =
		GfuiMenuCreateLabelControl(ScrHandle, menuXMLDescHdle, "RaceModeTitleLabel");
	GfuiLabelSetText(ScrHandle, nRaceModeTitleLabelId, pRaceMan->getName().c_str());

	// Create variable title label (track name).
	TrackTitleLabelId = GfuiMenuCreateLabelControl(ScrHandle, menuXMLDescHdle, "TrackTitleLabel");

	// Create Configure race, Configure players and Back buttons.
	GfuiMenuCreateButtonControl(ScrHandle, menuXMLDescHdle, "ConfigureRaceButton",
								NULL, RmConfigureRace);
	if (SupportsHumanDrivers)
		GfuiMenuCreateButtonControl(ScrHandle, menuXMLDescHdle, "ConfigurePlayersButton",
								NULL, rmOnPlayerConfig);
	
	GfuiMenuCreateButtonControl(ScrHandle, menuXMLDescHdle, "BackButton",
								RmRaceSelectMenuHandle, GfuiScreenActivate);

	if (SupportsHumanDrivers)
	{
		// Create "Load / Resume / Save race" buttons.
		SaveRaceConfigButtonId =
			GfuiMenuCreateButtonControl(ScrHandle, menuXMLDescHdle, "SaveRaceConfigButton",
									ScrHandle, rmOnSaveRaceToConfigFile);
		LoadRaceConfigButtonId =
			GfuiMenuCreateButtonControl(ScrHandle, menuXMLDescHdle, "LoadRaceConfigButton",
									ScrHandle, rmOnLoadRaceFromConfigFile);
		LoadRaceResultsButtonId =
			GfuiMenuCreateButtonControl(ScrHandle, menuXMLDescHdle, "LoadRaceResultsButton",
									ScrHandle, rmOnLoadRaceFromResultsFile);
	}

	// Create "Resume / Start race" buttons.
	ResumeRaceButtonId =
		GfuiMenuCreateButtonControl(ScrHandle, menuXMLDescHdle, "ResumeRaceButton",
							NULL, rmResumeRace);
	StartNewRaceButtonId =
		GfuiMenuCreateButtonControl(ScrHandle, menuXMLDescHdle, "StartNewRaceButton",
							NULL, rmStartNewRace);

	// Track outline image.
	TrackOutlineImageId =
		GfuiMenuCreateStaticImageControl(ScrHandle, menuXMLDescHdle, "TrackOutlineImage");

	// Competitors scroll-list
	CompetitorsScrollListId =
		GfuiMenuCreateScrollListControl(ScrHandle, menuXMLDescHdle, "CompetitorsScrollList",
								NULL, rmOnSelectCompetitor);

	// Close menu XML descriptor.
	GfParmReleaseHandle(menuXMLDescHdle);
	
	// Register keyboard shortcuts.
	GfuiMenuDefaultKeysAdd(ScrHandle);
	GfuiAddKey(ScrHandle, GFUIK_RETURN, "Start the race",
			   NULL, rmStartNewRace, NULL);
	GfuiAddKey(ScrHandle, GFUIK_ESCAPE, "Back to the Main menu",
			   RmRaceSelectMenuHandle, GfuiScreenActivate, NULL);

	// Activate screen.
	GfuiScreenActivate(ScrHandle);
}
