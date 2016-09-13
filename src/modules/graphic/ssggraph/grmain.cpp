/***************************************************************************

    file                 : grmain.cpp
    created              : Thu Aug 17 23:23:49 CEST 2000
    copyright            : (C) 2000 by Eric Espie
    email                : torcs@free.fr
    version              : $Id: grmain.cpp 6071 2015-08-14 16:27:16Z kakukri $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifdef WIN32
#include <windows.h>
#include <GL/gl.h>
#include <GL/glext.h>
#endif

#include <plib/ssg.h>
#include <glfeatures.h> // GfglFeatures
#include <robot.h>	//ROB_SECT_ARBITRARY
#include <graphic.h>

#include "grmain.h"
#include "grshadow.h"
#include "grskidmarks.h"
#include "grsmoke.h"
#include "grcar.h"
#include "grscreen.h"
#include "grscene.h"
#include "grloadac.h"
#include "grutil.h"
#include "grcarlight.h"
#include "grboard.h"
#include "grtracklight.h"
#include "grbackground.h"


int grMaxTextureUnits = 0;

tdble grMaxDammage = 10000.0;
int grNbCars = 0;

void *grHandle = 0;
void *grTrackHandle = 0;

int grWinx, grWiny, grWinw, grWinh;

tgrCarInfo *grCarInfo;
ssgContext grContext;
cGrScreen *grScreens[GR_NB_MAX_SCREEN];
tdble grLodFactorValue = 1.0;

// Frame/FPS info.
static cGrFrameInfo frameInfo;
static double fFPSPrevInstTime;   // Last "instant" FPS refresh time
static unsigned nFPSTotalSeconds; // Total duration since initView

// Mouse coords graphics backend to screen ratios.
static float fMouseRatioX, fMouseRatioY;

// Number of active screens.
int grNbActiveScreens = 1;
int grNbArrangeScreens = 0;
int grSpanSplit = 0;

// Current screen index.
static int nCurrentScreenIndex = 0;

static grssgLoaderOptions options(/*bDoMipMap*/true);

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


static void
grAdaptScreenSize(void)
{
    int i;

    switch (grNbActiveScreens)
	{
		default:
		case 1:
			// Hack to allow span-split to function OK
			if (grNbArrangeScreens > 1) grNbArrangeScreens = 0;

			// Always Full window.
			grScreens[0]->activate(grWinx, grWiny, grWinw, grWinh, 0.0);
			for (i=1; i < GR_NB_MAX_SCREEN; i++)
				grScreens[i]->deactivate();
			break;
		case 2:
			switch (grNbArrangeScreens) {
			default:
				grNbArrangeScreens = 0;
			case 0:
				// Top & Bottom half of the window
				grScreens[0]->activate(grWinx, grWiny + grWinh / 2, grWinw, grWinh / 2, 0.0);
				grScreens[1]->activate(grWinx, grWiny,              grWinw, grWinh / 2, 0.0);
				break;
			case 1:
				// Left & Right half of the window
				grScreens[0]->activate(grWinx,              grWiny, grWinw / 2, grWinh, grSpanSplit * (-0.5 + 10));
				grScreens[1]->activate(grWinx + grWinw / 2, grWiny, grWinw / 2, grWinh, grSpanSplit * (0.5 + 10));
				break;
			case 2:
				// 33/66% Left/Right 
				grScreens[0]->activate(grWinx,              grWiny, grWinw / 3,   grWinh, 0.0);
				grScreens[1]->activate(grWinx + grWinw / 3, grWiny, grWinw * 2/3, grWinh, 0.0);
				break;
			case 3:
				// 66/33% Left/Right 
				grScreens[0]->activate(grWinx,                grWiny, grWinw * 2/3, grWinh, 0.0);
				grScreens[1]->activate(grWinx + grWinw * 2/3, grWiny, grWinw / 3,   grWinh, 0.0);
				break;
			}

			for (i=2; i < GR_NB_MAX_SCREEN; i++)
				grScreens[i]->deactivate();
			break;
		case 3:
			switch (grNbArrangeScreens) {
			default:
				grNbArrangeScreens = 0;
			case 0:
				// Left/Right below wide
				grScreens[0]->activate(grWinx,              grWiny + grWinh / 2, grWinw,     grWinh / 2, 0.0);
				grScreens[1]->activate(grWinx,              grWiny,              grWinw / 2, grWinh / 2, 0.0);
				grScreens[2]->activate(grWinx + grWinw / 2, grWiny,              grWinw / 2, grWinh / 2, 0.0);
				break;
			case 1:
				// All side by side
				grScreens[0]->activate(grWinx,                grWiny, grWinw / 3,   grWinh, grSpanSplit * (-1 + 10));
				grScreens[1]->activate(grWinx + grWinw / 3,   grWiny, grWinw / 3,   grWinh, grSpanSplit * (0.0 + 10));
				grScreens[2]->activate(grWinx + grWinw * 2/3, grWiny, grWinw / 3,   grWinh, grSpanSplit * (1 + 10));
				break;
			case 2:
				// Left/Right above wide
				grScreens[0]->activate(grWinx,              grWiny + grWinh / 2, grWinw / 2, grWinh / 2, 0.0);
				grScreens[1]->activate(grWinx + grWinw / 2, grWiny + grWinh / 2, grWinw / 2, grWinh / 2, 0.0);
				grScreens[2]->activate(grWinx,              grWiny,              grWinw,     grWinh / 2, 0.0);
				break;
			case 3:
				// 50/50% Left plus Top/Bottom on Right
				grScreens[0]->activate(grWinx,              grWiny,              grWinw / 2, grWinh, 0.0);
				grScreens[1]->activate(grWinx + grWinw / 2, grWiny + grWinh / 2, grWinw / 2, grWinh / 2, 0.0);
				grScreens[2]->activate(grWinx + grWinw / 2, grWiny,              grWinw / 2, grWinh / 2, 0.0);
				break;
			case 5:
				// 50/50% Top/Bottom on Left plus Right
				grScreens[0]->activate(grWinx,              grWiny + grWinh / 2, grWinw / 2, grWinh / 2, 0.0);
				grScreens[1]->activate(grWinx + grWinw / 2, grWiny,              grWinw / 2, grWinh, 0.0);
				grScreens[2]->activate(grWinx,              grWiny,              grWinw / 2, grWinh / 2, 0.0);
				break;
			case 6:
				// 66/33% Left plus Top/Bottom on Right
				grScreens[0]->activate(grWinx,                grWiny,              grWinw * 2/3, grWinh, 0.0);
				grScreens[1]->activate(grWinx + grWinw * 2/3, grWiny + grWinh / 2, grWinw / 3,   grWinh / 2, 0.0);
				grScreens[2]->activate(grWinx + grWinw * 2/3, grWiny,              grWinw / 3,   grWinh / 2, 0.0);
				break;
			case 7:
				// 33/66% Top/Bottom on Left plus Right
				grScreens[0]->activate(grWinx,              grWiny + grWinh / 2, grWinw / 3,   grWinh / 2, 0.0);
				grScreens[1]->activate(grWinx + grWinw / 3, grWiny,              grWinw * 2/3, grWinh, 0.0);
				grScreens[2]->activate(grWinx,              grWiny,              grWinw / 3,   grWinh / 2, 0.0);
				break;
			}
			for (i=3; i < GR_NB_MAX_SCREEN; i++)
				grScreens[i]->deactivate();
			break;
		case 4:
			switch (grNbArrangeScreens) {
			case 8:
				// 3 side by side + 1 for rear view - only when spanning splits
				if (grSpanSplit) {
					grScreens[0]->activate(grWinx,                grWiny, grWinw / 4,   grWinh, grSpanSplit * (-1 + 10));
					grScreens[1]->activate(grWinx + grWinw / 4,   grWiny, grWinw / 4,   grWinh, grSpanSplit * (0.0 + 10));
					grScreens[2]->activate(grWinx + grWinw * 2/4, grWiny, grWinw / 4,   grWinh, grSpanSplit * (1 + 10));
					grScreens[3]->activate(grWinx + grWinw * 3/4, grWiny, grWinw / 4,   grWinh, 0.0);
					break;
				} 
			default:
				grNbArrangeScreens = 0;
			case 0:
				// Top/Bottom Left/Rigth Quarters
				grScreens[0]->activate(grWinx,              grWiny + grWinh / 2, grWinw / 2, grWinh / 2, 0.0);
				grScreens[1]->activate(grWinx + grWinw / 2, grWiny + grWinh / 2, grWinw / 2, grWinh / 2, 0.0);
				grScreens[2]->activate(grWinx,              grWiny,              grWinw / 2, grWinh / 2, 0.0);
				grScreens[3]->activate(grWinx + grWinw / 2, grWiny,              grWinw / 2, grWinh / 2, 0.0);
				break;
			case 1:
				// All side by side
				grScreens[0]->activate(grWinx,                grWiny, grWinw / 4,   grWinh, grSpanSplit * (-1.5 + 10));
				grScreens[1]->activate(grWinx + grWinw / 4,   grWiny, grWinw / 4,   grWinh, grSpanSplit * (-0.5 + 10));
				grScreens[2]->activate(grWinx + grWinw * 2/4, grWiny, grWinw / 4,   grWinh, grSpanSplit * (0.5 + 10));
				grScreens[3]->activate(grWinx + grWinw * 3/4, grWiny, grWinw / 4,   grWinh, grSpanSplit * (1.5 + 10));
				break;
			case 2:
				// Left/Middle/Right above wide
				grScreens[0]->activate(grWinx,                grWiny + grWinh / 2, grWinw / 3, grWinh / 2, 0.0);
				grScreens[1]->activate(grWinx + grWinw / 3,   grWiny + grWinh / 2, grWinw / 3, grWinh / 2, 0.0);
				grScreens[2]->activate(grWinx + grWinw * 2/3, grWiny + grWinh / 2, grWinw / 3, grWinh / 2, 0.0);
				grScreens[3]->activate(grWinx,                grWiny,              grWinw,     grWinh / 2, 0.0);
				break;
			case 3:
				// Left/Middle/Right below wide
				grScreens[0]->activate(grWinx,                grWiny + grWinh / 2, grWinw,     grWinh / 2, 0.0);
				grScreens[1]->activate(grWinx,                grWiny,              grWinw / 3, grWinh / 2, 0.0);
				grScreens[2]->activate(grWinx + grWinw / 3,   grWiny,              grWinw / 3, grWinh / 2, 0.0);
				grScreens[3]->activate(grWinx + grWinw * 2/3, grWiny,              grWinw / 3, grWinh / 2, 0.0);
				break;
			case 4:
				// 50/50% Left plus Top/Middle/Bottom on Right
				grScreens[0]->activate(grWinx,              grWiny,                grWinw / 2, grWinh, 0.0);
				grScreens[1]->activate(grWinx + grWinw / 2, grWiny + grWinh * 2/3, grWinw / 2, grWinh / 3, 0.0);
				grScreens[2]->activate(grWinx + grWinw / 2, grWiny + grWinh / 3,   grWinw / 2, grWinh / 3, 0.0);
				grScreens[3]->activate(grWinx + grWinw / 2, grWiny,                grWinw / 2, grWinh / 3, 0.0);
				break;
			case 5:
				// 50/50% Top/Middle/Bottom on Left plus Right
				grScreens[0]->activate(grWinx,              grWiny + grWinh * 2/3, grWinw / 2, grWinh / 3, 0.0);
				grScreens[1]->activate(grWinx + grWinw / 2, grWiny,                grWinw / 2, grWinh, 0.0);
				grScreens[2]->activate(grWinx,              grWiny + grWinh / 3  , grWinw / 2, grWinh / 3, 0.0);
				grScreens[3]->activate(grWinx,              grWiny,                grWinw / 2, grWinh / 3, 0.0);
				break;
			case 6:
				// 66/33% Left plus Top/Middle/Bottom on Right
				grScreens[0]->activate(grWinx,                grWiny,                grWinw * 2/3, grWinh, 0.0);
				grScreens[1]->activate(grWinx + grWinw * 2/3, grWiny + grWinh * 2/3, grWinw / 3,   grWinh / 3, 0.0);
				grScreens[2]->activate(grWinx + grWinw * 2/3, grWiny + grWinh / 3,   grWinw / 3,   grWinh / 3, 0.0);
				grScreens[3]->activate(grWinx + grWinw * 2/3, grWiny,                grWinw / 3,   grWinh / 3, 0.0);
				break;
			case 7:
				// 33/66% Top/Middle/Bottom on Left plus Right
				grScreens[0]->activate(grWinx,              grWiny + grWinh * 2/3, grWinw / 3,   grWinh / 3, 0.0);
				grScreens[1]->activate(grWinx + grWinw / 3, grWiny,                grWinw * 2/3, grWinh, 0.0);
				grScreens[2]->activate(grWinx,              grWiny + grWinh / 3  , grWinw / 3,   grWinh / 3, 0.0);
				grScreens[3]->activate(grWinx,              grWiny,                grWinw / 3,   grWinh / 3, 0.0);
				break;
			}
			for (i=4; i < GR_NB_MAX_SCREEN; i++)
				grScreens[i]->deactivate();
			break;
		case 5:
			switch (grNbArrangeScreens) {
			default:
				grNbArrangeScreens = 0;
			case 0:
				// Top/Bottom Left/Middle/Rigth Matrix
				grScreens[0]->activate(grWinx,                grWiny + grWinh / 2, grWinw / 2, grWinh / 2, 0.0);
				grScreens[1]->activate(grWinx + grWinw / 2  , grWiny + grWinh / 2, grWinw / 2, grWinh / 2, 0.0);
				grScreens[2]->activate(grWinx,                grWiny,              grWinw / 3, grWinh / 2, 0.0);
				grScreens[3]->activate(grWinx + grWinw / 3,   grWiny,              grWinw / 3, grWinh / 2, 0.0);
				grScreens[4]->activate(grWinx + grWinw * 2/3, grWiny,              grWinw / 3, grWinh / 2, 0.0);
				break;
			case 1:
				// All side by side
				grScreens[0]->activate(grWinx,                grWiny, grWinw / 5,   grWinh, grSpanSplit * (-2.0 + 10));
				grScreens[1]->activate(grWinx + grWinw / 5,   grWiny, grWinw / 5,   grWinh, grSpanSplit * (-1.0 + 10));
				grScreens[2]->activate(grWinx + grWinw * 2/5, grWiny, grWinw / 5,   grWinh, grSpanSplit * (0.0 + 10));
				grScreens[3]->activate(grWinx + grWinw * 3/5, grWiny, grWinw / 5,   grWinh, grSpanSplit * (1.0 + 10));
				grScreens[4]->activate(grWinx + grWinw * 4/5, grWiny, grWinw / 5,   grWinh, grSpanSplit * (2.0 + 10));
				break;
			}
			for (i=5; i < GR_NB_MAX_SCREEN; i++)
				grScreens[i]->deactivate();
			break;
		case 6:
			switch (grNbArrangeScreens) {
			case 2:
				if (grSpanSplit) {
					// five side by side + 1 for rear view - only when spanning splits
					grScreens[0]->activate(grWinx,                grWiny, grWinw / 6,   grWinh, grSpanSplit * (-2.0 + 10));
					grScreens[1]->activate(grWinx + grWinw / 6,   grWiny, grWinw / 6,   grWinh, grSpanSplit * (-1.0 + 10));
					grScreens[2]->activate(grWinx + grWinw * 2/6, grWiny, grWinw / 6,   grWinh, grSpanSplit * (0.0 + 10));
					grScreens[3]->activate(grWinx + grWinw * 3/6, grWiny, grWinw / 6,   grWinh, grSpanSplit * (1.0 + 10));
					grScreens[4]->activate(grWinx + grWinw * 4/6, grWiny, grWinw / 6,   grWinh, grSpanSplit * (2.0 + 10));
					grScreens[5]->activate(grWinx + grWinw * 5/6, grWiny, grWinw / 6,   grWinh, 0.0);
					break;
				}
			default:
				grNbArrangeScreens = 0;
			case 0:
				// Top/Bottom Left/Middle/Rigth Matrix
				grScreens[0]->activate(grWinx,                grWiny + grWinh / 2, grWinw / 3, grWinh / 2, 0.0);
				grScreens[1]->activate(grWinx + grWinw / 3,   grWiny + grWinh / 2, grWinw / 3, grWinh / 2, 0.0);
				grScreens[2]->activate(grWinx + grWinw * 2/3, grWiny + grWinh / 2, grWinw / 3, grWinh / 2, 0.0);
				grScreens[3]->activate(grWinx,                grWiny,              grWinw / 3, grWinh / 2, 0.0);
				grScreens[4]->activate(grWinx + grWinw / 3,   grWiny,              grWinw / 3, grWinh / 2, 0.0);
				grScreens[5]->activate(grWinx + grWinw * 2/3, grWiny,              grWinw / 3, grWinh / 2, 0.0);
				break;
			case 1:
				// All side by side
				grScreens[0]->activate(grWinx,                grWiny, grWinw / 6,   grWinh, grSpanSplit * (-2.5 + 10));
				grScreens[1]->activate(grWinx + grWinw / 6,   grWiny, grWinw / 6,   grWinh, grSpanSplit * (-1.5 + 10));
				grScreens[2]->activate(grWinx + grWinw * 2/6, grWiny, grWinw / 6,   grWinh, grSpanSplit * (-0.5 + 10));
				grScreens[3]->activate(grWinx + grWinw * 3/6, grWiny, grWinw / 6,   grWinh, grSpanSplit * (0.5 + 10));
				grScreens[4]->activate(grWinx + grWinw * 4/6, grWiny, grWinw / 6,   grWinh, grSpanSplit * (1.5 + 10));
				grScreens[5]->activate(grWinx + grWinw * 5/6, grWiny, grWinw / 6,   grWinh, grSpanSplit * (2.5 + 10));
				break;
			}
			for (i=6; i < GR_NB_MAX_SCREEN; i++)
				grScreens[i]->deactivate();
			break;
		}
}

static void
grSplitScreen(void *vp)
{
	// Change the number of active screens as specified.
    long p = (long)vp;

    switch (p) {
		case GR_SPLIT_ADD:
			if (grNbActiveScreens < GR_NB_MAX_SCREEN)
				grNbActiveScreens++;
				if (grSpanSplit) 
					grNbArrangeScreens=1;
				else
					grNbArrangeScreens=0;
			break;
		case GR_SPLIT_REM:
			if (grNbActiveScreens > 1)
				grNbActiveScreens--;
				if (grSpanSplit) 
					grNbArrangeScreens=1;
				else
					grNbArrangeScreens=0;
			break;
		case GR_SPLIT_ARR:
			grNbArrangeScreens++;
    }

	// Ensure current screen index stays in the righ range.
	if (nCurrentScreenIndex >= grNbActiveScreens) {
		nCurrentScreenIndex = grNbActiveScreens - 1;
		GfParmSetNum(grHandle, GR_SCT_DISPMODE, GR_ATT_CUR_SCREEN, NULL, nCurrentScreenIndex);
	}

	// Save nb of active screens to user settings.
    GfParmSetNum(grHandle, GR_SCT_DISPMODE, GR_ATT_NB_SCREENS, NULL, grNbActiveScreens);
    GfParmSetNum(grHandle, GR_SCT_DISPMODE, GR_ATT_ARR_SCREENS, NULL, grNbArrangeScreens);
    GfParmWriteFile(NULL, grHandle, "Graph");
    grAdaptScreenSize();
}

static void
grChangeScreen(void *vp)
{
    long p = (long)vp;

    switch (p) {
		case GR_NEXT_SCREEN:
			nCurrentScreenIndex = (nCurrentScreenIndex + 1) % grNbActiveScreens;
			break;
		case GR_PREV_SCREEN:
			nCurrentScreenIndex = (nCurrentScreenIndex - 1 + grNbActiveScreens) % grNbActiveScreens;
			break;
    }

    GfLogInfo("Changing current screen to #%d (out of %d)\n", nCurrentScreenIndex, grNbActiveScreens);

    GfParmSetNum(grHandle, GR_SCT_DISPMODE, GR_ATT_CUR_SCREEN, NULL, nCurrentScreenIndex);
    GfParmWriteFile(NULL, grHandle, "Graph");
}

class cGrScreen *
grGetCurrentScreen(void)
{
    return grScreens[nCurrentScreenIndex];
}

static void
grSetZoom(void *vp)
{
    grGetCurrentScreen()->setZoom((long)vp);
}

static void
grSelectCamera(void *vp)
{
    grGetCurrentScreen()->selectCamera((long)vp);

    // For SpanSplit ensure screens change together
    if (grSpanSplit && grGetCurrentScreen()->getViewOffset() ) {
        long cam;
        int i, subcam;

	cam = (long) vp;
	subcam = grGetCurrentScreen()->getNthCamera();

        for (i=0; i < grNbActiveScreens; i++)
    		if (grScreens[i]->getViewOffset() )
		            grScreens[i]->selectNthCamera(cam, subcam);
    }
}

cGrCamera * grGetCurCamera() 
{
    return grGetCurrentScreen()->getCurCamera();
}

static void
grSelectBoard(void *vp)
{
    grGetCurrentScreen()->selectBoard((long)vp);
}

static void
grSelectTrackMap(void * /* vp */)
{
    grGetCurrentScreen()->selectTrackMap();
}

static void
grPrevCar(void * /* dummy */)
{
    // For SpanSplit ensure screens change together
    if (grSpanSplit && grGetCurrentScreen()->getViewOffset() ) {
        int i;
        tCarElt *car = grGetCurrentScreen()->getCurrentCar();

        for (i=0; i < grNbActiveScreens; i++)
            if (grScreens[i]->getViewOffset() ) {
                grScreens[i]->setCurrentCar(car);
                grScreens[i]->selectPrevCar();
            }
    } else
        grGetCurrentScreen()->selectPrevCar();
}

static void
grNextCar(void * /* dummy */)
{
    // For SpanSplit ensure screens change together
    if (grSpanSplit && grGetCurrentScreen()->getViewOffset() ) {
        int i;
        tCarElt *car = grGetCurrentScreen()->getCurrentCar();

        for (i=0; i < grNbActiveScreens; i++)
            if (grScreens[i]->getViewOffset() ) {
                grScreens[i]->setCurrentCar(car);
                grScreens[i]->selectNextCar();
            }
    } else
        grGetCurrentScreen()->selectNextCar();
}

static void
grSwitchMirror(void * /* dummy */)
{
    grGetCurrentScreen()->switchMirror();
}

int
initView(int x, int y, int width, int height, int /* flag */, void *screen)
{
    int i;

    grWinx = x;
    grWiny = y;
    grWinw = width;
    grWinh = height;

    fMouseRatioX = width / 640.0;
    fMouseRatioY = height / 480.0;

    frameInfo.fInstFps = 0.0;
    frameInfo.fAvgFps = 0.0;
    frameInfo.nInstFrames = 0;
    frameInfo.nTotalFrames = 0;
	fFPSPrevInstTime = GfTimeClock();
    nFPSTotalSeconds = 0;

	// Create the screens and initialize each board.
    for (i = 0; i < GR_NB_MAX_SCREEN; i++) {
	 	grScreens[i] = new cGrScreen(i);
		grScreens[i]->initBoard();
    }

    GfuiAddKey(screen, GFUIK_END,      "Zoom Minimum", (void*)GR_ZOOM_MIN,	grSetZoom, NULL);
    GfuiAddKey(screen, GFUIK_HOME,     "Zoom Maximum", (void*)GR_ZOOM_MAX,	grSetZoom, NULL);
    GfuiAddKey(screen, '*',            "Zoom Default", (void*)GR_ZOOM_DFLT,	grSetZoom, NULL);

    GfuiAddKey(screen, GFUIK_PAGEUP,   "Select Previous Car", (void*)0, grPrevCar, NULL);
    GfuiAddKey(screen, GFUIK_PAGEDOWN, "Select Next Car",     (void*)0, grNextCar, NULL);

    GfuiAddKey(screen, GFUIK_F2,       "1st Person Views",  (void*)0, grSelectCamera, NULL);
    GfuiAddKey(screen, GFUIK_F3,       "3rd Person Views",  (void*)1, grSelectCamera, NULL);
    GfuiAddKey(screen, GFUIK_F4,       "Side Car Views",    (void*)2, grSelectCamera, NULL);
    GfuiAddKey(screen, GFUIK_F5,       "Up Car View",       (void*)3, grSelectCamera, NULL);
    GfuiAddKey(screen, GFUIK_F6,       "Persp Car View",    (void*)4, grSelectCamera, NULL);
    GfuiAddKey(screen, GFUIK_F7,       "All Circuit Views", (void*)5, grSelectCamera, NULL);
    GfuiAddKey(screen, GFUIK_F8,       "Action Cam Views",  (void*)6, grSelectCamera, NULL);
    GfuiAddKey(screen, GFUIK_F9,       "TV Camera Views",   (void*)7, grSelectCamera, NULL);
    GfuiAddKey(screen, GFUIK_F10,      "Helicopter Views",  (void*)8, grSelectCamera, NULL);
    GfuiAddKey(screen, GFUIK_F11,      "TV Director View",  (void*)9, grSelectCamera, NULL);

    GfuiAddKey(screen, '6',            "Dashboard",         (void*)6, grSelectBoard, NULL);
    GfuiAddKey(screen, '5',            "Debug Info",        (void*)3, grSelectBoard, NULL);
    GfuiAddKey(screen, '4',            "G/Cmd Graph",       (void*)4, grSelectBoard, NULL);
    GfuiAddKey(screen, '3',            "Leaders Board",     (void*)2, grSelectBoard, NULL);
    GfuiAddKey(screen, '2',            "Driver Counters",   (void*)1, grSelectBoard, NULL);
    GfuiAddKey(screen, '1',            "Driver Board",      (void*)0, grSelectBoard, NULL);
    GfuiAddKey(screen, '9',            "Mirror",            (void*)0, grSwitchMirror, NULL);
    GfuiAddKey(screen, '0',            "Arcade Board",      (void*)5, grSelectBoard, NULL);
    GfuiAddKey(screen, '+', GFUIM_CTRL, "Zoom In",           (void*)GR_ZOOM_IN,	 grSetZoom, NULL);
    GfuiAddKey(screen, '=', GFUIM_CTRL, "Zoom In",           (void*)GR_ZOOM_IN,	 grSetZoom, NULL);
    GfuiAddKey(screen, '-', GFUIM_CTRL, "Zoom Out",          (void*)GR_ZOOM_OUT, grSetZoom, NULL);
    GfuiAddKey(screen, '>',             "Zoom In",           (void*)GR_ZOOM_IN,	 grSetZoom, NULL);
    GfuiAddKey(screen, '<',             "Zoom Out",          (void*)GR_ZOOM_OUT, grSetZoom, NULL);
    GfuiAddKey(screen, '(',            "Split Screen",   (void*)GR_SPLIT_ADD, grSplitScreen, NULL);
    GfuiAddKey(screen, ')',            "UnSplit Screen", (void*)GR_SPLIT_REM, grSplitScreen, NULL);
    GfuiAddKey(screen, '_',            "Split Screen Arrangement", (void*)GR_SPLIT_ARR, grSplitScreen, NULL);
    GfuiAddKey(screen, GFUIK_TAB,      "Next (split) Screen", (void*)GR_NEXT_SCREEN, grChangeScreen, NULL);
    GfuiAddKey(screen, 'm',            "Track Maps",          (void*)0, grSelectTrackMap, NULL);

	GfLogInfo("Current screen is #%d (out of %d)\n", nCurrentScreenIndex, grNbActiveScreens);

    grInitScene();

    grLodFactorValue = GfParmGetNum(grHandle, GR_SCT_GRAPHIC, GR_ATT_LODFACTOR, NULL, 1.0);

    return 0; // true;
}


int
refresh(tSituation *s)
{
    int	i;

    GfProfStartProfile("refresh");

	// Compute FPS indicators every second.
    frameInfo.nInstFrames++;
	frameInfo.nTotalFrames++;
    const double dCurTime = GfTimeClock();
	const double dDeltaTime = dCurTime - fFPSPrevInstTime;
    if (dDeltaTime > 1.0) {
		++nFPSTotalSeconds;
		fFPSPrevInstTime = dCurTime;
		frameInfo.fInstFps = frameInfo.nInstFrames / dDeltaTime;
		frameInfo.nInstFrames = 0;
		frameInfo.fAvgFps = (double)frameInfo.nTotalFrames / nFPSTotalSeconds;
    }

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

    for (i = 0; i < grNbActiveScreens; i++) {
		grScreens[i]->update(s, &frameInfo);
    }

    grUpdateSmoke(s->currentTime);
    grTrackLightUpdate(s);

    GfProfStopProfile("refresh");
	
    return 0;
}

int
initCars(tSituation *s)
{
	char buf[256];
    char	idx[16];
    int		index;
    int		i;
    tCarElt 	*elt;
    void	*hdle;
    const char *pszSpanSplit;
    int grNbSuggestedScreens = 0;

    TRACE_GL("initCars: start");

    if (!grHandle)
    {
        sprintf(buf, "%s%s", GfLocalDir(), GR_PARAM_FILE);
        grHandle = GfParmReadFile(buf, GFPARM_RMODE_STD | GFPARM_RMODE_CREAT);
    }

    grInitCommonState();
    grInitCarlight(s->_ncars);
    grMaxDammage = (tdble)s->_maxDammage;
    grNbCars = s->_ncars;

    grCustomizePits();

    grCarInfo = (tgrCarInfo*)calloc(s->_ncars, sizeof(tgrCarInfo));

	for (i = 0; i < s->_ncars; i++) {
		elt = s->cars[i];
		/* Car pre-initialization */
		grPreInitCar(elt);
		/* Shadow init (Should be done before the cars for display order) */
		grInitShadow(elt);
		/* Skidmarks init */
		grInitSkidmarks(elt);
	}

    for (i = 0; i < s->_ncars; i++) {
	elt = s->cars[i];
	index = elt->index;
	hdle = elt->_paramsHandle;

	// WARNING: This index hack on the human robot for the Career mode
	//          does no more work with the new "welcome" module system
	//          (the "normal" index has no more the 10 limit) ... TO BE FIXED !!!!!!!
	if (elt->_driverType == RM_DRV_HUMAN && elt->_driverIndex > 10)
		sprintf(idx, "Robots/index/%d", elt->_driverIndex - 11);
	else
		sprintf(idx, "Robots/index/%d", elt->_driverIndex);

	grCarInfo[index].iconColor[0] = GfParmGetNum(hdle, idx, "red",   (char*)NULL, GfParmGetNum(hdle, ROB_SECT_ARBITRARY, "red",   NULL, 0));
	grCarInfo[index].iconColor[1] = GfParmGetNum(hdle, idx, "green", (char*)NULL, GfParmGetNum(hdle, ROB_SECT_ARBITRARY, "green", NULL, 0));
	grCarInfo[index].iconColor[2] = GfParmGetNum(hdle, idx, "blue",  (char*)NULL, GfParmGetNum(hdle, ROB_SECT_ARBITRARY, "blue",  NULL, 0));
	grCarInfo[index].iconColor[3] = 1.0;
	grInitCar(elt);

	// Pre-assign each human driver (if any) to a different screen
	// (set him as the "current driver" for this screen).
	if (grNbSuggestedScreens < GR_NB_MAX_SCREEN
		&& elt->_driverType == RM_DRV_HUMAN && !elt->_networkPlayer) 
	{
	    grScreens[grNbSuggestedScreens]->setCurrentCar(elt);
		GfLogTrace("Screen #%d : Assigned to %s\n", grNbSuggestedScreens, elt->_name);
	    grNbSuggestedScreens++;
	}
    }

	/* Check whether view should be spanned across vertical splits */
	pszSpanSplit = GfParmGetStr(grHandle, GR_SCT_GRAPHIC, GR_ATT_SPANSPLIT, GR_VAL_NO);
	grSpanSplit = strcmp(pszSpanSplit, GR_VAL_YES) ? 0 : 1;
	nCurrentScreenIndex = (int)GfParmGetNum(grHandle, GR_SCT_DISPMODE, GR_ATT_CUR_SCREEN, NULL, nCurrentScreenIndex);

	if (grSpanSplit == 0 && grNbSuggestedScreens > 1) {
		// Mulitplayer, so ignore the stored number of screens
		grNbActiveScreens = grNbSuggestedScreens;
		grNbArrangeScreens = 0;
	} else {
	     	// Load the real number of active screens and the arrangment.
		grNbActiveScreens = (int)GfParmGetNum(grHandle, GR_SCT_DISPMODE, GR_ATT_NB_SCREENS, NULL, 1.0);
		grNbArrangeScreens = (int)GfParmGetNum(grHandle, GR_SCT_DISPMODE, GR_ATT_ARR_SCREENS, NULL, 0.0);
	}

	// Ensure current screen index stays in the righ range.
	if (nCurrentScreenIndex >= grNbActiveScreens) {
		nCurrentScreenIndex = grNbActiveScreens - 1;
		GfParmSetNum(grHandle, GR_SCT_DISPMODE, GR_ATT_CUR_SCREEN, NULL, nCurrentScreenIndex);
	}

	// Initialize the cameras for all the screens.
    for (i = 0; i < GR_NB_MAX_SCREEN; i++) {
	grScreens[i]->initCams(s);
    }

    //Write back to file
    GfParmWriteFile(NULL, grHandle, "Graph");

    TRACE_GL("initCars: end");

	// Initialize other stuff.
    grInitSmoke(s->_ncars);
    grTrackLightInit();

	// Setup the screens (= OpenGL viewports) inside the physical game window.
    grAdaptScreenSize();

    return 0; // true;
}

void
shutdownCars(void)
{
	int i;

	GfOut("-- shutdownCars\n");
	if (grNbCars) {
		grShutdownBoardCar();
		grShutdownSkidmarks();
		grShutdownSmoke();
		grShutdownCarlight();
		grTrackLightShutdown();
		/* Delete ssg objects */
		CarsAnchor->removeAllKids();
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

	for (i = 0; i < GR_NB_MAX_SCREEN; i++) {
		grScreens[i]->setCurrentCar(NULL);
	}

	GfParmReleaseHandle(grHandle);
	grHandle = NULL;

	if (nFPSTotalSeconds > 0)
		GfLogTrace("Average frame rate: %.2f F/s\n",
				   (double)frameInfo.nTotalFrames/((double)nFPSTotalSeconds + GfTimeClock() - fFPSPrevInstTime));
}

int
initTrack(tTrack *track)
{
	// The inittrack does as well init the context, that is highly inconsistent, IMHO.
	// TODO: Find a solution to init the graphics first independent of objects.
	grContext.makeCurrent();

	setupOpenGLFeatures();
    
	grssgSetCurrentOptions(&options);

	// Now, do the real track loading job.
	grTrackHandle = GfParmReadFile(track->filename, GFPARM_RMODE_STD | GFPARM_RMODE_CREAT);
	if (grNbActiveScreens > 0)
		return grLoadScene(track);

	return -1;
}


void
shutdownTrack(void)
{
	// Do the real track termination job.
	grShutdownScene();

	if (grTrackHandle)
	{
		GfParmReleaseHandle(grTrackHandle);
		grTrackHandle = 0;
	}

	// And then the context termination job (should not be there, see initTrack).
	options.endLoad();
	
	grShutdownState();
}

void
shutdownView(void)
{
	for (int i = 0; i < GR_NB_MAX_SCREEN; i++)
	{
		delete grScreens[i];
		grScreens[i] = 0;
	}
}

//void SsgGraph::bendCar(int index, sgVec3 poc, sgVec3 force, int count)
//{
//	if (grCarInfo) 
//		grPropagateDamage (grCarInfo[index].carEntity, poc, force, count);
//}
