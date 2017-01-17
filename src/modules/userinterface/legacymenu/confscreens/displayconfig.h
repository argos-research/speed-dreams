/***************************************************************************

    file                 : displayconfig.h
    created              : October 2010
    copyright            : (C) 2010 Jean-Philippe Meuret
    web                  : speed-dreams.sourceforge.net
    version              : $Id: displayconfig.h 5930 2015-03-26 00:38:14Z torcs-ng $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _DISPLAYCONFIG_H_
#define _DISPLAYCONFIG_H_

#include <tgfclient.h>

#include "confscreens.h"

// Comment-out to activate max. refresh rate settings.
#define NoMaxRefreshRate 1


class DisplayMenu : public GfuiMenuScreen
{
public:

	DisplayMenu();
	~DisplayMenu();
	
	bool initialize(void* pPreviousMenu);
	
	enum EDisplayMode { eFullScreen = 0, eWindowed = 1, nDisplayModes };
	enum EVideoDetectMode { eAuto = 0, eManual = 1, nVideoDetectModes };
	enum EVideoInitMode { eCompatible = 0, eBestPossible = 1, nVideoInitModes };

	void setDisplayMode(EDisplayMode eMode);
	void setColorDepthIndex(int nIndex);
	void setScreenSizeIndex(int nIndex);
	void setVideoDetectMode(EVideoDetectMode eMode);
	void setVideoInitMode(EVideoInitMode eMode);
#ifndef NoMaxRefreshRate
	void setMaxRefreshRateIndex(int nIndex);
#endif	
	void storeSettings() const;
	void loadSettings();

	void updateControls();

protected:
	
	void resetColorDepths();
	void resetScreenSizes();

	// Control callback functions (must be static).
	static void onActivate(void *pDisplayMenu);
	static void onChangeScreenSize(tComboBoxInfo *pInfo);
	static void onChangeColorDepth(tComboBoxInfo *pInfo);
	static void onChangeDisplayMode(tComboBoxInfo *pInfo);
	static void onChangeVideoDetectMode(tComboBoxInfo *pInfo);
	static void onChangeVideoInitMode(tComboBoxInfo *pInfo);
#ifndef NoMaxRefreshRate
	static void onChangeMaxRefreshRate(tComboBoxInfo *pInfo);
#endif	

	static void onGarage(void *pDisplayMenu);
	static void onAccept(void *pDisplayMenu);
	static void onCancel(void *pDisplayMenu);

private:

	//! Possible screen sizes according to the current color depth and display mode.
	int _nNbScreenSizes;
	tScreenSize* _aScreenSizes;

	//! Possible color depths (bits per pixel).
	int _nNbColorDepths;
	int* _aColorDepths;

	//! Currently selected color depth (inside _aColorDepths).
	int _nColorDepth;

	//! Currently selected display mode.
	EDisplayMode _eDisplayMode;

	//! Currently selected screen size.
	int _nScreenWidth;
	int _nScreenHeight;
	
	//! Currently selected video features detection mode.
	EVideoDetectMode _eVideoDetectMode;

	//! Currently selected video initialization mode.
	EVideoInitMode _eVideoInitMode;

#ifndef NoMaxRefreshRate
	//! Currently selected max. refresh rate (Hz).
	int _nMaxRefreshRate;
#endif	
};

extern void* DisplayMenuInit(void* pPreviousMenu);
extern void DisplayMenuRelease(void);

#endif //_DISPLAYCONFIG_H_
