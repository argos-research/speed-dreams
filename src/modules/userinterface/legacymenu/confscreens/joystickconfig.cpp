/***************************************************************************

    file                 : joystickconfig.cpp
    created              : Wed Mar 21 21:46:11 CET 2001
    copyright            : (C) 2001 by Eric Espie
    email                : eric.espie@torcs.org
    version              : $Id: joystickconfig.cpp 6011 2015-05-19 18:28:37Z beaglejoe $

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
    		Human player joystick configuration menu
    @author	<a href=mailto:eric.espie@torcs.org>Eric Espie</a>
    @version	$Id: joystickconfig.cpp 6011 2015-05-19 18:28:37Z beaglejoe $
*/


#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <tgf.hpp>
#include <tgfclient.h>

#include "controlconfig.h"
#include "joystickconfig.h"


// Constants.
static const int NbMaxCalAxis = 4;
static const int NbCalSteps = 6;

static const int CmdOffset = -1;

// WARNING: These strings must match the names used for labels in the XML menu descriptor.
static const char *LabName[] = { "steer", "throttle", "brake", "clutch" };

// TODO: Put these strings in joystickconfigmenu.xml for translation.
static const char *Instructions[] = {
    "Center the joystick then press a button",
    "Hold steer left and press a button",
    "Hold steer right and press a button",
    "Apply full throttle and press a button",
    "Apply full brake and press a button",
    "Apply full clutch then press a button",
    "Joystick calibration completed",
    "Calibration failed"
};

// Current calibration step
static int CalState;

// Current command configuration to calibrate.
static tCmdInfo *Cmd;
static int MaxCmd;

// Joystick info.
#if SDL_JOYSTICK
static tCtrlJoyInfo joyInfo;// = NULL;
static tCtrlJoyInfo joyCenter;
#else
static jsJoystick* Joystick[GFCTRL_JOY_NUMBER];
static float       JoyAxis[GFCTRL_JOY_MAX_AXES * GFCTRL_JOY_NUMBER];
static float       JoyAxisCenter[GFCTRL_JOY_MAX_AXES * GFCTRL_JOY_NUMBER];
static int         JoyButtons[GFCTRL_JOY_NUMBER];
#endif

// Menu screen handle.
static void *ScrHandle = NULL;

// Next menu screen handle.
static void* NextMenuHandle = NULL;
static void* PrevMenuHandle = NULL;

// Screen controls Ids
static int InstId;

static int LabAxisId[NbMaxCalAxis];
static int LabMinId[NbMaxCalAxis];
static int LabMaxId[NbMaxCalAxis];

static int NextBut = 0;
static int CancelBut = 0;
static int DoneBut = 0;;

static void
onNext(void * /* dummy */)
{
#if SDL_JOYSTICK
//   GfctrlJoyRelease(joyInfo);
#else
    int index;

    /* Release up and running joysticks */
    for (index = 0; index < GFCTRL_JOY_NUMBER; index++)
	if (Joystick[index]) {
	    delete Joystick[index];
	    Joystick[index] = 0;
	}
#endif

    /* Back to previous screen */
    if (CalState == NbCalSteps && NextMenuHandle != NULL)
	GfuiScreenActivate(NextMenuHandle);
    else
	GfuiScreenActivate(PrevMenuHandle);
}

static void advanceStep (void)
{
    do {
	CalState++;
    } while (Cmd[CalState + CmdOffset].ref.type != GFCTRL_TYPE_JOY_AXIS && CalState < NbCalSteps);
}


static void
JoyCalAutomaton(void)
{
    static char buf[64];

    int axis;

    switch (CalState) {
    case 0:
#if SDL_JOYSTICK
   memcpy(&joyCenter, &joyInfo, sizeof(joyCenter));
#else
	memcpy(JoyAxisCenter, JoyAxis, sizeof(JoyAxisCenter));
#endif
	advanceStep();
	break;
    case 1:
	axis = Cmd[CalState + CmdOffset].ref.index;
#if SDL_JOYSTICK
   Cmd[CalState + CmdOffset].min = joyCenter.ax[axis];
   Cmd[CalState + CmdOffset].max = joyInfo.ax[axis];
#else
   Cmd[CalState + CmdOffset].min = JoyAxisCenter[axis];
   Cmd[CalState + CmdOffset].max = JoyAxis[axis];
#endif

	// record the polarity of the turn action
	if (Cmd[CalState + CmdOffset].max >= Cmd[CalState + CmdOffset].min)
		Cmd[CalState + CmdOffset].pow = 1.0;
	else
		Cmd[CalState + CmdOffset].pow = -1.0;

#if SDL_JOYSTICK
   sprintf(buf, "%.2f", joyInfo.ax[axis]);
#else
	sprintf(buf, "%.2f", JoyAxis[axis]);
#endif
	GfuiLabelSetText(ScrHandle, LabMinId[0], buf);
	advanceStep();
	break;
    case 2:
	axis = Cmd[CalState + CmdOffset].ref.index;
#if SDL_JOYSTICK
   Cmd[CalState + CmdOffset].min = joyCenter.ax[axis];
   Cmd[CalState + CmdOffset].max = joyInfo.ax[axis];
#else
	Cmd[CalState + CmdOffset].min = JoyAxisCenter[axis];
	Cmd[CalState + CmdOffset].max = JoyAxis[axis];
#endif

	// record the polarity of the turn action
	if (Cmd[CalState + CmdOffset].max >= Cmd[CalState + CmdOffset].min)
		Cmd[CalState + CmdOffset].pow = 1.0;
	else
		Cmd[CalState + CmdOffset].pow = -1.0;
#if SDL_JOYSTICK
   sprintf(buf, "%.2f", joyInfo.ax[axis]);
#else
	sprintf(buf, "%.2f", JoyAxis[axis]);
#endif
	GfuiLabelSetText(ScrHandle, LabMaxId[0], buf);
	advanceStep();
	break;
    case 3:
    case 4:
    case 5:
	axis = Cmd[CalState + CmdOffset].ref.index;
#if SDL_JOYSTICK
   Cmd[CalState + CmdOffset].min = joyCenter.ax[axis];
   Cmd[CalState + CmdOffset].max = joyInfo.ax[axis];
#else
	Cmd[CalState + CmdOffset].min = JoyAxisCenter[axis];
	Cmd[CalState + CmdOffset].max = JoyAxis[axis];
#endif
	Cmd[CalState + CmdOffset].pow = 1.0;
#if SDL_JOYSTICK
   sprintf(buf, "%.2f", joyCenter.ax[axis]);
#else
	sprintf(buf, "%.2f", JoyAxisCenter[axis]);
#endif
	GfuiLabelSetText(ScrHandle, LabMinId[CalState - 2], buf);
#if SDL_JOYSTICK
   sprintf(buf, "%.2f", joyInfo.ax[axis]);
#else
	sprintf(buf, "%.2f", JoyAxis[axis]);
#endif
	GfuiLabelSetText(ScrHandle, LabMaxId[CalState - 2], buf);
	advanceStep();

	break;
    }
    GfuiLabelSetText(ScrHandle, InstId, Instructions[CalState]);

    /* Change button appearance when done */
    if (CalState == NbCalSteps) {
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
   int		index;
#if SDL_JOYSTICK
   /* Check for activity on Joystick buttons */
   GfctrlJoyGetCurrentStates(&joyInfo);
   for (index = 0; index < GFCTRL_JOY_NUMBER * GFCTRL_JOY_MAX_BUTTONS; index++) {
      if (joyInfo.edgedn[index]) {
         /* Check whether to ignore */
         if(Cmd[CalState + CmdOffset].butIgnore == index)
            break;

         /* Button fired */
         JoyCalAutomaton();
         if (CalState >= NbCalSteps) {
            GfuiApp().eventLoop().setRecomputeCB(0);
         }
         GfuiApp().eventLoop().postRedisplay();
         return;
      }
#else
    int		mask;
    int		b, i;
    

    for (index = 0; index < GFCTRL_JOY_NUMBER; index++) {
	if (Joystick[index]) {
	    Joystick[index]->read(&b, &JoyAxis[index * GFCTRL_JOY_MAX_AXES]);
	    
	    /* Joystick buttons */
	    for (i = 0, mask = 1; i < 32; i++, mask *= 2) {
		if (((b & mask) != 0) && ((JoyButtons[index] & mask) == 0)) {
		    /* Check whether to ignore */
		    if(Cmd[CalState + CmdOffset].butIgnore == i + 32 * index)
			break;

		    /* Button fired */
		    JoyCalAutomaton();
		    if (CalState >= NbCalSteps) {
			GfuiApp().eventLoop().setRecomputeCB(0);
		    }
		    GfuiApp().eventLoop().postRedisplay();
		    JoyButtons[index] = b;
		    return;
		}
	    }
	    JoyButtons[index] = b;
	}
#endif
    }

    /* Let CPU take breath (and fans stay at low and quite speed) */
    GfSleep(0.001);
}


static void
onActivate(void * /* dummy */)
{
    int i;
    int step;
#if SDL_JOYSTICK
    //joyInfo = GfctrlJoyCreate();
    GfctrlJoyGetCurrentStates(&joyInfo);
#else
    int index;

    // Create and test joysticks ; only keep the up and running ones.
    for (index = 0; index < GFCTRL_JOY_NUMBER; index++) {
	Joystick[index] = new jsJoystick(index);
	if (Joystick[index]->notWorking()) {
	    /* don't configure the joystick */
	    delete Joystick[index];
	    Joystick[index] = 0;
	}
    }
#endif

    CalState = 0;
    GfuiLabelSetText(ScrHandle, InstId, Instructions[CalState]);
    GfuiApp().eventLoop().setRecomputeCB(Idle2);
    GfuiApp().eventLoop().postRedisplay();
#ifndef SDL_JOYSTICK
    for (index = 0; index < GFCTRL_JOY_NUMBER; index++) {
	if (Joystick[index]) {
	    Joystick[index]->read(&JoyButtons[index], &JoyAxis[index * GFCTRL_JOY_MAX_AXES]); /* initial value */
	}
    }
#endif

    for (i = 0; i < NbMaxCalAxis; i++) {
	if (i > 0) {
	    step = i + 2;
	} else {
	    step = i + 1;
	}
	if (Cmd[step + CmdOffset].ref.type == GFCTRL_TYPE_JOY_AXIS) {
	    GfuiLabelSetText(ScrHandle, LabAxisId[i], GfctrlGetNameByRef(GFCTRL_TYPE_JOY_AXIS, Cmd[step + CmdOffset].ref.index));
	} else {
	    GfuiLabelSetText(ScrHandle, LabAxisId[i], "---");
	}
	GfuiLabelSetText(ScrHandle, LabMinId[i], "");
 	GfuiLabelSetText(ScrHandle, LabMaxId[i], "");
    }

    GfuiEnable(ScrHandle, CancelBut, GFUI_ENABLE);
    if (DoneBut)
	GfuiEnable(ScrHandle, DoneBut, GFUI_DISABLE);
    else
	GfuiEnable(ScrHandle, NextBut, GFUI_DISABLE);
}


void *
JoyCalMenuInit(void *prevMenu, void *nextMenu, tCmdInfo *cmd, int maxcmd)
{
    int i;
    char pszBuf[64];

    Cmd = cmd;
    MaxCmd = maxcmd;
    PrevMenuHandle = prevMenu;
    NextMenuHandle = nextMenu;

    if (ScrHandle) {
	return ScrHandle;
    }
    
    // Create screen, load menu XML descriptor and create static controls.
    ScrHandle = GfuiScreenCreate(NULL, NULL, onActivate, NULL, NULL, 1);

    void *menuXMLDescHdle = GfuiMenuLoad("joystickconfigmenu.xml");

    GfuiMenuCreateStaticControls(ScrHandle, menuXMLDescHdle);

    // Create joystick axis label controls (axis name, axis Id, axis min value, axis max value)
    for (i = 0; i < NbMaxCalAxis; i++) {
	sprintf(pszBuf, "%saxislabel", LabName[i]);
	LabAxisId[i] = GfuiMenuCreateLabelControl(ScrHandle, menuXMLDescHdle, pszBuf);
	sprintf(pszBuf, "%sminlabel", LabName[i]);
	LabMinId[i] = GfuiMenuCreateLabelControl(ScrHandle, menuXMLDescHdle, pszBuf);
	sprintf(pszBuf, "%smaxlabel", LabName[i]);
	LabMaxId[i] = GfuiMenuCreateLabelControl(ScrHandle, menuXMLDescHdle, pszBuf);
    }

    // Create instruction variable label.
    InstId = GfuiMenuCreateLabelControl(ScrHandle, menuXMLDescHdle, "instructionlabel");
    
    // Create Cancel and Reset buttons.
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





