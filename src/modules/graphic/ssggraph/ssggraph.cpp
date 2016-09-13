/***************************************************************************

    file                 : ssggraph.cpp
    created              : Thu Aug 17 23:19:19 CEST 2000
    copyright            : (C) 2000 by Eric Espie
    email                : torcs@free.fr
    version              : $Id: ssggraph.cpp 6097 2015-08-30 23:12:09Z beaglejoe $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/



#include "ssggraph.h"

#include "grmain.h"
#include "grtexture.h"


// The SsgGraph: singleton.
SsgGraph* SsgGraph::_pSelf = 0;

int openGfModule(const char* pszShLibName, void* hShLibHandle)
{
    // Instanciate the (only) module instance.
    SsgGraph::_pSelf = new SsgGraph(pszShLibName, hShLibHandle);

    // Register it to the GfModule module manager if OK.
    if (SsgGraph::_pSelf)
        GfModule::register_(SsgGraph::_pSelf);

    // Report about success or error.
    return SsgGraph::_pSelf ? 0 : 1;
}

int closeGfModule()
{
    // Unregister it from the GfModule module manager.
    if (SsgGraph::_pSelf)
        SsgGraph::unregister(SsgGraph::_pSelf);

    // Delete the (only) module instance.
    delete SsgGraph::_pSelf;
    SsgGraph::_pSelf = 0;

    // Report about success or error.
    return 0;
}

SsgGraph& SsgGraph::self()
{
    // Pre-condition : 1 successfull openGfModule call.
    return *_pSelf;
}

SsgGraph::SsgGraph(const std::string& strShLibName, void* hShLibHandle)
: GfModule(strShLibName, hShLibHandle)
{
    // Override the default SSG loader options object with our's
    // (workaround try for ssggraph crash at re-load time).
    _pDefaultSSGLoaderOptions = new ssgLoaderOptions;
    ssgSetCurrentOptions(_pDefaultSSGLoaderOptions);

    // Initialize the PLib SSG layer.
    ssgInit();

    //Setup image loaders
    grRegisterCustomSGILoader();
}

SsgGraph::~SsgGraph()
{
    // Terminate the PLib SSG layer.
    delete _pDefaultSSGLoaderOptions;
}

// Implementation of IGraphicsEngine ****************************************

bool SsgGraph::loadTrack(tTrack* pTrack)
{
    //GfLogDebug("SsgGraph::loadTrack\n");
    return ::initTrack(pTrack) == 0;
}

bool SsgGraph::loadCars(tSituation* pSituation)
{
    //GfLogDebug("SsgGraph::loadCars\n");
    return ::initCars(pSituation) == 0;
}

bool SsgGraph::setupView(int x, int y, int width, int height, void* pMenuScreen)
{
    //GfLogDebug("SsgGraph::setupView\n");
    return ::initView(x, y, width, height, GR_VIEW_STD, pMenuScreen) == 0;
}

void SsgGraph::redrawView(tSituation* pSituation)
{
    ::refresh(pSituation);
}

// void SsgGraph::bendCar(int index, sgVec3 poc, sgVec3 force, int count)
// {
// 	::bendCar(index, poc, force, count);
// }

void SsgGraph::unloadCars()
{
    //GfLogDebug("SsgGraph::unloadCars\n");
    ::shutdownCars();
}

void SsgGraph::unloadTrack()
{
    //GfLogDebug("SsgGraph::unloadTrack\n");
    ::shutdownTrack();
}

void SsgGraph::shutdownView()
{
    //GfLogDebug("SsgGraph::shutdownView\n");
    ::shutdownView();
}


Camera* SsgGraph::getCurCam()
{
    Camera *cam = new Camera;
    cGrCamera *gcam = grGetCurCamera();

    cam->Centerv = gcam->getCenterv();
    cam->Upv = gcam->getUpv();
    cam->Speedv = gcam->getSpeedv();
    cam->Posv = gcam->getPosv();
  
    return cam;
}
