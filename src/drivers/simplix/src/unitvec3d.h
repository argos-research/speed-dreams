//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
// unitvec3d.h
//--------------------------------------------------------------------------*
// TORCS: "The Open Racing Car Simulator"
// A robot for Speed Dreams-Version 1.4.0/2.X
//--------------------------------------------------------------------------*
// Erweiterung des 3D-Vektors
//
// File         : unitvec2d.h
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
#ifndef _UNITVEC3D_H_
#define _UNITVEC3D_H_

#include <v3_t.h>
#include <tgf.h>

#include "unitglobal.h"
//#include "unitcommon.h" NOT ALLOWERD HERE!!!
#include "unitvec2d.h"

//==========================================================================*
// Deklaration der Klasse TVec3d
//--------------------------------------------------------------------------*
class TVec3d : public v3t<double>
{
  public:
	TVec3d() {};
	TVec3d(const v3t<double>& V) : v3t<double>(V) {};
	TVec3d(double X, double Y, double Z) : v3t<double>(X, Y, Z) {};
	TVec3d(const t3Dd& V) : v3t<double>(V.x, V.y, V.z) {};

	TVec3d& operator= (const v3t<double>& V)
	{
	  v3t<double>::operator=(V);
	  return *this;
	};

	TVec2d GetXY() const {return TVec2d(x, y);};
};
//==========================================================================*
#endif // _UNITVEC3D_H_
//--------------------------------------------------------------------------*
// end of file unitvec3d.h
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
