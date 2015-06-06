/***************************************************************************

    file                 : hostsettingsmenu.cpp
    created              : December 2009
    copyright            : (C) 2009 Brian Gavin
    web                  : speed-dreams.sourceforge.net

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


/*
This file deals with network host settings
*/

#include <cars.h>
#include <network.h>

#include "hostsettingsmenu.h"


static void *pPrevMenu = NULL;

std::string HostSettingsMenu::m_strCarCat;
bool HostSettingsMenu::m_bCollisions = true;
bool HostSettingsMenu::m_bHumanHost = true;


void HostSettingsMenu::onActivate(void *p)
{
}

void HostSettingsMenu::onCarControl(tComboBoxInfo *pInfo)
{
	m_strCarCat = pInfo->vecChoices[pInfo->nPos];
}

void HostSettingsMenu::onCarCollide(tComboBoxInfo *pInfo)
{
	if (pInfo->vecChoices[pInfo->nPos] == "Off")
		m_bCollisions = false;
	else
		m_bCollisions = true;

}
void HostSettingsMenu::onHumanHost(tComboBoxInfo *pInfo)
{
	if (pInfo->vecChoices[pInfo->nPos] == "Yes")
		m_bHumanHost = true;
	else
		m_bHumanHost = false;

}

void HostSettingsMenu::onPlayerReady(void *p)
{
	//TODO
}

void HostSettingsMenu::onAccept(void *p)
{
	NetGetServer()->SetHostSettings(m_strCarCat.c_str(),m_bCollisions);
	GfuiScreenActivate(pPrevMenu);
}

void HostSettingsMenu::onCancel(void *p)
{
	GfuiScreenActivate(pPrevMenu);
}

HostSettingsMenu::HostSettingsMenu()
: GfuiMenuScreen("hostsettingsmenu.xml")
{
}

bool HostSettingsMenu::initialize(void* pMenu)
{
	NetGetNetwork()->GetHostSettings(m_strCarCat,m_bCollisions);

	pPrevMenu = pMenu;

	void* pMenuHandle = GfuiScreenCreate(NULL,NULL,onActivate, 
										 NULL, (tfuiCallback)NULL, 1);
	setMenuHandle(pMenuHandle);

    openXMLDescriptor();
    
    createStaticControls();

	int carCatId = createComboboxControl("carcatcombobox",NULL, onCarControl);
	const std::vector<std::string>& vecCategories = GfCars::self()->getCategoryIds();
	
	int CatIndex = 0;
	for (unsigned int i=0;i<vecCategories.size();i++)
	{
		GfuiComboboxAddText(pMenuHandle,carCatId,vecCategories[i].c_str());
		if (m_strCarCat == vecCategories[i])
			CatIndex = i;
	}

	GfuiComboboxSetSelectedIndex(pMenuHandle,carCatId,CatIndex);

	int collId = createComboboxControl("carcollidecombobox", NULL, onCarCollide);
	GfuiComboboxAddText(pMenuHandle,collId,"On");
	GfuiComboboxAddText(pMenuHandle,collId,"Off");

	int humanHostId = createComboboxControl("hosthumanplayercombobox", NULL, onHumanHost);
	GfuiComboboxAddText(pMenuHandle,humanHostId,"Yes");
	GfuiComboboxAddText(pMenuHandle,humanHostId,"No");

	GfuiComboboxSetSelectedIndex(pMenuHandle,humanHostId,0);

    createButtonControl("accept", NULL, onAccept);
    createButtonControl("cancel", NULL, onCancel);

	addDefaultShortcuts();
    addShortcut(GFUIK_ESCAPE, "Back to previous menu", 0, 0, onCancel);

    closeXMLDescriptor();

	return true;
}
