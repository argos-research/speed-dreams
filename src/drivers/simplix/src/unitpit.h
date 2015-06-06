//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
// unitpit.h
//--------------------------------------------------------------------------*
// TORCS: "The Open Racing Car Simulator"
// A robot for Speed Dreams-Version 1.4.0/2.X
//--------------------------------------------------------------------------*
// Pit ans pitlane
// Box und Boxengasse
//
// File         : unitpit.h
// Created      : 2007.02.20
// Last changed : 2011.06.02
// Copyright    : © 2007-2011 Wolf-Dieter Beelitz
// eMail        : wdb@wdbee.de
// Version      : 3.01.000
//--------------------------------------------------------------------------*
// Diese Unit basiert auf dem erweiterten Robot-Tutorial bt
//
//    Copyright: (C) 2002-2004 Bernhard Wymann
//    eMail    : berniw@bluewin.ch
//
// und dem Roboter delphin 2006
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
#ifndef _PIT_H_
#define _PIT_H_

#include "unitglobal.h"
#include "unitcommon.h"
#include "unitdriver.h"
#include "unittrack.h"
#include "unitclothoid.h"
#include "unitparam.h"
#include "unitstrategy.h"

//==========================================================================*
// Klasse TPitLane
//--------------------------------------------------------------------------*
class TPitLane : public TClothoidLane
{
  private:
	PtCarElt oCar;                               // TORCS data of car

	double oPitDist;                             // Distance to decide
	double oPitEndPos;                           // End of speed limit
    double oPitEntryStartPos;                    // Start of pitlane entry
    double oPitEntryEndPos;                      // End of pitlane entry
	double oPitExitStartPos;                     // Start of pitlane exit
	double oPitExitEndPos;                       // End of pitlane exit
	double oPitStartPos;                         // Start of speed limit
	double oPitStopPos;                          // Position of pit
	double oStoppingDist;                        // Stopping distance
	double oPitStopOffset;                       // Offset from TORCS point

	int	oStopIdx;                                // Index of section of pit
	double oStopPos;                             // Position to stop

  private:
	double ToSplinePos(double TrackPos) const;   //

  public:
	void Init(PtCarElt Car);                     // Initialize oCar
    void MakePath                                // Build pitlane
	  (char* Filename,
	  TAbstractStrategy* Strategy,
	  TClothoidLane* BasePath,
	  const TParam& Param,
	  int Index);
    void SmoothPitPath
      (const TParam& Param);

	bool InPitSection(double TrackPos) const;
	bool CanStop(double TrackPos) const;
	bool Overrun(double TrackPos) const;

	double DistToPitEntry(double TrackPos) const;
	double DistToPitStop(double TrackPos, bool Pitting) const;
	double PitDist(){return oPitDist;};
	double StoppingDist(){return oStoppingDist;};
};
//==========================================================================*

//==========================================================================*
// Klasse TPit
//--------------------------------------------------------------------------*
class TPit
{
  public:
	PTrack oTrack;                               // TORCS track data
	PCarElt oCar;                                // TORCS car data
	TPitLane oPitLane[gNBR_RL];                  // Pitlanes
	tTrackOwnPit *oMyPit;			             // Pointer to my pit.
	tTrackPitInfo *oPitInfo;	                 // General pit info.

	bool oPitStop;					             // Pitstop planned.
	bool oInPitLane;			 		         // We are still in the pitlane.
	float oPitEntry;				  	         // Distance to start line of the pit entry.
	float oPitExit;					             // Distance to the start line of the pit exit.
	double oDistToPitEnd;                        // Remaining len of pitlane

	float oSpeedLimitSqr;			             // Pit speed limit squared.
	float oSpeedLimit;				             // Pit speed limit.
	float oPitSpeedLimitSqr;			         // The original speedlimit squared.

	float oPitTimer;					         // Timer for pit timeouts.

	static const float SPEED_LIMIT_MARGIN;

	TPit(TDriver *driver);
	~TPit();

	void SetPitstop(bool oPitStop);
	bool GetPitstop()
	  {return oPitStop;}

	void SetInPit(bool InPitLane)
	  {this->oInPitLane = InPitLane;}
	bool GetInPit()
	  {return oInPitLane;}

	float GetPitOffset(float Offset, float FromStart);

	bool IsBetween(float FromStart);
	bool IsTimeout(float Distance);

	float ToSplineCoord(float X);
	float GetSpeedLimitBrake(float SpeedSqr);

	void Update();

	bool HasPits()
	  {return oMyPit != NULL;}

};
//==========================================================================*
#endif // _PIT_H_
//--------------------------------------------------------------------------*
// end of file unitpit.h
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*


