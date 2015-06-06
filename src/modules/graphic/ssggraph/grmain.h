/***************************************************************************

    file                 : grmain.h
    created              : Fri Aug 18 00:00:41 CEST 2000
    copyright            : (C) 2000 by Eric Espie
    email                : torcs@free.fr
    version              : $Id: grmain.h 4815 2012-07-29 21:40:53Z torcs-ng $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _GRMAIN_H_
#define _GRMAIN_H_

#include <plib/ssg.h>	//ssgContect

#ifdef WIN32
#include <windows.h>
#include <GL/gl.h>
#include <GL/glext.h>
#endif

#include <graphic.h>
#include "grcam.h"
#include <raceman.h>	//tSituation


#ifdef WIN32
// Multi-texturing functions : Under Windows, not present in gl.h or any other ;
// you can only get them through a call to wglGetProcAddress at run-time.
extern PFNGLMULTITEXCOORD2FARBPROC glMultiTexCoord2fARB ;
extern PFNGLMULTITEXCOORD2FVARBPROC glMultiTexCoord2fvARB;
extern PFNGLACTIVETEXTUREARBPROC   glActiveTextureARB ;
extern PFNGLCLIENTACTIVETEXTUREARBPROC glClientActiveTextureARB ;
#endif

extern int grWinx, grWiny, grWinw, grWinh;
extern int grVectFlag;
extern int grVectDispFlag[];

extern double grCurTime;

extern void *grHandle;
extern void *grTrackHandle;

extern ssgContext grContext;
extern int grNbCars;

extern int  initTrack(tTrack *track);
extern int  initCars(tSituation *s);
#define GR_VIEW_STD  0 /* full screen view mode */
#define GR_VIEW_PART 1 /* partial screen view mode (scissor test) */
extern int  initView(int x, int y, int width, int height, int mode, void *screen);
extern int  refresh(tSituation *s);
extern void shutdownCars(void);
extern void shutdownTrack(void);
extern void shutdownView(void);
extern cGrCamera * grGetCurCamera(void);
//extern void bendCar (int index, sgVec3 poc, sgVec3 force, int cnt);

extern int grMaxTextureUnits;
extern tdble grMaxDammage;

// Number of active screens.
extern int grNbActiveScreens;

extern class cGrScreen *grScreens[];
extern class cGrScreen* grGetCurrentScreen(void);


#define GR_SPLIT_ADD	0
#define GR_SPLIT_REM	1
#define GR_SPLIT_ARR	2

#define GR_NEXT_SCREEN	0
#define GR_PREV_SCREEN	1

#define GR_NB_MAX_SCREEN 6

extern tdble grLodFactorValue;

class cGrFrameInfo
{
 public:
	double fInstFps;        // "Instant" frame rate (average along a 1 second shifting window).
	double fAvgFps;         // Average frame rate (since the beginning of the race).
	unsigned nInstFrames;   // Nb of frames since the last "instant" FPS refresh time
	unsigned nTotalFrames;  // Total nb of frames since initView
};

#endif /* _GRMAIN_H_ */ 
