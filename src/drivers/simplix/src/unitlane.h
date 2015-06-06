//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
// unitlane.h
//--------------------------------------------------------------------------*
// TORCS: "The Open Racing Car Simulator"
// A robot for Speed Dreams-Version 1.4.0/2.X
//--------------------------------------------------------------------------*
// Lane
// Fahrspur
//
// File         : unitlane.h
// Created      : 2007.11.17
// Last changed : 2011.06.07
// Copyright    : © 2007-2011 Wolf-Dieter Beelitz
// eMail        : wdb@wdbee.de
// Version      : 3.02.000
//--------------------------------------------------------------------------*
//--------------------------------------------------------------------------*
// Ein erweiterter TORCS-Roboters
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
#ifndef _UNITLANE_H_
#define _UNITLANE_H_

#include "unittrack.h"
#include "unitcarparam.h"
#include "unitfixcarparam.h"
#include "unitcubicspline.h"

//==========================================================================*
// Class TLane
//--------------------------------------------------------------------------*
class TLane 
{
  public:
	struct TPathPt
	{
		// Part 1: These data will be stored and reused from others as well
		TVec3d Center;                           // Lane specific center
		TVec3d Point;                            // Actual point (same as CalcPt())
		float Offset;                            // Offset from centre point
		float Crv;	 		                     // Curvature in xy
		float CrvZ;			                     // Curvature in z direction... e.g. bumps
		float NextCrv;                           // Cuvature comming next
		float WToL;                              // Lane specfic width to left
		float WToR;                              // Lane specfic width to right 
		float WPitToL;                           // Lane specfic width to left
		float WPitToR;                           // Lane specfic width to right 
		bool Fix;

		// Part 2: These data could be stored, but is recalculated from others
		// (So we don't have to store it)
		double MaxSpeed;                         // Max speed through this point
		double AccSpd;                           // Speed through this point, with modelled accel
		double Speed;                            // Speed through this point (braking only)
		double FlyHeight;                        // Predicted height of car above track (flying)

		// Part 3: These data may not be used from others (pointers)
		const TSection*	Sec;		             // Track seg that contains this Seg

		double Dist() const {return Sec->DistFromStart;}
		double WtoL() const {return WToL;}
		double WtoR() const {return WToR;}
		const TVec3d& Pt() const {return Center;}
		const TVec3d& Norm() const {return Sec->ToRight;}
		TVec3d CalcPt() const {return Center + Sec->ToRight * Offset;}
	};

  public:
    static const int TA_N = 10;                  // Nbr of points
    double TA_X[TA_N];                           // X-coordinates
    double TA_Y[TA_N];                           // Y-coordinates
    double TA_S[TA_N];                           // Directions

	TPathPt* oPathPoints;                        // Points in this lane

	TLane();
	virtual ~TLane();

	virtual TLane& operator= (const TLane& Lane);

	virtual bool ContainsPos
	  (double TrackPos) const;
	virtual bool GetLanePoint
	  (double TrackPos, TLanePoint& LanePoint) const;

	void SetLane(const TLane& Lane);
	void Initialise
	  (TTrackDescription* pTrack,
      const TFixCarParam& FixCarParam,
	  const TCarParam& CarParam,
	  double MaxLeft = FLT_MAX,
	  double MaxRight = FLT_MAX);
	void SmmothLane();

	const TPathPt& PathPoints(int Index) const;

	void Dump();
	void SmoothSpeeds();
	void CalcCurvaturesXY
	  (int Start, int Step = 1);
	void CalcCurvaturesZ
	  (int Start, int Step = 1);
	void CalcMaxSpeeds
	  (int Start, int Len, int Step = 1);
	void PropagateBreaking
	  (int Start, int Len, int Step = 1);
	void PropagatePitBreaking
	  (int Start, int Len, float PitStopPos, float ScaleMu);
	void PropagateAcceleration
	  (int Start, int Len, int Step = 1);

	void CalcCurvaturesXY
	  (int Step = 1);
	void CalcCurvaturesZ
	  (int Step = 1);
	void CalcMaxSpeeds
	  (int Step = 1);
	void PropagateBreaking
	  (int Step = 1);
	void PropagatePitBreaking
	  (int Start, float PitStopPos, float ScaleMu);
	void PropagateAcceleration
	  (int Step = 1);
	void CalcFwdAbsCrv
	  (int Range, int Step = 1);

	double CalcEstimatedTime
	  (int Start, int Len) const;
	double CalcEstimatedLapTime() const;
	double CalcTrackRollangle(double TrackPos);
	double CalcTrackTurnangle(int P, int Q);


  protected:
	TTrackDescription* oTrack;                   // TORCS track data
	TFixCarParam oFixCarParam;                   // Copy of car params
	TCarParam oCarParam;                         // Copy of car params
	TCubicSpline oTurnScale;                     // Scale of turns
    int Dummy;
};
//==========================================================================*
#endif // _UNITLANE_H_
//--------------------------------------------------------------------------*
// end of file unitlane.h
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
