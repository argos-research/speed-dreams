/***************************************************************************

    file                 : raceoptimization.cpp
    created              : Sun Feb 25 00:34:46 2001
    copyright            : (C) 2000 by Eric Espie (C) 2014 by Wolf-Dieter Beelitz
    email                : eric.espie@torcs.org
    version              : $Id: raceoptimization.cpp 4361 2012-01-07 14:05:16Z pouillot $

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
    		The menu for when the optimiztion is running
    @ingroup	racemantools		
    @author	<a href=mailto:eric.espie@torcs.org>Eric Espie</a>
    @version	$Id: raceloadingmenu.cpp 4361 2012-01-07 14:05:16Z pouillot $
*/

#include <cstring>
#include <cstdlib>
#include <portability.h>

#include <tgfclient.h>

#include <car.h>

#include "legacymenu.h"
#include "racescreens.h"

static int NTextLines = 0;

static float BGColor[4] = {0.0, 0.0, 0.0, 0.0};

static void* HScreen = 0;
static bool InShutdownMode = false;

static float** FGColors = 0;
static int*	TextLineIds = 0;
static char** TextLines = 0;
static int CurTextLineIdx;

static int StatusLabelId = 0;
static int TotalLapTimeId = 0;
static int ParameterListLabelId = 0;

static float** ParameterFGColors = 0;
static int*	ParameterIds = 0;
static char** Parameters = 0;
static int*	ParameterValueIds = 0;
static char** ParameterValues = 0;
static int*	ParameterRangeIds = 0;
static char** ParameterRanges = 0;

static char* BestLapTimeValue = NULL;
static char* TotalLapTimeValue = NULL;
static char* InitialLapTimeValue = NULL;

static int LoopsDoneValueId = 0;
static char bufLoopsDoneValue[10];
static int LoopsRemainingValueId = 0;
static char bufLoopsRemainingValue[10];
static int VariationScaleValueId = 0;
static char bufVariationScaleValue[10];

static int InitialLapTimeValueId = 0;
static int TotalLapTimeValueId = 0;
static int BestLapTimeValueId = 0;

static double LapTimeDifference = 0;

static void
onDeactivate(void* /* dummy */)
{
	if (!InShutdownMode)
	{
//		RmOptimizationScreenShutdown();
	}
}

static void 
onEscape(void * /* dummy */)
{
	LmRaceEngine().abortRace(); // Do cleanup to get back correct setup files
}

/** 
    @ingroup	racemantools
    @param	title	Screen title.
    @param	bgimg	Optional background image (0 for no img).
    @return	None.
*/
void
RmOptimizationScreenStart(const char *title, const char *bgimg)
{
    if (HScreen && GfuiScreenIsActive(HScreen))
		return;

	if (HScreen)
		RmOptimizationScreenShutdown();

//	void* prevHdle = GfuiGetScreen();

    // Create screen, load menu XML descriptor and create static controls.
    HScreen = GfuiScreenCreate(BGColor, NULL, NULL, NULL, onDeactivate, 0);

    void *hmenu = GfuiMenuLoad("optimizationscreen.xml");

    GfuiMenuCreateStaticControls(HScreen, hmenu);

    // Create variable title label.
    int titleId = GfuiMenuCreateLabelControl(HScreen, hmenu, "titlelabel");
    GfuiLabelSetText(HScreen, titleId, title);

	const char* StatusLabel = "Status";
    StatusLabelId = GfuiMenuCreateLabelControl(HScreen, hmenu, "StatusLabel");
    GfuiLabelSetText(HScreen, StatusLabelId, StatusLabel);

	const char* InitialLapTimeLabel = "Initial lap time:";
    int InitialLapTimeId = GfuiMenuCreateLabelControl(HScreen, hmenu, "InitialLapTimeLabel");
    GfuiLabelSetText(HScreen, InitialLapTimeId, InitialLapTimeLabel);
    InitialLapTimeValueId = GfuiMenuCreateLabelControl(HScreen, hmenu, "InitialLapTimeValue");
    GfuiLabelSetText(HScreen, InitialLapTimeValueId, "");

	const char* TotalLapTimeLabel = "Total lap time:";
    TotalLapTimeId = GfuiMenuCreateLabelControl(HScreen, hmenu, "TotalLapTimeLabel");
    GfuiLabelSetText(HScreen, TotalLapTimeId, TotalLapTimeLabel);
    TotalLapTimeValueId = GfuiMenuCreateLabelControl(HScreen, hmenu, "TotalLapTimeValue");
    GfuiLabelSetText(HScreen, TotalLapTimeValueId, "");

	const char* BestLapTimeLabel = "Best lap time:";
    int BestLapTimeId = GfuiMenuCreateLabelControl(HScreen, hmenu, "BestLapTimeLabel");
    GfuiLabelSetText(HScreen, BestLapTimeId, BestLapTimeLabel);
    BestLapTimeValueId = GfuiMenuCreateLabelControl(HScreen, hmenu, "BestLapTimeValue");
    GfuiLabelSetText(HScreen, BestLapTimeValueId, "");

	const char* LoopsDoneLabel = "Loops done:";
    int LoopsDoneId = GfuiMenuCreateLabelControl(HScreen, hmenu, "LoopsDoneLabel");
    GfuiLabelSetText(HScreen, LoopsDoneId, LoopsDoneLabel);
    LoopsDoneValueId = GfuiMenuCreateLabelControl(HScreen, hmenu, "LoopsDoneValue");
    GfuiLabelSetText(HScreen, LoopsDoneValueId, "");

	const char* LoopsRemainingLabel = "Loops remaining:";
    int LoopsRemainingId = GfuiMenuCreateLabelControl(HScreen, hmenu, "LoopsRemainingLabel");
    GfuiLabelSetText(HScreen, LoopsRemainingId, LoopsRemainingLabel);
    LoopsRemainingValueId = GfuiMenuCreateLabelControl(HScreen, hmenu, "LoopsRemainingValue");
    GfuiLabelSetText(HScreen, LoopsRemainingValueId, "");

	const char* VariationScaleLabel = "Variation scale:";
    int VariationScaleId = GfuiMenuCreateLabelControl(HScreen, hmenu, "VariationScaleLabel");
    GfuiLabelSetText(HScreen, VariationScaleId, VariationScaleLabel);
    VariationScaleValueId = GfuiMenuCreateLabelControl(HScreen, hmenu, "VariationScaleValue");
    GfuiLabelSetText(HScreen, VariationScaleValueId, "");

	const char* ParameterListLabel = "Parameters varied";
    ParameterListLabelId = GfuiMenuCreateLabelControl(HScreen, hmenu, "ParametersVariedLabel");
    GfuiLabelSetText(HScreen, ParameterListLabelId, ParameterListLabel);

	// Get layout / coloring properties.
    NTextLines = (int)GfuiMenuGetNumProperty(hmenu, "nLines", 38);
    const int yTopLine = (int)GfuiMenuGetNumProperty(hmenu, "yTopLine", 454);
    const int yLineShift = (int)GfuiMenuGetNumProperty(hmenu, "yLineShift", 12);
    const float alpha0 = GfuiMenuGetNumProperty(hmenu, "alpha0", 0.1);
    const float alphaSlope = GfuiMenuGetNumProperty(hmenu, "alphaSlope", 0.1);

	// Allocate line info arrays.
	FGColors = (float**)calloc(NTextLines, sizeof(float*));
	TextLines = (char**)calloc(NTextLines, sizeof(char*));
	TextLineIds = (int*)calloc(NTextLines, sizeof(int));

	ParameterFGColors = (float**)calloc(8, sizeof(float*));
	
	ParameterIds = (int*)calloc(8, sizeof(int));
	Parameters = (char**)calloc(8, sizeof(char*));

	ParameterValueIds = (int*)calloc(8, sizeof(int));
	ParameterValues = (char**)calloc(8, sizeof(char*));

	ParameterRangeIds = (int*)calloc(8, sizeof(int));
	ParameterRanges = (char**)calloc(8, sizeof(char*));

	int fontID = GFUI_FONT_SMALL_C;

	// Create one label and a value for each parameter
	int y = 188;
    for (int i = 0; i < 8; i++)
	{
		// Create and set the color for this line.
		ParameterFGColors[i] = (float*)calloc(4, sizeof(float));
		ParameterFGColors[i][0] = ParameterFGColors[i][1] = ParameterFGColors[i][2] = 1.0;
		ParameterFGColors[i][3] = 1;

		// Create the control from the template.
		ParameterIds[i] =
			GfuiMenuCreateLabelControl(HScreen, hmenu, "parameter", true, // from template
				"", GFUI_TPL_X, y, fontID, GFUI_TPL_WIDTH,
				GFUI_TPL_ALIGN, GFUI_TPL_MAXLEN, ParameterFGColors[i]);

		// Next line if not last.
		y -= 2*yLineShift;
    }

	fontID = GFUI_FONT_SMALL_T;
	// Create two values for each parameter
	y = 188;
    for (int i = 0; i < 8; i++)
	{
		// Create the control from the template.
		ParameterValueIds[i] =
			GfuiMenuCreateLabelControl(HScreen, hmenu, "value", true, // from template
				"", GFUI_TPL_X, y, fontID, GFUI_TPL_WIDTH,
				GFUI_TPL_ALIGN, GFUI_TPL_MAXLEN, ParameterFGColors[i]);

		// Create the control from the template.
		ParameterRangeIds[i] =
			GfuiMenuCreateLabelControl(HScreen, hmenu, "value", true, // from template
				"", GFUI_TPL_X, y - yLineShift, fontID, GFUI_TPL_WIDTH,
				GFUI_TPL_ALIGN, GFUI_TPL_MAXLEN, ParameterFGColors[i]);

		// Next line if not last.
		y -= 2*yLineShift;
    }

	fontID = GFUI_FONT_SMALL_T;
	// Create one label for each text line
	y = yTopLine;
    for (int i = 0; i < NTextLines; i++)
	{
		// Create and set the color for this line.
		FGColors[i] = (float*)calloc(4, sizeof(float));
		FGColors[i][0] = FGColors[i][1] = FGColors[i][2] = 1.0;
		FGColors[i][3] = (float)i * alphaSlope + alpha0;

		// Create the control from the template.
		TextLineIds[i] =
			GfuiMenuCreateLabelControl(HScreen, hmenu, "line", true, // from template
				"", GFUI_TPL_X, y, fontID, GFUI_TPL_WIDTH,
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
    
    // Link key handlers
	//GfuiAddKey(HScreen, GFUIK_ESCAPE, "Back to the Main menu", RmRaceSelectMenuHandle, GfuiScreenActivate, NULL);
    GfuiAddKey(HScreen, GFUIK_ESCAPE, "Continue", HScreen, onEscape, NULL);

    // Display screen.
    GfuiScreenActivate(HScreen);
    GfuiDisplay();
}

void
RmOptimizationScreenShutdown(void)
{
    if (HScreen)
	{
		InShutdownMode = true;

		for (int I = 0; I < NTextLines; I++)
		{
			free(FGColors[I]);
			if (TextLines[I])
				free(TextLines[I]);
		}

		freez(FGColors);
		freez(TextLines);
		freez(TextLineIds);
		
		freez(InitialLapTimeValue);
		freez(TotalLapTimeValue);
		freez(BestLapTimeValue);
	
		for (int I = 0; I < 8; I++)
		{
			freez(ParameterFGColors[I]);
			freez(Parameters[I]);
			freez(ParameterValues[I]);
			freez(ParameterRanges[I]);
		}

		freez(ParameterFGColors);
		freez(ParameterIds);
		freez(Parameters);
		freez(ParameterValueIds);
		freez(ParameterValues);
		freez(ParameterRangeIds);
		freez(ParameterRanges);

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
RmOptimizationScreenSetText(const char* text)
{
    GfLogTrace("%s\n", text);
    
    if (!HScreen)
	{
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
}
 
/** 
    @ingroup	racemantools
    @param	arrays of parameter labels, values and ranges
    @return	None.
*/
void
RmOptimizationScreenSetParameterText(int N, char** Labels, char** Values, char** Ranges)
{
    if (!HScreen)
	{
		//GfLogWarning("Can't display loading message : loading screen not created.\n");
		return;
	}

	bool AnyTextDisplayed = false;

	for (int I = 0; I < N; I++)
	{
		freez(Parameters[I]);
		if (Labels[I])
		{
			AnyTextDisplayed = true;
			Parameters[I] = strdup(Labels[I]);
			GfuiLabelSetText(HScreen, ParameterIds[I], Parameters[I]);
		}
		else
			GfuiLabelSetText(HScreen, ParameterIds[I], "");

		freez(ParameterValues[I]);
		if (Values[I])
		{
			ParameterValues[I] = strdup(Values[I]);
			GfuiLabelSetText(HScreen, ParameterValueIds[I], ParameterValues[I]);
		}
		else
			GfuiLabelSetText(HScreen, ParameterValueIds[I], "");

		freez(ParameterRanges[I]);
		if (Ranges[I])
		{
			ParameterRanges[I] = strdup(Ranges[I]);
			GfuiLabelSetText(HScreen, ParameterRangeIds[I], ParameterRanges[I]);
		}
		else
			GfuiLabelSetText(HScreen, ParameterRangeIds[I], "");
	}

	for (int I = N; I < 8; I++)
	{
		freez(Parameters[I]);
		GfuiLabelSetText(HScreen, ParameterIds[I], "");

		freez(ParameterValues[I]);
		GfuiLabelSetText(HScreen, ParameterValueIds[I], "");

		freez(ParameterRanges[I]);
		GfuiLabelSetText(HScreen, ParameterRangeIds[I], "");
	}

	if (!AnyTextDisplayed)
	{
	    void *hmenu = GfuiMenuLoad("optimizationscreen.xml");

		const char* StatusLabel = "Final Status";
		GfuiLabelSetText(HScreen, StatusLabelId, StatusLabel);

		const char* TotalLapTimeLabel = "Faster by:";
		GfuiLabelSetText(HScreen, TotalLapTimeId, TotalLapTimeLabel);

		freez(TotalLapTimeValue);
		TotalLapTimeValue = GfTime2Str(LapTimeDifference, 0, false, 3);
		GfuiLabelSetText(HScreen, TotalLapTimeValueId, TotalLapTimeValue);

		GfuiLabelSetText(HScreen, ParameterListLabelId, "Press any key to continue ...");

	    // Close menu XML descriptor.
		GfParmReleaseHandle(hmenu);
	}

	GfuiDisplay();
}
 
/** 
    @ingroup	racemantools
    @param	arrays of parameter labels, values and ranges
    @return	None.
*/
void
RmOptimizationScreenSetStatusText(int LoopsDone, int LoopsRemaining, double VariationScale, double InitialLapTime, double TotalLapTime, double BestLapTime)
{
    if (!HScreen)
	{
		//GfLogWarning("Can't display loading message : loading screen not created.\n");
		return;
	}

	snprintf(bufLoopsDoneValue,sizeof(bufLoopsDoneValue),"%d",LoopsDone);
	GfuiLabelSetText(HScreen, LoopsDoneValueId, bufLoopsDoneValue);

	snprintf(bufLoopsRemainingValue,sizeof(bufLoopsRemainingValue),"%d",LoopsRemaining);
	GfuiLabelSetText(HScreen, LoopsRemainingValueId, bufLoopsRemainingValue);

	snprintf(bufVariationScaleValue,sizeof(bufVariationScaleValue),"%.3f",VariationScale);
	GfuiLabelSetText(HScreen, VariationScaleValueId, bufVariationScaleValue);

	freez(InitialLapTimeValue);
	InitialLapTimeValue = GfTime2Str(InitialLapTime, 0, false, 3);
	GfuiLabelSetText(HScreen, InitialLapTimeValueId, InitialLapTimeValue);

	freez(TotalLapTimeValue);
	TotalLapTimeValue = GfTime2Str(TotalLapTime, 0, false, 3);
	GfuiLabelSetText(HScreen, TotalLapTimeValueId, TotalLapTimeValue);

	freez(BestLapTimeValue);
	BestLapTimeValue = GfTime2Str(BestLapTime, 0, false, 3);
	GfuiLabelSetText(HScreen, BestLapTimeValueId, BestLapTimeValue);

	LapTimeDifference = InitialLapTime - BestLapTime;

	GfuiDisplay();
}
