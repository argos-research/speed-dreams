//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
// unitcubic.h
//--------------------------------------------------------------------------*
// A robot for Speed Dreams-Version 2.X simuV4
//--------------------------------------------------------------------------*
// Kubisches Polynom
//
// File         : unitcubic.h
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
#ifndef _UNITCUBIC_H_
#define _UNITCUBIC_H_

//==========================================================================*
// Deklaration der Klasse TCubic
//--------------------------------------------------------------------------*
class TCubic  
{
  public:
	TCubic();                                    // Default constructor
	TCubic                                       // Parametric constructor
	  (double C0, double C1, double C2, double C3);
	TCubic                                       // Two point constructor
	  (double X0, double Y0, double S0,	double X1, double Y1, double S1);
	~TCubic();

	void Set                                     // Set coefficients 
	  (double C0, double C1, double C2, double C3);
	void Set                                     // Set two points 
	  (double X0, double Y0, double S0, double X1, double Y1, double S1);

	double CalcOffset(double X) const;           // Get offset
	double CalcGradient(double X) const;         // Get gradient
	double Calc2ndDerivative(double X) const;    // Get 2nd derivative

  public:
	double oCoeffs[4];	                         // Coefficients
};
//==========================================================================*
#endif // _UNITCUBIC_H_
//--------------------------------------------------------------------------*
// end of file unitcubic.h
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
