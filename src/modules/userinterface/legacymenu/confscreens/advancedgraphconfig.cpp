/***************************************************************************

    file                 :  advancedgraphconfig.cpp
    created              : Sun May  13 16:30:25 CEST 2012
    copyright            : (C) 2012 by Xavier Bertaux
    email                :  bertauxx@yahoo.fr
    version              : $Id: advancedgraphconfig.cpp 4542 2012-05-13 17:24:29Z torcs-ng $

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
    		
    @author	<a href=mailto:bertauxx@yahoo.fr>Xavier Bertaux</a>
    @version	$Id: advancedgraphconfig.cpp 4542 2012-05-13 17:24:29Z torcs-ng $
*/

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <portability.h>
#include <tgfclient.h>
#include <graphic.h>

#include "advancedgraphconfig.h"

static const char* BackgroundTypeValues[] = { GR_ATT_BGSKY_RING, GR_ATT_BGSKY_LAND };
static const int NbBackgroundTypeValues = sizeof(BackgroundTypeValues) / sizeof(BackgroundTypeValues[0]);
//static const char* SpectatorValues[] = { GR_ATT_AGR_NULL, GR_ATT_AGR_LITTLE, GR_ATT_AGR_MEDIUM, GR_ATT_AGR_FULL, GR_ATT_AGR_HIGH };
//static const int NbSpectatorValues = sizeof(SpectatorValues) / sizeof(SpectatorValues[0]);
static const char* ForestValues[] = { GR_ATT_AGR_NULL, GR_ATT_AGR_LITTLE, GR_ATT_AGR_MEDIUM, GR_ATT_AGR_FULL, GR_ATT_AGR_HIGH };
static const int NbForestValues = sizeof(ForestValues) / sizeof(ForestValues[0]);
static const char* TreeValues[] = { GR_ATT_AGR_NULL, GR_ATT_AGR_LITTLE, GR_ATT_AGR_MEDIUM, GR_ATT_AGR_FULL, GR_ATT_AGR_HIGH };
static const int NbTreeValues = sizeof(TreeValues) / sizeof(TreeValues[0]);
static const char* ParkingValues[] = { GR_ATT_AGR_NULL, GR_ATT_AGR_LITTLE, GR_ATT_AGR_MEDIUM, GR_ATT_AGR_FULL, GR_ATT_AGR_HIGH };
static const int NbParkingValues = sizeof(ParkingValues) / sizeof(ParkingValues[0]);
static const char* SpansplitValues[] = { GR_VAL_NO, GR_VAL_YES };
static const int NbSpansplitValues = sizeof(SpansplitValues) / sizeof(SpansplitValues[0]);
static const char* MonitorValues[] = { GR_VAL_MONITOR_16BY9, GR_VAL_MONITOR_4BY3, GR_VAL_MONITOR_NONE };
static const int NbMonitorValues = sizeof(MonitorValues) / sizeof(MonitorValues[0]);

static void	*ScrHandle = NULL;

static int	BackgroundTypeLabelId, BackgroundTypeLeftButtonId, BackgroundTypeRightButtonId;
static int	ForestLabelId, ForestLeftButtonId, ForestRightButtonId;
static int	TreeLabelId, TreeLeftButtonId, TreeRightButtonId;
static int	ParkingLabelId, ParkingLeftButtonId, ParkingRightButtonId;
static int	SpansplitLabelId, SpansplitLeftButtonId, SpansplitRightButtonId;
static int	MonitorLabelId, MonitorLeftButtonId, MonitorRightButtonId;

static int	BackgroundTypeIndex = 0;
//static int	SpectatorsIndex = 0;
static int	ForestIndex = 0;
static int	TreeIndex = 0;
static int	ParkingIndex = 0;
static int	SpansplitIndex = 0;
static float	BezelComp = 110.0f;
static int	BezelCompId;
static float	ScreenDist = 1.0f;
static int	ScreenDistId;
static float	ArcRatio = 1.0f;
static int	ArcRatioId;
static int	MonitorIndex = 0;

static char	buf[512];


// Options IO functions ===================================================================

static void
loadOptions()
{
    snprintf(buf, sizeof(buf), "%s%s", GfLocalDir(), GR_PARAM_FILE);
    void* grHandle = GfParmReadFile(buf, GFPARM_RMODE_STD | GFPARM_RMODE_CREAT);

	BackgroundTypeIndex = 0; // Default value index, in case file value not found in list.
	const char* pszBackgroundType =
		GfParmGetStr(grHandle, GR_SCT_GRAPHIC, GR_ATT_BGSKYTYPE, GR_ATT_BGSKY_RING);
	for (int i = 0; i < NbBackgroundTypeValues; i++) 
	{
		if (!strcmp(pszBackgroundType, BackgroundTypeValues[i]))
		{
			BackgroundTypeIndex = i;
			break;
		}
	}

	ForestIndex = 0; // Default value index, in case file value not found in list.
	const char* pszForest =
		GfParmGetStr(grHandle, GR_SCT_GRAPHIC, GR_ATT_AGR_FOREST, GR_ATT_AGR_NULL);
	for (int i = 0; i < NbForestValues; i++) 
	{
		if (!strcmp(pszForest, ForestValues[i]))
		{
			ForestIndex = i;
			break;
		}
	}

	TreeIndex = 0; // Default value index, in case file value not found in list.
	const char* pszTree =
		GfParmGetStr(grHandle, GR_SCT_GRAPHIC, GR_ATT_AGR_TREE, GR_ATT_AGR_NULL);
	for (int i = 0; i < NbTreeValues; i++) 
	{
		if (!strcmp(pszTree, TreeValues[i]))
		{
			TreeIndex = i;
			break;
		}
	}

	ParkingIndex = 0; // Default value index, in case file value not found in list.
	const char* pszParking =
		GfParmGetStr(grHandle, GR_SCT_GRAPHIC, GR_ATT_AGR_PARKING, GR_ATT_AGR_NULL);
	for (int i = 0; i < NbParkingValues; i++) 
	{
		if (!strcmp(pszParking, ParkingValues[i]))
		{
			ParkingIndex = i;
			break;
		}
	}

	SpansplitIndex = 0; // Default value index, in case file value not found in list.
	const char* pszSpansplit =
		GfParmGetStr(grHandle, GR_SCT_GRAPHIC, GR_ATT_SPANSPLIT, GR_VAL_NO);
	for (int i = 0; i < NbSpansplitValues; i++) 
	{
		if (!strcmp(pszSpansplit, SpansplitValues[i]))
		{
			SpansplitIndex = i;
			break;
		}
	}

	BezelComp = GfParmGetNum(grHandle, GR_SCT_GRAPHIC, GR_ATT_BEZELCOMP, "%", 110.0f);
	if (BezelComp > 150.0f) {
		BezelComp = 150.0f;
	} 
	else if (BezelComp < 50.0f) {
		BezelComp = 50.0f;
	}

	sprintf(buf, "%g", BezelComp);
	GfuiEditboxSetString(ScrHandle, BezelCompId, buf);

	ScreenDist = GfParmGetNum(grHandle, GR_SCT_GRAPHIC, GR_ATT_SCREENDIST, NULL, 1.0f);
	if (ScreenDist > 5.0f) {
		ScreenDist = 5.0f;
	} 
	else if (ScreenDist < 0.0f) {
		ScreenDist = 0.0f;
	}

	sprintf(buf, "%g", ScreenDist);
	GfuiEditboxSetString(ScrHandle, ScreenDistId, buf);

	ArcRatio = GfParmGetNum(grHandle, GR_SCT_GRAPHIC, GR_ATT_ARCRATIO, NULL, 1.0f);
	if (ArcRatio > 2.0f) {
		ArcRatio = 2.0f;
	} 
	else if (ArcRatio < 0.0f) {
		ArcRatio = 0.0f;
	}

	sprintf(buf, "%g", ArcRatio);
	GfuiEditboxSetString(ScrHandle, ArcRatioId, buf);

	MonitorIndex = 0; // Default value index, in case file value not found in list.
	const char* pszMonitor =
		GfParmGetStr(grHandle, GR_SCT_GRAPHIC, GR_ATT_MONITOR, GR_VAL_MONITOR_16BY9);
	for (int i = 0; i < NbMonitorValues; i++) 
	{
		if (!strcmp(pszMonitor, MonitorValues[i]))
		{
			MonitorIndex = i;
			break;
		}
	}

    GfParmReleaseHandle(grHandle);
}

static void
saveOptions()
{
    // Force current edit to loose focus (if one has it) and update associated variable.
    GfuiUnSelectCurrent();

    snprintf(buf, sizeof(buf), "%s%s", GfLocalDir(), GR_PARAM_FILE);
    void* grHandle = GfParmReadFile(buf, GFPARM_RMODE_STD | GFPARM_RMODE_CREAT);
    
    GfParmSetStr(grHandle, GR_SCT_GRAPHIC, GR_ATT_BGSKYTYPE, BackgroundTypeValues[BackgroundTypeIndex]);
	GfParmSetStr(grHandle, GR_SCT_GRAPHIC, GR_ATT_AGR_FOREST, ForestValues[ForestIndex]);
	GfParmSetStr(grHandle, GR_SCT_GRAPHIC, GR_ATT_AGR_TREE, TreeValues[TreeIndex]);
	GfParmSetStr(grHandle, GR_SCT_GRAPHIC, GR_ATT_AGR_PARKING, ParkingValues[ParkingIndex]);
	GfParmSetStr(grHandle, GR_SCT_GRAPHIC, GR_ATT_SPANSPLIT, SpansplitValues[SpansplitIndex]);
	GfParmSetNum(grHandle, GR_SCT_GRAPHIC, GR_ATT_BEZELCOMP, "%", BezelComp);
	GfParmSetNum(grHandle, GR_SCT_GRAPHIC, GR_ATT_SCREENDIST, NULL, ScreenDist);
	GfParmSetNum(grHandle, GR_SCT_GRAPHIC, GR_ATT_ARCRATIO, NULL, ArcRatio);
	GfParmSetStr(grHandle, GR_SCT_GRAPHIC, GR_ATT_MONITOR, MonitorValues[MonitorIndex]);
    
    GfParmWriteFile(NULL, grHandle, "graph");
    
    GfParmReleaseHandle(grHandle);
}

// GUI callback functions ===================================================================

static void
onChangeBackgroundType(void* vp)
{
    const long delta = (long)vp;
    BackgroundTypeIndex = (BackgroundTypeIndex + NbBackgroundTypeValues + delta) % NbBackgroundTypeValues;
    GfuiLabelSetText(ScrHandle, BackgroundTypeLabelId, BackgroundTypeValues[BackgroundTypeIndex]);
}

static void
onChangeForest(void* vp)
{
    const long delta = (long)vp;
    ForestIndex = (ForestIndex + NbForestValues + delta) % NbForestValues;
    GfuiLabelSetText(ScrHandle, ForestLabelId, ForestValues[ForestIndex]);
}

static void
onChangeTree(void* vp)
{
    const long delta = (long)vp;
    TreeIndex = (TreeIndex + NbTreeValues + delta) % NbTreeValues;
    GfuiLabelSetText(ScrHandle, TreeLabelId, TreeValues[TreeIndex]);
}

static void
onChangeParking(void* vp)
{
    const long delta = (long)vp;
    ParkingIndex = (ParkingIndex + NbParkingValues + delta) % NbParkingValues;
    GfuiLabelSetText(ScrHandle, ParkingLabelId, ParkingValues[ParkingIndex]);
} 

static void
onChangeSpansplit(void* vp)
{
    const long delta = (long)vp;
    SpansplitIndex = (SpansplitIndex + NbSpansplitValues + delta) % NbSpansplitValues;
    GfuiLabelSetText(ScrHandle, SpansplitLabelId, SpansplitValues[SpansplitIndex]);

    GfuiEnable(ScrHandle, BezelCompId, SpansplitIndex ? GFUI_ENABLE : GFUI_DISABLE);
    GfuiEnable(ScrHandle, ScreenDistId, SpansplitIndex ? GFUI_ENABLE : GFUI_DISABLE);
    GfuiEnable(ScrHandle, ArcRatioId, SpansplitIndex ? GFUI_ENABLE : GFUI_DISABLE);
} 

static void
onChangeBezelComp(void * )
{
    char* val = GfuiEditboxGetString(ScrHandle, BezelCompId);
    sscanf(val, "%g", &BezelComp);
    if (BezelComp > 150.0f)
		BezelComp = 150.0f;
    else if (BezelComp < 50.0f)
		BezelComp = 50.0f;
	
    char buf[32];
    sprintf(buf, "%g", BezelComp);
    GfuiEditboxSetString(ScrHandle, BezelCompId, buf);
}

static void
onChangeScreenDist(void * )
{
    char* val = GfuiEditboxGetString(ScrHandle, ScreenDistId);
    sscanf(val, "%g", &ScreenDist);
    if (ScreenDist > 25.0f)
		ScreenDist = 25.0f;
    else if (ScreenDist < 0.1f)
		ScreenDist = 0.1f;
	
    char buf[32];
    sprintf(buf, "%g", ScreenDist);
    GfuiEditboxSetString(ScrHandle, ScreenDistId, buf);
}

static void
onChangeArcRatio(void * )
{
    char* val = GfuiEditboxGetString(ScrHandle, ArcRatioId);
    sscanf(val, "%g", &ArcRatio);
    if (ArcRatio > 2.0f)
		ArcRatio = 2.0f;
    else if (ArcRatio < 0.0f)
		ArcRatio = 0.0f;
	
    char buf[32];
    sprintf(buf, "%g", ArcRatio);
    GfuiEditboxSetString(ScrHandle, ArcRatioId, buf);
}

static void
onChangeMonitor(void* vp)
{
    const long delta = (long)vp;
    MonitorIndex = (MonitorIndex + NbMonitorValues + delta) % NbMonitorValues;
    GfuiLabelSetText(ScrHandle, MonitorLabelId, MonitorValues[MonitorIndex]);
} 

static void
onActivate(void* /* dummy */)
{
    loadOptions();

	// Load GUI control values.
    onChangeBackgroundType(0);
	onChangeForest(0);
	onChangeTree(0);
	onChangeParking(0);	
	onChangeSpansplit(0);	
	onChangeBezelComp(0);	
	onChangeScreenDist(0);	
	onChangeArcRatio(0);	
	onChangeMonitor(0);	
}

static void
onAccept(void* tgtScrHdle)
{
    saveOptions();
    
    GfuiScreenActivate(tgtScrHdle);
}

static void
onCancel(void* tgtScrHdle)
{
    GfuiScreenActivate(tgtScrHdle);
}


// Menu initialization ==================================================================

void*
AdvancedGraphMenuInit(void* prevMenu)
{
    // Don't do it twice.
    if (ScrHandle)
		return ScrHandle;

    ScrHandle = GfuiScreenCreate((float*)NULL, NULL, onActivate, NULL, (tfuiCallback)NULL, 1);

    void* param = GfuiMenuLoad("advancedgraphconfigmenu.xml");

    GfuiMenuCreateStaticControls(ScrHandle, param);

	BackgroundTypeLeftButtonId =
		GfuiMenuCreateButtonControl(ScrHandle, param, "bgskyleftarrow", (void*)-1, onChangeBackgroundType);
    BackgroundTypeRightButtonId =
		GfuiMenuCreateButtonControl(ScrHandle, param, "bgskyrightarrow", (void*)1, onChangeBackgroundType);
    BackgroundTypeLabelId =	GfuiMenuCreateLabelControl(ScrHandle, param, "bgskydomelabel");

	ForestLeftButtonId =
		GfuiMenuCreateButtonControl(ScrHandle, param, "forestleftarrow", (void*)-1, onChangeForest);
    ForestRightButtonId =
		GfuiMenuCreateButtonControl(ScrHandle, param, "forestrightarrow", (void*)1, onChangeForest);
    ForestLabelId =	GfuiMenuCreateLabelControl(ScrHandle, param, "forestlabel");

	TreeLeftButtonId =
		GfuiMenuCreateButtonControl(ScrHandle, param, "treeleftarrow", (void*)-1, onChangeTree);
    TreeRightButtonId =
		GfuiMenuCreateButtonControl(ScrHandle, param, "treerightarrow", (void*)1, onChangeTree);
    TreeLabelId = GfuiMenuCreateLabelControl(ScrHandle, param, "treelabel");

	ParkingLeftButtonId =
		GfuiMenuCreateButtonControl(ScrHandle, param, "parkingleftarrow", (void*)-1, onChangeParking);
    ParkingRightButtonId =
		GfuiMenuCreateButtonControl(ScrHandle, param, "parkingrightarrow", (void*)1, onChangeParking);
    ParkingLabelId = GfuiMenuCreateLabelControl(ScrHandle, param, "parkinglabel");

	SpansplitLeftButtonId =
		GfuiMenuCreateButtonControl(ScrHandle, param, "spansplitleftarrow", (void*)-1, onChangeSpansplit);
    SpansplitRightButtonId =
		GfuiMenuCreateButtonControl(ScrHandle, param, "spansplitrightarrow", (void*)1, onChangeSpansplit);
    SpansplitLabelId = GfuiMenuCreateLabelControl(ScrHandle, param, "spansplitlabel");

    BezelCompId = GfuiMenuCreateEditControl(ScrHandle, param, "bezelcompedit", NULL, NULL, onChangeBezelComp);

    ScreenDistId = GfuiMenuCreateEditControl(ScrHandle, param, "screendistedit", NULL, NULL, onChangeScreenDist);

    ArcRatioId = GfuiMenuCreateEditControl(ScrHandle, param, "arcratioedit", NULL, NULL, onChangeArcRatio);

	MonitorLeftButtonId =
		GfuiMenuCreateButtonControl(ScrHandle, param, "monitorleftarrow", (void*)-1, onChangeMonitor);
    MonitorRightButtonId =
		GfuiMenuCreateButtonControl(ScrHandle, param, "monitorrightarrow", (void*)1, onChangeMonitor);
    MonitorLabelId = GfuiMenuCreateLabelControl(ScrHandle, param, "monitorlabel");

    GfuiMenuCreateButtonControl(ScrHandle, param, "ApplyButton", prevMenu, onAccept);
    GfuiMenuCreateButtonControl(ScrHandle, param, "CancelButton", prevMenu, onCancel);
    
    GfParmReleaseHandle(param);
    
    GfuiAddKey(ScrHandle, GFUIK_RETURN, "Apply", prevMenu, onAccept, NULL);
    GfuiAddKey(ScrHandle, GFUIK_ESCAPE, "Cancel", prevMenu, onCancel, NULL);
    GfuiAddKey(ScrHandle, GFUIK_F1, "Help", ScrHandle, GfuiHelpScreen, NULL);
    GfuiAddKey(ScrHandle, GFUIK_F12, "Screen-Shot", NULL, GfuiScreenShot, NULL);

    return ScrHandle;
}
