/***************************************************************************

    file        : racenexteventmenu.cpp
    created     : Fri Jan  3 22:24:41 CET 2003
    copyright   : (C) 2003 by Eric Espie                        
    email       : eric.espie@torcs.org   
    version     : $Id: racenexteventmenu.cpp 4481 2012-02-05 19:29:37Z pouillot $

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
    		The Next Event menu (where one sees which track the next event is run on)
    @author	<a href=mailto:eric.espie@torcs.org>Eric Espie</a>
    @version	$Id: racenexteventmenu.cpp 4481 2012-02-05 19:29:37Z pouillot $
*/

#include <cstdlib>
#include <cstdio>

#include <portability.h>

#include <raceman.h>
#include <tgfclient.h>

#include <race.h>
#include <racemanagers.h>

#include "legacymenu.h"
#include "racescreens.h"


// New track menu.
static void	*rmScrHandle = NULL;

static void
rmStateManage(void * /* dummy */)
{
	LmRaceEngine().updateState();
}

void
RmNextEventMenu(void)
{
	char buf[128];

	tRmInfo* reInfo = LmRaceEngine().inData();
	void	*params = reInfo->params;
	void	*results = reInfo->results;
	int		raceNumber;
	int		 xx;

	if (rmScrHandle) {
		GfuiScreenRelease(rmScrHandle);
	}

	GfLogTrace("Entering Next Event menu\n");
	
	// Create screen, load menu XML descriptor and create static controls.
	rmScrHandle = GfuiScreenCreate(NULL, 
								  NULL, (tfuiCallback)NULL, 
								  NULL, (tfuiCallback)NULL, 
								  1);
	void *menuXMLDescHdle = GfuiMenuLoad("racenexteventmenu.xml");
	GfuiMenuCreateStaticControls(rmScrHandle, menuXMLDescHdle);

	// Create background image from race params.
	const char* pszBGImg = GfParmGetStr(params, RM_SECT_HEADER, RM_ATTR_BGIMG, 0);
	if (pszBGImg) {
		GfuiScreenAddBgImg(rmScrHandle, pszBGImg);
	}

	// Create variable title label from race params.
	int titleId = GfuiMenuCreateLabelControl(rmScrHandle, menuXMLDescHdle, "TitleLabel");
	char pszTitle[128];
	if (LmRaceEngine().race()->getManager()->hasSubFiles())
	{
		const char* pszGroup = GfParmGetStr(reInfo->params, RM_SECT_HEADER, RM_ATTR_NAME, "<no group>");
		snprintf(pszTitle, sizeof(pszTitle), "%s - %s", reInfo->_reName, pszGroup);
	}
	else
		snprintf(pszTitle, sizeof(pszTitle), "%s", reInfo->_reName);
	GfuiLabelSetText(rmScrHandle, titleId, pszTitle);

	// Calculate which race of the series this is
	raceNumber = 1;
	for (xx = 1; xx < (int)GfParmGetNum(results, RE_SECT_CURRENT, RE_ATTR_CUR_TRACK, NULL, 1); ++xx) 
	{
		snprintf(buf, sizeof(buf), "%s/%d", RM_SECT_TRACKS, xx);
		if (!strcmp( GfParmGetStr(reInfo->params, buf, RM_ATTR_NAME, "free"), "free") == 0)
			++raceNumber;
	}

	// Create variable subtitle label from race params.
	snprintf(buf, sizeof(buf), "Race Day #%d/%d at %s",
			 raceNumber,
			 (int)GfParmGetNum(params, RM_SECT_TRACKS, RM_ATTR_NUMBER, NULL, -1 ) >= 0
			 ? (int)GfParmGetNum(params, RM_SECT_TRACKS, RM_ATTR_NUMBER, NULL, -1 )
			 : GfParmGetEltNb(params, RM_SECT_TRACKS), 
			 reInfo->track->name);
	int subTitleId = GfuiMenuCreateLabelControl(rmScrHandle, menuXMLDescHdle, "SubTitleLabel");
	GfuiLabelSetText(rmScrHandle, subTitleId, buf);

	// Create Start and Abandon buttons.
	GfuiMenuCreateButtonControl(rmScrHandle, menuXMLDescHdle, "StartButton", NULL, rmStateManage);
	GfuiMenuCreateButtonControl(rmScrHandle, menuXMLDescHdle, "AbandonButton", RmRaceSelectMenuHandle, GfuiScreenActivate);

	// Close menu XML descriptor.
	GfParmReleaseHandle(menuXMLDescHdle);
	
	// Register keyboard shortcuts.
	GfuiMenuDefaultKeysAdd(rmScrHandle);
	GfuiAddKey(rmScrHandle, GFUIK_RETURN, "Start Event", NULL, rmStateManage, NULL);
	GfuiAddKey(rmScrHandle, GFUIK_ESCAPE, "Abandon", RmRaceSelectMenuHandle, GfuiScreenActivate, NULL);

	// Activate screen.
	GfuiScreenActivate(rmScrHandle);
}

