/***************************************************************************

    file                 : joystickconfig.h
    created              : Wed Mar 21 23:06:29 CET 2001
    copyright            : (C) 2001 by Eric Espié
    email                : Eric.Espie@torcs.org
    version              : $Id: joystickconfig.h 3709 2011-07-06 01:56:57Z mungewell $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
 
/** @file    
    		
    @author	<a href=mailto:torcs@free.fr>Eric Espie</a>
    @version	$Id: joystickconfig.h 3709 2011-07-06 01:56:57Z mungewell $
*/

#ifndef _JOYSTICKCONFIG_H_
#define _JOYSTICKCONFIG_H_

#include "confscreens.h"


/* nextMenu : the menu to go to when "next" button is pressed */
extern void *JoyCalMenuInit(void *prevMenu, void *nextMenu, tCmdInfo *cmd, int maxcmd);

#endif /* _JOYSTICKCONFIG_H_ */ 



