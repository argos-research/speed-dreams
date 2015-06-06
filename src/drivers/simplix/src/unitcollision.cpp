//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
// unitcollision.cpp
//--------------------------------------------------------------------------*
// TORCS: "The Open Racing Car Simulator"
// A robot for Speed Dreams-Version 1.4.0/2.X
//--------------------------------------------------------------------------*
// Information about collisions to avoid
// Informationen über drohende Kollisionen
//
// File         : unitcollision.cpp
// Created      : 2007.11.25
// Last changed : 2013.07.05
// Copyright    : © 2007-2013 Wolf-Dieter Beelitz
// eMail        : wdb@wdbee.de
// Version      : 4.01.000
//--------------------------------------------------------------------------*
// Teile diese Unit basieren auf diversen Header-Dateien von TORCS
//
//    Copyright: (C) 2000 by Eric Espie
//    eMail    : torcs@free.fr
//
// dem erweiterten Robot-Tutorial bt
//
//    Copyright: (C) 2002-2004 Bernhard Wymann
//    eMail    : berniw@bluewin.ch
//
// dem Roboter mouse_2006
//
//    Copyright: (C) 2006-2007 Tim Foden
//
// dem Roboter wdbee_2007
//
//    Copyright: (C) 2006-2007 Wolf-Dieter Beelitz
//    eMail    : wdb@wdbee.de
//
// und dem Roboter delphin
//
//    Copyright: (C) 2006-2007 Wolf-Dieter Beelitz
//    eMail    : wdb@wdbee.de
//
//--------------------------------------------------------------------------*
// This program was developed and tested on windows XP
// There are no known Bugs, but:
// Who uses the files accepts, that no responsibility is adopted
// for bugs, dammages, aftereffects or consequential losses.
//
// Das Programm wurde unter Windows XP entwickelt und getestet.
// Fehler sind nicht bekannt, dennoch gilt:
// Wer die Dateien verwendet erkennt an, dass für Fehler, Schäden,
// Folgefehler oder Folgeschäden keine Haftung übernommen wird.
//--------------------------------------------------------------------------*
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// Im übrigen gilt für die Nutzung und/oder Weitergabe die
// GNU GPL (General Public License)
// Version 2 oder nach eigener Wahl eine spätere Version.
//--------------------------------------------------------------------------*
#include "unitglobal.h"
#include "unitcommon.h"

#include "unitdriver.h"
#include "unitcollision.h"

//==========================================================================*
// Default constructor
//--------------------------------------------------------------------------*
TCollision::TCollision()
{
}
//==========================================================================*

//==========================================================================*
// Destructor
//--------------------------------------------------------------------------*
TCollision::~TCollision()
{
}
//==========================================================================*

//==========================================================================*
// Avoid to side
//--------------------------------------------------------------------------*
double TCollision::AvoidTo
  (const TCollInfo& Coll, 
  const PCarElt oCar, TDriver& Me, bool& DoAvoid, double& TempOffset)
{
  int Flags = 0; 
  double AvoidTo = 0.0;                          // Undefined side
  double Offset = 0.0;

  // First priority: Anyone at side?
  if (Coll.OppsAtSide)                           // Opponents at side?
  {
    Flags = Coll.OppsAtSide;                     // Get corresponding flags
    AvoidTo = (Flags & F_LEFT) ? 1.0 : -1.0;     // Go away from opponent
	LogSimplix.debug("OppsAtSide: %g\n",AvoidTo);
  }
  else // No opponents at side ...
  {
	// Second priority: Lappers behind?
	if (Coll.LappersBehind)           
	{
	  Flags = Coll.LappersBehind;                // Get corresponding flags
	  if (Flags == (F_LEFT | F_RIGHT))
	  { // lapping cars on both sides behind!
	    Flags =                                  // Use the side defined
		  (Coll.NextSide < 0) ? F_LEFT : F_RIGHT;//   by collision
		AvoidTo = (Flags & F_LEFT) ? 1.0 : -1.0;
		LogSimplix.debug("LappersBehind: %g\n",AvoidTo);
	  }
	  else
	  {
		AvoidTo = (Flags & F_LEFT) ? 1.0 : -1.0;
		LogSimplix.debug("Lapper Behind: %g\n",AvoidTo);
	  }
	}
	// Third priority: More than one ahead?
    else if (Coll.OppsAhead == (F_LEFT | F_RIGHT))
    { // cars on both sides ahead, so avoid closest (or slowest) car
      Flags = (Coll.MinLSideDist < Coll.MinRSideDist) ? F_LEFT : F_RIGHT; 
	  AvoidTo = (Flags & F_LEFT) ? 1.0 : -1.0;
	  LogSimplix.debug("(Coll.OppsAhead == (F_LEFT | F_RIGHT)): %g\n",AvoidTo);
	}
	// Fourth priority: Anyone ahead? 
    else if (Coll.OppsAhead)
	{ // cars on one side ahead
      Flags = Coll.Flags;
	  AvoidTo = (Flags & F_TRK_LEFT) ? 1.0 : -1.0;
      LogSimplix.debug("(Coll.OppsAhead): %g\n",AvoidTo);
	}
	// Fifth priority: Anyone behind faster? 
    else if (Coll.OppsBehindFaster)
	{ 
      Flags = Coll.Flags;
	  AvoidTo = (Flags & F_TRK_LEFT) ? 1.0 : -1.0;
      LogSimplix.debug("(Coll.OppsBehindFaster): %g\n",AvoidTo);
	}
	else 
	{
      //LogSimplix.debug("AvoidTo5: %g\n",AvoidTo);
      return AvoidTo;                            // Do not avoid
	}

  }

  // We have to avoid
  DoAvoid = true;                              // Avoid to side
  double O = 0.0;

  if (Flags == (F_LEFT | F_RIGHT))             // Opps on both sides?
  {                                            //   Then use middle
    TempOffset = 0.5 *                         // Offset is an estimate
  	(Coll.MinRSideDist - Coll.MinLSideDist)    //   of where this is.
  	- CarToMiddle;
  }
  else
  {
    double OR = 0.0;
    double OL = 0.0;
    Me.DistBetweenRL(oCar,OL,OR,O);
    if (AvoidTo > 0)
    {
	  TempOffset = OR;
/*
  	  double B = MAX(0.0,Coll.ToR - 4.0);
	  TempOffset = OR - B/3;
	  if (TempOffset < O)
		TempOffset = O;
*/
    }
    else if (AvoidTo < 0)
    {
	  TempOffset = OL;
/*
	  double B = MAX(0.0,Coll.ToL - 4.0);
	  TempOffset = OL + B/3;
	  if (TempOffset > O)
		TempOffset = O;
*/
    }
  }

  Offset = Me.CalcPathTarget                   // Use offset to
    (DistanceFromStartLine, TempOffset);       //   find target

  LogSimplix.debug("DoAvoid Offset: S%g(I%g;D%g)\n",Offset,O+CarToMiddle,Offset-(O+CarToMiddle));

  return Offset; 
}
//--------------------------------------------------------------------------*
// end of file unitcollision.cpp
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
