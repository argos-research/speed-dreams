/***************************************************************************

    file                 : optionsmenu.cpp
    created              : Mon Apr 24 14:22:53 CEST 2000
    copyright            : (C) 2000, 2001 by Eric Espie
    email                : torcs@free.fr
    version              : $Id: optionsmenu.cpp 4986 2012-10-07 18:32:50Z pouillot $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <tgfclient.h>

#include <displayconfig.h>
#include <monitorconfig.h>
#include <graphconfig.h>
#include <advancedgraphconfig.h>
#include <openglconfig.h>
#include <soundconfig.h>
#include <simuconfig.h>
#include <aiconfig.h>

#include "optionsmenu.h"


static void *MenuHandle = NULL;

// SDW hack to get access to Monitor menu, doesn't have a defined position yet
// (Uncomment to select Monitor menu otherwise, the Display menu is used)
//#define MonitorMenu 1

#ifndef MonitorMenu

static void
onDisplayMenuActivate(void * /* dummy */)
{
    GfuiScreenActivate(DisplayMenuInit(MenuHandle));
}

#else

static void
onMonitorMenuActivate(void * /* dummy */)
{
    GfuiScreenActivate(MonitorMenuInit(MenuHandle));
}

#endif

static void
onGraphMenuActivate(void * /* dummy */)
{
    GfuiScreenActivate(GraphMenuInit(MenuHandle));
}

static void
onAdvancedGraphMenuActivate(void * /*dummy */)
{
	GfuiScreenActivate(AdvancedGraphMenuInit(MenuHandle));
}

static void
onOpenGLMenuActivate(void * /* dummy */)
{
    GfuiScreenActivate(OpenGLMenuInit(MenuHandle));
}

static void
onSoundMenuActivate(void * /* dummy */)
{
    GfuiScreenActivate(SoundMenuInit(MenuHandle));
}

static void
onSimuMenuActivate(void * /* dummy */)
{
    GfuiScreenActivate(SimuMenuInit(MenuHandle));
}

static void
onAIMenuActivate(void * /* dummy */)
{
    GfuiScreenActivate(AIMenuInit(MenuHandle));
}

void *
OptionsMenuInit(void *prevMenu)
{
    if (MenuHandle) 
		return MenuHandle;

    MenuHandle = GfuiScreenCreate((float*)NULL, NULL, NULL, NULL, (tfuiCallback)NULL, 1);

    void *param = GfuiMenuLoad("optionsmenu.xml");

    GfuiMenuCreateStaticControls(MenuHandle, param);
    
// SDW hack to get access to Monitor menu, doesn't have a defined position yet
#ifdef MonitorMenu
    GfuiMenuCreateButtonControl(MenuHandle, param, "display", NULL, onMonitorMenuActivate);
#else
    GfuiMenuCreateButtonControl(MenuHandle, param, "display", NULL, onDisplayMenuActivate);
#endif
    GfuiMenuCreateButtonControl(MenuHandle, param, "graphic", NULL, onGraphMenuActivate);
//#if _ADVANCED // CMAKE OPTION ADVANCED
	GfuiMenuCreateButtonControl(MenuHandle, param, "advanced", NULL, onAdvancedGraphMenuActivate);
//#endif

    GfuiMenuCreateButtonControl(MenuHandle, param, "opengl", NULL, onOpenGLMenuActivate);
    GfuiMenuCreateButtonControl(MenuHandle, param, "sound", NULL, onSoundMenuActivate);
    GfuiMenuCreateButtonControl(MenuHandle, param, "simulation", NULL, onSimuMenuActivate);
    GfuiMenuCreateButtonControl(MenuHandle, param, "ai", NULL, onAIMenuActivate);
    GfuiMenuCreateButtonControl(MenuHandle, param, "back", prevMenu, GfuiScreenActivate);

    GfParmReleaseHandle(param);

    GfuiMenuDefaultKeysAdd(MenuHandle);
    GfuiAddKey(MenuHandle, GFUIK_ESCAPE, "Back", prevMenu, GfuiScreenActivate, NULL);

    return MenuHandle;
}
