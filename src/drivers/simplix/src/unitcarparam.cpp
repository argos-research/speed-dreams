//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
// unitcarparam.cpp
//--------------------------------------------------------------------------*
// TORCS: "The Open Racing Car Simulator"
// A robot for Speed Dreams-Version 1.4.0/2.X
//--------------------------------------------------------------------------*
// Tuningparameter des Fahrzeugs
//
// File         : unitcarparam.cpp
// Created      : 2007.11.25
// Last changed : 2013.02.26
// Copyright    : © 2007-2013 Wolf-Dieter Beelitz
// eMail        : wdb@wdbee.de
// Version      : 4.02.000
//--------------------------------------------------------------------------*
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
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
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
#include "unitglobal.h"
#include "unitcommon.h"

#include "unitparabel.h"
#include "unitcarparam.h"

//==========================================================================*
// Default constructor
//--------------------------------------------------------------------------*
TCarParam::TCarParam():
  oScaleMu(0.8),
  oScaleMinMu(0.95),
  oScaleBrake(0.8),
  oScaleBrakePit(1.0),
  oScaleBump(0.4f),
  oScaleBumpOuter(0.6f),
  oScaleBumpLeft(0),
  oScaleBumpRight(0),
  oLimitSideUse(false), 
  oLimitSideWidth(1.5),
  oUglyCrvZ(-0.0035)
{
}
//==========================================================================*

//==========================================================================*
// Destructor
//--------------------------------------------------------------------------*
TCarParam::~TCarParam()
{
}
//===========================================================================

//==========================================================================*
// Zuweisung
//--------------------------------------------------------------------------*
TCarParam& TCarParam::operator= (const TCarParam& CarParam)
{
  oScaleMu = CarParam.oScaleMu;
  oScaleMinMu = CarParam.oScaleMinMu;
  oScaleBrake = CarParam.oScaleBrake;
  oScaleBrakePit = CarParam.oScaleBrakePit;
  oScaleBump = CarParam.oScaleBump;
  oScaleBumpOuter = CarParam.oScaleBumpOuter;
  oScaleBumpLeft = CarParam.oScaleBumpLeft;
  oScaleBumpRight = CarParam.oScaleBumpRight;
  oLimitSideUse = CarParam.oLimitSideUse;
  oLimitSideWidth = CarParam.oLimitSideWidth;
  oUglyCrvZ = CarParam.oUglyCrvZ;
  oBrakeForce = CarParam.oBrakeForce;

  return *this;
}
//===========================================================================

//--------------------------------------------------------------------------*
// end of file unitcarparam.cpp
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
