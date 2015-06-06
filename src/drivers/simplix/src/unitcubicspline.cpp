//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
// unitcubicspline.cpp
//--------------------------------------------------------------------------*
// TORCS: "The Open Racing Car Simulator"
// A robot for Speed Dreams-Version 1.4.0/2.X
//--------------------------------------------------------------------------*
// Cubic spline
// Kubischer Spline
//
// File         : unitcubicspline.cpp
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

#include "unitglobal.h"
#include "unitcommon.h"

#include "unitcubicspline.h"

//==========================================================================*
// Default constructor
//--------------------------------------------------------------------------*
TCubicSpline::TCubicSpline()
{
  oCount = 0;
  oSegs = NULL;
  oCubics = NULL;
}
//==========================================================================*

//==========================================================================*
// Constructor
//--------------------------------------------------------------------------*
TCubicSpline::TCubicSpline
  (int Count, const double* X, const double* Y, const double* S)
{
  oCount = Count;
  oSegs = new double[oCount];
  oCubics = new TCubic[oCount - 1];

  for (int I = 0; I < oCount; I++)
  {
 	oSegs[I] = X[I];
	if (I + 1 < oCount)
	oCubics[I].Set( X[I], Y[I], S[I], X[I+1], Y[I+1], S[I+1]);
  }
}
//==========================================================================*

//==========================================================================*
// Initialization
//--------------------------------------------------------------------------*
void TCubicSpline::Init
  (int Count, const double* X, const double* Y, const double* S)
{
  delete [] oSegs;
  delete [] oCubics;

  oCount = Count;
  oSegs = new double[oCount];
  oCubics = new TCubic[oCount - 1];

  for (int I = 0; I < oCount; I++)
  {
 	oSegs[I] = X[I];
	if (I + 1 < oCount)
	oCubics[I].Set( X[I], Y[I], S[I], X[I+1], Y[I+1], S[I+1]);
  }
}
//==========================================================================*

//==========================================================================*
// Destructor
//--------------------------------------------------------------------------*
TCubicSpline::~TCubicSpline()
{
  delete [] oSegs;
  delete [] oCubics;
}
//==========================================================================*

//==========================================================================*
// Get offset
//--------------------------------------------------------------------------*
double TCubicSpline::CalcOffset(double X) const
{
  int I = FindSeg(X);
  return oCubics[I].CalcOffset(X);
}
//==========================================================================*

//==========================================================================*
// Get gradient
//--------------------------------------------------------------------------*
double TCubicSpline::CalcGradient(double X) const
{
  int I = FindSeg(X);
  return oCubics[I].CalcGradient(X);
}
//==========================================================================*

//==========================================================================*
// Is X valid in spline
//--------------------------------------------------------------------------*
bool TCubicSpline::IsValidX(double X) const
{
  return X >= oSegs[0] && X <= oSegs[oCount - 1];
}
//==========================================================================*

//==========================================================================*
// Find segment to X
//--------------------------------------------------------------------------*
int	TCubicSpline::FindSeg(double X) const
{
  // binary chop search for interval.
  int Lo = 0;
  int Hi = oCount;

  while (Lo + 1 < Hi)
  {
	int Mid = (Lo + Hi) / 2;
	if (X >= oSegs[Mid])
	  Lo = Mid;
	else
 	  Hi = Mid;
  }

  return Lo;
}
//==========================================================================*
// end of file unitcubicspline.h
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
