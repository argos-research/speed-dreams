/***************************************************************************

    file                 : raceparamsmenu.cpp
    created              : Thu May  2 22:02:51 CEST 2002
    copyright            : (C) 2001 by Eric Espie
    email                : eric.espie@torcs.org
    version              : $Id: raceparamsmenu.cpp 5696 2013-10-01 22:15:36Z torcs-ng $

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
    		This is the race options menu.
    @ingroup	racemantools
    @author	<a href=mailto:eric.espie@torcs.org>Eric Espie</a>
    @version	$Id: raceparamsmenu.cpp 5696 2013-10-01 22:15:36Z torcs-ng $
*/

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <sstream>

#include <portability.h>
#include <tgfclient.h>

#include <track.h>
#include <car.h>
#include <raceman.h>
#include <robot.h>
#include <graphic.h>

#include <racemanagers.h>
#include <race.h>

#include "racescreens.h"


// Constants.
static const int NDisplayModeNumber = 2;
static const char *DispModeValues[NDisplayModeNumber] = { RM_VAL_INVISIBLE, RM_VAL_VISIBLE};
static const char *TimeOfDayValues[GfRace::nTimeSpecNumber] = RM_VALS_TIME;
static const char* CloudsValues[GfRace::nCloudsSpecNumber] = RM_VALS_CLOUDS;
static const char *RainValues[GfRace::nRainSpecNumber] = RM_VALS_RAIN;

// Global variables.
static void		*ScrHandle;
static tRmRaceParam	*MenuData;

// Menu control ids
static int		rmrpDistEditId;
static int		rmrpLapsEditId;
static int		rmrpDurationEditId;
static int		rmrpDispModeEditId;
static int		rmrpCloudsEditId, rmrpCloudsLeftArrowId, rmrpCloudsRightArrowId;
static int		rmrpTimeOfDayEditId;
static int		rmrpRainEditId;

// Race params
static unsigned rmrpConfMask;
static int		rmrpDistance;
static int		rmrpLaps;
static int		rmrpDuration;
static unsigned		rmrpDispMode;
static GfRace::ECloudsSpec		rmrpClouds;
static GfRace::ETimeOfDaySpec	rmrpTimeOfDay;
static GfRace::ERainSpec		rmrpRain;

static int		rmrpFeatures;
static bool rmrpSessionIsRace;
static int rmrpFallbackDistance;
static int rmrpExtraLaps;


static void
rmrpDeactivate(void *screen)
{
    GfuiScreenRelease(ScrHandle);
    
    if (screen) {
		GfuiScreenActivate(screen);
    }
}

static void
rmrpUpdDist(void * /* dummy */)
{
    char	buf[32];
    char	*val;

    val = GfuiEditboxGetString(ScrHandle, rmrpDistEditId);
    rmrpDistance = strtol(val, (char **)NULL, 0);
    if (rmrpDistance == 0) {
		strcpy(buf, "---");
    } else {
		snprintf(buf, sizeof(buf), "%d", rmrpDistance);
		rmrpLaps = 0;
		GfuiEditboxSetString(ScrHandle, rmrpLapsEditId, "---");
		if (rmrpFeatures & RM_FEATURE_TIMEDSESSION) {
			rmrpDuration = 0;
			GfuiEditboxSetString(ScrHandle, rmrpDurationEditId, "---");
		}
    }
    GfuiEditboxSetString(ScrHandle, rmrpDistEditId, buf);
}

static void
rmrpUpdLaps(void * /* dummy */)
{
    char	buf[32];
    char	*val;

    val = GfuiEditboxGetString(ScrHandle, rmrpLapsEditId);
    rmrpLaps = strtol(val, (char **)NULL, 0);
    if (rmrpLaps == 0) {
		strcpy(buf, "---");
    } else {
		snprintf(buf, sizeof(buf), "%d", rmrpLaps);
		rmrpDistance = 0;
		GfuiEditboxSetString(ScrHandle, rmrpDistEditId, "---");
		if ((rmrpFeatures & RM_FEATURE_TIMEDSESSION) && !rmrpSessionIsRace) {
			rmrpDuration = 0;
			GfuiEditboxSetString(ScrHandle, rmrpDurationEditId, "---");
		}
    }
    GfuiEditboxSetString(ScrHandle, rmrpLapsEditId, buf);
}

static void
rmrpUpdDuration(void * /*dummy*/)
{
    char buf[64];
    char *val;
    int nbSep = 0;
    int subresult = 0;
    int result = 0;

    val = GfuiEditboxGetString(ScrHandle, rmrpDurationEditId);
    
    while( true )
    {
		if (val[0] >= '0' && val[0] <= '9')
		{
			subresult *= 10;
			subresult += val[0] - '0';
		}
		else if (val[0] == ':')
		{
			if (nbSep == 0 || subresult < 60)
			{
				result *= 60;
				result += subresult;
				subresult = 0;
				++nbSep;
			}
			else
			{
				result = 0;
				break;
			}
		}
		else
		{
			break;
		}

		++val;
    }

    if (nbSep == 0 || subresult < 60)
    {
		result *= 60;
		result += subresult;
    }
    else
    {
		result = 0;
    }
    
    rmrpDuration = result;
    
    if (rmrpDuration <= 0) {
		strcpy( buf, "---");
    } else {
		snprintf(buf, sizeof(buf), "%d:%02d:%02d",
				 (int)floor((float)rmrpDuration / 3600.0f),
				 (int)floor((float)rmrpDuration / 60.0f) % 60,
				 (int)floor((float)rmrpDuration) % 60);
		rmrpDistance = 0;
		GfuiEditboxSetString(ScrHandle, rmrpDistEditId, "---");
		if (!rmrpSessionIsRace) {rmrpLaps = 0;
			GfuiEditboxSetString(ScrHandle, rmrpLapsEditId, "---");
		}
    }
    GfuiEditboxSetString(ScrHandle, rmrpDurationEditId, buf);
}

static void
rmChangeDisplayMode(void *vp)
{
    const long delta = (int)(long)vp;
    rmrpDispMode = 
		(rmrpDispMode + NDisplayModeNumber + delta) % NDisplayModeNumber;
    GfuiLabelSetText(ScrHandle, rmrpDispModeEditId, DispModeValues[rmrpDispMode]);
}

static void
rmChangeTimeOfDay(void *vp)
{
	const long delta = (int)(long)vp;
	rmrpTimeOfDay =
		(GfRace::ETimeOfDaySpec)
		((rmrpTimeOfDay + GfRace::nTimeSpecNumber + delta) % GfRace::nTimeSpecNumber);
	GfuiLabelSetText(ScrHandle, rmrpTimeOfDayEditId, TimeOfDayValues[rmrpTimeOfDay]);
}

static void rmChangeRain(void *vp);

static void
rmChangeClouds(void *vp)
{
    const long delta = (int)(long)vp;
    rmrpClouds =
		(GfRace::ECloudsSpec)
		((rmrpClouds + GfRace::nCloudsSpecNumber + delta) % GfRace::nCloudsSpecNumber);
    GfuiLabelSetText(ScrHandle, rmrpCloudsEditId, CloudsValues[rmrpClouds]);
}

static void
rmChangeRain(void *vp)
{
	const long delta = (int)(long)vp;
	rmrpRain =
		(GfRace::ERainSpec)
		((rmrpRain + GfRace::nRainSpecNumber + delta) % GfRace::nRainSpecNumber);
	GfuiLabelSetText(ScrHandle, rmrpRainEditId, RainValues[rmrpRain]);

    if (rmrpConfMask & RM_CONF_CLOUD_COVER)
	{
		// Make clouds state compatible if needed.
		int cloudsComboEnabled = GFUI_ENABLE;
		if (rmrpRain == GfRace::eRainRandom) // Random rain => Random clouds.
		{
			cloudsComboEnabled = GFUI_DISABLE;
			rmrpClouds = GfRace::eCloudsRandom;
		}
		/*else if (rmrpRain == GfRace::eRainReal) // Real rain => Real clouds.
		{
			cloudsComboEnabled = GFUI_DISABLE;
			rmrpClouds = GfRace::eCloudsReal;
		}*/
		else if (rmrpRain != GfRace::eRainNone)
		{
			cloudsComboEnabled = GFUI_DISABLE;
			rmrpClouds = GfRace::eCloudsFull; // Rain => Heavy clouds.
		}
		
		GfuiLabelSetText(ScrHandle, rmrpCloudsEditId, CloudsValues[rmrpClouds]);
		
		// Show / hide clouds combo arrow buttons (any rain => no sky choice).
		GfuiEnable(ScrHandle, rmrpCloudsLeftArrowId, cloudsComboEnabled);
		GfuiEnable(ScrHandle, rmrpCloudsRightArrowId, cloudsComboEnabled);
	}
}

static void
rmrpValidate(void * /* dummy */)
{
    // Force current edit to loose focus (if one has it) and update associated variable.
    GfuiUnSelectCurrent();

	// And then update configurable race session parameters from the current local settings,
	// if anything to configure (Don't change non-configurable parameters).
	GfRace::Parameters* pRaceSessionParams =
		MenuData->pRace->getParameters(MenuData->session);
	if (pRaceSessionParams && pRaceSessionParams->bfOptions)
	{
		if (rmrpConfMask & RM_CONF_RACE_LEN)
		{
			//in order to not delete hidden settings from the file, write back:
			//fallback distance to nDistance when no distance set by user
			if ((rmrpDistance == 0) && (rmrpFallbackDistance > 0))
				pRaceSessionParams->nDistance = rmrpFallbackDistance;
			else pRaceSessionParams->nDistance = rmrpDistance;
			//extra laps to nLaps when no lap number set by user
			if ((rmrpLaps == 0) && (rmrpExtraLaps > 0))
				pRaceSessionParams->nLaps = rmrpExtraLaps;
			else pRaceSessionParams->nLaps = rmrpLaps;
			if (rmrpFeatures & RM_FEATURE_TIMEDSESSION)
				pRaceSessionParams->nDuration = rmrpDuration;
		}
	
		if (rmrpConfMask & RM_CONF_TIME_OF_DAY)
		{
			pRaceSessionParams->eTimeOfDaySpec = (GfRace::ETimeOfDaySpec)rmrpTimeOfDay;
		}
	
		if (rmrpConfMask & RM_CONF_CLOUD_COVER)
		{
			pRaceSessionParams->eCloudsSpec = (GfRace::ECloudsSpec)rmrpClouds;
		}
	
		if (rmrpConfMask & RM_CONF_RAIN_FALL)
		{
			pRaceSessionParams->eRainSpec = (GfRace::ERainSpec)rmrpRain;
		}

		if (rmrpConfMask & RM_CONF_DISP_MODE)
		{
			pRaceSessionParams->bfDisplayMode = rmrpDispMode;
		}
	}
	
    rmrpDeactivate(MenuData->nextScreen);
}

static void
rmrpAddKeys(void)
{
    GfuiAddKey(ScrHandle, GFUIK_RETURN, "Accept", NULL, rmrpValidate, NULL);
    GfuiAddKey(ScrHandle, GFUIK_ESCAPE, "Cancel", MenuData->prevScreen, rmrpDeactivate, NULL);
    GfuiAddKey(ScrHandle, GFUIK_F1, "Help", ScrHandle, GfuiHelpScreen, NULL);
    GfuiAddKey(ScrHandle, GFUIK_F12, "Screen-Shot", NULL, GfuiScreenShot, NULL);
}

void
RmRaceParamsMenu(void *vrp)
{
    char buf[256];

	MenuData = (tRmRaceParam*)vrp;

	GfLogTrace("Entering %s Params menu for %s\n",
			   MenuData->session.c_str(), MenuData->pRace->getManager()->getName().c_str());

	//clear previous values
	rmrpFallbackDistance = 0;
	rmrpExtraLaps = 0;
	// Update the conf mask according to the session params, graphics options and race features.
	// 1) According to the availability of parameters for this session,.
	GfRace::Parameters* pRaceSessionParams =
		MenuData->pRace->getParameters(MenuData->session);
	rmrpConfMask = pRaceSessionParams ? pRaceSessionParams->bfOptions : 0;
	
	// 2) According to SkyDome settings.
	snprintf(buf, sizeof(buf), "%s%s", GfLocalDir(), GR_PARAM_FILE);
	void *grHandle = GfParmReadFile(buf, GFPARM_RMODE_STD);
	const bool bSkyDomeEnabled =
		(int)GfParmGetNum(grHandle, GR_SCT_GRAPHIC, GR_ATT_SKYDOMEDISTANCE, NULL, 0) != 0;
	GfParmReleaseHandle(grHandle);

	if (!bSkyDomeEnabled && (rmrpConfMask & RM_CONF_TIME_OF_DAY))
	{
		GfLogTrace("Will not configure Time of Day as Sky Dome is disabled\n");
		rmrpConfMask &= ~RM_CONF_TIME_OF_DAY;
	}
	if (!bSkyDomeEnabled && (rmrpConfMask & RM_CONF_CLOUD_COVER))
	{
		GfLogTrace("Will not configure Cloud Cover as Sky Dome is disabled\n");
		rmrpConfMask &= ~RM_CONF_CLOUD_COVER;
	}

	// 3) According to the race features.
    rmrpFeatures = MenuData->pRace->getSupportedFeatures();

	if (!(rmrpFeatures & RM_FEATURE_WETTRACK) && (rmrpConfMask & RM_CONF_RAIN_FALL))
	{
		GfLogTrace("Will not configure Rain Fall as some drivers don't support wet track\n");
		rmrpConfMask &= ~RM_CONF_RAIN_FALL;
	}

	// 4) According to the competitors.
	if ((rmrpConfMask & RM_CONF_DISP_MODE) && MenuData->pRace->hasHumanCompetitors())
	{
		GfLogTrace("Will not configure Display Mode as some human driver(s) are in\n");
		rmrpConfMask &= ~RM_CONF_DISP_MODE;
	}
	
    // Create the screen, load menu XML descriptor and create static controls.
    ScrHandle = GfuiScreenCreate((float*)NULL, NULL, NULL, NULL, (tfuiCallback)NULL, 1);   
    void *menuXMLDescHdle = GfuiMenuLoad("raceparamsmenu.xml");
    GfuiMenuCreateStaticControls(ScrHandle, menuXMLDescHdle);

    // Create the variable title label.
    int titleId = GfuiMenuCreateLabelControl(ScrHandle, menuXMLDescHdle, "titlelabel");
	std::string strTitle(MenuData->session);
	strTitle += " Options";
    GfuiLabelSetText(ScrHandle, titleId, strTitle.c_str());

	// Create the "nothing to configure here" label if actually nothing configurable.
	if (!rmrpConfMask)
	{
		GfuiMenuCreateLabelControl(ScrHandle, menuXMLDescHdle, "nooptionlabel");
	}
	
    // Otherwise, create and initialize the race length configuration controls.
    rmrpSessionIsRace = true; //TODO: when available, get it from pRaceSessionParam
    if (rmrpConfMask & RM_CONF_RACE_LEN) 
    {
		if (pRaceSessionParams->nDistance < 0)
			rmrpDistance = 0; // Default value.
		else
			rmrpDistance = pRaceSessionParams->nDistance;

		if (pRaceSessionParams->nDuration < 0)
			rmrpDuration = 0; // Default value.
		else
			rmrpDuration = pRaceSessionParams->nDuration;
		//if the session is timed, save the fallback distance, and unset rmrpDistance
		if ( (rmrpFeatures & RM_FEATURE_TIMEDSESSION) && (rmrpDuration > 0) && (rmrpDistance > 0) )
		{
			rmrpFallbackDistance = rmrpDistance;
			rmrpDistance = 0;
		}
		if (rmrpDuration > 0 && !(rmrpFeatures & RM_FEATURE_TIMEDSESSION) && rmrpDistance == 0) {
			if (rmrpSessionIsRace) rmrpDistance = (int)((float)rmrpDuration * 2.5 / 60);//calculate with 150 km/h
			else rmrpDistance = (int)((float)rmrpDuration / 60);//use 1 km/minute
		}

		if (pRaceSessionParams->nLaps < 0)
			rmrpLaps = 0; // Default value.
		else {
			rmrpLaps = pRaceSessionParams->nLaps;
			if ((!rmrpSessionIsRace) && ((rmrpDistance > 0) || (rmrpDuration > 0)))
				rmrpLaps = 0;
			if (rmrpDuration > 0 && !(rmrpFeatures & RM_FEATURE_TIMEDSESSION))
			{
			    rmrpExtraLaps = rmrpLaps;
			    rmrpLaps = 0;
			}
		}
		if (rmrpDistance == 0 && rmrpLaps == 0 && rmrpDuration == 0)
			rmrpLaps = 1;

		// Create Race distance label.
		GfuiMenuCreateLabelControl(ScrHandle, menuXMLDescHdle, "distancelabel");
		
		// Create and initialize Race distance edit.
		if (rmrpDistance == 0) 
		{
			strcpy(buf, "---");
		} 
		else 
		{
			snprintf(buf, sizeof(buf), "%d", rmrpDistance);
			rmrpLaps = 0;
		}
		
		rmrpDistEditId = GfuiMenuCreateEditControl(ScrHandle, menuXMLDescHdle, "distanceedit",
										   NULL, NULL, rmrpUpdDist);
		GfuiEditboxSetString(ScrHandle,rmrpDistEditId,buf);
		
		// Create Laps label.
		GfuiMenuCreateLabelControl(ScrHandle, menuXMLDescHdle, "lapslabel");
		
		// Create and initialize Laps edit.
		if (rmrpLaps == 0) 
		{
			strcpy(buf, "---");
		} 
		else 
		{
			snprintf(buf, sizeof(buf), "%d", rmrpLaps);
		}
		
		rmrpLapsEditId = GfuiMenuCreateEditControl(ScrHandle, menuXMLDescHdle, "lapsedit",
										   NULL, NULL, rmrpUpdLaps);
		GfuiEditboxSetString(ScrHandle,rmrpLapsEditId,buf);
		
		if (rmrpFeatures & RM_FEATURE_TIMEDSESSION)
		{
			// Create Session time label.
			GfuiMenuCreateLabelControl(ScrHandle, menuXMLDescHdle, "durationlabel");
			
			// Create and initialize Session time edit.
			if (rmrpDuration <= 0) 
			{
				strcpy(buf, "---");
			}
			else 
			{
				snprintf(buf, sizeof(buf), "%d:%02d:%02d",
						 (int)floor((float) rmrpDuration / 3600.0f ),
						 (int)floor( (float)rmrpDuration / 60.0f ) % 60,
						 (int)floor( (float)rmrpDuration ) % 60 );
			}
			
			rmrpDurationEditId =
				GfuiMenuCreateEditControl(ScrHandle, menuXMLDescHdle, "durationedit",
								  NULL, NULL, rmrpUpdDuration);
			GfuiEditboxSetString(ScrHandle,rmrpDurationEditId,buf);
		}
    }

    // Create and initialize Time of day combo box (2 arrow buttons and a variable label).
	if (rmrpConfMask & RM_CONF_TIME_OF_DAY)
	{
		if (pRaceSessionParams->eTimeOfDaySpec == GfRace::nTimeSpecNumber)
			rmrpTimeOfDay = GfRace::eTimeAfternoon; // Default value.
		else
			rmrpTimeOfDay = pRaceSessionParams->eTimeOfDaySpec;
		
		// Create Time of day label.
		GfuiMenuCreateLabelControl(ScrHandle,menuXMLDescHdle,"timeofdaylabel");

		// Create and initialize Time of day combo-box-like control.
		GfuiMenuCreateButtonControl(ScrHandle, menuXMLDescHdle, "timeofdayleftarrow",
							(void*)-1, rmChangeTimeOfDay);
		GfuiMenuCreateButtonControl(ScrHandle, menuXMLDescHdle, "timeofdayrightarrow",
							(void*)1, rmChangeTimeOfDay);
			
		rmrpTimeOfDayEditId = GfuiMenuCreateLabelControl(ScrHandle, menuXMLDescHdle, "timeofdayedit");
		GfuiLabelSetText(ScrHandle, rmrpTimeOfDayEditId, TimeOfDayValues[rmrpTimeOfDay]);
    }
	
	// Create and initialize Clouds combo box (2 arrow buttons and a variable label).
    if (rmrpConfMask & RM_CONF_CLOUD_COVER)
	{
		if (pRaceSessionParams->eCloudsSpec == GfRace::nCloudsSpecNumber)
			rmrpClouds = GfRace::eCloudsNone; // Default value.
		else
			rmrpClouds = pRaceSessionParams->eCloudsSpec;
			
		// Create Cloud cover label.
		GfuiMenuCreateLabelControl(ScrHandle, menuXMLDescHdle, "cloudslabel");

		// Create and initialize Cloud cover combo-box-like control.
		rmrpCloudsLeftArrowId =
			GfuiMenuCreateButtonControl(ScrHandle, menuXMLDescHdle, "cloudsleftarrow",
								(void*)-1, rmChangeClouds);
		rmrpCloudsRightArrowId =
			GfuiMenuCreateButtonControl(ScrHandle, menuXMLDescHdle, "cloudsrightarrow",
								(void*)+1, rmChangeClouds);
			
		rmrpCloudsEditId = GfuiMenuCreateLabelControl(ScrHandle, menuXMLDescHdle, "cloudsedit");
		GfuiLabelSetText(ScrHandle, rmrpCloudsEditId, CloudsValues[rmrpClouds]);
	}
	
	// Create and initialize Rain combo box (2 arrow buttons and a variable label).
	if ((rmrpConfMask & RM_CONF_RAIN_FALL) && (rmrpFeatures & RM_FEATURE_WETTRACK))
	{
		if (pRaceSessionParams->eRainSpec == GfRace::nRainSpecNumber)
			rmrpRain = GfRace::eRainNone; // Default value.
		else
			rmrpRain = pRaceSessionParams->eRainSpec;
			
		// Create Rain label.
		GfuiMenuCreateLabelControl(ScrHandle, menuXMLDescHdle, "rainlabel");

		// Create and initialize Rain combo-box-like control.
		GfuiMenuCreateButtonControl(ScrHandle, menuXMLDescHdle, "rainleftarrow",
							(void*)-1, rmChangeRain);
		GfuiMenuCreateButtonControl(ScrHandle, menuXMLDescHdle, "rainrightarrow",
							(void*)1, rmChangeRain);
			
		rmrpRainEditId = GfuiMenuCreateLabelControl(ScrHandle, menuXMLDescHdle, "rainedit");
		GfuiLabelSetText(ScrHandle, rmrpRainEditId, RainValues[rmrpRain]);
			
		rmChangeRain(0); // Make cloud cover settings compatible if needed.
	}
	
	// Create and initialize Display mode combo-box-like control.
    if (rmrpConfMask & RM_CONF_DISP_MODE) 
    {
		if (pRaceSessionParams->bfDisplayMode == RM_DISP_MODE_UNDEFINED)
			rmrpDispMode = RM_DISP_MODE_NORMAL; // Default value.
		else
			rmrpDispMode = pRaceSessionParams->bfDisplayMode & RM_DISP_MODE_NORMAL;

		// Create Display mode label.
		GfuiMenuCreateLabelControl(ScrHandle, menuXMLDescHdle, "displaylabel");

		// Create and initialize Display mode combo-box-like control.
		GfuiMenuCreateButtonControl(ScrHandle, menuXMLDescHdle, "displayleftarrow",
							(void*)-1, rmChangeDisplayMode);
		GfuiMenuCreateButtonControl(ScrHandle, menuXMLDescHdle, "displayrightarrow",
							(void*)+1, rmChangeDisplayMode);
		rmrpDispModeEditId = GfuiMenuCreateLabelControl(ScrHandle, menuXMLDescHdle, "displayedit");
		GfuiLabelSetText(ScrHandle, rmrpDispModeEditId, DispModeValues[rmrpDispMode]);
    }
	
    // Create Accept and Cancel buttons
    GfuiMenuCreateButtonControl(ScrHandle, menuXMLDescHdle, "nextbutton", NULL, rmrpValidate);
    GfuiMenuCreateButtonControl(ScrHandle, menuXMLDescHdle, "backbutton",
								MenuData->prevScreen, rmrpDeactivate);
    
    // Close menu XML descriptor.
    GfParmReleaseHandle(menuXMLDescHdle);
    
    // Register keyboard shortcuts.
    rmrpAddKeys();
    
    GfuiScreenActivate(ScrHandle);
}
