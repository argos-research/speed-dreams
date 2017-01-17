/***************************************************************************
                          tgfclient.cpp -- The Gaming Framework UI
                             -------------------                                         
    created              : Fri Aug 13 22:31:43 CEST 1999
    copyright            : (C) 1999 by Eric Espie
    email                : torcs@free.fr
    version              : $Id: tgfclient.cpp 5854 2014-11-23 17:55:52Z wdbee $
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "gui.h"

// Use new Memory Manager ...
#ifdef __DEBUG_MEMORYMANAGER__
#include "memmanager.h"

// Avoid memory leaks ...
int NextScreenID = 0;
int NumberOfScreens = 0;
tGfuiScreen* OwnerOfScreens[MAXSCREENS];

// Register all screens that are allocated
void RegisterScreens(void* screen)
{
	tGfuiScreen* _screen = (tGfuiScreen*) screen;
	_screen->ScreenID = ++NextScreenID;

	// Find a deleted entry
	for (int I = 0; I < NumberOfScreens; I++)
	{
		if (OwnerOfScreens[I] == NULL)
		{
			OwnerOfScreens[I] = _screen;
			return;
		}
	}

	if (NumberOfScreens < MAXSCREENS)
		OwnerOfScreens[NumberOfScreens++] = _screen;
	else
		GfLogInfo("NumberOfScreens: %d > MAXSCREENS\n", NumberOfScreens);
}

// Unregister all screens that are released
void UnregisterScreens(void* screen)
{
	// Find the entry
	for (int I = 0; I <= NumberOfScreens; I++)
	{
		if (OwnerOfScreens[I] == screen)
		{
			OwnerOfScreens[I] = NULL;
			return;
		}
	}
}

// Free screens that are stil allocated
void FreeScreens()
{
	// For debugging purposes:
	//doaccept(); // Do not free the blocks, just take it out of the list

	for (int I = 0; I < NumberOfScreens; I++)
	{
		// Get the screen from the owner
		tGfuiScreen* screen = OwnerOfScreens[I];
		if (screen) 
		{
			fprintf(stderr,"Unreleased screen: %d\n",screen->ScreenID);
			ScreenRelease(screen); // Free all resources
		}
	}

	// Back to normal mode
	dofree(); // Free the blocks

}

// Free screen
void FreeScreen(void* screen)
{
	if (screen) 
	{
		tGfuiScreen* _screen = (tGfuiScreen*) screen;
		fprintf(stderr,"Release screen: %d\n",_screen->ScreenID);
		ScreenRelease(screen); // Free all resources
	}
}

// Free screen
void ScreenRelease(void* scr)
{
	tGfuiObject *curObject;
	tGfuiObject *nextObject;
	tGfuiKey *curKey;
	tGfuiKey *nextKey;
	tGfuiScreen *screen = (tGfuiScreen*)scr;

	UnregisterScreens(screen);

	if (GfuiScreen == screen) {
		GfuiScreenDeactivate();
	}

	if (screen->bgImage != 0) {
		glDeleteTextures(1, &screen->bgImage);
	}

	curObject = screen->objects;
	if (curObject != NULL) {
		do {
			nextObject = curObject->next;
			gfuiReleaseObject(curObject);
			curObject = nextObject;
		} while (curObject != screen->objects);
	}

	curKey = screen->userKeys;
	if (curKey != NULL) {
		do {
			nextKey = curKey->next;
			free(curKey->name);
			free(curKey->descr);
			free(curKey);
			curKey = nextKey;
		} while (curKey != screen->userKeys);
	}
	if(screen->musicFilename != NULL) {
		free(screen->musicFilename);
	}
	free(screen);
}

// ... Avoid memory leaks
#else
void RegisterScreens(void* screen){};
void FreeScreens(){};
void FreeScreen(tGfuiScreen* screen){}
void ScreenRelease(void* scr){}
void UnregisterScreens(void* screen){};
#endif
// ... Use new Memory Manager

void GfuiInit(void)
{
    gfuiInit();
}

void GfuiShutdown(void)
{
    gfuiShutdown();

	// Free screens that are stil allocated
	FreeScreens();
	
	GfScrShutdown();
}
