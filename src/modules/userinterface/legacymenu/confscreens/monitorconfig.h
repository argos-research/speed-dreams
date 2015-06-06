/***************************************************************************

    file                 : displayconfig.h
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

#ifndef _MONITORCONFIG_H_
#define _MONITORCONFIG_H_

#include <tgfclient.h>

#include "confscreens.h"

class MonitorMenu : public GfuiMenuScreen
{
public:

	MonitorMenu();
	bool initialize(void* pPreviousMenu);
	
	enum EMonitorType { e4by3 = 0, e16by9 = 1, nMonitorTypes };
	enum ESpanSplit { eDisabled = 0, eEnabled = 1, nSpanSplits };

	void setMonitorType(EMonitorType eMode);
	void setSpanSplit(ESpanSplit eMode);
	void setBezelComp(float value);

	void storeSettings() const;
	void loadSettings();

	void updateControls();

protected:
	
	// Control callback functions (must be static).
	static void onActivate(void *pMonitorMenu);
	static void onChangeMonitorType(tComboBoxInfo *pInfo);
	static void onChangeSpanSplit(tComboBoxInfo *pInfo);
	static void onChangeBezelComp(void *);

	static void onAccept(void *pMonitorMenu);
	static void onCancel(void *pMonitorMenu);

private:

	//! Current Setting for Monitor Type
	EMonitorType _eMonitorType;

	//! Current Setting for Span Splits
	ESpanSplit _eSpanSplit;


};

extern void* MonitorMenuInit(void* pPreviousMenu);

#endif //_MONITORCONFIG_H_
