//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
// unittrack.h
//--------------------------------------------------------------------------*
// A robot for Speed Dreams-Version 2.X simuV4
//--------------------------------------------------------------------------*
// Streckenbeschreibung
// (C++-Portierung der Unit UnitTrack.pas)
//
// File         : unittrack.h
// Created      : 2007.11.17
// Last changed : 2014.11.29
// Copyright    : © 2007-2014 Wolf-Dieter Beelitz
// eMail        : wdbee@users.sourceforge.net
// Version      : 4.05.000
//--------------------------------------------------------------------------*
// Stellt Funktionen zur Streckenbeschreibung zur Verfügung
//--------------------------------------------------------------------------*
// Teile diese Unit basieren auf diversen Header-Dateien von TORCS
//
//    Copyright: (C) 2000 by Eric Espie
//    eMail    : torcs@free.fr
//
// dem Robot berniw two
//
//    Copyright: (C) 2000-2002 by Bernhard Wymann
//    eMail    : berniw@bluewin.ch
//
// dem Roboter delphin
//
//    Copyright: (C) 2006-2007 Wolf-Dieter Beelitz
//    eMail    : wdbee@users.sourceforge.net
//
// dem Roboter wdbee_2007
//
//    Copyright: (C) 2006-2007 Wolf-Dieter Beelitz
//    eMail    : wdbee@users.sourceforge.net
//
// und dem Roboter mouse_2006
//    Copyright: (C) 2006 Tim Foden
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
#ifndef _UNITTRACK_H_
#define _UNITTRACK_H_

// TORCS
#include <track.h>
#include <car.h>

// This robot
#include "unitglobal.h"
#include "unitcommon.h"
#include "unitcarparam.h"
#include "unitsection.h"
#include "unitlanepoint.h"

//==========================================================================*
// Class TTrackDescription
//--------------------------------------------------------------------------*
class TTrackDescription
{
  public:
	struct PitSideMod
	{
		PitSideMod() : side(-1), start(0), end(0) {}

		int		side;                            // side of pitlane
		int		start;                           // start of pitlane
		int		end;                             // end of pitlane
	};

  public:
	TTrackDescription();                         // Default constructor
	~TTrackDescription() ;                       // destructor

	double CalcPos                               // Calc position from offset
	  (tTrkLocPos& TrkPos, double Offset = 0) const;
	double CalcPos                               // Calc cars position from offset
	  (tCarElt* Car, double Offset = 0) const;
	double CalcPos                               // Calc position from coordinates
	  (float X, float Y,
	  const TSection* Hint = 0,
	  bool Sides = false) const;

	int Count() const;                           // nbr of section in track
    void Execute();                              // Make description of track
	double Friction(int Index) const;            // Friction of section[Index]
	double ForwardAngle(double TrackPos) const;  // Angle
    double LearnFriction
      (int Index, double Delta, double MinFriction);
    void InitialTargetSpeed(int Index, double TargetSpeed);
    double InitialTargetSpeed(int Index);

	int IndexFromPos                             // Get sections index from pos
	  (double TrackPos) const;
	void InitTrack                               // Initialize Track
	  (tTrack* Track, TCarParam& CarParam, PitSideMod* PitSideMod = 0);
	double MeanSectionLen() const;               // Mean length of sections
	TVec2d Normale(double TrackPos) const;        // To right
	double NormalizePos(double TrackPos) const;  // Keep pos in 0..Tracklength

	double Length() const;                       // Length of track in m

	const TSection&	operator[](int Index) const; // section of index
	const TSection&	Section(int Index) const;    // section of index
    void SmoothSides(double Delta);              // Smooth width changes

	tTrack* Track();                             // Get TORCS track data
	const tTrack* Track() const;                 // TORCS track data as const
	double Width() const;                        // Const track width

  private:
    void BuildPos2SecIndex();
    int NbrOfSections
	  (double Len, bool PitSection);             // Estimate nbr of sections
	void NormalizeDir                            // Calc Center and ToRight
	  (const tTrackSeg* pSeg, double toStart,
      double& T, TVec3d& Pt, TVec3d& Norm) const;

  private:
	int	oCount;                                  // Number of sections in track
	double oMeanSectionLen;                      // Mean length of sections
	TSection* oSections;                         // Array of sections
	tTrack*	oTrack;                              // TORCS data of track
    double oTrackRes;                            // sampling rate (per m)
    int oPitEntry;                               // Pit entry
    int oPitExit;                                // Pit exit
    int oPitSide;                                // Pit side
	PitSideMod oPitSideMod;                      // Pit side mode
};
//==========================================================================*
#endif // _UNITTRACK_H_
//--------------------------------------------------------------------------*
// end of file unittrack.h
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
