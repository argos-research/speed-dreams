/***************************************************************************

    file        : openglconfig.cpp
    created     : Fri Jun 3 12:52:07 CET 2004
    copyright   : (C) 2005 Bernhard Wymann
    email       : berniw@bluewin.ch
    version     : $Id: openglconfig.cpp 6158 2015-10-04 01:04:51Z torcs-ng $

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
             Open GL options menu
    @version	$Id: openglconfig.cpp 6158 2015-10-04 01:04:51Z torcs-ng $
*/

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <vector>
#include <sstream>

#include <portability.h>
#include <tgfclient.h>
#include <glfeatures.h>
#include <raceman.h>

#include "legacymenu.h"
#include "openglconfig.h"

				  
// Texture compression.
static const char *ATextureCompTexts[] =
	{GFSCR_ATT_TEXTURECOMPRESSION_DISABLED, GFSCR_ATT_TEXTURECOMPRESSION_ENABLED};
static const int NTextureComps =
	sizeof(ATextureCompTexts) / sizeof(ATextureCompTexts[0]);
static int NCurTextureCompIndex = 0;
static int TextureCompLabelId;
static int TextureCompLeftButtonId;
static int TextureCompRightButtonId;

// Bump Mapping.
static const char *ABumpMappingTexts[] =
	{GFSCR_ATT_BUMPMAPPING_DISABLED, GFSCR_ATT_BUMPMAPPING_ENABLED};
static const int NBumpMapping =
	sizeof(ABumpMappingTexts) / sizeof(ABumpMappingTexts[0]);
static int NCurBumpMappingIndex = 0;
static int BumpMappingLabelId;
static int BumpMappingLeftButtonId;
static int BumpMappingRightButtonId;

// Max texture size (WARNING: the order in the list is important, do not change).
static int AMaxTextureSizeTexts[] = {8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384};
static int NMaxTextureSizes = sizeof(AMaxTextureSizeTexts) / sizeof(AMaxTextureSizeTexts[0]);
static int NCurMaxTextureSizeIndex = 0;
static int MaxTextureSizeLabelId;

static const int NDefaultTextureSize = 64; // In case everything goes wrong.

// Multi-texturing.
static const char *AMultiTextureTexts[] =
	{GFSCR_ATT_MULTITEXTURING_DISABLED, GFSCR_ATT_MULTITEXTURING_ENABLED};
static const int NMultiTextures =
	sizeof(AMultiTextureTexts) / sizeof(AMultiTextureTexts[0]);
static int NCurMultiTextureIndex = 0;

static int MultiTextureLabelId;
static int MultiTextureLeftButtonId;
static int MultiTextureRightButtonId;

//Anisotropic-filtering
static const char *AAnisotropicFilteringTexts[] =
    {GFSCR_ATT_ANISOTROPICFILTERING_DISABLED, GFSCR_ATT_ANISOTROPICFILTERING_MEDIUM,GFSCR_ATT_ANISOTROPICFILTERING_HIGH};
static const int NAnisotropicFiltering =
    sizeof(AAnisotropicFilteringTexts) / sizeof(AAnisotropicFilteringTexts[0]);
static int NCurAnisotropicFilteringIndex = 0;

static int AnisotropicFilteringLabelId;
static int AnisotropicFilteringLeftButtonId;
static int AnisotropicFilteringRightButtonId;

/* list of available graphic engine */
static const int DefaultGraphicVersion = 1;
static const char *GraphicSchemeList[] = {RM_VAL_MOD_SSGRAPH, RM_VAL_MOD_OSGGRAPH};
static const char *GraphicDispNameList[] = 	{"ssggraph", "OsgGraph"};
static const int NbGraphicScheme = sizeof(GraphicSchemeList) / sizeof(GraphicSchemeList[0]);
static int CurGraphicScheme = DefaultGraphicVersion;


// Multi-sampling (initialized in OpenGLMenuInit).
static std::vector<std::string> VecMultiSampleTexts;
static int NMultiSamples = 0;
static int NCurMultiSampleIndex = 0;

static int MultiSampleLabelId;
static int MultiSampleLeftButtonId;
static int MultiSampleRightButtonId;

static int GraphicSchemeId;

// GUI screen handles.
static void	*ScrHandle = NULL;
static void	*PrevHandle = NULL;

static bool BMultiSamplingWasSelected = false;
static int BPrevMultiSamplingSamples = 0;

static void onAccept(void *)
{
	// Store current state of settings to the GL features layer.
	GfglFeatures::self().select(GfglFeatures::TextureCompression,
								 strcmp(ATextureCompTexts[NCurTextureCompIndex],
										GFSCR_ATT_TEXTURECOMPRESSION_ENABLED) ? false : true);
	GfglFeatures::self().select(GfglFeatures::TextureMaxSize,
								 AMaxTextureSizeTexts[NCurMaxTextureSizeIndex]);
	GfglFeatures::self().select(GfglFeatures::MultiTexturing,
								 strcmp(AMultiTextureTexts[NCurMultiTextureIndex],
										GFSCR_ATT_MULTITEXTURING_ENABLED) ? false : true);
	GfglFeatures::self().select(GfglFeatures::MultiSampling,
								VecMultiSampleTexts[NCurMultiSampleIndex]
								!= GFSCR_ATT_MULTISAMPLING_DISABLED);
	if (VecMultiSampleTexts[NCurMultiSampleIndex] != GFSCR_ATT_MULTISAMPLING_DISABLED)
		GfglFeatures::self().select(GfglFeatures::MultiSamplingSamples,
									(int)pow(2.0, (double)NCurMultiSampleIndex));

	GfglFeatures::self().select(GfglFeatures::BumpMapping, strcmp(ABumpMappingTexts[NCurBumpMappingIndex],
										GFSCR_ATT_BUMPMAPPING_ENABLED) ? false : true);


    GfglFeatures::self().select(GfglFeatures::AnisotropicFiltering, NCurAnisotropicFilteringIndex);

	// Store settings from the GL features layer to the screen.xml file.
	GfglFeatures::self().storeSelection();

	char buf[1024];
	snprintf(buf, sizeof(buf), "%s%s", GfLocalDir(), RACE_ENG_CFG);
	void *paramHandle = GfParmReadFile(buf, GFPARM_RMODE_REREAD | GFPARM_RMODE_CREAT);

	GfParmSetStr(paramHandle, RM_SECT_MODULES, RM_ATTR_MOD_GRAPHIC, GraphicSchemeList[CurGraphicScheme]);
	GfParmWriteFile(NULL, paramHandle, "raceengine");
	GfParmReleaseHandle(paramHandle);

	// Return to previous screen.
	GfuiScreenActivate(PrevHandle);

	// But actually restart the game if the multi-sampling feature settings changed
	// (we can't change this without re-initializing the video mode).
    if (GfglFeatures::self().isSelected(GfglFeatures::MultiSampling) != BMultiSamplingWasSelected
		|| GfglFeatures::self().getSelected(GfglFeatures::MultiSamplingSamples) != BPrevMultiSamplingSamples)
	{
		// Shutdown the user interface.
		LegacyMenu::self().shutdown();
		
		// Restart the game.
		GfuiApp().restart();
    }
}

// Toggle texture compression state enabled/disabled.
static void changeTextureCompressionState(void *vp)
{
	NCurTextureCompIndex = (NCurTextureCompIndex + (int)(long)vp + NTextureComps) % NTextureComps;
	GfuiLabelSetText(ScrHandle, TextureCompLabelId, ATextureCompTexts[NCurTextureCompIndex]);
}

// Toggle multi-texturing state enabled/disabled.
static void changeMultiTextureState(void *vp)
{
	NCurMultiTextureIndex = (NCurMultiTextureIndex + (int)(long)vp + NMultiTextures) % NMultiTextures;
	GfuiLabelSetText(ScrHandle, MultiTextureLabelId, AMultiTextureTexts[NCurMultiTextureIndex]);
}

// Toggle multi-sampling state enabled/disabled.
static void changeMultiSampleState(void *vp)
{
	NCurMultiSampleIndex = (NCurMultiSampleIndex + (int)(long)vp + NMultiSamples) % NMultiSamples;
	GfuiLabelSetText(ScrHandle, MultiSampleLabelId,
					 VecMultiSampleTexts[NCurMultiSampleIndex].c_str());
}


// Scroll through texture sizes smaller or equal the system limit.
static void changeMaxTextureSizeState(void *vp)
{
	char valuebuf[10];

	long delta = (long)vp;
	NCurMaxTextureSizeIndex += delta;
	if (NCurMaxTextureSizeIndex < 0)
	{
		NCurMaxTextureSizeIndex = NMaxTextureSizes - 1;
	} else if (NCurMaxTextureSizeIndex >= NMaxTextureSizes)
	{
		NCurMaxTextureSizeIndex= 0;
	}

	snprintf(valuebuf, sizeof(valuebuf), "%d", AMaxTextureSizeTexts[NCurMaxTextureSizeIndex]);
	GfuiLabelSetText(ScrHandle, MaxTextureSizeLabelId, valuebuf);
}

// Toggle bumpmapping state enabled/disabled.
static void changeBumpMappingState(void *vp)
{
	NCurBumpMappingIndex = (NCurBumpMappingIndex + (int)(long)vp + NBumpMapping) % NBumpMapping;
	GfuiLabelSetText(ScrHandle, BumpMappingLabelId, ABumpMappingTexts[NCurBumpMappingIndex]);
}

// Toggle anisotropic filtering state enabled/disabled.
static void changeAnisotropicFilteringState(void *vp)
{
    NCurAnisotropicFilteringIndex = (NCurAnisotropicFilteringIndex + (int)(long)vp + NAnisotropicFiltering) % NAnisotropicFiltering;
    GfuiLabelSetText(ScrHandle, AnisotropicFilteringLabelId, AAnisotropicFilteringTexts[NCurAnisotropicFilteringIndex]);
}

/* Change the graphc engine version (but only show really available modules) */
static void
onChangeGraphicVersion(void *vp)
{
	char buf[1024];

	if (!vp)
		return;

	const int oldGraphicVersion = CurGraphicScheme;
	do
	{
		CurGraphicScheme = (CurGraphicScheme + NbGraphicScheme + (int)(long)vp) % NbGraphicScheme;
	
		snprintf(buf, sizeof(buf), "%smodules/graphic/%s.%s", GfLibDir(), GraphicSchemeList[CurGraphicScheme], DLLEXT);
	}
	while (!GfFileExists(buf) && CurGraphicScheme != oldGraphicVersion);

	GfuiLabelSetText(ScrHandle, GraphicSchemeId, GraphicDispNameList[CurGraphicScheme]);
}



static void onActivate(void * /* dummy */)
{
	int i;
	char valuebuf[10];

	// Initialize current state and GUI from the GL features layer.
	// 1) Texture compression.
	if (GfglFeatures::self().isSupported(GfglFeatures::TextureCompression))
	{
		const char *pszTexComp =
			GfglFeatures::self().isSelected(GfglFeatures::TextureCompression)
			? GFSCR_ATT_TEXTURECOMPRESSION_ENABLED : GFSCR_ATT_TEXTURECOMPRESSION_DISABLED;
		for (i = 0; i < NTextureComps; i++)
		{
			if (!strcmp(pszTexComp, ATextureCompTexts[i]))
			{
				NCurTextureCompIndex = i;
				break;
			}
		}

		GfuiLabelSetText(ScrHandle, TextureCompLabelId,
						 ATextureCompTexts[NCurTextureCompIndex]);
	}
	else
	{
		GfuiEnable(ScrHandle, TextureCompLeftButtonId, GFUI_DISABLE);
		GfuiEnable(ScrHandle, TextureCompRightButtonId, GFUI_DISABLE);
		GfuiLabelSetText(ScrHandle, TextureCompLabelId, "Not supported");
	}

	// 2) Max texture size.
	int sizelimit =
		GfglFeatures::self().getSupported(GfglFeatures::TextureMaxSize);
	int tsize =
		GfglFeatures::self().getSelected(GfglFeatures::TextureMaxSize);

	int maxsizenb = 0;
	for (i = 0; i < NMaxTextureSizes; i++)
	{
		if (AMaxTextureSizeTexts[i] <= sizelimit)
		{
			maxsizenb = i;
		} else {
			break;
		}
	}

	// Limit to available sizes.
	NMaxTextureSizes = maxsizenb + 1;

	bool found = false;
	for (i = 0; i < NMaxTextureSizes; i++)
	{
		if (AMaxTextureSizeTexts[i] == tsize)
		{
			NCurMaxTextureSizeIndex = i;
			found = true;
			break;
		}
	}

	if (!found)
	{
		// Should never come here if there is no bug in OpenGL.
		tsize = NDefaultTextureSize;
		for (i = 0; i < NMaxTextureSizes; i++)
		{
			if (AMaxTextureSizeTexts[i] == tsize)
			{
				NCurMaxTextureSizeIndex = i;
				break;
			}
		}
	}
	
	snprintf(valuebuf, sizeof(valuebuf), "%d", AMaxTextureSizeTexts[NCurMaxTextureSizeIndex]);
	GfuiLabelSetText(ScrHandle, MaxTextureSizeLabelId, valuebuf);
	
	// 3) Multi-texturing.
	if (GfglFeatures::self().isSupported(GfglFeatures::MultiTexturing))
	{
		const char *pszMultiTex =
			GfglFeatures::self().isSelected(GfglFeatures::MultiTexturing)
			? GFSCR_ATT_MULTITEXTURING_ENABLED : GFSCR_ATT_MULTITEXTURING_DISABLED;
		for (i = 0; i < NMultiTextures; i++)
		{
			if (!strcmp(pszMultiTex, AMultiTextureTexts[i]))
			{
				NCurMultiTextureIndex = i;
				break;
			}
		}

		GfuiLabelSetText(ScrHandle, MultiTextureLabelId,
						 AMultiTextureTexts[NCurMultiTextureIndex]);
	}
	else
	{
		GfuiEnable(ScrHandle, MultiTextureLeftButtonId, GFUI_DISABLE);
		GfuiEnable(ScrHandle, MultiTextureRightButtonId, GFUI_DISABLE);
		GfuiLabelSetText(ScrHandle, MultiTextureLabelId, "Not supported");
	}
	
	// 4) Multi-sampling (anti-aliasing).
	if (GfglFeatures::self().isSupported(GfglFeatures::MultiSampling))
	{
		BMultiSamplingWasSelected =
			GfglFeatures::self().isSelected(GfglFeatures::MultiSampling);
		BPrevMultiSamplingSamples =
			GfglFeatures::self().getSelected(GfglFeatures::MultiSamplingSamples);

		if (!BMultiSamplingWasSelected)
			NCurMultiSampleIndex = 0;
		else
		{
			NCurMultiSampleIndex = 0;
			int nSamples = 1;
			while (nSamples < BPrevMultiSamplingSamples)
			{
				NCurMultiSampleIndex++;
				nSamples *= 2;
			}
		}

		GfuiLabelSetText(ScrHandle, MultiSampleLabelId,
						 VecMultiSampleTexts[NCurMultiSampleIndex].c_str());
	}
	else
	{
		GfuiEnable(ScrHandle, MultiSampleLeftButtonId, GFUI_DISABLE);
		GfuiEnable(ScrHandle, MultiSampleRightButtonId, GFUI_DISABLE);
		GfuiLabelSetText(ScrHandle, MultiSampleLabelId, "Not supported");
	}

	// Initialize current state and GUI from the GL features layer.
	// 6) Bump Mapping.
	if (GfglFeatures::self().isSupported(GfglFeatures::BumpMapping))
	{
		const char *pszBumpMapping =
			GfglFeatures::self().isSelected(GfglFeatures::BumpMapping)
			? GFSCR_ATT_BUMPMAPPING_ENABLED : GFSCR_ATT_BUMPMAPPING_DISABLED;
		for (i = 0; i < NBumpMapping; i++)
		{
			if (!strcmp(pszBumpMapping, ABumpMappingTexts[i]))
			{
				NCurBumpMappingIndex = i;
				break;
			}
		}

		GfuiLabelSetText(ScrHandle, BumpMappingLabelId,
						 ABumpMappingTexts[NCurBumpMappingIndex]);
	}
	else
	{
		GfuiEnable(ScrHandle, BumpMappingLeftButtonId, GFUI_DISABLE);
		GfuiEnable(ScrHandle, BumpMappingRightButtonId, GFUI_DISABLE);
		GfuiLabelSetText(ScrHandle, BumpMappingLabelId, "Not supported");
	}

    // Initialize current state and GUI from the GL features layer.
    // 7) Anisotropic Filtering.
    int ani_sup= GfglFeatures::self().getSupported(GfglFeatures::AnisotropicFiltering);
    if (ani_sup!=GfglFeatures::InvalidInt)
    {
        NCurAnisotropicFilteringIndex =
                GfglFeatures::self().getSelected(GfglFeatures::AnisotropicFiltering);
        GfuiLabelSetText(ScrHandle, AnisotropicFilteringLabelId,
                         AAnisotropicFilteringTexts[NCurAnisotropicFilteringIndex]);
    }
    else
    {
        GfuiEnable(ScrHandle, AnisotropicFilteringLeftButtonId, GFUI_DISABLE);
        GfuiEnable(ScrHandle, AnisotropicFilteringRightButtonId, GFUI_DISABLE);
        GfuiLabelSetText(ScrHandle, AnisotropicFilteringLabelId, "Not supported");
    }

	GfuiLabelSetText(ScrHandle, GraphicSchemeId, GraphicDispNameList[CurGraphicScheme]);
}

// OpenGL menu
void* OpenGLMenuInit(void *prevMenu)
{
	const char *graphicSchemeName;
	// Has screen already been created?
	if (ScrHandle)
		return ScrHandle;

	PrevHandle = prevMenu;

	ScrHandle = GfuiScreenCreate((float*)NULL, NULL, onActivate, NULL, (tfuiCallback)NULL, 1);
	void *hparmMenu = GfuiMenuLoad("opengloptionsmenu.xml");
	GfuiMenuCreateStaticControls(ScrHandle, hparmMenu);
	
	//Initialize current Graphic Engine
	// ssggraph / OsgGraph
	char buf[1024];
	snprintf(buf, sizeof(buf), "%s%s", GfLocalDir(), RACE_ENG_CFG);

	void *paramHandle = GfParmReadFile(buf, GFPARM_RMODE_REREAD | GFPARM_RMODE_CREAT);

	// graphic engine
	graphicSchemeName = GfParmGetStr(paramHandle, RM_SECT_MODULES, RM_ATTR_MOD_GRAPHIC, GraphicSchemeList[1]);
	for (int i = 0; i < NbGraphicScheme; i++) 
	{
		if (strcmp(graphicSchemeName, GraphicSchemeList[i]) == 0)
		{
			CurGraphicScheme = i;
			break;
		}
	}

	GfParmWriteFile(NULL, paramHandle, "raceengine");
	GfParmReleaseHandle(paramHandle);

	// Texture compression.
	TextureCompLeftButtonId =
		GfuiMenuCreateButtonControl(ScrHandle, hparmMenu, "TextureCompressionLeftArrowButton", (void*)-1,
							changeTextureCompressionState);
	TextureCompRightButtonId =
		GfuiMenuCreateButtonControl(ScrHandle, hparmMenu, "TextureCompressionRightArrowButton", (void*)+1,
							changeTextureCompressionState);
	TextureCompLabelId = GfuiMenuCreateLabelControl(ScrHandle,hparmMenu,"TextureCompressionLabel");

	// Texture sizing.
	GfuiMenuCreateButtonControl(ScrHandle,hparmMenu,"MaxTextureSizeLeftArrowButton", (void*)-1,
						changeMaxTextureSizeState);
	GfuiMenuCreateButtonControl(ScrHandle,hparmMenu,"MaxTextureSizeRightArrowButton", (void*)+1,
						changeMaxTextureSizeState);
	MaxTextureSizeLabelId = GfuiMenuCreateLabelControl(ScrHandle,hparmMenu,"MaxTextureSizeLabel");

	// Multi-texturing.
	MultiTextureLeftButtonId =
		GfuiMenuCreateButtonControl(ScrHandle, hparmMenu, "MultiTextureLeftArrowButton", (void*)-1,
							changeMultiTextureState);
	MultiTextureRightButtonId =
		GfuiMenuCreateButtonControl(ScrHandle, hparmMenu, "MultiTextureRightArrowButton", (void*)+1,
							changeMultiTextureState);
	MultiTextureLabelId = GfuiMenuCreateLabelControl(ScrHandle,hparmMenu,"MultiTextureLabel");

	// Multi-sampling.
	MultiSampleLeftButtonId =
		GfuiMenuCreateButtonControl(ScrHandle, hparmMenu, "MultiSampleLeftArrowButton", (void*)-1,
							changeMultiSampleState);
	MultiSampleRightButtonId =
		GfuiMenuCreateButtonControl(ScrHandle, hparmMenu, "MultiSampleRightArrowButton", (void*)+1,
							changeMultiSampleState);
	MultiSampleLabelId = GfuiMenuCreateLabelControl(ScrHandle,hparmMenu,"MultiSampleLabel");

	// Bump Mapping.
	BumpMappingLeftButtonId =
		GfuiMenuCreateButtonControl(ScrHandle, hparmMenu, "BumpMappingLeftArrowButton", (void*)-1,
							changeBumpMappingState);
	BumpMappingRightButtonId =
		GfuiMenuCreateButtonControl(ScrHandle, hparmMenu, "BumpMappingRightArrowButton", (void*)+1,
							changeBumpMappingState);
	BumpMappingLabelId = GfuiMenuCreateLabelControl(ScrHandle,hparmMenu,"BumpMappingLabel");

    // Anisotropic Filtering.
    AnisotropicFilteringLeftButtonId =
        GfuiMenuCreateButtonControl(ScrHandle, hparmMenu, "AnisotropicFilteringLeftArrowButton", (void*)-1,
                            changeAnisotropicFilteringState);
    AnisotropicFilteringRightButtonId =
        GfuiMenuCreateButtonControl(ScrHandle, hparmMenu, "AnisotropicFilteringRightArrowButton", (void*)+1,
                            changeAnisotropicFilteringState);
    AnisotropicFilteringLabelId = GfuiMenuCreateLabelControl(ScrHandle,hparmMenu,"AnisotropicFilteringLabel");

	GraphicSchemeId = GfuiMenuCreateLabelControl(ScrHandle, hparmMenu, "graphiclabel");
#ifndef OFFICIAL_ONLY
    GfuiMenuCreateButtonControl(ScrHandle, hparmMenu, "graphicleftarrow", (void*)-1, onChangeGraphicVersion);
    GfuiMenuCreateButtonControl(ScrHandle, hparmMenu, "graphicrightarrow", (void*)1, onChangeGraphicVersion);
#endif

	GfuiMenuCreateButtonControl(ScrHandle,hparmMenu,"ApplyButton",NULL, onAccept);
	GfuiMenuCreateButtonControl(ScrHandle,hparmMenu,"CancelButton",prevMenu, GfuiScreenActivate);

	GfParmReleaseHandle(hparmMenu);

	GfuiAddKey(ScrHandle, GFUIK_RETURN, "Apply", NULL, onAccept, NULL);
	GfuiAddKey(ScrHandle, GFUIK_ESCAPE, "Cancel", prevMenu, GfuiScreenActivate, NULL);
	GfuiAddKey(ScrHandle, GFUIK_F1, "Help", ScrHandle, GfuiHelpScreen, NULL);
	GfuiAddKey(ScrHandle, GFUIK_F12, "Screen-Shot", NULL, GfuiScreenShot, NULL);
	GfuiAddKey(ScrHandle, GFUIK_LEFT, "Decrease Texture Size Limit", (void*)-1, changeMaxTextureSizeState, NULL);
	GfuiAddKey(ScrHandle, GFUIK_RIGHT, "Increase Texture Size Limit", (void*)1, changeMaxTextureSizeState, NULL);
	GfuiAddKey(ScrHandle, ' ', "Toggle Texture Compression", (void*)1, changeTextureCompressionState, NULL);

	// Initialize multi-sampling levels.
	NMultiSamples = 1;
	VecMultiSampleTexts.push_back(GFSCR_ATT_MULTISAMPLING_DISABLED);
	if (GfglFeatures::self().isSupported(GfglFeatures::MultiSampling)
		&& GfglFeatures::self().getSupported(GfglFeatures::MultiSamplingSamples) > 1)
	{
		const int nMaxSamples =
			GfglFeatures::self().getSupported(GfglFeatures::MultiSamplingSamples);
		NMultiSamples += (int)(log((double)nMaxSamples) / log(2.0));
		std::ostringstream ossVal;
		for (int nVal = 2; nVal <= nMaxSamples; nVal *= 2)
		{
			ossVal.str("");
			ossVal << nVal << "x";
			VecMultiSampleTexts.push_back(ossVal.str());
		}
	}
	
	return ScrHandle;
}
