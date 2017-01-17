/***************************************************************************

    file                 : OsgMain.cpp
    created              : Thu Aug 17 23:23:49 CEST 2000
    copyright            : (C)2013 by Xavier Bertaux
    email                : bertauxx@yahoo.fr
    version              : $Id: OsgMain.cpp 4712 2012-05-10 06:02:49Z mungewell $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <osgDB/Registry>
#include <osgUtil/Optimizer>
#include <osg/Group>
#include <osgViewer/View>
#include <osgViewer/CompositeViewer>
#include <osgViewer/Viewer>

#include <glfeatures.h> // GfglFeatures
#include <robot.h>	//ROB_SECT_ARBITRARY
#include <graphic.h>

#include "OsgMain.h"
//#include "OsgOptions.h"
#include "OsgCar.h"
#include "OsgScenery.h"
#include "OsgRender.h"
#include "OsgMath.h"
#include "OsgScreens.h"

//extern	osg::Timer m_timer;
//extern	osg::Timer_t m_start_tick;

//SDOptions *Options = 0;
SDCars *cars = 0;
SDScenery *scenery = 0;
SDRender *render = 0;
SDScreens *screens = 0;

/*oid *getOptions()
{
    return Options;
}*/

void *getScreens()
{
    return screens;
}

void *getRender()
{
    return render;
}

void * getCars()
{
    return cars;
}

void * getScenery()
{
    return scenery;
}

static osg::Timer m_timer;

int grMaxTextureUnits = 0;

tdble grMaxDammage = 10000.0;
int grNbCars = 0;

void *grHandle = 0;
void *grTrackHandle = 0;

int m_Winx, m_Winy, m_Winw, m_Winh;

tdble grLodFactorValue = 1.0;

// Frame/FPS info.
static SDFrameInfo frameInfo;
static double fFPSPrevInstTime;   // Last "instant" FPS refresh time
static unsigned nFPSTotalSeconds; // Total duration since initView
double ratio = 0.0f;

// Mouse coords graphics backend to screen ratios.
static float fMouseRatioX, fMouseRatioY;

// Number of active screens.
int m_NbActiveScreens = 1;
int m_NbArrangeScreens = 0;

// Current screen index.
static int m_CurrentScreenIndex = 0;

static void SDPrevCar(void * /* dummy */)
{
    screens->getActiveView()->selectPrevCar();
}

static void SDNextCar(void * /* dummy */)
{
    screens->getActiveView()->selectNextCar();
}

void SDSelectCamera(void * vp)
{
    long t = (long)vp;
    screens->changeCamera(t);
}

void SDSetZoom(void * vp)
{
    long t = (long)vp;
    screens->getActiveView()->getCameras()->getSelectedCamera()->setZoom(t);
}

void SDSwitchMirror(void * vp)
{
    screens->getActiveView()->switchMirror();
}

void SDToggleHUD(void * vp)
{
    screens->toggleDebugHUD();
}

int initView(int x, int y, int width, int height, int /* flag */, void *screen)
{
    screens = new SDScreens();

    m_Winx = x;
    m_Winy = y;
    m_Winw = width;
    m_Winh = height;

    fMouseRatioX = width / 640.0;
    fMouseRatioY = height / 480.0;

    frameInfo.fInstFps = 0.0;
    frameInfo.fAvgFps = 0.0;
    frameInfo.nInstFrames = 0;
    frameInfo.nTotalFrames = 0;
    fFPSPrevInstTime = GfTimeClock();
    nFPSTotalSeconds = 0;

    screens->Init(x,y,width,height, render->getRoot(), render->getFogColor());

    GfuiAddKey(screen, GFUIK_END,      "Zoom Minimum", (void*)GR_ZOOM_MIN,	SDSetZoom, NULL);
    GfuiAddKey(screen, GFUIK_HOME,     "Zoom Maximum", (void*)GR_ZOOM_MAX,	SDSetZoom, NULL);
    GfuiAddKey(screen, '*',            "Zoom Default", (void*)GR_ZOOM_DFLT,	SDSetZoom, NULL);

    GfuiAddKey( screen, GFUIK_PAGEUP,   "Select Previous Car", (void*)0, SDPrevCar, NULL);
    GfuiAddKey( screen, GFUIK_PAGEDOWN, "Select Next Car",     (void*)0, SDNextCar, NULL);

    GfuiAddKey(screen, GFUIK_F2,       "Driver Views",      (void*)0, SDSelectCamera, NULL);
    GfuiAddKey(screen, GFUIK_F3,       "Car Views",         (void*)1, SDSelectCamera, NULL);
    GfuiAddKey(screen, GFUIK_F4,       "Side Car Views",    (void*)2, SDSelectCamera, NULL);
    GfuiAddKey(screen, GFUIK_F5,       "Up Car View",       (void*)3, SDSelectCamera, NULL);
    GfuiAddKey(screen, GFUIK_F6,       "Persp Car View",    (void*)4, SDSelectCamera, NULL);
    GfuiAddKey(screen, GFUIK_F7,       "All Circuit Views", (void*)5, SDSelectCamera, NULL);
    GfuiAddKey(screen, GFUIK_F8,       "Track View",        (void*)6, SDSelectCamera, NULL);
    GfuiAddKey(screen, GFUIK_F9,       "Track View Zoomed", (void*)7, SDSelectCamera, NULL);
    GfuiAddKey(screen, GFUIK_F10,      "Follow Car Zoomed", (void*)8, SDSelectCamera, NULL);
    GfuiAddKey(screen, GFUIK_F11,      "TV Director View",  (void*)9, SDSelectCamera, NULL);

    GfuiAddKey(screen, 'h',            "Activate DEBUG HUD",  (void*)0, SDToggleHUD, NULL);

    /*GfuiAddKey(screen, '5',            "Debug Info",        (void*)3, grSelectBoard, NULL);
    GfuiAddKey(screen, '4',            "G/Cmd Graph",       (void*)4, grSelectBoard, NULL);
    GfuiAddKey(screen, '3',            "Leaders Board",     (void*)2, grSelectBoard, NULL);
    GfuiAddKey(screen, '2',            "Driver Counters",   (void*)1, grSelectBoard, NULL);
    GfuiAddKey(screen, '1',            "Driver Board",      (void*)0, grSelectBoard, NULL);*/
    GfuiAddKey(screen, '9',            "Mirror",            (void*)0, SDSwitchMirror, NULL);
    //GfuiAddKey(screen, '0',            "Arcade Board",      (void*)5, grSelectBoard, NULL);*/
    GfuiAddKey(screen, '+', GFUIM_CTRL, "Zoom In",           (void*)GR_ZOOM_IN,	 SDSetZoom, NULL);
    GfuiAddKey(screen, '=', GFUIM_CTRL, "Zoom In",           (void*)GR_ZOOM_IN,	 SDSetZoom, NULL);
    GfuiAddKey(screen, '-', GFUIM_CTRL, "Zoom Out",          (void*)GR_ZOOM_OUT, SDSetZoom, NULL);
    GfuiAddKey(screen, '>',             "Zoom In",           (void*)GR_ZOOM_IN,	 SDSetZoom, NULL);
    GfuiAddKey(screen, '<',             "Zoom Out",          (void*)GR_ZOOM_OUT, SDSetZoom, NULL);
    //GfuiAddKey(screen, '(',            "Split Screen",   (void*)SD_SPLIT_ADD, SDSplitScreen, NULL);
    //GfuiAddKey(screen, ')',            "UnSplit Screen", (void*)SD_SPLIT_REM, SDSplitScreen, NULL);
    //GfuiAddKey(screen, '_',            "Split Screen Arrangement", (void*)SD_SPLIT_ARR, SDSplitScreen, NULL);
    //GfuiAddKey(screen, GFUIK_TAB,      "Next (split) Screen", (void*)SD_NEXT_SCREEN, SDChangeScreen, NULL);
    /*GfuiAddKey(screen, 'm',            "Track Maps",          (void*)0, grSelectTrackMap, NULL);*/

    GfLogInfo("Current screen is #%d (out of %d)\n", m_CurrentScreenIndex, m_NbActiveScreens);

    return 0; // true;
}

int refresh(tSituation *s)
{
    // Compute F/S indicators every second.
    frameInfo.nInstFrames++;
    frameInfo.nTotalFrames++;
    const double dCurTime = GfTimeClock();
    const double dDeltaTime = dCurTime - fFPSPrevInstTime;
    if (dDeltaTime > 1.0)
    {
        ++nFPSTotalSeconds;
        fFPSPrevInstTime = dCurTime;
        frameInfo.fInstFps = frameInfo.nInstFrames / dDeltaTime;
        frameInfo.nInstFrames = 0;
        frameInfo.fAvgFps = (double)frameInfo.nTotalFrames / nFPSTotalSeconds;
        // Trace F/S every 5 seconds.
        if (nFPSTotalSeconds % 5 == 2)
            GfLogInfo("Frame rate (F/s) : Instant = %.1f (Average %.1f)\n",
                      frameInfo.fInstFps, frameInfo.fAvgFps);
    }

    cars->updateCars();
    render->UpdateSky(s->currentTime, s->accelTime);
    screens->update(s, &frameInfo);

    return 0;
}

void shutdownCars(void)
{
    if (cars)
    {
        cars->unLoad();
        delete cars;
        cars = NULL;
        GfLogInfo("Delete cars in OsgMain\n");
    }



    // Trace final mean F/s.
    if (nFPSTotalSeconds > 0)
        GfLogTrace("Average frame rate: %.2f F/s\n",
                   (double)frameInfo.nTotalFrames/((double)nFPSTotalSeconds + GfTimeClock() - fFPSPrevInstTime));
}

int initTrack(tTrack *track)
{
    // The inittrack does as well init the context, that is highly inconsistent, IMHO.
    // TODO: Find a solution to init the graphics first independent of objects.

    // Now, do the real track loading job.
    grTrackHandle = GfParmReadFile(track->filename, GFPARM_RMODE_STD | GFPARM_RMODE_CREAT);

    //Options = new SDOptions;

    scenery = new SDScenery;
    render = new SDRender;

    scenery->LoadScene(track);
    render->Init(track);

    return 0;
}

int  initCars(tSituation *s)
{
    GfLogInfo("InitCars\n");
    char buf[256];
    cars = new SDCars;
    cars->loadCars(s, scenery->getSpeedWay(), scenery->getSpeedWayLong());
    render->addCars(cars->getCarsNode());
    GfLogInfo("All cars loaded\n");

    screens->InitCars(s);

    if (!grHandle)
    {
        snprintf(buf, 256, "%s%s", GfLocalDir(), GR_PARAM_FILE);
        grHandle = GfParmReadFile(buf, GFPARM_RMODE_STD | GFPARM_RMODE_CREAT);
    }

    return 0;
}

void shutdownTrack(void)
{
    // Do the real track termination job.
    osgDB::Registry::instance()->clearObjectCache();

    if (grTrackHandle)
    {
        GfParmReleaseHandle(grTrackHandle);
        grTrackHandle = 0;
    }

    if (scenery)
    {
        scenery->ShutdownScene();
        delete scenery;
        scenery = NULL;
        GfLogInfo("Delete scenery in OsgMain\n");
    }
}

void shutdownView(void)
{
    if (screens)
    {
        delete screens;
        screens = NULL;
        GfLogInfo("Delete screens in OsgMain\n");
    }

    if (render)
    {
        delete render;
        render = NULL;
        GfLogInfo("Delete render in OsgMain\n");
    }
}

Camera * getCamera(void)
{
    return screens->getActiveView()->getCamera();
}
