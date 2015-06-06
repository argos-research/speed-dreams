//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
// unitpitparam.h
//--------------------------------------------------------------------------*
// TORCS: "The Open Racing Car Simulator"
// A robot for Speed Dreams-Version 1.4.0/2.X
//--------------------------------------------------------------------------*
// Parameter der Box und der Anfahrt zur Box
//
// File         : unitpitparam.h
// Created      : 2007.04.14
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
#ifndef _UNITPITPARAM_H_
#define _UNITPITPARAM_H_

//==========================================================================*
// Deklaration der Klasse TPitParam
//--------------------------------------------------------------------------*
class TPitParam  
{
  private:

  public:
    TPitParam();                                 // Default constructor 
	~TPitParam();                                // Destructor

  public:
	double oEntryLong;	   	                     // Translation longitudinal
	double oExitLong;		                     // Translation longitudinal
    float oExitLength;                           // Dist in m

	double oLaneEntryOffset;                     // Additional offset to pit
	double oLaneExitOffset;                      // Additional offset to pit

	double oLatOffset;                           // Lateral offset of pit
	double oLongOffset;                          // Longitudinal offset of pit

	double oStoppingDist;                        // Stopping distance 

	int oUseFirstPit;                            // Use special path to first pit
    int oUseSmoothPit;                           // Use smoothing pitlane

};
//==========================================================================*
#endif // _UNITPITPARAM_H_
//--------------------------------------------------------------------------*
// end of file unitpitparam.h
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
