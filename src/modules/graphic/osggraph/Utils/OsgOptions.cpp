/***************************************************************************

    file                     : OsgOptions.cpp
    created                  : Thu Mar 31 00:00:41 CEST 2015
    copyright                : (C) 2015 by Xavier Bertaux
    email                    : bertauxx@yahoo.fr
    version                  : $Id: OsgOptions.cpp 5940 2015-04-01 03:12:09Z torcs-ng $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <stdlib.h>

#include "OsgMain.h"
#include "OsgOptions.h"

#define MAX_BODIES	2
#define MAX_CLOUDS	3
#define NMaxStars	3000
#define NPLANETS		0       //No planets displayed
#define NB_BG_FACES	36          //Background faces
#define BG_DIST			1.0f
#define SKYDYNAMIC_THR	12000	//Skydynamic setting threshold. No dynamic sky below that.
#define CLEAR_CLOUD 1
#define MORE_CLOUD 6
#define SCARCE_CLOUD 5
#define COVERAGE_CLOUD 8

static const char* ShadowValues[] = { GR_ATT_SHADOW_NONE, GR_ATT_SHADOW_SM, GR_ATT_SHADOW_SSM, GR_ATT_SHADOW_PSSM, GR_ATT_SHADOW_LSPM, GR_ATT_SHADOW_VOLUME, GR_ATT_SHADOW_VDSM};
static const int NbShadowValues = sizeof(ShadowValues) / sizeof(ShadowValues[0]);
static const char* TexSizeValues[] = { GR_ATT_SHADOW_512, GR_ATT_SHADOW_1024, GR_ATT_SHADOW_2048, GR_ATT_SHADOW_4096, GR_ATT_SHADOW_8192 };
static const int NbTexSizeValues = sizeof(TexSizeValues) / sizeof(TexSizeValues[0]);
static const char* QualityValues[] = { GR_ATT_AGR_LITTLE, GR_ATT_AGR_MEDIUM, GR_ATT_AGR_FULL };
static const int NbQualityValues = sizeof(QualityValues) / sizeof(QualityValues[0]);
static const char* ShadersValues[] = { GR_VAL_NO, GR_VAL_YES };
static const int NbShadersValues = sizeof(ShadersValues) / sizeof(ShadersValues[0]);
static const int CloudsTextureIndices[TR_CLOUDS_FULL+1] = {1, 3, 5, 7, 8};
static const int NCloudsTextureIndices = sizeof(CloudsTextureIndices) / sizeof(int);

SDOptions::SDOptions(void) :
    _SmokeValue(0),
    _SmokeDuration(0),
    _SmokeInterval(0),

    _SkidValue(0),
    _SkidLength(0),
    _SkidInterval(0),

    _LOD(0),

    _bgsky(true),
    _DynamicSkyDome(0),
    _SkyDomeDistance(20000),
    _Max_Visibility(10000),
    _PrecipitationDensity(100),
    _Rain(0),

    _CloudLayer(1),

    _SceneLOD(0),
    _Cockpit3D(false),

    _ShadowType(0),
    _ShadowSize(1024),
    _ShadowQuality(0),

    _Shaders(0),
    _ShadersQuality(0),
    _ShadersSize(256),

    _NormalMap(false)
{
}

SDOptions::~SDOptions(void)
{
}
