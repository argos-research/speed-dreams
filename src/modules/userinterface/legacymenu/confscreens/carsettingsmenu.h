/***************************************************************************

    file                 : carsettingsmenu.h
    created              : July 2009
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

#ifndef _CARSETTINGSMENU_H_
#define _CARSETTINGSMENU_H_

#include <tgfclient.h>

#include "confscreens.h"


class CarSettingsMenu : public GfuiMenuScreen
{
public:

	CarSettingsMenu();
	bool initialize(void *pPrevMenu,const char *pzaCar);
protected:
	//callback functions must be static
	static void onActivate(void *p);
	static void onAccept(void *p);
	static void onCancel(void *p);
	static void onCarPick(tComboBoxInfo * pInfo);

	static std::string m_strCar;
};

#endif /* _CARSETTINGSMENU_H_ */
