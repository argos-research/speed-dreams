/***************************************************************************

    file        : Utils.cpp
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

// Utils.cpp: implementation of the Utils class.
//
//////////////////////////////////////////////////////////////////////

#include "Utils.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Utils::Utils()
{

}

Utils::~Utils()
{

}

double	Utils::ClosestPtOnLine(
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

double	Utils::DistPtFromLine(
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
	double	dist = hypot(ptx - qx, pty - qy);
	return dist;
}

bool	Utils::LineCrossesLine(
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

bool	Utils::LineCrossesLine(
	const Vec2d&	p0,
	const Vec2d&	v0,
	const Vec2d&	p1,
	const Vec2d&	v1,

	double&			t )
{
	return LineCrossesLine(p0.x, p0.y, v0.x, v0.y, p1.x, p1.y, v1.x, v1.y, t);
}

bool	Utils::LineCrossesLineXY(
	const Vec3d&	p0,
	const Vec3d&	v0,
	const Vec3d&	p1,
	const Vec3d&	v1,

	double&			t )
{
	return LineCrossesLine(p0.x, p0.y, v0.x, v0.y, p1.x, p1.y, v1.x, v1.y, t);
}

bool	Utils::LineCrossesLine(
	const Vec2d&	p0,
	const Vec2d&	v0,
	const Vec2d&	p1,
	const Vec2d&	v1,

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

double	Utils::CalcCurvature(
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

double	Utils::CalcCurvature(
	const Vec2d& p1,
	const Vec2d& p2,
	const Vec2d& p3 )
{
	return CalcCurvature(p1.x, p1.y, p2.x, p2.y, p3.x, p3.y);
}

double	Utils::CalcCurvatureTan(
	const Vec2d& p1,
	const Vec2d& tangent,
	const Vec2d& p2 )
{
	Vec2d	v = VecUnit(VecNorm(tangent));
	Vec2d	u = VecNorm(p2 - p1);
	Vec2d	q = (p1 + p2) * 0.5;
	double	radius;
	if( !LineCrossesLine(p1, v, q, u, radius) )
		return 0;
	else
		return 1.0 / radius;
}

double	Utils::CalcCurvatureXY(
	const Vec3d& p1,
	const Vec3d& p2,
	const Vec3d& p3 )
{
	return CalcCurvature(p1.x, p1.y, p2.x, p2.y, p3.x, p3.y);
}

double	Utils::CalcCurvatureZ(
	const Vec3d& p1,
	const Vec3d& p2,
	const Vec3d& p3 )
{
	double	x1 = 0;
	double	x2 = (p1 - p2).len();
	double	x3 = x2 + (p2 - p3).len();
	return CalcCurvature(x1, p1.z, x2, p2.z, x3, p3.z);
}

bool	Utils::CalcTangent(
	const Vec2d&	p1,
	const Vec2d&	p2,
	const Vec2d&	p3,

	Vec2d&			tangent )
{
	Vec2d	mid1  = (p1 + p2) * 0.5;
	Vec2d	norm1 = VecNorm(p2 - p1);
	Vec2d	mid2  = (p2 + p3) * 0.5;
	Vec2d	norm2 = VecNorm(p3 - p2);

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

	Vec2d	centre = mid1 + norm1 * t;
//	tangent = p2 - centre;
//	tangent = VecNorm(p2 - centre);
	tangent = VecUnit(VecNorm(p2 - centre));
	if( norm1 * (p3 - p1) < 0 )
		tangent = -tangent;
	return true;
}

double	Utils::InterpCurvatureLin( double k0, double k1, double t )
{
	return k0 + (k1 - k0) * t;
}

double	Utils::InterpCurvatureRad( double k0, double k1, double t )
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

double	Utils::InterpCurvature( double k0, double k1, double t )
{
//	return InterpCurvatureRad(k0, k1, t);
	return InterpCurvatureLin(k0, k1, t);
}

double	Utils::VecAngXY( const Vec3d& v )
{
	return atan2(v.y, v.x);
}

double	Utils::VecLenXY( const Vec3d& v )
{
	return hypot(v.y, v.x);
}

Vec3d	Utils::VecNormXY( const Vec3d& v )
{
	return Vec3d(-v.y, v.x, v.z);
}

double	Utils::VecAngle( const Vec2d& v )
{
	return atan2(v.y, v.x);
}

Vec2d	Utils::VecNorm( const Vec2d& v )
{
	return Vec2d(-v.y, v.x);
}

Vec2d	Utils::VecUnit( const Vec2d& v )
{
	double	h = hypot(v.x, v.y);
	if( h == 0 )
		return Vec2d(0, 0);
	else
		return Vec2d(v.x / h, v.y / h);
}
