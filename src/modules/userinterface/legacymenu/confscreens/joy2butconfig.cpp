/***************************************************************************

    file                 : joy2butconfig.cpp
    created              : Wed Mar 21 21:46:11 CET 2001
    copyright            : (C) 2001 by Eric Espie
    email                : eric.espie@torcs.org
    version              : $Id: joy2butconfig.cpp 6389 2016-03-21 19:18:31Z wdbee $

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
    @version	$Id: joy2butconfig.cpp 6389 2016-03-21 19:18:31Z wdbee $
*/


#include <stdio.h>
#include <stdlib.h>
#include <cstring>

#include <tgf.hpp>
#include <tgfclient.h>
#include <playerpref.h>

#include "controlconfig.h"
#include "joy2butconfig.h"


// Constants.
static const int NbMaxCalAxis = 4;
static const int NbCalSteps = 6;

static const int CmdOffset = -1;

// TODO: Put these strings in joystickconfigmenu.xml for translation.
static const char *Instructions[] = {
    "Select the NULL position then press a button",
    "Select the 'Command' position then press a button",
    "Select the 'Command' position then press a button",
    "Joy-to-button calibration is complete",
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

typedef struct linked_item {
	struct 	linked_item *next;
	int	command;
	float	value;
} linked_item_t;

static int AtobAxis;
static int AtobCount;
static int AtobAxisID;
static int AtobCommandID;
static linked_item_t * AtobList;

static int NextBut = 0;
static int CancelBut = 0;
static int DoneBut = 0;;

static void
onNext(void * /* dummy */)
{
    /* Release up and running joysticks */
#if SDL_JOYSTICK
    //GfctrlJoyRelease(joyInfo);
#else
    int index;
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
    int i;
    int nextAxis = GFCTRL_JOY_NUMBER * GFCTRL_JOY_MAX_AXES;

    AtobCount++;

    if (CalState <= 0) {
	CalState++;
	AtobCount = 0;
    }

    /* scan through cmds looking for the next ATOB on this axis */
    while (AtobCount <= CMD_END_OF_LIST) {
	if (Cmd[AtobCount].ref.type == GFCTRL_TYPE_JOY_ATOB 
		&& Cmd[AtobCount].ref.index == AtobAxis) {
	    GfuiLabelSetText(ScrHandle, AtobCommandID, Cmd[AtobCount].name);
	    return;
	}
	AtobCount++;
    }

    /* Pause briefly to compute thresholds */
    if (CalState ==2) {
	CalState = 3;
	return;
    } 

    /* no more new ATOBs for this axis, scan for next smallest axis */
    AtobCount = 0;

    for (i = 0; i <= CMD_END_OF_LIST; i++) {
	if (Cmd[i].ref.type == GFCTRL_TYPE_JOY_ATOB 
		&& nextAxis > Cmd[i].ref.index && AtobAxis < Cmd[i].ref.index) {
	    nextAxis = Cmd[i].ref.index;
	    AtobCount ++;
	}
    }
    if (AtobCount) {
	AtobAxis = nextAxis;

	/* find first command on new axis */
	for (AtobCount = 0; AtobCount <= CMD_END_OF_LIST; AtobCount++) {
	    if (Cmd[AtobCount].ref.type == GFCTRL_TYPE_JOY_ATOB 
		    && Cmd[AtobCount].ref.index == AtobAxis) {
    		GfuiLabelSetText(ScrHandle, AtobAxisID, GfctrlGetNameByRef(GFCTRL_TYPE_JOY_ATOB, AtobAxis));
		GfuiLabelSetText(ScrHandle, AtobCommandID, Cmd[AtobCount].name);
		CalState = 1;
		return;
	    }
	}
    }

    /* no more ATOBs found, we're done */
}


static void
JoyCalAutomaton(void)
{
    linked_item_t * item_in_list;
    linked_item_t * new_in_list;
    float last_max;

    switch (CalState) {
    case 0:
	/* Grab snapshot of 'NULL' position */
#if SDL_JOYSTICK
   memcpy(&joyCenter, &joyInfo, sizeof(joyCenter));
#else
	memcpy(JoyAxisCenter, JoyAxis, sizeof(JoyAxisCenter));
#endif

	advanceStep();
	break;

    case 1:
	/* Start linked list with Null Position */
	AtobList = (linked_item_t*)malloc(sizeof(linked_item_t));
	AtobList->next = NULL;
	AtobList->command = -1;
#if SDL_JOYSTICK
   AtobList->value = joyCenter.ax[AtobAxis];
#else
	AtobList->value = JoyAxisCenter[AtobAxis];
#endif

	CalState = 2;

	/* fall through */
    case 2:
	/* Insert each ATOB into list */
	item_in_list = AtobList;

	new_in_list = (linked_item_t*)malloc(sizeof(linked_item_t));
	new_in_list->command = AtobCount;
#if SDL_JOYSTICK
   new_in_list->value = joyInfo.ax[AtobAxis];
#else
	new_in_list->value = JoyAxis[AtobAxis];
#endif
		
	if (new_in_list->value < item_in_list->value) {
	    /* insert first position*/
	    new_in_list->next = item_in_list;
	    AtobList = new_in_list;
	} else {
	    /* walk list */
	    while (item_in_list->next != NULL) {
		if (new_in_list->value < item_in_list->next->value) {
		    /* insert after current */
		    new_in_list->next = item_in_list->next;
		    item_in_list->next = new_in_list;
		    break;
		}

		/* step to next item */
		item_in_list = item_in_list->next; 
	    }

	    if (item_in_list->next == NULL) {
		/* insert at end */
		new_in_list->next = NULL;
		item_in_list->next = new_in_list;
	    }
	}

	advanceStep();

	if (CalState == 3) {
	   /* Walk list to compute min/max thresholds */
	   last_max=-1.0;

	   while (AtobList != NULL) {
		item_in_list = AtobList;
		if (item_in_list->command != -1)
		   Cmd[item_in_list->command].min = last_max;

		/* Split difference between current and next */
		if (item_in_list->next != NULL)
		   last_max = (item_in_list->value + item_in_list->next->value)/2;
		else
		   last_max = 1.0;

		if (item_in_list->command != -1)
		    Cmd[item_in_list->command].max = last_max;

		AtobList = item_in_list->next; 
		free(item_in_list);
	    }

	    advanceStep();
	}

	break;
    }
    GfuiLabelSetText(ScrHandle, InstId, Instructions[CalState]);

    /* Change button appearance when done */
    if (CalState == 3) {
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
    AtobAxis = GFCTRL_JOY_NUMBER * GFCTRL_JOY_MAX_AXES;

    /* Find commands which are ATOB */
    for (i = 0; i <= CMD_END_OF_LIST; i++) {
	if (Cmd[i].ref.type == GFCTRL_TYPE_JOY_ATOB) {
	    if (AtobAxis > Cmd[i].ref.index) AtobAxis = Cmd[i].ref.index;
	}
    }
    for (AtobCount = 0; AtobCount <= CMD_END_OF_LIST; AtobCount++) {
	if (Cmd[AtobCount].ref.index == AtobAxis) break;
    }

    /* Set label titles */
    GfuiLabelSetText(ScrHandle, AtobAxisID, GfctrlGetNameByRef(GFCTRL_TYPE_JOY_ATOB, AtobAxis));
    GfuiLabelSetText(ScrHandle, AtobCommandID, "---");

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

    GfuiEnable(ScrHandle, CancelBut, GFUI_ENABLE);
    if (DoneBut)
	GfuiEnable(ScrHandle, DoneBut, GFUI_DISABLE);
    else
	GfuiEnable(ScrHandle, NextBut, GFUI_DISABLE);
}


void *
Joy2butCalMenuInit(void *prevMenu, void *nextMenu, tCmdInfo *cmd, int maxcmd)
{
    Cmd = cmd;
    MaxCmd = maxcmd;
    PrevMenuHandle = prevMenu;
    NextMenuHandle = nextMenu;

    if (ScrHandle) {
	return ScrHandle;
    }
    
    // Create screen, load menu XML descriptor and create static controls.
    ScrHandle = GfuiScreenCreate(NULL, NULL, onActivate, NULL, NULL, 1);

    void *menuXMLDescHdle = GfuiMenuLoad("joy2butconfigmenu.xml");

    GfuiMenuCreateStaticControls(ScrHandle, menuXMLDescHdle);

    AtobAxisID = GfuiMenuCreateLabelControl(ScrHandle, menuXMLDescHdle, "AtobAxisID");
    AtobCommandID = GfuiMenuCreateLabelControl(ScrHandle, menuXMLDescHdle, "AtobCommandID");

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
