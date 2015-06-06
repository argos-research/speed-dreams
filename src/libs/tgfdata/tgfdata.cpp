/***************************************************************************

    file                 : tgfdata.cpp
    created              : September 2011
    copyright            : (C) 2011 Jean-Philippe Meuret
    web                  : speed-dreams.sourceforge.net
    version              : $Id: tgfdata.cpp 3893 2011-09-18 15:52:42Z pouillot $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "racemanagers.h"
#include "drivers.h"
#include "cars.h"
#include "tracks.h"

#include "tgfdata.h"


void GfData::initialize()
{
	// Nothing actually needed here for the moment,
	// as everything is lazily initialized/loaded.
	
	// BTW, we can imagine adding things here to force initializing/loading,
	// if it can make the game react more quickly in the menu ?
}

void GfData::shutdown()
{
	// Delete all the singletons.
	GfRaceManagers::shutdown();
	GfDrivers::shutdown();
	GfCars::shutdown();
	GfTracks::shutdown();
}
