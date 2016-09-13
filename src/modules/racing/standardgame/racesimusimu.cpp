/**************************************************************************

    file        : racesimusimu.cpp
    copyright   : (C) 2007 by Mart Kelder                 
    web         : http://speed-dreams.sourceforge.net   
    version     : $Id: racesimusimu.cpp 5803 2014-07-30 03:19:34Z mungewell $

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
    		Simulation simulation
*/

#include <tgf.h>
#include <raceman.h>

#include "racesituation.h"
#include "racecars.h"

#include "racesimusimu.h"


/* The idea of SimuSimu is that the result is simulated.
 * For that, various parameters are used:
 *
 * - Car parameters, as stated in the xml-file of the car (or default parameters). The car parameters can be overruled by robot setup files;
 * - Track parameters, as stated in the track xml-file (or default parameters if there is noting in the xml-file);
 * - Robot parameters / robot skills 
 *
 * Each state can use the already loaded parameters by using <attform>'s.
 *
 * The parameters are:
 *   CARS:
 *     - Top speed (m/s)
 *     - Time to accelerate from 0 to 250/9 m/s (=100 km/u) (s)
 *   TRACK:
 *     - % accelerate in one lap [0,1]
 *     - % brake in one lap [0,1]
 *     - % height-speed in one lap (note: hight-speed and low speed are not good defined yet) [0,1]
 *     - % low-speed in one lap [0,1]
 *     - Estimated lap time (given car parameters) [s]
 *   ROBOT:
 *     - Average additional time per lap [s].
 *     - Standard deviation [s]
 *     - Overtaking time penaty [s]
 *     - Lapping time penalty [s]
 *     - Unlapping time penalty [s]
 */

typedef struct CarParam
{
	float top_speed;
	float accel_time;
} tCarParam;

typedef struct TrackParam
{
	float proc_accel;
	float proc_brake;
	float proc_heightspeed;
	float proc_lowspeed;
} tTrackParam;

typedef struct RobotParam
{
	tCarElt *car;
	tCarParam *carParam;
	tTrackParam *track;
	float lap_time_mu;
	float lap_time_sigma;
	float pen_overtake;
	float pen_lap;
	float pen_unlap;
} tRobotParam;

typedef struct CarData
{
	int trackPos;
	int index;
} tCarData;

typedef struct SimuSimuData
{
	int nrRobots;
	tRobotParam *robots;
	tCarData *cars;
} tSimuSimuData;

static void ReSSInitCarParam( tCarParam *carParam, tCarElt *car )
{
	carParam->top_speed = 100.0f;
	carParam->accel_time = 20.0f;
}

static void ReSSInitTrackParam( tCarParam *carParam, tTrackParam *trackParam, tTrack *track )
{
	trackParam->proc_accel = 0.65f;
	trackParam->proc_brake = 0.30f;
	trackParam->proc_heightspeed = 0.5f;
	trackParam->proc_lowspeed = 0.5f;
}

static void ReSSInitRobotParam( tRobotParam *robotParam, tCarElt *car )
{
	robotParam->lap_time_mu = 60.0f;
	robotParam->lap_time_sigma = 1.5f;
	robotParam->pen_overtake = 1.3f;
	robotParam->pen_lap = 0.3f;
	robotParam->pen_unlap = 1.6f;
	robotParam->car = car;
}

static void ReSStCarInit( tCarElt *car )
{
	car->_curTime = 0.3f * car->_pos;
	car->_bestLapTime = 0.0f;
	car->_laps = 0;
	car->_bestLap = 0;
}

static tSimuSimuData* ReSSInit()
{
	int xx;
	tSimuSimuData *ret = (tSimuSimuData*)malloc( sizeof( tSimuSimuData ) );

	ret->nrRobots = ReInfo->s->_ncars;
	ret->robots = (tRobotParam*)malloc( sizeof( tRobotParam ) * ret->nrRobots );
	ret->cars = (tCarData*)malloc( sizeof( tCarData ) * ret->nrRobots );

	for( xx = 0; xx < ReInfo->s->_ncars; ++xx )
	{
		ret->robots[ xx ].track = (tTrackParam*)malloc( sizeof( tTrackParam ) );
		ret->robots[ xx ].carParam = (tCarParam*)malloc( sizeof( tCarParam ) );
		
		ReSSInitCarParam( ret->robots[ xx ].carParam, ReInfo->s->cars[ xx ] );
		ReSSInitTrackParam( ret->robots[ xx ].carParam, ret->robots[ xx ].track, ReInfo->track );
		ReSSInitRobotParam( &(ret->robots[ xx ]), ReInfo->s->cars[ xx ] );
		ReSStCarInit( ReInfo->s->cars[ xx ] );

		ret->cars[ xx ].trackPos = xx;
		ret->cars[ xx ].index = ReInfo->s->cars[ xx ]->index;
	}

	return ret;
}

static void ReSSFree( SimuSimuData *data )
{
	int xx;

	for( xx = 0; xx < data->nrRobots; ++xx )
	{
		free( data->robots[ xx ].carParam );
		free( data->robots[ xx ].track );
	}

	free( data->robots );
	free( data->cars );
	free( data );
}

static void ReSSStep( SimuSimuData *data )
{
	int xx;
	double laptime;
	tCarElt *car;

	//Search carindex
	car = ReInfo->s->cars[ 0 ];
	for( xx = 1; xx < ReInfo->s->_ncars; ++xx )
	{
		if( ReInfo->s->cars[ xx ]->_curTime < car->_curTime )
			car = ReInfo->s->cars[ xx ];
	}

	//Check if race has ended
	//GfLogDebug("ReSSStep: %s laps = %d / %d.\n", car->_name, car->_laps, ReInfo->s->_totLaps);
	if( car->_laps >= ReInfo->s->_totLaps )
	{
		ReInfo->s->_raceState = RM_RACE_ENDED;
		//GfLogDebug("ReSSStep: Race completed.\n");
		return;
	}

	//Calculate laptime
	laptime = 120.0f - 1.5f * car->_driveSkill;
	laptime += 16.0f * ( (double)rand() / (double)RAND_MAX ) - 8.0f;

	//Change structure
	car->_curTime += laptime;
	if( car->_bestLapTime > laptime || car->_bestLapTime == 0.0f ) {
		car->_bestLapTime = laptime;
		car->_bestLap = car->_laps;
	}
	++car->_laps;
}

static int ReSSSortFunc( const void *void1, const void *void2 )
{
	tCarElt *car1 = *((tCarElt**)void1);
	tCarElt *car2 = *((tCarElt**)void2);

	if( car1->_laps != car2->_laps ) {
		if( car1->_laps > car2->_laps )
			return -1;
		else
			return 1;
	} else if( car1->_curTime < car2->_curTime ) {
		return -1;
	} else if( car1->_curTime > car2->_curTime ) {
		return 1;
	} else {
		return 0;
	}
}

static void ReSSSort( tSituation *s )
{
	qsort( s->cars, s->_ncars, sizeof( tCarElt* ), ReSSSortFunc );
}

void ReSimuSimu()
{
	int xx;
	tSimuSimuData *data;

	data = ReSSInit();

	while( ! ( ReInfo->s->_raceState & RM_RACE_ENDED ) )
		ReSSStep( data );

	ReSSSort( ReInfo->s );
	ReSSFree( data );

	for( xx = 0; xx < ReInfo->s->_ncars; ++xx )
		ReInfo->s->cars[ xx ]->_state |= RM_CAR_STATE_FINISH;

	ReCarsSortCars();
}

