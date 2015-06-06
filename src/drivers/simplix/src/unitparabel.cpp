//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
// unitparabel.cpp
//--------------------------------------------------------------------------*
// TORCS: "The Open Racing Car Simulator"
// A robot for Speed Dreams-Version 1.4.0/2.X
//--------------------------------------------------------------------------*
// Roboter für TORCS-Version 1.3.0
// Parabel als quadratisches Polynom
// 
// File         : unitparabel.cpp 
// Created      : 2007.11.25
// Last changed : 2011.06.02
// Copyright    : © 2007-2011 Wolf-Dieter Beelitz
// eMail        : wdb@wdbee.de
// Version      : 3.01.000
//--------------------------------------------------------------------------*
// Ein erweiterter TORCS-Roboters
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

#include <math.h>

#include "unitglobal.h"
#include "unitcommon.h"

#include "unitparabel.h"

//==========================================================================*
// Default Constructor
//--------------------------------------------------------------------------*
TParabel::TParabel(): oA(0), oB(0), oC(0)
{
}
//==========================================================================*

//==========================================================================*
// Parametric constructor
//--------------------------------------------------------------------------*
TParabel::TParabel(double A, double B, double C)
{
  oA = A;
  oB = B;
  oC = C;
}
//==========================================================================*

//==========================================================================*
// Constructor
//--------------------------------------------------------------------------*
TParabel::TParabel(double X, double Y, double VelY, double AccY)
{
  Set(X, Y, VelY, AccY);
}
//==========================================================================*

//==========================================================================*
// Destructor
//--------------------------------------------------------------------------*
TParabel::~TParabel()
{
}
//==========================================================================*

//==========================================================================*
// Set parameters
//--------------------------------------------------------------------------*
void TParabel::Set(double A, double B, double C)
{
  oA = A;
  oB = B;
  oC = C;
}
//==========================================================================*

//==========================================================================*
// Set parameters
//--------------------------------------------------------------------------*
void TParabel::Set(double X, double Y, double VelY, double AccY)
{
  oA = AccY / 2;
  oB = VelY - 2 * oA * X;
  oC = Y - (oA * X + oB) * X;
}
//==========================================================================*

//==========================================================================*
// Find minimum
//--------------------------------------------------------------------------*
double TParabel::CalcMin() const
{
  // minimum is where slope == 0
  double X = -oB / (2 * oA);
  return X;
}
//==========================================================================*

//==========================================================================*
// Horner schema
//--------------------------------------------------------------------------*
double TParabel::CalcY(double X) const
{
  return (oA * X + oB) * X + oC;
}
//==========================================================================*

//==========================================================================*
// Solve 
//--------------------------------------------------------------------------*
bool TParabel::Solve(double Y, double& X0, double& X1) const
{
  if(oA == 0)
  {
	if(oB == 0)
	{
	  return false;
	}

	// y == bx + c
	//
	// x = (y - c) / b
	X0 = X1 = (Y - oC) / oB;
	return true;
  }

  // y == a * x * x + b * x + c
  //
  // a * x * x + b * x + (c - y) == 0
  //
  // x = (-b +/- sqrt(b * b - 4 * a * (c - y))] / (2 * a)

  double Inner = oB * oB - 4 * oA * (oC - Y);
  if (Inner < 0)
	return false;

  Inner = sqrt(Inner);
  X0 = (-oB - Inner) / (2 * oA);
  X1 = (-oB + Inner) / (2 * oA);

  return true;
}
//==========================================================================*

//==========================================================================*
// SmallestNonNegativeRoot
//--------------------------------------------------------------------------*
bool TParabel::SmallestNonNegativeRoot(double& T) const
{
  double X0, X1;
  if(!Solve(0, X0, X1))
	return false;

  T = X0;
  if (X1 >= 0 && X1 < X0)
	T = X1;

  return T >= 0;
}
//==========================================================================*

//==========================================================================*
// Add two parabels
//--------------------------------------------------------------------------*
TParabel TParabel::operator+ (const TParabel& P) const
{
  return TParabel(oA + P.oA, oB + P.oB, oC + P.oC);
}
//==========================================================================*

//==========================================================================*
// Substract two parabels
//--------------------------------------------------------------------------*
TParabel TParabel::operator- (const TParabel& P) const
{
  return TParabel(oA - P.oA, oB - P.oB, oC - P.oC);
}
//==========================================================================*
// end of file unitparabel.h
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
