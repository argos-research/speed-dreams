/***************************************************************************

    file        : OsgViewer.h
    created     : Sat Jan 2013 22:11:19 CEST 2013
    copyright   : (C) 2013 by Xavier Bertaux
    email       : bertauxx@yahoo.fr
    version     : $Id$
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _OSGVIEWER_H_
#define _OSGVIEWER_H_

#include <camera.h>
#include <raceman.h>        	//tSituation
#include <car.h>		//tCarElt

#include "OsgCamera.h"		//SDCameras

class SDFrameInfo;

class SDView
{
protected:
    osg::Camera * cam;
    osg::Camera * mirrorCam;

    int id;
    int x, y, width, height;
    float viewOffset;

    tCarElt	*curCar;		// Current car viewed.
    tCarElt	**cars;			// List of cars.

    bool selectNextFlag;
    bool selectPrevFlag;
    bool mirrorFlag;
    bool hasChangedMirrorFlag;

    SDCameras *cameras;
    SDCarCamMirror * mirror;

    //class cGrPerspCamera *curCam;			// The current camera.
    //class cGrCarCamMirror *mirrorCam;		// The mirror camera.
    //class cGrPerspCamera *dispCam;			// The display camera.
    //class cGrOrthoCamera *boardCam;			// The board camera.
    //class cGrBackgroundCam *bgCam;			// The background camera.

    //class cGrBoard *board;					// The board.

    void loadParams(tSituation *s);			// Load from parameters files.

public:
    SDView(osg::Camera * c, int x , int y, int width , int height, osg::Camera * mc);
    ~SDView();

    void Init(tSituation *s);
    void update(tSituation *s, const SDFrameInfo* frameInfo);

    inline void setCurrentCar(tCarElt *newCurCar) { curCar = newCurCar; }
    inline int getId(){return id;}
    inline void selectNextCar(void) { selectNextFlag = true; }
    inline void selectPrevCar(void) { selectPrevFlag = true; }

    void switchMirror(void);
    void de_activateMirror();

    inline tCarElt *getCurrentCar(void) { return curCar; }
    inline SDCameras *getCameras() { return cameras; }

    inline tdble getViewRatio() { return width/(tdble)height; }
    //used for sound
    Camera* getCamera();

    inline osg::Camera *  getOsgCam(void) { return cam; }
    inline osg::Camera *  getOsgMirrorCam(void) { return mirrorCam; }

    inline int  getScreenXPos(void) { return x; }
    inline int  getScreenYPos(void) { return y; }

    inline int  getScreenWidth(void) { return width; }
    inline int  getScreenHeight(void) { return height; }

    inline float getViewOffset() {return viewOffset;}

    void saveCamera();

    void activate(int x, int y, int width, int height,float v);
    void deactivate();
};

#endif //_OSGVIEWER_H_
