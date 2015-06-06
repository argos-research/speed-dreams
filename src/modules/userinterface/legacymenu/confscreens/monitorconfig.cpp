/***************************************************************************

    file                 : monitorconfig.cpp
    created              : October 2010
    copyright            : (C) 2010 Jean-Philippe Meuret
    web                  : speed-dreams.sourceforge.net
    version              : $Id$

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

/* Display configuration menu */

#include <sstream>

#include <tgfclient.h>

#include "legacymenu.h"
#include "monitorconfig.h"
#include "graphic.h"


// Some consts.
static const char* AMonitorTypes[MonitorMenu::nMonitorTypes] = { "4:3", "16:9" };
static const char* ASpanSplits[MonitorMenu::nSpanSplits] = { "Disabled", "Enabled" };

static float _nBezelComp;
static int nBezelCompID;

// The unique MonitorMenu instance.
static MonitorMenu* PMonitorMenu = 0;


// Call-backs ================================================================
void MonitorMenu::onActivate(void *pMonitorMenu)
{
	// Get the MonitorMenu instance.
	MonitorMenu* pMenu = static_cast<MonitorMenu*>(pMonitorMenu);

	// Load settings from XML file.
	pMenu->loadSettings();

	// Initialize GUI from loaded values.
	pMenu->updateControls();
}

void MonitorMenu::onChangeMonitorType(tComboBoxInfo *pInfo)
{
 	// Get the MonitorMenu instance from call-back user data.
	MonitorMenu* pMenu = static_cast<MonitorMenu*>(pInfo->userData);

	pMenu->setMonitorType((EMonitorType)pInfo->nPos);
}

void MonitorMenu::onChangeSpanSplit(tComboBoxInfo *pInfo)
{
 	// Get the MonitorMenu instance from call-back user data.
	MonitorMenu* pMenu = static_cast<MonitorMenu*>(pInfo->userData);

	pMenu->setSpanSplit((ESpanSplit)pInfo->nPos);
}

void MonitorMenu::onChangeBezelComp(void *)
{
    char* val = GfuiEditboxGetString(PMonitorMenu->getMenuHandle(), nBezelCompID);
    sscanf(val, "%g", &_nBezelComp);
    if (_nBezelComp > 120.0f)
		_nBezelComp = 100.0f;
    else if (_nBezelComp < 80.0f)
		_nBezelComp = 80.0f;
	
    char buf[32];
    sprintf(buf, "%g", _nBezelComp);
    GfuiEditboxSetString(PMonitorMenu->getMenuHandle(), nBezelCompID, buf);
#if 0
 	// Get the MonitorMenu instance from call-back user data.
	MonitorMenu* pMenu = static_cast<MonitorMenu*>(pInfo->userData);

	pMenu->setBezelComp((float)pInfo->nPos);
#endif
}

// Re-init screen to take new graphical settings into account (implies process restart).
void MonitorMenu::onAccept(void *pMonitorMenu)
{
	// Get the MonitorMenu instance from call-back user data.
	MonitorMenu* pMenu = static_cast<MonitorMenu*>(pMonitorMenu);

	// Force current control to loose focus (if one had it) and update associated variable.
	GfuiUnSelectCurrent();

	// Save display settings.
	pMenu->storeSettings();

	// Back to previous screen.
	GfuiScreenActivate(pMenu->getPreviousMenuHandle());
}

void MonitorMenu::onCancel(void *pMonitorMenu)
{
	// Get the MonitorMenu instance from call-back user data.
	const MonitorMenu* pMenu = static_cast<MonitorMenu*>(pMonitorMenu);

	// Back to previous screen.
	GfuiScreenActivate(pMenu->getPreviousMenuHandle());
}

void MonitorMenu::updateControls()
{
	int nControlId = getDynamicControlId("MonitorCombo");
	GfuiComboboxSetSelectedIndex(getMenuHandle(), nControlId, _eMonitorType);
	
	nControlId = getDynamicControlId("SpanSplitCombo");
	GfuiComboboxSetSelectedIndex(getMenuHandle(), nControlId, _eSpanSplit);

	nControlId = getDynamicControlId("BezelCompCombo");
	GfuiComboboxSetSelectedIndex(getMenuHandle(), nControlId, _nBezelComp);
}

void MonitorMenu::loadSettings()
{
	std::ostringstream ossConfFile;
	ossConfFile << GfLocalDir() << GR_PARAM_FILE;
	void* grHandle = 
		GfParmReadFile(ossConfFile.str().c_str(), GFPARM_RMODE_STD | GFPARM_RMODE_CREAT);

	// Monitor Type : 4:3 or 16:9
	const char *pszMonitorType =
		GfParmGetStr(grHandle, GR_SCT_MONITOR, GR_ATT_MONITOR, GR_VAL_MONITOR_16BY9);
	_eMonitorType = strcmp(pszMonitorType, GR_VAL_MONITOR_16BY9) ? e4by3: e16by9;

	// Span Split Screens
	const char *pszSpanSplit =
		GfParmGetStr(grHandle, GR_SCT_MONITOR, GR_ATT_SPANSPLIT, GR_VAL_NO);
	_eSpanSplit = strcmp(pszSpanSplit, GR_VAL_YES) ? eDisabled : eEnabled;

	// Bezel Compensation
	_nBezelComp = (float)GfParmGetNum(grHandle, GR_SCT_MONITOR, GR_ATT_BEZELCOMP, NULL, 100);

	if (_nBezelComp > 120.0f)
		_nBezelComp = 100.0f;
	else if (_nBezelComp < 80.0f)
		_nBezelComp = 80.0f;
	
	char buf[32];
	sprintf(buf, "%g", _nBezelComp);
	GfuiEditboxSetString(PMonitorMenu->getMenuHandle(), nBezelCompID, buf);

	// Release screen config params file.
	GfParmReleaseHandle(grHandle);
}

// Save graphical settings to XML file.
void MonitorMenu::storeSettings() const
{
	std::ostringstream ossConfFile;
	ossConfFile << GfLocalDir() << GR_PARAM_FILE;
	void* grHandle =
		GfParmReadFile(ossConfFile.str().c_str(), GFPARM_RMODE_STD | GFPARM_RMODE_CREAT);

	const char* pszMonitorType =
		(_eMonitorType == e16by9) ? GR_VAL_MONITOR_16BY9 : GR_VAL_MONITOR_4BY3;
	GfParmSetStr(grHandle, GR_SCT_MONITOR, GR_ATT_MONITOR, pszMonitorType);

	const char* pszSpanSplit =
		(_eSpanSplit == eEnabled) ? GR_VAL_YES : GR_VAL_NO;
	GfParmSetStr(grHandle, GR_SCT_MONITOR, GR_ATT_SPANSPLIT, pszSpanSplit);

	GfParmSetNum(grHandle, GR_SCT_MONITOR, GR_ATT_BEZELCOMP, (char*)NULL, _nBezelComp);

	// Write and release screen config params file.
	GfParmWriteFile(NULL, grHandle, "Screen");
	GfParmReleaseHandle(grHandle);
}

void MonitorMenu::setMonitorType(EMonitorType eMode)
{
	_eMonitorType = eMode;
}

void MonitorMenu::setSpanSplit(ESpanSplit eMode)
{
	_eSpanSplit = eMode;
}

void MonitorMenu::setBezelComp(float value)
{
	_nBezelComp = value;
}

MonitorMenu::MonitorMenu()
: GfuiMenuScreen("monitorconfigmenu.xml")
{
	_eMonitorType = e16by9;
	_eSpanSplit = eDisabled;
	_nBezelComp = 1.0;
}

bool MonitorMenu::initialize(void *pPreviousMenu)
{
	// Save the menu to return to.
	setPreviousMenuHandle(pPreviousMenu);

	// Create the menu and all its controls.
	createMenu(NULL, this, onActivate, NULL, (tfuiCallback)NULL, 1);

	void *param = GfuiMenuLoad("monitorconfigmenu.xml");

	openXMLDescriptor();
    
	createStaticControls();
    
	const int nMonitorTypeComboId =
		createComboboxControl("MonitorTypeCombo", this, onChangeMonitorType);

	const int nSpanSplitComboId =
		createComboboxControl("SpanSplitCombo", this, onChangeSpanSplit);

	nBezelCompID = GfuiMenuCreateEditControl(getMenuHandle(), param, "BezelCompEdit", (void*)1, NULL, onChangeBezelComp);
	//	createComboboxControl("BezelCompEdit", this, onChangeBezelComp);

	createButtonControl("ApplyButton", this, onAccept);
	createButtonControl("CancelButton", this, onCancel);

	addShortcut(GFUIK_RETURN, "Apply", this, onAccept, 0);
	addShortcut(GFUIK_ESCAPE, "Cancel", this, onCancel, 0);
    // TODO Keyboard shortcuts: Add support for shortcuts in GfuiCombobox ?
	//addShortcut(GFUIK_LEFT, "Previous Resolution", this, onChangeScreenSize, 0);
	//addShortcut(GFUIK_RIGHT, "Next Resolution", this, onChangeScreenSize, 0);
	addShortcut(GFUIK_F1, "Help", getMenuHandle(), GfuiHelpScreen, 0);
	addShortcut(GFUIK_F12, "Screen-Shot", 0, GfuiScreenShot, 0);

	closeXMLDescriptor();

	// Load constant value lists in combo-boxes.
	// 1) Monitor Type
	for (int nMonitorTypeInd = 0; nMonitorTypeInd < nMonitorTypes; nMonitorTypeInd++)
		GfuiComboboxAddText(getMenuHandle(), nMonitorTypeComboId, AMonitorTypes[nMonitorTypeInd]);

	// 2) Span Split Screens - for wide displays
	for (int nSpanSplitInd = 0; nSpanSplitInd < nSpanSplits; nSpanSplitInd++)
		GfuiComboboxAddText(getMenuHandle(), nSpanSplitComboId, ASpanSplits[nSpanSplitInd]);

	return true;
}

/** Create and activate the monitor options menu screen.
    @ingroup	screen
    @param	precMenu	previous menu to return to
*/
void* MonitorMenuInit(void *pPreviousMenu)
{
	if (!PMonitorMenu)
	{
		PMonitorMenu = new MonitorMenu;
	
		PMonitorMenu->initialize(pPreviousMenu);
	}

	return PMonitorMenu->getMenuHandle();
}

