//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
// unitlinreg.cpp
//--------------------------------------------------------------------------*
// A robot for Speed Dreams-Version 2.X simuV4
//--------------------------------------------------------------------------*
// Lineare Regression
// 
// File         : unitlinreg.cpp
// Created      : 2007.11.17
// Last changed : 2014.11.29
// Copyright    : © 2007-2014 Wolf-Dieter Beelitz
// eMail        : wdbee@users.sourceforge.net
// Version      : 4.05.000
//--------------------------------------------------------------------------*
// Teile diese Unit basieren auf 
//
// dem Roboter mouse_2006
//    Copyright: (C) 2006 Tim Foden
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

#include <robottools.h>

#include "unitglobal.h"
#include "unitcommon.h"
#include "unitlinreg.h"

//==========================================================================*
// Default constructor
//--------------------------------------------------------------------------*
TLinearRegression::TLinearRegression()
{
  Clear();
}
//==========================================================================*

//==========================================================================*
// Destructor
//--------------------------------------------------------------------------*
TLinearRegression::~TLinearRegression()
{
}
//==========================================================================*

//==========================================================================*
// Clear all data
//--------------------------------------------------------------------------*
void TLinearRegression::Clear()
{
  oCount = 0;
  oSumX = 0.0;
  oSumY = 0.0;
  oSumXY = 0.0;
  oSumXX = 0.0;
  oSumYY = 0.0;
}
//==========================================================================*

//==========================================================================*
// Add a value y = f(x)
//--------------------------------------------------------------------------*
void TLinearRegression::Add(double X, double Y)
{
  oCount++;
  oSumX += X;
  oSumY += Y;
  oSumXY += X * Y;
  oSumXX += X * X;
  oSumYY += Y * Y;
}
//==========================================================================*

//==========================================================================*
// Add a value P(x,y)
//--------------------------------------------------------------------------*
void TLinearRegression::Add(const TVec2d& Point)
{
  Add(Point.x, Point.y);
}
//==========================================================================*

//==========================================================================*
// vertical distance measure.
//--------------------------------------------------------------------------*
double TLinearRegression::CalcY(double X) const
{
  double Xbar = oSumX / oCount;
  double Ybar = oSumY / oCount;

  double A = (oSumXY + Xbar * oSumY + oSumX * Ybar + Xbar * Ybar) /
				(oSumXX + Xbar * Xbar + 2 * oSumX * Xbar);
				
  double B = Ybar - A * Xbar;
  
  return A * X + B;
}
//==========================================================================*

//==========================================================================*
// perpendicular distance measure.
//--------------------------------------------------------------------------*
void TLinearRegression::CalcLine(TVec2d& Point, TVec2d& V) const
{
  Point = TVec2d(oSumX / oCount, oSumY / oCount);

  // a = x - p.x, b = y - p.y
  double SumAA = oSumXX - 2 * Point.x * oSumX + oCount * Point.x * Point.x;
  double SumBB = oSumYY - 2 * Point.y * oSumY + oCount * Point.y * Point.y;
  double SumAB = oSumXY 
	- Point.y * oSumX - Point.x * oSumY + oCount * Point.x * Point.y;

  double Angle = atan2(2 * SumAB, SumAA - SumBB) / 2;
  V = TVec2d(cos(Angle), sin(Angle));
}
//==========================================================================*
// end of file unitlinreg.cpp
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*

