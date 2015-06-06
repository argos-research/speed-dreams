/***************************************************************************

    file                 : OsgMain.cpp
    created              : Thu Aug 17 23:23:49 CEST 2000
    copyright            : (C) 2013 by Xavier Bertaux
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

//#include "grOSG.h"
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
#include "OsgCar/OsgCar.h"
#include "OsgWorld/OsgScenery.h"
#include "OsgFX/OsgRender.h"
#include "OsgUtil/OsgMath.h"
#include "OsgView/OsgScreens.h"

//extern	osg::Timer m_timer;
//extern	osg::Timer_t m_start_tick;

SDCars *cars = NULL;
SDScenery *scenery = NULL;
SDRender *render = NULL;
SDScreens * screens = NULL;

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

//static osg::ref_ptr<osg::Group> m_sceneroot = NULL;
//static osg::ref_ptr<osg::Group> m_carroot = NULL;
static osg::Timer m_timer;
//static osg::Timer_t m_start_tick;

int grMaxTextureUnits = 0;

tdble grMaxDammage = 10000.0;
int grNbCars = 0;

void *grHandle = 0;
void *grTrackHandle = 0;

int m_Winx, m_Winy, m_Winw, m_Winh;

//tgrCarInfo *grCarInfo;
//ssgContext grContext;
//cGrScreen *Screens[SD_NB_MAX_SCREEN];
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

//static grssgLoaderOptions options(/*bDoMipMap*/true);

// TODO: Move this to glfeatures.
#ifdef WIN32
PFNGLMULTITEXCOORD2FARBPROC glMultiTexCoord2fARB = NULL;
PFNGLMULTITEXCOORD2FVARBPROC glMultiTexCoord2fvARB = NULL;
PFNGLACTIVETEXTUREARBPROC glActiveTextureARB = NULL;
PFNGLCLIENTACTIVETEXTUREARBPROC glClientActiveTextureARB = NULL;
#endif


// Set up OpenGL features from user settings.
static void setupOpenGLFeatures(void)
{
    static bool bInitialized = false;

    // Don't do it twice.
    if (bInitialized)
        return;

	// Multi-texturing.
	grMaxTextureUnits = 1;
	if (GfglFeatures::self().isSelected(GfglFeatures::MultiTexturing))
	{
		// Use the selected number of texture units.
		grMaxTextureUnits = GfglFeatures::self().getSelected(GfglFeatures::MultiTexturingUnits);

#ifdef WIN32
		// Retrieve the addresses of multi-texturing functions under Windows
		// They are not declared in gl.h or any other header ;
		// you can only get them through a call to wglGetProcAddress at run-time.
		glMultiTexCoord2fARB = (PFNGLMULTITEXCOORD2FARBPROC)wglGetProcAddress("glMultiTexCoord2fARB");
		glActiveTextureARB = (PFNGLACTIVETEXTUREARBPROC)wglGetProcAddress("glActiveTextureARB");
		glClientActiveTextureARB = (PFNGLCLIENTACTIVETEXTUREARBPROC)wglGetProcAddress("glClientActiveTextureARB");
		glMultiTexCoord2fvARB = (PFNGLMULTITEXCOORD2FVARBPROC)wglGetProcAddress("glMultiTexCoord2fvARB");
#endif
	}

	// Done once and for all.
	bInitialized = true;
}

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

void SDSplitScreen(void * vp)
{
    long t = (long)vp;
    screens->splitScreen(t);
}

void SDChangeScreen(void * vp)
{
    long t = (long)vp;
    screens->changeScreen(t);
}

void SDToggleHUD(void * vp)
{
    screens->toggleDebugHUD();
}

int initView(int x, int y, int width, int height, int /* flag */, void *screen)
{
    //render = new SDRender();
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

    //tdble grLodFactorValue = 1.0;
    //m_start_tick = m_timer.tick();

    //m_sceneViewer->getUsage();
    //m_sceneViewer->realize();

    screens->Init(x,y,width,height, render->getRoot());

    /*GfuiAddKey(screen, GFUIK_END,      "Zoom Minimum", (void*)GR_ZOOM_MIN,	grSetZoom, NULL);
    GfuiAddKey(screen, GFUIK_HOME,     "Zoom Maximum", (void*)GR_ZOOM_MAX,	grSetZoom, NULL);
    GfuiAddKey(screen, '*',            "Zoom Default", (void*)GR_ZOOM_DFLT,	grSetZoom, NULL);*/

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
    GfuiAddKey(screen, '(',            "Split Screen",   (void*)SD_SPLIT_ADD, SDSplitScreen, NULL);
    GfuiAddKey(screen, ')',            "UnSplit Screen", (void*)SD_SPLIT_REM, SDSplitScreen, NULL);
    GfuiAddKey(screen, '_',            "Split Screen Arrangement", (void*)SD_SPLIT_ARR, SDSplitScreen, NULL);
    GfuiAddKey(screen, GFUIK_TAB,      "Next (split) Screen", (void*)SD_NEXT_SCREEN, SDChangeScreen, NULL);
    /*GfuiAddKey(screen, 'm',            "Track Maps",          (void*)0, grSelectTrackMap, NULL);*/

    GfLogInfo("Current screen is #%d (out of %d)\n", m_CurrentScreenIndex, m_NbActiveScreens);

    //grLodFactorValue = GfParmGetNum(grHandle, GR_SCT_GRAPHIC, GR_ATT_LODFACTOR, NULL, 1.0);

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
            GfLogTrace("Frame rate (F/s) : Instant = %.1f (Average %.1f)\n",
                       frameInfo.fInstFps, frameInfo.fAvgFps);
    }

    cars->updateCars();
    render->UpdateSky(s->currentTime, s->accelTime);
    screens->update(s,&frameInfo);

    return 0;
}

    /*GfProfStartProfile("refresh");


    TRACE_GL("refresh: start");

	// Moved car collision damage propagation from grcar::grDrawCar.
	// Because it has to be done only once per graphics update, whereas grDrawCar
	// is called once for each car and for each screen.
	grPropagateDamage(s);

	// Update sky if dynamic time enabled.
	grUpdateSky(s->currentTime, s->accelTime);

    GfProfStartProfile("grDrawBackground/glClear");
    glDepthFunc(GL_LEQUAL);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    GfProfStopProfile("grDrawBackground/glClear");

    for (i = 0; i < m_NbActiveScreens; i++) {
                Screens[i]->update(s, &frameInfo);
    }

    grUpdateSmoke(s->currentTime);
    grTrackLightUpdate(s);

    GfProfStopProfile("refresh");

    return 0;
}*/


void shutdownCars(void)
{
    cars->unLoad();
    delete cars;
    cars = NULL;
    //delete m_carroot;

/*	int i;

	GfOut("-- shutdownCars\n");
	if (grNbCars) {
		grShutdownBoardCar();
		grShutdownSkidmarks();
		grShutdownSmoke();
		grShutdownCarlight();
		grTrackLightShutdown();*/
		/* Delete ssg objects */
		/*CarsAnchor->removeAllKids();
		ShadowAnchor->removeAllKids();
		for (i = 0; i < grNbCars; i++) {
			ssgDeRefDelete(grCarInfo[i].envSelector);
			ssgDeRefDelete(grCarInfo[i].shadowBase);
			if (grCarInfo[i].driverSelectorinsg == false) {
				delete grCarInfo[i].driverSelector;
			}
		}

		PitsAnchor->removeAllKids();
		ThePits = 0;
		free(grCarInfo);
	}

	GfParmReleaseHandle(grHandle);
	grHandle = NULL;

	for (i = 0; i < SD_NB_MAX_SCREEN; i++) {
		Screens[i]->setCurrentCar(NULL);
	}
    */

    // Trace final mean F/s.
        if (nFPSTotalSeconds > 0)
                GfLogTrace("Average frame rate: %.2f F/s\n",
                                   (double)frameInfo.nTotalFrames/((double)nFPSTotalSeconds + GfTimeClock() - fFPSPrevInstTime));
}

int initTrack(tTrack *track)
{
	// The inittrack does as well init the context, that is highly inconsistent, IMHO.
	// TODO: Find a solution to init the graphics first independent of objects.

	setupOpenGLFeatures();

	// Now, do the real track loading job.
	grTrackHandle = GfParmReadFile(track->filename, GFPARM_RMODE_STD | GFPARM_RMODE_CREAT);
    //osg::ref_ptr<osg::Group> sceneroot = NULL;
    //m_sceneroot = NULL;
	/*if (m_NbActiveScreens > 0)
		return grLoadScene(track);*/

	scenery = new SDScenery;
    render = new SDRender;

    scenery->LoadScene(track);
    render->Init(track);

    return 0;
}

int  initCars(tSituation *s)
{
	char buf[256];
    cars = new SDCars;
    cars->loadCars(s);
    render->addCars(cars->getCarsNode());
    //osgUtil::Optimizer optimizer;
    //optimizer.optimize(m_sceneroot);
	GfOut("All cars loaded\n");

	screens->InitCars(s);

        if (!grHandle)
        {
                sprintf(buf, "%s%s", GfLocalDir(), GR_PARAM_FILE);
                grHandle = GfParmReadFile(buf, GFPARM_RMODE_STD | GFPARM_RMODE_CREAT);
        }

        // viewer->Init(s);

        return 0;
}

void shutdownTrack(void)
{
    scenery->ShutdownScene();
    delete scenery;
    scenery = NULL;
    // Do the real track termination job.
    osgDB::Registry::instance()->clearObjectCache();
    //m_sceneroot = NULL;
        //grShutdownScene();

	if (grTrackHandle)
	{
		GfParmReleaseHandle(grTrackHandle);
		grTrackHandle = 0;
	}
}

void
shutdownView(void)
{
    delete screens;
    delete render;
    screens = NULL;
    render = NULL;
  //  delete viewer;
}

//void SsgGraph::bendCar(int index, sgVec3 poc, sgVec3 force, int count)
//{
//	if (grCarInfo)
//		grPropagateDamage (grCarInfo[index].carEntity, poc, force, count);
//}*/


Camera * getCamera(void)
{
    return screens->getActiveView()->getCamera();
}
