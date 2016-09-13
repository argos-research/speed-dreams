/***************************************************************************

    file                 : osggraph.cpp
    created              : Thu Aug 17 23:19:19 CEST 2000
    copyright            : (C) 2012 by Xavier Bertaux
    email                : bertauxx@yahoo.fr
    version              : $Id: osggraph.cpp 3741 2011-07-21 22:29:34Z torcs-ng $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "OsgGraph.h"
#include "OsgMain.h"

// The OsgGraph: singleton.
OsgGraph* OsgGraph::_pSelf = 0;

int openGfModule(const char* pszShLibName, void* hShLibHandle)
{
    // Instanciate the (only) module instance.
    OsgGraph::_pSelf = new OsgGraph(pszShLibName, hShLibHandle);

    // Register it to the GfModule module manager if OK.
    if (OsgGraph::_pSelf)
        GfModule::register_(OsgGraph::_pSelf);

    // Report about success or error.
    return OsgGraph::_pSelf ? 0 : 1;
}

int closeGfModule()
{
    // Unregister it from the GfModule module manager.
    if (OsgGraph::_pSelf)
        OsgGraph::unregister(OsgGraph::_pSelf);

    // Delete the (only) module instance.
    delete OsgGraph::_pSelf;
    OsgGraph::_pSelf = 0;

    // Report about success or error.
    return 0;
}

OsgGraph& OsgGraph::self()
{
    // Pre-condition : 1 successfull openGfModule call.
    return *_pSelf;
}

OsgGraph::OsgGraph(const std::string& strShLibName, void* hShLibHandle)
    : GfModule(strShLibName, hShLibHandle)
{
    GfLogDebug("OsgGraph::Init\n");
}

OsgGraph::~OsgGraph()
{
}

// Implementation of IGraphicsEngine ****************************************
bool OsgGraph::loadTrack(tTrack* pTrack)
{
    GfLogDebug("OsgGraph::loadTrack\n");
    return ::initTrack(pTrack) == 0;
}

bool OsgGraph::loadCars(tSituation* pSituation)
{ 
    GfLogDebug("OsgGraph::loadCars\n");
    return ::initCars(pSituation) == 0;
}

bool OsgGraph::setupView(int x, int y, int width, int height, void* pMenuScreen)
{
    GfLogDebug("OsgGraph::setupView\n");
    return ::initView(x, y, width, height, GR_VIEW_STD, pMenuScreen) == 0;
}

void OsgGraph::redrawView(tSituation* pSituation)
{
    ::refresh(pSituation);
}

void OsgGraph::unloadCars()
{
    GfLogDebug("OsgGraph::unloadCars\n");
    ::shutdownCars();
}

void OsgGraph::unloadTrack()
{
    GfLogDebug("OsgGraph::unloadTrack\n");
    ::shutdownTrack();
}

void OsgGraph::shutdownView()
{
    GfLogDebug("OsgGraph::shutdownView\n");
    ::shutdownView();
}

Camera* OsgGraph::getCurCam() 
{

    return getCamera();
}
