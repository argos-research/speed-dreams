/***************************************************************************

    file        : mouseconfig.cpp
    created     : Thu Mar 13 21:27:03 CET 2003
    copyright   : (C) 2003 by Eric Espi�                        
    email       : eric.espie@torcs.org   
    version     : $Id: mouseconfig.cpp 3893 2011-09-18 15:52:42Z pouillot $                                  

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
    		Human player mouse configuration menu
    @author	<a href=mailto:eric.espie@torcs.org>Eric Espie</a>
    @version	$Id: mouseconfig.cpp 3893 2011-09-18 15:52:42Z pouillot $
*/

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <tgf.hpp>
#include <tgfclient.h>

#include "controlconfig.h"
#include "mouseconfig.h"


// Constants.
static const int CmdOffset = 0;

// TODO: Put these strings in mouseconfigmenu.xml for translation.
static const char *Instructions[] = {
    "Move Mouse for maximum left steer then press a button",
    "Move Mouse for maximum right steer then press a button",
    "Move Mouse for full throttle then press a button",
    "Move Mouse for full brake then press a button",
    "Mouse calibration completed",
    "Calibration failed"
};

// Current calibration step
static int CalState;

// Current command configuration to calibrate.
static tCmdInfo *Cmd;
static int MaxCmd;

// Mouse info.
static tCtrlMouseInfo MouseInfo;

// Menu screen handle.
static void *ScrHandle = NULL;

// Next menu screen handle.
static void* NextMenuHandle = NULL;
static void* PrevMenuHandle = NULL;

// Screen controls Ids
static int InstId;

static int NextBut = 0;
static int CancelBut = 0;
static int DoneBut = 0;


static void
onNext(void * /* dummy */)
{
    /* Back to previous screen */
    if (CalState == 4 && NextMenuHandle != NULL)
	GfuiScreenActivate(NextMenuHandle);
    else
	GfuiScreenActivate(PrevMenuHandle);
}

static int
GetNextAxis(void)
{
    int i;

    for (i = CalState; i < 4; i++) {
	if (Cmd[CmdOffset + i].ref.type == GFCTRL_TYPE_MOUSE_AXIS) {
	    return i;
	}
    }

    return i;
}


static void Idle2(void);

static void
MouseCalAutomaton(void)
{
    float axv;

    switch (CalState) {
    case 0:
    case 1:
	GfctrlMouseGetCurrentState(&MouseInfo);
	axv = MouseInfo.ax[Cmd[CmdOffset + CalState].ref.index];
	if (fabs(axv) < 0.01) {
	    return;		/* ignore no move input */
	}
	Cmd[CmdOffset + CalState].max = axv;
	Cmd[CmdOffset + CalState].pow = 1.0 / axv;
	break;

    case 2:
    case 3:
	GfctrlMouseGetCurrentState(&MouseInfo);
	axv = MouseInfo.ax[Cmd[CmdOffset + CalState].ref.index];
	if (fabs(axv) < 0.01) {
	    return;		/* ignore no move input */
	}
	Cmd[CmdOffset + CalState].max = axv;
	Cmd[CmdOffset + CalState].pow = 1.0 / axv;
	break;
	
    }

    CalState++;
    CalState = GetNextAxis();
    GfuiLabelSetText(ScrHandle, InstId, Instructions[CalState]);
    if (CalState < 4) {
	GfuiApp().eventLoop().setRecomputeCB(Idle2);
    } else {
	GfuiApp().eventLoop().setRecomputeCB(0);
	GfuiApp().eventLoop().postRedisplay();
    }

    /* Change button appearance when done */
    if (CalState == 4) {
	GfuiEnable(ScrHandle, CancelBut, GFUI_DISABLE);
	if (DoneBut)
	   GfuiEnable(ScrHandle, DoneBut, GFUI_ENABLE);
	else
	   GfuiEnable(ScrHandle, NextBut, GFUI_ENABLE);
    }
}

static void
Idle2(void)
{
    int	i;

    GfctrlMouseGetCurrentState(&MouseInfo);

    /* Check for a mouse button pressed */
    for (i = 0; i < 3; i++) {
	if (MouseInfo.edgedn[i]) {
	    MouseCalAutomaton();
	    return;
	}
    }

    /* Let CPU take breath (and fans stay at low and quiet speed) */
    GfSleep(0.001);
}

static void
IdleMouseInit(void)
{
    /* Get the center mouse position  */
    memset(&MouseInfo, 0, sizeof(MouseInfo));
    GfctrlMouseGetCurrentState(&MouseInfo);
    GfctrlMouseInitCenter();
    GfuiApp().eventLoop().setRecomputeCB(Idle2);
}

static void
onActivate(void * /* dummy */)
{
    CalState = 0;
    GetNextAxis();
    GfuiLabelSetText(ScrHandle, InstId, Instructions[CalState]);
    if (CalState < 4) {
	GfuiApp().eventLoop().setRecomputeCB(IdleMouseInit);
	GfctrlMouseCenter();
    }

    GfuiEnable(ScrHandle, CancelBut, GFUI_ENABLE);
    if (DoneBut)
	GfuiEnable(ScrHandle, DoneBut, GFUI_DISABLE);
    else
	GfuiEnable(ScrHandle, NextBut, GFUI_DISABLE);
}

void *
MouseCalMenuInit(void *prevMenu, void *nextMenu, tCmdInfo *cmd, int maxcmd)
{
    Cmd = cmd;
    MaxCmd = maxcmd;
    NextMenuHandle = nextMenu;
    PrevMenuHandle = prevMenu;
    
    if (ScrHandle) {
	return ScrHandle;
    }

    // Create screen, load menu XML descriptor and create static controls.
    ScrHandle = GfuiScreenCreate(NULL, NULL, onActivate, NULL, NULL, 1);

    void *menuXMLDescHdle = GfuiMenuLoad("mouseconfigmenu.xml");

    GfuiMenuCreateStaticControls(ScrHandle, menuXMLDescHdle);

    // Create instruction variable label.
    InstId = GfuiMenuCreateLabelControl(ScrHandle, menuXMLDescHdle, "instructionlabel");
    
    // Create Back and Reset buttons.
    GfuiMenuCreateButtonControl(ScrHandle, menuXMLDescHdle, "resetbutton", NULL, onActivate);

    if (nextMenu != NULL) {
	NextBut = GfuiMenuCreateButtonControl(ScrHandle, menuXMLDescHdle, "nextbutton", NULL, onNext);
	GfuiEnable(ScrHandle, NextBut, GFUI_DISABLE);
    } else {
	DoneBut = GfuiMenuCreateButtonControl(ScrHandle, menuXMLDescHdle, "donebutton", NULL, onNext);
	GfuiEnable(ScrHandle, DoneBut, GFUI_DISABLE);
    }

    CancelBut = GfuiMenuCreateButtonControl(ScrHandle, menuXMLDescHdle, "cancelbutton", NULL, onNext);

    // Close menu XML descriptor.
    GfParmReleaseHandle(menuXMLDescHdle);
    
    // Register keyboard shortcuts.
    GfuiMenuDefaultKeysAdd(ScrHandle);
    GfuiAddKey(ScrHandle, GFUIK_ESCAPE, "Next", NULL, onNext, NULL);
    GfuiAddKey(ScrHandle, GFUIK_RETURN, "Next", NULL, onNext, NULL);

    return ScrHandle;
}
