//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
// unitparabel.h
//--------------------------------------------------------------------------*
// A robot for Speed Dreams-Version 2.X simuV4
//--------------------------------------------------------------------------*
// Parabel als quadratisches Polynom
//
// File         : unitparabel.h
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
#ifndef _UNITPARABEL_H_
#define _UNITPARABEL_H_

#include "unitglobal.h"

//==========================================================================*
// Deklaration der Klasse TParabel
//--------------------------------------------------------------------------*
class TParabel
{
  public:
	TParabel();
	TParabel(double A, double B, double C);
	TParabel(double X, double Y, double VelY, double AccY);
	~TParabel();

	void Set(double A, double B, double C);
	void Set(double X, double Y, double VelY, double AccY);

	double CalcMin() const;
	double CalcY(double X) const;
	bool Solve(double Y, double& X0, double& X1) const;
	bool SmallestNonNegativeRoot(double& T) const;

	TParabel operator+ (const TParabel& Q) const;
	TParabel operator- (const TParabel& Q) const;

  private:
	double oA;
	double oB;
	double oC;
};
//==========================================================================*
#endif // _UNITPARABEL_H_
//--------------------------------------------------------------------------*
// end of file unitparabel.h
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
