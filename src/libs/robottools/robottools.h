/***************************************************************************

    file                 : robottools.h
    created              : Mon Feb 28 22:31:13 CET 2000
    copyright            : (C) 2000 by Eric Espie
    email                : torcs@free.fr
    version              : $Id: robottools.h 2917 2010-10-17 19:03:40Z pouillot $

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
    		Robots Tools
    @author	<a href=mailto:eric.espie@torcs.org>Eric Espie</a>
    @version	$Id: robottools.h 2917 2010-10-17 19:03:40Z pouillot $
*/

#ifndef _ROBOTTOOLS_H_
#define _ROBOTTOOLS_H_

#include <car.h>
#include <track.h>

// DLL exported symbols declarator for Windows.
#ifdef WIN32
# ifdef ROBOTTOOLS_DLL
#  define ROBOTTOOLS_API __declspec(dllexport)
# else
#  define ROBOTTOOLS_API __declspec(dllimport)
# endif
#else
# define ROBOTTOOLS_API
#endif


#define RELAXATION2(target, prev, rate)			\
do {								\
    tdble __tmp__;						\
    __tmp__ = target;						\
    target = (prev) + (rate) * ((target) - (prev)) * 0.01;	\
    prev = __tmp__;						\
} while (0)

#define FLOAT_RELAXATION2(target, prev, rate) 			\
do {								\
    tdble __tmp__;						\
    __tmp__ = target;						\
    target = (prev) + (rate) * ((target) - (prev)) * 0.01f;	\
    prev = __tmp__;						\
} while (0)

#define RELAXATION(target, prev, rate) 				\
do {								\
    target = (prev) + (rate) * ((target) - (prev)) * 0.01;	\
    prev = (target);						\
} while (0)

#define FLOAT_RELAXATION(target, prev, rate) 				\
do {								\
    target = (prev) + (rate) * ((target) - (prev)) * 0.01f;	\
    prev = (target);						\
} while (0)


/*******************
 * Track Utilities *
 *******************/

/* for variable width segments */
ROBOTTOOLS_API tdble RtTrackGetWidth(tTrackSeg *seg, tdble toStart);

/*
 * Convert a Local position (segment, toRight, toStart)
 * into a Global one (X, Y)
 *
 * The ToStart position refers to the current segment,
 * the function will not search for next segment if toStart
 * is greater than the segment length.
 * toStart represent an angle in radian for curves
 * and a length in meters for straights.
 *
 */
ROBOTTOOLS_API void RtTrackLocal2Global(tTrkLocPos *p, tdble *X, tdble *Y, int flag);

/*
 * Convert a Global (segment, X, Y) position into a Local one (segment, toRight, toStart)
 *
 * The segment in the Global position is used to start the search of a good segment
 * in term of toStart value.
 * The segments are scanned in order to find a toStart value between 0 and the length
 * of the segment for straights or the arc of the curve.
 *
 * The sides parameters is to indicate wether to use the track sides (1) or not (0) in
 * the toRight computation.
 */
ROBOTTOOLS_API void RtTrackGlobal2Local(tTrackSeg *segment, tdble X, tdble Y, tTrkLocPos *p, int type);

/*
 * Returns the absolute height in meters of the road
 * at the Local position p.
 * 
 * If the point lies outside the track (and sides)
 * the height is computed using the tangent to the banking
 * of the segment (or side).

                + Point given
               .^
              . |
             .  |
            .   |
           /    | heigth
          /     |
   ______/      v
   ^    ^^  ^
   |    ||  |
    track side

 */
ROBOTTOOLS_API tdble RtTrackHeightL(tTrkLocPos *p);

/*
 * Returns the absolute height in meters of the road
 * at the Global position (segment, X, Y)
 */
ROBOTTOOLS_API tdble RtTrackHeightG(tTrackSeg *seg, tdble X, tdble Y);

/*
 * Give the normal vector of the border of the track
 * including the sides.
 *
 * The side parameter is used to indicate the right (TR_RGT)
 * of the left (TR_LFT) side to consider.
 *
 * The Global position given (segment, X, Y) is used
 * to project the point on the border, it is not necessary
 * to give a point directly on the border itself.
 *
 * The vector is normalized.
 */
ROBOTTOOLS_API void RtTrackSideNormalG(tTrackSeg *seg, tdble X, tdble Y, int side, t3Dd *norm);

/*
 * Used to get the tangent angle for a track position
 * The angle is given in radian.
 *
 * the angle 0 is parallel to the first segment start.
 */
ROBOTTOOLS_API tdble RtTrackSideTgAngleL(tTrkLocPos *p);

/*
 * Used to get the normal vector of the road itself (pointing
 * upward).
 *
 * Local coordinates are used to locate the point where to
 * get the road normal vector.
 *
 * The vector is normalized.
 */
ROBOTTOOLS_API void RtTrackSurfaceNormalL(tTrkLocPos *p, t3Dd *norm);

/** Get the current segment
 */
ROBOTTOOLS_API tTrackSeg *RtTrackGetSeg(tTrkLocPos *p);

ROBOTTOOLS_API int RtDistToPit(struct CarElt *car, tTrack *track, tdble *dL, tdble *dW);

ROBOTTOOLS_API tdble RtGetDistFromStart(tCarElt *car);
ROBOTTOOLS_API tdble RtGetDistFromStart2(tTrkLocPos *p);


/****************
 * Telemetry    *
 ****************/

/** Initialization of a telemetry session.
    @param	ymin	Minimum value for Y.
    @param	ymax	Maximum value for Y.
    @return	None
 */
ROBOTTOOLS_API void RtTelemInit(tdble ymin, tdble ymax);

/** Create a new telemetry channel.
    @param	name	Name of the channel.
    @param	var	Address of the variable to monitor.
    @param	min	Minimum value of this variable.
    @param	max	Maximum value of this variable.
    @return	None
 */
ROBOTTOOLS_API void RtTelemNewChannel(const char * name, tdble * var, tdble min, tdble max);
ROBOTTOOLS_API void RtTelemStartMonitoring(const char * filename);
ROBOTTOOLS_API void RtTelemStopMonitoring(void);
ROBOTTOOLS_API void RtTelemUpdate(double time);
ROBOTTOOLS_API void RtTelemShutdown(void);


/********************
 * Miscellaneous    *
 ********************/

/**
 * A utility function used to get the string of a index number, or the string of the carname.
 *
 * @param index The index of the robot
 * @param bot_dname The dll name (or so name) of the robot
 * @param extended TRUE if the car is an extended one; FALSE otherwise
 * @param result The resulting char* string. If must be already allocated. The contents of the parameter will be changed.
 * @param resultLength The length of the result array
 */
ROBOTTOOLS_API void RtGetCarindexString(int index, const char *bot_dname, char careerMode,
										char *result, int resultLength);

#endif /* _ROBOTTOOLS_H_ */
