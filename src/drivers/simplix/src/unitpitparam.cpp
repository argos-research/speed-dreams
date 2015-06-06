//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
// unitpitparam.cpp
//--------------------------------------------------------------------------*
// TORCS: "The Open Racing Car Simulator"
// A robot for Speed Dreams-Version 1.4.0/2.X
//--------------------------------------------------------------------------*
// Interface to TORCS
// 
// File         : unitpitparam.cpp 
// Created      : 2008.04.11
// Last changed : 2011.06.02
// Copyright    : © 2007-2011 Wolf-Dieter Beelitz
// eMail        : wdb@wdbee.de
// Version      : 3.01.000
//--------------------------------------------------------------------------*
// Ein erweiterter TORCS-Roboters
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

#include "unitpitparam.h"

//==========================================================================*
// Default constructor
//--------------------------------------------------------------------------*
TPitParam::TPitParam():
  oEntryLong(0.0),
  oExitLong(0.0),
  oExitLength(200.0f),
  oLaneEntryOffset(0.0),
  oLaneExitOffset(0.0),      
  oLatOffset(0),
  oLongOffset(0),
  oStoppingDist(0.0),
  oUseFirstPit(0),
  oUseSmoothPit(0)
{
}
//==========================================================================*

//==========================================================================*
// Destructor
//--------------------------------------------------------------------------*
TPitParam::~TPitParam()
{
}
//===========================================================================

//--------------------------------------------------------------------------*
// end of file unitpitparam.cpp
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
