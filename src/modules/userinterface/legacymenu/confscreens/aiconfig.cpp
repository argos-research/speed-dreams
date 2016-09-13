/***************************************************************************

    file        : aiconfig.cpp
    created     : Sat Dec  26 12:00:00 CET 2009
	copyright   : (C) 2009 The Speed Dreams Team
	web         : speed-dreams.sourceforge.net
    version     : $Id: aiconfig.cpp 6143 2015-09-24 16:49:32Z torcs-ng $

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

    @version	$Id: aiconfig.cpp 6143 2015-09-24 16:49:32Z torcs-ng $
*/

#include <cstdio>
#include <cstdlib>

#include <tgfclient.h>
#include <robot.h>
#include <portability.h>

#include "aiconfig.h"


static const char* AIGlobalSkillFilePathName = "config/raceman/extra/skill.xml";

/* Available skill level names and associated values */
static const char *SkillLevels[] = { ROB_VAL_ARCADE, ROB_VAL_SEMI_ROOKIE, ROB_VAL_ROOKIE, ROB_VAL_AMATEUR, ROB_VAL_SEMI_PRO, ROB_VAL_PRO };
static const tdble SkillLevelValues[] = { 30.0, 20.0, 10.0, 7.0, 3.0, 0.0};
static const int NSkillLevels = sizeof(SkillLevels) / sizeof(SkillLevels[0]);
static int CurSkillLevelIndex = 0;

/* GUI label ids */
static int SkillLevelId;

/* GUI screen handles */
static void	*ScrHandle = NULL;
static void	*PrevHandle = NULL;


/* Load the parameter values from the corresponding parameter file */
static void ReadAICfg(void)
{
	tdble aiSkillValue;
	int i;

	char buf[256];
	snprintf(buf, 256, "%s%s", GfLocalDir(), AIGlobalSkillFilePathName);

	void *paramHandle = GfParmReadFile(buf, GFPARM_RMODE_REREAD | GFPARM_RMODE_CREAT);
	aiSkillValue = GfParmGetNum(paramHandle, "skill", "level", 0, SkillLevelValues[0]);

	CurSkillLevelIndex = NSkillLevels-1; // In case aiSkillValue < 0.
	for (i = 0; i < NSkillLevels; i++) {
		if (aiSkillValue >= SkillLevelValues[i]) {
			CurSkillLevelIndex = i;
			break;
		}
	}

	GfParmReleaseHandle(paramHandle);

	GfuiLabelSetText(ScrHandle, SkillLevelId, SkillLevels[CurSkillLevelIndex]);
}


/* Save the choosen values into the corresponding parameter file */
static void SaveSkillLevel(void * /* dummy */)
{
	char buf[256];
	snprintf(buf, 256, "%s%s", GfLocalDir(), AIGlobalSkillFilePathName);

	void *paramHandle = GfParmReadFile(buf, GFPARM_RMODE_REREAD | GFPARM_RMODE_CREAT);
	GfParmSetNum(paramHandle, "skill", "level", 0, SkillLevelValues[CurSkillLevelIndex]);
	GfParmWriteFile(NULL, paramHandle, "Skill");
	GfParmReleaseHandle(paramHandle);

	/* return to previous screen */
	GfuiScreenActivate(PrevHandle);

	return;
}

/* Change the global AI skill level */
static void
ChangeSkillLevel(void *vp)
{
    const int delta = ((long)vp < 0) ? -1 : 1;

	CurSkillLevelIndex = (CurSkillLevelIndex + delta + NSkillLevels) % NSkillLevels;

    GfuiLabelSetText(ScrHandle, SkillLevelId, SkillLevels[CurSkillLevelIndex]);
}


static void onActivate(void * /* dummy */)
{
    ReadAICfg();
}


/* Menu creation */
void *
AIMenuInit(void *prevMenu)
{
    /* screen already created */
    if (ScrHandle) {
        return ScrHandle;
    }
    PrevHandle = prevMenu;

    ScrHandle = GfuiScreenCreate((float*)NULL, NULL, onActivate, NULL, (tfuiCallback)NULL, 1);

	void *param = GfuiMenuLoad("aiconfigmenu.xml");
    GfuiMenuCreateStaticControls(ScrHandle, param);

    GfuiMenuCreateButtonControl(ScrHandle,param,"skillleftarrow",(void*)-1,ChangeSkillLevel);
    GfuiMenuCreateButtonControl(ScrHandle,param,"skillrightarrow",(void*)1,ChangeSkillLevel);

    SkillLevelId = GfuiMenuCreateLabelControl(ScrHandle,param,"skilllabel");
    GfuiMenuCreateButtonControl(ScrHandle,param,"ApplyButton",prevMenu,SaveSkillLevel);
    GfuiMenuCreateButtonControl(ScrHandle,param,"CancelButton",prevMenu,GfuiScreenActivate);

    GfParmReleaseHandle(param);

    GfuiAddKey(ScrHandle, GFUIK_RETURN, "Apply", NULL, SaveSkillLevel, NULL);
    GfuiAddKey(ScrHandle, GFUIK_ESCAPE, "Cancel", prevMenu, GfuiScreenActivate, NULL);
    GfuiAddKey(ScrHandle, GFUIK_F1, "Help", ScrHandle, GfuiHelpScreen, NULL);
    GfuiAddKey(ScrHandle, GFUIK_F12, "Screen-Shot", NULL, GfuiScreenShot, NULL);
    GfuiAddKey(ScrHandle, GFUIK_LEFT, "Previous Skill Level", (void*)-1, ChangeSkillLevel, NULL);
    GfuiAddKey(ScrHandle, GFUIK_RIGHT, "Next Skill Level", (void*)+1, ChangeSkillLevel, NULL);

    return ScrHandle;
}
