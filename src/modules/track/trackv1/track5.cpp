/***************************************************************************

    file                 : track5.cpp
    created              : Sat May 18 12:46:26 CEST 2002
    copyright            : (C) 2002 by Eric Espie
    email                : eric.espie@torcs.org
    version              : $Id: track5.cpp 6394 2016-03-29 18:27:45Z wdbee $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <cstdlib>
#include <cmath>
#include <cstdio>
#include <cstring>

#include <tgf.h>
#include <portability.h>
#include <robottools.h>
#include <track.h>

#include "trackinc.h"

//TODO Joe
// Move to D:\source\speed-dreams\src\interfaces\track.h
#define TRK_ATT_START_BEARING  "original bearing"
#define TRK_ATT_START_X  "start X"
#define TRK_ATT_START_Y  "start Y"
#define TRK_ATT_START_Z  "start Z"
#define TRK_ATT_MIN_X  "minimum X"
#define TRK_ATT_MIN_Y  "minimum Y"
#define TRK_ATT_MIN_Z  "minimum Z"
#define TRK_ATT_EXTENT_X  "extent X"
#define TRK_ATT_EXTENT_Y  "extent Y"
#define TRK_ATT_EXTENT_Z  "extent Z"

static tdble	xmin, xmax, ymin, ymax, zmin, zmax;

/*
 * Sides global variables
 */
static const char *SectSide[2]    = {TRK_SECT_RSIDE, TRK_SECT_LSIDE};
static const char *SectBorder[2]  = {TRK_SECT_RBORDER, TRK_SECT_LBORDER};
static const char *SectBarrier[2] = {TRK_SECT_RBARRIER, TRK_SECT_LBARRIER};

static const char *ValStyle[] = {TRK_VAL_PLAN, TRK_VAL_CURB, TRK_VAL_WALL, TRK_VAL_FENCE, TRK_VAL_FENCE};


static tdble sideEndWidth[2];
static tdble sideStartWidth[2];
static int sideBankType[2];
static const char *sideMaterial[2];
static tTrackSurface *sideSurface[2];


static int envIndex;
static tdble DoVfactor=1.0;

static tdble borderWidth[2];
static tdble borderHeight[2];
static int borderStyle[2];
static const char *borderMaterial[2];
static tTrackSurface *borderSurface[2];

static tdble barrierWidth[2];
static tdble barrierHeight[2];
static int barrierStyle[2];
static const char *barrierMaterial[2];
static tTrackSurface *barrierSurface[2];

// Pits variables
static tTrackSeg	*pitEntrySeg = NULL;
static tTrackSeg	*pitExitSeg = NULL;
static tTrackSeg	*pitStart = NULL;
static tTrackSeg	*pitBuildingsStart = NULL;
static tTrackSeg	*pitBuildingsEnd = NULL;
static tTrackSeg	*pitEnd = NULL;

static tTrackPitInfo	*pits = NULL;

static tdble	GlobalStepLen = 0;
static char	path[256];
static char	path2[256];


//inline void TSTX(tdble x) { xmin = MIN(xmin, x); xmax = MAX(xmax, x); }
//inline void TSTY(tdble y) { ymin = MIN(ymin, y); ymax = MAX(ymax, y); }
//inline void TSTZ(tdble z) { zmin = MIN(zmin, z); zmax = MAX(zmax, z); }

#define TSTX(x)			\
    if (xmin > (x)) xmin = (x);	\
    if (xmax < (x)) xmax = (x);

#define TSTY(y)			\
    if (ymin > (y)) ymin = (y);	\
    if (ymax < (y)) ymax = (y);

#define TSTZ(z)			\
    if (zmin > (z)) zmin = (z);	\
    if (zmax < (z)) zmax = (z);


static tTrackSurface*
AddTrackSurface(void *TrackHandle, tTrack *theTrack, const char *material)
{
    tTrackSurface	*curSurf;

    /* search within existing surfaces */
    curSurf = theTrack->surfaces;
    while (curSurf) {
    if (strcmp(curSurf->material, material) == 0) {
        return curSurf;
    }
    curSurf = curSurf->next;
    }

    /* Create a new surface */
    curSurf = (tTrackSurface*)malloc(sizeof(tTrackSurface));
    if (!curSurf) {
    GfFatal("AddTrackSurface: Memory allocation failed\n");
    }

    curSurf->material = material;
    sprintf(path, "%s/%.*s", TRK_SECT_SURFACES,
    (int)(sizeof(path) - strlen(TRK_SECT_SURFACES)),
    material);
    curSurf->kFriction     = curSurf->kFrictionDry =
        GfParmGetNum(TrackHandle, path, TRK_ATT_FRICTION, (char*)NULL, 0.8f);
    curSurf->kRollRes      = GfParmGetNum(TrackHandle, path, TRK_ATT_ROLLRES, (char*)NULL, 0.001f);
    curSurf->kRoughness    = GfParmGetNum(TrackHandle, path, TRK_ATT_ROUGHT, (char*)NULL, 0.0f) /  2.0f;
    curSurf->kRoughWaveLen = (tdble)(2.0 * PI / GfParmGetNum(TrackHandle, path, TRK_ATT_ROUGHTWL, (char*)NULL, 1.0f));
    curSurf->kDammage      = GfParmGetNum(TrackHandle, path, TRK_ATT_DAMMAGE, (char*)NULL, 10.0f);
    curSurf->kRebound      = GfParmGetNum(TrackHandle, path, TRK_ATT_REBOUND, (char*)NULL, 0.5f);

    //GfLogDebug("                      %.4f | %s\n",
    //		   curSurf->kFrictionDry, curSurf->material);

    curSurf->next = theTrack->surfaces;
    theTrack->surfaces = curSurf;

    return curSurf;
}


static void
InitSides(void *TrackHandle, tTrack *theTrack)
{
    int 	side;
    const char 	*style;
    static char	path[256];

    for (side = 0; side < 2; side++) {
    /* Sides */
    sprintf(path, "%s/%s", TRK_SECT_MAIN, SectSide[side]);
    sideMaterial[side] = GfParmGetStr(TrackHandle, path, TRK_ATT_SURF, TRK_VAL_GRASS);
    sideSurface[side] = AddTrackSurface(TrackHandle, theTrack, sideMaterial[side]);
    sideEndWidth[side] = GfParmGetNum(TrackHandle, path, TRK_ATT_WIDTH, (char*)NULL, 0.0);
    /* banking of sides */
    if (strcmp(TRK_VAL_LEVEL, GfParmGetStr(TrackHandle, path, TRK_ATT_BANKTYPE, TRK_VAL_LEVEL)) == 0) {
        sideBankType[side] = 0;
    } else {
        sideBankType[side] = 1;
    }

    /* Borders */
    sprintf(path, "%s/%s", TRK_SECT_MAIN, SectBorder[side]);
    borderMaterial[side] = GfParmGetStr(TrackHandle, path, TRK_ATT_SURF, TRK_VAL_GRASS);
    borderSurface[side] = AddTrackSurface(TrackHandle, theTrack, borderMaterial[side]);
    borderWidth[side] = GfParmGetNum(TrackHandle, path, TRK_ATT_WIDTH, (char*)NULL, 0.0);
    borderHeight[side] = GfParmGetNum(TrackHandle, path, TRK_ATT_HEIGHT, (char*)NULL, 0.0);
    style = GfParmGetStr(TrackHandle, path, TRK_ATT_STYLE, TRK_VAL_PLAN);
    if (strcmp(style, TRK_VAL_PLAN) == 0) {
        borderStyle[side] = TR_PLAN;
    } else if (strcmp(style, TRK_VAL_CURB) == 0) {
        borderStyle[side] = TR_CURB;
    } else {
        borderStyle[side] = TR_WALL;
    }

    /* Barrier parameters */
    sprintf(path, "%s/%s", TRK_SECT_MAIN, SectBarrier[side]);
    barrierMaterial[side] = GfParmGetStr(TrackHandle, path, TRK_ATT_SURF, TRK_VAL_BARRIER);
    barrierSurface[side] = AddTrackSurface(TrackHandle, theTrack, barrierMaterial[side]);
    barrierHeight[side] = GfParmGetNum(TrackHandle, path, TRK_ATT_HEIGHT, (char*)NULL, 0.6f);
    style = GfParmGetStr(TrackHandle, path, TRK_ATT_STYLE, TRK_VAL_FENCE);
    if (strcmp(style, TRK_VAL_FENCE) == 0) {
        barrierStyle[side] = TR_FENCE;
        barrierWidth[side] = 0;
    } else {
        barrierStyle[side] = TR_WALL;
        barrierWidth[side] = GfParmGetNum(TrackHandle, path, TRK_ATT_WIDTH, (char*)NULL, 0.5);
    }
    }
}


static void
AddSides(tTrackSeg *curSeg, void *TrackHandle, tTrack *theTrack, int curStep, int steps)
{
    tTrackSeg	*curSide;
    tTrackSeg	*mSeg;
    tTrackSeg	*curBorder;
    tTrackBarrier *curBarrier;
    tdble	x, y, z;
    tdble	al, alfl;
    int		j;
    tdble	x1, x2, y1, y2;
    tdble	w, sw, ew, bw;
    tdble	minWidth;
    tdble	maxWidth;
    int		type;
    int		side;
    const char	*style;
    tdble	Kew;
    static char	path[256];
    static char	path2[256];
    char	*segName;

    x = y = z = 0;
    mSeg = curSeg;

    sprintf(path, "%s/%s", TRK_SECT_MAIN, TRK_LST_SEGMENTS);
    segName = GfParmListGetCurEltName(TrackHandle, path);
    sprintf(path, "%s/%s/%.*s", TRK_SECT_MAIN, TRK_LST_SEGMENTS,
    (int)(sizeof(path) - strlen(TRK_SECT_MAIN) - strlen(TRK_LST_SEGMENTS)),
    segName);
    for (side = 0; side < 2; side++) {
    curSeg = mSeg;
    if (curStep == 0) {
        /* Side parameters */
        sprintf(path2, "%s/%s", path, SectSide[side]);
        sw = GfParmGetNum(TrackHandle, path2, TRK_ATT_SWIDTH, (char*)NULL, sideEndWidth[side]);
        w = GfParmGetNum(TrackHandle, path2, TRK_ATT_WIDTH, (char*)NULL, sw);
        ew = GfParmGetNum(TrackHandle, path2, TRK_ATT_EWIDTH, (char*)NULL, w);
        sideStartWidth[side] = sw;
        sideEndWidth[side] = ew;
        sideMaterial[side] = GfParmGetStr(TrackHandle, path2, TRK_ATT_SURF, sideMaterial[side]);
        sideSurface[side] = AddTrackSurface(TrackHandle, theTrack, sideMaterial[side]);

        /* Border parameters */
        sprintf(path2, "%s/%s", path, SectBorder[side]);
        bw = GfParmGetNum(TrackHandle, path2, TRK_ATT_WIDTH, (char*)NULL, borderWidth[side]);
        borderWidth[side] = bw;
        borderHeight[side] = GfParmGetNum(TrackHandle, path2, TRK_ATT_HEIGHT, (char*)NULL, 0.0);
        borderMaterial[side] = GfParmGetStr(TrackHandle, path2, TRK_ATT_SURF, borderMaterial[side]);
        borderSurface[side] = AddTrackSurface(TrackHandle, theTrack, borderMaterial[side]);
        style = GfParmGetStr(TrackHandle, path2, TRK_ATT_STYLE, ValStyle[borderStyle[side]]);
        if (strcmp(style, TRK_VAL_PLAN) == 0) {
        borderStyle[side] = TR_PLAN;
        } else if (strcmp(style, TRK_VAL_CURB) == 0) {
        borderStyle[side] = TR_CURB;
        } else {
        borderStyle[side] = TR_WALL;
        }

        /* Barrier parameters */
        sprintf(path2, "%s/%s", path, SectBarrier[side]);
        barrierMaterial[side] = GfParmGetStr(TrackHandle, path2, TRK_ATT_SURF, barrierMaterial[side]);
        barrierSurface[side] = AddTrackSurface(TrackHandle, theTrack, barrierMaterial[side]);
        barrierHeight[side] = GfParmGetNum(TrackHandle, path2, TRK_ATT_HEIGHT, (char*)NULL, barrierHeight[side]);
        style = GfParmGetStr(TrackHandle, path2, TRK_ATT_STYLE, ValStyle[barrierStyle[side]]);
        if (strcmp(style, TRK_VAL_FENCE) == 0) {
        barrierStyle[side] = TR_FENCE;
        barrierWidth[side] = 0;
        } else {
        barrierStyle[side] = TR_WALL;
        barrierWidth[side] = GfParmGetNum(TrackHandle, path2, TRK_ATT_WIDTH, (char*)NULL, barrierWidth[side]);
        }
    } else {
        sw = sideStartWidth[side];
        ew = sideEndWidth[side];
        bw = borderWidth[side];
    }
    Kew = (ew - sw) / (tdble)steps;
    ew = sw + (tdble)(curStep+1) * Kew;
    sw = sw + (tdble)(curStep) * Kew;

    /* Borders */
    if (bw != 0.0) {
        curBorder = (tTrackSeg*)calloc(1, sizeof(tTrackSeg));
        if (side == 1) {
        curSeg->lside = curBorder;
        curBorder->vertex[TR_SR] = curSeg->vertex[TR_SL];
        curBorder->vertex[TR_ER] = curSeg->vertex[TR_EL];
        curBorder->type2 = TR_LBORDER;
        } else {
        curSeg->rside = curBorder;
        curBorder->vertex[TR_SL] = curSeg->vertex[TR_SR];
        curBorder->vertex[TR_EL] = curSeg->vertex[TR_ER];
        curBorder->type2 = TR_RBORDER;
        }

        type = sideBankType[side];
        curBorder->startWidth = bw;
        curBorder->endWidth = bw;
        curBorder->width = bw;
        curBorder->type = curSeg->type;
        curBorder->surface = borderSurface[side];
        curBorder->height = borderHeight[side];
        curBorder->style = borderStyle[side];
        curBorder->envIndex = envIndex;
        curBorder->DoVfactor = DoVfactor;
        curBorder->angle[TR_XS] = curSeg->angle[TR_XS] * (tdble)type;
        curBorder->angle[TR_XE] = curSeg->angle[TR_XE] * (tdble)type;
        curBorder->angle[TR_ZS] = curSeg->angle[TR_ZS];
        curBorder->angle[TR_ZE] = curSeg->angle[TR_ZE];
        curBorder->angle[TR_CS] = curSeg->angle[TR_CS];

        switch(curSeg->type) {
        case TR_STR:
        curBorder->length = curSeg->length;
        curBorder->cos = curSeg->cos;
        curBorder->sin = curSeg->sin;

        switch(side) {
        case 1:
            curBorder->vertex[TR_SL].x = curBorder->vertex[TR_SR].x + bw * curSeg->rgtSideNormal.x;
            curBorder->vertex[TR_SL].y = curBorder->vertex[TR_SR].y + bw * curSeg->rgtSideNormal.y;
            curBorder->vertex[TR_SL].z = curBorder->vertex[TR_SR].z + (tdble)type * bw * tan(curSeg->angle[TR_XS]);
            x = curBorder->vertex[TR_EL].x = curBorder->vertex[TR_ER].x + bw * curSeg->rgtSideNormal.x;
            y = curBorder->vertex[TR_EL].y = curBorder->vertex[TR_ER].y + bw * curSeg->rgtSideNormal.y;
            z = curBorder->vertex[TR_EL].z = curBorder->vertex[TR_ER].z + (tdble)type * bw * tan(curSeg->angle[TR_XE]);
            break;
        case 0:
            curBorder->vertex[TR_SR].x = curBorder->vertex[TR_SL].x - bw * curSeg->rgtSideNormal.x;
            curBorder->vertex[TR_SR].y = curBorder->vertex[TR_SL].y - bw * curSeg->rgtSideNormal.y;
            curBorder->vertex[TR_SR].z = curBorder->vertex[TR_SL].z - (tdble)type * bw * tan(curSeg->angle[TR_XS]);
            x = curBorder->vertex[TR_ER].x = curBorder->vertex[TR_EL].x - bw * curSeg->rgtSideNormal.x;
            y = curBorder->vertex[TR_ER].y = curBorder->vertex[TR_EL].y - bw * curSeg->rgtSideNormal.y;
            z = curBorder->vertex[TR_ER].z = curBorder->vertex[TR_EL].z - (tdble)type * bw * tan(curSeg->angle[TR_XE]);
            break;
        }
        curBorder->angle[TR_YR] = atan2(curBorder->vertex[TR_ER].z - curBorder->vertex[TR_SR].z, curBorder->length);
        curBorder->angle[TR_YL] = atan2(curBorder->vertex[TR_EL].z - curBorder->vertex[TR_SL].z, curBorder->length);

        curBorder->Kzl = tan(curBorder->angle[TR_YR]);
        curBorder->Kzw = (curBorder->angle[TR_XE] - curBorder->angle[TR_XS]) / curBorder->length;
        curBorder->Kyl = 0;

        curBorder->rgtSideNormal.x = curSeg->rgtSideNormal.x;
        curBorder->rgtSideNormal.y = curSeg->rgtSideNormal.y;

        TSTX(x);
        TSTY(y);
        TSTZ(z);
        break;

        case TR_LFT:
        curBorder->center.x = curSeg->center.x;
        curBorder->center.y = curSeg->center.y;

        switch(side) {
        case 1:
            curBorder->radius = (tdble)(curSeg->radiusl - bw / 2.0);
            curBorder->radiusr = curSeg->radiusl;
            curBorder->radiusl = curSeg->radiusl - bw;
            curBorder->arc = curSeg->arc;
            curBorder->length = curBorder->radius * curBorder->arc;

            curBorder->vertex[TR_SL].x = curBorder->vertex[TR_SR].x - bw * cos(curBorder->angle[TR_CS]);
            curBorder->vertex[TR_SL].y = curBorder->vertex[TR_SR].y - bw * sin(curBorder->angle[TR_CS]);
            curBorder->vertex[TR_SL].z = curBorder->vertex[TR_SR].z + (tdble)type * bw * tan(curSeg->angle[TR_XS]);
            curBorder->vertex[TR_EL].x = curBorder->vertex[TR_ER].x - bw * cos(curBorder->angle[TR_CS] + curBorder->arc);
            curBorder->vertex[TR_EL].y = curBorder->vertex[TR_ER].y - bw * sin(curBorder->angle[TR_CS] + curBorder->arc);
            z = curBorder->vertex[TR_EL].z = curBorder->vertex[TR_ER].z + (tdble)type * bw * tan(curSeg->angle[TR_XE]);

            curBorder->angle[TR_YR] = atan2(curBorder->vertex[TR_ER].z - curBorder->vertex[TR_SR].z,
                            curBorder->arc * curBorder->radiusr);
            curBorder->angle[TR_YL] = atan2(curBorder->vertex[TR_EL].z - curBorder->vertex[TR_SL].z,
                            curBorder->arc * curBorder->radiusl);

            curBorder->Kzl = tan(curBorder->angle[TR_YR]) * curBorder->radiusr;
            curBorder->Kzw = (curBorder->angle[TR_XE] - curBorder->angle[TR_XS]) / curBorder->arc;
            curBorder->Kyl = 0;

            /* to find the boundary */
            al = (tdble)(curBorder->arc / 36.0);
            alfl = curBorder->angle[TR_CS];

            for (j = 0; j < 36; j++) {
            alfl += al;
            x1 = curBorder->center.x + (curBorder->radiusl) * cos(alfl);   /* location of end */
            y1 = curBorder->center.y + (curBorder->radiusl) * sin(alfl);
            TSTX(x1);
            TSTY(y1);
            }
            TSTZ(z);
            break;

        case 0:
            curBorder->radius = (tdble)(curSeg->radiusr + bw / 2.0);
            curBorder->radiusl = curSeg->radiusr;
            curBorder->radiusr = curSeg->radiusr + bw;
            curBorder->arc = curSeg->arc;
            curBorder->length = curBorder->radius * curBorder->arc;

            curBorder->vertex[TR_SR].x = curBorder->vertex[TR_SL].x + bw * cos(curBorder->angle[TR_CS]);
            curBorder->vertex[TR_SR].y = curBorder->vertex[TR_SL].y + bw * sin(curBorder->angle[TR_CS]);
            curBorder->vertex[TR_SR].z = curBorder->vertex[TR_SL].z - (tdble)type * bw * tan(curSeg->angle[TR_XS]);
            curBorder->vertex[TR_ER].x = curBorder->vertex[TR_EL].x + bw * cos(curBorder->angle[TR_CS] + curBorder->arc);
            curBorder->vertex[TR_ER].y = curBorder->vertex[TR_EL].y + bw * sin(curBorder->angle[TR_CS] + curBorder->arc);
            z = curBorder->vertex[TR_ER].z = curBorder->vertex[TR_EL].z - (tdble)type * bw * tan(curSeg->angle[TR_XE]);

            curBorder->angle[TR_YR] = atan2(curBorder->vertex[TR_ER].z - curBorder->vertex[TR_SR].z,
                            curBorder->arc * curBorder->radiusr);
            curBorder->angle[TR_YL] = atan2(curBorder->vertex[TR_EL].z - curBorder->vertex[TR_SL].z,
                            curBorder->arc * curBorder->radiusl);

            curBorder->Kzl = tan(curBorder->angle[TR_YR]) * (curBorder->radiusr);
            curBorder->Kzw = (curBorder->angle[TR_XE] - curBorder->angle[TR_XS]) / curBorder->arc;
            curBorder->Kyl = 0;

            /* to find the boundary */
            al = (tdble)(curBorder->arc / 36.0);
            alfl = curBorder->angle[TR_CS];

            for (j = 0; j < 36; j++) {
            alfl += al;
            x2 = curBorder->center.x + (curBorder->radiusr) * cos(alfl);   /* location of end */
            y2 = curBorder->center.y + (curBorder->radiusr) * sin(alfl);
            TSTX(x2);
            TSTY(y2);
            }
            TSTZ(z);
            break;

        }
        break;
        case TR_RGT:
        curBorder->center.x = curSeg->center.x;
        curBorder->center.y = curSeg->center.y;

        switch(side) {
        case 1:
            curBorder->radius = (tdble)(curSeg->radiusl + bw / 2.0);
            curBorder->radiusr = curSeg->radiusl;
            curBorder->radiusl = curSeg->radiusl + bw;
            curBorder->arc = curSeg->arc;
            curBorder->length = curBorder->radius * curBorder->arc;

            curBorder->vertex[TR_SL].x = curBorder->vertex[TR_SR].x + bw * cos(curBorder->angle[TR_CS]);
            curBorder->vertex[TR_SL].y = curBorder->vertex[TR_SR].y + bw * sin(curBorder->angle[TR_CS]);
            curBorder->vertex[TR_SL].z = curBorder->vertex[TR_SR].z + (tdble)type * bw * tan(curSeg->angle[TR_XS]);
            curBorder->vertex[TR_EL].x = curBorder->vertex[TR_ER].x + bw * cos(curBorder->angle[TR_CS] - curBorder->arc);
            curBorder->vertex[TR_EL].y = curBorder->vertex[TR_ER].y + bw * sin(curBorder->angle[TR_CS] - curBorder->arc);
            z = curBorder->vertex[TR_EL].z = curBorder->vertex[TR_ER].z + (tdble)type * bw * tan(curSeg->angle[TR_XE]);

            curBorder->angle[TR_YR] = atan2(curBorder->vertex[TR_ER].z - curBorder->vertex[TR_SR].z,
                            curBorder->arc * curBorder->radiusr);
            curBorder->angle[TR_YL] = atan2(curBorder->vertex[TR_EL].z - curBorder->vertex[TR_SL].z,
                            curBorder->arc * curBorder->radiusl);

            curBorder->Kzl = tan(curBorder->angle[TR_YR]) * curBorder->radiusr;
            curBorder->Kzw = (curBorder->angle[TR_XE] - curBorder->angle[TR_XS]) / curBorder->arc;
            curBorder->Kyl = 0;

            /* to find the boundary */
            al = (tdble)(curBorder->arc / 36.0);
            alfl = curBorder->angle[TR_CS];

            for (j = 0; j < 36; j++) {
            alfl -= al;
            x1 = curBorder->center.x + (curBorder->radiusl) * cos(alfl);   /* location of end */
            y1 = curBorder->center.y + (curBorder->radiusl) * sin(alfl);
            TSTX(x1);
            TSTY(y1);
            }
            TSTZ(z);
            break;

        case 0:
            curBorder->radius = (tdble)(curSeg->radiusr - bw / 2.0);
            curBorder->radiusl = curSeg->radiusr;
            curBorder->radiusr = curSeg->radiusr - bw;
            curBorder->arc = curSeg->arc;
            curBorder->length = curBorder->radius * curBorder->arc;

            curBorder->vertex[TR_SR].x = curBorder->vertex[TR_SL].x - bw * cos(curBorder->angle[TR_CS]);
            curBorder->vertex[TR_SR].y = curBorder->vertex[TR_SL].y - bw * sin(curBorder->angle[TR_CS]);
            curBorder->vertex[TR_SR].z = curBorder->vertex[TR_SL].z - (tdble)type * bw * tan(curSeg->angle[TR_XS]);
            curBorder->vertex[TR_ER].x = curBorder->vertex[TR_EL].x - bw * cos(curBorder->angle[TR_CS] - curBorder->arc);
            curBorder->vertex[TR_ER].y = curBorder->vertex[TR_EL].y - bw * sin(curBorder->angle[TR_CS] - curBorder->arc);
            z = curBorder->vertex[TR_ER].z = curBorder->vertex[TR_EL].z - (tdble)type * bw * tan(curSeg->angle[TR_XE]);

            curBorder->angle[TR_YR] = atan2(curBorder->vertex[TR_ER].z - curBorder->vertex[TR_SR].z,
                            curBorder->arc * curBorder->radiusr);
            curBorder->angle[TR_YL] = atan2(curBorder->vertex[TR_EL].z - curBorder->vertex[TR_SL].z,
                            curBorder->arc * curBorder->radiusl);

            curBorder->Kzl = tan(curBorder->angle[TR_YR]) * (curBorder->radiusr);
            curBorder->Kzw = (curBorder->angle[TR_XE] - curBorder->angle[TR_XS]) / curBorder->arc;
            curBorder->Kyl = 0;

            /* to find the boundary */
            al = (tdble)(curBorder->arc / 36.0);
            alfl = curBorder->angle[TR_CS];

            for (j = 0; j < 36; j++) {
            alfl -= al;
            x2 = curBorder->center.x + (curBorder->radiusr) * cos(alfl);   /* location of end */
            y2 = curBorder->center.y + (curBorder->radiusr) * sin(alfl);
            TSTX(x2);
            TSTY(y2);
            }
            TSTZ(z);
            break;
        }
        break;
        }

        curSeg = curBorder;
    }


    /* Sides */
    if ((sw != 0.0) || (ew != 0)) {
        curSide = (tTrackSeg*)calloc(1, sizeof(tTrackSeg));
        if (side == 1) {
        curSeg->lside = curSide;
        curSide->vertex[TR_SR] = curSeg->vertex[TR_SL];
        curSide->vertex[TR_ER] = curSeg->vertex[TR_EL];
        curSide->type2 = TR_LSIDE;
        } else {
        curSeg->rside = curSide;
        curSide->vertex[TR_SL] = curSeg->vertex[TR_SR];
        curSide->vertex[TR_EL] = curSeg->vertex[TR_ER];
        curSide->type2 = TR_RSIDE;
        }

        type = sideBankType[side];
        curSide->startWidth = sw;
        curSide->endWidth = ew;
        curSide->width = minWidth = MIN(sw, ew);
        maxWidth = MAX(sw, ew);
        curSide->type = curSeg->type;
        curSide->surface = sideSurface[side];
        curSide->envIndex = envIndex;
        curSide->DoVfactor = DoVfactor;
        curSide->angle[TR_XS] = curSeg->angle[TR_XS] * (tdble)type;
        curSide->angle[TR_XE] = curSeg->angle[TR_XE] * (tdble)type;
        curSide->angle[TR_ZS] = curSeg->angle[TR_ZS];
        curSide->angle[TR_ZE] = curSeg->angle[TR_ZE];
        curSide->angle[TR_CS] = curSeg->angle[TR_CS];

        switch(curSeg->type) {
        case TR_STR:
        curSide->length = curSeg->length;
        curSide->cos = curSeg->cos;
        curSide->sin = curSeg->sin;

        switch(side) {
        case 1:
            curSide->vertex[TR_SL].x = curSide->vertex[TR_SR].x + sw * curSeg->rgtSideNormal.x;
            curSide->vertex[TR_SL].y = curSide->vertex[TR_SR].y + sw * curSeg->rgtSideNormal.y;
            curSide->vertex[TR_SL].z = curSide->vertex[TR_SR].z + (tdble)type * sw * tan(curSeg->angle[TR_XS]);
            x = curSide->vertex[TR_EL].x = curSide->vertex[TR_ER].x + ew * curSeg->rgtSideNormal.x;
            y = curSide->vertex[TR_EL].y = curSide->vertex[TR_ER].y + ew * curSeg->rgtSideNormal.y;
            z = curSide->vertex[TR_EL].z = curSide->vertex[TR_ER].z + (tdble)type * ew * tan(curSeg->angle[TR_XE]);
            break;
        case 0:
            curSide->vertex[TR_SR].x = curSide->vertex[TR_SL].x - sw * curSeg->rgtSideNormal.x;
            curSide->vertex[TR_SR].y = curSide->vertex[TR_SL].y - sw * curSeg->rgtSideNormal.y;
            curSide->vertex[TR_SR].z = curSide->vertex[TR_SL].z - (tdble)type * sw * tan(curSeg->angle[TR_XS]);
            x = curSide->vertex[TR_ER].x = curSide->vertex[TR_EL].x - ew * curSeg->rgtSideNormal.x;
            y = curSide->vertex[TR_ER].y = curSide->vertex[TR_EL].y - ew * curSeg->rgtSideNormal.y;
            z = curSide->vertex[TR_ER].z = curSide->vertex[TR_EL].z - (tdble)type * ew * tan(curSeg->angle[TR_XE]);
            break;
        }
        curSide->angle[TR_YR] = atan2(curSide->vertex[TR_ER].z - curSide->vertex[TR_SR].z, curSide->length);
        curSide->angle[TR_YL] = atan2(curSide->vertex[TR_EL].z - curSide->vertex[TR_SL].z, curSide->length);

        curSide->Kzl = tan(curSide->angle[TR_YR]);
        curSide->Kzw = (curSide->angle[TR_XE] - curSide->angle[TR_XS]) / curSide->length;
        curSide->Kyl = (ew - sw) / curSide->length;

        curSide->rgtSideNormal.x = curSeg->rgtSideNormal.x;
        curSide->rgtSideNormal.y = curSeg->rgtSideNormal.y;

        TSTX(x);
        TSTY(y);
        TSTZ(z);
        break;

        case TR_LFT:
        curSide->center.x = curSeg->center.x;
        curSide->center.y = curSeg->center.y;

        switch(side) {
        case 1:
            curSide->radius = (tdble)(curSeg->radiusl - sw / 2.0);
            curSide->radiusr = curSeg->radiusl;
            curSide->radiusl = curSeg->radiusl - maxWidth;
            curSide->arc = curSeg->arc;
            curSide->length = curSide->radius * curSide->arc;

            curSide->vertex[TR_SL].x = curSide->vertex[TR_SR].x - sw * cos(curSide->angle[TR_CS]);
            curSide->vertex[TR_SL].y = curSide->vertex[TR_SR].y - sw * sin(curSide->angle[TR_CS]);
            curSide->vertex[TR_SL].z = curSide->vertex[TR_SR].z + (tdble)type * sw * tan(curSeg->angle[TR_XS]);
            curSide->vertex[TR_EL].x = curSide->vertex[TR_ER].x - ew * cos(curSide->angle[TR_CS] + curSide->arc);
            curSide->vertex[TR_EL].y = curSide->vertex[TR_ER].y - ew * sin(curSide->angle[TR_CS] + curSide->arc);
            z = curSide->vertex[TR_EL].z = curSide->vertex[TR_ER].z + (tdble)type * ew * tan(curSeg->angle[TR_XE]);

            curSide->angle[TR_YR] = atan2(curSide->vertex[TR_ER].z - curSide->vertex[TR_SR].z,
                          curSide->arc * curSide->radiusr);
            curSide->angle[TR_YL] = atan2(curSide->vertex[TR_EL].z - curSide->vertex[TR_SL].z,
                          curSide->arc * curSide->radiusl);

            curSide->Kzl = tan(curSide->angle[TR_YR]) * curSide->radiusr;
            curSide->Kzw = (curSide->angle[TR_XE] - curSide->angle[TR_XS]) / curSide->arc;
            curSide->Kyl = (ew - sw) / curSide->arc;

            /* to find the boundary */
            al = (tdble)(curSide->arc / 36.0);
            alfl = curSide->angle[TR_CS];

            for (j = 0; j < 36; j++) {
            alfl += al;
            x2 = curSide->center.x + (curSide->radiusl) * cos(alfl);
            y2 = curSide->center.y + (curSide->radiusl) * sin(alfl);
            TSTX(x2);
            TSTY(y2);
            }
            TSTZ(z);
            break;

        case 0:
            curSide->radius = (tdble)(curSeg->radiusr + sw / 2.0);
            curSide->radiusl = curSeg->radiusr;
            curSide->radiusr = curSeg->radiusr + maxWidth;
            curSide->arc = curSeg->arc;
            curSide->length = curSide->radius * curSide->arc;

            curSide->vertex[TR_SR].x = curSide->vertex[TR_SL].x + sw * cos(curSide->angle[TR_CS]);
            curSide->vertex[TR_SR].y = curSide->vertex[TR_SL].y + sw * sin(curSide->angle[TR_CS]);
            curSide->vertex[TR_SR].z = curSide->vertex[TR_SL].z - (tdble)type * sw * tan(curSeg->angle[TR_XS]);
            curSide->vertex[TR_ER].x = curSide->vertex[TR_EL].x + ew * cos(curSide->angle[TR_CS] + curSide->arc);
            curSide->vertex[TR_ER].y = curSide->vertex[TR_EL].y + ew * sin(curSide->angle[TR_CS] + curSide->arc);
            z = curSide->vertex[TR_ER].z = curSide->vertex[TR_EL].z - (tdble)type * ew * tan(curSeg->angle[TR_XE]);

            curSide->angle[TR_YR] = atan2(curSide->vertex[TR_ER].z - curSide->vertex[TR_SR].z,
                          curSide->arc * curSide->radiusr);
            curSide->angle[TR_YL] = atan2(curSide->vertex[TR_EL].z - curSide->vertex[TR_SL].z,
                          curSide->arc * curSide->radiusl);

            curSide->Kzl = tan(curSide->angle[TR_YR]) * (curSide->radiusr);
            curSide->Kzw = (curSide->angle[TR_XE] - curSide->angle[TR_XS]) / curSide->arc;
            curSide->Kyl = (ew - sw) / curSide->arc;

            /* to find the boundary */
            al = (tdble)(curSide->arc / 36.0);
            alfl = curSide->angle[TR_CS];

            for (j = 0; j < 36; j++) {
            alfl += al;
            x2 = curSide->center.x + (curSide->radiusr) * cos(alfl);
            y2 = curSide->center.y + (curSide->radiusr) * sin(alfl);
            TSTX(x2);
            TSTY(y2);
            }
            TSTZ(z);
            break;

        }
        break;
        case TR_RGT:
        curSide->center.x = curSeg->center.x;
        curSide->center.y = curSeg->center.y;

        switch(side) {
        case 1:
            curSide->radius = (tdble)(curSeg->radiusl + sw / 2.0);
            curSide->radiusr = curSeg->radiusl;
            curSide->radiusl = curSeg->radiusl + maxWidth;
            curSide->arc = curSeg->arc;
            curSide->length = curSide->radius * curSide->arc;

            curSide->vertex[TR_SL].x = curSide->vertex[TR_SR].x + sw * cos(curSide->angle[TR_CS]);
            curSide->vertex[TR_SL].y = curSide->vertex[TR_SR].y + sw * sin(curSide->angle[TR_CS]);
            curSide->vertex[TR_SL].z = curSide->vertex[TR_SR].z + (tdble)type * sw * tan(curSeg->angle[TR_XS]);
            curSide->vertex[TR_EL].x = curSide->vertex[TR_ER].x + ew * cos(curSide->angle[TR_CS] - curSide->arc);
            curSide->vertex[TR_EL].y = curSide->vertex[TR_ER].y + ew * sin(curSide->angle[TR_CS] - curSide->arc);
            z = curSide->vertex[TR_EL].z = curSide->vertex[TR_ER].z + (tdble)type * ew * tan(curSeg->angle[TR_XE]);

            curSide->angle[TR_YR] = atan2(curSide->vertex[TR_ER].z - curSide->vertex[TR_SR].z,
                          curSide->arc * curSide->radiusr);
            curSide->angle[TR_YL] = atan2(curSide->vertex[TR_EL].z - curSide->vertex[TR_SL].z,
                          curSide->arc * curSide->radiusl);

            curSide->Kzl = tan(curSide->angle[TR_YR]) * curSide->radiusr;
            curSide->Kzw = (curSide->angle[TR_XE] - curSide->angle[TR_XS]) / curSide->arc;
            curSide->Kyl = (ew - sw) / curSide->arc;

            /* to find the boundary */
            al = (tdble)(curSide->arc / 36.0);
            alfl = curSide->angle[TR_CS];

            for (j = 0; j < 36; j++) {
            alfl -= al;
            x1 = curSide->center.x + (curSide->radiusl) * cos(alfl);   /* location of end */
            y1 = curSide->center.y + (curSide->radiusl) * sin(alfl);
            TSTX(x1);
            TSTY(y1);
            }
            TSTZ(z);
            break;

        case 0:
            curSide->radius = (tdble)(curSeg->radiusr - sw / 2.0);
            curSide->radiusl = curSeg->radiusr;
            curSide->radiusr = curSeg->radiusr - maxWidth;
            curSide->arc = curSeg->arc;
            curSide->length = curSide->radius * curSide->arc;

            curSide->vertex[TR_SR].x = curSide->vertex[TR_SL].x - sw * cos(curSide->angle[TR_CS]);
            curSide->vertex[TR_SR].y = curSide->vertex[TR_SL].y - sw * sin(curSide->angle[TR_CS]);
            curSide->vertex[TR_SR].z = curSide->vertex[TR_SL].z - (tdble)type * sw * tan(curSeg->angle[TR_XS]);
            curSide->vertex[TR_ER].x = curSide->vertex[TR_EL].x - ew * cos(curSide->angle[TR_CS] - curSide->arc);
            curSide->vertex[TR_ER].y = curSide->vertex[TR_EL].y - ew * sin(curSide->angle[TR_CS] - curSide->arc);
            z = curSide->vertex[TR_ER].z = curSide->vertex[TR_EL].z - (tdble)type * ew * tan(curSeg->angle[TR_XE]);

            curSide->angle[TR_YR] = atan2(curSide->vertex[TR_ER].z - curSide->vertex[TR_SR].z,
                          curSide->arc * curSide->radiusr);
            curSide->angle[TR_YL] = atan2(curSide->vertex[TR_EL].z - curSide->vertex[TR_SL].z,
                          curSide->arc * curSide->radiusl);

            curSide->Kzl = tan(curSide->angle[TR_YR]) * (curSide->radiusr);
            curSide->Kzw = (curSide->angle[TR_XE] - curSide->angle[TR_XS]) / curSide->arc;
            curSide->Kyl = (ew - sw) / curSide->arc;

            /* to find the boundary */
            al = (tdble)(curSide->arc / 36.0);
            alfl = curSide->angle[TR_CS];

            for (j = 0; j < 36; j++) {
            alfl -= al;
            x2 = curSide->center.x + (curSide->radiusr) * cos(alfl);   /* location of end */
            y2 = curSide->center.y + (curSide->radiusr) * sin(alfl);
            TSTX(x2);
            TSTY(y2);
            }
            TSTZ(z);
            break;
        }
        break;
        }
    }

    /* Barrier */
    curBarrier = (tTrackBarrier*)malloc(sizeof(tTrackBarrier));
    if (!curBarrier) {
        GfFatal("AddSides: memory allocation error");
    }
    curBarrier->style = barrierStyle[side];
    curBarrier->width = barrierWidth[side];
    curBarrier->height = barrierHeight[side];
    curBarrier->surface = barrierSurface[side];

    // Compute normal of barrier for side collisions.
    tTrackSeg *bseg = mSeg;
    int bstart, bend;
    float bsign;

    if (side == TR_SIDE_LFT) {
        bstart = TR_SL;
        bend = TR_EL;
        bsign = -1.0f;
    } else {
        bstart = TR_SR;
        bend = TR_ER;
        bsign = 1.0f;
    }

    while (bseg->side[side] != NULL) {
        bseg = bseg->side[side];
    }

    vec2f n(
        -(bseg->vertex[bend].y - bseg->vertex[bstart].y)*bsign,
         (bseg->vertex[bend].x - bseg->vertex[bstart].x)*bsign
    );

    n.normalize();
    curBarrier->normal = n;

    mSeg->barrier[side] = curBarrier;
    }
}


//
// InitPits
// Tries to discover each specific pit segment, like entry/exit,
// start/end, pit building start.
//
// @param theTrack pointer to the track structure
// @param TrackHandle handle of the track XML file
// @return true on success
static bool InitPits(tTrack *theTrack, void *TrackHandle) {
    int i;
    int			segId;

    //Set each pit-related ptr to initial value
    pitEntrySeg = NULL;
    pitExitSeg = NULL;
    pitStart = NULL;
    pitBuildingsStart = NULL;
    pitBuildingsEnd = NULL;
    pitEnd = NULL;

    //Search for the pit section in the track XML file
    pits = &(theTrack->pits);
    sprintf(path2, "%s/%s", TRK_SECT_MAIN, TRK_SECT_PITS);
    char *segName = GfParmGetStrNC(TrackHandle, path2, TRK_ATT_ENTRY, NULL);

    //If there exists a pit section, we search and set each ptr
    //to the appropriate part of the pit.
    bool found = false;
    if (segName != 0) {
        //Search for pit entry
        sprintf(path, "%s/%s/%.*s", TRK_SECT_MAIN, TRK_LST_SEGMENTS, (int)(sizeof(path) - strlen(TRK_SECT_MAIN) - strlen(TRK_LST_SEGMENTS)), segName);
        segId = (int)GfParmGetNum(TrackHandle, path, TRK_ATT_ID, (char*)NULL, -1);
        pitEntrySeg = theTrack->seg;
        for(i = 0; i <= theTrack->nseg; i++) {
            if (pitEntrySeg->id == segId) {
                found = true;
            } else if (found) {
                pitEntrySeg = pitEntrySeg->next;
                break;
            }
            pitEntrySeg = pitEntrySeg->prev;
        }// for i
        if (found) 
	        GfOut("PitEntry: %s\n", pitEntrySeg->name);
		else
            pitEntrySeg = NULL;


        //Search for pit exit
        segName = GfParmGetStrNC(TrackHandle, path2, TRK_ATT_EXIT, NULL);
        if (segName != 0) {
            /* Search backward the last segment with that name */
            pitExitSeg = theTrack->seg; /* last track segment */
            found = false;
            for(i = 0; i <= theTrack->nseg; i++) {
                /* set the flag on the last segment of pit_exit */
                if (!strcmp(segName, pitExitSeg->name)) {
                    found = true;
                    break;
                }
                pitExitSeg = pitExitSeg->prev;
            }
            if (found) 
		        GfOut("PitExit: %s\n", pitExitSeg->name);
			else
                pitExitSeg = NULL;
        }

        //Search for pits start
        segName = GfParmGetStrNC(TrackHandle, path2, TRK_ATT_START, NULL);
        if (segName != 0) {
            pitStart = theTrack->seg;
            found = false;
            for(i = 0; i <= theTrack->nseg; i++) {
                if (!strcmp(segName, pitStart->name)) {
                    found = true;
                } else if (found) {
                    pitStart = pitStart->next;
                    break;
                }
                pitStart = pitStart->prev;
            }
            if (!found) {
                pitStart = NULL;
            }
        }
		if (pitStart != NULL) {
	        GfOut("PitStart: %s\n", pitStart->name);

	        //Search for pit buildings start
		    segName = GfParmGetStrNC(TrackHandle, path2, TRK_ATT_BUILDINGS_START, NULL);
			if (segName != 0) {
				pitBuildingsStart = theTrack->seg;
	            found = false;
		        for(i = 0; i <= theTrack->nseg; i++) {
			        if (!strcmp(segName, pitBuildingsStart->name)) {
				        found = true;
					} else if (found) {
	                    pitBuildingsStart = pitBuildingsStart->next;
		                break;
			        }
				pitBuildingsStart = pitBuildingsStart->prev;
				}
	            if (!found) {
		            pitBuildingsStart = pitStart;
			    }
	        } else {
		        pitBuildingsStart = pitStart;
			}
	        GfOut("PitBuildingStart: %s\n", pitBuildingsStart->name);
		}

        //Search for pits end
        segName = GfParmGetStrNC(TrackHandle, path2, TRK_ATT_END, NULL);
        if (segName != 0)
        {
            /* Search backward the last segment with that name */
            pitEnd = theTrack->seg; /* last track segment */
            found = false;
            for(i = 0; i <= theTrack->nseg; i++)
            {
                if (!strcmp(segName, pitEnd->name))
                {
                    found = true;
                    break;
                }

                pitEnd = pitEnd->prev;
            }

            if (!found)
            {
                pitEnd = NULL;
            }
        }
		if (pitEnd != NULL) {
	        GfOut("PitEnd: %s\n", pitEnd->name);

			//Search for pit buildings end
		    segName = GfParmGetStrNC(TrackHandle, path2, TRK_ATT_BUILDINGS_STOP, NULL);
			if (segName != 0) {
				/* Search backward the last segment with that name */
	            pitBuildingsEnd = theTrack->seg; /* last track segment */
		        found = false;
			    for(i = 0; i <= theTrack->nseg; i++) {
				    if (!strcmp(segName, pitBuildingsEnd->name)) {
					    found = true;
						break;
	                }
		            pitBuildingsEnd = pitBuildingsEnd->prev;
			    }
				if (!found) {
					pitBuildingsEnd = pitEnd;
	            } 
		    } else {
			    pitBuildingsEnd = pitEnd;
	        }
		    GfOut("PitBuildingsEnd: %s\n", pitBuildingsEnd->name);
        }

        //Decide which side the pit is located
        const char *paramVal = GfParmGetStr(TrackHandle, path2, TRK_ATT_SIDE, "right");
        pits->side = (strcmp(paramVal, "right") == 0) ? TR_RGT : TR_LFT;

        //Set pitlane speed limit
        pits->speedLimit = GfParmGetNum(TrackHandle, path2, TRK_ATT_SPD_LIM, (char*)NULL, 25.0);

        //Decide pit style
        pits->type = (int)GfParmGetNum(TrackHandle, path2, TRK_ATT_PIT_STYLE, NULL, TR_PIT_ON_TRACK_SIDE);
        GfOut("track5:: Pit style: %d\n", pits->type);

        if ((pitEntrySeg != NULL) && (pitExitSeg != NULL)
            && (pitStart != NULL) && (pitEnd != NULL))
        {
            pits->pitEntry = pitEntrySeg;
            pits->pitExit  = pitExitSeg;
            pits->pitStart = pitStart;
            pits->pitEnd = pitEnd;
            pitEntrySeg->raceInfo |= TR_PITENTRY;
            pitExitSeg->raceInfo |= TR_PITEXIT;
            pits->len   = GfParmGetNum(TrackHandle, path2, TRK_ATT_LEN, (char*)NULL, 15.0);
            pits->width = GfParmGetNum(TrackHandle, path2, TRK_ATT_WIDTH, (char*)NULL, 5.0);
            pits->pitindicator = (int) GfParmGetNum(TrackHandle, path2, TRK_ATT_PIT_INDICATOR, (char*)NULL, 0.0);
            found = true;
        } else
        {
            found = false;
        }
    }//if segName != 0

    return found;
}//InitPits


//
// AddPitDoors
// Decides the locations of pit buildings and doors.
// It is only used to draw the pit doors later in GrScene,
// as the pit buildings are drawn statically by Trackgen.
//
// @param theTrack pointer to the track structure
// @param TrackHandle handle of the track XML file
// @param found if InitPits found each pit segment OK
static void AddPitDoors(tTrack *theTrack, void *TrackHandle, bool found) {
    tTrackSeg		*curSeg;
    tTrackSeg		*curSeg2;
    tTrackSeg		*mSeg;
    tTrackSeg		*curPitSeg = NULL;

    if (found) {
        switch(pits->type) {
            case TR_PIT_NO_BUILDING:
            case TR_PIT_ON_TRACK_SIDE:
                {//dummy for eliminating warnings of locally declared variables cross-jumping with cases
                    pits->nPitSeg  = 0;

					// If undefined used defaults
					if (pitBuildingsStart == NULL)
                        pitBuildingsStart = pitStart;
                    if (pitBuildingsEnd == NULL)
                        pitBuildingsEnd = pitEnd;

					if (pitBuildingsStart->lgfromstart > pitBuildingsEnd->lgfromstart) {
                        pits->nPitSeg = (int)((theTrack->length - pitBuildingsStart->lgfromstart
                            + pitBuildingsEnd->lgfromstart + pitBuildingsEnd->length + pits->len / 2.0) / pits->len);
                    } else {
                        pits->nPitSeg = (int)((pitBuildingsEnd->lgfromstart + pitBuildingsEnd->length
                            - pitBuildingsStart->lgfromstart + pits->len / 2.0) / pits->len);
                    }
                    pits->nMaxPits = MIN(pits->nPitSeg,(int)GfParmGetNum(TrackHandle, path2, TRK_ATT_MAX_PITS, (char*)NULL, (tdble) pits->nPitSeg));
                    pits->driversPits = (tTrackOwnPit*)calloc(pits->nMaxPits, sizeof(tTrackOwnPit));

                    mSeg = pitBuildingsStart->prev;

                    bool		changeSeg = true;
                    tdble		offset = 0;
                    tdble		toStart = 0;
                    int i = 0;
                    while (i < pits->nMaxPits) {
                        if (changeSeg) {
                            changeSeg = false;
                            offset = 0;
                            mSeg = mSeg->next;
                            if (toStart >= mSeg->length) {
                                toStart -= mSeg->length;
                                changeSeg = true;
                                continue;
                            }

                            switch (pits->side) {
                                case TR_RGT:
                                    curPitSeg = mSeg->rside;
                                    if (curPitSeg->rside) {
                                        offset = curPitSeg->width;
                                        curPitSeg = curPitSeg->rside;
                                    }
                                    break;

                                case TR_LFT:
                                    curPitSeg = mSeg->lside;
                                    if (curPitSeg->lside) {
                                        offset = curPitSeg->width;
                                        curPitSeg = curPitSeg->lside;
                                    }
                                    break;
                            }//switch pits->side
                        }//if changeSeg

                        pits->driversPits[i].pos.type = TR_LPOS_MAIN;
                        //NB: TR_LPOS_MAIN not handled by RtTrackLocal2Global!
                        //It is coincidentally equal to TR_TORIGHT, that's why it works.
                        //Should clear up.
                        pits->driversPits[i].pos.seg = mSeg;

                        //RtTrackLocal2Global expects toStart as a length in meters for straight,
                        //and as an angle in radian for curves
                        //TODO(kilo)
                        //Proper handling of this should enable non-linear pitlanes.
                        //Postponed after 2.0
#if 0
                        tdble pitCenter = toStart + pits->len / 2.0f;
                        switch(mSeg->type) {
                            case TR_STR:
                                pits->driversPits[i].pos.toStart = pitCenter;
                                break;

                            case TR_LFT:
                            case TR_RGT:
                                pits->driversPits[i].pos.toStart = pitCenter / mSeg->radius;
                                break;
                        }
#endif
                        //TODO(kilo) get rid of following 3 lines when above feature is ready
                        pits->driversPits[i].pos.seg = mSeg;
                        pits->driversPits[i].pos.toStart = (tdble)(toStart + pits->len / 2.0);
                        GfLogDebug("toStart: %s %.2f ", mSeg->name, pits->driversPits[i].pos.toStart);


                        switch (pits->side) {
                            case TR_RGT:
                                pits->driversPits[i].pos.toRight  = -offset - RtTrackGetWidth(curPitSeg, toStart) + pits->width / 2.0f;
                                pits->driversPits[i].pos.toLeft   = mSeg->width - pits->driversPits[i].pos.toRight;
                                pits->driversPits[i].pos.toMiddle = mSeg->width / 2.0f - pits->driversPits[i].pos.toRight;
                                break;

                            case TR_LFT:
                                pits->driversPits[i].pos.toLeft   = -offset - RtTrackGetWidth(curPitSeg, toStart) + pits->width / 2.0f;
                                pits->driversPits[i].pos.toRight  = mSeg->width - pits->driversPits[i].pos.toLeft;
                                pits->driversPits[i].pos.toMiddle = mSeg->width / 2.0f - pits->driversPits[i].pos.toLeft;
                                break;
                        }//switch pits->side

                        toStart += pits->len;
                        if (toStart >= mSeg->length) {
                            toStart -= mSeg->length;
                            changeSeg = true;
                        }

                        i++;
                    }//while i
                    GfLogDebug("\n");

					// Setup pit speed limit
					for (mSeg = pitStart->prev; mSeg != pitEnd->next->next; mSeg = mSeg->next) {
						curSeg = curSeg2 = NULL;

						switch(pits->side) {

						case TR_RGT:
						curSeg = mSeg->rside;
						if (curSeg)
							curSeg2 = curSeg->rside;
						break;

						case TR_LFT:
						curSeg = mSeg->lside;
						if (curSeg)
							curSeg2 = curSeg->lside;
						break;

						}

					    if ((mSeg != pitStart->prev) && (mSeg != pitEnd->next)) {
							if (curSeg) {
								curSeg->raceInfo |= TR_PIT | TR_SPEEDLIMIT;
								if (curSeg2) {
									curSeg2->raceInfo |= TR_PIT | TR_SPEEDLIMIT;
								}
							}
						} else if (mSeg == pitStart->prev) {
							if (curSeg) {
								curSeg->raceInfo |= TR_PITSTART;
								if (curSeg2) {
									curSeg2->raceInfo |= TR_PITSTART;
								}
							}
						} else if (mSeg == pitEnd->next) {
							if (curSeg) {
								curSeg->raceInfo |= TR_PITEND;
								if (curSeg2) {
									curSeg2->raceInfo |= TR_PITEND;
								} 
							}
						}
					}
                }//dummy
                break;

            case TR_PIT_ON_SEPARATE_PATH:
                //NOT IMPLEMENTED YET
                break;

            case TR_PIT_NONE:
                //No action needed
                break;
        }//switch pits->type
    }//if found

    for (mSeg = pitBuildingsStart; mSeg != pitBuildingsEnd; mSeg = mSeg->next) {
        curSeg2 = NULL;

        switch(pits->side) {
            case TR_RGT:
                mSeg->barrier[0]->style = TR_PITBUILDING;
                curSeg = mSeg->rside;
				if (curSeg) {
					curSeg2 = curSeg->rside;
					if (curSeg2 != NULL)
						curSeg2->raceInfo |= TR_PITBUILD;
				}
                break;

            case TR_LFT:
                mSeg->barrier[1]->style = TR_PITBUILDING;
                curSeg = mSeg->lside;
				if (curSeg) {
	                curSeg2 = curSeg->lside;
					if (curSeg2 != NULL)
					    curSeg2->raceInfo |= TR_PITBUILD;
				}
            break;
        }//switch pits->side
    }//for mSeg
}

static void
normSeg(tTrackSeg *curSeg)
{
    curSeg->vertex[TR_SR].x -= xmin;
    curSeg->vertex[TR_SR].y -= ymin;
    curSeg->vertex[TR_SR].z -= zmin;
    curSeg->vertex[TR_SL].x -= xmin;
    curSeg->vertex[TR_SL].y -= ymin;
    curSeg->vertex[TR_SL].z -= zmin;
    curSeg->vertex[TR_ER].x -= xmin;
    curSeg->vertex[TR_ER].y -= ymin;
    curSeg->vertex[TR_ER].z -= zmin;
    curSeg->vertex[TR_EL].x -= xmin;
    curSeg->vertex[TR_EL].y -= ymin;
    curSeg->vertex[TR_EL].z -= zmin;
    curSeg->center.x -= xmin;
    curSeg->center.y -= ymin;
}

static void
CreateSegRing(void *TrackHandle, tTrack *theTrack, tTrackSeg *start, tTrackSeg *end, int ext)
{
    int		j;
    int		segread, curindex;
    tdble	radius, radiusend = 0, dradius;
    tdble	innerradius;
    tdble	arc;
    tdble	length;
    tTrackSeg	*curSeg;
    tTrackSeg	*root;
    tdble	alf;
    tdble	xr, yr, newxr, newyr;
    tdble	xl, yl, newxl, newyl;
    tdble	cenx, ceny;
    tdble	width, wi2;
    tdble	x1, x2, y1, y2;
    tdble	al, alfl;
    tdble	zsl, zsr, zel, zer, zs, ze;
    tdble	bankings, bankinge, dz;
    //tdble dzl, dzr;
    tdble	etgt, stgt;
    tdble	etgtl, stgtl;
    tdble	etgtr, stgtr;
    tdble	stepslg = 0;
    int		steps, curStep;
    const char  *segtype = (const char*)NULL;
    const char	*material;
    tTrackSurface *surface;
    char	*segName;
    int		type;
    const char	*profil;
    tdble	totLength;

    tdble	tl, dtl, T1l, T2l;
    tdble	tr, dtr, T1r, T2r;
    tdble	curzel, curzer, curArc, curLength, curzsl, curzsr;
    tdble	grade;

    void	*segNameHash = NULL;

    static char	path[256];
    #define MAX_TMP_INTS	256
    int		mi[MAX_TMP_INTS];
    int		ind = 0;

    radius = arc = length = alf = xr = yr = newxr = newyr = xl = yl = 0;
    zel = zer = etgtl = etgtr = newxl = newyl = 0;
    type = 0;

    width = GfParmGetNum(TrackHandle, TRK_SECT_MAIN, TRK_ATT_WIDTH, (char*)NULL, 15.0);
    wi2 = (tdble)(width / 2.0);

    grade = -100000.0;
    root = (tTrackSeg*)NULL;
    totLength = 0;

    sprintf(path, "%s/%s", TRK_SECT_MAIN, TRK_LST_SEGMENTS);
    if (start == NULL) {
        xr = xl = 0.0;
        yr = 0.0;
        yl = width;
      xr = GfParmGetNum(TrackHandle, TRK_SECT_MAIN, TRK_ATT_START_X, (char*)NULL, 0.0);
      yr = GfParmGetNum(TrackHandle, TRK_SECT_MAIN, TRK_ATT_START_Y, (char*)NULL, 0.0);
        alf = GfParmGetNum(TrackHandle, TRK_SECT_MAIN, TRK_ATT_START_BEARING, 0, FLOAT_DEG2RAD(0.0f));
        xr += wi2 * sin(alf);
        xl = xr - width * sin(alf);
        yr -= wi2 * cos(alf);
        yl = yr + width * cos(alf);

        zsl = zsr = zel = zer = zs = ze = GfParmGetNum(TrackHandle, TRK_SECT_MAIN, TRK_ATT_START_Z, (char*)NULL, 0.0);
        stgt = etgt = 0.0;
        stgtl = etgtl = 0.0;
        stgtr = etgtr = 0.0;
    } else {
        GfParmListSeekFirst(TrackHandle, path);
        segtype = GfParmGetCurStr(TrackHandle, path, TRK_ATT_TYPE, "");
        if (strcmp(segtype, TRK_VAL_STR) == 0) {
        } else if (strcmp(segtype, TRK_VAL_LFT) == 0) {
        } else if (strcmp(segtype, TRK_VAL_RGT) == 0) {
            xr = start->vertex[TR_SR].x;
            yr = start->vertex[TR_SR].y;
            zsl = zsr = zel = zer = zs = ze = start->vertex[TR_SR].z;
            alf = start->angle[TR_ZS];
            xl = xr - width * sin(alf);
            yl = yr + width * cos(alf);
            stgt = etgt = 0.0;
            stgtl = etgtl = 0.0;
            stgtr = etgtr = 0.0;
        }
    }


    //GfLogDebug("Track physics : kFrictionDry | Surface :\n");

    /* Main Track */
    material = GfParmGetStr(TrackHandle, TRK_SECT_MAIN, TRK_ATT_SURF, TRK_VAL_ASPHALT);
    surface = AddTrackSurface(TrackHandle, theTrack, material);
    envIndex = 0;
    DoVfactor =1.0;
    InitSides(TrackHandle, theTrack);

    if (ext) {
        segNameHash = GfHashCreate(GF_HASH_TYPE_STR);
    }
    segread = 0;
    curindex = 0;
    GfParmListSeekFirst(TrackHandle, path);
    do {
        segtype = GfParmGetCurStr(TrackHandle, path, TRK_ATT_TYPE, NULL);
        if (segtype == 0) {
            continue;
        }
        segread++;

        zsl = zel;
        zsr = zer;
        TSTZ(zsl);
        TSTZ(zsr);

        /* Turn Marks */
        if (ext) {
            char *marks = GfParmGetCurStrNC(TrackHandle, path, TRK_ATT_MARKS, NULL);
            ind = 0;
            if (marks) {
                marks = strdup(marks);
                char *s = strtok(marks, ";");
                while ((s != NULL) && (ind < MAX_TMP_INTS)) {
                    mi[ind] = (int)strtol(s, NULL, 0);
                    ind++;
                    s = strtok(NULL, ";");
                }
                free(marks);
            }
        }

        /* surface change */
        material = GfParmGetCurStr(TrackHandle, path, TRK_ATT_SURF, material);
        surface = AddTrackSurface(TrackHandle, theTrack, material);
        envIndex = (int) GfParmGetCurNum(TrackHandle, path, TRK_ATT_ENVIND, (char*)NULL, (float) (envIndex+1)) - 1;
        // TODO: is the (int) intended?
        DoVfactor = (float) ((int) GfParmGetCurNum(TrackHandle, path, TRK_ATT_DOVFACTOR, (char*)NULL, 1.0)) ;

        /* get segment type and lenght */
        if (strcmp(segtype, TRK_VAL_STR) == 0) {
            /* straight */
            length = GfParmGetCurNum(TrackHandle, path, TRK_ATT_LG, (char*)NULL, 0);
            type = TR_STR;
            radius = radiusend = 0;
        } else if (strcmp(segtype, TRK_VAL_LFT) == 0) {
            /* left curve */
            radius = GfParmGetCurNum(TrackHandle, path, TRK_ATT_RADIUS, (char*)NULL, 0);
            radiusend = GfParmGetCurNum(TrackHandle, path, TRK_ATT_RADIUSEND, (char*)NULL, radius);
            arc = GfParmGetCurNum(TrackHandle, path, TRK_ATT_ARC, (char*)NULL, 0);
            type = TR_LFT;
            length = (tdble)((radius + radiusend) / 2.0 * arc);
        } else if (strcmp(segtype, TRK_VAL_RGT) == 0) {
            /* right curve */
            radius = GfParmGetCurNum(TrackHandle, path, TRK_ATT_RADIUS, (char*)NULL, 0);
            radiusend = GfParmGetCurNum(TrackHandle, path, TRK_ATT_RADIUSEND, (char*)NULL, radius);
            arc = GfParmGetCurNum(TrackHandle, path, TRK_ATT_ARC, (char*)NULL, 0);
            type = TR_RGT;
            length = (tdble)((radius + radiusend) / 2.0 * arc);
        }
        segName = GfParmListGetCurEltName(TrackHandle, path);
        if (ext) {
            if (GfHashGetStr(segNameHash, segName)) {
                GfLogError("DUPLICATED SEGMENT NAME \"%s\" PLEASE CHANGE IT !!!!\n", segName);
                exit(1);
            }
            GfHashAddStr(segNameHash, segName, segName);
        }

        /* elevation and banking */
        zsl = GfParmGetCurNum(TrackHandle, path, TRK_ATT_ZSL, (char*)NULL, zsl);
        zsr = GfParmGetCurNum(TrackHandle, path, TRK_ATT_ZSR, (char*)NULL, zsr);
        zel = GfParmGetCurNum(TrackHandle, path, TRK_ATT_ZEL, (char*)NULL, zel);
        zer = GfParmGetCurNum(TrackHandle, path, TRK_ATT_ZER, (char*)NULL, zer);
        ze = zs = -100000.0;
        ze = GfParmGetCurNum(TrackHandle, path, TRK_ATT_ZE, (char*)NULL, ze);
        zs = GfParmGetCurNum(TrackHandle, path, TRK_ATT_ZS, (char*)NULL, zs);
        grade = GfParmGetCurNum(TrackHandle, path, TRK_ATT_GRADE, (char*)NULL, grade);
        if (zs != -100000.0) {
            zsr = zsl = zs;
        } else {
            zs = (tdble)((zsl + zsr) / 2.0);
        }
        if (ze != -100000.0) {
            zer = zel = ze;
        } else if (grade != -100000.0) {
            ze = zs + length * grade;
        } else {
            ze = (tdble)((zel + zer) / 2.0);
        }
        bankings = atan2(zsl - zsr, width);
        bankinge = atan2(zel - zer, width);
        bankings = GfParmGetCurNum(TrackHandle, path, TRK_ATT_BKS, (char*)NULL, bankings);
        bankinge = GfParmGetCurNum(TrackHandle, path, TRK_ATT_BKE, (char*)NULL, bankinge);
        dz = (tdble)(tan(bankings) * width / 2.0);
        zsl = zs + dz;
        zsr = zs - dz;
        dz = (tdble)(tan(bankinge) * width / 2.0);
        zel = ze + dz;
        zer = ze - dz;

        TSTZ(zsl);
        TSTZ(zsr);

        /* Get segment profil */
        profil = GfParmGetCurStr(TrackHandle, path, TRK_ATT_PROFIL, TRK_VAL_SPLINE);
        stgtl = etgtl;
        stgtr = etgtr;
        if (strcmp(profil, TRK_VAL_SPLINE) == 0) {
            steps = (int)GfParmGetCurNum(TrackHandle, path, TRK_ATT_PROFSTEPS, (char*)NULL, 1.0);
            if (steps == 1) {
                stepslg = GfParmGetCurNum(TrackHandle, path, TRK_ATT_PROFSTEPSLEN, (char*)NULL, GlobalStepLen);
                if (stepslg) {
                    steps = (int)(length / stepslg) + 1;
                } else {
                    steps = 1;
                }
            }
            stgtl = GfParmGetCurNum(TrackHandle, path, TRK_ATT_PROFTGTSL, (char*)NULL, stgtl);
            etgtl = GfParmGetCurNum(TrackHandle, path, TRK_ATT_PROFTGTEL, (char*)NULL, etgtl);
            stgtr = GfParmGetCurNum(TrackHandle, path, TRK_ATT_PROFTGTSR, (char*)NULL, stgtr);
            etgtr = GfParmGetCurNum(TrackHandle, path, TRK_ATT_PROFTGTER, (char*)NULL, etgtr);
            stgt = etgt = -100000.0;
            stgt = GfParmGetCurNum(TrackHandle, path, TRK_ATT_PROFTGTS, (char*)NULL, stgt);
            etgt = GfParmGetCurNum(TrackHandle, path, TRK_ATT_PROFTGTE, (char*)NULL, etgt);
            if (stgt != -100000.0) {
                stgtl = stgtr = stgt;
            }
            if (etgt != -100000.0) {
                etgtl = etgtr = etgt;
            }
        } else {
            steps = 1;
            stgtl = etgtl = (zel - zsl) / length;
            stgtr = etgtr = (zer - zsr) / length;
        }
        GfParmSetCurNum(TrackHandle, path, TRK_ATT_ID, (char*)NULL, (tdble)curindex);

        //dzl = zel - zsl; // Never used.
        //dzr = zer - zsr; // Never used.
        T1l = stgtl * length;
        T2l = etgtl * length;
        tl = 0.0;
        dtl = (tdble)(1.0 / steps);
        T1r = stgtr * length;
        T2r = etgtr * length;
        tr = 0.0;
        dtr = (tdble)(1.0 / steps);

        curStep = 0;
        curzel = zsl;
        curzer = zsr;
        curArc = arc / (tdble)steps;
        curLength = length / (tdble)steps;
        dradius = (radiusend - radius) / (tdble)steps;
        if (radiusend != radius) {
            /* Compute the correct curLength... */
            if (steps != 1) {
                dradius = (radiusend - radius) / (tdble)(steps - 1);
                tdble tmpAngle = 0;
                tdble tmpRadius = radius;
                for (curStep = 0; curStep < steps; curStep++) {
                    tmpAngle += curLength / tmpRadius;
                    tmpRadius += dradius;
                }
                curLength *= arc / tmpAngle;
            }
        }
        curStep = 0;

        while (curStep < steps) {

            tl += dtl;
            tr += dtr;

            curzsl = curzel;
            curzel = TrackSpline(zsl, zel, T1l, T2l, tl);

            curzsr = curzer;
            curzer = TrackSpline(zsr, zer, T1r, T2r, tr);

            if (dradius != 0) {
                curArc = curLength / radius;
            }

            /* allocate a new segment */
            curSeg = (tTrackSeg*)calloc(1, sizeof(tTrackSeg));
            if (root == NULL) {
                root = curSeg;
                curSeg->next = curSeg;
                curSeg->prev = curSeg;
            } else {
                curSeg->next = root->next;
                curSeg->next->prev = curSeg;
                curSeg->prev = root;
                root->next = curSeg;
                root = curSeg;
            }
            curSeg->type2 = TR_MAIN;
            curSeg->name = segName;
            curSeg->id = curindex;
            curSeg->width = curSeg->startWidth = curSeg->endWidth = width;
            curSeg->surface = surface;
            curSeg->envIndex = envIndex;
            curSeg->DoVfactor = DoVfactor;
            /*GfLogDebug("curseg id =%d factor =%f\n",curSeg->id,curSeg->DoVfactor);*/
            curSeg->lgfromstart = totLength;

            if (ext && ind) {
                int	*mrks = (int*)calloc(ind, sizeof(int));
                tSegExt	*segExt = (tSegExt*)calloc(1, sizeof(tSegExt));

                memcpy(mrks, mi, ind*sizeof(int));
                segExt->nbMarks = ind;
                segExt->marks = mrks;
                curSeg->ext = segExt;
                ind = 0;
            }


            switch (type) {
                case TR_STR:
                    /* straight */
                    curSeg->type = TR_STR;
                    curSeg->length = curLength;
                    curSeg->sin = sin(alf);								// Precalculate these
                    curSeg->cos = cos(alf);

                    newxr = xr + curLength * cos(alf);      /* find end coordinates */
                    newyr = yr + curLength * sin(alf);
                    newxl = xl + curLength * cos(alf);
                    newyl = yl + curLength * sin(alf);

                    curSeg->vertex[TR_SR].x = xr;
                    curSeg->vertex[TR_SR].y = yr;
                    curSeg->vertex[TR_SR].z = curzsr;

                    curSeg->vertex[TR_SL].x = xl;
                    curSeg->vertex[TR_SL].y = yl;
                    curSeg->vertex[TR_SL].z = curzsl;

                    curSeg->vertex[TR_ER].x = newxr;
                    curSeg->vertex[TR_ER].y = newyr;
                    curSeg->vertex[TR_ER].z = curzer;

                    curSeg->vertex[TR_EL].x = newxl;
                    curSeg->vertex[TR_EL].y = newyl;
                    curSeg->vertex[TR_EL].z = curzel;

                    curSeg->angle[TR_ZS] = alf;
                    curSeg->angle[TR_ZE] = alf;
                    curSeg->angle[TR_YR] = atan2(curSeg->vertex[TR_ER].z - curSeg->vertex[TR_SR].z, curLength);
                    curSeg->angle[TR_YL] = atan2(curSeg->vertex[TR_EL].z - curSeg->vertex[TR_SL].z, curLength);
                    curSeg->angle[TR_XS] = atan2(curSeg->vertex[TR_SL].z - curSeg->vertex[TR_SR].z, width);
                    curSeg->angle[TR_XE] = atan2(curSeg->vertex[TR_EL].z - curSeg->vertex[TR_ER].z, width);

                    curSeg->Kzl = tan(curSeg->angle[TR_YR]);
                    curSeg->Kzw = (curSeg->angle[TR_XE] - curSeg->angle[TR_XS]) / curLength;
                    curSeg->Kyl = 0;

                    curSeg->rgtSideNormal.x = -sin(alf);
                    curSeg->rgtSideNormal.y = cos(alf);

                    TSTX(newxr); TSTX(newxl);
                    TSTY(newyr); TSTY(newyl);

                    break;

                case TR_LFT:
                    /* left curve */
                    curSeg->type = TR_LFT;
                    curSeg->radius = radius;
                    curSeg->radiusr = radius + wi2;
                    curSeg->radiusl = radius - wi2;
                    curSeg->arc = curArc;
                    curSeg->length = curLength;
                    curSeg->sin = 0.0;	//Not used for curves
                    curSeg->cos = 0.0;

                    innerradius = radius - wi2; /* left side aligned */
                    cenx = xl - innerradius * sin(alf);  /* compute center location: */
                    ceny = yl + innerradius * cos(alf);
                    curSeg->center.x = cenx;
                    curSeg->center.y = ceny;

                    curSeg->angle[TR_ZS] = alf;
                    curSeg->angle[TR_CS] = (tdble)(alf - PI / 2.0);
                    alf += curArc;
                    curSeg->angle[TR_ZE] = alf;

                    newxl = cenx + innerradius * sin(alf);   /* location of end */
                    newyl = ceny - innerradius * cos(alf);
                    newxr = cenx + (innerradius + width) * sin(alf);   /* location of end */
                    newyr = ceny - (innerradius + width) * cos(alf);

                    curSeg->vertex[TR_SR].x = xr;
                    curSeg->vertex[TR_SR].y = yr;
                    curSeg->vertex[TR_SR].z = curzsr;

                    curSeg->vertex[TR_SL].x = xl;
                    curSeg->vertex[TR_SL].y = yl;
                    curSeg->vertex[TR_SL].z = curzsl;

                    curSeg->vertex[TR_ER].x = newxr;
                    curSeg->vertex[TR_ER].y = newyr;
                    curSeg->vertex[TR_ER].z = curzer;

                    curSeg->vertex[TR_EL].x = newxl;
                    curSeg->vertex[TR_EL].y = newyl;
                    curSeg->vertex[TR_EL].z = curzel;

                    curSeg->angle[TR_YR] = atan2(curSeg->vertex[TR_ER].z - curSeg->vertex[TR_SR].z, curArc * (innerradius + width));
                    curSeg->angle[TR_YL] = atan2(curSeg->vertex[TR_EL].z - curSeg->vertex[TR_SL].z, curArc * innerradius);
                    curSeg->angle[TR_XS] = atan2(curSeg->vertex[TR_SL].z - curSeg->vertex[TR_SR].z, width);
                    curSeg->angle[TR_XE] = atan2(curSeg->vertex[TR_EL].z - curSeg->vertex[TR_ER].z, width);

                    curSeg->Kzl = tan(curSeg->angle[TR_YR]) * (innerradius + width);
                    curSeg->Kzw = (curSeg->angle[TR_XE] - curSeg->angle[TR_XS]) / curArc;
                    curSeg->Kyl = 0;

                    /* to find the boundary */
                    al = (tdble)(curArc / 36.0);
                    alfl = curSeg->angle[TR_CS];

                    for (j = 0; j < 36; j++) {
                        alfl += al;
                        x1 = curSeg->center.x + (innerradius) * cos(alfl);   /* location of end */
                        y1 = curSeg->center.y + (innerradius) * sin(alfl);
                        x2 = curSeg->center.x + (innerradius + width) * cos(alfl);   /* location of end */
                        y2 = curSeg->center.y + (innerradius + width) * sin(alfl);
                        TSTX(x1); TSTX(x2);
                        TSTY(y1); TSTY(y2);
                    }

                    break;

                case TR_RGT:
                    /* right curve */
                    curSeg->type = TR_RGT;
                    curSeg->radius = radius;
                    curSeg->radiusr = radius - wi2;
                    curSeg->radiusl = radius + wi2;
                    curSeg->arc = curArc;
                    curSeg->length = curLength;
                    curSeg->sin = 0.0;	//Not used for curves
                    curSeg->cos = 0.0;

                    innerradius = radius - wi2; /* right side aligned */
                    cenx = xr + innerradius * sin(alf);  /* compute center location */
                    ceny = yr - innerradius * cos(alf);
                    curSeg->center.x = cenx;
                    curSeg->center.y = ceny;

                    curSeg->angle[TR_ZS] = alf;
                    curSeg->angle[TR_CS] = (tdble)(alf + PI / 2.0);
                    alf -= curSeg->arc;
                    curSeg->angle[TR_ZE] = alf;

                    newxl = cenx - (innerradius + width) * sin(alf);   /* location of end */
                    newyl = ceny + (innerradius + width) * cos(alf);
                    newxr = cenx - innerradius * sin(alf);   /* location of end */
                    newyr = ceny + innerradius * cos(alf);

                    curSeg->vertex[TR_SR].x = xr;
                    curSeg->vertex[TR_SR].y = yr;
                    curSeg->vertex[TR_SR].z = curzsr;

                    curSeg->vertex[TR_SL].x = xl;
                    curSeg->vertex[TR_SL].y = yl;
                    curSeg->vertex[TR_SL].z = curzsl;

                    curSeg->vertex[TR_ER].x = newxr;
                    curSeg->vertex[TR_ER].y = newyr;
                    curSeg->vertex[TR_ER].z = curzer;

                    curSeg->vertex[TR_EL].x = newxl;
                    curSeg->vertex[TR_EL].y = newyl;
                    curSeg->vertex[TR_EL].z = curzel;

                    curSeg->angle[TR_YR] = atan2(curSeg->vertex[TR_ER].z - curSeg->vertex[TR_SR].z, curArc * innerradius);
                    curSeg->angle[TR_YL] = atan2(curSeg->vertex[TR_EL].z - curSeg->vertex[TR_SL].z, curArc * (innerradius + width));
                    curSeg->angle[TR_XS] = atan2(curSeg->vertex[TR_SL].z - curSeg->vertex[TR_SR].z, width);
                    curSeg->angle[TR_XE] = atan2(curSeg->vertex[TR_EL].z - curSeg->vertex[TR_ER].z, width);

                    curSeg->Kzl = tan(curSeg->angle[TR_YR]) * innerradius;
                    curSeg->Kzw = (curSeg->angle[TR_XE] - curSeg->angle[TR_XS]) / curArc;
                    curSeg->Kyl = 0;

                    /* to find the boundaries */
                    al = (tdble)(curSeg->arc / 36.0);
                    alfl = curSeg->angle[TR_CS];

                    for (j = 0; j < 36; j++) {
                        alfl -= al;
                        x1 = curSeg->center.x + (innerradius + width) * cos(alfl);   /* location of end */
                        y1 = curSeg->center.y + (innerradius + width) * sin(alfl);
                        x2 = curSeg->center.x + innerradius * cos(alfl);   /* location of end */
                        y2 = curSeg->center.y + innerradius * sin(alfl);
                        TSTX(x1); TSTX(x2);
                        TSTY(y1); TSTY(y2);
                    }
                    break;

            }

            AddSides(curSeg, TrackHandle, theTrack, curStep, steps);

            totLength += curSeg->length;
            xr = newxr;
            yr = newyr;
            xl = newxl;
            yl = newyl;
            curindex++;
            curStep++;
            if (type != TR_STR) {
                /* 		GfLogDebug("radius = %f arc = %f steps %d, length %f, stepslg %f\n", radius, RAD2DEG(curArc), steps, length, curLength); */
                radius += dradius;
            }
        }

    } while (GfParmListSeekNext(TrackHandle, path) == 0);

    if (ext) {
        GfHashRelease(segNameHash, NULL);
    }

    /* GfLogDebug("\n"); */

    theTrack->seg = root;
    theTrack->length = totLength;
    theTrack->nseg = curindex;
}




/*
 * Read version 5 track segments
 */
void ReadTrack5(tTrack *theTrack, void *TrackHandle,
                                    tRoadCam **camList, int ext) {
    int			i;
    tTrackSeg		*curSeg = NULL;
    int			segId;
    tRoadCam		*curCam;
    tTrkLocPos		trkPos;
    char *segName;

    xmax = ymax = zmax = -99999999999.99f;
    xmin = ymin = zmin = 99999999999.99f;

    //Decide step length
    GlobalStepLen = GfParmGetNum(TrackHandle, TRK_SECT_MAIN, TRK_ATT_PROFSTEPSLEN, (char*)NULL, 0);
    //Chop all the track to uniform length sub-segments
    CreateSegRing(TrackHandle, theTrack, (tTrackSeg*)NULL, (tTrackSeg*)NULL, ext);
    //Check for the limits of the pit
    bool found = InitPits(theTrack, TrackHandle);
    //Add coords for drawing pit doors in modules/grscene.cpp
    AddPitDoors(theTrack, TrackHandle, found);


    /*
     * camera definitions
     */
    if (GfParmListSeekFirst(TrackHandle, TRK_SECT_CAM) == 0) {
    do {
        curCam = (tRoadCam*)calloc(1, sizeof(tRoadCam));
        if (!curCam) {
        GfFatal("ReadTrack5: Memory allocation error");
        }
        if (*camList == NULL) {
        *camList = curCam;
        curCam->next = curCam;
        } else {
        curCam->next = (*camList)->next;
        (*camList)->next = curCam;
        *camList = curCam;
        }
        curCam->name = GfParmListGetCurEltName(TrackHandle, TRK_SECT_CAM);
        segName = GfParmGetCurStrNC(TrackHandle, TRK_SECT_CAM, TRK_ATT_SEGMENT, NULL);
        if (segName == 0) {
        GfFatal("Bad Track Definition: in Camera %s %s is missing\n", curCam->name, TRK_ATT_SEGMENT);
        }
        sprintf(path2, "%s/%s/%.*s", TRK_SECT_MAIN, TRK_LST_SEGMENTS,
        (int)(sizeof(path2) - strlen(TRK_SECT_MAIN) - strlen(TRK_LST_SEGMENTS)),
        segName);
        segId = (int)GfParmGetNum(TrackHandle, path2, TRK_ATT_ID, (char*)NULL, 0);
        curSeg = theTrack->seg;
        for(i=0; i<theTrack->nseg; i++)  {
        if (curSeg->id == segId) {
            break;
        }
        curSeg = curSeg->next;
        }

        trkPos.seg = curSeg;
        trkPos.toRight = GfParmGetCurNum(TrackHandle, TRK_SECT_CAM, TRK_ATT_TORIGHT, (char*)NULL, 0);
        trkPos.toStart = GfParmGetCurNum(TrackHandle, TRK_SECT_CAM, TRK_ATT_TOSTART, (char*)NULL, 0);
        TrackLocal2Global(&trkPos, &(curCam->pos.x), &(curCam->pos.y));
        curCam->pos.z = TrackHeightL(&trkPos) + GfParmGetCurNum(TrackHandle, TRK_SECT_CAM, TRK_ATT_HEIGHT, (char*)NULL, 0);

        segName = GfParmGetCurStrNC(TrackHandle, TRK_SECT_CAM, TRK_ATT_CAM_FOV, NULL);
        if (segName == 0) {
        GfFatal("Bad Track Definition: in Camera %s %s is missing\n", curCam->name, TRK_ATT_CAM_FOV);
        }
        sprintf(path2, "%s/%s/%.*s", TRK_SECT_MAIN, TRK_LST_SEGMENTS,
        (int)(sizeof(path) - strlen(TRK_SECT_MAIN) - strlen(TRK_LST_SEGMENTS)),
        segName);
        segId = (int)GfParmGetNum(TrackHandle, path2, TRK_ATT_ID, (char*)NULL, 0);
        curSeg = theTrack->seg;
        for(i=0; i<theTrack->nseg; i++)  {
        if (curSeg->id == segId) {
            break;
        }
        curSeg = curSeg->next;
        }
        segName = GfParmGetCurStrNC(TrackHandle, TRK_SECT_CAM, TRK_ATT_CAM_FOVE, NULL);
        if (segName == 0) {
        GfFatal("Bad Track Definition: in Camera %s %s is missing\n", curCam->name, TRK_ATT_CAM_FOVE);
        }
        sprintf(path2, "%s/%s/%.*s", TRK_SECT_MAIN, TRK_LST_SEGMENTS,
        (int)(sizeof(path) - strlen(TRK_SECT_MAIN) - strlen(TRK_LST_SEGMENTS)),
        segName);
        segId = (int)GfParmGetNum(TrackHandle, path2, TRK_ATT_ID, (char*)NULL, 0);

        do {
        curSeg->cam = curCam;
        curSeg = curSeg->next;
        } while (curSeg->id != segId);
    } while (GfParmListSeekNext(TrackHandle, TRK_SECT_CAM) == 0);
    }

    xmin = GfParmGetNum(TrackHandle, TRK_SECT_MAIN, TRK_ATT_MIN_X, (char*)NULL, xmin);
    ymin = GfParmGetNum(TrackHandle, TRK_SECT_MAIN, TRK_ATT_MIN_Y, (char*)NULL, ymin);
    zmin = GfParmGetNum(TrackHandle, TRK_SECT_MAIN, TRK_ATT_MIN_Z, (char*)NULL, zmin);
    xmax = GfParmGetNum(TrackHandle, TRK_SECT_MAIN, TRK_ATT_EXTENT_X, (char*)NULL, xmax);
    ymax = GfParmGetNum(TrackHandle, TRK_SECT_MAIN, TRK_ATT_EXTENT_Y, (char*)NULL, ymax);
    zmax = GfParmGetNum(TrackHandle, TRK_SECT_MAIN, TRK_ATT_EXTENT_Z, (char*)NULL, zmax);

    /* Update the coord to be positives */
    theTrack->min.x = 0;
    theTrack->min.y = 0;
    theTrack->min.z = 0;
    theTrack->max.x = xmax - xmin;
    theTrack->max.y = ymax - ymin;
    theTrack->max.z = zmax - zmin;

    curSeg = theTrack->seg;
    for(i=0; i<theTrack->nseg; i++)  {         /* read the segment data: */
    if ((curSeg->lgfromstart + curSeg->length) > (theTrack->length - 50.0)) {
        curSeg->raceInfo |= TR_LAST;
    } else if (curSeg->lgfromstart < 50.0) {
        curSeg->raceInfo |= TR_START;
    } else {
        curSeg->raceInfo |= TR_NORMAL;
    }
    normSeg(curSeg);
    if (curSeg->lside) {
        normSeg(curSeg->lside);
        if (curSeg->lside->lside) {
        normSeg(curSeg->lside->lside);
        }
    }
    if (curSeg->rside) {
        normSeg(curSeg->rside);
        if (curSeg->rside->rside) {
        normSeg(curSeg->rside->rside);
        }
    }
    curSeg = curSeg->next;
    }


    if (*camList != NULL) {
    curCam = *camList;
    do {
        curCam = curCam->next;
        curCam->pos.x -= xmin;
        curCam->pos.y -= ymin;
        curCam->pos.z -= zmin;
    } while (curCam != *camList);
    }
}
