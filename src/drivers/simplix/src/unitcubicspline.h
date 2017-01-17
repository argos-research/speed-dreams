//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
// unitcubicspline.h
//--------------------------------------------------------------------------*
// A robot for Speed Dreams-Version 2.X simuV4
//--------------------------------------------------------------------------*
// Cubic spline
// Kubischer Spline
//
// File         : unitcubicspline.h
// Created      : 2007.11.25
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
#ifndef _UNITCUBICSPLINE_H_
#define _UNITCUBICSPLINE_H_

#include "unitcubic.h"

//==========================================================================*
// Deklaration der Klasse TCubicSpline
//--------------------------------------------------------------------------*
class TCubicSpline  
{
  public:
	TCubicSpline();                              // Default constructor
	TCubicSpline                                 // Constructor
	  (int Count, const double* X, 
	  const double* Y, const double* S);
	~TCubicSpline();

	void Init(int Count, const double* X, 
		const double* Y, const double* S);

	double CalcOffset(double X) const;           // Get offset
	double CalcGradient(double X) const;         // Get gradient

	bool IsValidX(double X) const;               // Is x valid in spline?

  private:
	int	FindSeg(double X) const;                 // Find seg to x

  private:
	int	oCount;                                  // Nbr of segemnts
	double*	oSegs;                               // Segments
	TCubic*	oCubics;                             // Cubics
};
//==========================================================================*
#endif // _UNITCUBICSPLINE_H_
//--------------------------------------------------------------------------*
// end of file unitcubicspline.h
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
