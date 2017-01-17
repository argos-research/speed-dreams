/***************************************************************************

    file                 : raceloadingmenu.cpp
    created              : Sun Feb 25 00:34:46 2001
    copyright            : (C) 2000 by Eric Espie
    email                : eric.espie@torcs.org
    version              : $Id: raceloadingmenu.cpp 6270 2015-11-23 19:44:40Z madbad $

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
    		The menu for when the race is running
    @ingroup	racemantools		
    @author	<a href=mailto:eric.espie@torcs.org>Eric Espie</a>
    @version	$Id: raceloadingmenu.cpp 6270 2015-11-23 19:44:40Z madbad $
*/

#include <cstring>
#include <cstdlib>

#include <tgfclient.h>

#include <car.h>

#include "racescreens.h"


static int NTextLines = 0;

static float BGColor[4] = {0.0, 0.0, 0.0, 0.0};

static void* HScreen = 0;

static float** FGColors = 0;
static int*	TextLineIds = 0;
static char** TextLines = 0;
static int CurTextLineIdx;


/** 
    @ingroup	racemantools
    @param	title	Screen title.
    @param	bgimg	Optional background image (0 for no img).
    @return	None.
*/
void
RmLoadingScreenStart(const char *title, const char *bgimg)
{
    if (HScreen && GfuiScreenIsActive(HScreen))
		return;

	if (HScreen)
		RmLoadingScreenShutdown();

    // Create screen, load menu XML descriptor and create static controls.
    HScreen = GfuiScreenCreate(BGColor, NULL, NULL, NULL, NULL, 0);

    void *hmenu = GfuiMenuLoad("loadingscreen.xml");

    GfuiMenuCreateStaticControls(HScreen, hmenu);

    // Create variable title label.
    int titleId = GfuiMenuCreateLabelControl(HScreen, hmenu, "titlelabel");
    GfuiLabelSetText(HScreen, titleId, title);

	// Get layout / coloring properties.
    NTextLines = (int)GfuiMenuGetNumProperty(hmenu, "nLines", 20);
    const int yTopLine = (int)GfuiMenuGetNumProperty(hmenu, "yTopLine", 400);
    const int yLineShift = (int)GfuiMenuGetNumProperty(hmenu, "yLineShift", 16);
    const float alpha0 = GfuiMenuGetNumProperty(hmenu, "alpha0", 0.2);
    const float alphaSlope = GfuiMenuGetNumProperty(hmenu, "alphaSlope", 0.0421);

	// Allocate line info arrays.
	FGColors = (float**)calloc(NTextLines, sizeof(float*));
	TextLines = (char**)calloc(NTextLines, sizeof(char*));
	TextLineIds = (int*)calloc(NTextLines, sizeof(int));

	// Create one label for each text line
	int y = yTopLine;
    for (int i = 0; i < NTextLines; i++)
	{
		// Create and set the color for this line.
		FGColors[i] = (float*)calloc(4, sizeof(float));
		FGColors[i][0] = FGColors[i][1] = FGColors[i][2] = 1.0;
		FGColors[i][3] = (float)i * alphaSlope + alpha0;

		// Create the control from the template.
		TextLineIds[i] =
			GfuiMenuCreateLabelControl(HScreen, hmenu, "line", true, // from template
									   "", GFUI_TPL_X, y, GFUI_TPL_FONTID, GFUI_TPL_WIDTH,
									   GFUI_TPL_ALIGN, GFUI_TPL_MAXLEN, FGColors[i]);

		// Next line if not last.
		y -= yLineShift;
    }

    CurTextLineIdx = 0;
    
    // Add background image.
    if (bgimg)
		GfuiScreenAddBgImg(HScreen, bgimg);

    // Close menu XML descriptor.
    GfParmReleaseHandle(hmenu);
    
    // Display screen.
    GfuiScreenActivate(HScreen);
    GfuiDisplay();

	#ifdef WEBSERVER
    //force one redisplay
    GfuiApp().eventLoop().forceRedisplay(); 
	#endif //WEBSERVER

}

void
RmLoadingScreenShutdown(void)
{
    if (HScreen)
	{
		for (int i = 0; i < NTextLines; i++)
		{
			free(FGColors[i]);
			if (TextLines[i])
				free(TextLines[i]);
		}
		freez(FGColors);
		freez(TextLines);
		freez(TextLineIds);
		
		GfuiScreenRelease(HScreen);
		HScreen = 0;
    }
}


/** 
    @ingroup	racemantools
    @param	text	Text to display.
    @return	None.
*/
void
RmLoadingScreenSetText(const char *text)
{
    GfLogTrace("%s\n", text);
    
    if (!HScreen)
	{
		//GfLogWarning("Can't display loading message : loading screen not created.\n");
		return;
	}
	
	if (TextLines[CurTextLineIdx])
		freez(TextLines[CurTextLineIdx]);
	if (text)
	{
		TextLines[CurTextLineIdx] = strdup(text);
		CurTextLineIdx = (CurTextLineIdx + 1) % NTextLines;
	}
	
	int i = CurTextLineIdx;
	int j = 0;
	do
	{
		if (TextLines[i])
			GfuiLabelSetText(HScreen, TextLineIds[j], TextLines[i]);
		j++;
		i = (i + 1) % NTextLines;
	}
	while (i != CurTextLineIdx);
	
	GfuiDisplay();
	#ifdef WEBSERVER
    //force one redisplay
    GfuiApp().eventLoop().forceRedisplay(); 
	#endif //WEBSERVER
}
 
