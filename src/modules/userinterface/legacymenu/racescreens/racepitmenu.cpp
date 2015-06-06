/***************************************************************************

    file                 : racepitmenu.cpp
    created              : Mon Apr 24 18:16:37 CEST 2000
    copyright            : (C) 2000 by Eric Espie
    email                : torcs@free.fr
    version              : $Id: racepitmenu.cpp 5042 2012-11-11 15:38:27Z pouillot $

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
    		Pit menu for human drivers
    @ingroup	racemantools
    @author	<a href=mailto:torcs@free.fr>Eric Espie</a>
    @version	$Id: racepitmenu.cpp 5042 2012-11-11 15:38:27Z pouillot $
*/

#include <cstdlib>

#include <portability.h>

#include <tgfclient.h>

#include <car.h>
#include <isoundengine.h>
#include <raceman.h>

#include "legacymenu.h"
#include "racescreens.h"


static void		*menuHandle = NULL;
static int		fuelId;
static int		repairId;
static tCarElt		*rmCar;
static tfuiCallback rmCallback;


static void
rmUpdtFuel(void * /* dummy */)
{
    char	*val;
    char	buf[32];
    
    val = GfuiEditboxGetString(menuHandle, fuelId);
    rmCar->pitcmd.fuel = (tdble)strtod(val, (char **)NULL);
    sprintf(buf, "%.1f", rmCar->pitcmd.fuel);
    GfuiEditboxSetString(menuHandle, fuelId, buf);
}

static void
rmUpdtRepair(void * /* dummy */)
{
    char	*val;
    char	buf[32];
    
    val = GfuiEditboxGetString(menuHandle, repairId);
    rmCar->pitcmd.repair = strtol(val, (char **)NULL, 0);
    sprintf(buf, "%d", rmCar->pitcmd.repair);
    GfuiEditboxSetString(menuHandle, repairId, buf);
}

static void
rmStopAndGo(void * /* dummy */)
{
    rmCar->_pitStopType = RM_PIT_STOPANDGO;
    rmCallback(rmCar);
}

static void
rmRepair(void* /* dummy */)
{
	rmCar->_pitStopType = RM_PIT_REPAIR;
	rmCallback(rmCar);
}


/**
 * This function shows the pit menu and let the user fill in the amount
 * of fuel he wants and the number of damage he want to repair
 *
 * @param car The current car (pitcmd is modified on user decisions)
 * @param s The current situation (used to calculate the remaining time)
 * @param callback The function which is called after the user made a decision
 */
void
RmPitMenuStart(tCarElt *car, tSituation *s, tfuiCallback callback)
{
    char buf[128];

    rmCar = car;
    rmCallback = callback;

    if (menuHandle)
        GfuiScreenRelease(menuHandle);

	GfLogInfo("Entering Pit menu\n");

    // Create screen, load menu XML descriptor and create static controls.
    menuHandle = GfuiScreenCreate(NULL, NULL, NULL, NULL, NULL, 1);

    void *menuXMLDescHdle = GfuiMenuLoad("pitmenu.xml");

    GfuiMenuCreateStaticControls(menuHandle, menuXMLDescHdle);

    // Create variable title label.
    int titleId = GfuiMenuCreateLabelControl(menuHandle, menuXMLDescHdle, "titlelabel");
    snprintf(buf, sizeof(buf), "Pit Stop for %s", car->_name);
    GfuiLabelSetText(menuHandle, titleId, buf);

   // Create labels for remaining laps and remaining fuel.
    int remainLapsTimeId = GfuiMenuCreateLabelControl(menuHandle, menuXMLDescHdle, "remaininglapstimelabel");
    if( s->_totTime > 0 && s->_totTime > s->currentTime ) // Timed part of the timed session
    {
		GfuiMenuCreateLabelControl(menuHandle, menuXMLDescHdle, "remainingtimelabel");
    	if( s->_extraLaps > 0)
    	    snprintf(buf, sizeof(buf), "%s + %d laps", GfTime2Str( s->_totTime - s->currentTime, NULL, true, 0 ), s->_extraLaps);
		else
    	    snprintf(buf, sizeof(buf), "%s", GfTime2Str( s->_totTime - s->currentTime, NULL, true, 0 ) );
    }
    else
    {
		GfuiMenuCreateLabelControl(menuHandle, menuXMLDescHdle, "remaininglapslabel");
    	snprintf(buf, sizeof(buf), "%d", car->_remainingLaps); //Laps tot drive to win the race
    }
    GfuiLabelSetText(menuHandle, remainLapsTimeId, buf);

    int remainFuelId = GfuiMenuCreateLabelControl(menuHandle, menuXMLDescHdle, "remainingfuellabel");
    snprintf(buf, sizeof(buf), "%.1f", car->_fuel);
    GfuiLabelSetText(menuHandle, remainFuelId, buf);

    // Create edit boxes for fuel and repair amounts.
    fuelId = GfuiMenuCreateEditControl(menuHandle, menuXMLDescHdle, "fuelamountedit", NULL, NULL, rmUpdtFuel);
    snprintf(buf, sizeof(buf), "%.1f", car->pitcmd.fuel);
    GfuiEditboxSetString(menuHandle, fuelId, buf);

    repairId = GfuiMenuCreateEditControl(menuHandle, menuXMLDescHdle, "repairamountedit", NULL, NULL, rmUpdtRepair);
    snprintf(buf, sizeof(buf), "%d", (int)car->pitcmd.repair);
    GfuiEditboxSetString(menuHandle, repairId, buf);

    // Create Repair and Stop&Go buttons.
    GfuiMenuCreateButtonControl(menuHandle, menuXMLDescHdle, "repairbutton", NULL, rmRepair);
    GfuiMenuCreateButtonControl(menuHandle, menuXMLDescHdle, "stopgobutton", NULL, rmStopAndGo);

    // Close menu XML descriptor.
    GfParmReleaseHandle(menuXMLDescHdle);
    
    // Register keyboard shortcuts.
    GfuiMenuDefaultKeysAdd(menuHandle);

    // Activate the created screen.
    GfuiScreenActivate(menuHandle);
}

// Pit menu return callback.
static void
rmOnBackFromPitMenu(void *pvcar)
{
	const tCarElt* car = (tCarElt*)pvcar;
	LmRaceEngine().setPitCommand(car->index, &car->pitcmd);

	LegacyMenu::self().activateGameScreen();
}

bool
RmCheckPitRequest()
{
	// If one (human) driver is in pit, switch the display loop to the pit menu.
	if (LmRaceEngine().outData()->_rePitRequester)
	{
		// Mute sound.
		if (LegacyMenu::self().soundEngine())
			LegacyMenu::self().soundEngine()->mute(true);
		
		// TODO pit music??

		// First, stop the race engine.
		LmRaceEngine().stop();

		// Then open the pit menu (will return in ReCarsUpdateCarPitCmd).
		RmPitMenuStart(LmRaceEngine().outData()->_rePitRequester, LmRaceEngine().outData()->s, rmOnBackFromPitMenu);

		return true;
	}

	return false;
}
