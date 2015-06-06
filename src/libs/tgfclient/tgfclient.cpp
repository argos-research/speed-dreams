/***************************************************************************
                          tgfclient.cpp -- The Gaming Framework UI
                             -------------------                                         
    created              : Fri Aug 13 22:31:43 CEST 1999
    copyright            : (C) 1999 by Eric Espie
    email                : torcs@free.fr
    version              : $Id: tgfclient.cpp 3893 2011-09-18 15:52:42Z pouillot $
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "gui.h"


void GfuiInit(void)
{
    gfuiInit();
}

void GfuiShutdown(void)
{
    gfuiShutdown();
	
	GfScrShutdown();
}
