/***************************************************************************

    file                     : OsgMath.h
    created                  : Fri Aug 18 00:00:41 CEST 2012
    copyright                : (C) 2012 by Xavier Bertaux
    email                    : bertauxx@yahoo.fr
    version                  : $Id: OsgMath.h 4693 2012-04-13 03:12:09Z torcs-ng $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include <portability.h>
#include <stdlib.h>
#ifndef _OSGMATH_H_
#define _OSGMATH_H_

#define SD_ZERO  0.0
#define SD_HALF  0.5
#define SD_ONE   1.0
#define SD_TWO   2.0
#define SD_THREE 3.0
#define SD_FOUR  4.0
#define SD_45    45.0
#define SD_60    60.0
#define SD_90    90.0
#define SD_180   180.0
#define SD_MAX   DBL_MAX

#ifdef M_PI
#define SD_PI  3.1415926535f
#else
#define SD_PI  ((float) SD_PI)
#endif

/** PI / 2 */
#ifdef M_PI_2
#  define  SD_PI_2  M_PI_2
#else
#  define  SD_PI_2  1.57079632679489661923
#endif

#define SD_2PI (float)SD_PI * (float)SD_PI

#define SD_DEGREES_TO_RADIANS   (SD_PI/SD_180)
#define SD_RADIANS_TO_DEGREES   (SD_180/SD_PI)
#define SD_EARTH_RAD            6378.155
#define SD_FEET_TO_METER        0.3048


typedef float osgMat3[3][3];
typedef float osgMat4[4][4];

struct osgCoord
{
	osg::Vec3 xyz ;
	osg::Vec3 hpr ;
};

extern void osgXformPnt3( osg::Vec3 dst, const osg::Vec3 src, const osgMat4 mat );
extern void osgMakeCoordMat4 ( osgMat4 m, const float x, const float y, const float z, const float h, const float p, const float r );
inline void osgXformPnt3( osg::Vec3 dst, const osgMat4 mat ) { osgXformPnt3 ( dst, dst, mat ); }

inline float sdASin ( float s )
                { return (float) asin (s) * SD_RADIANS_TO_DEGREES ; }
inline float sdACos ( float s )
                { return (float) acos (s) * SD_RADIANS_TO_DEGREES ; }
inline float sdATan ( float s )
                { return (float) atan (s) * SD_RADIANS_TO_DEGREES ; }
inline float sdATan2 ( float y, float x )
                { return (float) atan2 ( y,x ) * SD_RADIANS_TO_DEGREES ; }
inline float sdSin ( float s )
                { return (float)sin (s * SD_DEGREES_TO_RADIANS) ; }
inline float sdCos ( float s )
                { return (float)cos (s * SD_DEGREES_TO_RADIANS) ; }
inline float sdTan ( float s )
                { return (float)tan (s * SD_DEGREES_TO_RADIANS) ; }

// return a random number between [0.0, 1.0)
inline double SDRandom(void)
{
  return(rand() / (double)RAND_MAX);
}

#endif /* _OSGMATH_H_ */
