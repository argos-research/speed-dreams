/***************************************************************************

    file                 : glfeatures.cpp
    created              : Wed Jun 1 14:56:31 CET 2005
    copyright            : (C) 2005 by Bernhard Wymann
    version              : $Id: glfeatures.cpp 6296 2015-12-01 15:53:13Z beaglejoe $

***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <sstream>
#include <limits>

#include <SDL.h>
#include <SDL_opengl.h>

#include "glfeatures.h"

// Avoid C lib <cstdlib> "max" to overload <limits> ones.
#undef min


static const char* pszNoUnit = 0;


/** Report if a given OpenGL extension is supported

    Warning: Should not be called before any successfull call to SDL_SetVideoMode()

    Note: Copied from freeGLUT 2.4.0
*/
#if SDL_MAJOR_VERSION < 2
static bool gfglIsOpenGLExtensionSupported(const char* extension)
{
  const char *extensions, *start;
  const int len = strlen(extension);

  // TODO: Make sure there is a current window, and thus a current context available

  if (strchr(extension, ' '))
    return false;

  start = extensions = (const char *)glGetString(GL_EXTENSIONS);

  if (!extensions)
	return false;

  while (true)
  {
     const char *p = strstr(extensions, extension);
     if (!p)
		 return 0;  // Not found
	 
     // Check that the match isn't a super string
     if ((p == start || p[-1] == ' ') && (p[len] == ' ' || p[len] == 0))
        return true;
	 
     // Skip the false match and continue
     extensions = p + len;
  }

  return false;
}
#endif

// GfglFeatures singleton --------------------------------------------------------

// Initialization.
GfglFeatures* GfglFeatures::_pSelf = 0;

GfglFeatures::GfglFeatures()
//: hparmConfig(0)
{
}

GfglFeatures& GfglFeatures::self()
{
	if (!_pSelf)
		_pSelf = new GfglFeatures;

	return *_pSelf;
}

// Values for the "not supported" / "not selected" numerical cases.
int GfglFeatures::InvalidInt = std::numeric_limits<int>::min();

// Config file management.
void* GfglFeatures::openConfigFile()
{
	std::ostringstream ossParm;
	ossParm << GfLocalDir() << GFSCR_CONF_FILE;

	return GfParmReadFile(ossParm.str().c_str(), GFPARM_RMODE_STD | GFPARM_RMODE_CREAT);
}

void GfglFeatures::closeConfigFile(void* hparmConfig, bool bWrite)
{
	// Write if specified.
	if (bWrite)
		GfParmWriteFile(NULL, hparmConfig, "Screen");
	
	// Close.
	GfParmReleaseHandle(hparmConfig);
}

// Standard supported features detection.
void GfglFeatures::detectStandardSupport()
{
	// 1) Double-buffer.
	int nValue;
	SDL_GL_GetAttribute(SDL_GL_DOUBLEBUFFER, &nValue);
	_mapSupportedBool[DoubleBuffer] = nValue ? true : false;

	// 2) Color buffer depth.
	SDL_GL_GetAttribute(SDL_GL_DEPTH_SIZE, &nValue);
	//glGetIntegerv(GL_DEPTH_BITS, &nValue);
	_mapSupportedInt[ColorDepth] = nValue;

	// 3) Alpha channel depth.
	SDL_GL_GetAttribute(SDL_GL_ALPHA_SIZE, &nValue);
	//glGetIntegerv(GL_ALPHA_BITS, &nValue);
	_mapSupportedInt[AlphaDepth] = nValue;

	// 4) Max texture size.
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &nValue);
 	if (nValue > 16384) // Max in-game supported value (must be consistent with openglconfig.cpp)
 		nValue = 16384;
	_mapSupportedInt[TextureMaxSize] = nValue;

	// 5) Texture compression.
	//    Note: Check if at least one internal format is available. This is a workaround for
	//    driver problems and not a bugfix. According to the specification OpenGL should
	//    choose an uncompressed alternate format if it can't provide the requested
	//    compressed one... but it does not on all cards/drivers.
#if SDL_MAJOR_VERSION >= 2
    bool bValue = SDL_GL_ExtensionSupported("GL_ARB_texture_compression");
#else
	bool bValue = gfglIsOpenGLExtensionSupported("GL_ARB_texture_compression");
#endif
	if (bValue) 
	{
		int nFormats;
		glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS_ARB, &nFormats);
		if (nFormats == 0) 
			bValue = false;
	}
	_mapSupportedBool[TextureCompression] = bValue;

	// 6) Multi-texturing (automatically select all the texturing units).
#if SDL_MAJOR_VERSION >= 2
    bValue = SDL_GL_ExtensionSupported("GL_ARB_multitexture");
#else
	bValue = gfglIsOpenGLExtensionSupported("GL_ARB_multitexture");
#endif
	glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &nValue);
	if (nValue < 2)
		bValue = false;

	_mapSupportedBool[MultiTexturing] = bValue;

	if (bValue)
		_mapSupportedInt[MultiTexturingUnits] = nValue;

	// 7) Rectangle textures.
#if SDL_MAJOR_VERSION >= 2
    _mapSupportedBool[TextureRectangle] =
            SDL_GL_ExtensionSupported("GL_ARB_texture_rectangle");
#else
	_mapSupportedBool[TextureRectangle] =
		gfglIsOpenGLExtensionSupported("GL_ARB_texture_rectangle");
#endif

	// 8) Non-power-of-2 textures.
#if SDL_MAJOR_VERSION >= 2
    _mapSupportedBool[TextureNonPowerOf2] =
            SDL_GL_ExtensionSupported("GL_ARB_texture_non_power_of_two");
#else
	_mapSupportedBool[TextureNonPowerOf2] =
		gfglIsOpenGLExtensionSupported("GL_ARB_texture_non_power_of_two");
#endif

	// 9) Stereo Vision (need a proper check)
	_mapSupportedBool[StereoVision] = false;

	// 10) Bump Mapping 
#if SDL_MAJOR_VERSION >= 2
    bValue = SDL_GL_ExtensionSupported("GL_ARB_multitexture")
            && SDL_GL_ExtensionSupported("GL_ARB_texture_cube_map")
            && SDL_GL_ExtensionSupported("GL_ARB_texture_env_combine")
            && SDL_GL_ExtensionSupported("GL_ARB_texture_env_dot3")
            && SDL_GL_ExtensionSupported("GL_ARB_imaging");
#else
	bValue = 
		gfglIsOpenGLExtensionSupported("GL_ARB_multitexture")
		&& gfglIsOpenGLExtensionSupported("GL_ARB_texture_cube_map")
		&& gfglIsOpenGLExtensionSupported("GL_ARB_texture_env_combine")
		&& gfglIsOpenGLExtensionSupported("GL_ARB_texture_env_dot3")
		&& gfglIsOpenGLExtensionSupported("GL_ARB_imaging");
#endif
	
	_mapSupportedBool[BumpMapping] = bValue;


    // 10) Anisotropic filtrering
#if SDL_MAJOR_VERSION >= 2
    bValue = SDL_GL_ExtensionSupported("GL_EXT_texture_filter_anisotropic");
#else
    bValue = gfglIsOpenGLExtensionSupported("GL_EXT_texture_filter_anisotropic");
#endif

    _mapSupportedInt[AnisotropicFiltering] = bValue?2:InvalidInt;

    // 11) MultiSampling
#if SDL_MAJOR_VERSION >= 2
    bValue = SDL_GL_ExtensionSupported("GL_ARB_multisample");
#else
    bValue = gfglIsOpenGLExtensionSupported("GL_ARB_multisample");
#endif
    _mapSupportedBool[MultiSampling] = bValue;
    _mapSupportedInt[MultiSamplingSamples] = bValue?8:InvalidInt; // Good but reasonable value. (8)

}
#if SDL_MAJOR_VERSION >= 2
// Best supported features detection for the given specs of the frame buffer.
bool GfglFeatures::detectBestSupportSDL2(int& nWidth, int& nHeight, int& nDepth,
                                     bool& bAlpha, bool& bFullScreen, bool& bBumpMapping, bool& bStereoVision, int &nAniFilt)
{
	GfLogInfo("Detecting best SDL2 supported features for a %dx%dx%d%s frame buffer.\n",
			  nWidth, nHeight, nDepth, bFullScreen ? " full-screen" : "");

	// I) Detection of the max possible values for requested features.
	//    (to do that, we need to try setting up the video modes for real).
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	SDL_Window* testWindow = NULL;
	SDL_Renderer* renderer = NULL;
	SDL_Surface* pWinSurface = 0;

	int nAlphaChannel = bAlpha ? 1 : 0;
	int nCurrDepth = nDepth;
	int nFullScreen = bFullScreen ? 1 : 0;
	int nStereoVision = bStereoVision ? 1 : 0;


	while (!pWinSurface && nFullScreen >= 0)
	{
		GfLogTrace("Trying %s mode\n", nFullScreen ? "full-screen" : "windowed");

		const int bfVideoMode = SDL_WINDOW_OPENGL | (nFullScreen ? SDL_WINDOW_FULLSCREEN : 0);
		SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);

		nAlphaChannel = bAlpha ? 1 : 0;
		while (!pWinSurface && nAlphaChannel >= 0)
		{
			GfLogTrace("Trying with%s alpha channel\n", nAlphaChannel ? "" : "out");
			nCurrDepth = nDepth;
			while (!pWinSurface && nCurrDepth >= 16)
			{
				GfLogTrace("Trying %d bits RVB+A color depth\n", nCurrDepth);
				SDL_GL_SetAttribute(SDL_GL_RED_SIZE, nCurrDepth/4);
				SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, nCurrDepth/4);
				SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, nCurrDepth/4);
				SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, (3*nCurrDepth)/4);
				SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, nAlphaChannel ? nCurrDepth/4 : 0);

				while (!pWinSurface && nStereoVision >= 0)
				{
					GfLogTrace("Trying with%s stereo vision\n", nStereoVision ? "" : "out");
					if (nStereoVision)
						SDL_GL_SetAttribute(SDL_GL_STEREO, GL_TRUE);
					else
						SDL_GL_SetAttribute(SDL_GL_STEREO, GL_FALSE);
		
					// Anti-aliasing : detect the max supported number of samples
					// (assumed to be <= 32).
					int nMaxMultiSamples = 32; // Hard coded max value for the moment.
					while (!pWinSurface && nMaxMultiSamples > 1)
					{
						// Set the anti-aliasing attributes and setup the video mode.
						GfLogTrace("Trying %dx anti-aliasing\n", nMaxMultiSamples);
						SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
						SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, nMaxMultiSamples);
						
						testWindow = SDL_CreateWindow("SDL2 test",
							SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
							nWidth, nHeight, SDL_WINDOW_HIDDEN | SDL_WINDOW_OPENGL);
						//SDL_SetWindowSize(GfuiWindow, nWidth, nHeight);
						if(testWindow)
						{
							renderer = SDL_CreateRenderer(testWindow, -1, 0);
							SDL_RenderPresent(renderer);
							if(renderer)
							{

								SDL_GLContext context = 0;
								context = SDL_GL_CreateContext(testWindow);
								if(context)
								{

									pWinSurface = SDL_CreateRGBSurface(0, nWidth, nHeight, nCurrDepth,
#if SDL_BYTEORDER == SDL_LIL_ENDIAN 
										0x00FF0000, 0x0000FF00, 0x000000FF,
#else 
										0x000000FF, 0x0000FF00, 0x00FF0000,
#endif 
										0x00000000);

									// Now check if we have a video mode, and if it actually features
									// what we specified.
									int nActualSampleBuffers = 0;
									int nActualMultiSamples = 0;
									if (pWinSurface)
									{
										SDL_GL_GetAttribute(SDL_GL_MULTISAMPLEBUFFERS, &nActualSampleBuffers);
										SDL_GL_GetAttribute(SDL_GL_MULTISAMPLESAMPLES, &nActualMultiSamples);
									}
									GfLogDebug("nMaxMultiSamples=%d : nActualSampleBuffers=%d, nActualMultiSamples=%d\n",
										nMaxMultiSamples, nActualSampleBuffers, nActualMultiSamples);

									// If not, try a lower number of samples.
									if (nActualSampleBuffers == 0 || nActualMultiSamples != nMaxMultiSamples)
									{
										SDL_FreeSurface(pWinSurface);
										pWinSurface = 0;
									}
									SDL_GL_DeleteContext(context);
									context = NULL;
								}
								SDL_DestroyRenderer(renderer);
								renderer = NULL;
							}
							SDL_DestroyWindow(testWindow);
							testWindow = NULL;
						}
						if (!pWinSurface)
						{
							GfLogTrace("%d+%d bit %dx anti-aliased double-buffer not supported\n",
									   3*nCurrDepth/4, nCurrDepth/4, nMaxMultiSamples);
							nMaxMultiSamples /= 2;
						}
					}
	
					// Failed : try without anti-aliasing.
					if (!pWinSurface)
					{
						SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
						SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
						testWindow = SDL_CreateWindow("SDL2 test",
							SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
							nWidth, nHeight, SDL_WINDOW_HIDDEN | SDL_WINDOW_OPENGL);
						if(testWindow)
						{
							//SDL_SetWindowSize(GfuiWindow, nWidth, nHeight);

							renderer = SDL_CreateRenderer(testWindow, -1, 0);
							if(renderer)
							{
								SDL_RenderPresent(renderer);

								SDL_GLContext context;
								context = SDL_GL_CreateContext(testWindow);
								if(context)
								{

									pWinSurface = SDL_CreateRGBSurface(0, nWidth, nHeight, nCurrDepth,
#if SDL_BYTEORDER == SDL_LIL_ENDIAN 
										0x00FF0000, 0x0000FF00, 0x000000FF,
#else 
										0x000000FF, 0x0000FF00, 0x00FF0000,
#endif
										0x00000000);

									SDL_GL_DeleteContext(context);
									context = NULL;
								}
								SDL_DestroyRenderer(renderer);
								renderer = NULL;
							}
							SDL_DestroyWindow(testWindow);
							testWindow = NULL;
						}
						if (!pWinSurface)
							GfLogTrace("%d+%d bit double-buffer not supported\n",
							3*nCurrDepth/4, nCurrDepth/4);
					}
	
					// Failed : try without StereoVision
					if (!pWinSurface)
						nStereoVision--;
				}

				// Failed : try with lower color depth.
				if (!pWinSurface)
					nCurrDepth -= 8;
			}

			// Failed : try without alpha channel if not already done
			// (Note: it this really relevant ?).
			if (!pWinSurface)
				nAlphaChannel--;
		}

		// Failed : try a windowed mode if not already done.
		if (!pWinSurface)
			nFullScreen--;
	}

	// Failed : no more idea :-(
	if (!pWinSurface)
	{
		// Reset support data (will result in emptying the section when storing,
		// thus forcing new detection when checkSupport will be called again).
		_mapSupportedBool.clear();
		_mapSupportedInt.clear();
		
		GfLogError("No supported 'best' video mode found for a %dx%dx%d%s frame buffer.\n",
				   nWidth, nHeight, nDepth, bFullScreen ? " full-screen" : "");
		
		return false;
	}
	
	testWindow = SDL_CreateWindow("SDL2 test",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		nWidth, nHeight, SDL_WINDOW_HIDDEN | SDL_WINDOW_OPENGL);
	if(testWindow)
	{
		renderer = SDL_CreateRenderer(testWindow, -1, 0);
		if(renderer)
		{
			SDL_RenderPresent(renderer);

			SDL_GLContext context;
			context = SDL_GL_CreateContext(testWindow);
			if(context)
			{
				// II) Read-out what we have from the up-and-running frame buffer
				//     and set "supported" values accordingly.

				// 1) Standard features.
				detectStandardSupport();

				// 2) Multi-sampling = anti-aliasing
				int nValue;
				SDL_GL_GetAttribute(SDL_GL_MULTISAMPLEBUFFERS, &nValue);
				_mapSupportedBool[MultiSampling] = nValue != 0;
				//GfLogDebug("SDL_GL_GetAttribute(SDL_GL_MULTISAMPLEBUFFERS) = %d\n", nValue);
				if (nValue)
				{
					SDL_GL_GetAttribute(SDL_GL_MULTISAMPLESAMPLES, &nValue);
					//GfLogDebug("SDL_GL_GetAttribute(SDL_GL_MULTISAMPLESAMPLES) = %d\n", nValue);
					if (nValue > 1)
						_mapSupportedInt[MultiSamplingSamples] = nValue;
					else
						_mapSupportedBool[MultiSampling] = false;
				}

				// III) Return the updated frame buffer specs.
				//nWidth = nWidth; // Unchanged.
				//nHeight = nHeight; // Unchanged.
				nDepth = nCurrDepth;
				bFullScreen = nFullScreen ? true : false;
				bAlpha = nAlphaChannel ? true : false;

				SDL_GL_DeleteContext(context);
				context = NULL;
			}
			SDL_DestroyRenderer(renderer);
			renderer = NULL;
		}
		SDL_DestroyWindow(testWindow);
		testWindow = NULL;
	}
	return true;
}
#endif
// Best supported features detection for the given specs of the frame buffer.
bool GfglFeatures::detectBestSupport(int& nWidth, int& nHeight, int& nDepth,
                                     bool& bAlpha, bool& bFullScreen, bool& bBumpMapping, bool& bStereoVision, int &nAniFilt)
{
	GfLogInfo("Detecting best supported features for a %dx%dx%d%s frame buffer.\n",
			  nWidth, nHeight, nDepth, bFullScreen ? " full-screen" : "");

	// I) Detection of the max possible values for requested features.
	//    (to do that, we need to try setting up the video modes for real).
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);


	SDL_Surface* pWinSurface = 0;

	int nAlphaChannel = bAlpha ? 1 : 0;
	int nCurrDepth = nDepth;
	int nFullScreen = bFullScreen ? 1 : 0;
	int nStereoVision = bStereoVision ? 1 : 0;


	while (!pWinSurface && nFullScreen >= 0)
	{
		GfLogTrace("Trying %s mode\n", nFullScreen ? "full-screen" : "windowed");

#if SDL_MAJOR_VERSION >= 2
		const int bfVideoMode = SDL_WINDOW_OPENGL | (nFullScreen ? SDL_WINDOW_FULLSCREEN : 0);
        SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
#else
		const int bfVideoMode = SDL_OPENGL | (nFullScreen ? SDL_FULLSCREEN : 0);
#endif

		nAlphaChannel = bAlpha ? 1 : 0;
		while (!pWinSurface && nAlphaChannel >= 0)
		{
			GfLogTrace("Trying with%s alpha channel\n", nAlphaChannel ? "" : "out");
			nCurrDepth = nDepth;
			while (!pWinSurface && nCurrDepth >= 16)
			{
				GfLogTrace("Trying %d bits RVB+A color depth\n", nCurrDepth);
				SDL_GL_SetAttribute(SDL_GL_RED_SIZE, nCurrDepth/4);
				SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, nCurrDepth/4);
				SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, nCurrDepth/4);
				SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, (3*nCurrDepth)/4);
				SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, nAlphaChannel ? nCurrDepth/4 : 0);

				while (!pWinSurface && nStereoVision >= 0)
				{
					GfLogTrace("Trying with%s stereo vision\n", nStereoVision ? "" : "out");
					if (nStereoVision)
						SDL_GL_SetAttribute(SDL_GL_STEREO, GL_TRUE);
					else
						SDL_GL_SetAttribute(SDL_GL_STEREO, GL_FALSE);
		
					// Anti-aliasing : detect the max supported number of samples
					// (assumed to be <= 32).
					int nMaxMultiSamples = 32; // Hard coded max value for the moment.
					while (!pWinSurface && nMaxMultiSamples > 1)
					{
						// Set the anti-aliasing attributes and setup the video mode.
						GfLogTrace("Trying %dx anti-aliasing\n", nMaxMultiSamples);
						SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
						SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, nMaxMultiSamples);
#if SDL_MAJOR_VERSION >= 2
						SDL_SetWindowSize(GfuiWindow, nWidth, nHeight);

						SDL_Renderer *renderer = SDL_CreateRenderer(GfuiWindow, -1, 0);
						SDL_RenderPresent(renderer);

						SDL_GLContext context;
						context = SDL_GL_CreateContext(GfuiWindow);

						pWinSurface = SDL_CreateRGBSurface(0, nWidth, nHeight, nCurrDepth,
#if SDL_BYTEORDER == SDL_LIL_ENDIAN 
							0x00FF0000, 0x0000FF00, 0x000000FF,
#else 
							0x000000FF, 0x0000FF00, 0x00FF0000,
#endif 
							0x00000000); 
#else
						pWinSurface = SDL_SetVideoMode(nWidth, nHeight, nCurrDepth, bfVideoMode);
#endif
	
						// Now check if we have a video mode, and if it actually features
						// what we specified.
						int nActualSampleBuffers = 0;
						int nActualMultiSamples = 0;
						if (pWinSurface) {
							SDL_GL_GetAttribute(SDL_GL_MULTISAMPLEBUFFERS, &nActualSampleBuffers);
							SDL_GL_GetAttribute(SDL_GL_MULTISAMPLESAMPLES, &nActualMultiSamples);
						}
	 					//GfLogDebug("nMaxMultiSamples=%d : nActualSampleBuffers=%d, nActualMultiSamples=%d\n",
	 					//		   nMaxMultiSamples, nActualSampleBuffers, nActualMultiSamples);
	
						// If not, try a lower number of samples.
						if (nActualSampleBuffers == 0 || nActualMultiSamples != nMaxMultiSamples)
							pWinSurface = 0;
						if (!pWinSurface)
						{
							GfLogTrace("%d+%d bit %dx anti-aliased double-buffer not supported\n",
									   3*nCurrDepth/4, nCurrDepth/4, nMaxMultiSamples);
							nMaxMultiSamples /= 2;
						}
					}
	
					// Failed : try without anti-aliasing.
					if (!pWinSurface)
					{
						SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
						SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
#if SDL_MAJOR_VERSION >= 2
						SDL_SetWindowSize(GfuiWindow, nWidth, nHeight);

						SDL_Renderer *renderer = SDL_CreateRenderer(GfuiWindow, -1, 0);
						SDL_RenderPresent(renderer);

						SDL_GLContext context;
						context = SDL_GL_CreateContext(GfuiWindow);

						pWinSurface = SDL_CreateRGBSurface(0, nWidth, nHeight, nCurrDepth,
#if SDL_BYTEORDER == SDL_LIL_ENDIAN 
							0x00FF0000, 0x0000FF00, 0x000000FF,
#else 
							0x000000FF, 0x0000FF00, 0x00FF0000,
#endif 
							0x00000000); 
#else
						pWinSurface = SDL_SetVideoMode(nWidth, nHeight, nCurrDepth, bfVideoMode);
#endif
						if (!pWinSurface)
							GfLogTrace("%d+%d bit double-buffer not supported\n",
									   3*nCurrDepth/4, nCurrDepth/4);
					}
	
					// Failed : try without StereoVision
					if (!pWinSurface)
						nStereoVision--;
				}

				// Failed : try with lower color depth.
				if (!pWinSurface)
					nCurrDepth -= 8;
			}

			// Failed : try without alpha channel if not already done
			// (Note: it this really relevant ?).
			if (!pWinSurface)
				nAlphaChannel--;
		}

		// Failed : try a windowed mode if not already done.
		if (!pWinSurface)
			nFullScreen--;
	}

	// Failed : no more idea :-(
	if (!pWinSurface)
	{
		// Reset support data (will result in emptying the section when storing,
		// thus forcing new detection when checkSupport will be called again).
		_mapSupportedBool.clear();
		_mapSupportedInt.clear();
		
		GfLogError("No supported 'best' video mode found for a %dx%dx%d%s frame buffer.\n",
				   nWidth, nHeight, nDepth, bFullScreen ? " full-screen" : "");
		
		return false;
	}	
	
	// II) Read-out what we have from the up-and-running frame buffer
	//     and set "supported" values accordingly.
	
	// 1) Standard features.
	detectStandardSupport();
		
	// 2) Multi-sampling = anti-aliasing
	int nValue;
	SDL_GL_GetAttribute(SDL_GL_MULTISAMPLEBUFFERS, &nValue);
	_mapSupportedBool[MultiSampling] = nValue != 0;
	//GfLogDebug("SDL_GL_GetAttribute(SDL_GL_MULTISAMPLEBUFFERS) = %d\n", nValue);
	if (nValue)
	{
		SDL_GL_GetAttribute(SDL_GL_MULTISAMPLESAMPLES, &nValue);
		//GfLogDebug("SDL_GL_GetAttribute(SDL_GL_MULTISAMPLESAMPLES) = %d\n", nValue);
		if (nValue > 1)
			_mapSupportedInt[MultiSamplingSamples] = nValue;
		else
			_mapSupportedBool[MultiSampling] = false;
	}

	// III) Return the updated frame buffer specs.
	//nWidth = nWidth; // Unchanged.
	//nHeight = nHeight; // Unchanged.
	nDepth = nCurrDepth;
	bFullScreen = nFullScreen ? true : false;
	bAlpha = nAlphaChannel ? true : false;
	
	return true;
}

bool GfglFeatures::loadSupport(int &nWidth, int &nHeight, int &nDepth,
                               bool &bAlpha, bool &bFullScreen, bool &bBump, bool &bStereo, int &nAniFilt,void* hparmConfig)
{
	// Clear support data.
	_mapSupportedBool.clear();
	_mapSupportedInt.clear();

	// Open the config file if not already done.
	void* hparm = hparmConfig ? hparmConfig : openConfigFile();

	// Load the frame buffer specs for the stored supported features.
    nWidth =
		(int)GfParmGetNum(hparm, GFSCR_SECT_GLDETSPECS, GFSCR_ATT_WIN_X, pszNoUnit, 0);
    nHeight =
		(int)GfParmGetNum(hparm, GFSCR_SECT_GLDETSPECS, GFSCR_ATT_WIN_Y, pszNoUnit, 0);
    nDepth =
		(int)GfParmGetNum(hparm, GFSCR_SECT_GLDETSPECS, GFSCR_ATT_BPP, pszNoUnit, 0);
    nAniFilt =
        (int)GfParmGetNum(hparm, GFSCR_SECT_GLDETSPECS, GFSCR_ATT_ANISOTROPICFILTERING, pszNoUnit, 0);
    bAlpha =
		std::string(GfParmGetStr(hparm, GFSCR_SECT_GLDETSPECS, GFSCR_ATT_ALPHACHANNEL, GFSCR_VAL_NO))
		== GFSCR_VAL_YES;
    bFullScreen =
		std::string(GfParmGetStr(hparm, GFSCR_SECT_GLDETSPECS, GFSCR_ATT_FSCR, GFSCR_VAL_NO))
		== GFSCR_VAL_YES;
    bStereo =
		std::string(GfParmGetStr(hparm, GFSCR_SECT_GLDETSPECS, GFSCR_ATT_STEREOVISION, GFSCR_VAL_NO))
		== GFSCR_VAL_YES;
	bBump =
		std::string(GfParmGetStr(hparm, GFSCR_SECT_GLDETSPECS, GFSCR_ATT_BUMPMAPPING, GFSCR_VAL_NO))
		== GFSCR_VAL_YES;

	// Check that we have something supported, and return if not.
	if (nWidth == 0 || nHeight == 0 || nDepth == 0)
	{
		GfLogTrace("No info found about best supported features for these specs ; "
				   "will need a detection pass.\n");

		// Close config file if we open it.
		if (!hparmConfig)
			closeConfigFile(hparm);

		return false;
	}
		
	
	// Here, we only update _mapSupportedXXX only if something relevant in the config file
	// If there's nothing or something not expected, it means no support.
	
	// 1) Double-buffer.
	const std::string strDoubleBuffer =
		GfParmGetStr(hparm, GFSCR_SECT_GLDETFEATURES, GFSCR_ATT_DOUBLEBUFFER, "");
	if (strDoubleBuffer == GFSCR_VAL_YES)
		_mapSupportedBool[DoubleBuffer] = true;
	else if (strDoubleBuffer == GFSCR_VAL_NO)
		_mapSupportedBool[DoubleBuffer] = false;

	// 2) Color buffer depth.
	const int nColorDepth =
		(int)GfParmGetNum(hparm, GFSCR_SECT_GLDETFEATURES, GFSCR_ATT_COLORDEPTH,
						  pszNoUnit, (tdble)0);
	if (nColorDepth > 0)
		_mapSupportedInt[ColorDepth] = nColorDepth;

	// 3) Alpha-channel depth.
	const int nAlphaDepth =
		(int)GfParmGetNum(hparm, GFSCR_SECT_GLDETFEATURES, GFSCR_ATT_ALPHADEPTH,
						  pszNoUnit, (tdble)-1);
	if (nAlphaDepth >= 0)
		_mapSupportedInt[AlphaDepth] = nAlphaDepth;

	// 4) Max texture size.
	const int nMaxTexSize =
		(int)GfParmGetNum(hparm, GFSCR_SECT_GLDETFEATURES, GFSCR_ATT_MAXTEXTURESIZE,
						  pszNoUnit, (tdble)0);
	if (nMaxTexSize > 0)
		_mapSupportedInt[TextureMaxSize] = nMaxTexSize;

	// 5) Texture compression.
	const std::string strTexComp =
		GfParmGetStr(hparm, GFSCR_SECT_GLDETFEATURES, GFSCR_ATT_TEXTURECOMPRESSION, "");
	if (strTexComp == GFSCR_VAL_YES)
		_mapSupportedBool[TextureCompression] = true;
	else if (strTexComp == GFSCR_VAL_NO)
		_mapSupportedBool[TextureCompression] = false;

	// 6) Multi-texturing.
	const std::string strMultiTex =
		GfParmGetStr(hparm, GFSCR_SECT_GLDETFEATURES, GFSCR_ATT_MULTITEXTURING, "");
	if (strMultiTex == GFSCR_VAL_YES)
		_mapSupportedBool[MultiTexturing] = true;
	else if (strMultiTex == GFSCR_VAL_NO)
		_mapSupportedBool[MultiTexturing] = false;

	const int nMultiTexUnits =
		(int)GfParmGetNum(hparm, GFSCR_SECT_GLDETFEATURES, GFSCR_ATT_MULTITEXTURINGUNITS,
						  pszNoUnit, (tdble)0);
	if (nMultiTexUnits > 0)
		_mapSupportedInt[MultiTexturingUnits] = nMultiTexUnits;

	// 7) Rectangle textures).
	const std::string strRectTex =
		GfParmGetStr(hparm, GFSCR_SECT_GLDETFEATURES, GFSCR_ATT_RECTANGLETEXTURES, "");
	if (strRectTex == GFSCR_VAL_YES)
		_mapSupportedBool[TextureRectangle] = true;
	else if (strRectTex == GFSCR_VAL_NO)
		_mapSupportedBool[TextureRectangle] = false;

	// 8) Non-power-of-2 textures.
	const std::string strNonPoTTex =
		GfParmGetStr(hparm, GFSCR_SECT_GLDETFEATURES, GFSCR_ATT_NONPOTTEXTURES, "");
	if (strNonPoTTex == GFSCR_VAL_YES)
		_mapSupportedBool[TextureNonPowerOf2] = true;
	else if (strNonPoTTex == GFSCR_VAL_NO)
		_mapSupportedBool[TextureNonPowerOf2] = false;

	// 9) Multi-sampling.
	const std::string strMultiSamp =
		GfParmGetStr(hparm, GFSCR_SECT_GLDETFEATURES, GFSCR_ATT_MULTISAMPLING, "");
	if (strMultiSamp == GFSCR_VAL_YES)
		_mapSupportedBool[MultiSampling] = true;
	else if (strMultiSamp == GFSCR_VAL_NO)
		_mapSupportedBool[MultiSampling] = false;
	
	const int nMultiSampSamples =
		(int)GfParmGetNum(hparm, GFSCR_SECT_GLDETFEATURES, GFSCR_ATT_MULTISAMPLINGSAMPLES,
						  pszNoUnit, (tdble)0);
	if (nMultiSampSamples > 0)
		_mapSupportedInt[MultiSamplingSamples] = nMultiSampSamples;

	// 10) Stereo Vision
	const std::string strStereoVision =
		GfParmGetStr(hparm, GFSCR_SECT_GLDETFEATURES, GFSCR_ATT_STEREOVISION, "");
	if (strStereoVision == GFSCR_VAL_YES)
		_mapSupportedBool[StereoVision] = true;
	else if (strStereoVision == GFSCR_VAL_NO)
		_mapSupportedBool[StereoVision] = false;

	// 11) Bump Mapping.
	const std::string strBumpMapping =
		GfParmGetStr(hparm, GFSCR_SECT_GLDETFEATURES, GFSCR_ATT_BUMPMAPPING, "");
    if (strTexComp == GFSCR_VAL_YES) //strTexComp ? Bug ?
		_mapSupportedBool[BumpMapping] = true;
	else if (strTexComp == GFSCR_VAL_NO)
		_mapSupportedBool[BumpMapping] = false;

    // 11) Anisotropic Filtering.
    const int nAF =
        (int)GfParmGetNum(hparm, GFSCR_SECT_GLDETFEATURES, GFSCR_ATT_ANISOTROPICFILTERING,
                          pszNoUnit, (tdble)0);
    if (nMaxTexSize > 0)
        _mapSupportedInt[AnisotropicFiltering] =nAF;

	
	// Close config file if we open it.
	if (!hparmConfig)
		closeConfigFile(hparm);

	// Trace best supported features.
	dumpSupport();

	return true;
}

void GfglFeatures::storeSupport(int nWidth, int nHeight, int nDepth,
                                bool bAlpha, bool bFullScreen, bool bBump, bool bStereo, int nAniFilt, void* hparmConfig)
{
	// Open the config file if not already done.
	void* hparm = hparmConfig ? hparmConfig : openConfigFile();

	// If there's support for nothing, remove all.
	if (_mapSupportedBool.empty() && _mapSupportedInt.empty())
	{
		// Frame buffer specs.
		GfParmRemoveSection(hparm, GFSCR_SECT_GLDETSPECS);
	
		// Supported values.
		GfParmRemoveSection(hparm, GFSCR_SECT_GLDETFEATURES);
	}

	// If there's support for anything, store it.
	else
	{
		// Write new frame buffer specs for the stored supported features.
		GfParmSetNum(hparm, GFSCR_SECT_GLDETSPECS, GFSCR_ATT_WIN_X, pszNoUnit,
					 (tdble)nWidth);
		GfParmSetNum(hparm, GFSCR_SECT_GLDETSPECS, GFSCR_ATT_WIN_Y, pszNoUnit,
					 (tdble)nHeight);
		GfParmSetNum(hparm, GFSCR_SECT_GLDETSPECS, GFSCR_ATT_BPP, pszNoUnit,
					 (tdble)nDepth);
        GfParmSetNum(hparm, GFSCR_SECT_GLDETSPECS, GFSCR_ATT_ANISOTROPICFILTERING, pszNoUnit,
                     (tdble)nAniFilt);
        GfParmSetStr(hparm, GFSCR_SECT_GLDETSPECS, GFSCR_ATT_ALPHACHANNEL,
					 bAlpha ? GFSCR_VAL_YES : GFSCR_VAL_NO);
		GfParmSetStr(hparm, GFSCR_SECT_GLDETSPECS, GFSCR_ATT_FSCR,
					 bFullScreen ? GFSCR_VAL_YES : GFSCR_VAL_NO);
		GfParmSetStr(hparm, GFSCR_SECT_GLDETSPECS, GFSCR_ATT_STEREOVISION,
					 bStereo ? GFSCR_VAL_YES : GFSCR_VAL_NO);
		GfParmSetStr(hparm, GFSCR_SECT_GLDETSPECS, GFSCR_ATT_BUMPMAPPING,
					 bBump ? GFSCR_VAL_YES : GFSCR_VAL_NO);
	
		// Write new values (remove the ones with no value supported).
		// 1) Double-buffer.
		GfParmSetStr(hparm, GFSCR_SECT_GLDETFEATURES, GFSCR_ATT_DOUBLEBUFFER,
					 isSupported(DoubleBuffer) ? GFSCR_VAL_YES : GFSCR_VAL_NO);

		// 2) Color buffer depth.
		if (getSupported(ColorDepth) != InvalidInt)
			GfParmSetNum(hparm, GFSCR_SECT_GLDETFEATURES, GFSCR_ATT_COLORDEPTH, pszNoUnit,
						 (tdble)getSupported(ColorDepth));
		else
			GfParmRemove(hparm, GFSCR_SECT_GLDETFEATURES, GFSCR_ATT_COLORDEPTH);

		// 3) Alpha-channel depth.
		if (getSupported(AlphaDepth) != InvalidInt)
			GfParmSetNum(hparm, GFSCR_SECT_GLDETFEATURES, GFSCR_ATT_ALPHADEPTH, pszNoUnit,
						 (tdble)getSupported(AlphaDepth));
		else
			GfParmRemove(hparm, GFSCR_SECT_GLDETFEATURES, GFSCR_ATT_ALPHADEPTH);

		// 4) Max texture size.
		if (getSupported(TextureMaxSize) != InvalidInt)
			GfParmSetNum(hparm, GFSCR_SECT_GLDETFEATURES, GFSCR_ATT_MAXTEXTURESIZE, pszNoUnit,
						 (tdble)getSupported(TextureMaxSize));
		else
			GfParmRemove(hparm, GFSCR_SECT_GLDETFEATURES, GFSCR_ATT_MAXTEXTURESIZE);
		
		// 5) Texture compression.
		GfParmSetStr(hparm, GFSCR_SECT_GLDETFEATURES, GFSCR_ATT_TEXTURECOMPRESSION,
					 isSupported(TextureCompression) ? GFSCR_VAL_YES : GFSCR_VAL_NO);
		
		// 6) Multi-texturing.
		GfParmSetStr(hparm, GFSCR_SECT_GLDETFEATURES, GFSCR_ATT_MULTITEXTURING,
					 isSupported(MultiTexturing) ? GFSCR_VAL_YES : GFSCR_VAL_NO);
		if (getSupported(MultiTexturingUnits) != InvalidInt)
			GfParmSetNum(hparm, GFSCR_SECT_GLDETFEATURES, GFSCR_ATT_MULTITEXTURINGUNITS, pszNoUnit,
						 (tdble)getSupported(MultiTexturingUnits));
		else
			GfParmRemove(hparm, GFSCR_SECT_GLDETFEATURES, GFSCR_ATT_MULTITEXTURINGUNITS);

		// 7) Rectangle textures).
		GfParmSetStr(hparm, GFSCR_SECT_GLDETFEATURES, GFSCR_ATT_RECTANGLETEXTURES,
					 isSupported(TextureRectangle) ? GFSCR_VAL_YES : GFSCR_VAL_NO);

		// 8) Non-power-of-2 textures.
		GfParmSetStr(hparm, GFSCR_SECT_GLDETFEATURES, GFSCR_ATT_NONPOTTEXTURES,
					 isSupported(TextureNonPowerOf2) ? GFSCR_VAL_YES : GFSCR_VAL_NO);

		// 9) Multi-sampling.
		GfParmSetStr(hparm, GFSCR_SECT_GLDETFEATURES, GFSCR_ATT_MULTISAMPLING,
					 isSupported(MultiSampling) ? GFSCR_VAL_YES : GFSCR_VAL_NO);
	
		if (getSupported(MultiSamplingSamples) != InvalidInt)
			GfParmSetNum(hparm, GFSCR_SECT_GLDETFEATURES, GFSCR_ATT_MULTISAMPLINGSAMPLES, pszNoUnit,
						 (tdble)getSupported(MultiSamplingSamples));
		else
			GfParmRemove(hparm, GFSCR_SECT_GLDETFEATURES, GFSCR_ATT_MULTISAMPLINGSAMPLES);

		// 10) Stereo Vision
		GfParmSetStr(hparm, GFSCR_SECT_GLDETFEATURES, GFSCR_ATT_STEREOVISION,
					 isSupported(StereoVision) ? GFSCR_VAL_YES : GFSCR_VAL_NO);
		
		// 11) Bump Mapping
		GfParmSetStr(hparm, GFSCR_SECT_GLDETFEATURES, GFSCR_ATT_BUMPMAPPING,
					 isSupported(BumpMapping) ? GFSCR_VAL_YES : GFSCR_VAL_NO);

        // 12) Aniso Filtering
        if (getSupported(AnisotropicFiltering) != InvalidInt)
            GfParmSetNum(hparm, GFSCR_SECT_GLDETFEATURES, GFSCR_ATT_ANISOTROPICFILTERING, pszNoUnit,
                         (tdble)getSupported(AnisotropicFiltering));
        else
            GfParmRemove(hparm, GFSCR_SECT_GLDETFEATURES, GFSCR_ATT_ANISOTROPICFILTERING);

	}		
	
	// Write new params to config file.
	GfParmWriteFile(NULL, hparm, "Screen");
	
	// Close config file if we open it.
	if (!hparmConfig)
		closeConfigFile(hparm);

	// Trace resulting best supported features.
	dumpSupport();
}

bool GfglFeatures::checkBestSupport(int nWidth, int nHeight, int nDepth,
                                    bool bAlpha, bool bFullScreen, bool bBump, bool bStereo,int nAniFilt, void* hparmConfig)
{
	// Open the config file if not already done.
	void* hparm = hparmConfig ? hparmConfig : openConfigFile();

	// Get the frame buffer specs that are associated with the detected
	// Open GL features in the config file, if any.
    int nDetWidth, nDetHeight, nDetDepth, nDetAni;
	bool bDetFullScreen, bDetAlpha, bDetBump, bDetStereo;
	bool bPrevSupportFound =
        loadSupport(nDetWidth, nDetHeight, nDetDepth, bDetAlpha, bDetFullScreen, bDetBump, bDetStereo,nDetAni, hparm);

	// Compare with the requested frame buffer specs
	// and run a new supported feature detection if any diffference.
	bool bSupportFound = true;
	if (!bPrevSupportFound || nWidth != nDetWidth || nHeight != nDetHeight || nDepth != nDetDepth
        || bAlpha != bDetAlpha || bFullScreen != bDetFullScreen || bStereo != bDetStereo || bBump != bDetBump || nAniFilt!= nDetAni)
	{
		nDetWidth = nWidth;
		nDetHeight = nHeight;
		nDetDepth = nDepth;
		bDetFullScreen = bFullScreen;
		bDetAlpha = bAlpha;
		bDetStereo = bStereo;
		bDetBump = bBump;
        nDetAni = nAniFilt;
		bSupportFound =
#if SDL_MAJOR_VERSION < 2
            detectBestSupport(nDetWidth, nDetHeight, nDetDepth, bDetAlpha, bDetFullScreen, bDetBump, bDetStereo, nDetAni);
#else
            detectBestSupportSDL2(nDetWidth, nDetHeight, nDetDepth, bDetAlpha, bDetFullScreen, bDetBump, bDetStereo, nDetAni);
#endif

		// Store support data in any case.
        storeSupport(nDetWidth, nDetHeight, nDetDepth, bDetAlpha, bDetFullScreen, bDetBump, bDetStereo,nDetAni, hparm);

		// If frame buffer specs supported, update relevant user settings and restart.
		if (bSupportFound)
		{
			// Write new user settings about the frame buffer specs
			// (the detection process might have down-casted them ...).
			// Note: Sure the specs are in the 'in-test' state here,
			//       otherwise they would not have changed.
			GfParmSetNum(hparm, GFSCR_SECT_INTESTPROPS, GFSCR_ATT_WIN_X, pszNoUnit,
						 (tdble)nDetWidth);
			GfParmSetNum(hparm, GFSCR_SECT_INTESTPROPS, GFSCR_ATT_WIN_Y, pszNoUnit,
						 (tdble)nDetHeight);
			GfParmSetNum(hparm, GFSCR_SECT_INTESTPROPS, GFSCR_ATT_BPP, pszNoUnit,
						 (tdble)nDetDepth);
			GfParmSetStr(hparm, GFSCR_SECT_INTESTPROPS, GFSCR_ATT_ALPHACHANNEL,
						 bDetAlpha ? GFSCR_VAL_YES : GFSCR_VAL_NO);
			GfParmSetStr(hparm, GFSCR_SECT_INTESTPROPS, GFSCR_ATT_FSCR,
						 bDetFullScreen ? GFSCR_VAL_YES : GFSCR_VAL_NO);

			// But make sure they are not validated yet at restart (only next time if OK).
			GfParmSetStr(hparm, GFSCR_SECT_INTESTPROPS, GFSCR_ATT_TESTSTATE,
						 GFSCR_VAL_TODO);

			// Write new params to config file.
			GfParmWriteFile(NULL, hparm, "Screen");

			// Close the config file ...
			closeConfigFile(hparm);

			// ... as we are restarting ...
			GfuiApp().restart();

			// Next time we pass in this function, loadSupport() will give
			// the right values for all features ...
		}
	}

	if (!hparmConfig)
		closeConfigFile(hparm);
	
	return bSupportFound;
}

void GfglFeatures::dumpHardwareInfo() const
{
	GfLogInfo("Video hardware info :\n");
	GfLogInfo("  Vendor   : %s\n", glGetString(GL_VENDOR));
	GfLogInfo("  Renderer : %s\n", glGetString(GL_RENDERER));
	GfLogInfo("  Version  : %s\n", glGetString(GL_VERSION));
}

void GfglFeatures::dumpSupport() const
{
	GfLogInfo("Supported OpenGL features :\n");

	if (_mapSupportedBool.empty() && _mapSupportedInt.empty())
	{
		GfLogInfo("  Unknown (detection failed).\n");
		return;
	}
	
	GfLogInfo("  Double buffer           : %s\n",
			  isSupported(DoubleBuffer) ? "Yes" : "No");
	GfLogInfo("  Color depth             : %d bits\n",
			  getSupported(ColorDepth));
	GfLogInfo("  Alpha channel           : %s",
			  getSupported(AlphaDepth) > 0 ? "Yes" : "No");
	if (getSupported(AlphaDepth) > 0)
		GfLogInfo(" (%d bits)", getSupported(AlphaDepth));
	GfLogInfo("\n");
	GfLogInfo("  Max texture size        : %d\n",
			  getSupported(TextureMaxSize));
	GfLogInfo("  Texture compression     : %s\n",
			  isSupported(TextureCompression) ? "Yes" : "No");
	GfLogInfo("  Multi-texturing         : %s",
			  isSupported(MultiTexturing) ? "Yes" : "No");
	if (isSupported(MultiTexturing))
		GfLogInfo(" (%d units)", getSupported(MultiTexturingUnits));
	GfLogInfo("\n");
	GfLogInfo("  Rectangle textures      : %s\n",
			  isSupported(TextureRectangle) ? "Yes" : "No");
	GfLogInfo("  Non power-of-2 textures : %s\n",
			  isSupported(TextureNonPowerOf2) ? "Yes" : "No");
	GfLogInfo("  Multi-sampling          : %s",
			  isSupported(MultiSampling) ? "Yes" : "No");
	if (isSupported(MultiSampling) && getSupported(MultiSamplingSamples) > 1)
		GfLogInfo(" (%d samples)", getSupported(MultiSamplingSamples));
	GfLogInfo("\n");
	GfLogInfo("  Stereo Vision           : %s\n",
			  isSupported(StereoVision) ? "Yes" : "No");
	GfLogInfo("  Bump Mapping            : %s\n",
			  isSupported(BumpMapping) ? "Yes" : "No");
    GfLogInfo("  Anisotropic Filtering   : %d\n",
              getSupported(AnisotropicFiltering));
}

// Load the selected OpenGL features from the config file.
void GfglFeatures::loadSelection(void* hparmConfig)
{
	// Open the config file if not already done.
	void* hparm = hparmConfig ? hparmConfig : openConfigFile();

	// Select the OpenGL features according to the user settings (when relevant)
	// or/and to the supported values (by default, select the max supported values).

	// 1) Double-buffer : not user-customizable.
	_mapSelectedBool[DoubleBuffer] = isSupported(DoubleBuffer);

	// 2) Color buffer depth : not user-customizable.
	_mapSelectedInt[ColorDepth] = getSupported(ColorDepth);

	// 3) Alpha-channel depth : not user-customizable.
	_mapSelectedInt[AlphaDepth] = getSupported(AlphaDepth);

	// 4) Max texture size : load from config file.
	_mapSelectedInt[TextureMaxSize] =
		(int)GfParmGetNum(hparm, GFSCR_SECT_GLSELFEATURES, GFSCR_ATT_MAXTEXTURESIZE,
						  pszNoUnit, (tdble)getSupported(TextureMaxSize));
	if (_mapSelectedInt[TextureMaxSize] > getSupported(TextureMaxSize))
		_mapSelectedInt[TextureMaxSize] = getSupported(TextureMaxSize);

	// 5) Texture compression : load from config file.
	_mapSelectedBool[TextureCompression] =
		isSupported(TextureCompression)
		&& std::string(GfParmGetStr(hparm, GFSCR_SECT_GLSELFEATURES, GFSCR_ATT_TEXTURECOMPRESSION,
									GFSCR_ATT_TEXTURECOMPRESSION_ENABLED))
		   == GFSCR_ATT_TEXTURECOMPRESSION_ENABLED;

	// 6) Multi-texturing : load from config file.
	_mapSelectedBool[MultiTexturing] =
		isSupported(MultiTexturing)
		&& std::string(GfParmGetStr(hparm, GFSCR_SECT_GLSELFEATURES, GFSCR_ATT_MULTITEXTURING,
									GFSCR_ATT_MULTITEXTURING_ENABLED))
		   == GFSCR_ATT_MULTITEXTURING_ENABLED;
	_mapSelectedInt[MultiTexturingUnits] = 
		(int)GfParmGetNum(hparm, GFSCR_SECT_GLSELFEATURES, GFSCR_ATT_MULTITEXTURINGUNITS,
						  pszNoUnit, (tdble)getSupported(TextureMaxSize));
	if (_mapSelectedInt[MultiTexturingUnits] > getSupported(MultiTexturingUnits))
		_mapSelectedInt[MultiTexturingUnits] = getSupported(MultiTexturingUnits);

	// 7) Rectangle textures : not user-customizable.
	_mapSelectedBool[TextureRectangle] = isSupported(TextureRectangle);

	// 8) Non-power-of-2 textures : not user-customizable.
	_mapSelectedBool[TextureNonPowerOf2] = isSupported(TextureNonPowerOf2);

	// 9) Multi-sampling : load from config file.
	const std::string strMultiSamp =
		GfParmGetStr(hparm, GFSCR_SECT_GLSELFEATURES, GFSCR_ATT_MULTISAMPLING,
					 GFSCR_ATT_MULTISAMPLING_ENABLED);
	_mapSelectedBool[MultiSampling] =
		isSupported(MultiSampling)
		&& std::string(GfParmGetStr(hparm, GFSCR_SECT_GLSELFEATURES, GFSCR_ATT_MULTISAMPLING,
									GFSCR_ATT_MULTISAMPLING_ENABLED))
		   == GFSCR_ATT_MULTISAMPLING_ENABLED;
	
	_mapSelectedInt[MultiSamplingSamples] =
		(int)GfParmGetNum(hparm, GFSCR_SECT_GLSELFEATURES, GFSCR_ATT_MULTISAMPLINGSAMPLES,
						  pszNoUnit, (tdble)8); // Good but reasonable value.
	if (_mapSelectedInt[MultiSamplingSamples] > getSupported(MultiSamplingSamples))
		_mapSelectedInt[MultiSamplingSamples] = getSupported(MultiSamplingSamples);

	// 10) Stereo Vision : load from config file.
	const std::string strStereoVision =
		GfParmGetStr(hparm, GFSCR_SECT_GLSELFEATURES, GFSCR_ATT_STEREOVISION,
					 GFSCR_ATT_STEREOVISION_ENABLED);
	_mapSelectedBool[StereoVision] =
		isSupported(StereoVision)
		&& std::string(GfParmGetStr(hparm, GFSCR_SECT_GLSELFEATURES, GFSCR_ATT_STEREOVISION,
									GFSCR_ATT_STEREOVISION_ENABLED))
		   == GFSCR_ATT_STEREOVISION_ENABLED;

	// 11) Bump Mapping : load from config file.
	_mapSelectedBool[BumpMapping] =
		isSupported(BumpMapping)
		&& std::string(GfParmGetStr(hparm, GFSCR_SECT_GLSELFEATURES, GFSCR_ATT_BUMPMAPPING,
									GFSCR_ATT_BUMPMAPPING_ENABLED))
		   == GFSCR_ATT_BUMPMAPPING_ENABLED;

    // 12) Anisotropic Filtering : load from config file.
    _mapSelectedInt[AnisotropicFiltering] = (int)GfParmGetNum(hparm, GFSCR_SECT_GLSELFEATURES, GFSCR_ATT_ANISOTROPICFILTERING,
                                    pszNoUnit, (tdble)getSupported(AnisotropicFiltering));
	// Close config file if we open it.
	if (!hparmConfig)
		closeConfigFile(hparm);
	
	// Display what we have really selected (after checking / fixing to supported values).
	dumpSelection();
}


// Save settings to screen.xml
void GfglFeatures::storeSelection(void* hparmConfig) const
{
	// Display what we have selected.
	dumpSelection();

	// Open the config file if not already done.
	void* hparm = hparmConfig ? hparmConfig : openConfigFile();

	// Write new values.
	GfParmSetStr(hparm, GFSCR_SECT_GLSELFEATURES, GFSCR_ATT_TEXTURECOMPRESSION,
				 isSelected(TextureCompression)
				 ? GFSCR_ATT_TEXTURECOMPRESSION_ENABLED : GFSCR_ATT_TEXTURECOMPRESSION_DISABLED);
	if (getSupported(TextureMaxSize) != InvalidInt)
		GfParmSetNum(hparm, GFSCR_SECT_GLSELFEATURES, GFSCR_ATT_MAXTEXTURESIZE, pszNoUnit,
					 (tdble)getSelected(TextureMaxSize));
	else
		GfParmRemove(hparm, GFSCR_SECT_GLSELFEATURES, GFSCR_ATT_MAXTEXTURESIZE);
		
	GfParmSetStr(hparm, GFSCR_SECT_GLSELFEATURES, GFSCR_ATT_MULTITEXTURING,
				 isSelected(MultiTexturing)
				 ? GFSCR_ATT_MULTITEXTURING_ENABLED : GFSCR_ATT_MULTITEXTURING_DISABLED);
	GfParmSetStr(hparm, GFSCR_SECT_GLSELFEATURES, GFSCR_ATT_MULTISAMPLING,
				 isSelected(MultiSampling)
				 ? GFSCR_ATT_MULTISAMPLING_ENABLED : GFSCR_ATT_MULTISAMPLING_DISABLED);
	if (getSupported(MultiSamplingSamples) != InvalidInt)
		GfParmSetNum(hparm, GFSCR_SECT_GLSELFEATURES, GFSCR_ATT_MULTISAMPLINGSAMPLES, pszNoUnit,
					 (tdble)getSelected(MultiSamplingSamples));
	else
		GfParmRemove(hparm, GFSCR_SECT_GLSELFEATURES, GFSCR_ATT_MULTISAMPLINGSAMPLES);


	// Force 'best possible' mode for video initialization when anti-aliasing selected
	if (isSelected(MultiSampling))
	{
		// Use the 'in-test' specs if present, and reset the test state
		// (force a new validation).
		if (GfParmExistsSection(hparm, GFSCR_SECT_INTESTPROPS))
		{
			GfParmSetStr(hparm, GFSCR_SECT_INTESTPROPS, GFSCR_ATT_TESTSTATE,
						 GFSCR_VAL_INPROGRESS);
			GfParmSetStr(hparm, GFSCR_SECT_INTESTPROPS, GFSCR_ATT_VINIT,
						 GFSCR_VAL_VINIT_BEST);
		}

		// Otherwise, use the 'validated' specs ... no new validation needed
		// (if we can en/disable multi-sampling, it means that we already checked
		//  that it was possible, and how much).
		else
		{
			GfParmSetStr(hparm, GFSCR_SECT_VALIDPROPS, GFSCR_ATT_VINIT,
						 GFSCR_VAL_VINIT_BEST);
		}
	}

	GfParmSetStr(hparm, GFSCR_SECT_GLSELFEATURES, GFSCR_ATT_STEREOVISION,
				 isSelected(StereoVision)
				 ? GFSCR_ATT_STEREOVISION_ENABLED : GFSCR_ATT_STEREOVISION_DISABLED);

	GfParmSetStr(hparm, GFSCR_SECT_GLSELFEATURES, GFSCR_ATT_BUMPMAPPING,
				 isSelected(BumpMapping)
				 ? GFSCR_ATT_BUMPMAPPING_ENABLED : GFSCR_ATT_BUMPMAPPING_DISABLED);

    if (getSupported(AnisotropicFiltering) != InvalidInt)
        GfParmSetNum(hparm, GFSCR_SECT_GLSELFEATURES, GFSCR_ATT_ANISOTROPICFILTERING, pszNoUnit,
                     (tdble)getSelected(AnisotropicFiltering));
    else
        GfParmRemove(hparm, GFSCR_SECT_GLSELFEATURES, GFSCR_ATT_ANISOTROPICFILTERING);
	
	// Write new params to config file.
	GfParmWriteFile(NULL, hparm, "Screen");
	
	// Close config file if we open it.
	if (!hparmConfig)
		closeConfigFile(hparm);
}

void GfglFeatures::dumpSelection() const
{
	GfLogInfo("Selected OpenGL features :\n");
	GfLogInfo("  Double buffer           : %s\n", isSelected(DoubleBuffer) ? "On" : "Off");
	if (getSelected(ColorDepth) != InvalidInt)
		GfLogInfo("  Color depth             : %d bits\n", getSelected(ColorDepth));
	else
		GfLogInfo("  Color depth             : no selection\n");
	GfLogInfo("  Alpha channel           : %s",
			  getSelected(AlphaDepth) > 0 ? "On" : "Off");
	if (getSelected(AlphaDepth) > 0)
		GfLogInfo(" (%d bits)", getSelected(AlphaDepth));
	GfLogInfo("\n");
	if (getSelected(TextureMaxSize) != InvalidInt)
		GfLogInfo("  Max texture size        : %d\n", getSelected(TextureMaxSize));
	else
		GfLogInfo("  Max texture size        : no selection\n");
	GfLogInfo("  Texture compression     : %s\n", isSelected(TextureCompression) ? "On" : "Off");
	GfLogInfo("  Multi-texturing         : %s", isSelected(MultiTexturing) ? "On" : "Off");
	if (isSelected(MultiTexturing))
		GfLogInfo(" (%d units)", getSelected(MultiTexturingUnits));
	GfLogInfo("\n");
	GfLogInfo("  Rectangle textures      : %s\n", isSelected(TextureRectangle) ? "On" : "Off");
	GfLogInfo("  Non power-of-2 textures : %s\n", isSelected(TextureNonPowerOf2) ? "On" : "Off");
	GfLogInfo("  Multi-sampling          : %s", isSelected(MultiSampling) ? "On" : "Off");
	if (isSelected(MultiSampling))
		GfLogInfo(" (%d samples)", getSelected(MultiSamplingSamples));
	GfLogInfo("\n");
	GfLogInfo("  Stereo vision           : %s\n", isSelected(StereoVision) ? "On" : "Off");
	GfLogInfo("  Bump Mapping            : %s\n", isSelected(BumpMapping) ? "On" : "Off");
    GfLogInfo("  Anisotropic Filtering   : %d\n",
              getSupported(AnisotropicFiltering));
}

// Bool features management.
bool GfglFeatures::isSelected(EFeatureBool eFeature) const
{
	const std::map<EFeatureBool, bool>::const_iterator itFeature =
		_mapSelectedBool.find(eFeature);
	return itFeature == _mapSelectedBool.end() ? false : itFeature->second;
}

bool GfglFeatures::isSupported(EFeatureBool eFeature) const
{
	const std::map<EFeatureBool, bool>::const_iterator itFeature =
		_mapSupportedBool.find(eFeature);
	return itFeature == _mapSupportedBool.end() ? false : itFeature->second;
}

void GfglFeatures::select(EFeatureBool eFeature, bool bSelected)
{
	if (!bSelected || isSupported(eFeature))
		_mapSelectedBool[eFeature] = bSelected;
// 	GfLogDebug("GfglFeatures::select(Bool:%d, %s) : supp=%s, new=%s\n",
// 			   (int)eFeature, bSelected ? "true" : "false",
// 			   isSupported(eFeature) ? "true" : "false",
// 			   _mapSelectedBool[eFeature] ? "true" : "false");
}

// Int features management.
int GfglFeatures::getSelected(EFeatureInt eFeature) const
{
	const std::map<EFeatureInt, int>::const_iterator itFeature =
		_mapSelectedInt.find(eFeature);
	return itFeature == _mapSelectedInt.end() ? InvalidInt : itFeature->second;
}

int GfglFeatures::getSupported(EFeatureInt eFeature) const
{
	const std::map<EFeatureInt, int>::const_iterator itFeature =
		_mapSupportedInt.find(eFeature);
	return itFeature == _mapSupportedInt.end() ? InvalidInt : itFeature->second;
}

void GfglFeatures::select(EFeatureInt eFeature, int nSelectedValue)
{
	if (nSelectedValue > getSupported(eFeature))
		nSelectedValue = getSupported(eFeature);
	_mapSelectedInt[eFeature] = nSelectedValue;
// 	GfLogDebug("GfglFeatures::select(Int:%d, %d) : supp=%s, new=%d\n",
// 			   (int)eFeature, nSelectedValue, getSupported(eFeature) >= 0 ? "true" : "false",
// 			   _mapSelectedInt[eFeature]);
}

// Other services.
void* GfglFeatures::getProcAddress(const char* pszName)
{
	return SDL_GL_GetProcAddress(pszName);
}
