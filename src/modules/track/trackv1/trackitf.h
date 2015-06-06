/***************************************************************************

    file                 : trackitf.h
    created              : Wed Mar 31 22:12:01 CEST 2011
    copyright            : (C) 2011 by Jean-Philippe Meuret                         
    web                  : http://www.speed-dreams.org
    version              : $Id: trackitf.h 5349 2013-03-23 17:59:22Z pouillot $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _TRACKITF_H__
#define _TRACKITF_H__

//#include <itrack.h>
#include <itrackloader.h>

#include <tgf.hpp>


// DLL exported symbols declarator for Windows.
#ifdef WIN32
# ifdef TRACK_DLL
#  define TRACK_API __declspec(dllexport)
# else
#  define TRACK_API __declspec(dllimport)
# endif
#else
# define TRACK_API
#endif


// The C interface of the module.
extern "C" int TRACK_API openGfModule(const char* pszShLibName, void* hShLibHandle);
extern "C" int TRACK_API closeGfModule();

// The module main class (Singleton, inherits GfModule, and implements ITrackLoader).
class TRACK_API TrackModule : public GfModule, public ITrackLoader //, public ITrack
{
 public:

	// Implementation of ITrackLoader.
	virtual tTrack* load(const char* filename, bool grExts = false);
	virtual void unload();

	// Implementation of ITrack ?
	//virtual tdble globalHeight(tTrackSeg*, tdble x, tdble y);
	//virtual tdble localHeight(tTrkLocPos* pos);
	//virtual void global2Local(tTrackSeg* seg, tdble x, tdble y, tTrkLocPos* pos, int sides);
	//virtual void local2Global(tTrkLocPos* pos, tdble* x, tdble* y);
	//virtual void sideNormal(tTrackSeg* seg, tdble x, tdble y, int side, t3Dd* norm);
	//virtual void surfaceNormal(tTrkLocPos* pos, t3Dd* norm);

	// Accessor to the singleton.
	static TrackModule& self();

	// Destructor.
	virtual ~TrackModule();

 protected:

	// Protected constructor to avoid instanciation outside (but friends).
	TrackModule(const std::string& strShLibName, void* hShLibHandle);
	
	// Make the C interface functions nearly member functions.
	friend int openGfModule(const char* pszShLibName, void* hShLibHandle);
	friend int closeGfModule();

 protected:

	// The singleton.
	static TrackModule* _pSelf;
};

#endif /* _TRACKITF_H__ */ 



