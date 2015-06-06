/***************************************************************************

    file        : ssggraph.h
    copyright   : (C) 2011 by Jean-Philippe Meuret                        
    email       : pouillot@users.sourceforge.net   
    version     : $Id: ssggraph.h 5095 2013-01-12 17:57:47Z pouillot $

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
    		The "ssggraph" graphics engine module
    @version    $Id: ssggraph.h 5095 2013-01-12 17:57:47Z pouillot $
*/

#ifndef _SSGGRAPH_H_
#define _SSGGRAPH_H_

#include <igraphicsengine.h>

#include <tgf.hpp>

class ssgLoaderOptions;


// DLL exported symbols declarator for Windows.
#ifdef WIN32
# ifdef SSGGRAPH_DLL
#  define SSGGRAPH_API __declspec(dllexport)
# else
#  define SSGGRAPH_API __declspec(dllimport)
# endif
#else
# define SSGGRAPH_API
#endif


// The C interface of the module.
extern "C" int SSGGRAPH_API openGfModule(const char* pszShLibName, void* hShLibHandle);
extern "C" int SSGGRAPH_API closeGfModule();

// The module main class
// (Singleton, inherits GfModule, and implements IGraphicsEngine).
class SSGGRAPH_API SsgGraph : public GfModule, public IGraphicsEngine
{
 public:

	// Implementation of IGraphicsEngine.
	virtual bool loadTrack(struct Track* pTrack);
	virtual bool loadCars(struct Situation *pSituation);
	virtual bool setupView(int x, int y, int width, int height, void* pMenuScreen);
	virtual void redrawView(struct Situation *pSituation);
	//virtual void bendCar(int index, sgVec3 poc, sgVec3 force, int count = 0);
	virtual void shutdownView();
	virtual void unloadCars();
	virtual void unloadTrack();
	virtual Camera* getCurCam();



	// Accessor to the singleton.
	static SsgGraph& self();

	// Destructor.
	virtual ~SsgGraph();

 protected:

	// Protected constructor to avoid instanciation outside (but friends).
	SsgGraph(const std::string& strShLibName, void* hShLibHandle);
	
	// Make the C interface functions nearly member functions.
	friend int openGfModule(const char* pszShLibName, void* hShLibHandle);
	friend int closeGfModule();

 protected:

	// The singleton.
	static SsgGraph* _pSelf;

	// The default SSGLoaderOptions instance.
	ssgLoaderOptions* _pDefaultSSGLoaderOptions;
};

#endif /* _SSGGRAPH_H_ */ 
