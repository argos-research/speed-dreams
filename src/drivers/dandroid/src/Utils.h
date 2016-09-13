/***************************************************************************

    file        : Utils.h
    created     : 9 Apr 2006
    copyright   : (C) 2006 Tim Foden

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#ifndef _UTILS_H_
#define _UTILS_H_

#include "Vec2d.h"
#include "Vec3d.h"

#define MN(x, y)	((x) < (y) ? (x) : (y))
#define MX(x, y)	((x) > (y) ? (x) : (y))

class Utils  
{
  public:
  Utils();
  ~Utils();

  static double	ClosestPtOnLine( double ptx, double pty, double px, double py, double vx, double vy );
  static double	DistPtFromLine( double ptx, double pty, double px, double py, double vx, double vy );

  static bool	LineCrossesLine( double p0x, double p0y, double v0x, double v0y, double p1x, double p1y, double v1x, double v1y, double& t );
  static bool	LineCrossesLine( const Vec2d& p0, const Vec2d& v0, const Vec2d& p1, const Vec2d& v1, double& t );
  static bool	LineCrossesLineXY( const Vec3d& p0, const Vec3d& v0, const Vec3d& p1, const Vec3d& v1, double& t );

  static bool	LineCrossesLine( const Vec2d& p0, const Vec2d& v0, const Vec2d& p1, const Vec2d& v1, double& t0, double& t1 );

  static double	CalcCurvature( double p1x, double p1y, double p2x, double p2y, double p3x, double p3y );
  static double	CalcCurvature( const Vec2d& p1, const Vec2d& p2, const Vec2d& p3 );
  static double	CalcCurvatureTan( const Vec2d& p1, const Vec2d& tangent, const Vec2d& p2 );
  static double	CalcCurvatureXY( const Vec3d& p1, const Vec3d& p2, const Vec3d& p3 );
  static double	CalcCurvatureZ( const Vec3d& p1, const Vec3d& p2, const Vec3d& p3 );

  static bool		CalcTangent( const Vec2d& p1, const Vec2d& p2, const Vec2d& p3, Vec2d& tangent );

  static double	InterpCurvatureRad( double k0, double k1, double t );
  static double	InterpCurvatureLin( double k0, double k1, double t );
  static double	InterpCurvature( double k0, double k1, double t );

  static double	VecAngXY( const Vec3d& v );
  static double	VecLenXY( const Vec3d& v );
  static Vec3d	VecNormXY( const Vec3d& v );

  static double	VecAngle( const Vec2d& v );
  static Vec2d	VecNorm( const Vec2d& v );
  static Vec2d	VecUnit( const Vec2d& v );
};

#endif // _UTILS_H_
