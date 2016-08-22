//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
// unitcommon.cpp
//--------------------------------------------------------------------------*
// A robot for Speed Dreams-Version 2.X simuV4
//--------------------------------------------------------------------------*
// Gemeinsam nutzbare Funktionen
//
// File         : unitcommon.cpp
// Created      : 2007.11.17
// Last changed : 2014.11.29
// Copyright    : © 2007-2014 Wolf-Dieter Beelitz
// eMail        : wdbee@users.sourceforge.net
// Version      : 4.05.000
//--------------------------------------------------------------------------*
// Diese Unit basiert auf dem Roboter mouse_2006
//
//    Copyright: (C) 2006-2007 Tim Foden
//
//--------------------------------------------------------------------------*
// Das Programm wurde unter Windows XP entwickelt und getestet.
// Fehler sind nicht bekannt, dennoch gilt:
// Wer die Dateien verwendet erkennt an, dass für Fehler, Schäden,
// Folgefehler oder Folgeschäden keine Haftung übernommen wird.
//
// Im übrigen gilt für die Nutzung und/oder Weitergabe die
// GNU GPL (General Public License)
// Version 2 oder nach eigener Wahl eine spätere Version.
//--------------------------------------------------------------------------*
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//--------------------------------------------------------------------------*
#ifndef _UNITCOMMON_H_
#define _UNITCOMMON_H_

#include "unitglobal.h"

#include "unitvec2d.h"
#include "unitvec3d.h"

//==========================================================================*
// Class TUtils
//--------------------------------------------------------------------------*
class TUtils  
{
  public:
	TUtils();
	~TUtils();

    static void* MergeParamFile(PCarSettings Params, const char* FileName);

	static double ClosestPtOnLine( double ptx, double pty, double px, double py,
									 double vx, double vy );
	static double DistPtFromLine( double ptx, double pty, double px, double py,
									double vx, double vy );

	static bool	LineCrossesLine( double p0x, double p0y, double v0x, double v0y,
								 double p1x, double p1y, double v1x, double v1y,
								 double& t );
	static bool	LineCrossesLine( const TVec2d& p0, const TVec2d& v0,
								 const TVec2d& p1, const TVec2d& v1,
								 double& t );
	static bool	LineCrossesLineXY( const TVec3d& p0, const TVec3d& v0,
								   const TVec3d& p1, const TVec3d& v1,
								   double& t );

	static bool	LineCrossesLine( const TVec2d& p0, const TVec2d& v0,
								 const TVec2d& p1, const TVec2d& v1,
								 double& t0, double& t1 );

	static double CalcCurvature( double p1x, double p1y,
								   double p2x, double p2y,
								   double p3x, double p3y );
	static double CalcCurvature( const TVec2d& p1, const TVec2d& p2,
								   const TVec2d& p3 );
	static double CalcCurvatureTan( const TVec2d& p1, const TVec2d& tangent,
									  const TVec2d& p2 );
	static double CalcCurvatureXY( const TVec3d& p1, const TVec3d& p2,
									 const TVec3d& p3 );
	static double CalcCurvatureZ( const TVec3d& p1, const TVec3d& p2,
									const TVec3d& p3 );

	static bool CalcTangent( const TVec2d& p1, const TVec2d& p2,
								 const TVec2d& p3, TVec2d& tangent );

	static double InterpCurvatureRad( double k0, double k1, double t );
	static double InterpCurvatureLin( double k0, double k1, double t );
	static double InterpCurvature( double k0, double k1, double t );

	static double VecAngXY( const TVec3d& v );
	static double VecLenXY( const TVec3d& v );
	static TVec3d VecNormXY( const TVec3d& v );

	static double VecAngle( const TVec2d& v );
	static TVec2d VecNorm( const TVec2d& v );
	static TVec2d VecUnit( const TVec2d& v );
};
//==========================================================================*
#endif // _UNITCOMMON_H_
//--------------------------------------------------------------------------*
// end of file unittrack.h
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
