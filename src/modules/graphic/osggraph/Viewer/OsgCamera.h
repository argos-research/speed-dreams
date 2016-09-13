/***************************************************************************

    file                 : OsgCamera.h
    created              : Tue Feb 26 12:24:02 CEST 2013
    copyright            : (C) 2012 by Gaëtan André
    email                : gaetan.andre@gmail.com
    version              : $Id$

***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _OSGCAMERA_H_
#define _OSGCAMERA_H_

#include <osg/Vec3>
#include <vector>
#include <camera.h>

#define GR_ZOOM_IN 	0
#define GR_ZOOM_OUT 	1
#define GR_ZOOM_MAX 	2
#define GR_ZOOM_MIN 	3
#define GR_ZOOM_DFLT    4

#define CAMERA_LISTS 10

class SDCamera;
class SDView;

class SDCameras
{
protected:
    SDView * screen;
    std::vector<SDCamera *> cameras[CAMERA_LISTS];
    int selectedList;
    int selectedCamera;
    bool cameraHasChanged;

public:
    SDCameras(SDView *c,int ncars);
    SDCamera * getSelectedCamera();
    void nextCamera(int list);
    void update(tCarElt * car, tSituation * s);
    inline int getIntSelectedCamera() { return selectedCamera; }
    inline int getIntSelectedList() { return selectedList; }
    void selectCamera(int list, int cam);

    inline void getIntSelectedListAndCamera(int *list,int *cam)
    {
        *list = selectedList;
        *cam = selectedCamera;
    }

    static void loadSpanValues();

    ~SDCameras();
};

class SDCamera
{
private :
    int			id;				/* Camera Id */
    int			drawCurrent;	/* flag to draw the current car */
	int			drawCockpit;	/* flag to draw the current cockpit */
    int			drawDriver;		/* flag to draw the driver */
    int			drawBackground;	/* flag to draw the background */
    int			mirrorAllowed;	/* flag to allow the display of mirror */

protected:
    SDView *screen;
    osg::Vec3 eye,center,up,speed;
    int Speed;

public:
    // Constructor
    SDCamera(SDView * screen, int myid = 0, int mydrawCurrent = 0, int mydrawCkt= 0, int mydrawdrv = 0, int mydrawBackground = 0, int mymirrorAllowed = 0);

    inline int getId() { return id; }

    virtual void update(tCarElt *car, tSituation *s) = 0;	/* Change the camera if necessary */
    virtual void setProjection(void) = 0;
    virtual void setModelView(void) = 0;
    virtual void setZoom(int cmd) = 0;				/* Set the zoom with commands */
    virtual void loadDefaults(char *attr) = 0;			/* Load the default values from parameter file */
    virtual void onSelect(tCarElt *car, tSituation *s) = 0;	/* called when the camera is selected */

    virtual float getLODFactor(float x, float y, float z) = 0;	/* Get the LOD factor for an object located at x,y,z */

    virtual void setViewOffset(float newOffset)=0;
    inline float getAspectRatio()		{ return 0.0; }
	inline int   getdrawCockpit()		{ return drawCockpit; }
	inline int   getdrawDriver()		{ return drawDriver; }
    inline int   getMirrorAllowed()		{ return mirrorAllowed; }
    inline osg::Vec3 getCameraPosition(){ return eye; }

    //camera for sound
    Camera * getGenericCamera();
    //void update(tCarElt * car, tSituation * s);

    /* Set the camera view */
    void action(void)
    {
        setProjection();
        setModelView();
    }

    // Destructor
    virtual ~SDCamera(){}
};

class SDPerspCamera : public SDCamera
{
protected:
    float fovy;
    float fovymin;
    float fovymax;
    float fovydflt;
    float fnear;
    float ffar;
    float fogstart;
    float fogend;
    float viewOffset;
    float spanAngle;
    float spanOffset;

public:
    SDPerspCamera(SDView *myscreen, int id, int drawCurr, int drawCkt, int drawDrv, int drawBG, int mirrorAllowed,
                  float myfovy, float myfovymin, float myfovymax,
                  float myfnear, float myffar = 1500.0, float myfogstart = 1400.0, float myfogend = 1500.0);

    //virtual void update(tCarElt *car, tSituation *s) = 0;	/* Change the camera if necessary */
    virtual void setProjection(void);
    virtual void setModelView(void);
    virtual void loadDefaults(char *attr);
    float getSpanAngle(void);
    void setViewOffset(float newOffset);
    virtual void setZoom(int cmd);
    float getLODFactor(float x, float y, float z);
    float getFogStart(void) { return fogstart; }
    float getFogEnd(void) { return fogend; }

    virtual void limitFov(void)  {}
    void onSelect(tCarElt *car, tSituation *s) {}

    virtual float getFovY(void) { return fovy; }

};

class SDCarCamMirror : public SDPerspCamera
{
protected:
    int		vpx, vpy, vpw, vph;	/* viewport size */
    int		mx, my, mw, mh;		/* drawing area */
    float   aspectRatio;        /* the aspect ratio of the mirror: mw / mh */
    float   origFovY;           /* fovy set using constructor */

public:
    SDCarCamMirror(SDView *myscreen, int id, int drawCurr, int drawBG,
                   float myfovy, float myfovymin, float myfovymax,
                   float myfnear, float myffar = 1500.0,
                   float myfogstart = 1400.0, float myfogend = 1500.0);

    void update (tCarElt *car, tSituation *s);

    virtual float getAspectRatio() { return aspectRatio; }

    void setViewport (int x, int y, int w, int h);
    void setScreenPos (int x, int y, int w, int h);

    virtual void setModelView(void);

    /** Called by cGrScreen::activate() after the screen updated it's screen size.
     *  Cameras should use the SDCamera::screen property to get the updated information. */
    void adaptScreenSize();

    virtual void limitFov(void);
};

#endif // _OSGCAMERA_H_
