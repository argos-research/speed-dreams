/***************************************************************************

    file                 : hostsettingsmenu.h
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

#ifndef _HOSTSETTINGSMENU_H_
#define _HOSTSETTINGSMENU_H_

#include <tgfclient.h>

#include "confscreens.h"


class HostSettingsMenu : public GfuiMenuScreen
{
public:
	HostSettingsMenu();
	bool initialize(void *pPrevMenu);

protected:
	//callback functions must be static
	static void onActivate(void *p);
	static void onAccept(void *p);
	static void onCancel(void *p);
	static void onCarControl(tComboBoxInfo * pInfo);
	static void onCarCollide(tComboBoxInfo * pInfo);
	static void onHumanHost(tComboBoxInfo *pChoices);
	static void onPlayerReady(void *p);

protected:
	static std::string m_strCarCat;
	static bool m_bCollisions;
	static bool m_bHumanHost;
};

#endif /* _HOSTSETTINGSMENU_H_ */
