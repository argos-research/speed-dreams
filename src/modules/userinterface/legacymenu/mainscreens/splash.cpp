/***************************************************************************

    file                 : splash.cpp
    created              : Sat Mar 18 23:49:03 CET 2000
    copyright            : (C) 2000 by Eric Espie
    email                : torcs@free.fr
    version              : $Id: splash.cpp 4761 2012-06-22 04:58:08Z mungewell $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <SDL.h>

#include <tgfclient.h>
#include <glfeatures.h>

#include "splash.h"


static int splImgWidth, splImgHeight; // Real image size (from image file).
static int splImgPow2Width, splImgPow2Height; // Smallest possible containing 2^N x 2^P.
static GLuint splTextureId = 0;

static bool splDisplaying = false;
static bool splTimedOut = false;
static bool splBackgroundWorkDone = false;
static bool (*splBackgroundWork)(void) = 0;
static bool (*splOnClosed)(void) = 0;


/*
 * Function
 *	splashClose
 *
 * Description
 *	Close the splash screen and start main menu
 *
 * Parameters
 *	None
 *
 * Return
 *	Nothing
 *
 * Remarks
 *	
 */
static void splashClose()
{
	// Don't close if background work not completed.
	if (!splBackgroundWorkDone)
		return;

	// Completed ? OK, let's close.
	splDisplaying = false;
	glDeleteTextures(1, &splTextureId);
	splTextureId = 0;

	// And do what was specified.
	if (splOnClosed)
		splOnClosed();
}

/*
 * Function
 *	splashIdle
 *
 * Description
 *	Called by main loop when nothing to do : 
 *  check if splash screen must be closed and close it if so.
 *
 * Parameters
 *	
 *
 * Return
 *	
 *
 * Remarks
 *	
 */
static void splashIdle()
{
	// Do the specified work "in the background" if not already not.
	if (!splBackgroundWorkDone && splBackgroundWork)
	{
		splBackgroundWork();

		// And now it's done, remember it.
		splBackgroundWorkDone = true;
	}
	else
	{
		// Wait a little, to let the CPU take breath.
		GfSleep(0.001);
	}

	// Close if the splash screen delay is over.
	if (splTimedOut)
		splashClose();
}

/*
 * Function
 *	splashKey
 *
 * Description
 *	
 *
 * Parameters
 *	
 *
 * Return
 *	
 *
 * Remarks
 *	
 */
static void splashKey(int /* key */, int /* modifiers */, int /* x */, int /* y */)
{
	splashClose();
}

/*
 * Function
 *	splashTimer
 *
 * Description
 *	End of splash timer callback : can't close splash screen itself,
 *  as not run under the control of the thread that created it
 *
 * Parameters
 *	None
 *
 * Return
 *	None
 *
 * Remarks
 *	
 */
static void splashTimer(int /* value */)
{
	// The splash screen delay is now over.
	if (splDisplaying) 
		splTimedOut = true;
}
	

/*
 * Function
 *	splashDisplay
 *
 * Description
 *
 *
 * Parameters
 *	
 *
 * Return
 *	
 *
 * Remarks
 *	
 */
static void splashDisplay( void )
{
	splDisplaying = true;
		
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glDisable(GL_CULL_FACE);
	glDisable(GL_ALPHA_TEST);
	
	int nScrW, nScrH, nViewW, nViewH;
	GfScrGetSize(&nScrW, &nScrH, &nViewW, &nViewH);
	
	glViewport((nScrW-nViewW) / 2, (nScrH-nViewH) / 2, nViewW, nViewH);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, nScrW, 0, nScrH);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
		
	if (splTextureId) 
	{
		// Prepare texture display.
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, splTextureId);

		// Compute the initial width of the right area and the height of the bottom area
		// of the texture that will not be displayed
		// (We display only the top left rectangle of the quad texture
		//  that corresponds to the original image).
		GLfloat tx1 = 0.0f;
		GLfloat tx2 = splImgWidth / (GLfloat)splImgPow2Width;

		GLfloat ty1 = 1.0f - splImgHeight / (GLfloat)splImgPow2Height;
 		GLfloat ty2 = 1.0;

		// Compute the width/height of the symetrical left/right / top/bottom
		// areas of original image that will need to be clipped
		// in order to keep its aspect ratio.
		const GLfloat rfactor = splImgWidth * (GLfloat)nViewH / splImgHeight / (GLfloat)nViewW;

		if (rfactor >= 1.0f) {
			// If aspect ratio of view is smaller than image's one, "cut off" sides.
			const GLfloat tdx = splImgWidth * (rfactor - 1.0f) / splImgPow2Width / 2.0f;
			tx1 += tdx;
			tx2 -= tdx;
		} else {
			// If aspect ratio of view is larger than image's one, 
			// "cut off" top and bottom.
			const GLfloat tdy = splImgHeight * rfactor / splImgPow2Height / 2.0f;
			ty2 = (ty1+1)/2 + tdy;
			ty1 = (ty1+1)/2 - tdy;
		}

		// Display texture.
		glBegin(GL_QUADS);
		glTexCoord2f(tx1, ty1); glVertex3f(0.0, 0.0, 0.0);
		glTexCoord2f(tx1, ty2); glVertex3f(0.0, nScrH, 0.0);
		glTexCoord2f(tx2, ty2); glVertex3f(nScrW, nScrH, 0.0);
		glTexCoord2f(tx2, ty1); glVertex3f(nScrW, 0.0, 0.0);
		glEnd();
		glDisable(GL_TEXTURE_2D);
	}
		
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, 640, 0, 480);
	
	static float grWhite[4] = {1.0, 1.0, 1.0, 1.0};
	GfuiDrawString(GfuiApp().version().c_str(), grWhite, GFUI_FONT_SMALL_C,
				   440-8, 8, 200, GFUI_ALIGN_HR);

	GfuiSwapBuffers();
}

static void splashMouse(int /* b */, int s, int /* x */, int /* y */)
{
	if (s == SDL_RELEASED) 
		splashClose();
}


/*
 * Function
 *	SplashScreen
 *
 * Description
 *	Display the splash screen and do some specified work in the background (fnBackWork).
 *  Then close it and call the specified fnOnClosed function :
 *  * on mouse click, or keyboard hit or 7 second time-out, if interactive,
 *  * when fnBackWork returns, if not interactive.
 *
 * Parameters
 *	fnBackWork : function to call for background work (loading stuff, ...)
 *  fnOnClosed : function to call when closing the splash screen.
 *  bInteractive : if false, 
 *
 * Return
 *	true on success, false if anything bad happened.
 *
 * Remarks
 *	
 */
bool SplashScreen(bool (*fnBackWork)(void), bool (*fnOnClosed)(void), bool bInteractive)
{
	splBackgroundWork = fnBackWork;
	splOnClosed = fnOnClosed;
	splTimedOut = !bInteractive;

	// Free splash texture if was loaded already.
	if (splTextureId) 
		GfTexFreeTexture(splTextureId); 

	// Load splash texture from file.
	splTextureId = GfTexReadTexture("data/img/splash.jpg", &splImgWidth, &splImgHeight,
									&splImgPow2Width, &splImgPow2Height);

	// Prevent fnOnClosed being called (by splashClose)
	// before fnBackWork (called by splashIdle) is done.
	splBackgroundWorkDone = false;

	// Setup event loop callbacks.
	GfuiApp().eventLoop().setRedisplayCB(splashDisplay);
	if (bInteractive)
	{
		GfuiApp().eventLoop().setKeyboardDownCB(splashKey);
		GfuiApp().eventLoop().setMouseButtonCB(splashMouse);
		GfuiApp().eventLoop().setTimerCB(7000, splashTimer);
	}
	GfuiApp().eventLoop().setRecomputeCB(splashIdle);
    
	return true;
}

