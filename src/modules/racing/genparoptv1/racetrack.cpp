/***************************************************************************

    file        : racetrack.cpp
    copyright   : (C) 2010 by Xavier Bertaux
    web         : www.speed-dreams.org 
    version     : $Id: racetrack.cpp 5863 2014-11-26 12:50:06Z wdbee $

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
    		Track related functions
    @author	    Xavier Bertaux
    @version	$Id: racetrack.cpp 5863 2014-11-26 12:50:06Z wdbee $
*/

#include <cstdlib>
#include <cstdio>
#include <ctime>

#include <tgf.h>
#include <portability.h>

#include <raceman.h>
#include <track.h>

#include <tracks.h>

#include "genparoptv1.h"

#include "racesituation.h"
#include "raceinit.h"
#include "racetrack.h"


// Local functions.
//static void reTrackDump(const tTrack *track, int verbose);
static void reTrackInitTimeOfDay(void);
static void reTrackInitWeather(void);
static void reTrackUpdatePhysics(void);

/** Initialize the track for a race manager.
    @return <tt>0 ... </tt>Ok<br>
    <tt>-1 .. </tt>Error
*/
int
ReTrackInit(void)
{
	char buf[256];
	
	const char  *trackName;
	const char  *catName;

	const int curTrkIdx =
		(int)GfParmGetNum(ReInfo->params, RE_SECT_CURRENT, RE_ATTR_CUR_TRACK, NULL, 1);
	snprintf(buf, sizeof(buf), "%s/%d", RM_SECT_TRACKS, curTrkIdx);
	trackName = GfParmGetStr(ReInfo->params, buf, RM_ATTR_NAME, 0);
	if (!trackName)
		return -1;

	catName = GfParmGetStr(ReInfo->params, buf, RM_ATTR_CATEGORY, 0);
	if (!catName) 
		return -1;

	snprintf(buf, sizeof(buf), "tracks/%s/%s/%s.%s", catName, trackName, trackName, TRKEXT);
	ReInfo->track = ReTrackLoader().load(buf);

	snprintf(buf, sizeof(buf), "Loading %s track", ReInfo->track->name);
	ReUI().addOptimizationMessage(buf);

	reTrackInitTimeOfDay();
	reTrackInitWeather();

	//reTrackDump(ReInfo->track, 0);

	return 0;
}//ReTrackInit

/** Dump the track segments on screen
    @param  track track to dump
    @param  verbose if set to 1 all the segments are described (long)
    @ingroup  racemantools
 */
static void
reTrackDump(const tTrack *track, int verbose)
{
	char buf[128];
	
	snprintf(buf, sizeof(buf), "  by %s (%.0f m long, %.0f m wide) ...", 
			 track->authors, track->length, track->width);
	ReUI().addOptimizationMessage(buf);

	GfLogInfo("++++++++++++ Track ++++++++++++\n");
	GfLogInfo("Name     = %s\n", track->name);
	GfLogInfo("Authors  = %s\n", track->authors);
	GfLogInfo("Filename = %s\n", track->filename);
	GfLogInfo("NSeg     = %d\n", track->nseg);
	GfLogInfo("Version  = %d\n", track->version);
	GfLogInfo("Length   = %f m\n", track->length);
	GfLogInfo("Width    = %f m\n", track->width);
	GfLogInfo("XSize    = %f m\n", track->max.x);
	GfLogInfo("YSize    = %f m\n", track->max.y);
	GfLogInfo("ZSize    = %f m\n", track->max.z);
  
	switch (track->pits.type) {
		case TR_PIT_NONE:
			GfLogInfo("Pits     = none\n");
			break;
      
		case TR_PIT_ON_TRACK_SIDE:
			GfLogInfo("Pits     = present on track side\n");
			break;
      
		case TR_PIT_ON_SEPARATE_PATH:
			GfLogInfo("Pits     = present on separate path\n");
			break;

		case TR_PIT_NO_BUILDING:
			GfLogInfo("Pits     = present, no building style\n");
			break;
    }//switch pits.type

	const int seconds = (int)track->local.timeofday;
	GfLogInfo("TimeOfDay= %02d:%02d:%02d\n", seconds / 3600, (seconds % 3600) / 60, seconds % 60);
	GfLogInfo("Sun asc. = %.1f d\n", RAD2DEG(track->local.sunascension));
	GfLogInfo("Clouds   = %d (0=none, 1=few, 2=scarce, 3=many, 4=full)\n", track->local.clouds);
	GfLogInfo("Rain     = %d (0=none, 1=little, 2=medium, 3=heavy)\n", track->local.rain);
	GfLogInfo("Water    = %d (0=none, 1=some, 2=more, 3=swampy)\n", track->local.water);

	if (verbose) {
		int i;
		tTrackSeg *seg;
#ifdef SD_DEBUG
		const char  *stype[4] = { "", "RGT", "LFT", "STR" };
#endif

		for (i = 0, seg = track->seg->next; i < track->nseg; i++, seg = seg->next) {
			GfLogTrace("  segment %d -------------- \n", seg->id);
#ifdef SD_DEBUG
			GfLogTrace("        type    %s\n", stype[seg->type]);
#endif
			GfLogTrace("        length  %f m\n", seg->length);
			GfLogTrace("  radius  %f m\n", seg->radius);
			GfLogTrace("  arc %f d Zs %f d Ze %f d Zcs %f d\n", RAD2DEG(seg->arc),
					   RAD2DEG(seg->angle[TR_ZS]),
					   RAD2DEG(seg->angle[TR_ZE]),
					   RAD2DEG(seg->angle[TR_CS]));
			GfLogTrace(" Za  %f d\n", RAD2DEG(seg->angle[TR_ZS]));
			GfLogTrace("  vertices: %-8.8f %-8.8f %-8.8f ++++ ",
					   seg->vertex[TR_SR].x,
					   seg->vertex[TR_SR].y,
					   seg->vertex[TR_SR].z);
			GfLogTrace("%-8.8f %-8.8f %-8.8f\n",
					   seg->vertex[TR_SL].x,
					   seg->vertex[TR_SL].y,
					   seg->vertex[TR_SL].z);
			GfLogTrace("  vertices: %-8.8f %-8.8f %-8.8f ++++ ",
					   seg->vertex[TR_ER].x,
					   seg->vertex[TR_ER].y,
					   seg->vertex[TR_ER].z);
			GfLogTrace("%-8.8f %-8.8f %-8.8f\n",
					   seg->vertex[TR_EL].x,
					   seg->vertex[TR_EL].y,
					   seg->vertex[TR_EL].z);
			GfLogTrace("  prev    %d\n", seg->prev->id);
			GfLogTrace("  next    %d\n", seg->next->id);
		}//for i
		GfLogTrace("From Last To First\n");
		GfLogTrace("Dx = %-8.8f  Dy = %-8.8f Dz = %-8.8f\n",
				   track->seg->next->vertex[TR_SR].x - track->seg->vertex[TR_ER].x,
				   track->seg->next->vertex[TR_SR].y - track->seg->vertex[TR_ER].y,
				   track->seg->next->vertex[TR_SR].z - track->seg->vertex[TR_ER].z);
    }//if verbose
}//reTrackDump


// Initialize track time of day from race settings
void
reTrackInitTimeOfDay(void)
{
	static const char *TimeOfDayValues[] = RM_VALS_TIME;
	static const int NTimeOfDayValues = sizeof(TimeOfDayValues) / sizeof(const char*);
	
	tTrackLocalInfo *trackLocal = &ReInfo->track->local;

	// Load time of day settings for the session
	// (defaults to  "All sesions" one, or else "afternoon").
	int timeofday = RM_IND_TIME_AFTERNOON;
	const char* pszTimeOfDay =
		GfParmGetStr(ReInfo->params, ReInfo->_reRaceName, RM_ATTR_TIME_OF_DAY, 0);
	if (!pszTimeOfDay)
		 pszTimeOfDay =
			 GfParmGetStr(ReInfo->params, RM_VAL_ANYRACE, RM_ATTR_TIME_OF_DAY, RM_VAL_TIME_AFTERNOON);
	for (int i = 0; i < NTimeOfDayValues; i++)
		if (!strcmp(pszTimeOfDay, TimeOfDayValues[i]))
		{
			timeofday = i;
			break;
		}

	trackLocal->timeofdayindex = timeofday;
	switch (timeofday) 
	{
		case RM_IND_TIME_DAWN:
			trackLocal->timeofday = 6 * 3600 + 13 * 60 + 20; // 06:13:20
			break;
					
		case RM_IND_TIME_MORNING:
			trackLocal->timeofday = 10 * 3600 + 0 * 60 + 0; // 10:00:00
			break;
					
		case RM_IND_TIME_NOON:
		case RM_IND_TIME_24HR:
			trackLocal->timeofday = 12 * 3600 + 0 * 60 + 0; // 12:00:00
			break;
					
		case RM_IND_TIME_AFTERNOON:
			trackLocal->timeofday = 15 * 3600 + 0 * 60 + 0; // 15:00:00
			break;
					
		case RM_IND_TIME_DUSK:
			trackLocal->timeofday = 17 * 3600 + 46 * 60 + 40; // 17:46:40
			break;
					
		case RM_IND_TIME_NIGHT:
			trackLocal->timeofday = 0 * 3600 + 0 * 60 + 0; // Midnight = 00:00:00
			break;
		case RM_IND_TIME_REAL:					
		case RM_IND_TIME_NOW:
		{
			time_t t = time(0);
			struct tm *ptm = localtime(&t);
			trackLocal->timeofday = ptm->tm_hour * 3600.0f + ptm->tm_min * 60.0f + ptm->tm_sec;
			GfLogDebug("  Now time of day\n");
			break;
		}

		case RM_IND_TIME_TRACK:
			// Already loaded by the track loader (or else default value).
			GfLogDebug("  Track-defined time of day\n");
			break;

		case RM_IND_TIME_RANDOM:			
			trackLocal->timeofday = (tdble)(rand() % (24*60*60));
			break;

		default:
			trackLocal->timeofday = 15 * 3600 + 0 * 60 + 0; // 15:00:00
			trackLocal->timeofdayindex = RM_IND_TIME_AFTERNOON;
			GfLogError("Unsupported value %d for user timeofday (assuming 15:00)\n",
					   timeofday);
			break;
				
	}//switch timeofday

}

// Initialize track weather info from race settings
void
reTrackInitWeather(void)
{
	static const char* CloudsValues[] = RM_VALS_CLOUDS;
	static const int NCloudsValues = sizeof(CloudsValues) / sizeof(const char*);
	
	static const char *RainValues[] = RM_VALS_RAIN;
	static const int NRainValues = sizeof(RainValues) / sizeof(const char*);

	tTrackLocalInfo *trackLocal = &ReInfo->track->local;

	// Load cloud cover settings for the session
	// (defaults to  "All sesions" one, or else "none").
	int clouds = TR_CLOUDS_NONE;
	const char* pszClouds =
		GfParmGetStr(ReInfo->params, ReInfo->_reRaceName, RM_ATTR_CLOUDS, 0);
	if (!pszClouds)
		pszClouds =
			GfParmGetStr(ReInfo->params, RM_VAL_ANYRACE, RM_ATTR_CLOUDS, RM_VAL_CLOUDS_NONE);
	for (int i = 0; i < NCloudsValues; i++)
		if (!strcmp(pszClouds, CloudsValues[i]))
		{
			clouds = i;
			break;
		}

	// Load rain fall (and track dry/wet conditions) settings for the session
	// if feature supported (defaults to  "All sesions" one, or else "none").
	int rain = TR_RAIN_NONE;
	if (ReInfo->s->_features & RM_FEATURE_WETTRACK)
	{
		const char* pszRain =
			GfParmGetStr(ReInfo->params, ReInfo->_reRaceName, RM_ATTR_RAIN, 0);
		if (!pszRain)
			pszRain =
				GfParmGetStr(ReInfo->params, RM_VAL_ANYRACE, RM_ATTR_RAIN, RM_VAL_RAIN_NONE);
		for (int i = 0; i < NRainValues; i++)
			if (!strcmp(pszRain, RainValues[i]))
			{
				rain = i;
				break;
			}
	}

	// Take care of the random case for rain falls and ground water.
	const bool bRandomRain = (rain == TR_RAIN_RANDOM);
	if (bRandomRain)
	{
		// Force random clouds, in case there is no rain at the end.
		clouds = TR_CLOUDS_RANDOM;
		
		// Random rain (if random[0,1] < trackLocal->anyrainlkhood, then it rains).
		const tdble randDraw = (tdble)(rand()/(double)RAND_MAX);

		GfLogTrace("Rain likelyhoods : overall=%.2f, little=%.2f, medium=%.2f\n",
				   trackLocal->anyrainlkhood, trackLocal->littlerainlkhood,
				   trackLocal->mediumrainlkhood);
		GfLogDebug("Overall rain random draw = %.2f,\n", randDraw);
		if (randDraw < trackLocal->anyrainlkhood)
		{
			// Now, let's determine how much it rains :
			// if random[0,1] < little rain likelyhood => rain = little rain
			const tdble randDraw2 = (tdble)(rand()/(double)RAND_MAX);
			GfLogDebug("Specific rain random draw = %.2f,\n", randDraw2);
			if (randDraw2 < trackLocal->littlerainlkhood)
				rain = TR_RAIN_LITTLE;
			// else if random[0,1] < medium + little rain likelyhood => rain = medium rain
			else if (randDraw2 <  trackLocal->littlerainlkhood + trackLocal->mediumrainlkhood)
				rain = TR_RAIN_MEDIUM;
			// otherwise, random[0,1] >= medium + little rain likelyhood => rain = Heavy rain
			else
				rain = TR_RAIN_HEAVY;
		}
		else
		{
			// No Rain.
			rain = TR_RAIN_NONE;
		}
	}
	
	// Take care of the random case for clouds cover.
	const bool bRandomClouds = (clouds == TR_CLOUDS_RANDOM);
	if (bRandomClouds)
	{
		if (rain != TR_RAIN_NONE)
		{
			// If any rain level, heavy clouds.
			clouds = TR_CLOUDS_FULL;
		}
		else
		{
			// Really random clouds.
			clouds = rand() % (TR_CLOUDS_FULL + 1);
		}
	}

	// Ground water = rain for the moment (might change in the future).
	const int water = rain;
	
	GfLogInfo("Weather : Using %s rain (%d) and ground water (%d) + %s clouds (%d) settings\n",
			  bRandomRain ? "random" : "user defined", rain, water,
			  bRandomClouds ? "random" : "user defined", clouds);
	
	// Update track local info.
	trackLocal->rain = rain;
	trackLocal->clouds = clouds;
	trackLocal->water = water;

	// Update track physics from computed local info.
	reTrackUpdatePhysics();
}

// Update track info ...
void
ReTrackUpdate(void)
{
	// TODO: New weather conditions when starting a new event ?
	
	reTrackUpdatePhysics();
}

// Update Track Physics (compute kFriction from current "water level" on ground).
void
reTrackUpdatePhysics(void)
{
	tTrackLocalInfo *trackLocal = &ReInfo->track->local;

	// Get the wet / dry friction coefficients ratio.
	void* hparmTrackConsts =
		GfParmReadFile(TRK_PHYSICS_FILE, GFPARM_RMODE_STD | GFPARM_RMODE_CREAT);
	const tdble kFrictionWetDryRatio = 
		GfParmGetNum(hparmTrackConsts, TRKP_SECT_SURFACES, TRKP_VAL_FRICTIONWDRATIO, (char*)NULL, 0.5f);
	GfParmReleaseHandle(hparmTrackConsts);
	
	// Determine the "wetness" of the track (inside  [0, 1]).
	const tdble wetness = (tdble)trackLocal->water / TR_WATER_MUCH;

	GfLogDebug("ReTrackUpdate : water = %d, wetness = %.2f, wet/dry mu = %.4f\n",
			   trackLocal->water, wetness, kFrictionWetDryRatio);

	// Set the actual friction for each _ground_ surface of the track.
	GfLogDebug("ReTrackUpdate : kFriction | kRollRes | Surface :\n");
	tTrackSurface *curSurf;
	curSurf = ReInfo->track->surfaces;
	do
	{
		// Linear interpolation of kFriction from dry to wet according to wetness.
		curSurf->kFriction =
			curSurf->kFrictionDry * (1 - wetness)
			+ curSurf->kFrictionDry * kFrictionWetDryRatio * wetness;

		// For the moment, we don't change curSurf->kRollRes (might change in the future).
			
		GfLogDebug("                   %.4f |   %.4f | %s\n",
				   curSurf->kFriction, curSurf->kRollRes, curSurf->material);

		curSurf = curSurf->next;

	} while ( curSurf );
}

/** Shutdown the track for a race manager.
    @return <tt>0 ... </tt>Ok<br>
    <tt>-1 .. </tt>Error
*/
int
ReTrackShutdown(void)
{
	return 0;
}

