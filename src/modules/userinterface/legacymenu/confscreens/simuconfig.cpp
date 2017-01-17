/***************************************************************************

    file        : simuconfig.cpp
    created     : Wed Nov  3 21:48:26 CET 2004
    copyright   : (C) 2004 by Eric Espie                       
    email       : eric.espie@free.fr  
    version     : $Id: simuconfig.cpp 6158 2015-10-04 01:04:51Z torcs-ng $

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
    		Simutation option menu
    @version	$Id: simuconfig.cpp 6158 2015-10-04 01:04:51Z torcs-ng $
*/

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <raceman.h>
#include <portability.h>
#include <tgfclient.h>

#include "simuconfig.h"

#include "gui.h"

/*#ifndef OFFICIAL_ONLY
#define OFFICIAL_ONLY 1
#endif*/


/* list of available simulation engine */
static const int DefaultSimuVersion = 1;
static const char *SimuVersionList[] =
	{RM_VAL_MOD_SIMU_V2, RM_VAL_MOD_SIMU_V2_1, RM_VAL_MOD_SIMU_V3, RM_VAL_MOD_SIMU_V4, RM_VAL_MOD_SIMU_REPLAY};
static const char *SimuVersionDispNameList[] = 	{"V2.0", "V2.1", "V3.0", "V4.0", "Replay"};
static const int NbSimuVersions = sizeof(SimuVersionList) / sizeof(SimuVersionList[0]);
static int CurSimuVersion = DefaultSimuVersion;

/* list of available multi-threading schemes */
static const char *MultiThreadSchemeList[] = {RM_VAL_AUTO, RM_VAL_ON, RM_VAL_OFF};
static const int NbMultiThreadSchemes = sizeof(MultiThreadSchemeList) / sizeof(MultiThreadSchemeList[0]);

/* list of available thread affinity schemes */
static const char *ThreadAffinitySchemeList[] = {RM_VAL_ON, RM_VAL_OFF};
static const int NbThreadAffinitySchemes = sizeof(ThreadAffinitySchemeList) / sizeof(ThreadAffinitySchemeList[0]);

static int CurMultiThreadScheme = 0;    // Auto
static int CurThreadAffinityScheme = 0; // On

/* list of available replay record schemes */
static const char *ReplaySchemeList[] = {RM_VAL_REPLAY_OFF, RM_VAL_REPLAY_LOW, RM_VAL_REPLAY_NORMAL, RM_VAL_REPLAY_HIGH, RM_VAL_REPLAY_PERFECT};
static const char *ReplaySchemeDispNameList[] = 	{"off", "Low", "Normal", "High", "Perfect"};
static const int NbReplaySchemes = sizeof(ReplaySchemeList) / sizeof(ReplaySchemeList[0]);
static int CurReplayScheme = 0;

/* list of available start paused schemes */
static const char *StartPausedSchemeList[] = {RM_VAL_ON, RM_VAL_OFF};
static const int NbStartPausedSchemes = sizeof(StartPausedSchemeList) / sizeof(StartPausedSchemeList[0]);
static int CurStartPausedScheme = 0; // On

/* list of available cooldown schemes */
static const char *CooldownSchemeList[] = {RM_VAL_ON, RM_VAL_OFF};
static const int NbCooldownSchemes = sizeof(CooldownSchemeList) / sizeof(CooldownSchemeList[0]);
static int CurCooldownScheme = 0; // On

/* gui label ids */
static int SimuVersionId;
static int MultiThreadSchemeId;
static int ThreadAffinitySchemeId;
static int ReplayRateSchemeId;

static int StartPausedSchemeId;

static int CooldownSchemeId;

/* gui screen handles */
static void *ScrHandle = NULL;
static void *PrevScrHandle = NULL;


static void loadSimuCfg(void)
{
	const char *simuVersionName;
	const char *multiThreadSchemeName;
	const char *threadAffinitySchemeName;
	const char *replayRateSchemeName;
	const char *startPausedSchemeName;
	const char *cooldownSchemeName;

	int i;

	char buf[1024];
	snprintf(buf, sizeof(buf), "%s%s", GfLocalDir(), RACE_ENG_CFG);

	void *paramHandle = GfParmReadFile(buf, GFPARM_RMODE_REREAD | GFPARM_RMODE_CREAT);

	// Simulation engine name.
	simuVersionName = GfParmGetStr(paramHandle, RM_SECT_MODULES, RM_ATTR_MOD_SIMU, SimuVersionList[DefaultSimuVersion]);
	for (i = 0; i < NbSimuVersions; i++)
	{
		if (strcmp(simuVersionName, SimuVersionList[i]) == 0)
		{
			CurSimuVersion = i;
			break;
		}
	}

	// Check if the selected simulation module is there, and fall back to the default one if not.
	snprintf(buf, sizeof(buf), "%smodules/simu/%s.%s", GfLibDir(), SimuVersionList[CurSimuVersion], DLLEXT);
	if (!GfFileExists(buf))
	{
		GfLogWarning("User settings %s physics engine module not found ; falling back to %s\n",
					 SimuVersionList[CurSimuVersion], SimuVersionList[DefaultSimuVersion]);
		CurSimuVersion = DefaultSimuVersion;
	}

	// Multi-threading.
	multiThreadSchemeName = GfParmGetStr(paramHandle, RM_SECT_RACE_ENGINE, RM_ATTR_MULTI_THREADING, MultiThreadSchemeList[0]);
	for (i = 0; i < NbMultiThreadSchemes; i++)
	{
		if (strcmp(multiThreadSchemeName, MultiThreadSchemeList[i]) == 0)
		{
			CurMultiThreadScheme = i;
			break;
		}
	}

	// Thread affinity.
	threadAffinitySchemeName = GfParmGetStr(paramHandle, RM_SECT_RACE_ENGINE, RM_ATTR_THREAD_AFFINITY, ThreadAffinitySchemeList[0]);
	for (i = 0; i < NbThreadAffinitySchemes; i++) 
	{
		if (strcmp(threadAffinitySchemeName, ThreadAffinitySchemeList[i]) == 0)
		{
			CurThreadAffinityScheme = i;
			break;
		}
	}

	// Replay Rate
#ifdef THIRD_PARTY_SQLITE3
	replayRateSchemeName = GfParmGetStr(paramHandle, RM_SECT_RACE_ENGINE, RM_ATTR_REPLAY_RATE, ReplaySchemeList[0]);
	for (i = 0; i < NbReplaySchemes; i++) 
	{
		if (strcmp(replayRateSchemeName, ReplaySchemeList[i]) == 0)
		{
			CurReplayScheme = i;
			break;
		}
	}
#else
	CurReplayScheme = 0;
#endif

	// startpaused
	startPausedSchemeName = GfParmGetStr(paramHandle,RM_SECT_RACE_ENGINE, RM_ATTR_STARTPAUSED, StartPausedSchemeList[1]);
	for (i = 0; i < NbStartPausedSchemes; i++) 
	{
		if (strcmp(startPausedSchemeName, StartPausedSchemeList[i]) == 0) 
		{
			CurStartPausedScheme = i;
			break;
		}
	}

	// cooldown
	cooldownSchemeName = GfParmGetStr(paramHandle,RM_SECT_RACE_ENGINE, RM_ATTR_COOLDOWN, CooldownSchemeList[1]);
	for (i = 0; i < NbCooldownSchemes; i++)
	{
		if (strcmp(cooldownSchemeName, CooldownSchemeList[i]) == 0) 
		{
			CurCooldownScheme = i;
			break;
		}
	}

	GfParmReleaseHandle(paramHandle);

	GfuiLabelSetText(ScrHandle, SimuVersionId, SimuVersionDispNameList[CurSimuVersion]);
	GfuiLabelSetText(ScrHandle, MultiThreadSchemeId, MultiThreadSchemeList[CurMultiThreadScheme]);
	GfuiLabelSetText(ScrHandle, ThreadAffinitySchemeId, ThreadAffinitySchemeList[CurThreadAffinityScheme]);
	GfuiLabelSetText(ScrHandle, ReplayRateSchemeId, ReplaySchemeDispNameList[CurReplayScheme]);

#ifndef THIRD_PARTY_SQLITE3
	GfuiEnable(ScrHandle, ReplayRateSchemeId, GFUI_DISABLE);
#endif
	GfuiLabelSetText(ScrHandle, StartPausedSchemeId, StartPausedSchemeList[CurStartPausedScheme]);
	GfuiLabelSetText(ScrHandle, CooldownSchemeId, CooldownSchemeList[CurCooldownScheme]);
}


/* Save the choosen values in the corresponding parameter file */
static void storeSimuCfg(void * /* dummy */)
{
	char buf[1024];
	snprintf(buf, sizeof(buf), "%s%s", GfLocalDir(), RACE_ENG_CFG);

	void *paramHandle = GfParmReadFile(buf, GFPARM_RMODE_REREAD | GFPARM_RMODE_CREAT);
	GfParmSetStr(paramHandle, RM_SECT_MODULES, RM_ATTR_MOD_SIMU, SimuVersionList[CurSimuVersion]);
	GfParmSetStr(paramHandle, RM_SECT_RACE_ENGINE, RM_ATTR_MULTI_THREADING, MultiThreadSchemeList[CurMultiThreadScheme]);
	GfParmSetStr(paramHandle, RM_SECT_RACE_ENGINE, RM_ATTR_THREAD_AFFINITY, ThreadAffinitySchemeList[CurThreadAffinityScheme]);
	GfParmSetStr(paramHandle, RM_SECT_RACE_ENGINE, RM_ATTR_REPLAY_RATE, ReplaySchemeList[CurReplayScheme]);
	GfParmSetStr(paramHandle, RM_SECT_RACE_ENGINE, RM_ATTR_STARTPAUSED, StartPausedSchemeList[CurStartPausedScheme]);
	GfParmSetStr(paramHandle, RM_SECT_RACE_ENGINE, RM_ATTR_COOLDOWN, CooldownSchemeList[CurCooldownScheme]);
	GfParmWriteFile(NULL, paramHandle, "raceengine");
	GfParmReleaseHandle(paramHandle);
	
	/* return to previous screen */
	GfuiScreenActivate(PrevScrHandle);
	return;
}

/* Change the simulation version (but only show really available modules) */
static void
onChangeSimuVersion(void *vp)
{
	char buf[1024];

	if (!vp)
		return;

	const int oldSimuVersion = CurSimuVersion;
	do
	{
		CurSimuVersion = (CurSimuVersion + NbSimuVersions + (int)(long)vp) % NbSimuVersions;
	
		snprintf(buf, sizeof(buf), "%smodules/simu/%s.%s", GfLibDir(), SimuVersionList[CurSimuVersion], DLLEXT);
	}
	while (!GfFileExists(buf) && CurSimuVersion != oldSimuVersion);

	GfuiLabelSetText(ScrHandle, SimuVersionId, SimuVersionDispNameList[CurSimuVersion]);
}

/* Change the multi-threading scheme */
static void
onChangeMultiThreadScheme(void *vp)
{
	CurMultiThreadScheme =
		(CurMultiThreadScheme + NbMultiThreadSchemes + (int)(long)vp) % NbMultiThreadSchemes;
	
	GfuiLabelSetText(ScrHandle, MultiThreadSchemeId, MultiThreadSchemeList[CurMultiThreadScheme]);
}


/* Change the thread affinity scheme */
static void
onChangeThreadAffinityScheme(void *vp)
{
	CurThreadAffinityScheme =
		(CurThreadAffinityScheme + NbThreadAffinitySchemes + (int)(long)vp) % NbThreadAffinitySchemes;
	
	GfuiLabelSetText(ScrHandle, ThreadAffinitySchemeId, ThreadAffinitySchemeList[CurThreadAffinityScheme]);
}


/* Change the replay rate scheme */
static void
onChangeReplayRateScheme(void *vp)
{
	CurReplayScheme =
		(CurReplayScheme + NbReplaySchemes + (int)(long)vp) % NbReplaySchemes;
	
	GfuiLabelSetText(ScrHandle, ReplayRateSchemeId, ReplaySchemeDispNameList[CurReplayScheme]);
}

/* Change the startpaused scheme */
static void
onChangeStartPausedScheme(void *vp)
{
	CurStartPausedScheme =
		(CurStartPausedScheme + NbStartPausedSchemes + (int)(long)vp) % NbStartPausedSchemes;
	
	GfuiLabelSetText(ScrHandle, StartPausedSchemeId, StartPausedSchemeList[CurStartPausedScheme]);
}

/* Change the cooldown scheme */
static void
onChangeCooldownScheme(void *vp)
{
	CurCooldownScheme =
		(CurCooldownScheme + NbCooldownSchemes + (int)(long)vp) % NbCooldownSchemes;
	
	GfuiLabelSetText(ScrHandle, CooldownSchemeId, CooldownSchemeList[CurCooldownScheme]);
}

static void onActivate(void * /* dummy */)
{
    loadSimuCfg();
}


/* Menu creation */
void *
SimuMenuInit(void *prevMenu)
{
    /* screen already created */
    if (ScrHandle) {
	return ScrHandle;
    }
    PrevScrHandle = prevMenu;

    ScrHandle = GfuiScreenCreate((float*)NULL, NULL, onActivate, NULL, (tfuiCallback)NULL, 1);

    void *menuDescHdle = GfuiMenuLoad("simuconfigmenu.xml");
    GfuiMenuCreateStaticControls(ScrHandle, menuDescHdle);

    SimuVersionId = GfuiMenuCreateLabelControl(ScrHandle,menuDescHdle,"simulabel");
    GfuiMenuCreateButtonControl(ScrHandle, menuDescHdle, "simuleftarrow", (void*)-1, onChangeSimuVersion);
    GfuiMenuCreateButtonControl(ScrHandle, menuDescHdle, "simurightarrow", (void*)1, onChangeSimuVersion);

    MultiThreadSchemeId = GfuiMenuCreateLabelControl(ScrHandle, menuDescHdle, "mthreadlabel");

    GfuiMenuCreateButtonControl(ScrHandle, menuDescHdle, "mthreadleftarrow", (void*)-1, onChangeMultiThreadScheme);
    GfuiMenuCreateButtonControl(ScrHandle, menuDescHdle, "mthreadrightarrow", (void*)1, onChangeMultiThreadScheme);

    ThreadAffinitySchemeId = GfuiMenuCreateLabelControl(ScrHandle, menuDescHdle, "threadafflabel");

    GfuiMenuCreateButtonControl(ScrHandle, menuDescHdle, "threadaffleftarrow", (void*)-1, onChangeThreadAffinityScheme);
    GfuiMenuCreateButtonControl(ScrHandle, menuDescHdle, "threadaffrightarrow", (void*)1, onChangeThreadAffinityScheme);
	
    ReplayRateSchemeId = GfuiMenuCreateLabelControl(ScrHandle, menuDescHdle, "replayratelabel");
#ifdef THIRD_PARTY_SQLITE3
    GfuiMenuCreateButtonControl(ScrHandle, menuDescHdle, "replayrateleftarrow", (void*)-1, onChangeReplayRateScheme);
    GfuiMenuCreateButtonControl(ScrHandle, menuDescHdle, "replayraterightarrow", (void*)1, onChangeReplayRateScheme);
#endif

    StartPausedSchemeId = GfuiMenuCreateLabelControl(ScrHandle, menuDescHdle, "startpausedlabel");

    GfuiMenuCreateButtonControl(ScrHandle, menuDescHdle, "startpausedleftarrow", (void*)-1, onChangeStartPausedScheme);
    GfuiMenuCreateButtonControl(ScrHandle, menuDescHdle, "startpausedrightarrow", (void*)1, onChangeStartPausedScheme);


    CooldownSchemeId = GfuiMenuCreateLabelControl(ScrHandle, menuDescHdle, "cooldownlabel");

    GfuiMenuCreateButtonControl(ScrHandle, menuDescHdle, "cooldownleftarrow", (void*)-1, onChangeCooldownScheme);
    GfuiMenuCreateButtonControl(ScrHandle, menuDescHdle, "cooldownrightarrow", (void*)1, onChangeCooldownScheme);

    GfuiMenuCreateButtonControl(ScrHandle, menuDescHdle, "ApplyButton", PrevScrHandle, storeSimuCfg);
    GfuiMenuCreateButtonControl(ScrHandle, menuDescHdle, "CancelButton", PrevScrHandle, GfuiScreenActivate);


    GfParmReleaseHandle(menuDescHdle);
    
    GfuiAddKey(ScrHandle, GFUIK_RETURN, "Apply", NULL, storeSimuCfg, NULL);
    GfuiAddKey(ScrHandle, GFUIK_ESCAPE, "Cancel", PrevScrHandle, GfuiScreenActivate, NULL);
    GfuiAddKey(ScrHandle, GFUIK_F1, "Help", ScrHandle, GfuiHelpScreen, NULL);
    GfuiAddKey(ScrHandle, GFUIK_F12, "Screen-Shot", NULL, GfuiScreenShot, NULL);
    GfuiAddKey(ScrHandle, GFUIK_LEFT, "Previous simu engine version", (void*)-1, onChangeSimuVersion, NULL);
    GfuiAddKey(ScrHandle, GFUIK_RIGHT, "Next simu engine version", (void*)1, onChangeSimuVersion, NULL);
    GfuiAddKey(ScrHandle, GFUIK_UP, "Previous multi-threading scheme", (void*)-1, onChangeMultiThreadScheme, NULL);
    GfuiAddKey(ScrHandle, GFUIK_DOWN, "Next multi-threading scheme", (void*)1, onChangeMultiThreadScheme, NULL);

    return ScrHandle;  
}
