//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
// unitlane.cpp
//--------------------------------------------------------------------------*
// A robot for Speed Dreams-Version 2.X simuV4
//--------------------------------------------------------------------------*
// Lane
// Fahrspur
//
// File         : unitlane.cpp
// Created      : 2007.11.25
// Last changed : 2014.11.29
// Copyright    : © 2007-2014 Wolf-Dieter Beelitz
// eMail        : wdbee@users.sourceforge.net
// Version      : 4.05.000
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
//
//    Copyright: (C) 2006-2007 Tim Foden
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
#include <robottools.h>

#include "unitglobal.h"
#include "unitcommon.h"

#include "unitcubicspline.h"
#include "unitdriver.h"
#include "unitlane.h"
#include "unitlinalg.h"
#include "unittmpcarparam.h"

//==========================================================================*
// Default constructor
//--------------------------------------------------------------------------*
TLane::TLane():
  oPathPoints(NULL),
  oTrack(NULL)
{
}
//==========================================================================*

//==========================================================================*
// Destructor
//--------------------------------------------------------------------------*
TLane::~TLane()
{
  delete [] oPathPoints;
}
//==========================================================================*

//==========================================================================*
// Set operator (Sets lane)
//--------------------------------------------------------------------------*
TLane& TLane::operator= (const TLane& Lane)
{
  SetLane(Lane);
  return *this;
}
//==========================================================================*

//==========================================================================*
// Set lane
//--------------------------------------------------------------------------*
void TLane::SetLane(const TLane& Lane)
{
  oTrack = Lane.oTrack;
  oFixCarParam = Lane.oFixCarParam;
  oCarParam = Lane.oCarParam;

  const int Count = oTrack->Count();

  delete [] oPathPoints;
  oPathPoints = new TPathPt[Count];

  memcpy(oPathPoints, Lane.oPathPoints, Count * sizeof(*oPathPoints));

  for (int I = 0; I < TA_N; I++)
  {
    TA_X[I] = Lane.TA_X[I];
    TA_Y[I] = Lane.TA_Y[I];
    TA_S[I] = Lane.TA_S[I];
  }
  oTurnScale.Init(TA_N,TA_X,TA_Y,TA_S);
}
//==========================================================================*

//==========================================================================*
// Check wether position is in lane
//--------------------------------------------------------------------------*
bool TLane::ContainsPos(double TrackPos) const
{
  if (TrackPos > 0.0)
    return true;                                 // Allways true because
  else                                           // this lane type
    return true;                                 // contains all points
}
//==========================================================================*

//==========================================================================*
// Get information to the point nearest the given position
//--------------------------------------------------------------------------*
bool TLane::GetLanePoint(double TrackPos, TLanePoint& LanePoint) const
{
  int Count = oTrack->Count();

  int Idx0 = oTrack->IndexFromPos(TrackPos);
  int Idxp = (Idx0 - 1 + Count) % Count;
  int Idx1 = (Idx0 + 1) % Count;
  int Idx2 = (Idx0 + 2) % Count;

  double Dist0 = oPathPoints[Idx0].Dist();
  double Dist1 = oPathPoints[Idx1].Dist();
  if (Idx1 == 0)
    Dist1 = oTrack->Length();

  TVec3d P0 = oPathPoints[Idxp].CalcPt();
  TVec3d P1 = oPathPoints[Idx0].CalcPt();
  TVec3d P2 = oPathPoints[Idx1].CalcPt();
  TVec3d P3 = oPathPoints[Idx2].CalcPt();

  double Crv1 = TUtils::CalcCurvatureXY(P0, P1, P2);
  double Crv2 = TUtils::CalcCurvatureXY(P1, P2, P3);
  double Crv1z = TUtils::CalcCurvatureZ(P0, P1, P2);
  double Crv2z = TUtils::CalcCurvatureZ(P1, P2, P3);

  double Tx = (TrackPos - Dist0) / (Dist1 - Dist0);

  LanePoint.Index = Idx0;
  LanePoint.Crv = (1.0 - Tx) * Crv1 + Tx * Crv2;
  LanePoint.Crvz = (1.0 - Tx) * Crv1z + Tx * Crv2z;

  //LogSimplix.error("#0:%.3f 1:%.3f 2:%.3f 3:%.3f CZ1:%.3f CZ2:%.3f CZ:%.3f\n",P0.z,P1.z,P2.z,P3.z,Crv1z,Crv2z,LanePoint.Crvz);

  LanePoint.T = Tx;
  LanePoint.Offset =
	(oPathPoints[Idx0].Offset)
	+ Tx * (oPathPoints[Idx1].Offset - oPathPoints[Idx0].Offset);

  double Ang0 = TUtils::VecAngXY(oPathPoints[Idx1].CalcPt() -
	oPathPoints[Idx0].CalcPt());
  double Ang1 = TUtils::VecAngXY(oPathPoints[Idx2].CalcPt() -
	oPathPoints[Idx1].CalcPt());

  double DeltaAng = Ang1 - Ang0;
  DOUBLE_NORM_PI_PI(DeltaAng);
  LanePoint.Angle = Ang0 + LanePoint.T * DeltaAng;

  TVec2d Tang1, Tang2;
  TUtils::CalcTangent(P0.GetXY(), P1.GetXY(), P2.GetXY(), Tang1);
  TUtils::CalcTangent(P1.GetXY(), P2.GetXY(), P3.GetXY(), Tang2);
  //TVec2d Dir = TUtils::VecUnit(Tang1) * (1 - Tx) + TUtils::VecUnit(Tang2) * Tx;

  Ang0 = TUtils::VecAngle(Tang1);
  Ang1 = TUtils::VecAngle(Tang2);
  DeltaAng = Ang1 - Ang0;
  DOUBLE_NORM_PI_PI(DeltaAng);

  LanePoint.Speed = oPathPoints[LanePoint.Index].Speed + (oPathPoints[Idx1].Speed
	- oPathPoints[LanePoint.Index].Speed) * LanePoint.T;
  LanePoint.AccSpd = oPathPoints[LanePoint.Index].AccSpd + (oPathPoints[Idx1].AccSpd
	- oPathPoints[LanePoint.Index].AccSpd) * LanePoint.T;

  return true;
}
//==========================================================================*

//==========================================================================*
// Initialize lane from track limiting width to left and right
//--------------------------------------------------------------------------*
void TLane::Initialise
  (TTrackDescription* Track,
  const TFixCarParam& FixCarParam,
  const TCarParam& CarParam,
  double MaxLeft, double MaxRight)
{
  delete [] oPathPoints;
  oTrack = Track;
  oPathPoints = new TPathPt[Track->Count()];
  oCarParam = CarParam;                          // Copy car params
  oFixCarParam = FixCarParam;                    // Copy car params

  // To avoid uninitialized alignment bytes within the allocated memory for 
  // linux compilers not doing this initialization without our explizit 
  // request we should fill it with zeros. Otherwise we would get valgrind
  // warnings writing the data to file using only one memory block per
  // path point.
  memset(oPathPoints, 0, Track->Count() * sizeof(*oPathPoints));

  if (MaxLeft < 999.0)
  {
    for (int I = 0; I < Track->Count(); I++)
    {
	  const TSection& Sec = (*oTrack)[I];
	  oPathPoints[I].Sec = &Sec;
	  oPathPoints[I].Center = Sec.Center;
	  oPathPoints[I].Crv = 0;
	  oPathPoints[I].CrvZ	= 0;
	  oPathPoints[I].Offset = 0.0;
	  oPathPoints[I].Point = oPathPoints[I].CalcPt();
	  oPathPoints[I].MaxSpeed	= 10;
	  oPathPoints[I].Speed = 10;
	  oPathPoints[I].AccSpd = 10;
	  oPathPoints[I].FlyHeight = 0;
//	  oPathPoints[I].BufL	= 0;
//	  oPathPoints[I].BufR	= 0;
	  oPathPoints[I].NextCrv = 0.0;
	  oPathPoints[I].WToL = (float) MaxLeft;
  	  oPathPoints[I].WToR = (float) Sec.WidthToRight;
	  oPathPoints[I].WPitToL = (float) Sec.PitWidthToLeft;
      oPathPoints[I].WPitToR = (float) Sec.PitWidthToRight;
	  oPathPoints[I].Fix = false;
    }
    oPathPoints[0].WToL = oPathPoints[1].WToL;
	oPathPoints[0].WToR = oPathPoints[1].WToR;
  }
  else if (MaxRight < 999.0)
  {
    for (int I = 0; I < Track->Count(); I++)
    {
	  const TSection& Sec = (*oTrack)[I];
	  oPathPoints[I].Sec = &Sec;
	  oPathPoints[I].Center	= Sec.Center;
	  oPathPoints[I].Crv = 0;
	  oPathPoints[I].CrvZ = 0;
	  oPathPoints[I].Offset = 0.0;
	  oPathPoints[I].Point = oPathPoints[I].CalcPt();
	  oPathPoints[I].MaxSpeed = 10;
	  oPathPoints[I].Speed = 10;
	  oPathPoints[I].AccSpd	= 10;
	  oPathPoints[I].FlyHeight = 0;
//	  oPathPoints[I].BufL = 0;
//	  oPathPoints[I].BufR = 0;
	  oPathPoints[I].NextCrv = 0.0;
	  oPathPoints[I].WToL = (float) Sec.WidthToLeft;
	  oPathPoints[I].WToR = (float) MaxRight;
      oPathPoints[I].WPitToL = (float) Sec.PitWidthToLeft;
      oPathPoints[I].WPitToR = (float) Sec.PitWidthToRight;
	  oPathPoints[I].Fix = false;
    }
    oPathPoints[0].WToL = oPathPoints[1].WToL;
	oPathPoints[0].WToR = oPathPoints[1].WToR;
  }
  else
  {
    for (int I = 0; I < Track->Count(); I++)
    {
	  const TSection& Sec = (*oTrack)[I];
	  oPathPoints[I].Sec = &Sec;
	  oPathPoints[I].Center = Sec.Center;
	  oPathPoints[I].Crv = 0;
	  oPathPoints[I].CrvZ = 0;
	  oPathPoints[I].Offset = 0.0;
	  oPathPoints[I].Point = oPathPoints[I].CalcPt();
	  oPathPoints[I].MaxSpeed	= 10;
	  oPathPoints[I].Speed = 10;
	  oPathPoints[I].AccSpd = 10;
	  oPathPoints[I].FlyHeight = 0;
//	  oPathPoints[I].BufL	= 0;
//	  oPathPoints[I].BufR	= 0;
	  oPathPoints[I].NextCrv = 0.0;
      oPathPoints[I].WToL = (float) Sec.WidthToLeft;
	  oPathPoints[I].WToR = (float) Sec.WidthToRight;
      oPathPoints[I].WPitToL = (float) Sec.PitWidthToLeft;
      oPathPoints[I].WPitToR = (float) Sec.PitWidthToRight;
	  oPathPoints[I].Fix = false;
    }
    oPathPoints[0].WToL = oPathPoints[1].WToL;
	oPathPoints[0].WToR = oPathPoints[1].WToR;

  }
  CalcCurvaturesXY();
  CalcCurvaturesZ();

  TA_X[0] = 0.0;
  TA_X[1] = 0.4;
  TA_X[2] = 0.5;
  TA_X[3] = 0.6;
  TA_X[4] = 0.7;
  TA_X[5] = 0.8;
  TA_X[6] = 0.9;
  TA_X[7] = 1.0;
  TA_X[8] = 1.1;
  TA_X[9] = 10.0;
/*
  TA_Y[0] = 1.0;
  TA_Y[1] = 1.0;
  TA_Y[2] = 0.995;
  TA_Y[3] = 0.97;
  TA_Y[4] = 0.9;
  TA_Y[5] = 0.7;
  TA_Y[6] = 0.5;
  TA_Y[7] = 0.35;
  TA_Y[8] = 0.305;
  TA_Y[9] = 0.30;
*/
  TA_Y[0] = 1.0;
  TA_Y[1] = 1.0;
  TA_Y[2] = 1.0;
  TA_Y[3] = 0.995;
  TA_Y[4] = 0.97;
  TA_Y[5] = 0.9;
  TA_Y[6] = 0.7;
  TA_Y[7] = 0.55;
  TA_Y[8] = 0.505;
  TA_Y[9] = 0.50;

  TA_S[0] = 0.0;
  TA_S[9] = 0.0;

  oTurnScale.Init(TA_N,TA_X,TA_Y,TA_S);         
}
//==========================================================================*

//==========================================================================*
// Get path point from index
//--------------------------------------------------------------------------*
const TLane::TPathPt& TLane::PathPoints(int Index) const
{
  return oPathPoints[Index];
}
//==========================================================================*

//==========================================================================*
// Calc curvature in XY
//--------------------------------------------------------------------------*
void TLane::CalcCurvaturesXY(int Start, int Step)
{
  const int N = oTrack->Count();

  for (int I = 0; I < N; I++)
  {
	int	P  = (Start + I) % N;                    // Point
	int	Pp = (P - Step + N) % N;                 // Prev Point
	int	Pn = (P + Step) % N;                     // Next Point

	oPathPoints[P].Crv = (float)
	  TUtils::CalcCurvatureXY(
	    oPathPoints[Pp].CalcPt(),
	    oPathPoints[P].CalcPt(),
 	    oPathPoints[Pn].CalcPt());
  }

  // Overwrite values at start to avoid slowdown caused by track errors
  for (int I = 0; I <= Step; I++)
  {
    oPathPoints[I].Crv = 0.0;
    oPathPoints[N-1-I].Crv = 0.0;
  }
}
//==========================================================================*

//==========================================================================*
// Calc curvature in Z
//--------------------------------------------------------------------------*
void TLane::CalcCurvaturesZ(int Start, int Step)
{
  const int N = oTrack->Count();

  Step *= 3;

  for (int I = 0; I < N; I++)
  {
	int	P  = (Start + I) % N;                    // Point
	int	Pp = (P - Step + N) % N;                 // Prev Point
	int	Pn = (P + Step) % N;                     // Next Point

	oPathPoints[P].CrvZ = 6 * (float) TUtils::CalcCurvatureZ(
	  oPathPoints[Pp].CalcPt(),
      oPathPoints[P].CalcPt(),
	  oPathPoints[Pn].CalcPt());
  }

  // Overwrite values at start to avoid slowdown caused by track errors
  for (int I = 0; I <= Step; I++)
  {
    oPathPoints[I].CrvZ = 0.0;
    oPathPoints[N-1-I].CrvZ = 0.0;
  }
}
//==========================================================================*

//==========================================================================*
// Calc max possible speed depending on car modell
//--------------------------------------------------------------------------*
void TLane::CalcMaxSpeeds
  (int Start, int Len, int Step)
{
  const int N = oTrack->Count();

  for (int I = 0; I < Len; I += Step)
  {
	int P = (Start + I) % N;
	int Q = (P + 1) % N;
    TVec3d Delta = oPathPoints[P].CalcPt() - oPathPoints[Q].CalcPt();
    double Dist = TUtils::VecLenXY(Delta);
    double TrackRollAngle = atan2(oPathPoints[P].Norm().z, 1);
    double TrackTiltAngle = 1.1 * atan2(Delta.z, Dist);
    double CrvZ = oPathPoints[Q].CrvZ;
		 
	double Speed = oFixCarParam.CalcMaxSpeed(
      oCarParam,
      oPathPoints[P].Crv,
      oPathPoints[Q].Crv,
	  CrvZ,
	  oTrack->Friction(P),
  	  TrackRollAngle,
  	  TrackTiltAngle);

	if (TDriver::UseGPBrakeLimit)
	{

      //double TrackTurnangle1 = CalcTrackTurnangle(P, (P + 30) % N);
      //double TrackTurnangle2 = 0.7 * CalcTrackTurnangle((P + N - 30) % N, P);
      //double TrackTurnangle = MAX(fabs(TrackTurnangle1),fabs(TrackTurnangle2));
	  //Speed *= oTurnScale.CalcOffset(TrackTurnangle);
	}
	else
	{
      double TrackTurnangle = CalcTrackTurnangle(P, (P + 50) % N);
      if (TrackTurnangle > 0.7)
	    Speed *= 0.75;
      if (TrackTurnangle < 0.2)
	    Speed *= 1.05;
	}

	if (Speed < 5)
		Speed = 5.0;

	oPathPoints[P].MaxSpeed = Speed;
	oPathPoints[P].Speed = Speed;
	oPathPoints[P].AccSpd = Speed;
	if (TDriver::FirstPropagation)
	  oTrack->InitialTargetSpeed(P,Speed);
  }
}
//==========================================================================*

//==========================================================================*
// Smooth speeds
//--------------------------------------------------------------------------*
void TLane::SmoothSpeeds()
{
  const int N = oTrack->Count();

  for (int I = 0; I < N; I++)
  {
	int P = I % N;
	int Q = (P + 2) % N;

	double Speed = oPathPoints[Q].Speed;
	if (oPathPoints[P].Speed < Speed)
	{
      LogSimplix.error("# Speed %g <= %g\n",oPathPoints[P].Speed,Speed);
  	  oPathPoints[P].MaxSpeed = Speed;
	  oPathPoints[P].Speed = Speed;
	  oPathPoints[P].AccSpd = Speed;
	}
  }
}
//==========================================================================*

//==========================================================================*
// Dump
//--------------------------------------------------------------------------*
void TLane::Dump()
{
  const int N = oTrack->Count();

  for (int I = 0; I < N; I++)
  {
	int P = I % N;
    LogSimplix.error("#%d %.3f\n",I,oPathPoints[P].CrvZ);
  }
}
//==========================================================================*

//==========================================================================*
// Propagate braking
//--------------------------------------------------------------------------*
void TLane::PropagateBreaking
  (int Start, int Len, int Step)
{
  const int N = oTrack->Count();

  for (int I = Step * ((2*Len - 1) / Step); I >= 0; I -= Step )
  {
	int	P = (Start + I) % N;
	int Q = (P + Step) % N;

	if (oPathPoints[P].Speed > oPathPoints[Q].Speed)
	{
	  // see if we need to adjust spd[i] to make it possible
	  //   to slow to spd[j] by the next seg.
      TVec3d Delta = oPathPoints[P].CalcPt() - oPathPoints[Q].CalcPt();
      double Dist = TUtils::VecLenXY(Delta);
      double K = (oPathPoints[P].Crv + oPathPoints[Q].Crv) * 0.5;
	  if (fabs(K) > 0.0001)
	    Dist = 2 * asin(0.5 * Dist * K) / K;
	  double TrackRollAngle = atan2(oPathPoints[P].Norm().z, 1);
	  double TrackTiltAngle = 1.1 * atan2(Delta.z, Dist);

	  double U = oFixCarParam.CalcBraking(
        oCarParam,
  		oPathPoints[P].Crv,
		oPathPoints[P].CrvZ,
		oPathPoints[Q].Crv,
		oPathPoints[Q].CrvZ,
		oPathPoints[Q].Speed,
		Dist,
		oTrack->Friction(P),
		TrackRollAngle,
		TrackTiltAngle);

	  if (oPathPoints[P].Speed > U)
		oPathPoints[P].Speed = oPathPoints[P].AccSpd = U;

	  if (oPathPoints[P].FlyHeight > 0.1)
		oPathPoints[P].Speed = oPathPoints[Q].Speed;

	}
  }
}
//==========================================================================*

//==========================================================================*
// Propagate braking
//--------------------------------------------------------------------------*
void TLane::PropagatePitBreaking
  (int Start, int Len, float PitStopPos, float ScaleMu)
{
  /*const float base = 0.5f; */
  int Step = 1;
  int L = 10;
  const int N = oTrack->Count();
  const int M = Step * ((Len - 1) / Step);

  //LogSimplix.error("Start: %d (%g m)",Start,PitStopPos);

  for (int I = M; I >= 0; I -= Step )
  {
	int	P = (Start - 1 + I - M + N) % N;
	int Q = (P + Step) % N; 

	if (oPathPoints[P].Speed > oPathPoints[Q].Speed)
	{
	  // see if we need to adjust spd[i] to make it possible
	  //   to slow to spd[j] by the next seg.
      TVec3d Delta = oPathPoints[P].CalcPt() - oPathPoints[Q].CalcPt();
      double Dist = TUtils::VecLenXY(Delta);
      double K = (oPathPoints[P].Crv + oPathPoints[Q].Crv) * 0.5;
	  if (fabs(K) > 0.0001)
	    Dist = 2 * asin(0.5 * Dist * K) / K;
	  double TrackRollAngle = atan2(oPathPoints[P].Norm().z, 1);
	  double TrackTiltAngle = 1.1 * atan2(Delta.z, Dist);

	  double Factor = MIN(1.0,fabs(oPathPoints[Q].Dist() - PitStopPos) / oFixCarParam.oPitBrakeDist);
	  double Friction = oTrack->Friction(P) * (Factor * ScaleMu + (1 - Factor) * oCarParam.oScaleBrakePit * ScaleMu);
	  if (L)
		Friction *= 0.5;

	  //LogSimplix.debug("F %g: %g/%g )",Factor,oTrack->Friction(P),Friction);
	  //LogSimplix.debug("SQ %g )",oPathPoints[Q].Speed);

	  double U = oFixCarParam.CalcBrakingPit(
        oCarParam,
  		oPathPoints[P].Crv,
		oPathPoints[P].CrvZ,
		oPathPoints[Q].Crv,
		oPathPoints[Q].CrvZ,
		oPathPoints[Q].Speed,
		Dist,
		Friction,
		TrackRollAngle,
		TrackTiltAngle); 

	  if (L) 
	  {
		  L--;
		  double DeltaSpeed = (U - oPathPoints[Q].Speed);
		  if (DeltaSpeed > 0.5)
			  U = 0.5 + oPathPoints[Q].Speed;
	  }

	  if (oPathPoints[P].Speed > U)
		oPathPoints[P].Speed = oPathPoints[P].AccSpd = U;

	  if (!L) 
	    if (oPathPoints[P].FlyHeight > 0.1)
	      oPathPoints[P].Speed = oPathPoints[Q].Speed;

	  //LogSimplix.debug("SP %g\n)",oPathPoints[P].Speed);
      //LogSimplix.debug("I:%d P:%d Q:%d F:%.3f U:%.2f S:%.2f\n",I,P,Q,Factor,U*3.6,oPathPoints[P].Speed*3.6);

	}
  }
}
//==========================================================================*

//==========================================================================*
// Propagate acceleration
//--------------------------------------------------------------------------*
void TLane::PropagateAcceleration
  (int Start, int Len, int Step)
{
  const int N = oTrack->Count();

  for (int I = 0; I < 2*Len; I += Step )
  {
	int Q = (Start + I + N) % N;
	int	P = (Q - Step + N) % N;

	if (Q == 0)
	  P = (N - 3);

	if (oPathPoints[P].AccSpd < oPathPoints[Q].AccSpd)
	{
	  // see if we need to adjust spd[Q] to make it possible
	  //   to speed up to spd[P] from spd[Q].
      TVec3d Delta = oPathPoints[P].CalcPt() - oPathPoints[Q].CalcPt();
      double Dist = TUtils::VecLenXY(Delta);

	  double K = (oPathPoints[P].Crv + oPathPoints[Q].Crv) * 0.5;
	  if (fabs(K) > 0.0001)
	    Dist = 2 * asin(0.5 * Dist * K) / K;

	  double TrackRollAngle = atan2(oPathPoints[P].Norm().z, 1);
	  double TrackTiltAngle = 1.1 * atan2(Delta.z, Dist);

	  double V = oFixCarParam.CalcAcceleration(
	    oPathPoints[P].Crv,
		oPathPoints[P].CrvZ,
		oPathPoints[Q].Crv,
		oPathPoints[Q].CrvZ,
		oPathPoints[P].AccSpd,
		Dist,
		oTrack->Friction(P),
		TrackRollAngle,
		TrackTiltAngle);

		//if (oPathPoints[Q].AccSpd > V)
		oPathPoints[Q].AccSpd = MIN(V,oPathPoints[Q].Speed);
	}
  }
}
//==========================================================================*

//==========================================================================*
// Calculate curvature in XY
//--------------------------------------------------------------------------*
void TLane::CalcCurvaturesXY(int Step)
{
  CalcCurvaturesXY(0, Step);
}
//==========================================================================*

//==========================================================================*
// Calculate curvature in Z
//--------------------------------------------------------------------------*
void TLane::CalcCurvaturesZ(int Step)
{
  CalcCurvaturesZ( 0, Step);
}
//==========================================================================*

//==========================================================================*
// Calculate max possible speed
//--------------------------------------------------------------------------*
void TLane::CalcMaxSpeeds(int Step)
{
  CalcMaxSpeeds(0, oTrack->Count(), Step);
}
//==========================================================================*

//==========================================================================*
// Propagate breaking
//--------------------------------------------------------------------------*
void TLane::PropagateBreaking
  (int Step)
{
  PropagateBreaking( 0, oTrack->Count(), Step);
}
//==========================================================================*

//==========================================================================*
// Propagate breaking
//--------------------------------------------------------------------------*
void TLane::PropagatePitBreaking
  (int Start, float PitStopPos, float ScaleMu)
{
  PropagatePitBreaking( Start, oTrack->Count() - 20, PitStopPos, ScaleMu);
}
//==========================================================================*

//==========================================================================*
// Propagate acceleration
//--------------------------------------------------------------------------*
void TLane::PropagateAcceleration
  (int Step)
{
  PropagateAcceleration( 0, oTrack->Count(), Step);
}
//==========================================================================*

//==========================================================================*
// Calculate forward absolute curvature
//--------------------------------------------------------------------------*
void TLane::CalcFwdAbsCrv(int Range, int Step)
{
  const int	N = oTrack->Count() - 1;

  int Count = Range / Step;
  int P = Count * Step;
  int Q = P;
  double TotalCrv = 0;

  while (P > 0)
  {
	TotalCrv += oPathPoints[P].Crv;
	P -= Step;
  }

  oPathPoints[0].NextCrv = (float) (TotalCrv / Count);
  TotalCrv += fabs(oPathPoints[0].Crv);
  TotalCrv -= fabs(oPathPoints[Q].Crv);

  P = (N / Step) * Step;
  Q -= Step;
  if (Q < 0)
    Q = (N / Step) * Step;

  while (P > 0)
  {
	oPathPoints[P].NextCrv = (float) (TotalCrv / Count);
	TotalCrv += fabs(oPathPoints[P].Crv);
	TotalCrv -= fabs(oPathPoints[Q].Crv);

	P -= Step;
	Q -= Step;
	if (Q < 0)
	  Q = (N / Step) * Step;
  }
}
//==========================================================================*

//==========================================================================*
// Calculate estimated time
//--------------------------------------------------------------------------*
double TLane::CalcEstimatedTime(int Start, int Len) const
{
  double TotalTime = 0;

  const int N = oTrack->Count();
  for (int I = 0; I < Len; I++)
  {
	int	P = (Start + I) % N;
	int	Q = (P + 1) % N;
	double Dist = TUtils::VecLenXY(
	  oPathPoints[P].CalcPt() - oPathPoints[Q].CalcPt());

	TotalTime += Dist / ((oPathPoints[P].AccSpd + oPathPoints[Q].AccSpd) * 0.5);
  }

  return TotalTime;
}
//==========================================================================*

//==========================================================================*
// Calculate estimated lap time
//--------------------------------------------------------------------------*
double	TLane::CalcEstimatedLapTime() const
{
  double LapTime = 0;

  const int N = oTrack->Count();
  for (int I = 0; I < N; I++)
  {
	int	Q = (I + 1) % N;
	double Dist = TUtils::VecLenXY(
	  oPathPoints[I].CalcPt() - oPathPoints[Q].CalcPt());
	LapTime += Dist / ((oPathPoints[I].AccSpd + oPathPoints[Q].AccSpd) * 0.5);
  }

  return LapTime;
}
//==========================================================================*

//==========================================================================*
// Calculate Track Rollangle
//--------------------------------------------------------------------------*
double TLane::CalcTrackRollangle(double TrackPos)
{
  int P = oTrack->IndexFromPos(TrackPos);
  return atan2(oPathPoints[P].Norm().z, 1);
}
//==========================================================================*

//==========================================================================*
// Calculate Track Turnangle
//--------------------------------------------------------------------------*
double TLane::CalcTrackTurnangle(int P, int Q)
{
  double TotalCrv = 0;
  while (P < Q)
	TotalCrv += oPathPoints[P++].Crv;

  return fabs(TotalCrv);
}
//==========================================================================*

//--------------------------------------------------------------------------*
// end of file unitlane.cpp
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
