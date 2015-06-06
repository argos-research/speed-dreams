/***************************************************************************
                       guihelp.cpp -- automatic help on keys                                
                             -------------------                                         
    created              : Fri Aug 13 22:20:37 CEST 1999
    copyright            : (C) 1999 by Eric Espie                         
    email                : torcs@free.fr   
    version              : $Id: guihelp.cpp 5528 2013-06-22 16:15:55Z beaglejoe $                                  
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
    		GUI help screen management.
    @author	<a href=mailto:torcs@free.fr>Eric Espie</a>
    @version	$Id: guihelp.cpp 5528 2013-06-22 16:15:55Z beaglejoe $
    @ingroup	gui
*/

#include <cstdlib>

#include "gui.h"


static int NRecursions = 0;

void
gfuiInitHelp(void)
{
}

static void
onActivate(void* /* dummy */)
{
	NRecursions++;
}

static void
onDeactivate(void* /* dummy */)
{
	NRecursions--;
}

/** Generate a help screen.
    @ingroup	gui
    @param	targetScreen	The screen to display help for and to return to when exiting
    @warning	The help screen is activated.
 */
void
GfuiHelpScreen(void *targetScreen)
{
	GfuiHelpScreen(targetScreen, 0);
}

/** Generate a help screen.
    @ingroup	gui
    @param	targetScreen	The screen to display help for
    @param	returnScreen	The screen to return to when exiting
    @warning	The help screen is activated.
 */
void
GfuiHelpScreen(void *targetScreen, void *returnScreen)
{
	// The return screen is the target screen if not specified (0).
	if (!returnScreen)
		returnScreen = targetScreen;

    // Create help screen, load menu XML descriptor and create static controls.
    void* scrHandle = GfuiScreenCreate(0, 0, onActivate, 0, onDeactivate);
    
    void *hmenu = GfuiMenuLoad("helpmenu.xml");

    GfuiMenuCreateStaticControls(scrHandle, hmenu);

	// Get menu properties.
	const int nXLeftColumn = (int)GfuiMenuGetNumProperty(hmenu, "xLeftColumn", 30);
	const int nXRightColumn = (int)GfuiMenuGetNumProperty(hmenu, "xRightColumn", 330);
	const int nNameFieldWidth = (int)GfuiMenuGetNumProperty(hmenu, "nameFieldWidth", 80);
	const int nLineShift = (int)GfuiMenuGetNumProperty(hmenu, "lineShift", 12);
	const int nYTopLine = (int)GfuiMenuGetNumProperty(hmenu, "yTopLine", 380);

    // Create 2 columns table for the keyboard shortcuts explanations
    int ys = nYTopLine;
    int yn = nYTopLine;
    
    tGfuiScreen	*pscrTgt = (tGfuiScreen*)targetScreen;

    tGfuiKey *curKey = pscrTgt->userKeys;
	while (curKey)
	{
		curKey = curKey->next;
		
		// Decide if this key goes on the left of right column.
		bool bLeft;
		switch(curKey->key) {
			case GFUIK_BACKSPACE:
			case GFUIK_F1:
			case GFUIK_F2:
			case GFUIK_F3:
			case GFUIK_F4:
			case GFUIK_F5:
			case GFUIK_F6:
			case GFUIK_F7:
			case GFUIK_F8:
			case GFUIK_F9:
			case GFUIK_F10:
			case GFUIK_F11:
			case GFUIK_F12:
			case GFUIK_LEFT:
			case GFUIK_UP:
			case GFUIK_RIGHT:
			case GFUIK_DOWN:
			case GFUIK_PAGEUP:
			case GFUIK_PAGEDOWN:
			case GFUIK_HOME:
			case GFUIK_END:
			case GFUIK_INSERT:
			case GFUIK_DELETE:
			case GFUIK_CLEAR:
			case GFUIK_PAUSE:
				bLeft = true;
				break;

			default:
				bLeft = curKey->modifier != GFUIM_NONE;
				break;
		}

		// Determine control coordinates, whether left or right column.
		int x, y;
		if (bLeft)
		{
			x = nXLeftColumn;
			y = ys;
			ys -= nLineShift;
		}
		else
		{
			x = nXRightColumn;
			y = yn;
			yn -= nLineShift;
		}

		// Create label controls.
		GfuiMenuCreateLabelControl(scrHandle, hmenu, "keyName", true, // from template
								   curKey->name, x, y);
		GfuiMenuCreateLabelControl(scrHandle, hmenu, "keyDesc", true, // from template
								   curKey->descr, x + nNameFieldWidth, y);

		// Stop if no more keys to explain.
		if (curKey == pscrTgt->userKeys)
			curKey = (tGfuiKey*)NULL;

    }
    

    // Create Back button.
    GfuiMenuCreateButtonControl(scrHandle, hmenu, "backbutton", returnScreen, GfuiScreenReplace);

    // Create version label.
    const int versionId = GfuiMenuCreateLabelControl(scrHandle, hmenu, "versionlabel");
    GfuiLabelSetText(scrHandle, versionId, GfuiApp().version().c_str());

    // Close menu XML descriptor.
    GfParmReleaseHandle(hmenu);
    
    // Add keyboard shortcuts.
    GfuiAddKey(scrHandle, GFUIK_ESCAPE, "Back to the menu", returnScreen, GfuiScreenReplace, NULL);
    GfuiAddKey(scrHandle, GFUIK_RETURN, "Back to the menu", returnScreen, GfuiScreenReplace, NULL);
	if (NRecursions == 0)
		GfuiAddKey(scrHandle, GFUIK_F1, "Help on Help menu", scrHandle, GfuiHelpScreen, NULL);
    GfuiAddKey(scrHandle, GFUIK_F12, "Screen-shot", NULL, GfuiScreenShot, NULL);
    GfuiAddKey(scrHandle, GFUIK_UP, "Select previous entry", NULL, gfuiSelectPrev, NULL);
    GfuiAddKey(scrHandle, GFUIK_DOWN, "Select next entry", NULL, gfuiSelectNext, NULL);

    GfuiScreenActivate(scrHandle);
}
