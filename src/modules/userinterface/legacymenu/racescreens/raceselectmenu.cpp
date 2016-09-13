/***************************************************************************

    file        : raceselectmenu.cpp
    created     : Sat Nov 16 09:36:29 CET 2002
    copyright   : (C) 2002 by Eric Espie
    email       : eric.espie@torcs.org   
    version     : $Id: raceselectmenu.cpp 5854 2014-11-23 17:55:52Z wdbee $                                  

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
    		Race selection menu
    @author	<a href=mailto:eric.espie@torcs.org>Eric Espie</a>
    @version	$Id: raceselectmenu.cpp 5854 2014-11-23 17:55:52Z wdbee $
*/

#include <map>
#include <algorithm>

#include <tgfclient.h>

#include <raceman.h>

#include <racemanagers.h>
#include <race.h>

#include "legacymenu.h"
#include "racescreens.h"


// The Race Select menu.
void *RmRaceSelectMenuHandle = NULL;


static std::map<std::string, int> rmMapSubTypeComboIds;


/* Called when the menu is activated */
static void
rmOnActivate(void * /* dummy */)
{
	GfLogTrace("Entering Race Mode Select menu\n");

    LmRaceEngine().reset();
}

/* Exit from Race engine */
static void
rmOnRaceSelectShutdown(void *prevMenu)
{
    GfuiScreenActivate(prevMenu);
	
    LmRaceEngine().cleanup();
	
	LegacyMenu::self().shutdownGraphics(/*bUnloadModule=*/true);
}


static void
rmOnSelectRaceMan(void *pvRaceManTypeIndex)
{
	// Get the race managers with the given type
	const std::vector<std::string>& vecRaceManTypes = GfRaceManagers::self()->getTypes();
	const std::string strRaceManType = vecRaceManTypes[(long)pvRaceManTypeIndex];
	const std::vector<GfRaceManager*> vecRaceMans =
		GfRaceManagers::self()->getRaceManagersWithType(strRaceManType);

	// If more than 1, get the one with the currently selected sub-type.
	GfRaceManager* pSelRaceMan = 0;
	if (vecRaceMans.size() > 1)
	{
		const int nSubTypeComboId = rmMapSubTypeComboIds[strRaceManType];
		const char* pszSelSubType = GfuiComboboxGetText(RmRaceSelectMenuHandle, nSubTypeComboId);
		std::vector<GfRaceManager*>::const_iterator itRaceMan;
		for (itRaceMan = vecRaceMans.begin(); itRaceMan != vecRaceMans.end(); itRaceMan++)
		{
			if ((*itRaceMan)->getSubType() == pszSelSubType)
			{
				// Start configuring it.
				pSelRaceMan = *itRaceMan;
				break;
			}
		}
	}

	// If only 1, no choice.
	else if (vecRaceMans.size() == 1)
	{
		pSelRaceMan = vecRaceMans.back();
	}

	if (pSelRaceMan)
	{
		// Give the selected race manager to the race engine.
		LmRaceEngine().selectRaceman(pSelRaceMan);
		
		// Start the race configuration menus sequence.
		LmRaceEngine().configureRace(/* bInteractive */ true);
	}
	else
	{
		GfLogError("No such race manager (type '%s')\n", strRaceManType.c_str());
	}
}

static void
rmOnChangeRaceMan(tComboBoxInfo *)
{
}

/* Initialize the single player menu */
void *
RmRaceSelectInit(void *prevMenu)
{
    if (RmRaceSelectMenuHandle) 
		return RmRaceSelectMenuHandle;

	// Ask the RaceEngine what types of races should be allowed here
	bool SupportsHumanDrivers = LmRaceEngine().supportsHumanDrivers();

    // Create screen, load menu XML descriptor and create static controls.
    RmRaceSelectMenuHandle = GfuiScreenCreate((float*)NULL, 
											NULL, rmOnActivate, 
											NULL, (tfuiCallback)NULL, 
											1);
    void *hMenuXMLDesc = GfuiMenuLoad("raceselectmenu.xml");
    GfuiMenuCreateStaticControls(RmRaceSelectMenuHandle, hMenuXMLDesc);

    // Create the raceman type buttons and sub-type combo-boxes (if any).
	const std::vector<std::string>& vecRaceManTypes = GfRaceManagers::self()->getTypes();
	std::vector<std::string>::const_iterator itRaceManType;
	// For each race manager type :
	for (itRaceManType = vecRaceManTypes.begin();
		 itRaceManType != vecRaceManTypes.end(); itRaceManType++)
	{
		// Get the racemanagers with this type
		const std::vector<GfRaceManager*> vecRaceMans =
			GfRaceManagers::self()->getRaceManagersWithType(itRaceManType->c_str());

		// Create the race manager type button.
		std::string strButtonCtrlName(*itRaceManType);
		if (!SupportsHumanDrivers)
		{
			if ((strButtonCtrlName != "Practice")
				&& (strButtonCtrlName != "OptimizationMT"))
				continue;
		}
		else
		{
			if (strButtonCtrlName == "OptimizationMT")
				continue;
		}

		strButtonCtrlName.erase(std::remove(strButtonCtrlName.begin(), strButtonCtrlName.end(), ' '), strButtonCtrlName.end()); // Such a pain to remove spaces !
		strButtonCtrlName += "Button";
		GfuiMenuCreateButtonControl(RmRaceSelectMenuHandle, hMenuXMLDesc, strButtonCtrlName.c_str(),
							(void*)(itRaceManType - vecRaceManTypes.begin()),
							rmOnSelectRaceMan);

		// Look for sub-types : if any, we have a sub-type combo box for this type.
		bool bCreateCombo = false;
		std::vector<GfRaceManager*>::const_iterator itRaceMan;
		for (itRaceMan = vecRaceMans.begin(); itRaceMan != vecRaceMans.end(); itRaceMan++)
		{
			if (!(*itRaceMan)->getSubType().empty())
			{
				bCreateCombo = true;
				break;
			}
		}

		if (!bCreateCombo)
			continue;
		
		// Create the race manager sub-type combo-box.
		std::string strComboCtrlName(*itRaceManType);
		strComboCtrlName.erase(std::remove(strComboCtrlName.begin(), strComboCtrlName.end(), ' '), strComboCtrlName.end()); // Such a pain to remove spaces !
		strComboCtrlName += "Combo";
		rmMapSubTypeComboIds[*itRaceManType] =
			GfuiMenuCreateComboboxControl(RmRaceSelectMenuHandle, hMenuXMLDesc,
								  strComboCtrlName.c_str(), 0, rmOnChangeRaceMan);

		// Add one item in the combo for each race manager of this type.
		for (itRaceMan = vecRaceMans.begin(); itRaceMan != vecRaceMans.end(); itRaceMan++)
		{
			GfuiComboboxAddText(RmRaceSelectMenuHandle, rmMapSubTypeComboIds[*itRaceManType],
								(*itRaceMan)->getSubType().c_str());
		}

		// Select the first one by default.
		GfuiComboboxSetPosition(RmRaceSelectMenuHandle, rmMapSubTypeComboIds[*itRaceManType], 0);

		// Disable combo if only one race manager.
		if (vecRaceMans.size() == 1)
			GfuiEnable(RmRaceSelectMenuHandle, rmMapSubTypeComboIds[*itRaceManType], GFUI_DISABLE);
	}
	
    // Create Back button
    GfuiMenuCreateButtonControl(RmRaceSelectMenuHandle, hMenuXMLDesc, "BackButton",
								prevMenu, rmOnRaceSelectShutdown);

    // Close menu XML descriptor.
    GfParmReleaseHandle(hMenuXMLDesc);
    
    // Register keyboard shortcuts.
    GfuiMenuDefaultKeysAdd(RmRaceSelectMenuHandle);
    GfuiAddKey(RmRaceSelectMenuHandle, GFUIK_ESCAPE, "Back To Main Menu",
			   prevMenu, rmOnRaceSelectShutdown, NULL);

    // Give the race engine the menu to come back to.
    LmRaceEngine().initializeState(RmRaceSelectMenuHandle);

    return RmRaceSelectMenuHandle;
}
