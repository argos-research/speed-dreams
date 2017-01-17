/***************************************************************************

    file        : control.cpp
    created     : Thu Mar  6 22:01:33 CET 2003
    copyright   : (C) 2003 by Eric Espi�                        
    email       : eric.espie@torcs.org   
    version     : $Id: control.cpp 6012 2015-05-19 21:13:38Z beaglejoe $                                  

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
    		Human control (joystick, mouse and keyboard).
    @author	<a href=mailto:eric.espie@torcs.org>Eric Espie</a>
    @version	$Id: control.cpp 6012 2015-05-19 21:13:38Z beaglejoe $
    @ingroup	ctrl
*/

#include <stdlib.h>
#include <stdio.h>
#include <cstring>

//#ifndef SDL_JOYSTICK
//#ifdef __APPLE__
//#include <js.h>
//#else
//#include <plib/js.h>
//#endif
//#endif



#include "tgfclient.h"
#if SDL_MAJOR_VERSION >= 2
#include "SDL_haptic.h"
#endif


static const char *GfJoyBtn[] = {
"BTN1-0","BTN2-0","BTN3-0","BTN4-0","BTN5-0","BTN6-0","BTN7-0","BTN8-0","BTN9-0","BTN10-0","BTN11-0","BTN12-0","BTN13-0","BTN14-0","BTN15-0","BTN16-0",
"BTN17-0","BTN18-0","BTN19-0","BTN20-0","BTN21-0","BTN22-0","BTN23-0","BTN24-0","BTN25-0","BTN26-0","BTN27-0","BTN28-0","BTN29-0","BTN30-0","BTN31-0","BTN32-0",
"BTN1-1","BTN2-1","BTN3-1","BTN4-1","BTN5-1","BTN6-1","BTN7-1","BTN8-1","BTN9-1","BTN10-1","BTN11-1","BTN12-1","BTN13-1","BTN14-1","BTN15-1","BTN16-1",
"BTN17-1","BTN18-1","BTN19-1","BTN20-1","BTN21-1","BTN22-1","BTN23-1","BTN24-1","BTN25-1","BTN26-1","BTN27-1","BTN28-1","BTN29-1","BTN30-1","BTN31-1","BTN32-1",
"BTN1-2","BTN2-2","BTN3-2","BTN4-2","BTN5-2","BTN6-2","BTN7-2","BTN8-2","BTN9-2","BTN10-2","BTN11-2","BTN12-2","BTN13-2","BTN14-2","BTN15-2","BTN16-2",
"BTN17-2","BTN18-2","BTN19-2","BTN20-2","BTN21-2","BTN22-2","BTN23-2","BTN24-2","BTN25-2","BTN26-2","BTN27-2","BTN28-2","BTN29-2","BTN30-2","BTN31-2","BTN32-2",
"BTN1-3","BTN2-3","BTN3-3","BTN4-3","BTN5-3","BTN6-3","BTN7-3","BTN8-3","BTN9-3","BTN10-3","BTN11-3","BTN12-3","BTN13-3","BTN14-3","BTN15-3","BTN16-3",
"BTN17-3","BTN18-3","BTN19-3","BTN20-3","BTN21-3","BTN22-3","BTN23-3","BTN24-3","BTN25-3","BTN26-3","BTN27-3","BTN28-3","BTN29-3","BTN30-3","BTN31-3","BTN32-3",
"BTN1-4","BTN2-4","BTN3-4","BTN4-4","BTN5-4","BTN6-4","BTN7-4","BTN8-4","BTN9-4","BTN10-4","BTN11-4","BTN12-4","BTN13-4","BTN14-4","BTN15-4","BTN16-4",
"BTN17-4","BTN18-4","BTN19-4","BTN20-4","BTN21-4","BTN22-4","BTN23-4","BTN24-4","BTN25-4","BTN26-4","BTN27-4","BTN28-4","BTN29-4","BTN30-4","BTN31-4","BTN32-4",
"BTN1-5","BTN2-5","BTN3-5","BTN4-5","BTN5-5","BTN6-5","BTN7-5","BTN8-5","BTN9-5","BTN10-5","BTN11-5","BTN12-5","BTN13-5","BTN14-5","BTN15-5","BTN16-5",
"BTN17-5","BTN18-5","BTN19-5","BTN20-5","BTN21-5","BTN22-5","BTN23-5","BTN24-5","BTN25-5","BTN26-5","BTN27-5","BTN28-5","BTN29-5","BTN30-5","BTN31-5","BTN32-5",
"BTN1-6","BTN2-6","BTN3-6","BTN4-6","BTN5-6","BTN6-6","BTN7-6","BTN8-6","BTN9-6","BTN10-6","BTN11-6","BTN12-6","BTN13-6","BTN14-6","BTN15-6","BTN16-6",
"BTN17-6","BTN18-6","BTN19-6","BTN20-6","BTN21-6","BTN22-6","BTN23-6","BTN24-6","BTN25-6","BTN26-6","BTN27-6","BTN28-6","BTN29-6","BTN30-6","BTN31-6","BTN32-6",
"BTN1-7","BTN2-7","BTN3-7","BTN4-7","BTN5-7","BTN6-7","BTN7-7","BTN8-7","BTN9-7","BTN10-7","BTN11-7","BTN12-7","BTN13-7","BTN14-7","BTN15-7","BTN16-7",
"BTN17-7","BTN18-7","BTN19-7","BTN20-7","BTN21-7","BTN22-7","BTN23-7","BTN24-7","BTN25-7","BTN26-7","BTN27-7","BTN28-7","BTN29-7","BTN30-7","BTN31-7","BTN32-7"
};

static const char *GfJoyAxis[] = {
    "AXIS0-0", "AXIS1-0", "AXIS2-0", "AXIS3-0", "AXIS4-0", "AXIS5-0", "AXIS6-0", "AXIS7-0", "AXIS8-0", "AXIS9-0", "AXIS10-0", "AXIS11-0",
    "AXIS0-1", "AXIS1-1", "AXIS2-1", "AXIS3-1", "AXIS4-1", "AXIS5-1", "AXIS6-1", "AXIS7-1", "AXIS8-1", "AXIS9-1", "AXIS10-1", "AXIS11-1",
    "AXIS0-2", "AXIS1-2", "AXIS2-2", "AXIS3-2", "AXIS4-2", "AXIS5-2", "AXIS6-2", "AXIS7-2", "AXIS8-2", "AXIS9-2", "AXIS10-2", "AXIS11-2",
    "AXIS0-3", "AXIS1-3", "AXIS2-3", "AXIS3-3", "AXIS4-3", "AXIS5-3", "AXIS6-3", "AXIS7-3", "AXIS8-3", "AXIS9-3", "AXIS10-3", "AXIS11-3",
    "AXIS0-4", "AXIS1-4", "AXIS2-4", "AXIS3-4", "AXIS4-4", "AXIS5-4", "AXIS6-4", "AXIS7-4", "AXIS8-4", "AXIS9-4", "AXIS10-4", "AXIS11-4",
    "AXIS0-5", "AXIS1-5", "AXIS2-5", "AXIS3-5", "AXIS4-5", "AXIS5-5", "AXIS6-5", "AXIS7-5", "AXIS8-5", "AXIS9-5", "AXIS10-5", "AXIS11-5",
    "AXIS0-6", "AXIS1-6", "AXIS2-6", "AXIS3-6", "AXIS4-6", "AXIS5-6", "AXIS6-6", "AXIS7-6", "AXIS8-6", "AXIS9-6", "AXIS10-6", "AXIS11-6",
    "AXIS0-7", "AXIS1-7", "AXIS2-7", "AXIS3-7", "AXIS4-7", "AXIS5-7", "AXIS6-7", "AXIS7-7", "AXIS8-7", "AXIS9-7", "AXIS10-7", "AXIS11-7"
};

static const char *GfJoyAtob[] = {
    "ATOB0-0", "ATOB1-0", "ATOB2-0", "ATOB3-0", "ATOB4-0", "ATOB5-0", "ATOB6-0", "ATOB7-0", "ATOB8-0", "ATOB9-0", "ATOB10-0", "ATOB11-0",
    "ATOB0-1", "ATOB1-1", "ATOB2-1", "ATOB3-1", "ATOB4-1", "ATOB5-1", "ATOB6-1", "ATOB7-1", "ATOB8-1", "ATOB9-1", "ATOB10-1", "ATOB11-1",
    "ATOB0-2", "ATOB1-2", "ATOB2-2", "ATOB3-2", "ATOB4-2", "ATOB5-2", "ATOB6-2", "ATOB7-2", "ATOB8-2", "ATOB9-2", "ATOB10-2", "ATOB11-2",
    "ATOB0-3", "ATOB1-3", "ATOB2-3", "ATOB3-3", "ATOB4-3", "ATOB5-3", "ATOB6-3", "ATOB7-3", "ATOB8-3", "ATOB9-3", "ATOB10-3", "ATOB11-3",
    "ATOB0-4", "ATOB1-4", "ATOB2-4", "ATOB3-4", "ATOB4-4", "ATOB5-4", "ATOB6-4", "ATOB7-4", "ATOB8-4", "ATOB9-4", "ATOB10-4", "ATOB11-4",
    "ATOB0-5", "ATOB1-5", "ATOB2-5", "ATOB3-5", "ATOB4-5", "ATOB5-5", "ATOB6-5", "ATOB7-5", "ATOB8-5", "ATOB9-5", "ATOB10-5", "ATOB11-5",
    "ATOB0-6", "ATOB1-6", "ATOB2-6", "ATOB3-6", "ATOB4-6", "ATOB5-6", "ATOB6-6", "ATOB7-6", "ATOB8-6", "ATOB9-6", "ATOB10-6", "ATOB11-6",
    "ATOB0-7", "ATOB1-7", "ATOB2-7", "ATOB3-7", "ATOB4-7", "ATOB5-7", "ATOB6-7", "ATOB7-7", "ATOB8-7", "ATOB9-7", "ATOB10-7", "ATOB11-7"
};

static const char *GfMouseBtn[] = {"MOUSE_LEFT_BTN", "MOUSE_MIDDLE_BTN", "MOUSE_RIGHT_BTN", "MOUSE_WHEEL_UP", "MOUSE_ WHEEL_DN", "MOUSE_X1", "MOUSE_X2"};

static const char *GfMouseAxis[] = {"MOUSE_LEFT", "MOUSE_RIGHT", "MOUSE_UP", "MOUSE_DOWN"};

typedef struct
{
    const char	*descr;
    int		val;
} tgfKeyBinding;

static tgfKeyBinding GfKey[] = {
    {"Backspace", GFUIK_BACKSPACE},
    {"Tab",		GFUIK_TAB},
    {"Enter",	GFUIK_RETURN},
    {"Escape",	GFUIK_ESCAPE},
    {"Space",	GFUIK_SPACE},
    {"F1",		GFUIK_F1},
    {"F2",		GFUIK_F2},
    {"F3",		GFUIK_F3},
    {"F4",		GFUIK_F4},
    {"F5",		GFUIK_F5},
    {"F6",		GFUIK_F6},
    {"F7",		GFUIK_F7},
    {"F8",		GFUIK_F8},
    {"F9",		GFUIK_F9},
    {"F10",		GFUIK_F10},
    {"F11",		GFUIK_F11},
    {"F12",		GFUIK_F12},
    {"Left Arrow",	GFUIK_LEFT},
    {"Up Arrow",	GFUIK_UP},
    {"Right Arrow",	GFUIK_RIGHT},
    {"Down Arrow",	GFUIK_DOWN},
    {"Page Up",		GFUIK_PAGEUP},
    {"Page Down",	GFUIK_PAGEDOWN},
    {"Home",		GFUIK_HOME},
    {"End",		GFUIK_END},
    {"Insert",		GFUIK_INSERT}
//    {"Delele",		GFUIK_DEL}
};

static int gfmaxJoyButton	= sizeof(GfJoyBtn)	/ sizeof(GfJoyBtn[0]);
static int gfmaxJoyAxis		= sizeof(GfJoyAxis)	/ sizeof(GfJoyAxis[0]);
static int gfmaxJoyAtob		= sizeof(GfJoyAtob)	/ sizeof(GfJoyAtob[0]);
static int gfmaxMouseButton	= sizeof(GfMouseBtn)	/ sizeof(GfMouseBtn[0]);
static int gfmaxMouseAxis	= sizeof(GfMouseAxis)	/ sizeof(GfMouseAxis[0]);
static int gfmaxKey		= sizeof(GfKey)		/ sizeof(GfKey[0]);

static int gfctrlJoyPresent = GFCTRL_JOY_UNTESTED;


#if SDL_JOYSTICK
static SDL_Joystick *Joysticks[GFCTRL_JOY_NUMBER] = {NULL};
static tCtrlJoyInfo *joyInfoCopy = NULL;
//static tCtrlJoyInfo joyInfo;
#if SDL_MAJOR_VERSION >= 2
static SDL_Haptic *Haptics[GFCTRL_JOY_NUMBER] = {NULL};
static SDL_HapticEffect cfx[GFCTRL_JOY_NUMBER];
static unsigned int	cfx_timeout[GFCTRL_JOY_NUMBER];
static unsigned int 	rfx_timeout[GFCTRL_JOY_NUMBER];
static int id[GFCTRL_JOY_NUMBER];
#endif
#else
static jsJoystick *Joysticks[GFCTRL_JOY_NUMBER] = {NULL};
#endif


/** Get a control reference by its name
    @ingroup	ctrl
    @param	name	name of the control
    @return	pointer on a static structure tCtrlRef
    @see	tCtrlRef
*/
tCtrlRef *
GfctrlGetRefByName(const char *name)
{
    static tCtrlRef	ref;
    int 		i;

    if (!name || !strlen(name)) {
	ref.index = -1;
	ref.type = GFCTRL_TYPE_NOT_AFFECTED;
	return &ref;
    }
    if (strcmp("---", name) == 0) {
	ref.index = -1;
	ref.type = GFCTRL_TYPE_NOT_AFFECTED;
	return &ref;
    }
    for (i = 0; i < gfmaxJoyButton; i++) {
	if (strcmp(name, GfJoyBtn[i]) == 0) {
	    ref.index = i;
	    ref.type = GFCTRL_TYPE_JOY_BUT;
	    return &ref;
	}
    }
    for (i = 0; i < gfmaxJoyAxis; i++) {
	if (strcmp(name, GfJoyAxis[i]) == 0) {
	    ref.index = i;
	    ref.type = GFCTRL_TYPE_JOY_AXIS;
	    return &ref;
	}
    }
    for (i = 0; i < gfmaxJoyAtob; i++) {
	if (strcmp(name, GfJoyAtob[i]) == 0) {
	    ref.index = i;
	    ref.type = GFCTRL_TYPE_JOY_ATOB;
	    return &ref;
	}
    }
    for (i = 0; i < gfmaxMouseButton; i++) {
	if (strcmp(name, GfMouseBtn[i]) == 0) {
	    ref.index = i;
	    ref.type = GFCTRL_TYPE_MOUSE_BUT;
	    return &ref;
	}
    }
    for (i = 0; i < gfmaxMouseAxis; i++) {
	if (strcmp(name, GfMouseAxis[i]) == 0) {
	    ref.index = i;
	    ref.type = GFCTRL_TYPE_MOUSE_AXIS;
	    return &ref;
	}
    }
    for (i = 0; i < gfmaxKey; i++) {
	if (strcmp(name, GfKey[i].descr) == 0) {
	    ref.index = GfKey[i].val;
	    ref.type = GFCTRL_TYPE_KEYBOARD;
	    return &ref;
	}
    }
    ref.index = name[0];
    ref.type = GFCTRL_TYPE_KEYBOARD;
    return &ref;
}

/** Get a control name by its reference
    @ingroup	ctrl
    @param	type	type of control
    @param	index	reference index
    @return	pointer on a static structure tCtrlRef
*/
const char *
GfctrlGetNameByRef(int type, int index)
{
    static char buf[4];
    int i;
    
    switch (type) {
    case GFCTRL_TYPE_NOT_AFFECTED:
	return NULL;
    case GFCTRL_TYPE_JOY_BUT:
	if (index < gfmaxJoyButton) {
	    return GfJoyBtn[index];
	} else {
	    return NULL;
	}
	break;
    case GFCTRL_TYPE_JOY_AXIS:
	if (index < gfmaxJoyAxis) {
	    return GfJoyAxis[index];
	} else {
	    return NULL;
	}
	break;
    case GFCTRL_TYPE_JOY_ATOB:
	if (index < gfmaxJoyAtob) {
	    return GfJoyAtob[index];
	} else {
	    return NULL;
	}
	break;
    case GFCTRL_TYPE_MOUSE_BUT:
	if (index < gfmaxMouseButton) {
	    return GfMouseBtn[index];
	} else {
	    return NULL;
	}
	break;
    case GFCTRL_TYPE_MOUSE_AXIS:
	if (index < gfmaxMouseAxis) {
	    return GfMouseAxis[index];
	} else {
	    return NULL;
	}
	break;
    case GFCTRL_TYPE_KEYBOARD:
	for (i = 0; i < gfmaxKey; i++) {
	    if (index == GfKey[i].val) {
		return GfKey[i].descr;
	    }
	}
	if (isprint(index)) {
	    sprintf(buf, "%c", index);
	    return buf;
	}
	return NULL;
	break;
    default:
	break;
    }
    return NULL;
}


// First time (lazy) initialization.
void
gfctrlJoyInit(void)
{
#ifndef SDL_JOYSTICK
    gfctrlJoyPresent = GFCTRL_JOY_NONE;

    for (int index = 0; index < GFCTRL_JOY_NUMBER; index++) {
		if (!Joysticks[index]) {
			Joysticks[index] = new jsJoystick(index);
		}
    
		// Don't configure the joystick if it doesn't work
		if (Joysticks[index]->notWorking()) {
			delete Joysticks[index];
			Joysticks[index] = 0;
		} else {
			gfctrlJoyPresent = GFCTRL_JOY_PRESENT;
		}
    }
#else
#if SDL_MAJOR_VERSION >= 2
    memset(&cfx, 0, sizeof(cfx));

    if (SDL_InitSubSystem(SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC) < 0) {
#else
    if (SDL_Init(SDL_INIT_JOYSTICK) < 0) {
#endif
        GfLogError("Couldn't initialize SDL: %s\n", SDL_GetError());
        gfctrlJoyPresent = GFCTRL_JOY_UNTESTED;
	return;
    }
#if SDL_MAJOR_VERSION >= 2
    // Ignore the joystick events, we will poll directly as it is faster
    SDL_JoystickEventState(SDL_IGNORE);
#endif
    gfctrlJoyPresent = SDL_NumJoysticks();
    if (gfctrlJoyPresent > GFCTRL_JOY_NUMBER) gfctrlJoyPresent = GFCTRL_JOY_NUMBER;

    for (int index = 0; index < gfctrlJoyPresent; index++) {
		if (!Joysticks[index]) {
			Joysticks[index] = SDL_JoystickOpen(index);
		}
    
		// Don't configure the joystick if it doesn't work
		if (Joysticks[index] ==  NULL) {
			GfLogError("Couldn't open joystick %d: %s\n", index, SDL_GetError());
#if SDL_MAJOR_VERSION >= 2
		} else {
			cfx_timeout[index] = 0;
			rfx_timeout[index] = 0;
			
			// Find which Haptic device relates to this joystick
			Haptics[index] = SDL_HapticOpenFromJoystick(Joysticks[index]);

			if (!Haptics[index]) {
				GfLogInfo("Joystick %d does not support haptic\n", index);
				break;
#if 0
			} else {
				// add an CF effect on startup
				gfctrlJoyConstantForce(index, 50000, 9000);
#endif
			}

			// Check for Rumble capability
			if (SDL_HapticRumbleSupported(Haptics[index]) == SDL_TRUE) {
				if (SDL_HapticRumbleInit(Haptics[index]) != 0) 
					GfLogError("Couldn't init rumble on joystick %d: %s\n", index, SDL_GetError());
#if 0
				else
					gfctrlJoyRumble(index, 0.5);
#endif
			}
#endif
                }
     }
#endif
}

#if SDL_JOYSTICK
void
gfctrlJoyConstantForce(int index, unsigned int level, int dir)
{
#if SDL_MAJOR_VERSION >= 2
	if (!Haptics[index]) return;

	if ((SDL_HapticQuery(Haptics[index]) & SDL_HAPTIC_CONSTANT) == 0) return;

	cfx[index].type = SDL_HAPTIC_CONSTANT;
	cfx[index].constant.direction.type = SDL_HAPTIC_POLAR;
	cfx[index].constant.direction.dir[0] = dir;
	cfx[index].constant.length = 1000;
	cfx[index].constant.level = level;
	cfx[index].constant.attack_length = 0;
	cfx[index].constant.fade_length = 1000;

#if __WIN32__
	if (SDL_HapticGetEffectStatus(Haptics[index], id[index]) == SDL_TRUE)
#else
	// Linux SDL doesn't support checking status at the moment :-(
	if (cfx_timeout[index] > SDL_GetTicks())
#endif
		SDL_HapticUpdateEffect(Haptics[index], id[index], &cfx[index]);
	else {
		SDL_HapticDestroyEffect(Haptics[index], id[index]);
		id[index] = SDL_HapticNewEffect(Haptics[index], &cfx[index]);
		SDL_HapticRunEffect(Haptics[index], id[index], 1);
	}

	cfx_timeout[index] = SDL_GetTicks() + cfx[index].constant.length;
#endif
}

void
gfctrlJoyRumble(int index, float level)
{
#if SDL_MAJOR_VERSION >= 2
	if (!Haptics[index]) return;

	if (SDL_HapticRumbleSupported(Haptics[index]) != SDL_TRUE) return;

	// we have to stop the rumble before updating
	if (rfx_timeout[index] > SDL_GetTicks()) {
		if (SDL_HapticRumbleStop(Haptics[index]) != 0)
			GfLogError("Failed to stop rumble: %s\n", SDL_GetError() );
	}

	if (SDL_HapticRumblePlay(Haptics[index], level, 100) != 0)
		GfLogError("Failed to play rumble: %s\n", SDL_GetError() );

	rfx_timeout[index] = SDL_GetTicks() + 100;
#endif
}
#endif

// Shutdown time.
void
gfctrlJoyShutdown(void)
{
   if (gfctrlJoyPresent != GFCTRL_JOY_UNTESTED)
#ifndef SDL_JOYSTICK
	
		for (int index = 0; index < GFCTRL_JOY_NUMBER; index++)
			delete Joysticks[index];

	
#else
      for (int index = 0; index < gfctrlJoyPresent; index++) {
			SDL_JoystickClose(Joysticks[index]);
			Joysticks[index] = NULL;
#if SDL_MAJOR_VERSION >= 2
			if (Haptics[index]) {
				SDL_HapticClose(Haptics[index]);
				Haptics[index] = NULL;
			}			
#endif
		}
#endif
      gfctrlJoyPresent = GFCTRL_JOY_UNTESTED;
}

/** Create the joystick control
    @ingroup	ctrl
    @return	pointer on a tCtrlJoyInfo structure
		<br>0 .. if no joystick present
    @note	call GfctrlJoyRelease to free the tCtrlJoyInfo structure
    @see	GfctrlJoyRelease
    @see	tCtrlJoyInfo
*/
tCtrlJoyInfo *
GfctrlJoyCreate(void)
{
    if (gfctrlJoyPresent == GFCTRL_JOY_UNTESTED)
       gfctrlJoyInit();

    tCtrlJoyInfo* joyInfo = (tCtrlJoyInfo *)calloc(1, sizeof(tCtrlJoyInfo));

#if SDL_JOYSTICK
    joyInfoCopy = joyInfo;
#endif

    return joyInfo;
}

/** Release the tCtrlJoyInfo structure
    @ingroup	ctrl
    @param	joyInfo	joystick structure
    @return	none
*/
void
GfctrlJoyRelease(tCtrlJoyInfo *joyInfo)
{
    freez(joyInfo);
}


/** Check if any joystick is present
    @ingroup	ctrl
    @return	GFCTRL_JOY_NONE	if no joystick
		<br>GFCTRL_JOY_PRESENT if a joystick is present
*/
int
GfctrlJoyIsAnyPresent(void)
{
#if SDL_JOYSTICK
   if (gfctrlJoyPresent == 0)
#else
    if (gfctrlJoyPresent == GFCTRL_JOY_UNTESTED)
#endif
		gfctrlJoyInit();

    return gfctrlJoyPresent;
}
#if SDL_MAJOR_VERSION >= 2
#if SDL_JOYSTICK
int
GfctrlSDL2JoyGetCurrentStates(tCtrlJoyInfo *joyInfo)
{
   int			ind;
   int			i,j;
   unsigned int	b;
   unsigned int	mask;
   if (gfctrlJoyPresent == GFCTRL_JOY_PRESENT)
   {
      // Update all the joysticks
      SDL_JoystickUpdate();
      for (ind = 0; ind < gfctrlJoyPresent; ind++) 
      {
         if (Joysticks[ind])
         {
            j = SDL_JoystickNumAxes(Joysticks[ind]);
            if (j > GFCTRL_JOY_MAX_AXES) j = GFCTRL_JOY_MAX_AXES;

            for (i=0; i < j;i++)
               joyInfo->ax[GFCTRL_JOY_MAX_AXES * ind + i] = ((float) SDL_JoystickGetAxis(Joysticks[ind],i)) / 32768;

            b = 0;
            for (i=0; i < GFCTRL_JOY_MAX_BUTTONS;i++) 
            {
               mask = (unsigned int)SDL_JoystickGetButton(Joysticks[ind], i);
               b |= (mask << i);
            }

            /* Joystick buttons */
            for (i = 0, mask = 1; i < GFCTRL_JOY_MAX_BUTTONS; i++, mask *= 2) 
            {
               if (((b & mask) != 0) && ((joyInfo->oldb[ind] & mask) == 0)) {
                  joyInfo->edgeup[i + GFCTRL_JOY_MAX_BUTTONS * ind] = 1;
               } else {
                  joyInfo->edgeup[i + GFCTRL_JOY_MAX_BUTTONS * ind] = 0;
               }
               if (((b & mask) == 0) && ((joyInfo->oldb[ind] & mask) != 0)) {
                  joyInfo->edgedn[i + GFCTRL_JOY_MAX_BUTTONS * ind] = 1;
               } else {
                  joyInfo->edgedn[i + GFCTRL_JOY_MAX_BUTTONS * ind] = 0;
               }
               if ((b & mask) != 0) {
                  joyInfo->levelup[i + GFCTRL_JOY_MAX_BUTTONS * ind] = 1;
               } else {
                  joyInfo->levelup[i + GFCTRL_JOY_MAX_BUTTONS * ind] = 0;
               }
            }
            joyInfo->oldb[ind] = b;
         }
      }
   }
   else
   {
      return -1;
   }
   return 0;
}
#endif
#endif
/** Get the current state of the joysticks
    @ingroup	ctrl
    @param	joyInfo	Target joystick structure
    @return	<tt>0 ... </tt>Ok
		<br><tt>-1 .. </tt>Error
    @note	The tCtrlJoyInfo structure is updated with the new values
*/
int
GfctrlJoyGetCurrentStates(tCtrlJoyInfo *joyInfo)
{
#if SDL_MAJOR_VERSION >= 2
#ifdef SDL_JOYSTICK
   return GfctrlSDL2JoyGetCurrentStates(joyInfo);
#endif
#endif
    int			ind;
#ifndef SDL_JOYSTICK
    int			i;
    int			b;
    unsigned int	mask;

    if (gfctrlJoyPresent == GFCTRL_JOY_PRESENT) {
    	for (ind = 0; ind < GFCTRL_JOY_NUMBER; ind++) {
	    if (Joysticks[ind]) {
		Joysticks[ind]->read(&b, &(joyInfo->ax[GFCTRL_JOY_MAX_AXES * ind]));

		/* Joystick buttons */
		for (i = 0, mask = 1; i < GFCTRL_JOY_MAX_BUTTONS; i++, mask *= 2) {
		    if (((b & mask) != 0) && ((joyInfo->oldb[ind] & mask) == 0)) {
			joyInfo->edgeup[i + GFCTRL_JOY_MAX_BUTTONS * ind] = 1;
		    } else {
			joyInfo->edgeup[i + GFCTRL_JOY_MAX_BUTTONS * ind] = 0;
		    }
		    if (((b & mask) == 0) && ((joyInfo->oldb[ind] & mask) != 0)) {
			joyInfo->edgedn[i + GFCTRL_JOY_MAX_BUTTONS * ind] = 1;
		    } else {
			joyInfo->edgedn[i + GFCTRL_JOY_MAX_BUTTONS * ind] = 0;
		    }
		    if ((b & mask) != 0) {
			joyInfo->levelup[i + GFCTRL_JOY_MAX_BUTTONS * ind] = 1;
		    } else {
			joyInfo->levelup[i + GFCTRL_JOY_MAX_BUTTONS * ind] = 0;
		    }
		}
		joyInfo->oldb[ind] = b;
	    }
	}
    } else {
	return -1;
    }
#else
    int			i,j;

    if (gfctrlJoyPresent != GFCTRL_JOY_UNTESTED) {
    	for (ind = 0; ind < gfctrlJoyPresent; ind++) {

            // This polling is missing sometimes, skiping buttons or leaving axes stuck on
     if (Joysticks[ind]) {
#if SDL_MAJOR_VERSION < 2
		// poll each axis 
		// note: for SDL2 use events, set in GfctrlJoySetAxis()
		j = SDL_JoystickNumAxes(Joysticks[ind]);
		if (j > GFCTRL_JOY_MAX_AXES) j = GFCTRL_JOY_MAX_AXES;

		for (i=0; i < j;i++)
                    joyInfo->ax[GFCTRL_JOY_MAX_AXES * ind + i] = ((float) SDL_JoystickGetAxis(Joysticks[ind],i)) / 32768;
#endif

		// read each button
		j = SDL_JoystickNumButtons(Joysticks[ind]);
		if (j > GFCTRL_JOY_MAX_BUTTONS) j = GFCTRL_JOY_MAX_BUTTONS;
 
		for (i=0; i < j;i++) {
#if SDL_MAJOR_VERSION < 2
		    GfctrlJoySetButton(ind, i, (int) SDL_JoystickGetButton(Joysticks[ind], i));
#else
		    // buttons are read set by events, we just pass them up when requested

                    if (joyInfo->oldb[GFCTRL_JOY_MAX_BUTTONS * ind + i] == joyInfo->levelup[GFCTRL_JOY_MAX_BUTTONS * ind + i]) {
			// already informed system of event up/down already, so clear it
 			joyInfo->edgeup[i + GFCTRL_JOY_MAX_BUTTONS * ind] = 0;
         joyInfo->edgedn[i + GFCTRL_JOY_MAX_BUTTONS * ind] = 0;
		   }

                    // store button for next round
                    joyInfo->oldb[GFCTRL_JOY_MAX_BUTTONS * ind + i] = joyInfo->levelup[GFCTRL_JOY_MAX_BUTTONS * ind + i];
#endif
 		}
 	    }
 	}
     } else {
 	return -1;
     }
#endif
    return 0;
}

#if SDL_JOYSTICK
void
GfctrlJoySetAxis(int joy, int axis, float value)
{
	if (joyInfoCopy == NULL) return;

	joyInfoCopy->ax[GFCTRL_JOY_MAX_AXES * joy + axis] = value;
	//GfLogInfo("SDL Joy %d Axis %d = Value %f\n", joy, axis, value);
}

void
GfctrlJoySetButton(int joy, int button, int value)
{
	if (joyInfoCopy == NULL) return;

	if ((value == SDL_PRESSED) && !joyInfoCopy->levelup[GFCTRL_JOY_MAX_BUTTONS * joy + button] ) {
	    joyInfoCopy->edgeup[GFCTRL_JOY_MAX_BUTTONS * joy + button] = 1;
	} else {
	    joyInfoCopy->edgeup[GFCTRL_JOY_MAX_BUTTONS * joy + button] = 0;
	}

	if ((value != SDL_PRESSED) && joyInfoCopy->levelup[GFCTRL_JOY_MAX_BUTTONS * joy + button] ) {
	    joyInfoCopy->edgedn[GFCTRL_JOY_MAX_BUTTONS * joy + button] = 1;
	} else {
	    joyInfoCopy->edgedn[GFCTRL_JOY_MAX_BUTTONS * joy + button] = 0;
	}

	if (value == SDL_PRESSED) {
	    joyInfoCopy->levelup[GFCTRL_JOY_MAX_BUTTONS * joy + button] = 1;
	} else {
	    joyInfoCopy->levelup[GFCTRL_JOY_MAX_BUTTONS * joy + button] = 0;
	}
}
#endif

/** Initialize the mouse control
    @ingroup	ctrl
    @return	pointer on a tCtrlMouseInfo structure
		<br>0 .. if no mouse present
    @note	call GfctrlMouseRelease to free the tCtrlMouseInfo structure
    @see	GfctrlMouseRelease
*/
tCtrlMouseInfo *
GfctrlMouseCreate(void)
{
    tCtrlMouseInfo* mouseInfo = (tCtrlMouseInfo *)calloc(1, sizeof(tCtrlMouseInfo));

    return mouseInfo;
}

/** Release the tCtrlMouseInfo structure
    @ingroup	ctrl
    @param	mouseInfo	mouse structure
    @return	none
*/
void
GfctrlMouseRelease(tCtrlMouseInfo *mouseInfo)
{
    free(mouseInfo);
}

static tMouseInfo refMouse;

/** Get the mouse current values
    @ingroup	ctrl
    @param	mouseInfo	mouse structure
    @return	<tt>0 ... </tt>Ok
		<br><tt>-1 .. </tt>Error
    @note	The tCtrlMouseInfo structure is updated with the new values
*/
int
GfctrlMouseGetCurrentState(tCtrlMouseInfo *mouseInfo)
{
    float	mouseMove;
    tMouseInfo	*mouse;
    int		i;

    mouse = GfuiMouseInfo();

    mouseMove = (float)(refMouse.X - mouse->X);
    
    if (mouseMove < 0) {
	mouseInfo->ax[1] = -mouseMove;
	mouseInfo->ax[0] = 0;
    } else {
	mouseInfo->ax[0] = mouseMove;
	mouseInfo->ax[1] = 0;
    }
    mouseMove = (float)(refMouse.Y - mouse->Y);
    if (mouseMove < 0) {
	mouseInfo->ax[2] = -mouseMove;
	mouseInfo->ax[3] = 0;
    } else {
	mouseInfo->ax[3] = mouseMove;
	mouseInfo->ax[2] = 0;
    }
    for (i = 0; i < 7; i++) {
	if (mouseInfo->button[i] != mouse->button[i]) {
	    if (mouse->button[i]) {
		mouseInfo->edgedn[i] = 1;
		mouseInfo->edgeup[i] = 0;
	    } else {
		mouseInfo->edgeup[i] = 1;
		mouseInfo->edgedn[i] = 0;
	    }
	    mouseInfo->button[i] = mouse->button[i];
	} else {
	    mouseInfo->edgeup[i] = 0;
	    mouseInfo->edgedn[i] = 0;
	}
    }
    return 0;
}


/** Recentre the mouse on the screen.
    @ingroup	ctrl
    @return	none
*/
void
GfctrlMouseCenter(void)
{
    int sw, sh, vw, vh;

    GfScrGetSize(&sw, &sh, &vw, &vh);
    GfuiMouseSetPos(sw / 2, sh / 2);
}

/** Get the reference position.
    @ingroup	ctrl
    @return	none
*/
void
GfctrlMouseInitCenter(void)
{
    memcpy(&refMouse, GfuiMouseInfo(), sizeof(refMouse));
}
