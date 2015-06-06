/***************************************************************************

    file        : fileselect.cpp
    created     : Sun Feb 16 13:09:23 CET 2003
    copyright   : (C) 2003 by Eric Espie                       
    email       : eric.espie@torcs.org   
    version     : $Id: fileselect.cpp 4495 2012-02-12 15:26:59Z pouillot $
 ***************************************************************************/

/***************************************************************************
 *																		 *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or	 *
 *   (at your option) any later version.								   *
 *																		 *
 ***************************************************************************/

/** @file   
			Files open/save menu screens.
	@ingroup	racemantools
	@author	<a href=mailto:eric.espie@torcs.org>Eric Espie</a>
	@version	$Id: fileselect.cpp 4495 2012-02-12 15:26:59Z pouillot $
*/


#include <tgfclient.h>

#include "racescreens.h"


static void		*ScrHandle = NULL;

static int		FilesScrollListId;
static int		FileNameEditId;
static int		LoadButtonId;
static int		SaveButtonId;

static tRmFileSelect	*RmFs;
static tFList		*FileList = NULL;
static tFList		*FileSelected;

static void
rmOnActivate(void * /* dummy */ )
{
	GfLogTrace("Entering File Select menu (filter: %s/%s*%s)\n",
			   RmFs->dirPath.c_str(), RmFs->namePrefix.c_str(), RmFs->nameSuffix.c_str());

	// Fill-in the Scroll List with the names of the files in the specified folder.
	GfuiScrollListClear(ScrHandle, FilesScrollListId);
	
	FileList = GfDirGetListFiltered(RmFs->dirPath.c_str(),
									RmFs->namePrefix.c_str(), RmFs->nameSuffix.c_str());
	if (FileList)
	{
		tFList	*fileCur;

		FileSelected = FileList;
		fileCur = FileList;
		do {
			fileCur = fileCur->next;
			GfuiScrollListInsertElement(ScrHandle, FilesScrollListId, fileCur->name, 1000, (void*)fileCur);
		} while (fileCur != FileList);
	}

	// Clear the file name edit box.
    GfuiEditboxSetString(ScrHandle, FileNameEditId, "");

	// Show/Hide Load/Save buttons according to the file selection mode.
	GfuiVisibilitySet(ScrHandle, LoadButtonId, 
					  RmFs->mode == RmFSModeLoad ? GFUI_VISIBLE : GFUI_INVISIBLE);
	GfuiVisibilitySet(ScrHandle, SaveButtonId, 
					  RmFs->mode == RmFSModeSave ? GFUI_VISIBLE : GFUI_INVISIBLE);

	// Inhibit file name control editbox if only loading.
	GfuiEnable(ScrHandle, FileNameEditId,
			   RmFs->mode == RmFSModeLoad ? GFUI_DISABLE : GFUI_ENABLE);
}

static void
rmOnClickOnFile(void * /*dummy*/)
{
	GfuiScrollListGetSelectedElement(ScrHandle, FilesScrollListId, (void**)&FileSelected);
    GfuiEditboxSetString(ScrHandle, FileNameEditId, FileSelected->name);
}

static void
rmOnChangeFileName(void * /* dummy */)
{
}

static void
rmOnDeactivate(void * /* dummy */ )
{
    // Force current edit to loose focus (if one has it) and update associated variable.
    GfuiUnSelectCurrent();

	// Free allocated memory for the folder file list.
	if (FileList) {
		GfDirFreeList(FileList, NULL);
		FileList = NULL;
	}

	// Fire the return screen.
	GfuiScreenActivate(RmFs->prevScreen);
}

static void
rmOnSelect(void * /* dummy */ )
{
    char* pszFileName = GfuiEditboxGetString(ScrHandle, FileNameEditId);

	if (pszFileName && strlen(pszFileName) > 0)
	{
		RmFs->select(pszFileName);
		rmOnDeactivate(0);
	}
}

/** File selection
	@param	pFileSelect	Pointer on tRmFileSelect structure (cast to void)
	@return	none
*/
void*
RmFileSelect(void *pFileSelect)
{
	RmFs = (tRmFileSelect*)pFileSelect;

	if (ScrHandle)
		return ScrHandle;

	// Create screen, load menu XML descriptor and create static controls.
	ScrHandle = GfuiScreenCreate(NULL, NULL, rmOnActivate, NULL, NULL, 1);

	void *menuXMLDescHdle = GfuiMenuLoad("fileselectmenu.xml");

	GfuiMenuCreateStaticControls(ScrHandle, menuXMLDescHdle);

	// Create variable title label.
	const int titleId = GfuiMenuCreateLabelControl(ScrHandle, menuXMLDescHdle, "TitleLabel");
	GfuiLabelSetText(ScrHandle, titleId, RmFs->title.c_str());
	
	// Create the Scroll List containing the File list
	FilesScrollListId = GfuiMenuCreateScrollListControl(ScrHandle, menuXMLDescHdle, "FilesScrollList",
														NULL, rmOnClickOnFile);

	// Create the filename edit box
    FileNameEditId = GfuiMenuCreateEditControl(ScrHandle, menuXMLDescHdle, "SelectedFileNameEdit",
											   NULL, NULL, rmOnChangeFileName);

	// Create Load/Save and Cancel buttons.
	LoadButtonId = GfuiMenuCreateButtonControl(ScrHandle, menuXMLDescHdle, "LoadButton", NULL, rmOnSelect);
	SaveButtonId = GfuiMenuCreateButtonControl(ScrHandle, menuXMLDescHdle, "SaveButton", NULL, rmOnSelect);
	GfuiMenuCreateButtonControl(ScrHandle, menuXMLDescHdle, "CancelButton", NULL, rmOnDeactivate);

	// Close menu XML descriptor.
	GfParmReleaseHandle(menuXMLDescHdle);
	
	// Register keyboard shortcuts.
	GfuiAddKey(ScrHandle, GFUIK_ESCAPE, "Cancel", NULL, rmOnDeactivate, NULL);
	GfuiMenuDefaultKeysAdd(ScrHandle);

	return ScrHandle;
}
