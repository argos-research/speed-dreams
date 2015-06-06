//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
// unitlanepoint.h
//--------------------------------------------------------------------------*
// TORCS: "The Open Racing Car Simulator"
// A robot for Speed Dreams-Version 1.4.0/2.X
//--------------------------------------------------------------------------*
// Point of a lane
// Punkt einer Fahrspur
//
// File         : unitlanepoint.h
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
// Diese Version wurde mit MS Visual C++ 2005 Express Edition entwickelt.
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
#ifndef _UNITLANEPOINT_H_
#define _UNITLANEPOINT_H_

//==========================================================================*
// Deklaration der Klasse TLanePoint
//--------------------------------------------------------------------------*
class TLanePoint  
{
  public:
	TLanePoint();
	~TLanePoint();

  public:
	double T;		                             // Parametric distance to next seg [0..1]
	double Offset; 	                             // Offset from middle for the path.
	double Angle;	                             // Global angle
	double Crv;		                             // Curvature at point
	double Crvz;                                 // Curvature in z at point
	double Speed;                                // Speed
	double AccSpd;                               // Accelleration speed
	int Index;	                                 // Index of sec.
};
//==========================================================================*
#endif // _UNITLANEPOINT_H_
//--------------------------------------------------------------------------*
// end of file unitlanepoint.h
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
