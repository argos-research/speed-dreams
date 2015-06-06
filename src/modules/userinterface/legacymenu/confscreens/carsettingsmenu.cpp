/***************************************************************************

    file                 : carsettingsmenu.cpp
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
This file deals with car settings
*/

#include <cars.h>
#include <network.h>

#include "carsettingsmenu.h"


static void *pPrevMenu = NULL;
std::string CarSettingsMenu::m_strCar;

void CarSettingsMenu::onActivate(void *P)
{

}

void CarSettingsMenu::onCarPick(tComboBoxInfo *pInfo)
{
	m_strCar = pInfo->vecChoices[pInfo->nPos];
}

void CarSettingsMenu::onAccept(void *p)
{
	GfCar *pCar = GfCars::self()->getCarWithName(m_strCar);
	NetGetNetwork()->SetCarInfo(pCar->getId().c_str());
	GfuiScreenActivate(pPrevMenu);
}

void CarSettingsMenu::onCancel(void *p)
{
	GfuiScreenActivate(pPrevMenu);
}

CarSettingsMenu::CarSettingsMenu()
: GfuiMenuScreen("carselectionmenu.xml")
{
}

bool CarSettingsMenu::initialize(void* pMenu,const char *pszCar)
{
	std::string strCarCat;
	bool bCollisions;
	NetGetNetwork()->GetHostSettings(strCarCat,bCollisions);
	pPrevMenu = pMenu;

	void* pMenuHandle = GfuiScreenCreate(NULL,NULL,onActivate, 
										   NULL, (tfuiCallback)NULL, 
										   1);
	setMenuHandle(pMenuHandle);

    openXMLDescriptor();
    
    createStaticControls();
    
	int carCatId = createComboboxControl("modelcombo",NULL,onCarPick);
	createComboboxControl("skincombo",NULL,NULL);
	createStaticImageControl("carpreviewimage");
	createProgressbarControl("topspeedprogress");
	createProgressbarControl("accelerationprogress");
	createProgressbarControl("handlingprogress");
	createProgressbarControl("brakingprogress");

	const std::vector<std::string> vecCarRealNames =
		GfCars::self()->getCarNamesInCategory(strCarCat);
	
	m_strCar = pszCar;
	int carIndex = 0;
	for (unsigned i=0;i<vecCarRealNames.size();i++)
	{
		GfuiComboboxAddText(pMenuHandle,carCatId,vecCarRealNames[i].c_str());
		if (vecCarRealNames[i] == m_strCar)
			carIndex = i;
	}
	
	GfuiComboboxSetSelectedIndex(pMenuHandle,carCatId,carIndex);

	createButtonControl("accept",NULL,onAccept);
    createButtonControl("cancel",NULL,onCancel);

    closeXMLDescriptor();
    
	return true;
}
