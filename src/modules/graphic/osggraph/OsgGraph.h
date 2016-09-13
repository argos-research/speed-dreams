/***************************************************************************

    file        : OsgGraph.h
    copyright   : (C) 2012 by Xavier Bertaux
    email       : bertauxx@yahoo.fr
    version     : $Id: OsgGraph.h 3741 2011-07-21 22:29:34Z torcs-ng $

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
            The "osggraph" graphics engine module
    @version    $Id: ssggraph.h 3741 2011-07-21 22:29:34Z pouillot $
*/

#ifndef _OSGGRAPH_H_
#define _OSGGRAPH_H_

#include <igraphicsengine.h>

#include <tgf.hpp>

//class osgLoaderOptions;


// DLL exported symbols declarator for Windows.
#ifdef WIN32
# ifdef OSGGRAPH_DLL
#  define OSGGRAPH_API __declspec(dllexport)
# else
#  define OSGGRAPH_API __declspec(dllimport)
# endif
#else
# define OSGGRAPH_API
#endif


// The C interface of the module.
extern "C" int OSGGRAPH_API openGfModule(const char* pszShLibName, void* hShLibHandle);
extern "C" int OSGGRAPH_API closeGfModule();

// The module main class
// (Singleton, inherits GfModule, and implements IGraphicsEngine).
class OSGGRAPH_API OsgGraph : public GfModule, public IGraphicsEngine
{
public:

    // Implementation of IGraphicsEngine.
    virtual bool loadTrack(struct Track* pTrack);
    virtual bool loadCars(struct Situation *pSituation);
    virtual bool setupView(int x, int y, int width, int height, void* pMenuScreen);
    virtual void redrawView(struct Situation *pSituation);
    virtual void shutdownView();
    virtual void unloadCars();
    virtual void unloadTrack();
    virtual Camera *getCurCam();

    // Accessor to the singleton.
    static OsgGraph& self();

    // Destructor.
    virtual ~OsgGraph();

protected:

    // Protected constructor to avoid instanciation outside (but friends).
    OsgGraph(const std::string& strShLibName, void* hShLibHandle);

    // Make the C interface functions nearly member functions.
    friend int openGfModule(const char* pszShLibName, void* hShLibHandle);
    friend int closeGfModule();

protected:

    // The singleton.
    static OsgGraph* _pSelf;
};

#endif /* _OSGGRAPH_H_ */
