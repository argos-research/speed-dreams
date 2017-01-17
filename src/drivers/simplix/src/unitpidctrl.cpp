//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
// unitpidctrl.cpp
//--------------------------------------------------------------------------*
// A robot for Speed Dreams-Version 2.X simuV4
//--------------------------------------------------------------------------*
// PID Controller
// 
// File         : unitpidctrl.cpp
// Created      : 2007.11.257
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

#include "unitglobal.h"
#include "unitcommon.h"
#include "unitpidctrl.h"

//==========================================================================*
// Default constructor
//--------------------------------------------------------------------------*
TPidController::TPidController(): oLastPropValue(0), oTotal(0),
  oMaxTotal(100), oMinTotal(-100), oTotalRate(0), oP(1),	oI(0), oD(0)
{
}
//==========================================================================*

//==========================================================================*
// Destructor
//--------------------------------------------------------------------------*
TPidController::~TPidController()
{
}
//==========================================================================*

//==========================================================================*
//
//--------------------------------------------------------------------------*
double TPidController::Sample(double PropValue)
{
  return Sample(PropValue, PropValue - oLastPropValue);
}
//==========================================================================*

//==========================================================================*
//
//--------------------------------------------------------------------------*
double TPidController::Sample(double PropValue, double DiffValue)
{
  oLastPropValue = PropValue;

  double Cntrl = PropValue * oP;

  if (oD != 0)
  {
    Cntrl += DiffValue * oD;
  }

  if (oI != 0)
  {
	if (oTotalRate == 0)
	  oTotal += PropValue;
	else
	  oTotal += (PropValue - oTotal) * oTotalRate;

	if (oTotal > oMaxTotal)
	  oTotal = oMaxTotal;
	else if (oTotal < oMinTotal)
	  oTotal = oMinTotal;
	
	Cntrl += oTotal * oI;
  }

  return Cntrl;
}
//==========================================================================*
// end of file unitpidctrl.h
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
