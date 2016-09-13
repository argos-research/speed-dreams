/***************************************************************************

    file                 : track.cpp
    created              : Sun Jan 30 22:54:56 CET 2000
    copyright            : (C) 2000 by Eric Espie
    email                : torcs@free.fr
    version              : $Id: track.cpp 6164 2015-10-04 23:14:42Z torcs-ng $

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
#include <cstdio>
#include <cstring>
#include <ctime>
#include <cmath>

//#include <raceman.h>	//TODO(kilo): not needed?
#include <portability.h>

#include "trackinc.h"


static const tdble DEGPRAD = (tdble)(180.0 / PI);   /* degrees per radian */

static tTrack	*theTrack = NULL;
static tRoadCam *theCamList;
static void	*TrackHandle;

static void GetTrackHeader(void *TrackHandle);
static void FinishTrackLoading(void *TrackHandle);

/*
 * External function used to (re)build a track
 * from the track file
 */
tTrack *
TrackBuildv1(const char *trackfile)
{
    TrackShutdown();

    theTrack = (tTrack*)calloc(1, sizeof(tTrack));
    theCamList = (tRoadCam*)NULL;

    theTrack->params = TrackHandle =
		GfParmReadFile (trackfile, GFPARM_RMODE_STD | GFPARM_RMODE_CREAT | GFPARM_RMODE_PRIVATE);
    
    theTrack->filename = strdup(trackfile);

    GetTrackHeader(TrackHandle);

	switch(theTrack->version) {
    case 0:
    case 1:
    case 2:
    case 3:
	ReadTrack3(theTrack, TrackHandle, &theCamList, 0);
	break;
    case 4:
	ReadTrack4(theTrack, TrackHandle, &theCamList, 0);
   break;
    case 5:
	ReadTrack5(theTrack, TrackHandle, &theCamList, 0);
	break;
    
    }

    FinishTrackLoading(TrackHandle);

    return theTrack;
}

tTrack *
TrackBuildEx(const char *trackfile)
{
    	void	*TrackHandle;

    	theTrack = (tTrack*)calloc(1, sizeof(tTrack));
    	theCamList = (tRoadCam*)NULL;

    	theTrack->params = TrackHandle = GfParmReadFile (trackfile, GFPARM_RMODE_STD | GFPARM_RMODE_CREAT | GFPARM_RMODE_PRIVATE);
    
    	theTrack->filename = strdup(trackfile);

    	GetTrackHeader(TrackHandle);

    	switch(theTrack->version) 
	{
    		case 0:
    		case 1:
    		case 2:
    		case 3:
			ReadTrack3(theTrack, TrackHandle, &theCamList, 1);
			break;
    		case 4:
			ReadTrack4(theTrack, TrackHandle, &theCamList, 1);
         break;
         case 5:
         ReadTrack5(theTrack, TrackHandle, &theCamList, 1);
			break;

    	}
    
    	return theTrack;
}


/*
 * Function
 *	GetTrackHeader
 *
 * Description
 *	Get the header of the track file
 *	in order to know the number of segments
 * Parameters
 *	
 *
 * Return
 *	
 *
 * Remarks
 *	
 */
static void
GetTrackHeader(void *TrackHandle) {
    // Read header
    theTrack->name = GfParmGetStr(TrackHandle, TRK_SECT_HDR, TRK_ATT_NAME, "no name");
    theTrack->descr = GfParmGetStr(TrackHandle, TRK_SECT_HDR, TRK_ATT_DESCR, "no description");
    theTrack->version = (int)GfParmGetNum(TrackHandle, TRK_SECT_HDR, TRK_ATT_VERSION, (char*)NULL, 0);
    theTrack->width = GfParmGetNum(TrackHandle, TRK_SECT_MAIN, TRK_ATT_WIDTH, (char*)NULL, 15.0);
    theTrack->authors = GfParmGetStr(TrackHandle, TRK_SECT_HDR, TRK_ATT_AUTHOR, "none");
    theTrack->category = GfParmGetStr(TrackHandle, TRK_SECT_HDR, TRK_ATT_CAT, "road");
    theTrack->subcategory = GfParmGetStr(TrackHandle, TRK_SECT_HDR, TRK_ATT_SUBCAT, "none");

    // Read Local Info section
    tTrackLocalInfo *local = &theTrack->local;
    local->station = GfParmGetStr(TrackHandle, TRK_SECT_LOCAL, TRK_ATT_STATION, "LFPG");
    local->timezone = (int)GfParmGetNum(TrackHandle, TRK_SECT_LOCAL, TRK_ATT_TIMEZONE, (char*)NULL, 0);
    local->anyrainlkhood = GfParmGetNum(TrackHandle, TRK_SECT_LOCAL, TRK_ATT_ANYRAINLKHD, (char*)NULL, 0);
    local->littlerainlkhood = GfParmGetNum(TrackHandle, TRK_SECT_LOCAL, TRK_ATT_LITTLERAINLKHD, (char*)NULL, 0);
    local->mediumrainlkhood = GfParmGetNum(TrackHandle, TRK_SECT_LOCAL, TRK_ATT_MEDIUMRAINLKHD, (char*)NULL, 0);
    local->timeofday = GfParmGetNum(TrackHandle, TRK_SECT_LOCAL, TRK_ATT_TIMEOFDAY, (char*)NULL, (tdble)(15 * 3600 + 0 * 60 + 0)); // 15:00:00
    local->sunascension = GfParmGetNum(TrackHandle, TRK_SECT_LOCAL, TRK_ATT_SUN_ASCENSION, (char*)NULL, 0.0f);

    // Read Graphic section
    tTrackGraphicInfo *graphic = &theTrack->graphic;
    graphic->model3d = GfParmGetStr(TrackHandle, TRK_SECT_GRAPH, TRK_ATT_3DDESC, 0);
    graphic->background = GfParmGetStr(TrackHandle, TRK_SECT_GRAPH, TRK_ATT_BKGRND, "background.png");
    graphic->bgtype = (int)GfParmGetNum(TrackHandle, TRK_SECT_GRAPH, TRK_ATT_BGTYPE, (char*)NULL, 0.0);

    graphic->bgColor[0] = (float)GfParmGetNum(TrackHandle, TRK_SECT_GRAPH, TRK_ATT_BGCLR_R, (char*)NULL, 0.0f);
    graphic->bgColor[1] = (float)GfParmGetNum(TrackHandle, TRK_SECT_GRAPH, TRK_ATT_BGCLR_G, (char*)NULL, 0.0f);
    graphic->bgColor[2] = (float)GfParmGetNum(TrackHandle, TRK_SECT_GRAPH, TRK_ATT_BGCLR_B, (char*)NULL, 0.1f);

    // Environment map images
    char buf[256];
    sprintf(buf, "%s/%s", TRK_SECT_GRAPH, TRK_LST_ENV);
    graphic->envnb = GfParmGetEltNb(TrackHandle, buf);
    if (graphic->envnb < 1)
      graphic->envnb = 1;

    graphic->env = (const char**)calloc(graphic->envnb, sizeof(const char*));
    const char **env = graphic->env;
    for (int i = 1; i <= graphic->envnb; ++i) {
      sprintf(buf, "%s/%s/%d", TRK_SECT_GRAPH, TRK_LST_ENV, i);
      *env = GfParmGetStr(TrackHandle, buf, TRK_ATT_ENVNAME, "env.png");
      ++env;
    }

    // Track lights
    graphic->nb_lights = GfParmGetEltNb(TrackHandle, TRK_SECT_TRACKLIGHTS );
    GfLogDebug( "Number of lights: %d\n", graphic->nb_lights );
    if (graphic->nb_lights > 0 ) {
      graphic->lights = (tGraphicLightInfo*)malloc( sizeof( tGraphicLightInfo ) * graphic->nb_lights );
      for (int i = 0; i < graphic->nb_lights; ++i) {
        sprintf(buf, "%s/%d/%s", TRK_SECT_TRACKLIGHTS, i + 1, TRK_SECT_TOPLEFT);
        graphic->lights[ i ].topleft.x = GfParmGetNum(TrackHandle, buf, TRK_ATT_X, (char*)NULL, 0.0f);
        graphic->lights[ i ].topleft.y = GfParmGetNum(TrackHandle, buf, TRK_ATT_Y, (char*)NULL, 0.0f);
        graphic->lights[ i ].topleft.z = GfParmGetNum(TrackHandle, buf, TRK_ATT_Z, (char*)NULL, 0.0f);
        sprintf(buf, "%s/%d/%s", TRK_SECT_TRACKLIGHTS, i + 1, TRK_SECT_BOTTOMRIGHT);
        graphic->lights[ i ].bottomright.x = GfParmGetNum(TrackHandle, buf, TRK_ATT_X, (char*)NULL, 0.0f);
        graphic->lights[ i ].bottomright.y = GfParmGetNum(TrackHandle, buf, TRK_ATT_Y, (char*)NULL, 0.0f);
        graphic->lights[ i ].bottomright.z = GfParmGetNum(TrackHandle, buf, TRK_ATT_Z, (char*)NULL, 0.0f);
        sprintf(buf, "%s/%d", TRK_SECT_TRACKLIGHTS, i + 1);
        graphic->lights[ i ].onTexture = strdup(GfParmGetStr(TrackHandle, buf, TRK_ATT_TEXTURE_ON, ""));
        graphic->lights[ i ].offTexture = strdup(GfParmGetStr(TrackHandle, buf, TRK_ATT_TEXTURE_OFF, ""));
        graphic->lights[ i ].index = (int)GfParmGetNum(TrackHandle, buf, TRK_ATT_INDEX, (char*)NULL, 0.0f);
        graphic->lights[ i ].role = 0;
        if( strcmp( GfParmGetStr(TrackHandle, buf, TRK_ATT_ROLE, ""), "st_red" ) == 0 )
          graphic->lights[ i ].role = GR_TRACKLIGHT_START_RED;
        else if( strcmp( GfParmGetStr(TrackHandle, buf, TRK_ATT_ROLE, ""), "st_green" ) == 0 )
          graphic->lights[ i ].role = GR_TRACKLIGHT_START_GREEN;
        else if( strcmp( GfParmGetStr(TrackHandle, buf, TRK_ATT_ROLE, ""), "st_green_st" ) == 0 )
          graphic->lights[ i ].role = GR_TRACKLIGHT_START_GREENSTART;
        else if( strcmp( GfParmGetStr(TrackHandle, buf, TRK_ATT_ROLE, ""), "st_yellow" ) == 0 )
          graphic->lights[ i ].role = GR_TRACKLIGHT_START_YELLOW;
        graphic->lights[ i ].red = GfParmGetNum(TrackHandle, buf, TRK_ATT_RED, (char*)NULL, 1.0f);
        graphic->lights[ i ].green = GfParmGetNum(TrackHandle, buf, TRK_ATT_GREEN, (char*)NULL, 1.0f);
        graphic->lights[ i ].blue = GfParmGetNum(TrackHandle, buf, TRK_ATT_BLUE, (char*)NULL, 1.0f);
      }  // for i
    }  // if nb_lights

    theTrack->nseg = 0;

    // Search for track filename, without any path info, eg: 'foo.xml'
    const char *s = strrchr(theTrack->filename, '/');
    if (s == NULL) {
      s = theTrack->filename;
    } else {
      ++s;
    }

    // Internal name is track filename, without extension, eg: 'foo'
    theTrack->internalname = strdup(s);
    char *cs = strrchr(theTrack->internalname, '.');
    if (cs != NULL) {
      *cs = 0;
    }

    // Default turnmark is 1m*1m, right next to the track
    graphic->turnMarksInfo.height = GfParmGetNum(TrackHandle, TRK_SECT_TURNMARKS, TRK_ATT_HEIGHT, NULL, 1);
    graphic->turnMarksInfo.width  = GfParmGetNum(TrackHandle, TRK_SECT_TURNMARKS, TRK_ATT_WIDTH,  NULL, 1);
    graphic->turnMarksInfo.vSpace = GfParmGetNum(TrackHandle, TRK_SECT_TURNMARKS, TRK_ATT_VSPACE, NULL, 0);
    graphic->turnMarksInfo.hSpace = GfParmGetNum(TrackHandle, TRK_SECT_TURNMARKS, TRK_ATT_HSPACE, NULL, 0);
}  // GetTrackHeader


/**
 * This function initialize some values which can only be done after the track is loaded.
 * 
 * This function for example defines the sector start and ends
 *
 * @param TrackHandle The handle containing the information about the track
 */
static void
FinishTrackLoading(void* TrackHandle)
{
	double *distances = NULL;
	double currentDistance;
	double tmpDistance;
	int currentLength;
	int xx;

	theTrack->numberOfSectors = GfParmGetEltNb(TrackHandle, TRK_SECT_SECTORS);

	if (theTrack->numberOfSectors < 0)
		theTrack->numberOfSectors = 0;
	//TODO(kilo): possible divison by zero!!!
	if (theTrack->length / (double)theTrack->numberOfSectors < 100.0f )
	{
		theTrack->numberOfSectors = (int)floor( theTrack->length / 100.0f );
		GfOut( "WARNING: too many sectors" );
	}

	if (theTrack->numberOfSectors == 0)
	{
		/* Default is:
		 *   1 sector on circuits of 1km or shorter
		 *   3 sectors on circuits between 1km and 6km
		 *   the minimum number of sectors such that every sector is at most 2km if the track is longer then 6km
		 *
		 *   Note: the sector end at start-finish is added later
		 */
		if (theTrack->length < 1000.0f)
			theTrack->numberOfSectors = 0;
		else if(theTrack->length < 6000.0f)
			theTrack->numberOfSectors = 2;
		else
			theTrack->numberOfSectors = (int)floor( theTrack->length / 2000.0f );

		if (theTrack->numberOfSectors > 0)
		{
			distances = (double*)malloc( sizeof( double ) * theTrack->numberOfSectors );
			for( xx = 0; xx < theTrack->numberOfSectors; ++xx )
				distances[ xx ] = theTrack->length * (double)(xx + 1) / (double)(theTrack->numberOfSectors + 1);
		}
	}
	else
	{
		distances = (double*)malloc( sizeof( double ) * theTrack->numberOfSectors );
		currentLength = 0;

		if (GfParmListSeekFirst( TrackHandle, TRK_SECT_SECTORS ) == 0)
		{
			do
			{
				currentDistance = GfParmGetCurNum( TrackHandle, TRK_SECT_SECTORS, TRK_ATT_SECTOR_DFS, NULL, 0.0f);
				if (currentDistance <= 0.0f || currentDistance >= theTrack->length)
					continue; /* Don't add the startline as sector */
				for (xx = 0; xx < currentLength; ++xx)
				{
					if (distances[xx] > currentDistance)
					{
						tmpDistance = distances[xx];
						distances[xx] = currentDistance;
						currentDistance = tmpDistance;
					}
				}

				distances[currentLength] = currentDistance;
				++currentLength;
			} while (GfParmListSeekNext(TrackHandle, TRK_SECT_SECTORS) == 0);
		}

		theTrack->numberOfSectors = currentLength;
	}

	/* All know, now allocte the structures with the right size */
	if (theTrack->numberOfSectors > 0)
	{
		theTrack->sectors = (double*)malloc( sizeof(double) * theTrack->numberOfSectors );

		for( xx = 0; xx < theTrack->numberOfSectors; ++xx )
			theTrack->sectors[xx] = distances[xx];
	}
	else
	{
		theTrack->sectors = NULL;
	}

	/* Add the finish line as last sector */
	++theTrack->numberOfSectors;

	/* Free unused memory */
	if (distances)
		free( distances );
}

static void
freeSeg(tTrackSeg *seg)
{
	if (seg->barrier[0]) {
		free(seg->barrier[0]);
	}
	if (seg->barrier[1]) {
		free(seg->barrier[1]);
	}
	if (seg->ext) {
		free(seg->ext->marks);
		free(seg->ext);
	}
	if (seg->lside) {
		freeSeg(seg->lside);
	}
	if (seg->rside) {
		freeSeg(seg->rside);
	}
	free(seg);
}

void
TrackShutdown(void)
{
	tTrackSeg *curSeg;
	tTrackSeg *nextSeg;
	tTrackSurface *curSurf;
	tTrackSurface *nextSurf;
	tRoadCam *curCam;
	tRoadCam *nextCam;
	int xx;

	if (!theTrack) {
		return;
	}

	nextSeg = theTrack->seg->next;
	do {
		curSeg = nextSeg;
		nextSeg = nextSeg->next;
		freeSeg(curSeg);
	} while (curSeg != theTrack->seg);

	curSurf = theTrack->surfaces;
	while (curSurf) {
		nextSurf = curSurf->next;
		free(curSurf);
		curSurf = nextSurf;
	}

	curCam = theCamList;
	if (curCam) {
	do {
		nextCam = curCam->next;
		free(curCam);
		curCam = nextCam;
	} while (curCam != theCamList);
	}
	theCamList = NULL;

	if (theTrack->pits.driversPits) free(theTrack->pits.driversPits);
	free(theTrack->graphic.env);
	if(theTrack->graphic.nb_lights > 0)
	{
	    for (xx = 0; xx < theTrack->graphic.nb_lights; ++xx)
	    {
	        free(theTrack->graphic.lights[ xx ].onTexture);
	        free(theTrack->graphic.lights[ xx ].offTexture);
	    }
	    free(theTrack->graphic.lights);
	}
	free(theTrack->internalname);
	free(theTrack->filename);
	if (theTrack->sectors)
		free(theTrack->sectors);
	free(theTrack);

	GfParmReleaseHandle(TrackHandle);
	theTrack = NULL;
}
