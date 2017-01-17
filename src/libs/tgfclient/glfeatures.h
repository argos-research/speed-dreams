/***************************************************************************

    file                 : glfeatures.h
    created              : Wed Jun 1 14:56:31 CET 2005
    copyright            : (C) 2005 by Bernhard Wymann
    version              : $Id: glfeatures.h 6285 2015-11-28 18:30:04Z beaglejoe $

***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _GLFEATURES_H_
#define _GLFEATURES_H_

// Multi-plateform Open GL includes : use this header files when calling OpenGL

#ifdef WIN32
#  include <windows.h>
#endif //WIN32

#ifdef __APPLE__
#  include <OpenGL/gl.h>
#  include <OpenGL/glu.h>
#  include <OpenGL/glext.h>
#else //__APPLE__
#  include <GL/gl.h>
#  include <GL/glu.h>
#  include <GL/glext.h>
#endif 

#include "tgfclient.h"


/* OpenGL features interface 

   Makes the difference between "selected", "supported" and "enabled" features :
   - "selected" means that the user choosed to use the feature
     (through the OpenGL option menu or the screen.xml file),
   - "supported" means that the underlying hardware/driver actually supports the feature,
   - "enabled" means that the feature is actually enabled in the underlying hardware/driver.
   
   GfglFeatures generally doesn't automatically select features : call select() for this
   (Exceptions: MultiTexturingUnits = all available ones, MultiSamplingSamples = max level).
   GfglFeatures doesn't enables features : not done here.
   
   A feature that is not supported can not be selected (or enabled).
   A feature that is selected is not necessarily enabled (not done here).
   Integer features must follow an "increasing order" : a value better value is greater,
   and thus a selected value can't be greater than a supported one.
*/

class TGFCLIENT_API GfglFeatures
{
 public:
	
	// Access to the unique instance.
	static GfglFeatures& self();

	// Check best supported OpenGL features, and store report to the config file
	// (default = GFSCR_CONF_FILE). May restart the game.
	bool checkBestSupport(int nWidth, int nHeight, int nDepth,
                          bool bAlpha, bool bFullScreen, bool bBump, bool bStereo,int nAniFilt, void* hparmConfig = 0);

	// Detect standard supported features. Don't restart the game.
	// Precondiftion: SDL_setVideoMode(...)
	void detectStandardSupport();

	// Dump detected supported features (in the current trace stream).
	void dumpSupport() const;
	
	// Load user-selected features from the config file (default = GFSCR_CONF_FILE).
	// Precondiftion: checkBestSupport() or checkStandardSupport().
	void loadSelection(void* hparmConfig = 0);
	
	// Store user-selected features to the config file (default = GFSCR_CONF_FILE).
	// Precondiftion: loadSelection()
	void storeSelection(void* hparmConfig = 0) const;
	
	// Dump user-selected features (in the current trace stream).
	void dumpSelection() const;

	// Dump info about the underlying hardware
	// Precondiftion: SDL_setVideoMode(...)
	void dumpHardwareInfo() const;

	// Bool-valued features.
	enum EFeatureBool
	{
		DoubleBuffer,
		TextureCompression, // GL_ARB_texture_compression
		TextureRectangle, // GL_ARB_texture_rectangle, in case mipmapping NOT needed.
		TextureNonPowerOf2, // GL_ARB_texture_non_power_of_two, in case mipmapping needed.
		MultiTexturing, // GL_ARB_multitexture
		MultiSampling, // GL_ARB_multisample
        BumpMapping,   // Bump Mapping
		StereoVision  // StereoVision
	};
	void select(EFeatureBool eFeature, bool bSelected);
	bool isSelected(EFeatureBool eFeature) const;
	bool isSupported(EFeatureBool eFeature) const;

	// Integer-valued features (use InvalidInt for the "not supported" / "not selected" cases).
	static int InvalidInt;
	enum EFeatureInt
	{
		ColorDepth, AlphaDepth,
		TextureMaxSize,
		MultiTexturingUnits,
        MultiSamplingSamples,
        AnisotropicFiltering
	};
	void select(EFeatureInt eFeature, int nSelectedValue);
	int getSelected(EFeatureInt eFeature) const;
	int getSupported(EFeatureInt eFeature) const;

	// Get the pointer to the named OpenGL extension function.
	static void* getProcAddress(const char* pszName);
	
 private:
	
	//! Singleton pattern => private constructor.
	GfglFeatures();

	// Update supported OpenGL features according to the given frame buffer specs.
    bool detectBestSupport(int& nWidth, int& nHeight, int& nDepth,
                           bool& bAlpha, bool& bBump, bool& bStereo, bool& bFullScreen, int& nAniFilt);
#if SDL_MAJOR_VERSION >= 2
	bool detectBestSupportSDL2(int& nWidth, int& nHeight, int& nDepth,
                           bool& bAlpha, bool& bBump, bool& bStereo, bool& bFullScreen, int& nAniFilt);
#endif

	bool loadSupport(int &nWidth, int &nHeight, int &nDepth,
                     bool &bAlpha, bool &bFullScreen, bool &bBump, bool &bStereo, int &nAniFilt, void* hparmConfig = 0);

	void storeSupport(int nWidth, int nHeight, int nDepth,
                      bool bAlpha, bool bFullScreen, bool bBump, bool bStereo,int nAniFilt, void* hparmConfig = 0);

	static void* openConfigFile();
	static void closeConfigFile(void* hparmConfig, bool bWrite = false);
	
 private:

	//! The unique instance (singleton pattern).
	static GfglFeatures* _pSelf;

	//! The config files params pointer.
	//void* hparmConfig;
	
	//! Maps of supported features (bool and int-valued).
	std::map<EFeatureBool, bool> _mapSupportedBool;
	std::map<EFeatureInt, int>   _mapSupportedInt;

	//! Maps of selected features (bool and int-valued).
	std::map<EFeatureBool, bool> _mapSelectedBool;
	std::map<EFeatureInt, int>   _mapSelectedInt;
};

#endif // _GLFEATURES_H_

