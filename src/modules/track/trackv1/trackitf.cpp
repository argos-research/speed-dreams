/***************************************************************************

    file                 : trackitf.cpp
    created              : Sun Jan 30 22:57:50 CET 2000
    copyright            : (C) 2000 by Eric Espie
    email                : torcs@free.fr
    version              : $Id: trackitf.cpp 6097 2015-08-30 23:12:09Z beaglejoe $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "trackitf.h"
#include "trackinc.h"


// The TrackModule singleton.
TrackModule* TrackModule::_pSelf = 0;

int openGfModule(const char* pszShLibName, void* hShLibHandle)
{
	// Instanciate the (only) module instance.
	TrackModule::_pSelf = new TrackModule(pszShLibName, hShLibHandle);

	// Register it to the GfModule module manager if OK.
	if (TrackModule::_pSelf)
		GfModule::register_(TrackModule::_pSelf);

	// Report about success or error.
	return TrackModule::_pSelf ? 0 : 1;
}

int closeGfModule()
{
	// Unregister it from the GfModule module manager.
	if (TrackModule::_pSelf)
		TrackModule::unregister(TrackModule::_pSelf);
	
	// Delete the (only) module instance.
	delete TrackModule::_pSelf;
	TrackModule::_pSelf = 0;

	// Report about success or error.
	return 0;
}

TrackModule& TrackModule::self()
{
	// Pre-condition : 1 successfull openGfModule call.
	return *_pSelf;
}

TrackModule::TrackModule(const std::string& strShLibName, void* hShLibHandle)
: GfModule(strShLibName, hShLibHandle)
{
}

TrackModule::~TrackModule()
{
}

// Implementation of ITrackLoader***********************************
struct Track* TrackModule::load(const char* filename, bool grExts)
{
	return grExts ? ::TrackBuildEx(filename) : ::TrackBuildv1(filename);
}

void TrackModule::unload()
{
	::TrackShutdown();
}


// Implementation of ITrack ****************************************
//tdble TrackModule::globalHeight(tTrackSeg* seg, tdble x, tdble y)
//{
//	return TrackHeightG(seg, x, y);
//}
//
//tdble TrackModule::localHeight(tTrkLocPos* pos)
//{
//	return TrackHeightL(pos);
//}
//
//void TrackModule::global2Local(tTrackSeg* seg, tdble x, tdble y, tTrkLocPos* pos, int sides)
//{
//	TrackGlobal2Local(seg, x, y, pos, sides);
//}
//
//void TrackModule::local2Global(tTrkLocPos* pos, tdble* x, tdble* y)
//{
//	TrackLocal2Global(pos, x, y);
//}
//
//void TrackModule::sideNormal(tTrackSeg* seg, tdble x, tdble y, int side, t3Dd* norm)
//{
//	TrackSideNormal(seg, x, y, side, norm);
//}
//
//void TrackModule::surfaceNormal(tTrkLocPos* pos, t3Dd* norm)
//{
//	TrackSurfaceNormal(pos, norm);
//}
