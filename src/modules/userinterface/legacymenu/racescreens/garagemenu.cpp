/***************************************************************************

    file                 : garagemenu.cpp
    created              : December 2009
    copyright            : (C) 2009 Brian Gavin, 2010 Jean-Philippe Meuret
    web                  : speed-dreams.sourceforge.net
    version              : $Id: garagemenu.cpp 4920 2012-09-06 07:04:49Z kmetykog $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


/* Garage menu : Car selection, preview and settings */

#include <sys/stat.h>
#include <algorithm>
#include <string>
#include <sstream>
#include <iomanip>

#include <tgfclient.h>

#include <race.h>
#include <cars.h>
#include <drivers.h>

#include "garagemenu.h"


void RmGarageMenu::onActivateCB(void *pGarageMenu)
{
	GfLogTrace("Entering Garage menu\n");

	// Get the RmGarageMenu instance.
	RmGarageMenu* pMenu = static_cast<RmGarageMenu*>(pGarageMenu);

	// Get infos about the current car for the current driver
	const GfDriver* pDriver = pMenu->getDriver();
	const GfCar* pCurCar = pDriver->getCar();

	// Initialize the GUI contents.
	GfuiLabelSetText(pMenu->getMenuHandle(), pMenu->getDynamicControlId("DriverNameLabel"),
					 pDriver->getName().c_str());
	const std::string strSelCatName = pMenu->resetCarCategoryComboBox(pCurCar->getCategoryName());
	pCurCar = pMenu->resetCarModelComboBox(strSelCatName, pCurCar->getName());
	pMenu->resetCarDataSheet(pCurCar->getId());
	pMenu->resetSkinComboBox(pCurCar->getName(), &pDriver->getSkin());
	pMenu->resetCarPreviewImage(pDriver->getSkin());

	GfuiEnable(pMenu->getMenuHandle(), pMenu->getDynamicControlId("CarSettingsButton"), GFUI_DISABLE);
}

const GfCar* RmGarageMenu::getSelectedCarModel() const
{
	const char* pszSelCarName =
		GfuiComboboxGetText(getMenuHandle(), getDynamicControlId("ModelCombo"));

	if (pszSelCarName)
		return GfCars::self()->getCarWithName(pszSelCarName);

	return 0;
}

const GfDriverSkin& RmGarageMenu::getSelectedSkin() const
{
	return _vecPossSkins[_nCurSkinIndex];
}

void RmGarageMenu::setSelectedSkinIndex(int nSkinIndex)
{
	_nCurSkinIndex = nSkinIndex;
}


void RmGarageMenu::onChangeCategory(tComboBoxInfo *pInfo)
{
	// Get the RmGarageMenu instance from call-back user data.
	RmGarageMenu* pMenu = static_cast<RmGarageMenu*>(pInfo->userData);

	// Update GUI.
	const GfCar* pSelCar = pMenu->resetCarModelComboBox(pInfo->vecChoices[pInfo->nPos]);
	pMenu->resetCarDataSheet(pSelCar->getId());
	pMenu->resetSkinComboBox(pSelCar->getName());
	pMenu->resetCarPreviewImage(pMenu->getSelectedSkin());
}

void RmGarageMenu::onChangeModel(tComboBoxInfo *pInfo)
{
	// Get the RmGarageMenu instance from call-back user data.
	RmGarageMenu* pMenu = static_cast<RmGarageMenu*>(pInfo->userData);

	// Update GUI.
	const GfCar* pSelCar = pMenu->getSelectedCarModel();
	pMenu->resetCarDataSheet(pSelCar->getId());
	pMenu->resetSkinComboBox(pSelCar->getName());
	pMenu->resetCarPreviewImage(pMenu->getSelectedSkin());
}

void RmGarageMenu::onChangeSkin(tComboBoxInfo *pInfo)
{
	// Get the RmGarageMenu instance from call-back user data.
	RmGarageMenu* pMenu = static_cast<RmGarageMenu*>(pInfo->userData);

	// Update currently selected skin skin index.
	pMenu->setSelectedSkinIndex(pInfo->nPos);
	
	// Update GUI.
	pMenu->resetCarPreviewImage(pMenu->getSelectedSkin());
}

void RmGarageMenu::onCarSettingsCB(void *pGarageMenu)
{
	// Get the RmGarageMenu instance from call-back user data.
	// const RmGarageMenu* pMenu = static_cast<RmGarageMenu*>(pGarageMenu);
	// TODO.
}

void RmGarageMenu::onAcceptCB(void *pGarageMenu)
{
	// Get the RmGarageMenu instance from call-back user data.
	RmGarageMenu* pMenu = static_cast<RmGarageMenu*>(pGarageMenu);

	// Assign new skin choice to the driver.
	GfDriver* pDriver = pMenu->getDriver();
	pDriver->setSkin(pMenu->getSelectedSkin());
	
	// Assign new car choice to the driver (only human drivers can change it).
	if (pDriver->isHuman())
		pDriver->setCar(pMenu->getSelectedCarModel());
	
	// Back to previous screen.
	GfuiScreenActivate(pMenu->getPreviousMenuHandle());
}

void RmGarageMenu::onCancelCB(void *pGarageMenu)
{
	// Get the RmGarageMenu instance from call-back user data.
	const RmGarageMenu* pMenu = static_cast<RmGarageMenu*>(pGarageMenu);

	// Back to previous screen.
	GfuiScreenActivate(pMenu->getPreviousMenuHandle());
}

RmGarageMenu::RmGarageMenu()
: GfuiMenuScreen("garagemenu.xml"), _pRace(0), _pDriver(0), _nCurSkinIndex(0)
{
}

std::string RmGarageMenu::resetCarCategoryComboBox(const std::string& strSelCatName)
{
	const int nCatComboId = getDynamicControlId("CategoryCombo");

	// Retrieve the available car categories.
	const std::vector<std::string>& vecCatNames = GfCars::self()->getCategoryNames();
	const std::vector<std::string>& vecCatIds = GfCars::self()->getCategoryIds();

	// Load the combo-box from their names (and determine the requested category index).
	unsigned nCurCatIndex = 0;
	GfuiComboboxClear(getMenuHandle(), nCatComboId);
	for (unsigned nCatIndex = 0; nCatIndex < vecCatNames.size(); nCatIndex++)
	{
		if (getRace()->acceptsCarCategory(vecCatIds[nCatIndex]))
		{
			GfuiComboboxAddText(getMenuHandle(), nCatComboId, vecCatNames[nCatIndex].c_str());
			if (!strSelCatName.empty() && vecCatNames[nCatIndex] == strSelCatName)
				nCurCatIndex = nCatIndex;
		}
	}
	
	// Select the requested category in the combo-box.
	GfuiComboboxSetSelectedIndex(getMenuHandle(), nCatComboId, nCurCatIndex);

	// Disable the combo-box for non human drivers (robot drivers can't change their car),
	// or if it contains only 1 category.
	GfuiEnable(getMenuHandle(), nCatComboId,
			   getDriver()->isHuman()
			   && GfuiComboboxGetNumberOfChoices(getMenuHandle(), nCatComboId) > 1
			   ? GFUI_ENABLE : GFUI_DISABLE);
	
	//GfLogDebug("resetCarCategoryComboBox(%s) : cur=%d\n",
	//		   strSelCatName.c_str(), nCurCatIndex);

	// Return actually selected category name (may differ from the requested one).
	return vecCatNames[nCurCatIndex];
}

GfCar* RmGarageMenu::resetCarModelComboBox(const std::string& strCatName,
											  const std::string& strSelCarName)
{
	const int nModelComboId = getDynamicControlId("ModelCombo");

	// Retrieve car models in the selected category.
	const std::vector<GfCar*> vecCarsInCat =	
		GfCars::self()->getCarsInCategoryWithName(strCatName);

	// Load the combo-box from their real names (and determine the selected model index).
	unsigned nCurrCarIndexInCat = 0;
	GfuiComboboxClear(getMenuHandle(), nModelComboId);
	for (unsigned nCarIndex = 0; nCarIndex < vecCarsInCat.size(); nCarIndex++)
	{
		GfuiComboboxAddText(getMenuHandle(), nModelComboId,
							vecCarsInCat[nCarIndex]->getName().c_str());
		if (!strSelCarName.empty() && vecCarsInCat[nCarIndex]->getName() == strSelCarName)
			nCurrCarIndexInCat = nCarIndex;
	}

	// Select the right car in the combo-box.
	GfuiComboboxSetSelectedIndex(getMenuHandle(), nModelComboId, nCurrCarIndexInCat);

	// Disable the combo-box for non human drivers (robot drivers can't change their car).
	// or if it contains only 1 model.
	GfuiEnable(getMenuHandle(), nModelComboId,
			   getDriver()->isHuman()
			   && GfuiComboboxGetNumberOfChoices(getMenuHandle(), nModelComboId) > 1
			   ? GFUI_ENABLE : GFUI_DISABLE);
	
	//GfLogDebug("resetCarModelComboBox(cat=%s, selCar=%s) : cur=%d (nCarsInCat=%d)\n",
	//		   strCatName.c_str(), strSelCarName.c_str(),
	//		   nCurrCarIndexInCat, vecCarsInCat.size());

	// Return the actually selected car (may differ from the requested car).
	return vecCarsInCat[nCurrCarIndexInCat];
}

void RmGarageMenu::resetCarDataSheet(const std::string& strSelCarId)
{
	static const char* pszDriveWheels[GfCar::eNDriveTrains+1] =
		{ "Rear", "Front", "4", "?" };
	static const char* pszEnginePosition[GfCar::eNEnginePositions+1] =
		{ "Front", "Front-mid", "Mid", "Rear-mid", "Rear", "?" };
	static const char* pszEngineShape[GfCar::eNEngineShapes+1] =
		{ "V", "L", "H", "W", "?" };
	
	// Retrieve selected car.
	const GfCar* pSelCar = GfCars::self()->getCar(strSelCarId);
	
	// Update GUI.
	std::ostringstream ossSpecValue;
	
	ossSpecValue << (long)pSelCar->getMass() << " kg ";
	const long nFRMassPercent = (long)(pSelCar->getFrontRearMassRatio() * 100);
	if (nFRMassPercent > 50)
		ossSpecValue << "(" << nFRMassPercent << "% front)";
	else
		ossSpecValue << "(" << 100 - nFRMassPercent << "% rear)";
	GfuiLabelSetText(getMenuHandle(), getDynamicControlId("MassLabel"),
					 ossSpecValue.str().c_str());

	ossSpecValue.str("");
	ossSpecValue << pszDriveWheels[pSelCar->getDriveTrain()] << " WD, "
				 << pSelCar->getGearsCount() << " gears";
	GfuiLabelSetText(getMenuHandle(), getDynamicControlId("DriveTrainLabel"),
					 ossSpecValue.str().c_str());

	ossSpecValue.str("");
	ossSpecValue << std::fixed << std::setprecision(0)
				 << pSelCar->getMaxPower() / 75 / G << " bhp ("
				 << pSelCar->getMaxPowerSpeed() * 30.0 / PI << " rpm)";
	GfuiLabelSetText(getMenuHandle(), getDynamicControlId("MaxPowerLabel"),
					 ossSpecValue.str().c_str());
	
	ossSpecValue.str("");
	ossSpecValue << pSelCar->getMaxTorque() << " N.m ("
				 << pSelCar->getMaxTorqueSpeed() * 30.0 / PI << " rpm)";
	GfuiLabelSetText(getMenuHandle(), getDynamicControlId("MaxTorqueLabel"),
					 ossSpecValue.str().c_str());

	ossSpecValue.str("");
	if (pSelCar->getEnginePosition() != GfCar::eNEnginePositions)
		ossSpecValue << pszEnginePosition[pSelCar->getEnginePosition()] << ' ';
	if (pSelCar->getEngineShape() != GfCar::eNEngineShapes)
		ossSpecValue << pszEngineShape[pSelCar->getEngineShape()];
	if (pSelCar->getCylinders() > 0)
	{
		ossSpecValue << pSelCar->getCylinders() << " ";
		if (pSelCar->getEngineShape() == GfCar::eNEngineShapes)
			ossSpecValue << "cyl. ";
	}
	if (pSelCar->getEngineCapacity() > 0)
		ossSpecValue << std::setprecision(1) << pSelCar->getEngineCapacity() << " l ";
	if (pSelCar->isTurboCharged())
		ossSpecValue << "turbo";
	if (ossSpecValue.str().empty())
		ossSpecValue << "missing information";
	
	GfuiLabelSetText(getMenuHandle(), getDynamicControlId("EngineLabel"),
					 ossSpecValue.str().c_str());
	
	GfuiProgressbarSetValue(getMenuHandle(), getDynamicControlId("TopSpeedProgress"),
							pSelCar->getTopSpeed() * 3.6f);
	GfuiProgressbarSetValue(getMenuHandle(), getDynamicControlId("PowerMassRatioProgress"),
							pSelCar->getMaxPower() / 75 / G / pSelCar->getMass());
	GfuiProgressbarSetValue(getMenuHandle(), getDynamicControlId("LowSpeedGripProgress"),
							pSelCar->getLowSpeedGrip());
	GfuiProgressbarSetValue(getMenuHandle(), getDynamicControlId("HighSpeedGripProgress"),
							pSelCar->getHighSpeedGrip());
	GfuiProgressbarSetValue(getMenuHandle(), getDynamicControlId("CorneringProgress"),
							pSelCar->getInvertedZAxisInertia());
	
	GfLogDebug("%s : topSp=%.1f, powMass=%.2f, lowSpGrip=%.1f, highSpGrip=%.1f, 1/ZInertia=%.5f\n",
			   strSelCarId.c_str(),
			   pSelCar->getTopSpeed()*3.6f, pSelCar->getMaxPower() / 75 / G / pSelCar->getMass(),
			   pSelCar->getLowSpeedGrip(), pSelCar->getHighSpeedGrip(),
			   pSelCar->getInvertedZAxisInertia());
}

void RmGarageMenu::resetSkinComboBox(const std::string& strCarName,
										const GfDriverSkin* pSelSkin)
{
	const int nSkinComboId = getDynamicControlId("SkinCombo");

	// Get really available skins and previews for this car and current driver.
	const std::string strCarId =
		GfCars::self()->getCarWithName(strCarName)->getId();
	_vecPossSkins = getDriver()->getPossibleSkins(strCarId);
		
	// Load the skin list in the combo-box (and determine the selected skin index).
	GfuiComboboxClear(getMenuHandle(), nSkinComboId);
	_nCurSkinIndex = 0;
	std::vector<GfDriverSkin>::const_iterator itSkin;
	for (itSkin = _vecPossSkins.begin(); itSkin != _vecPossSkins.end(); itSkin++)
	{
		std::string strDispSkinName =
			itSkin->getName().empty() ? "standard" : itSkin->getName();
		//#736: display skin name starting with capital letter
		strDispSkinName[0] = toupper(strDispSkinName[0]);
		GfuiComboboxAddText(getMenuHandle(), nSkinComboId, strDispSkinName.c_str());
		if (pSelSkin && itSkin->getName() == pSelSkin->getName())
			_nCurSkinIndex = itSkin - _vecPossSkins.begin();
	}

	// Select the right skin in the combo-box.
	GfuiComboboxSetSelectedIndex(getMenuHandle(), nSkinComboId, _nCurSkinIndex);

	// Desactivate the combo if only 1 skin, activate it otherwise.
	GfuiEnable(getMenuHandle(), nSkinComboId,
			   _vecPossSkins.size() > 1 ? GFUI_ENABLE : GFUI_DISABLE);
}

void RmGarageMenu::resetCarPreviewImage(const GfDriverSkin& selSkin)
{
	const int nCarImageId = getDynamicControlId("PreviewImage");

	// Load the preview image.
	if (GfFileExists(selSkin.getCarPreviewFileName().c_str()))
		GfuiStaticImageSet(getMenuHandle(), nCarImageId, selSkin.getCarPreviewFileName().c_str());
	else
		GfuiStaticImageSet(getMenuHandle(), nCarImageId, "data/img/nocarpreview.png");
}

void RmGarageMenu::runMenu(GfRace* pRace, GfDriver* pDriver)
{
	// Initialize if not already done.
	if (!getMenuHandle())
		initialize();
	
	// Store target race.
	setRace(pRace);
	
	// Store target driver.
	setDriver(pDriver);

	// Normally expected job.
	GfuiMenuScreen::runMenu();
}

bool RmGarageMenu::initialize()
{
	// Create the menu and all its controls.
	createMenu(NULL, this, onActivateCB, NULL, (tfuiCallback)NULL, 1);

    openXMLDescriptor();
    
    createStaticControls();
    
	createLabelControl("DriverNameLabel");
	
	createComboboxControl("CategoryCombo", this, onChangeCategory);
	createComboboxControl("ModelCombo", this, onChangeModel);
	createComboboxControl("SkinCombo", this, onChangeSkin);
	
	createStaticImageControl("PreviewImage");
	
	createLabelControl("DriveTrainLabel");
	createLabelControl("MaxPowerLabel");
	createLabelControl("MaxTorqueLabel");
	createLabelControl("MassLabel");
	createLabelControl("EngineLabel");
	
	createProgressbarControl("TopSpeedProgress");
	createProgressbarControl("PowerMassRatioProgress");
	createProgressbarControl("HighSpeedGripProgress");
	createProgressbarControl("LowSpeedGripProgress");
	createProgressbarControl("CorneringProgress");

    createButtonControl("CarSettingsButton", this, onCarSettingsCB);
	createButtonControl("AcceptButton", this, onAcceptCB);
    createButtonControl("CancelButton", this, onCancelCB);

    closeXMLDescriptor();

    // Keyboard shortcuts.
    addShortcut(GFUIK_ESCAPE, "Cancel", this, onCancelCB, NULL);
    addShortcut(GFUIK_RETURN, "Accept", this, onAcceptCB, NULL);
    addShortcut(GFUIK_F1, "Help", getMenuHandle(), GfuiHelpScreen, NULL);
    addShortcut(GFUIK_F12, "Screen-Shot", NULL, GfuiScreenShot, NULL);
    // TODO Keyboard shortcuts: Add support for shortcuts in GfuiCombobox ?
    //GfuiAddKey(ScrHandle, GFUIK_UP, "Move Up", this, onChangeModel, NULL);

	return true;
}

void RmGarageMenu::setDriver(GfDriver* pDriver)
{
	_pDriver = pDriver;
}

const GfDriver* RmGarageMenu::getDriver() const
{
	return _pDriver;
}

GfDriver* RmGarageMenu::getDriver()
{
	return _pDriver;
}

void RmGarageMenu::setRace(GfRace* pRace)
{
	_pRace = pRace;
}

const GfRace* RmGarageMenu::getRace() const
{
	return _pRace;
}

