//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
// unitcommon.cpp
//--------------------------------------------------------------------------*
// TORCS: "The Open Racing Car Simulator"
// A robot for Speed Dreams-Version 1.4.0/2.X
//--------------------------------------------------------------------------*
// Common used functions
// Gemeinsam nutzbare Funktionen
//
// File         : unitcommon.cpp
// Created      : 2007.11.25
// Last changed : 2011.06.02
// Copyright    : © 2007-2011 Wolf-Dieter Beelitz
// eMail        : wdb@wdbee.de
// Version      : 3.01.000
//--------------------------------------------------------------------------*
// This unit is based on the robot mouse_2006 
// Diese Unit basiert auf dem Roboter mouse_2006
//
//    Copyright: (C) 2006-2007 Tim Foden
//
//--------------------------------------------------------------------------*
// This program was developed and tested on windows XP
// There are no known Bugs, but:
// Who uses the files accepts, that no responsibility is adopted
// for bugs, dammages, aftereffects or consequential losses.
//
// Das Programm wurde unter Windows XP entwickelt und getestet.
// Fehler sind nicht bekannt, dennoch gilt:
// Wer die Dateien verwendet erkennt an, dass für Fehler, Schäden,
// Folgefehler oder Folgeschäden keine Haftung übernommen wird.
//--------------------------------------------------------------------------*
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// Im übrigen gilt für die Nutzung und/oder Weitergabe die
// GNU GPL (General Public License)
// Version 2 oder nach eigener Wahl eine spätere Version.
//--------------------------------------------------------------------------*

#include "unitcommon.h"

//==========================================================================*
// Default constructor
//--------------------------------------------------------------------------*
TUtils::TUtils()
{
}
//==========================================================================*

//==========================================================================*
// Destructor
//--------------------------------------------------------------------------*
TUtils::~TUtils()
{
}
//==========================================================================*

//==========================================================================*
// Utility to merge parameter files
//--------------------------------------------------------------------------*
void* TUtils::MergeParamFile( 
  PCarSettings Params, 
  const char* FileName)
{
  PCarSettings NewParams =                       // Open setup file
	GfParmReadFile(FileName, GFPARM_RMODE_STD, false);

  if(NewParams == NULL)                          // Return old one, 
    return Params;                               //   if new one is empty 

  if(Params == NULL)                             // Return new one, 
    return NewParams;                            //   if old one is empty  

  return GfParmMergeHandles(Params, NewParams,   // Merge setup files
    GFPARM_MMODE_SRC 
	| GFPARM_MMODE_DST 
	| GFPARM_MMODE_RELSRC 
	| GFPARM_MMODE_RELDST);
}
//==========================================================================*

//==========================================================================*
// Utility to find the closest point on a line (Lot auf Linie)
//--------------------------------------------------------------------------*
double TUtils::ClosestPtOnLine(
	double ptx,
	double pty,
	double px,
	double py,
	double vx,
	double vy )
{
	// P from AB
	// Q is closest pt on AB
	// (P-Q).(B-A) == 0 then Q is closest pt.
	// Q = A + t.(B-A)
	// (P-(A+t.(B-A)).(B-A)
	// use AB for const B-A, and AP for P-A.
	// (AP + tAB).AB == AP.AB + tAB.AB
	// t = -AP.AB / AB.AB == PA.AB / AB.AB

	double	pax = px - ptx;
	double	pay = py - pty;
	double	den = vx * vx + vy * vy;
	if( den == 0 )
		return 0;

	double	num = pax * vx + pay * vy;
	double	t = num / den;
	return t;
}
//==========================================================================*

//==========================================================================*
// Utility to find distance of a point on a line (Lot auf Linie)
//--------------------------------------------------------------------------*
double TUtils::DistPtFromLine(
	double ptx,
	double pty,
	double px,
	double py,
	double vx,
	double vy )
{
	double	t = ClosestPtOnLine(ptx, pty, px, py, vx, vy);
	double	qx = px + vx * t;
	double	qy = py + vy * t;
	double	dist = myhypot(ptx - qx, pty - qy);
	return dist;
}
//==========================================================================*

//==========================================================================*
// Utility to find crossing point (Schnittpunkt zweier Geraden)
//--------------------------------------------------------------------------*
bool TUtils::LineCrossesLine(
	double	p0x,
	double	p0y,
	double	v0x,
	double	v0y,
	double	p1x,
	double	p1y,
	double	v1x,
	double	v1y,

	double&	t )
{
//	double	denom = lv0 % lv1;
	double	denom = v0x * v1y - v0y * v1x;
	if( denom == 0 )
		return false;

//	double	numer = lv1 % (lp0 - lp1);
	double	numer = v1x * (p0y - p1y) - v1y * (p0x - p1x);

	t = numer / denom;

	return true;
}
//==========================================================================*

//==========================================================================*
// Utility to find crossing point (Schnittpunkt zweier Geraden)
//--------------------------------------------------------------------------*
bool TUtils::LineCrossesLine(
	const TVec2d&	p0,
	const TVec2d&	v0,
	const TVec2d&	p1,
	const TVec2d&	v1,

	double&			t )
{
	return LineCrossesLine(p0.x, p0.y, v0.x, v0.y, p1.x, p1.y, v1.x, v1.y, t);
}
//==========================================================================*

//==========================================================================*
// Utility to find crossing point (Schnittpunkt zweier Geraden)
//--------------------------------------------------------------------------*
bool TUtils::LineCrossesLineXY(
	const TVec3d&	p0,
	const TVec3d&	v0,
	const TVec3d&	p1,
	const TVec3d&	v1,

	double&			t )
{
	return LineCrossesLine(p0.x, p0.y, v0.x, v0.y, p1.x, p1.y, v1.x, v1.y, t);
}
//==========================================================================*

//==========================================================================*
// Utility to find crossing point (Schnittpunkt zweier Geraden)
//--------------------------------------------------------------------------*
bool TUtils::LineCrossesLine(
	const TVec2d&	p0,
	const TVec2d&	v0,
	const TVec2d&	p1,
	const TVec2d&	v1,

	double&			t0,
	double&			t1 )
{
	double	denom = v0.x * v1.y - v0.y * v1.x;
	if( denom == 0 )
		return false;

	double	numer0 = v1.x * (p0.y - p1.y) - v1.y * (p0.x - p1.x);
	double	numer1 = v0.x * (p1.y - p0.y) - v0.y * (p1.x - p0.x);

	t0 =  numer0 / denom;
	t1 = -numer1 / denom;

	return true;
}
//==========================================================================*

//==========================================================================*
// Utility to get curvature (Inverser Radius)
//--------------------------------------------------------------------------*
double TUtils::CalcCurvature(
	double p1x, double p1y,
	double p2x, double p2y,
	double p3x, double p3y )
{
	double	px = p1x - p2x;
	double	py = p1y - p2y;
	double	qx = p2x - p3x;
	double	qy = p2y - p3y;
	double	sx = p3x - p1x;
	double	sy = p3y - p1y;

	double	K = (2 * (px * qy - py * qx)) /
						 sqrt((px * px + py * py) *
							  (qx * qx + qy * qy) *
							  (sx * sx + sy * sy));
	return K;
}
//==========================================================================*

//==========================================================================*
// Utility to get curvature (Inverser Radius)
//--------------------------------------------------------------------------*
double TUtils::CalcCurvature(
	const TVec2d& p1,
	const TVec2d& p2,
	const TVec2d& p3 )
{
	return CalcCurvature(p1.x, p1.y, p2.x, p2.y, p3.x, p3.y);
}
//==========================================================================*

//==========================================================================*
// Utility to get curvature (Inverser Radius)
//--------------------------------------------------------------------------*
double TUtils::CalcCurvatureTan(
	const TVec2d& p1,
	const TVec2d& tangent,
	const TVec2d& p2 )
{
	TVec2d	v = VecUnit(VecNorm(tangent));
	TVec2d	u = VecNorm(p2 - p1);
	TVec2d	q = (p1 + p2) * 0.5;
	double	radius;
	if( !LineCrossesLine(p1, v, q, u, radius) )
		return 0;
	else
		return 1.0 / radius;
}
//==========================================================================*

//==========================================================================*
// Utility to get curvature (Inverser Radius)
//--------------------------------------------------------------------------*
double TUtils::CalcCurvatureXY(
	const TVec3d& p1,
	const TVec3d& p2,
	const TVec3d& p3 )
{
	return CalcCurvature(p1.x, p1.y, p2.x, p2.y, p3.x, p3.y);
}
//==========================================================================*

//==========================================================================*
// Utility to get curvature in height profil (Inverser Radius i. Höhenprofil)
//--------------------------------------------------------------------------*
double TUtils::CalcCurvatureZ(
	const TVec3d& p1,
	const TVec3d& p2,
	const TVec3d& p3 )
{
	double	x1 = 0;
	double	x2 = (p1 - p2).len();
	double	x3 = x2 + (p2 - p3).len();
	return CalcCurvature(x1, p1.z, x2, p2.z, x3, p3.z);
}
//==========================================================================*

//==========================================================================*
// Utility to get tangent (Tangente)
//--------------------------------------------------------------------------*
bool TUtils::CalcTangent(
	const TVec2d&	p1,
	const TVec2d&	p2,
	const TVec2d&	p3,

	TVec2d&			tangent )
{
	TVec2d	mid1  = (p1 + p2) * 0.5;
	TVec2d	norm1 = VecNorm(p2 - p1);
	TVec2d	mid2  = (p2 + p3) * 0.5;
	TVec2d	norm2 = VecNorm(p3 - p2);

	double	t;
	if( !LineCrossesLine(mid1, norm1, mid2, norm2, t) )
	{
		if( p1 != p3 )
		{
			tangent = VecUnit(p3 - p1);
			return true;
		}

		return false;
	}

	TVec2d	centre = mid1 + norm1 * t;
//	tangent = p2 - centre;
//	tangent = VecNorm(p2 - centre);
	tangent = VecUnit(VecNorm(p2 - centre));
	if( norm1 * (p3 - p1) < 0 )
		tangent = -tangent;
	return true;
}
//==========================================================================*

//==========================================================================*
// Utility to interpolate a curve linear 
//--------------------------------------------------------------------------*
double TUtils::InterpCurvatureLin( double k0, double k1, double t )
{
	return k0 + (k1 - k0) * t;
}
//==========================================================================*

//==========================================================================*
// Utility to interpolate a curve radial
//--------------------------------------------------------------------------*
double TUtils::InterpCurvatureRad( double k0, double k1, double t )
{
	// r = r0 + (r1 - r0) * t;
	//
	// 1/k = 1/k0 + (1/k1 - 1/k0) * t
	// 1/k = (k1 + (k0 - k1) * t) / (k0 * k1);
	// k = (k0 * k1) / (k1 + (k0 - k1) * t)
	//
	double	den = k1 + (k0 - k1) * t;
	if( fabs(den) < 0.000001 )
		den = 0.000001;
	return k0 * k1 / den;
}
//==========================================================================*

//==========================================================================*
// Utility to interpolate a curve 
//--------------------------------------------------------------------------*
double TUtils::InterpCurvature( double k0, double k1, double t )
{
//	return InterpCurvatureRad(k0, k1, t);
	return InterpCurvatureLin(k0, k1, t);
}
//==========================================================================*

//==========================================================================*
// Utility to get direction angle
//--------------------------------------------------------------------------*
double TUtils::VecAngXY( const TVec3d& v )
{
	return atan2(v.y, v.x);
}
//==========================================================================*

//==========================================================================*
// Utility to get length of 3D-Vector in 2D projection
//--------------------------------------------------------------------------*
double TUtils::VecLenXY( const TVec3d& v )
{
	return myhypot(v.y, v.x);
}
//==========================================================================*

//==========================================================================*
// Utility to normalize a 3D vector in 2D projection
//--------------------------------------------------------------------------*
TVec3d TUtils::VecNormXY( const TVec3d& v )
{
	return TVec3d(-v.y, v.x, v.z);
}
//==========================================================================*

//==========================================================================*
// Utility to get direction angle
//--------------------------------------------------------------------------*
double TUtils::VecAngle( const TVec2d& v )
{
	return atan2(v.y, v.x);
}
//==========================================================================*

//==========================================================================*
// Utility to normalize a 2D vector
//--------------------------------------------------------------------------*
TVec2d TUtils::VecNorm( const TVec2d& v )
{
	return TVec2d(-v.y, v.x);
}
//==========================================================================*

//==========================================================================*
// Utility to normalize a 2D vector
//--------------------------------------------------------------------------*
TVec2d TUtils::VecUnit( const TVec2d& v )
{
	double	h = myhypot(v.x, v.y);
	if( h == 0 )
		return TVec2d(0, 0);
	else
		return TVec2d(v.x / h, v.y / h);
}
//==========================================================================*
// end of file unitcommon.cpp
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*

