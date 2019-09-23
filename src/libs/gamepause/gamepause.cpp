/***************************************************************************

    created              : Sat Mar 18 23:16:38 CET 2000
    copyright            : (C) 2000 by Eric Espie
    email                : torcs@free.fr
    version              : $Id: human.cpp 5522 2013-06-17 21:03:25Z torcs-ng $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "legacymenu.h"
#include "gamepause.h"



void
RaceResume()
{
        if (LegacyMenu::self().soundEngine())
            LegacyMenu::self().soundEngine()->mute(false);

		LmRaceEngine().start();
}

void
RacePause()
{

		if (LegacyMenu::self().soundEngine())
			LegacyMenu::self().soundEngine()->mute(true);

		LmRaceEngine().stop();
}
