/***************************************************************************

    file                 : garagemenu.h
    created              : July 2010
    copyright            : (C) 2010 Jean-Philippe Meuret
    web                  : speed-dreams.sourceforge.net
    version              : $Id: garagemenu.h 4519 2012-02-22 10:59:21Z pouillot $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _GARAGEMENU_H_
#define _GARAGEMENU_H_

#include <map>

#include <tgfclient.h>

class GfRace;
class GfDriverSkin;
class GfDriver;
class GfCar;


class RmGarageMenu : public GfuiMenuScreen
{
public:

	RmGarageMenu();
	void runMenu(GfRace* pRace, GfDriver* pDriver);
	
	std::string resetCarCategoryComboBox(const std::string& strSelCatName = "");
	GfCar* resetCarModelComboBox(const std::string& strCatName,
								 const std::string& strSelCarName = "");
	void resetCarDataSheet(const std::string& strSelCarId);
	void resetSkinComboBox(const std::string& strCarName,
						   const GfDriverSkin* pSelSkin = 0);
	void resetCarPreviewImage(const GfDriverSkin& selSkin);

protected:
	
	bool initialize();

	void setDriver(GfDriver* pDriver);
	const GfDriver* getDriver() const;
	GfDriver* getDriver();

	void setRace(GfRace* pRace);
	const GfRace* getRace() const;

	const GfCar* getSelectedCarModel() const;
	const GfDriverSkin& getSelectedSkin() const;
	void setSelectedSkinIndex(int nSkinIndex);

	// Control callback functions (have to be static, as used as tgfclient controls callbacks).
	static void onActivateCB(void *pGarageMenu);
	static void onChangeCategory(tComboBoxInfo *pInfo);
	static void onChangeModel(tComboBoxInfo *pInfo);
	static void onChangeSkin(tComboBoxInfo *pInfo);
	static void onCarSettingsCB(void *pGarageMenu);
	static void onAcceptCB(void *pGarageMenu);
	static void onCancelCB(void *pGarageMenu);

private:

	// The target race.
	GfRace* _pRace;
	
	// The target driver.
	GfDriver* _pDriver;
	
	// Possible driver skins and the currently selected one.
	std::vector<GfDriverSkin> _vecPossSkins;
	size_t _nCurSkinIndex;

};

#endif //_GARAGEMENU_H_
