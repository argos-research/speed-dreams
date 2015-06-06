/***************************************************************************

    file        : grscreen.h
    created     : Thu May 15 22:11:19 CEST 2003
    copyright   : (C) 2003 by Eric Espié
    email       : eric.espie@torcs.org 
    version     : $Id: grscreen.h 5271 2013-03-07 07:28:29Z mungewell $
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _GRSCREEN_H_
#define _GRSCREEN_H_

#include <car.h>			//tCarElt
#include "grcam.h"		//Cameras

class cGrBoard;
class cGrFrameInfo;

class cGrScreen {
	protected:
		int id;
	
		tCarElt	*curCar;		// Current car viewed.
		tCarElt	**cars;			// List of cars.
	
		int	curCamHead;							// The current camera list.
		tGrCamHead		cams[10];				// From F2 to F11.
	
		class cGrPerspCamera *curCam;			// The current camera.
		class cGrCarCamMirror *mirrorCam;		// The mirror camera.
		class cGrPerspCamera *dispCam;			// The display camera.
		class cGrOrthoCamera *boardCam;			// The board camera.
		class cGrBackgroundCam *bgCam;			// The background camera.
	
		class cGrBoard *board;					// The board.
	
		int drawCurrent;						// Should the current car be drawn.
	
		int scrx, scry, scrw, scrh;
		float viewOffset;
		float viewRatio;
		int fakeWidth;
		int boardWidth;
	
		bool active;								// Is the screen activated.
		bool selectNextFlag;
		bool selectPrevFlag;
		int mirrorFlag;
	
		void loadParams(tSituation *s);			// Load from parameters files.
		void saveCamera(void);

	public:
		cGrScreen(int id);
		~cGrScreen();
	
		void activate(int x, int y, int w, int h, float v);
		inline void deactivate(void) { active = false; }
		inline void setZoom(const long zoom) { curCam->setZoom(zoom); }
		int  isInScreen(int x, int y);
		void update(tSituation *s, const cGrFrameInfo* frameInfo);
		void camDraw(tSituation *s);
		void updateCurrent(tSituation *s);

		void selectCamera(long cam);
		void selectNthCamera(long cam, int nthCam);
		int getNthCamera(void);
		float getViewOffset(void) { return viewOffset; }
		void selectBoard(const long brd);
		void selectTrackMap();
	
		void setCurrentCar(tCarElt *newCurCar);

		void initCams(tSituation *s);
		void initBoard(void);
	
		inline void selectNextCar(void) { selectNextFlag = true; }
		inline void selectPrevCar(void) {	selectPrevFlag = true; }
		void switchMirror(void);
	
		inline tCarElt *getCurrentCar(void) { return curCar; }
		inline cGrCamera* getCurCamera(void) { return curCam; }
		inline float getViewRatio(void) { return viewRatio; }
		inline int getCurCamHead(void) { return curCamHead; }
		inline bool isActive(void) { return active; }
		inline int getId(void) { return id; }
		inline int getScrX (void) { return scrx; }
		inline int getScrY (void) { return scry; }
		inline int getScrW (void) { return scrw; }
		inline int getScrH (void) { return scrh; }
		inline int getBoardWidth(void) { return boardWidth; }
};

#endif //_GRSCREEN_H_
