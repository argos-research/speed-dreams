/***************************************************************************
                 itrackloader.h -- Interface for track loaders

    created              : Wed Mar 31 22:12:01 CEST 2011
    copyright            : (C) 2011 by Jean-Philippe Meuret                         
    web                  : http://www.speed-dreams.org
    version              : $Id: itrackloader.h 4902 2012-08-27 10:04:20Z kmetykog $
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
    	Interface for track loaders
    @version	$Id: itrackloader.h 4902 2012-08-27 10:04:20Z kmetykog $
*/

#ifndef __ITRACKLOADER__H__
#define __ITRACKLOADER__H__

#include <track.h>


class ITrackLoader
{
 public:

	virtual tTrack* load(const char* filename, bool grExts = false) = 0;
	virtual void unload() = 0;

	// ???? a ITrack interface ??????
	//virtual tdble globalHeight(tTrackSeg*, tdble x, tdble y) = 0;
	//virtual tdble localHeight(tTrkLocPos* pos) = 0;
 	//virtual void global2Local(tTrackSeg* seg, tdble x, tdble y, tTrkLocPos* pos, int sides) = 0;
 	//virtual void local2Global(tTrkLocPos* pos, tdble* x, tdble* y) = 0;
 	//virtual void sideNormal(tTrackSeg* seg, tdble x, tdble y, int side, t3Dd* norm) = 0;
	//virtual void surfaceNormal(tTrkLocPos* pos, t3Dd* norm) = 0;
};

#endif // __ITRACKLOADER__H__
