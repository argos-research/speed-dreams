/***************************************************************************
                           screen.cpp -- screen init
                             -------------------
    created              : Fri Aug 13 22:29:56 CEST 1999
    copyright            : (C) 1999, 2004 by Eric Espie, Bernhard Wymann
    email                : torcs@free.fr
    version              : $Id: guiscreen.cpp 6316 2015-12-23 19:19:47Z beaglejoe $
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

/** @file
    Screen management.
    @author	<a href=mailto:torcs@free.fr>Eric Espie</a>
    @version	$Id: guiscreen.cpp 6316 2015-12-23 19:19:47Z beaglejoe $
    @ingroup	screen
*/

#include <cstdio>
#include <cstring>
#include <cmath>
#include <sstream>

#ifdef WIN32
#include <windows.h>
#include <process.h>
#else
#include <unistd.h>
#endif

#include <SDL.h>
#include <SDL_video.h>

#include <portability.h>

#include "tgfclient.h"
#include "gui.h"

#include "glfeatures.h"

// The screen properties.
static int GfScrWidth;
static int GfScrHeight;
static int GfViewWidth;
static int GfViewHeight;
static int GfScrCenX;
static int GfScrCenY;

// The screen surface.
static SDL_Surface *PScreenSurface = NULL;

/* Default list of screen color depths (bits per pixel, alpha included) in case
   something went wrong during hardware / driver capabilities detection */
static int ADefScreenColorDepths[] = { 16, 24, 32 };
static const int NDefScreenColorDepths =
	sizeof(ADefScreenColorDepths) / sizeof(ADefScreenColorDepths[0]);

/* Default list of screen sizes ("resolutions") in case
   something went wrong during hardware / driver capabilities detection */
static tScreenSize ADefScreenSizes[] =
{
	{  320,  200 },
	{  320,  240 },
	{  400,  300 },
	{  416,  312 },
	{  480,  360 },
	{  512,  384 },
	{  576,  384 },
	{  576,  432 },
	{  640,  384 },
	{  640,  400 },
	{  640,  480 },
	{  640,  512 },
	{  700,  525 },
	{  720,  450 },
	{  800,  512 },
	{  800,  600 },
	{  832,  624 },
	{  840,  525 },
	{  896,  672 },
	{  928,  696 },
	{  960,  600 },
	{  960,  720 },
	{ 1024,  600 },
	{ 1024,  768 },
	{ 1152,  768 },
	{ 1152,  864 },
	{ 1280,  600 },
	{ 1280,  720 },
	{ 1280,  768 },
	{ 1280,  800 },
	{ 1280,  960 },
	{ 1280, 1024 },
	{ 1366,  768 },
	{ 1400, 1050 },
	{ 1440,  900 },
	{ 1600,  900 },
	{ 1600, 1024 },
	{ 1680, 1050 },
	{ 1792, 1344 },
	{ 1800, 1440 },
	{ 1920, 1080 },
	{ 1920, 1200 },
	{ 3840, 2160 }
};
static const int NDefScreenSizes =
	sizeof(ADefScreenSizes) / sizeof(ADefScreenSizes[0]);

/** Get the default / fallback screen / window sizes (pixels).
    @ingroup	screen
    @param	pnSizes	Address of number of default sizes (output).
    @return	Array of detected supported sizes (static data, never free).
 */
tScreenSize* GfScrGetDefaultSizes(int* pnSizes)
{
	*pnSizes = NDefScreenSizes;
	
	return ADefScreenSizes;
}

/** Get the supported screen / window sizes (pixels) for the given color depth and display mode.
    @ingroup	screen
    @param	nColorDepth	Requested color depth (bits)
    @param	bFullScreen	Requested display mode : full-screeen mode if true, windowed otherwise.
    @param	pnSizes	Address of number of detected supported sizes (output) (-1 if any size is supported).
    @return	Array of detected supported sizes (allocated on the heap, must use free at the end), or 0 if no detected supported size, or -1 if any size is supported.
	@note   The vertical refresh rate is not taken into account as a parameter for detection here, due to SDL API not supporting this ; fortunately, when selecting a given video mode, SDL ensures to (silently) select a safe refresh rate for the selected mode, which may be of some importantce especially in full-screen modes.
 */
tScreenSize* GfScrGetSupportedSizes(int nColorDepth, bool bFullScreen, int* pnSizes)
{
#if SDL_MAJOR_VERSION >= 2
	/* Build list of available screen sizes */
	int avail;
	SDL_DisplayMode mode;
	Uint32 format;
	SDL_Rect bounds;
	tScreenSize* aSuppSizes;
    tScreenSize* tmpSuppSizes;
	tScreenSize last;

	last.width = 0;
	last.height = 0;

	if (bFullScreen)
		avail = SDL_GetNumDisplayModes(0);
	else
		avail = NDefScreenSizes;

	GfLogInfo("SDL2: modes availabled %d\n", avail);
	*pnSizes = 0;

	if(SDL_GetDisplayBounds(0, &bounds) == 0)
		GfLogInfo("Display bounds %dx%d\n", bounds.w , bounds.h );
	else {
		bounds.w = 0;
		bounds.h = 0;
	}

	if (avail) {
		/* Overzealous malloc */
		tmpSuppSizes = (tScreenSize*)malloc((avail+1) * sizeof(tScreenSize));

		while (avail) {
			if (bFullScreen == 0) {
				/* list any size <= desktop size */
				if (ADefScreenSizes[avail-1].width <= bounds.w 
						&&  ADefScreenSizes[avail-1].height <= bounds.h) {
					tmpSuppSizes[*pnSizes].width  = ADefScreenSizes[avail-1].width;
					tmpSuppSizes[*pnSizes].height = ADefScreenSizes[avail-1].height;

					GfLogInfo(" %dx%d,", tmpSuppSizes[*pnSizes].width, tmpSuppSizes[*pnSizes].height);
					(*pnSizes)++;
				}
			}
			else if (SDL_GetDisplayMode(0, avail-1, &mode) == 0) {
				if (SDL_BITSPERPIXEL(mode.format) == nColorDepth
#if 1	// ignore multiple entries with different frequencies
						&& (last.width != mode.w || last.height != mode.h)
#endif
						) {
					tmpSuppSizes[*pnSizes].width  = mode.w;
					tmpSuppSizes[*pnSizes].height = mode.h;

					GfLogInfo(" %dx%d,", tmpSuppSizes[*pnSizes].width, tmpSuppSizes[*pnSizes].height);
					(*pnSizes)++;

					last.width = mode.w;
					last.height = mode.h;
				}
			}
			avail--;
		}

		/* work around SDL2 bug, add desktop bounds as option */
		if (bFullScreen && (bounds.w != last.width || bounds.h != last.height)) {
			tmpSuppSizes[*pnSizes].width  = bounds.w;
			tmpSuppSizes[*pnSizes].height = bounds.h;

			GfLogInfo(" %dx%d,", tmpSuppSizes[*pnSizes].width, tmpSuppSizes[*pnSizes].height);
			(*pnSizes)++;
		}

	} else {
		GfLogInfo(" None.");
		tmpSuppSizes = (tScreenSize*) NULL;
	}
	GfLogInfo("\nModes selected %d\n", *pnSizes);

   // reverse the array so they appear in correct order in GUI...
   if(*pnSizes > 0) {
      aSuppSizes = (tScreenSize*)malloc((*pnSizes) * sizeof(tScreenSize));
      int maxindex = *pnSizes - 1;
      for(int i = maxindex; i >= 0; i--) {
         aSuppSizes[i] = tmpSuppSizes[maxindex - i];
      }
      // ...and free the temporary array
      free(tmpSuppSizes);
   }
   else {
      aSuppSizes = NULL;
   }
#else
	// Query system video capabilities.
	const SDL_VideoInfo* sdlVideoInfo = SDL_GetVideoInfo();

	if (!sdlVideoInfo)
	{
		GfLogWarning("Could not SDL_GetVideoInfo (%s)\n", SDL_GetError());
		*pnSizes = 0;
		return 0;
	}
	
	// Get best supported pixel format.
	SDL_PixelFormat sdlPixelFormat;
	memcpy(&sdlPixelFormat, &(sdlVideoInfo->vfmt), sizeof(SDL_PixelFormat));
		
	//sdlPixelFormat.palette = 0;
	//sdlPixelFormat.BitsPerPixel = ;
	//sdlPixelFormat.BytesPerPixel = ;
	//sdlPixelFormat.Rloss = ;
	//sdlPixelFormat.Gloss = ;
	//sdlPixelFormat.Bloss = ;
	//sdlPixelFormat.Aloss = ;
	//sdlPixelFormat.Rshift = ;
	//sdlPixelFormat.Gshift = ;
	//sdlPixelFormat.Bshift = ;
	//sdlPixelFormat.Ashift = ;
	//sdlPixelFormat.Rmask = ;
	//sdlPixelFormat.Gmask = ;
	//sdlPixelFormat.Bmask = ;
	//sdlPixelFormat.Amask = ;
	//sdlPixelFormat.colorkey = ;
	//sdlPixelFormat.alpha = ;

	// Update the pixel format to match the requested color depth.
	sdlPixelFormat.BitsPerPixel = nColorDepth;
	sdlPixelFormat.BytesPerPixel = nColorDepth / 8;

	// Select the requested display mode.
	Uint32 sdlDisplayMode = SDL_OPENGL;
	if (bFullScreen)
		sdlDisplayMode |= SDL_FULLSCREEN;
	
	// Get the supported sizes for this pixel format.
	SDL_Rect **asdlSuppSizes = SDL_ListModes(&sdlPixelFormat, sdlDisplayMode);

	GfLogInfo("Available %u-bit %s video sizes :",
			  sdlPixelFormat.BitsPerPixel, bFullScreen ? "full-screen" : "windowed");

	tScreenSize* aSuppSizes;
	if (asdlSuppSizes == (SDL_Rect**)0)
	{
		GfLogInfo(" None.\n");
		aSuppSizes = (tScreenSize*)0;
		*pnSizes = 0;
	}
	else if (asdlSuppSizes == (SDL_Rect**)-1)
	{
		GfLogInfo(" Any.\n");
		aSuppSizes = (tScreenSize*)-1;
		*pnSizes = -1;
	}
	else
	{
		// Count the supported sizes.
		*pnSizes = 0;
		while (asdlSuppSizes[*pnSizes])
			(*pnSizes)++;

		// Copy them into the output array.
		aSuppSizes = (tScreenSize*)malloc((*pnSizes)*sizeof(tScreenSize));
		for (int nSizeInd = 0; nSizeInd < *pnSizes; nSizeInd++)
		{
			aSuppSizes[nSizeInd].width  = asdlSuppSizes[*pnSizes - 1 - nSizeInd]->w;
			aSuppSizes[nSizeInd].height = asdlSuppSizes[*pnSizes - 1 - nSizeInd]->h;
			GfLogInfo(" %dx%d,", aSuppSizes[nSizeInd].width, aSuppSizes[nSizeInd].height);
		}
		GfLogInfo("\n");
	}
#endif
	
	return aSuppSizes;
}

/** Get the default / fallback screen / window color depths (bits per pixels, alpha included).
    @ingroup	screen
    @param	pnColorDepths	Address of number of default sizes (output).
    @return	Array of detected supported sizes (static data, never free).
 */
int* GfScrGetDefaultColorDepths(int* pnColorDepths)
{
	*pnColorDepths = NDefScreenColorDepths;
	
	return ADefScreenColorDepths;
}

/** Get the supported color depths as supported by the underlying hardware/driver.
    @ingroup	screen
    @param	pnDepths	Address of number of detected color depths (output)
    @return	Array of detected supported color depths (allocated on the heap, must use free at the end)
 */
int* GfScrGetSupportedColorDepths(int* pnDepths)
{
#if SDL_MAJOR_VERSION >= 2
	// Need to completely re-write this function
	*pnDepths = NDefScreenColorDepths;
	
	return ADefScreenColorDepths;
#else
	// Determine the maximum supported color depth (default to 32 in any case).
	const SDL_VideoInfo* sdlVideoInfo = SDL_GetVideoInfo();
	int nMaxColorDepth = 32;
	if (sdlVideoInfo)
	{
		nMaxColorDepth = sdlVideoInfo->vfmt->BitsPerPixel;
		if (nMaxColorDepth > 32)
			nMaxColorDepth = 32;
	}
	else
		GfLogWarning("Could not SDL_GetVideoInfo (%s)\n", SDL_GetError());

	// We support a minimum color depth of 16 bits.
	const int nMinColorDepth = 16;

	// So we can't have more than ... supported color depths.
	const int nMaxColorDepths = 1 + (nMaxColorDepth - nMinColorDepth) / 8;
	
	// Check video backend capabilities for each color depth between min and max,
	// and store in target array if OK.
	int nSuppSizes;
	tScreenSize* aSuppSizes;
	int* aSuppDepths = (int*)malloc(nMaxColorDepths*sizeof(int));
	*pnDepths = 0;
	for (int nDepthInd = 0; nDepthInd < nMaxColorDepths; nDepthInd++)
	{
		const int nCheckedColorDepth = nMinColorDepth + 8 * nDepthInd;

		// Windowed mode.
		aSuppSizes = GfScrGetSupportedSizes(nCheckedColorDepth, false, &nSuppSizes);
		const bool bWindowedOK = (aSuppSizes != 0);
		if (aSuppSizes && aSuppSizes != (tScreenSize*)-1)
			free(aSuppSizes);

		// Full-screen mode
		aSuppSizes = GfScrGetSupportedSizes(nCheckedColorDepth, true, &nSuppSizes);
		const bool bFullScreenOK = (aSuppSizes != 0);
		if (aSuppSizes && aSuppSizes != (tScreenSize*)-1)
			free(aSuppSizes);

		// Keep this color depth if one of the display modes work
		// TODO: Shouldn't we use "and" here ?
		if (bWindowedOK || bFullScreenOK)
		{
			aSuppDepths[*pnDepths] = nCheckedColorDepth;
			(*pnDepths)++;
		}
	}

	// Report supported depths.
	if (*pnDepths == 0)
	{
		// Fallback : assume at least 24 bit depth is supported.
		GfLogWarning("SDL reports no supported color depth : assuming 32 bit is OK");
		aSuppDepths[*pnDepths] = 32;
		(*pnDepths)++;
	}
	else
	{
		GfLogInfo("Supported color depths (bits) :");
		for (int nDepthInd = 0; nDepthInd < *pnDepths; nDepthInd++)
			GfLogInfo(" %d,", aSuppDepths[nDepthInd]);
		GfLogInfo("\n");
	}

	return aSuppDepths;
#endif
}

static void gfScrReshapeViewport(int width, int height)
{
    glViewport((width-GfViewWidth)/2, (height-GfViewHeight)/2, GfViewWidth,  GfViewHeight);
    glMatrixMode(GL_PROJECTION );
    glLoadIdentity();
    glOrtho(0.0, 640.0, 0.0, 480.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    GfScrWidth = width;
    GfScrHeight = height;
    GfScrCenX = width / 2;
    GfScrCenY = height / 2;
}
#if SDL_MAJOR_VERSION >= 2

SDL_Surface* gfScrCreateWindow(int nWinWidth, int nWinHeight, int nTotalDepth,int bfVideoMode)
{
    if(GfuiWindow)
    {
        SDL_DestroyWindow(GfuiWindow);
        GfuiWindow = NULL;
    }
    if(PScreenSurface)
    {
        SDL_FreeSurface(PScreenSurface);
        PScreenSurface = NULL;
    }
    // Set window/icon captions
    std::ostringstream ossCaption;
    ossCaption << GfuiApp().name() << ' ' << GfuiApp().version();

    GfuiWindow = SDL_CreateWindow(ossCaption.str().c_str(),
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        nWinWidth, nWinHeight, SDL_WINDOW_HIDDEN | SDL_WINDOW_OPENGL);


#if !defined(__APPLE__)
    // Set window icon (MUST be a 32x32 icon for Windows, and with black pixels as alpha ones,
    // as BMP doesn't support transparency).
    std::ostringstream ossIconFilename;
    ossIconFilename << GfDataDir() << "data/icons/icon.bmp";
    SDL_Surface* surfIcon = SDL_LoadBMP(ossIconFilename.str().c_str());
    if (surfIcon)
    {
        SDL_SetColorKey(surfIcon, TRUE, SDL_MapRGB(surfIcon->format, 0, 0, 0));
        SDL_SetWindowIcon(GfuiWindow, surfIcon);
        SDL_FreeSurface(surfIcon);
    }
#endif
    // attempt to make window operational
    SDL_Renderer *renderer = SDL_CreateRenderer(GfuiWindow, -1, 0);
    SDL_RenderPresent(renderer);

    /* Create OpenGL context */
    SDL_GLContext context;
    context = SDL_GL_CreateContext(GfuiWindow);

    // If specified, try best possible settings.
    PScreenSurface = SDL_CreateRGBSurface(0, nWinWidth, nWinHeight, nTotalDepth,
#if SDL_BYTEORDER == SDL_LIL_ENDIAN 
        0x00FF0000, 0x0000FF00, 0x000000FF,
#else
        0x000000FF, 0x0000FF00, 0x00FF0000,
#endif
        0x00000000);

    if (bfVideoMode & SDL_WINDOW_FULLSCREEN)
    {
        SDL_Rect bounds;

        /* Work around SDL2 bug */
        if (SDL_GetDisplayBounds(0, &bounds) == 0) {
            if (bounds.w == nWinWidth && bounds.h == nWinHeight)
                SDL_SetWindowFullscreen(GfuiWindow, SDL_WINDOW_FULLSCREEN_DESKTOP);
            else SDL_SetWindowFullscreen(GfuiWindow, SDL_WINDOW_FULLSCREEN);
        } else SDL_SetWindowFullscreen(GfuiWindow, SDL_WINDOW_FULLSCREEN);
    }
    return PScreenSurface;
}

bool GfScrInitSDL2(int nWinWidth, int nWinHeight, int nFullScreen)
{
	// Prepare video mode.
	int bfVideoMode = SDL_WINDOW_OPENGL;

	// Initialize SDL video subsystem (and exit if not supported).
	if (SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
	{
		GfLogError("Couldn't initialize SDL audio/video sub-system (%s)\n", SDL_GetError());
		return false;
	}

	// Get selected frame buffer specs from config file
	// 1) Load the config file
	std::ostringstream ossConfigFilename;
	ossConfigFilename << GfLocalDir() << GFSCR_CONF_FILE;
	void* hparmScreen =
		GfParmReadFile(ossConfigFilename.str().c_str(), GFPARM_RMODE_STD | GFPARM_RMODE_CREAT);

	// 2) Check / update test state of any 'in-test' specs.
	if (GfParmExistsSection(hparmScreen, GFSCR_SECT_INTESTPROPS))
	{
		// Remove the 'in-test' specs if the test failed (we are still in the 'in progress'
		// test state because the game crashed during the test).
		if (std::string(GfParmGetStr(hparmScreen, GFSCR_SECT_INTESTPROPS, GFSCR_ATT_TESTSTATE,
									 GFSCR_VAL_INPROGRESS)) == GFSCR_VAL_INPROGRESS)
		{
			GfLogInfo("Reverting to last validated screen specs, as last test failed.\n");
			GfParmRemoveSection(hparmScreen, GFSCR_SECT_INTESTPROPS);
		}

		// If the test has not yet been done, mark it as 'in progress'
		else
		{
			GfLogInfo("Testing new screen specs : let's see what's happening ...\n");
			GfParmSetStr(hparmScreen, GFSCR_SECT_INTESTPROPS, GFSCR_ATT_TESTSTATE,
						 GFSCR_VAL_INPROGRESS);
		}
		
		// Write the config file to disk (in case the forthcoming test makes the game crash,
		// or in order the Options / Display menu shows the actual current settings).
		GfParmWriteFile(NULL, hparmScreen, "Screen");
	}

	// 3) Select the 'in-test' specs if present, otherwise the 'validated' ones.
	const char* pszScrPropSec =
		GfParmExistsSection(hparmScreen, GFSCR_SECT_INTESTPROPS)
		? GFSCR_SECT_INTESTPROPS : GFSCR_SECT_VALIDPROPS;

	// 4) Get/Read the specs.
	if (nWinWidth < 0)
		nWinWidth =
			(int)GfParmGetNum(hparmScreen, pszScrPropSec, GFSCR_ATT_WIN_X, (char*)NULL, 800);
	if (nWinHeight < 0)
		nWinHeight =
			(int)GfParmGetNum(hparmScreen, pszScrPropSec, GFSCR_ATT_WIN_Y, (char*)NULL, 600);
    int nTotalDepth =
		(int)GfParmGetNum(hparmScreen, pszScrPropSec, GFSCR_ATT_BPP, (char*)NULL, 32);
	bool bAlphaChannel =
		std::string(GfParmGetStr(hparmScreen, pszScrPropSec, GFSCR_ATT_ALPHACHANNEL,
								 GFSCR_VAL_YES))
		== GFSCR_VAL_YES;
	bool bFullScreen;
	if (nFullScreen < 0)
		bFullScreen =
			std::string(GfParmGetStr(hparmScreen, pszScrPropSec, GFSCR_ATT_FSCR, GFSCR_VAL_NO))
			== GFSCR_VAL_YES;
	else
		bFullScreen = nFullScreen ? true : false;

    bool bBumpMap =
		std::string(GfParmGetStr(hparmScreen, pszScrPropSec, GFSCR_ATT_BUMPMAPPING, 
								 GFSCR_VAL_NO))
		== GFSCR_VAL_YES;

	int nAniFilt =
		(int)GfParmGetNum(hparmScreen, pszScrPropSec, GFSCR_ATT_ANISOTROPICFILTERING, (char*)NULL, 0);

	bool bStereo =
		std::string(GfParmGetStr(hparmScreen, pszScrPropSec, GFSCR_ATT_STEREOVISION,
								 GFSCR_VAL_NO))
		== GFSCR_VAL_YES;
    bool bTryBestVInitMode =
		std::string(GfParmGetStr(hparmScreen, pszScrPropSec, GFSCR_ATT_VINIT,
								 GFSCR_VAL_VINIT_BEST))
		== GFSCR_VAL_VINIT_BEST;
	
	if(bFullScreen)
		bfVideoMode |= SDL_WINDOW_FULLSCREEN;

	// TODO ?
	// Add new values to the config OpenGL Major and Minor 
	// and setup GL Major/Minor before window creation
	// SDL_GL_SetSwapInterval(1) for for vsync (may have to go AFTER window creation)

	if (bTryBestVInitMode) 
	{
		GfLogInfo("Trying 'best possible mode' for video initialization.\n");

		// Detect best supported features for the specified frame buffer specs.
		// Warning: Restarts the game if the frame buffer specs changed since last call.
		// If specified and possible, setup the best possible settings.
		if (GfglFeatures::self().checkBestSupport(nWinWidth, nWinHeight, nTotalDepth,
                                                  bAlphaChannel, bFullScreen, bBumpMap, bStereo,nAniFilt,hparmScreen))
		{
			// Load Open GL user settings from the config file.
			GfglFeatures::self().loadSelection();
	
			// Setup the video mode parameters.
			const int nColorDepth =
				GfglFeatures::self().getSelected(GfglFeatures::ColorDepth);
			SDL_GL_SetAttribute(SDL_GL_RED_SIZE, nColorDepth/3);
			SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, nColorDepth/3);
			SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, nColorDepth/3);
			SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, nColorDepth);

			const int nAlphaDepth =
				GfglFeatures::self().getSelected(GfglFeatures::AlphaDepth);
			SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, nAlphaDepth);
			
			const int nDoubleBuffer =
				GfglFeatures::self().isSelected(GfglFeatures::DoubleBuffer) ? 1 : 0;
			SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, nDoubleBuffer);
			
			const int nMultiSampling =
				GfglFeatures::self().isSelected(GfglFeatures::MultiSampling) ? 1 : 0;
			SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, nMultiSampling);
			if (nMultiSampling)
			{
				const int nMaxMultiSamples =
					GfglFeatures::self().getSelected(GfglFeatures::MultiSamplingSamples);
				SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, nMaxMultiSamples);
			}

			const int nStereoVision =
				GfglFeatures::self().isSelected(GfglFeatures::StereoVision) ? 1 : 0;
			SDL_GL_SetAttribute(SDL_GL_STEREO, nStereoVision);

			// Try the video mode with these parameters : should always work
			// (unless you downgraded you hardware / OS and didn't clear your config file).
			PScreenSurface = gfScrCreateWindow(nWinWidth, nWinHeight, nTotalDepth,bfVideoMode);
		}

		// If best mode not supported, or test actually failed,
		// revert to a supported mode (restart the game).
		if (!PScreenSurface)
		{
			GfLogWarning("Failed to setup best supported video mode "
						 "whereas previously detected !\n");
			GfLogWarning("Tip: You should remove your %s%s file and restart,\n",
						 GfLocalDir(), GFSCR_CONF_FILE);
			GfLogWarning("     if something changed in your OS"
						 " or video hardware/driver configuration.\n");

			// If testing new screen specs, remember that the test failed
			// in order to revert to the previous validated specs on restart.
			if (std::string(pszScrPropSec) == GFSCR_SECT_INTESTPROPS)
			{
				GfParmSetStr(hparmScreen, pszScrPropSec, GFSCR_ATT_TESTSTATE,
							 GFSCR_VAL_FAILED);
			}
				
			// Force compatible video init. mode if not testing a new video mode.
			else
			{
				GfLogWarning("Falling back to a more compatible default mode ...\n");
				GfParmSetStr(hparmScreen, pszScrPropSec, GFSCR_ATT_VINIT,
							 GFSCR_VAL_VINIT_COMPATIBLE);
			}
			GfParmWriteFile(NULL, hparmScreen, "Screen");
			GfParmReleaseHandle(hparmScreen);

			// And restart the game.
			GfuiApp().restart(); // Never returns.
		}
	}
	
	// Video initialization with generic compatible settings.
	if (!PScreenSurface)
	{
		GfLogInfo("Trying 'default compatible' mode for video initialization.\n");

		// cancel StereoVision
		SDL_GL_SetAttribute(SDL_GL_STEREO, 0);

		PScreenSurface = gfScrCreateWindow(nWinWidth, nWinHeight, nTotalDepth,bfVideoMode);
		if (!PScreenSurface)
			GfLogTrace("Can't get a %s%dx%dx%d compatible video mode\n",
					   bFullScreen ? "full-screen " : "", nWinWidth, nWinHeight, nTotalDepth);
	}

	// Failed : Try and remove the full-screen requirement if present ...
	if (!PScreenSurface && bFullScreen)
	{
		bfVideoMode &= ~SDL_WINDOW_FULLSCREEN;
		PScreenSurface = gfScrCreateWindow(nWinWidth, nWinHeight, nTotalDepth,bfVideoMode);
		if (!PScreenSurface)
			GfLogTrace("Can't get a non-full-screen %dx%dx%d compatible video mode\n",
					   nWinWidth, nWinHeight, nTotalDepth);
		
		// Update screen specs.
		GfParmSetStr(hparmScreen, pszScrPropSec, GFSCR_ATT_FSCR, GFSCR_VAL_NO);
		GfParmWriteFile(NULL, hparmScreen, "Screen");
	}

	// Failed : Try with a lower fallback size  : should be supported everywhere ...
	if (!PScreenSurface)
	{
		nWinWidth = ADefScreenSizes[0].width;
		nWinHeight = ADefScreenSizes[0].height;
		PScreenSurface = gfScrCreateWindow(nWinWidth, nWinHeight, nTotalDepth,bfVideoMode);
		if (!PScreenSurface)
			GfLogTrace("Can't get a %dx%dx%d compatible video mode\n",
					   nWinWidth, nWinHeight, nTotalDepth);
		
		// Update screen specs.
		GfParmSetNum(hparmScreen, pszScrPropSec, GFSCR_ATT_WIN_X, 0, (tdble)nWinWidth);
		GfParmSetNum(hparmScreen, pszScrPropSec, GFSCR_ATT_WIN_Y, 0, (tdble)nWinHeight);
		GfParmWriteFile(NULL, hparmScreen, "Screen");
	}

	// Failed : Try with a lower fallback color depth : should be supported everywhere ...
	if (!PScreenSurface)
	{
		nTotalDepth = ADefScreenColorDepths[0];
		PScreenSurface = gfScrCreateWindow(nWinWidth, nWinHeight, nTotalDepth,bfVideoMode);
		if (!PScreenSurface)
			GfLogTrace("Can't get a %dx%dx%d compatible video mode\n",
					   nWinWidth, nWinHeight, nTotalDepth);
		
		// Update screen specs.
		GfParmSetNum(hparmScreen, pszScrPropSec, GFSCR_ATT_BPP, 0, (tdble)nTotalDepth);
		GfParmWriteFile(NULL, hparmScreen, "Screen");
	}

	// Close the config file.
	GfParmReleaseHandle(hparmScreen);

	// Failed : No way ... no more ideas !
	if (!PScreenSurface)
	{
		GfLogError("Unable to get any compatible video mode"
				   " (fallback resolution / color depth not supported) : giving up !\n\n");
		return false;
	}

	// If we get here, that's because we succeeded in getting a valid video mode :-)
	
	// If 'compatible mode' selected, detect only standard Open GL features
	// and load OpenGL settings from the config file.
	if (!bTryBestVInitMode) 
	{
		GfglFeatures::self().detectStandardSupport();
		GfglFeatures::self().dumpSupport();
		GfglFeatures::self().loadSelection();
	}

	// Save view geometry and screen center.
	GfViewWidth = nWinWidth;
	GfViewHeight = nWinHeight;
	GfScrCenX = nWinWidth / 2;
	GfScrCenY = nWinHeight / 2;

	// Report about selected SDL video mode.
	GfLogInfo("Selected SDL video mode :\n");
	GfLogInfo("  Full screen : %s\n", (bfVideoMode & SDL_WINDOW_FULLSCREEN) ? "Yes" : "No");
	GfLogInfo("  Size        : %dx%d\n", nWinWidth, nWinHeight);
	GfLogInfo("  Color depth : %d bits\n", nTotalDepth);
	
	// Report about underlying hardware (needs a running frame buffer).
	GfglFeatures::self().dumpHardwareInfo();

	if(GfuiWindow)
	{
		SDL_ShowWindow(GfuiWindow);
		SDL_RestoreWindow(GfuiWindow);
	}
	
#ifdef WIN32
	// Under Windows, give an initial position to the window if not full-screen mode
	// (under Linux/Mac OS X, no need, the window manager smartly takes care of this).
	if (!(bfVideoMode & SDL_WINDOW_FULLSCREEN))
	{
		// Try to center the game Window on the desktop, but keep the title bar visible if any.
		const HWND hDesktop = GetDesktopWindow();
		RECT rectDesktop;
		GetWindowRect(hDesktop, &rectDesktop);
		const int nWMWinXPos =
			nWinWidth >= rectDesktop.right ? 0 : (rectDesktop.right - nWinWidth) / 2;
		const int nWMWinYPos =
			nWinHeight >= rectDesktop.bottom ? 0 : (rectDesktop.bottom - nWinHeight) / 2;
		GfuiInitWindowPositionAndSize(nWMWinXPos, nWMWinYPos, nWinWidth, nWinHeight);
	}
#endif



	// Initialize the Open GL viewport.
	gfScrReshapeViewport(nWinWidth, nWinHeight);

	// Setup the event loop about the new display.
	GfuiApp().eventLoop().setReshapeCB(gfScrReshapeViewport);
	GfuiApp().eventLoop().postRedisplay();
	return true;
}
#else
bool GfScrInitSDL1(int nWinWidth, int nWinHeight, int nFullScreen)
{
    /*if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
	{
       GfLogError("\nUnable to initialize SDL:  (%s)\n", SDL_GetError());
        return false;
    }*/

	// Initialize SDL video subsystem (and exit if not supported).
	if (SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
	{
		GfLogError("Couldn't initialize SDL audio/video sub-system (%s)\n", SDL_GetError());
		return false;
	}
	
	// Enable unicode translation for SDL key press events, even if already done before
	// (SDL_InitSubSystem(SDL_INIT_VIDEO) seems to break it).
	SDL_EnableUNICODE(/*enable=*/1);
	
	// Set window/icon captions
	std::ostringstream ossCaption;
	ossCaption << GfuiApp().name() << ' ' << GfuiApp().version();

	SDL_WM_SetCaption(ossCaption.str().c_str(), ossCaption.str().c_str());

	// Set window icon (MUST be a 32x32 icon for Windows, and with black pixels as alpha ones, 
	// as BMP doesn't support transparency).
	std::ostringstream ossIconFilename;
	ossIconFilename << GfDataDir() << "data/icons/icon.bmp";
	SDL_Surface* surfIcon = SDL_LoadBMP(ossIconFilename.str().c_str());
	if (surfIcon)
	{
	    SDL_SetColorKey(surfIcon, SDL_SRCCOLORKEY, SDL_MapRGB(surfIcon->format, 0, 0, 0));
	    SDL_WM_SetIcon(surfIcon, 0);
	    SDL_FreeSurface(surfIcon);
	}

	// Query system video capabilities.
	// Note: Does not work very well as long as you don't force SDL to use
	//       a special hardware driver ... which we don't want at all (the default is the one).
//typedef struct{
//  Uint32 hw_available:1;
//  Uint32 wm_available:1;
//  Uint32 blit_hw:1;
//  Uint32 blit_hw_CC:1;
//  Uint32 blit_hw_A:1;
//  Uint32 blit_sw:1;
//  Uint32 blit_sw_CC:1;
//  Uint32 blit_sw_A:1;
//  Uint32 blit_fill;
//  Uint32 video_mem;
//  SDL_PixelFormat *vfmt;
//} SDL_VideoInfo;

	const SDL_VideoInfo* sdlVideoInfo = SDL_GetVideoInfo();
	
	if (!sdlVideoInfo)
	{
		GfLogError("Could not SDL_GetVideoInfo (%s)\n", SDL_GetError());
		return false;
	}
	
	char pszDriverName[32];
	GfLogInfo("SDL video backend info :\n");
	GfLogInfo("  Driver                : %s\n", SDL_VideoDriverName(pszDriverName, 32));
	GfLogInfo("  Maximum color depth   : %d bits\n", sdlVideoInfo->vfmt->BitsPerPixel);
	// These ones don't report actually real values on some configurations.
	// GfLogInfo("  Hardware acceleration : %s\n", sdlVideoInfo->hw_available ? "Yes" : "No");
	// GfLogInfo("  Total video memory    : %u Kb\n", sdlVideoInfo->video_mem);

	// Get selected frame buffer specs from config file
	// 1) Load the config file
	std::ostringstream ossConfigFilename;
	ossConfigFilename << GfLocalDir() << GFSCR_CONF_FILE;
	void* hparmScreen =
		GfParmReadFile(ossConfigFilename.str().c_str(), GFPARM_RMODE_STD | GFPARM_RMODE_CREAT);

	// 2) Check / update test state of any 'in-test' specs.
	if (GfParmExistsSection(hparmScreen, GFSCR_SECT_INTESTPROPS))
	{
		// Remove the 'in-test' specs if the test failed (we are still in the 'in progress'
		// test state because the game crashed during the test).
		if (std::string(GfParmGetStr(hparmScreen, GFSCR_SECT_INTESTPROPS, GFSCR_ATT_TESTSTATE,
									 GFSCR_VAL_INPROGRESS)) == GFSCR_VAL_INPROGRESS)
		{
			GfLogInfo("Reverting to last validated screen specs, as last test failed.\n");
			GfParmRemoveSection(hparmScreen, GFSCR_SECT_INTESTPROPS);
		}

		// If the test has not yet been done, mark it as 'in progress'
		else
		{
			GfLogInfo("Testing new screen specs : let's see what's happening ...\n");
			GfParmSetStr(hparmScreen, GFSCR_SECT_INTESTPROPS, GFSCR_ATT_TESTSTATE,
						 GFSCR_VAL_INPROGRESS);
		}
		
		// Write the config file to disk (in case the forthcoming test makes the game crash,
		// or in order the Options / Display menu shows the actual current settings).
		GfParmWriteFile(NULL, hparmScreen, "Screen");
	}

	// 3) Select the 'in-test' specs if present, otherwise the 'validated' ones.
	const char* pszScrPropSec =
		GfParmExistsSection(hparmScreen, GFSCR_SECT_INTESTPROPS)
		? GFSCR_SECT_INTESTPROPS : GFSCR_SECT_VALIDPROPS;

	// 4) Get/Read the specs.
	if (nWinWidth < 0)
		nWinWidth =
			(int)GfParmGetNum(hparmScreen, pszScrPropSec, GFSCR_ATT_WIN_X, (char*)NULL, 800);
	if (nWinHeight < 0)
		nWinHeight =
			(int)GfParmGetNum(hparmScreen, pszScrPropSec, GFSCR_ATT_WIN_Y, (char*)NULL, 600);
    int nTotalDepth =
		(int)GfParmGetNum(hparmScreen, pszScrPropSec, GFSCR_ATT_BPP, (char*)NULL, 32);
    bool bAlphaChannel =
		std::string(GfParmGetStr(hparmScreen, pszScrPropSec, GFSCR_ATT_ALPHACHANNEL,
								 GFSCR_VAL_YES))
		== GFSCR_VAL_YES;
	bool bFullScreen;
	if (nFullScreen < 0)
		bFullScreen =
			std::string(GfParmGetStr(hparmScreen, pszScrPropSec, GFSCR_ATT_FSCR, GFSCR_VAL_NO))
			== GFSCR_VAL_YES;
	else
		bFullScreen = nFullScreen ? true : false;

    bool bBumpMap =
		std::string(GfParmGetStr(hparmScreen, pszScrPropSec, GFSCR_ATT_BUMPMAPPING, 
								 GFSCR_VAL_NO))
		== GFSCR_VAL_YES;

    int nAniFilt =
        (int)GfParmGetNum(hparmScreen, pszScrPropSec, GFSCR_ATT_ANISOTROPICFILTERING, (char*)NULL, 0);



    bool bStereo =
		std::string(GfParmGetStr(hparmScreen, pszScrPropSec, GFSCR_ATT_STEREOVISION,
								 GFSCR_VAL_NO))
		== GFSCR_VAL_YES;
    bool bTryBestVInitMode =
		std::string(GfParmGetStr(hparmScreen, pszScrPropSec, GFSCR_ATT_VINIT,
								 GFSCR_VAL_VINIT_BEST))
		== GFSCR_VAL_VINIT_BEST;

	// Prepare video mode.
	int bfVideoMode = SDL_OPENGL;
	if (bFullScreen)
		bfVideoMode |= SDL_FULLSCREEN;

	PScreenSurface = NULL;
	if (bTryBestVInitMode) 
	{
		GfLogInfo("Trying 'best possible mode' for video initialization.\n");

		// Detect best supported features for the specified frame buffer specs.
		// Warning: Restarts the game if the frame buffer specs changed since last call.
		// If specified and possible, setup the best possible settings.
		if (GfglFeatures::self().checkBestSupport(nWinWidth, nWinHeight, nTotalDepth,
                                                  bAlphaChannel, bFullScreen, bBumpMap, bStereo,nAniFilt,hparmScreen))
		{
			// Load Open GL user settings from the config file.
			GfglFeatures::self().loadSelection();
	
			// Setup the video mode parameters.
			const int nColorDepth =
				GfglFeatures::self().getSelected(GfglFeatures::ColorDepth);
			SDL_GL_SetAttribute(SDL_GL_RED_SIZE, nColorDepth/3);
			SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, nColorDepth/3);
			SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, nColorDepth/3);
			SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, nColorDepth);

			const int nAlphaDepth =
				GfglFeatures::self().getSelected(GfglFeatures::AlphaDepth);
			SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, nAlphaDepth);
			
			const int nDoubleBuffer =
				GfglFeatures::self().isSelected(GfglFeatures::DoubleBuffer) ? 1 : 0;
			SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, nDoubleBuffer);
			
			const int nMultiSampling =
				GfglFeatures::self().isSelected(GfglFeatures::MultiSampling) ? 1 : 0;
			SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, nMultiSampling);
			if (nMultiSampling)
			{
				const int nMaxMultiSamples =
					GfglFeatures::self().getSelected(GfglFeatures::MultiSamplingSamples);
				SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, nMaxMultiSamples);
			}

			const int nStereoVision =
				GfglFeatures::self().isSelected(GfglFeatures::StereoVision) ? 1 : 0;
			SDL_GL_SetAttribute(SDL_GL_STEREO, nStereoVision);
			
			// Try the video mode with these parameters : should always work
			// (unless you downgraded you hardware / OS and didn't clear your config file).
			PScreenSurface = SDL_SetVideoMode(nWinWidth, nWinHeight, nTotalDepth, bfVideoMode);
		}

		// If best mode not supported, or test actually failed,
		// revert to a supported mode (restart the game).
		if (!PScreenSurface)
		{
			GfLogWarning("Failed to setup best supported video mode "
						 "whereas previously detected !\n");
			GfLogWarning("Tip: You should remove your %s%s file and restart,\n",
						 GfLocalDir(), GFSCR_CONF_FILE);
			GfLogWarning("     if something changed in your OS"
						 " or video hardware/driver configuration.\n");

			// If testing new screen specs, remember that the test failed
			// in order to revert to the previous validated specs on restart.
			if (std::string(pszScrPropSec) == GFSCR_SECT_INTESTPROPS)
			{
				GfParmSetStr(hparmScreen, pszScrPropSec, GFSCR_ATT_TESTSTATE,
							 GFSCR_VAL_FAILED);
			}
				
			// Force compatible video init. mode if not testing a new video mode.
			else
			{
				GfLogWarning("Falling back to a more compatible default mode ...\n");
				GfParmSetStr(hparmScreen, pszScrPropSec, GFSCR_ATT_VINIT,
							 GFSCR_VAL_VINIT_COMPATIBLE);
			}
			GfParmWriteFile(NULL, hparmScreen, "Screen");
			GfParmReleaseHandle(hparmScreen);

			// And restart the game.
			GfuiApp().restart(); // Never returns.
		}
	}
	
	// Video initialization with generic compatible settings.
	if (!PScreenSurface)
	{
		GfLogInfo("Trying 'default compatible' mode for video initialization.\n");

		// cancel StereoVision
		SDL_GL_SetAttribute(SDL_GL_STEREO, 0);

		PScreenSurface = SDL_SetVideoMode(nWinWidth, nWinHeight, nTotalDepth, bfVideoMode);
		if (!PScreenSurface)
			GfLogTrace("Can't get a %s%dx%dx%d compatible video mode\n",
					   bFullScreen ? "full-screen " : "", nWinWidth, nWinHeight, nTotalDepth);
	}

	// Failed : Try and remove the full-screen requirement if present ...
	if (!PScreenSurface && bFullScreen)
	{
		bfVideoMode &= ~SDL_FULLSCREEN;
		PScreenSurface = SDL_SetVideoMode(nWinWidth, nWinHeight, nTotalDepth, bfVideoMode);
		if (!PScreenSurface)
			GfLogTrace("Can't get a non-full-screen %dx%dx%d compatible video mode\n",
					   nWinWidth, nWinHeight, nTotalDepth);
		
		// Update screen specs.
		GfParmSetStr(hparmScreen, pszScrPropSec, GFSCR_ATT_FSCR, GFSCR_VAL_NO);
		GfParmWriteFile(NULL, hparmScreen, "Screen");
	}

	// Failed : Try with a lower fallback size  : should be supported everywhere ...
	if (!PScreenSurface)
	{
		nWinWidth = ADefScreenSizes[0].width;
		nWinHeight = ADefScreenSizes[0].height;
		PScreenSurface = SDL_SetVideoMode(nWinWidth, nWinHeight, nTotalDepth, bfVideoMode);
		if (!PScreenSurface)
			GfLogTrace("Can't get a %dx%dx%d compatible video mode\n",
					   nWinWidth, nWinHeight, nTotalDepth);
		
		// Update screen specs.
		GfParmSetNum(hparmScreen, pszScrPropSec, GFSCR_ATT_WIN_X, 0, (tdble)nWinWidth);
		GfParmSetNum(hparmScreen, pszScrPropSec, GFSCR_ATT_WIN_Y, 0, (tdble)nWinHeight);
		GfParmWriteFile(NULL, hparmScreen, "Screen");
	}

	// Failed : Try with a lower fallback color depth : should be supported everywhere ...
	if (!PScreenSurface)
	{
		nTotalDepth = ADefScreenColorDepths[0];
		PScreenSurface = SDL_SetVideoMode(nWinWidth, nWinHeight, nTotalDepth, bfVideoMode);
		if (!PScreenSurface)
			GfLogTrace("Can't get a %dx%dx%d compatible video mode\n",
					   nWinWidth, nWinHeight, nTotalDepth);
		
		// Update screen specs.
		GfParmSetNum(hparmScreen, pszScrPropSec, GFSCR_ATT_BPP, 0, (tdble)nTotalDepth);
		GfParmWriteFile(NULL, hparmScreen, "Screen");
	}

	// Close the config file.
	GfParmReleaseHandle(hparmScreen);

	// Failed : No way ... no more ideas !
	if (!PScreenSurface)
	{
		GfLogError("Unable to get any compatible video mode"
				   " (fallback resolution / color depth not supported) : giving up !\n\n");
		return false;
	}

	// If we get here, that's because we succeeded in getting a valid video mode :-)
	
	// If 'compatible mode' selected, detect only standard Open GL features
	// and load OpenGL settings from the config file.
	if (!bTryBestVInitMode) 
	{
		GfglFeatures::self().detectStandardSupport();
		GfglFeatures::self().dumpSupport();
		GfglFeatures::self().loadSelection();
	}

	// Save view geometry and screen center.
    GfViewWidth = nWinWidth;
    GfViewHeight = nWinHeight;
    GfScrCenX = nWinWidth / 2;
    GfScrCenY = nWinHeight / 2;

	// Report about selected SDL video mode.
	GfLogInfo("Selected SDL video mode :\n");
 	GfLogInfo("  Full screen : %s\n", (bfVideoMode & SDL_FULLSCREEN) ? "Yes" : "No");
 	GfLogInfo("  Size        : %dx%d\n", nWinWidth, nWinHeight);
 	GfLogInfo("  Color depth : %d bits\n", nTotalDepth);
	
	// Report about underlying hardware (needs a running frame buffer).
	GfglFeatures::self().dumpHardwareInfo();
	
#ifdef WIN32
	// Under Windows, give an initial position to the window if not full-screen mode
	// (under Linux/Mac OS X, no need, the window manager smartly takes care of this).
	if (!(bfVideoMode & SDL_FULLSCREEN))
	{
		// Try to center the game Window on the desktop, but keep the title bar visible if any.
		const HWND hDesktop = GetDesktopWindow();
		RECT rectDesktop;
		GetWindowRect(hDesktop, &rectDesktop);
		const int nWMWinXPos =
			nWinWidth >= rectDesktop.right ? 0 : (rectDesktop.right - nWinWidth) / 2;
		const int nWMWinYPos =
			nWinHeight >= rectDesktop.bottom ? 0 : (rectDesktop.bottom - nWinHeight) / 2;
		GfuiInitWindowPositionAndSize(nWMWinXPos, nWMWinYPos, nWinWidth, nWinHeight);
	}
#endif

	// Initialize the Open GL viewport.
	gfScrReshapeViewport(nWinWidth, nWinHeight);

	// Setup the event loop about the new display.
	GfuiApp().eventLoop().setReshapeCB(gfScrReshapeViewport);
	GfuiApp().eventLoop().postRedisplay();
	return true;
}
#endif
bool GfScrInit(int nWinWidth, int nWinHeight, int nFullScreen)
{
#if SDL_MAJOR_VERSION >= 2
    return GfScrInitSDL2(nWinWidth,nWinHeight,nFullScreen);
#else
    return GfScrInitSDL1(nWinWidth,nWinHeight,nFullScreen);
#endif
}

/** Shutdown the screen
    @ingroup	screen
    @return	none
*/
void GfScrShutdown(void)
{
	GfLogTrace("Shutting down screen.\n");

	// Shutdown SDL video sub-system.
	SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

	// If there's an 'in-test' screen properties section in the config file,
	// * if the test state is 'to do', do nothing (will be taken care of in next GfScrInit),
	// * if the test state is 'in progress', validate the new screen properties,
	// * if the test state is 'failed', revert to the validated screen properties.
	std::ostringstream ossConfigFilename;
	ossConfigFilename << GfLocalDir() << GFSCR_CONF_FILE;
	void* hparmScreen = GfParmReadFile(ossConfigFilename.str().c_str(), GFPARM_RMODE_STD);

	if (GfParmExistsSection(hparmScreen, GFSCR_SECT_INTESTPROPS))
	{
		if (std::string(GfParmGetStr(hparmScreen, GFSCR_SECT_INTESTPROPS, GFSCR_ATT_TESTSTATE,
									 GFSCR_VAL_INPROGRESS)) == GFSCR_VAL_INPROGRESS)
		{
			GfLogInfo("Validating new screen specs (test was successful).\n");
		
			// Copy the 'in test' props to the 'validated' ones.
			GfParmSetNum(hparmScreen, GFSCR_SECT_VALIDPROPS, GFSCR_ATT_WIN_X, 0,
						 GfParmGetNum(hparmScreen, GFSCR_SECT_INTESTPROPS, GFSCR_ATT_WIN_X, 0, 800));
			GfParmSetNum(hparmScreen, GFSCR_SECT_VALIDPROPS, GFSCR_ATT_WIN_Y, 0,
						 GfParmGetNum(hparmScreen, GFSCR_SECT_INTESTPROPS, GFSCR_ATT_WIN_Y, 0, 600));
			GfParmSetNum(hparmScreen, GFSCR_SECT_VALIDPROPS, GFSCR_ATT_BPP, 0,
						 GfParmGetNum(hparmScreen, GFSCR_SECT_INTESTPROPS, GFSCR_ATT_BPP, 0, 32));
			// GfParmSetNum(hparmScreen, GFSCR_SECT_VALIDPROPS, GFSCR_ATT_MAXREFRESH, 0,
			// 			 GfParmGetNum(hparmScreen, GFSCR_SECT_INTESTPROPS, GFSCR_ATT_MAXREFRESH, 0, 0));
			GfParmSetStr(hparmScreen, GFSCR_SECT_VALIDPROPS, GFSCR_ATT_VDETECT,
						 GfParmGetStr(hparmScreen, GFSCR_SECT_INTESTPROPS, GFSCR_ATT_VDETECT, GFSCR_VAL_VDETECT_AUTO));
			const  char* pszVInitMode =
				GfParmGetStr(hparmScreen, GFSCR_SECT_INTESTPROPS, GFSCR_ATT_VINIT,
							 GFSCR_VAL_VINIT_COMPATIBLE);
			GfParmSetStr(hparmScreen, GFSCR_SECT_VALIDPROPS, GFSCR_ATT_VINIT, pszVInitMode);
			GfParmSetStr(hparmScreen, GFSCR_SECT_VALIDPROPS, GFSCR_ATT_FSCR,
						 GfParmGetStr(hparmScreen, GFSCR_SECT_INTESTPROPS, GFSCR_ATT_FSCR, GFSCR_VAL_NO));
			// Store OpenGL settings if best video init mode selected
			// (because loadSelection can changed them).
			if (std::string(pszVInitMode) == GFSCR_VAL_VINIT_BEST)
				GfglFeatures::self().storeSelection(hparmScreen);
		}
		else if (std::string(GfParmGetStr(hparmScreen, GFSCR_SECT_INTESTPROPS, GFSCR_ATT_TESTSTATE,
										  GFSCR_VAL_INPROGRESS)) == GFSCR_VAL_FAILED)
		{
			GfLogInfo("Canceling new screen specs, back to old ones (test failed).\n");
		}


		if (std::string(GfParmGetStr(hparmScreen, GFSCR_SECT_INTESTPROPS, GFSCR_ATT_TESTSTATE,
									 GFSCR_VAL_INPROGRESS)) != GFSCR_VAL_TODO)
		{
			// Remove the 'in-test' section.
			GfParmRemoveSection(hparmScreen, GFSCR_SECT_INTESTPROPS);
		
			// Write the screen config file to disk.
			GfParmWriteFile(NULL, hparmScreen, "Screen");
		}
		else
		{
			GfLogInfo("New screen specs will be tested when restarting.\n");
		}
	}
	
	// Release screen config params file.
	GfParmReleaseHandle(hparmScreen);
}


/** Get the screen and viewport sizes.
    @ingroup	screen
    @param	scrw	address of screen with
    @param	scrh	address of screen height
    @param	vieww	address of viewport with
    @param	viewh	address of viewport height
    @return	none
 */
void GfScrGetSize(int *scrW, int *scrH, int *viewW, int *viewH)
{
    *scrW = GfScrWidth;
    *scrH = GfScrHeight;
    *viewW = GfViewWidth;
    *viewH = GfViewHeight;
}

bool GfScrToggleFullScreen()
{
#if SDL_MAJOR_VERSION >= 2
	Uint32 flags = SDL_GetWindowFlags(GfuiWindow);

	if ((flags & SDL_WINDOW_FULLSCREEN) || (flags & SDL_WINDOW_FULLSCREEN_DESKTOP)) {
		SDL_SetWindowFullscreen(GfuiWindow, 0);
		return FALSE;
	} else {
		SDL_Rect bounds;

		/* Work around SDL2 bug */
		if (SDL_GetDisplayBounds(0, &bounds) == 0) {
			if (SDL_FALSE) //(bounds.w == nWinWidth && bounds.h == nWinHeight)
				SDL_SetWindowFullscreen(GfuiWindow, SDL_WINDOW_FULLSCREEN_DESKTOP);
			else SDL_SetWindowFullscreen(GfuiWindow, SDL_WINDOW_FULLSCREEN);
		} else SDL_SetWindowFullscreen(GfuiWindow, SDL_WINDOW_FULLSCREEN);

		return TRUE;
	}
#else
	return SDL_WM_ToggleFullScreen(PScreenSurface) != 0;
#endif
}

/** Capture screen pixels into an RGB buffer (caller must free the here-allocated buffer).
    @ingroup	screen
    @param	scrw	address of screen with
    @param	scrh	address of screen height
    @return	none
 */
unsigned char* GfScrCapture(int* viewW, int *viewH)
{
    unsigned char *img;
    int sW, sH;
	
    GfScrGetSize(&sW, &sH, viewW, viewH);
    img = (unsigned char*)malloc((*viewW) * (*viewH) * 3);
    if (img)
	{
		glPixelStorei(GL_PACK_ROW_LENGTH, 0);
		glPixelStorei(GL_PACK_ALIGNMENT, 1);
		glReadBuffer(GL_FRONT);
		glReadPixels((sW-(*viewW))/2, (sH-(*viewH))/2, *viewW, *viewH,
					 GL_RGB, GL_UNSIGNED_BYTE, (GLvoid*)img);
    }

	return img;
}

/** Capture screen pixels into a PNG file
    @ingroup	screen
    @param	filename	filename of the png file
    @return	0 Ok
		<br>-1 Error
 */
int GfScrCaptureAsPNG(const char *filename)
{
	int viewW, viewH;

	// Capture screen to an RGB image (in memory) (and measure elapsed time).
	const double dCaptureBeginTime = GfTimeClock();

	unsigned char* img = GfScrCapture(&viewW, &viewH);

	const double dCaptureEndTime = GfTimeClock();

	const double dCaptureDuration = dCaptureEndTime - dCaptureBeginTime;
	
	// Write RGB image to the PNG file (and measure elapsed time).
	const int nStatus = GfTexWriteImageToPNG(img, filename, viewW, viewH);

	const double dFileWriteDuration = GfTimeClock() - dCaptureEndTime;

	// Free the image buffer.
	if (img)
		free(img);

	if (!nStatus)
		GfLogTrace("Captured screen to %s (capture=%.3f s, PNG=%.3f s)\n",
				   filename, dCaptureDuration, dFileWriteDuration);
	else
		GfLogError("Failed to capture screen to %s\n", filename);

	return nStatus;
}

#if SDL_MAJOR_VERSION >= 2
SDL_Window* GfScrGetMainWindow()
{
    return GfuiWindow;
}
#endif
