/***************************************************************************

    file        : grscreen.cpp
    created     : Thu May 15 22:11:03 CEST 2003
    copyright   : (C) 2003 by Eric Espie
    email       : eric.espie@torcs.org
    version     : $Id: grscreen.cpp 6164 2015-10-04 23:14:42Z torcs-ng $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <plib/ssg.h>

#include <tgfclient.h>

#include "grscreen.h"
#include "grtrackmap.h"

#include "grmain.h"
#include "grscene.h"	//grDrawScene
#include "grbackground.h"	//grPre/postDrawBackground
#include "grcar.h"	//grDrawCar
#include "grboard.h"	//cGrBoard
#include "grrain.h"
#include "grutil.h"

static char buf[1024];
static char path[1024];
static char path2[1024];

cGrScreen::cGrScreen(int myid)
{
	id = myid;
	curCar = NULL;
	curCam = NULL;
	mirrorCam = NULL;
	dispCam = NULL;
	boardCam = NULL;
	bgCam = NULL;
	board = NULL;
	curCamHead = 0;
	drawCurrent = 0;
	active = false;
	selectNextFlag = false;
	selectPrevFlag = false;
	mirrorFlag = 1;
	memset(cams, 0, sizeof(cams));
	viewRatio = 1.33;
	cars = 0;
	viewOffset = 0;
	
	scrx = 0;
	scry = 0;
	scrw = 800;
	scrh = 600;
}

cGrScreen::~cGrScreen()
{
	int i;
	class cGrCamera *cam;
	
	for (i = 0; i < 10; i++) {
		while ((cam =  GF_TAILQ_FIRST(&cams[i]))) {
			cam->remove(&cams[i]);
			delete cam;
		}
	}
	
	delete boardCam;
	delete mirrorCam;
	delete bgCam;
	
	if (board)
		board->shutdown();
	delete board; // 'delete 0' is safe.
	board = 0;
	
	FREEZ(cars);
}

void
cGrScreen::selectBoard(const long brd)
{
	board->selectBoard(brd);
}


int cGrScreen::isInScreen(int x, int y)
{
	if (!active) {
		return 0;
	}
	
	if ((x >= scrx) &&
		(y >= scry) &&
		(x < (scrx + scrw)) &&
		(y < (scry + scrh)))
	{
		return 1;
	}
	
	return 0;
}

/* Set Screen size & position */
void cGrScreen::activate(int x, int y, int w, int h, float v)
{
	viewRatio = (float)w / (float)h;
	
	scrx = x;
	scry = y;
	scrw = w;
	scrh = h;
	viewOffset = v;
	
	if (boardCam) delete boardCam;

	// Configure board to allocated screen size
	fakeWidth = (int)((float) scrw * 600 / (float) scrh);
	if (fakeWidth < 800)
		fakeWidth = 800;

	boardCam = new cGrOrthoCamera(this, 0, fakeWidth, 0, 600);
	board->setWidth(fakeWidth);

	if (mirrorCam) {
	    // call this method after scr* values have been updated
	    mirrorCam->adaptScreenSize();
	}

	if (curCam) {
		curCam->limitFov ();
#if 0	//SDW
		curCam->setZoom (GR_ZOOM_DFLT);
#endif
		curCam->setViewOffset(v);
	}
	active = true;
}

void cGrScreen::selectTrackMap()
{
	int viewmode;
	
	board->getTrackMap()->selectTrackMap();
	viewmode = board->getTrackMap()->getViewMode();
	
	sprintf(path, "%s/%d", GR_SCT_DISPMODE, id);
	GfParmSetNum(grHandle, path, GR_ATT_MAP, NULL, (tdble)viewmode);

	/* save also as user's preference if human */
	if (curCar->_driverType == RM_DRV_HUMAN) {
		sprintf(path2, "%s/%s", GR_SCT_DISPMODE, curCar->_name);
		GfParmSetNum(grHandle, path2, GR_ATT_MAP, NULL, (tdble)viewmode);
	}
	GfParmWriteFile(NULL, grHandle, "Graph");
}

void cGrScreen::setCurrentCar(tCarElt *newCurCar)
{
	curCar = newCurCar; 

	// probably don't want to clear entry in 'graph.xml'
	if (newCurCar == NULL) return;

	if (curCar)
	{
		sprintf(path, "%s/%d", GR_SCT_DISPMODE, id);
		GfParmSetStr(grHandle, path, GR_ATT_CUR_DRV, curCar->_name);
		GfParmWriteFile(NULL, grHandle, "Graph");
	}
}

void cGrScreen::switchMirror(void)
{
	mirrorFlag = 1 - mirrorFlag;
	sprintf(path, "%s/%d", GR_SCT_DISPMODE, id);
	GfParmSetNum(grHandle, path, GR_ATT_MIRROR, NULL, (tdble)mirrorFlag);

	/* save also as user's preference if human */
	if (curCar->_driverType == RM_DRV_HUMAN) {
		sprintf(path2, "%s/%s", GR_SCT_DISPMODE, curCar->_name);
		GfParmSetNum(grHandle, path2, GR_ATT_MIRROR, NULL, (tdble)mirrorFlag);
	}
	GfParmWriteFile(NULL, grHandle, "Graph");
}


/* Select the camera by number */
void cGrScreen::selectCamera(long cam)
{
	if (cam == curCamHead) {
		/* Same camera list, choose the next one */
		curCam = curCam->next();
		if (curCam == (cGrPerspCamera*)GF_TAILQ_END(&cams[cam])) {
			curCam = (cGrPerspCamera*)GF_TAILQ_FIRST(&cams[cam]);
		}
	} else {
		/* Change of camera list, take the first one */
		curCamHead = cam;
		curCam = (cGrPerspCamera*)GF_TAILQ_FIRST(&cams[cam]);
	}

	if (curCam == NULL) {
		/* back to default camera */
		curCamHead = 0;
		curCam = (cGrPerspCamera*)GF_TAILQ_FIRST(&cams[curCamHead]);
	}

	curCam->setViewOffset(viewOffset);
	saveCamera();
}

int cGrScreen::getNthCamera(void)
{
	int i = 0;
	cGrCamera* testCam = (cGrPerspCamera*)GF_TAILQ_FIRST(&cams[curCamHead]);

	// walk the cam list
	while (1) {
		if (testCam == curCam)
			return i;

		testCam = testCam->next();
		i++;
	}
}

void cGrScreen::selectNthCamera(long cam, int nthCam)
{
	int i = 0;
	curCamHead = cam;
	curCam = (cGrPerspCamera*)GF_TAILQ_FIRST(&cams[curCamHead]);

	while (i < nthCam) {
		curCam = curCam->next();
		if (curCam == (cGrPerspCamera*)GF_TAILQ_END(&cams[cam])){
			curCam = (cGrPerspCamera*)GF_TAILQ_FIRST(&cams[curCamHead]);
			break;
		}
		i++;
	}

	curCam->setViewOffset(viewOffset);
	saveCamera();
}

void cGrScreen::saveCamera(void)
{
	sprintf(path, "%s/%d", GR_SCT_DISPMODE, id);
	GfParmSetStr(grHandle, path, GR_ATT_CUR_DRV, curCar->_name);
	GfParmSetNum(grHandle, path, GR_ATT_CAM, (char*)NULL, (tdble)curCam->getId());
	GfParmSetNum(grHandle, path, GR_ATT_CAM_HEAD, (char*)NULL, (tdble)curCamHead);
	
	/* save also as user's preference if human */
	if (curCar->_driverType == RM_DRV_HUMAN) {
		sprintf(path2, "%s/%s", GR_SCT_DISPMODE, curCar->_name);
		GfParmSetNum(grHandle, path2, GR_ATT_CAM, (char*)NULL, (tdble)curCam->getId());
		GfParmSetNum(grHandle, path2, GR_ATT_CAM_HEAD, (char*)NULL, (tdble)curCamHead);
	}
	
	sprintf(buf, "%s-%d-%d", GR_ATT_FOVY, curCamHead, curCam->getId());
	curCam->loadDefaults(buf);
	drawCurrent = curCam->getDrawCurrent();
	curCam->limitFov ();
	GfParmWriteFile(NULL, grHandle, "Graph");
}

static class cGrPerspCamera *TheDispCam;	/* the display camera */

static int
compareCars(const void *car1, const void *car2)
{
	const float d1 = TheDispCam->getDist2(*(tCarElt**)car1);
	const float d2 = TheDispCam->getDist2(*(tCarElt**)car2);
	
	if (d1 > d2) {
		return -1;
	} else {
		return 1;
	}
}

void cGrScreen::camDraw(tSituation *s)
{
    GfProfStartProfile("dispCam->beforeDraw*");
    dispCam->beforeDraw();
    GfProfStopProfile("dispCam->beforeDraw*");

	glDisable(GL_COLOR_MATERIAL);
	
	GfProfStartProfile("dispCam->update*");
	dispCam->update(curCar, s);
	GfProfStopProfile("dispCam->update*");

	// Draw the static background.
	// Exclude this when sky dome enabled, because it is then actually invisible.
	if (dispCam->getDrawBackground() &&  (grSkyDomeDistance == 0)) 
	{
		glDisable(GL_LIGHTING);
		glDisable(GL_DEPTH_TEST);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		grDrawStaticBackground(dispCam, bgCam);
		glClear(GL_DEPTH_BUFFER_BIT);
	}
	glEnable(GL_DEPTH_TEST);

	// Activate the current display camera.
	GfProfStartProfile("dispCam->action*");
	dispCam->action();
	GfProfStopProfile("dispCam->action*");

	GfProfStartProfile("grDrawCar*");
	
	// Draw the fog
	// TODO: Make this consistent with sky dome own fog / excluded when sky dome enabled ?
 	glFogf(GL_FOG_START, dispCam->getFogStart());
 	glFogf(GL_FOG_END, dispCam->getFogEnd());
 	glEnable(GL_FOG);
	
	// Sort the cars by distance for transparent windows
	TheDispCam = dispCam; // Needed by compareCars() ordering function
	if (dispCam != mirrorCam)
		qsort(cars, s->_ncars, sizeof(tCarElt*), compareCars);
	
	for (int i = 0; i < s->_ncars; i++) {
		grDrawCar(s, cars[i], curCar, dispCam->getDrawCurrent(), dispCam->getDrawDriver(), s->currentTime, dispCam);
	}
	
	GfProfStopProfile("grDrawCar*");

	GfProfStartProfile("grDrawScene*");

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Draw the sky dome if enabled (first part)
	if (dispCam->getDrawBackground() &&  grSkyDomeDistance > 0 ) 
	{
  		grPreDrawSky(s, dispCam->getFogStart(), dispCam->getFogEnd());
  	}

	// Draw the rest of the scene (track, buildings, landscape, cars, ...)
	grDrawScene();

	// Draw the sky dome if enabled (last part)
	if (dispCam->getDrawBackground() &&  grSkyDomeDistance > 0 )
	{
		grPostDrawSky();
	}
	
	GfProfStopProfile("grDrawScene*");

	// Draw the precipitation if any.
	if (dispCam->isMirrorAllowed() == 1) {
		// angle the rain for 1st person views
		grRain.drawPrecipitation(grTrack->local.rain, 1.0, 0.0, 
			curCar->_roll * SG_RADIANS_TO_DEGREES, 0.0, curCar->_speed_x);
	} else
		grRain.drawPrecipitation(grTrack->local.rain, 1.0, 0.0, 0.0, 0.0, 0.0);

    GfProfStartProfile("dispCam->afterDraw*");
    dispCam->afterDraw();
    GfProfStopProfile("dispCam->afterDraw*");
}


/* Update screen display */
void cGrScreen::update(tSituation *s, const cGrFrameInfo* frameInfo)
{
	if (!active) {
		return;
	}
	
	int carChanged = 0;
	if (selectNextFlag) {
		for (int i = 0; i < (s->_ncars - 1); i++) {
			if (curCar == s->cars[i]) {
				curCar = s->cars[i + 1];
				carChanged = 1;
				break;
			}
		}
		selectNextFlag = false;
	}

	if (selectPrevFlag) {
		for (int i = 1; i < s->_ncars; i++) {
			if (curCar == s->cars[i]) {
				curCar = s->cars[i - 1];
				carChanged = 1;
				break;
			}
		}
		selectPrevFlag = false;
	}
	if (carChanged) {
		sprintf(path, "%s/%d", GR_SCT_DISPMODE, id);
		GfParmSetStr(grHandle, path, GR_ATT_CUR_DRV, curCar->_name);
		loadParams (s);
		board->setWidth(fakeWidth);
		GfParmWriteFile(NULL, grHandle, "Graph");
		curCam->onSelect(curCar, s);
	}

	if (grNbActiveScreens > 1) {
		// only need to scissor with split screens (to prevent a problem with Nouvuea driver)
		glEnable(GL_SCISSOR_TEST);
		glViewport(scrx, scry, scrw, scrh);
		glScissor(scrx, scry, scrw, scrh);
		dispCam = curCam;
		camDraw(s);
		glDisable(GL_SCISSOR_TEST);
	} else {
		glViewport(scrx, scry, scrw, scrh);
		dispCam = curCam;
		camDraw(s);
	}

	/* Mirror */
	if (mirrorFlag && curCam->isMirrorAllowed ()) {
		dispCam = mirrorCam;
		camDraw (s);
	}
	
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);    
	glDisable(GL_COLOR_MATERIAL);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_FOG);
	glEnable(GL_TEXTURE_2D);
	
	GfProfStartProfile("boardCam*");
	boardCam->action();
	GfProfStopProfile("boardCam*");
	
	GfProfStartProfile("grDisp**");
	glDisable(GL_TEXTURE_2D);
	
	TRACE_GL("cGrScreen::update glDisable(GL_DEPTH_TEST)");
	if (boardWidth != 100) {
		// Only need to scissor if board does not occupy whole (split) screen
		glEnable(GL_SCISSOR_TEST);
		glScissor(scrx + (scrw * (100 - boardWidth)/200), scry, scrw * boardWidth / 100, scrh);
		board->refreshBoard(s, frameInfo, curCar,
						grNbActiveScreens > 1 && grGetCurrentScreen() == this);
		glDisable(GL_SCISSOR_TEST);
	} else {
		board->refreshBoard(s, frameInfo, curCar,
						grNbActiveScreens > 1 && grGetCurrentScreen() == this);
	}
	TRACE_GL("cGrScreen::update display boards");
	
	GfProfStopProfile("grDisp**");
}

void cGrScreen::loadParams(tSituation *s)
{
	int camNum;
	int i;
	class cGrCamera *cam;
	const char *carName;
	const char *pszSpanSplit;

	// Initialize the screen "current car" if not already done.
	sprintf(path, "%s/%d", GR_SCT_DISPMODE, id);
	
	if (!curCar) {

		// Load the name of the "current driver", and search it in the race competitors.
		carName = GfParmGetStr(grHandle, path, GR_ATT_CUR_DRV, "");
		for (i = 0; i < s->_ncars; i++) {
			if (!strcmp(s->cars[i]->_name, carName)) {
				break;
			}
		}

		// Found : this is the "current driver".
		if (i < s->_ncars) {
			curCar = s->cars[i];

		// Not found, and screen id OK => "current driver" = competitors[screen id].
		} else if (id < s->_ncars) {
			curCar = s->cars[id];

		// Not found and screen id not usable => "current driver" = competitors[0].
		} else {
			curCar = s->cars[0];
		}
		
		GfLogTrace("Screen #%d : Assigned to %s\n", id, curCar->_name);

		GfParmSetStr(grHandle, path, GR_ATT_CUR_DRV, curCar->_name);
		GfParmWriteFile(NULL, grHandle, "Graph");
	}

	// Load "current camera" settings (attached to the "current car").
	curCamHead	= (int)GfParmGetNum(grHandle, path, GR_ATT_CAM_HEAD, NULL, 9);
	camNum	= (int)GfParmGetNum(grHandle, path, GR_ATT_CAM, NULL, 0);
	mirrorFlag	= (int)GfParmGetNum(grHandle, path, GR_ATT_MIRROR, NULL, (tdble)mirrorFlag);

	// Only apply driver preferences when not spanning split screens
	pszSpanSplit = GfParmGetStr(grHandle, GR_SCT_GRAPHIC, GR_ATT_SPANSPLIT, GR_VAL_NO);
	if (strcmp(pszSpanSplit, GR_VAL_YES)) {
		sprintf(path2, "%s/%s", GR_SCT_DISPMODE, curCar->_name);
		curCamHead	= (int)GfParmGetNum(grHandle, path2, GR_ATT_CAM_HEAD, NULL, (tdble)curCamHead);
		camNum	= (int)GfParmGetNum(grHandle, path2, GR_ATT_CAM, NULL, (tdble)camNum);
		mirrorFlag	= (int)GfParmGetNum(grHandle, path2, GR_ATT_MIRROR, NULL, (tdble)mirrorFlag);
	}

	// Get board width (needed for scissor)
	boardWidth      = (int)GfParmGetNum(grHandle, path, GR_ATT_BOARDWIDTH, NULL, 100);
	if (boardWidth < 0 || boardWidth > 100)
		boardWidth = 100;

	// Retrieve the "current camera".
	cam = GF_TAILQ_FIRST(&cams[curCamHead]);
	curCam = NULL;
	while (cam) {
		if (cam->getId() == camNum) {
			curCam = (cGrPerspCamera*)cam;
			break;
		}
		cam = cam->next();
	}

	// Back to the default camera if not found (and save it as the new current one).
	if (!curCam) {
		curCamHead = 0;
		curCam = (cGrPerspCamera*)GF_TAILQ_FIRST(&cams[curCamHead]);
		GfParmSetNum(grHandle, path, GR_ATT_CAM, NULL, (tdble)curCam->getId());
		GfParmSetNum(grHandle, path, GR_ATT_CAM_HEAD, NULL, (tdble)curCamHead);
	}
	
	sprintf(buf, "%s-%d-%d", GR_ATT_FOVY, curCamHead, curCam->getId());
	curCam->loadDefaults(buf);
	drawCurrent = curCam->getDrawCurrent();
	board->loadDefaults(curCar);
}


/* Create cameras */
void cGrScreen::initCams(tSituation *s)
{
	// Get the factor of visibiity from the graphics settings and from the track.
	tdble fovFactor = GfParmGetNum(grHandle, GR_SCT_GRAPHIC, GR_ATT_FOVFACT, (char*)NULL, 1.0);
	fovFactor *= GfParmGetNum(grTrackHandle, TRK_SECT_GRAPH, TRK_ATT_FOVFACT, (char*)NULL, 1.0);

	// If sky dome is enabled, we have a "fixed far" cut plane.
	// Warning: In theory, 2*grSkyDomeDistance+1 should be enough, but it is not (why ?).
	const tdble fixedFar = grSkyDomeDistance ? (2.1f * grSkyDomeDistance + 1.0f) : 0;
	
	GfLogTrace("Screen #%d : FOV = %.2f, Far=%.0f\n", id, fovFactor, fixedFar);

	// Background camera.
	if (!bgCam) {
		bgCam = new cGrBackgroundCam(this);
	}
	
	// Mirror camera.
	if (!mirrorCam) {
		mirrorCam = new cGrCarCamMirror(
			this,
			-1,
			0,					// drawCurr
			1,					// drawBG
			50.0,				// fovy
			0.0,				// fovymin
			360.0,				// fovymax
			0.3,				// near
			fixedFar ? fixedFar : 300.0 * fovFactor, // far
			fixedFar ? 2*fixedFar/3 : 200.0 * fovFactor,	// fogstart
			fixedFar ? fixedFar : 300.0 * fovFactor	// fogend
		);
	}
	
	// Scene Cameras
	memset(cams, 0, sizeof(cams));
	grCamCreateSceneCameraList(this, cams, fovFactor, fixedFar);
	
	cars = (tCarElt**)calloc(s->_ncars, sizeof (tCarElt*));
	for (int i = 0; i < s->_ncars; i++) {
		cars[i] = s->cars[i];
	}
	
	loadParams(s);
}

void cGrScreen::initBoard(void)
{
	if (board == NULL) {
		board = new cGrBoard (id);
	}
	board->initBoard ();
}
