//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
// unitpit.cpp
//--------------------------------------------------------------------------*
// A robot for Speed Dreams-Version 2.X simuV4
//--------------------------------------------------------------------------*
// Pit and pitlane
// Box und Boxengasse
// 
// File         : unitpit.cpp
// Created      : 2007.02.20
// Last changed : 2014.11.29
// Copyright    : © 2007-2014 Wolf-Dieter Beelitz
// eMail        : wdbee@users.sourceforge.net
// Version      : 4.05.000
//--------------------------------------------------------------------------*
// Diese Unit basiert auf dem erweiterten Robot-Tutorial bt
//
//    Copyright: (C) 2002-2004 Bernhard Wymann
//    eMail    : berniw@bluewin.ch
//
// und dem Roboter delphin 2006
//
//    Copyright: (C) 2006-2007 Wolf-Dieter Beelitz
//    eMail    : wdbee@users.sourceforge.net
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
// WARNING:
//
// To calculate the racingline into and out of the pit we have to use the 
// data that is provided by TORCS/SD. These values are defined by the track 
// designers mostly having no knowledge about what we will get from the 
// system to understand, what they want us to understand from their ideas.
// There is no check, that the data of the track is consistent and usable.
// So we have to expect all issues that can happen, because they will happen.
// You don't belive? So look at the tracks and you will find things like pit 
// entries or exits being blocked by pitwalls, pit entry/exit 
// segments with a length below 3.2 m, pitlanes being so small, that we cannot
// avoid opponents, values that are changed after the 3D model was generated 
// resulting in showing wrong hints to the driver ...
//
// No, they do not want make our job harder as it is, they simply do not know 
// what they are doing and if you ask, you get answers like "but is is the 
// real track xyz and the pitlane in reality looks as it is defined here and 
// I want to make is as realistic as possible, so it is impossible to make the
// radius of the curve of the pitlane > 0.
//
// So we try to check everything and adjust the parameters to make the robot
// able to drive on all tracks without manual adjustments. Yes, you heard right, 
// on all tracks, even on tracks like corkscrew, longday or melvurn without 
// setup the track parameters to correct values.
//
// What did you say? All means all? All means even monandgo?
// But at monandgo the radius at pitlane is < 0 at some segments!
// All is all?
// Hmm ... OK, so drive into the pit entry, close the eyes for a moment 
// and the force will lead you to your pit. You don't want to drive with 
// closed eyes? But it is the easiest way to make it happen!
// (look for oCloseYourEyes in the code of TDriver::SteerAngle).
//
// BTW, if you understand this as a little bit of scorching criticism on the 
// ignorance of the track designers to real existing limiting conditions, I'm 
// very sorry, but this is exactly what I mean.
//
// In opposite to the simplix for racers, this robot is not designed to win a 
// championship. So let's accept, that the solution is not ideal for some 
// tracks, but it should work on all tracks known so far.
//
// The basic information and the assumptions needed to do our job are:
//
// The section from the start of the first pit to the end of the last is called 
// "the pitlane" here.
//
// PitInfo->driversPits->pos : center of the pit in m
//
// Distances:
// PitInfo->width            : width of the pit in m
// driversPits->pos.toMiddle : signed distance of the center of the pit to 
//                             the the middle of the track in m
//
// PitInfo->side             : side of pits
//
// 
// Assumptions:
// Track width is constant!
// Pitlane is a straight along the track!
// Width of the pit is > width of our and the other cars!
//
// The part of the code you could be mostly interested in is 
// void TPitLane::MakePath(...), here all the calculation is done.
//--------------------------------------------------------------------------*

#include "unitglobal.h"
#include "unitcommon.h"

#include "unitlinalg.h"
#include "unitpit.h"
#include "unitcubicspline.h"

//==========================================================================*
// [m/s] savety margin to avoid pit speeding.
//--------------------------------------------------------------------------*
const float TPit::SPEED_LIMIT_MARGIN = 0.5;
//==========================================================================*

//==========================================================================*
// Konstruktor
//--------------------------------------------------------------------------*
TPit::TPit(TDriver *Driver)
{
  oTrack = Driver->Track();
  oCar = Driver->Car();
  oMyPit = Driver->Car()->_pit;
  oPitInfo = &oTrack->pits;
  oPitStop = oInPitLane = false;
  oPitTimer = 0.0;

  if (oMyPit != NULL)
  {
	oSpeedLimit = oPitInfo->speedLimit - SPEED_LIMIT_MARGIN;
	oSpeedLimitSqr = oSpeedLimit*oSpeedLimit;
	oPitSpeedLimitSqr = oPitInfo->speedLimit*oPitInfo->speedLimit;
  }
  else
    LogSimplix.debug("\n\n\n SIMPLIX: NO PIT \n\n\n");

  for (int I = 0; I < gNBR_RL; I++)
    oPitLane[I].Init(Driver->Car());
}
//==========================================================================*

//==========================================================================*
// Destruktor
//--------------------------------------------------------------------------*
TPit::~TPit()
{
}
//==========================================================================*

//==========================================================================*
// Transforms track coordinates to spline parameter coordinates.
//--------------------------------------------------------------------------*
float TPit::ToSplineCoord(float X)
{
  X -= oPitEntry;
  while (X < 0.0f)
	X += oTrack->length;

  return X;
}
//==========================================================================*

//==========================================================================*
// Computes offset to track middle for trajectory.
//--------------------------------------------------------------------------*
float TPit::GetPitOffset(float Offset, float FromStart)
{
  if (oMyPit != NULL)
  {
	if (GetInPit() || (GetPitstop() && IsBetween(FromStart)))
	  FromStart = ToSplineCoord(FromStart);
  }
  return Offset;
}
//==========================================================================*

//==========================================================================*
// Sets the pitstop flag if we are not in the pit range.
//--------------------------------------------------------------------------*
void TPit::SetPitstop(bool PitStop)
{
  if (oMyPit == NULL)
	return;

  float FromStart = DistanceFromStartLine;

  if (!PitStop)
	this->oPitStop = PitStop;                 // Reset every time
  else if (!IsBetween(FromStart))
	this->oPitStop = PitStop;
  else if (!PitStop)
  {
	this->oPitStop = PitStop;
	oPitTimer = 0.0f;
  }
}
//==========================================================================*

//==========================================================================*
// Check if the argument fromstart is in the range of the pit.
//--------------------------------------------------------------------------*
bool TPit::IsBetween(float FromStart)
{
  if (oPitEntry <= oPitExit)
  {
	LogSimplix.debug("1. FromStart: %g\n",FromStart);
	if (FromStart >= oPitEntry && FromStart <= oPitExit)
  	  return true;
	else
	  return false;
  }
  else
  {
	// Warning: TORCS reports sometimes negative values for "fromstart"!
	LogSimplix.debug("2. FromStart: %g\n",FromStart);
	if (FromStart <= oPitExit || FromStart >= oPitEntry)
	  return true;
	else
	  return false;
  }
}
//==========================================================================*

//==========================================================================*
// Checks if we stay too long without getting captured by the pit.
// Distance is the distance to the pit along the track, when the pit is
// ahead it is > 0, if we overshoot the pit it is < 0.
//--------------------------------------------------------------------------*
bool TPit::IsTimeout(float Distance)
{
  if (CarSpeedLong > 1.0f || Distance > 3.0f || !GetPitstop())
  {
	oPitTimer = 0.0f;
	return false;
  }
  else
  {
	oPitTimer += (float) RCM_MAX_DT_ROBOTS;
	if (oPitTimer > 3.0f)
	{
	  oPitTimer = 0.0f;
	  return true;
	}
	else
	  return false;
  }
}
//==========================================================================*

//==========================================================================*
// Update pit data and strategy.
//--------------------------------------------------------------------------*
void TPit::Update()
{
  if (oMyPit != NULL)
  {
	if (IsBetween(DistanceFromStartLine))
	{
	  if (GetPitstop())
		SetInPit(true);
	}
	else
	  SetInPit(false);

	if (GetPitstop())
	  CarRaceCmd = RM_CMD_PIT_ASKED;

  }
}
//==========================================================================*

//==========================================================================*
// Get speed limit brake
//--------------------------------------------------------------------------*
float TPit::GetSpeedLimitBrake(float SpeedSqr)
{
  return (SpeedSqr-oSpeedLimitSqr)/(oPitSpeedLimitSqr-oSpeedLimitSqr);
}
//==========================================================================*

//==========================================================================*
// Make Path
//--------------------------------------------------------------------------*
void TPitLane::Init(PtCarElt Car)
{
  oCar = Car;
  oPitStopOffset = 0.0;
}
//==========================================================================*

//==========================================================================*
// Smooth Path with pitlane
//--------------------------------------------------------------------------*
void TPitLane::SmoothPitPath
  (const TParam& Param)
{
  int I;                                         // Loop counter

  int NSEG = oTrack->Count();                    // Number of sections in the path
  int Idx0 = oTrack->IndexFromPos(oPitEntryStartPos);
  int Idx1 = oTrack->IndexFromPos(oPitExitEndPos);  

  // Modify path for use of normal smoothing
  for (I = Idx0; I != Idx1; I = (I + 1) % NSEG)
  {
    oPathPoints[I].WToL = oPathPoints[I].WPitToL;
    oPathPoints[I].WToR = oPathPoints[I].WPitToR;
  }

  // Smooth pit path
  float BumpMode = (float) Param.oCarParam.oScaleBump;
  SmoothPath(Param,TClothoidLane::TOptions(1.0,1.016f,BumpMode));
}
//==========================================================================*

//==========================================================================*
// Make Path with pitlane
//--------------------------------------------------------------------------*
void TPitLane::MakePath
  (char* Filename, 
  TAbstractStrategy* Strategy,
  TClothoidLane* BasePath,
  const TParam& Param, int Index)
{
  // 1. Check whether we got a place to build our pit
  const tTrackOwnPit* Pit = CarPit;              // Get my pit
  if (Pit == NULL)                               // If pit is NULL
  {                                              // nothing to do
	LogSimplix.debug("\n\nPit = NULL\n\n");                 // here
	return;
  }

  // 2. We have a place, let us build all we need to use it ...
  const int NPOINTS = 9;                         // Nbr of points defined
  double X[NPOINTS];                             // X-coordinates
  double Y[NPOINTS];                             // Y-coordinates
  double S[NPOINTS];                             // Directions

  int I;                                         // Loop counter
  bool FirstPit = false;                         // Reset flag
  TCarParam CarParam = Param.oCarParam3;         // Copy parameters
  TLane::SetLane(*BasePath);                     // Copy Pathpoints
  const tTrackPitInfo* PitInfo =                 // Get pit infos
	&oTrack->Track()->pits;

  // At which side of the track is the place to build the pit?
  int Sign =                                     // Get the side of pits
	(PitInfo->side == TR_LFT) ? -1 : 1;

  // To be able to avoid in the pitlane we need three lanes.
  // To calculate the offsets we need to know the ranking
  float F[3] = {0.5, 1.0, 0.0};                  // Lane offsets
  if (Sign < 0)                                  // If pits are on the
  {                                              // left side
    F[1] = 0.0;                                  // swap the ranking
    F[2] = 1.0;                                  // of the lane offsets
  }

  // Different car types need different distances to stop, depending on their
  // braking parameters. The simplix allows a manual fine tuning using some
  // additional parameters.
  oStoppingDist = Param.Pit.oStoppingDist;       // Distance to brake
  oPitStopOffset = -Param.Pit.oLongOffset;       // Offset for fine tuning
  oCarParam.oScaleBrake =                        // Limit brake to be used
	MAX(0.10f,CarParam.oScaleBrake);             //   in pitlane
  oCarParam.oScaleMu =                           // Scale friction estimation
	MAX(0.10f,CarParam.oScaleMu);                //   of pitlane

  // To define the offset of the pitlane from the middle of the track
  // we use the distance of the center of the pit to middle of the track
  // Here we assume that the width of the pit and the width of the cars is 
  // in a relation, that we still have a safty margin between cars standing 
  // not perfectly adjusted in the pit while we want to drive through
  // the pitlane.
  double PitLaneOffset =                         // Offset of the pitlane 
	fabs(PitInfo->driversPits->pos.toMiddle)     //   from the middle of the
	- PitInfo->width;                            //   track

  // To get nearly same steering angles the distance to start to steer to the 
  // pit depends on the side of and the distance between the driven lanes in  
  // the pitlane. We use three lanes, a left, a center and a right lane.
  // Being alone we drive at the center lane. To avoid we can follow another.
  // If the distance of the driven lane to the pit is larger, we have to 
  // start to steer earlier. To calculate it, we use the factor Ratio.
  float Ratio = (float) (0.5 * 
	  PitInfo->len / (fabs(PitInfo->driversPits->pos.toMiddle) - PitLaneOffset));

  // Here we assume, that at a pit entry there is no barrier, means we expect 
  // that there is a drivable side (Side->style = TR_PLAN).
  // We will detect, if there is a pitwall between the track and the pitlane
  // and the length we can use to enter the pitlane will be limited by it.
  // If there is no pitwall we will stop at start of the speedlimit.
  // Same is done for the pit exit, just in the opposite direction.
  // BUT: If the track designer did not understand the definitions, we may find 
  // a pitwall at the pit entry segment as well (i.e. at melvurn)!
  // In this case we go backward and try to find the entry before.

  // Get possible length for pit entry
  tTrackSeg* Seg = PitInfo->pitEntry;
  tTrackSeg* Side;
  //tTrackSeg* NextSide;
  bool forward = true;
  double EntryLength = 0;                        // Usable length of pit entry

  if (Sign < 0)                                  // Get side segment
    Side = Seg->lside;
  else
    Side = Seg->rside;
	  
  if (Side != NULL)                              // If there is a side
  {                                              // Check driveability
	  if (Side->style != TR_PLAN)                // In case of a pitwall
		  forward = false;                       // we have to go backward 
  }

  if (forward)                                   // If we can use the side
    EntryLength += Seg->length;                  // add the length 

  do                                             // loop over some segments
  {
	  if (forward)                               // Get next segment depending 
	    Seg = Seg->next;                         // on the search direction
	  else
	    Seg = Seg->prev;

	  if (Sign < 0)                              // Get side segemnt
	    Side = Seg->lside;
	  else
	    Side = Seg->rside;

      if ((Side == NULL)                         // In case of not drivable
		|| (Side->raceInfo & (TR_SPEEDLIMIT | TR_PITLANE)) 
		|| (Side->style != TR_PLAN))             // or not allowed to use
		break;                                   // we got the end

	  if (forward)                               // Dpending on the search 
        EntryLength += Seg->length;              // direction we add the
	  else                                       // segments length
        EntryLength -= Seg->length;

	  if (EntryLength < -150)                    // In case of backward we 
		  break;                                 // assume we dont need more

  } while (Side != NULL);                        // In case of no side we stop 


  // Get possible length for pit exit
  Seg = PitInfo->pitExit;
  double ExitLength = 0;
  double NotUsableLength = 0;
  bool usable = false;
  bool backward = true; 
  double SearchLength = PitInfo->pitExit->lgfromstart 
	- PitInfo->pitEnd->lgfromstart;
  //LogSimplix.debug("ExitLength 0: %d %s %.3f %.3f %.3f\n",Seg->id,Seg->name,Seg->length,ExitLength,SearchLength);

  if (SearchLength < 0)
	SearchLength += oTrack->Length();

  //LogSimplix.debug("ExitLength 0: %d %s %.3f %.3f %.3f\n",Seg->id,Seg->name,Seg->length,ExitLength,SearchLength);

  do
  {
    if (Sign < 0)                                // Get side segment
      Side = Seg->lside;
    else
      Side = Seg->rside;
	  
    if (Side != NULL)                            // If there is a side
    {                                            // Check driveability
/*
		double AvailableWidth = Side->startWidth;
		if (Sign < 0)                            // Get next side segment
			NextSide = Side->lside;              // and add the width
		else                                     // if there is
			NextSide = Side->rside;              // additional side
		if (NextSide != NULL)                    // available
			AvailableWidth += NextSide->startWidth;

        LogSimplix.debug("Side 1: %d %d %.3f\n",Seg->id,Side->style,Side->endWidth);
	    if ((Side->style == TR_PLAN)             // In case of a barrier,
			&& (AvailableWidth > CarWidth))      // pitwall or too small 
*/
	    if (Side->style == TR_PLAN)              // In case of a barrier,
	  	  usable = true;                         // we have to go backward 
    }

    if (!usable)                                 // If we cannot use the side
      NotUsableLength += Seg->length;            // add the length 

	if (NotUsableLength > SearchLength - 1.0)
	{
        LogSimplix.debug("NotUsableLength 1: %.3f\n",NotUsableLength);
		backward = false;
		break;
	}

    if (!usable)                                 // If we cannot use the side
	{
      Seg = Seg->prev;
	  ExitLength += Seg->length;     
	  //LogSimplix.debug("ExitLength 1: %d %s %.3f %.3f\n",Seg->id,Seg->name,Seg->length,ExitLength);
	}

  } while (!usable);

  if (!usable)
  {
    Seg = PitInfo->pitExit;
	ExitLength = SearchLength;
    //LogSimplix.debug("ExitLength 2: %g\n",ExitLength);
	NotUsableLength = 0.0;
  }

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// WARNING: 
// In case the optimization is activated for release mode:
//--------------------------------------------------------
// If we do not use the variable backward here, it's value
// is wrong for next use in release mode while it is OK
// in debug mode. So there is an optimization used, that
// is not allowed here!
// To get more details look here 
// > http://support.microsoft.com/kb/925792/en-us
  if (backward)   
    LogSimplix.debug("backward\n");
  else
    LogSimplix.debug("foreward\n");
//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

  do
  {
	  if (backward)                              // Get next segment depending 
	    Seg = Seg->prev;                         // on the search direction
	  else
	    Seg = Seg->next;

	  if (Sign < 0)
	    Side = Seg->lside;
	  else
	    Side = Seg->rside;

      if ((Side == NULL) 
		|| (Side->raceInfo & (TR_SPEEDLIMIT | TR_PITLANE)) 
		|| (Side->style != TR_PLAN))
		break;

      ExitLength += Seg->length;               // add the length 
	  //LogSimplix.debug("ExitLength 3: %d %s %.3f %.3f (%d %d)\n",Seg->id,Seg->name,Seg->length,ExitLength,Side->style,Side->raceInfo);

  } while (Side != NULL);

  // The simplix robot knows two parameters to define the distance of the
  // driven lane from the basic pitlane.
  // We use the convention, that the incoming cars will use the outer side,
  // while the outgoing cars will use the inner one.
  // The default values are 3 m at entry and 5 m at exit.
  double LaneEntryOffset = Param.Pit.oLaneEntryOffset;
  double LaneExitOffset = Param.Pit.oLaneExitOffset;

  // To be able to drive at "modern" tracks, means tracks that do not respect
  // the limitations of a TORCS pitlane, we define 9 leading points (instead 
  // of 7 used by the classic robot tutorial written by Bernhard Wymann 
  // (Thanks for this great work!).
  // The Points are define by an X_ and an Y_ coordinate.
  // The X_ coordinates are measured as distance along the track from the 
  // startline. The Y_ coordinates are the local offsets from the middle of 
  // the track.
  // The first point is, where we leave the normal racningline, the last is
  // where we will return to it (_StartEntry/_EndExit). 
  // The next pair of points, the "new" points, are where we have to be at 
  // the safe side of a pitwall. The second point is at the end of the pit 
  // entry, the later at the end of a pitwall (_EndEntry/_StartExit)
  // If we do not use the first pit, the next point is at the start of the 
  // speedlimit, its counterpart at the end (_StartPitlane/_EndPitlane). 
  // We will follow the lane through the pitlane until we have to steer into 
  // the pit. The point to change the direction is calculated from the 
  // position of our pit. Same is done for the point, where we will reach the 
  // outgoing lane (_StartPit/_EndPit).
  // The point we want to stop at is the pit (_Pit).
  //
  // Remember, we defined names for the points X[i], Y[i], but in loops we 
  // use X[i], Y[i] instead.
  //
  // The pit racingline through these points is calculated using splines.
  //

  // Compute definitions for pit spline. We start with the X coordinates.
  if (forward)
  {
    X_StartEntry =                               // Start of entry 
	  PitInfo->pitEntry->lgfromstart             
	  + Param.Pit.oEntryLong;                    
    X_EndEntry =                                 // End of entry
	  PitInfo->pitEntry->lgfromstart             
	  + EntryLength
	  + Param.Pit.oEntryLong;                    
  }
  else
  {
    X_StartEntry =                               // Start of entry
	  PitInfo->pitEntry->lgfromstart             
	  + EntryLength
	  + Param.Pit.oEntryLong;                    
    X_EndEntry =                                 // End of entry
	  PitInfo->pitEntry->lgfromstart             
	  + Param.Pit.oEntryLong;                    
  }
  X_StartPitlane =                               // Start of speedlimit
	  PitInfo->pitStart->lgfromstart;            // mostly start of pitlane
  X_Pit =                                        // Center of our own pit
	  Pit->pos.seg->lgfromstart                  
	  + Pit->pos.toStart + oPitStopOffset;       
  X_StartPit =                                   // Start enter own pit here
	  X_Pit - PitInfo->len                       
	  - F[Index] * Ratio * LaneEntryOffset;
  X_EndPit =                                     // Leave own pit here
	  X_Pit + PitInfo->len                       
	  + F[Index] * Ratio * LaneExitOffset;
  X_EndPitlane =                                 // End of speed limit
	  X_StartPitlane + PitInfo->nPitSeg * PitInfo->len; 
  X_StartExit = 
	  PitInfo->pitExit->lgfromstart              // Start of pitlane exit
	  - ExitLength
	  + Param.Pit.oExitLong;
  X_EndExit =                                    // End of pitlane exit
	  PitInfo->pitExit->lgfromstart              
	  + PitInfo->pitExit->length 
	  - NotUsableLength
	  + Param.Pit.oExitLong;

  oPitEntryStartPos = X_StartEntry;              // Save as start of spline

  // In many cases the pits are defined along the start straight, so some 
  // of our points are located before the start line, some are after it.
  // To get correct distances, we have to correct the X_ values to be 
  // strictly growing.
  // Normalizing spline segments to X[I+1] >= X[I] even if startline is 
  // crossed between.
  for (I = 1; I < NPOINTS; I++)
  {
    X[I] = ToSplinePos(X[I]);
    S[I] = 0.0;
  }

  // Now the dark side of TORCS and SD ...
  // We discussed, that the normal order of the points is not correct if we
  // use the first or the last pit. For the first pit the point to start 
  // steering is before start of the speedlimit. At the end it is vice versa.
  // But as described earlier, there are tracks having senceless definitons, 
  // here we have to adjust our assumptions, means the X coordinates of the 
  // points have to be manipulated to get a usefull pit racingline.

  // Fix start of pitlane point for first pit if necessary.
  if (X_StartPit < X_StartPitlane)
  {
    FirstPit = true;                             // Hey we use the first pit!
    X_StartPitlane = X_StartPit - 1.0;           // Congratulation 
  }

  // Fix end of pitlane point for last pit if necessary.
  if (X_EndPitlane < X_EndPit)                   // This is not your race!
    X_EndPitlane = X_EndPit + 1.0;

  // Fix broken pit exit.
  if (X_EndExit < X_EndPitlane)
    X_EndExit = X_EndPitlane + 50.0;

  // In case we had to recalculate X coordinates, we have to make sure, 
  // that they are still strictly growing.
  for (I = 1; I < NPOINTS; I++)
  {
    X[I] = ToSplinePos(X[I]);
    S[I] = 0.0;
  }

  oPitEntryStartPos = X_StartEntry;              // Save this values for 
  oPitEntryEndPos   = X_EndEntry;                // later use by name
  oPitStartPos      = X_StartPitlane;            // and to restrict it     
  oPitEndPos        = X_EndPitlane;              // to real track 
  oPitExitStartPos  = X_StartExit;               // coordinates without
  oPitExitEndPos    = X_EndExit;                 // changing the X[i]s

  // For tuning the exit we need to know the distance 
  // from pit to end of pitlane
  Strategy->oPit->oDistToPitEnd = 
	oPitEndPos - X_Pit;

  // The growing X coordinates are contained in the X[i] array, so we can 
  // use the named positions to get the real track based distance from 
  // startline. 
  if (oPitEntryEndPos > oTrack->Length())        
	  oPitEntryEndPos -= oTrack->Length();
  if (oPitStartPos > oTrack->Length())           
	  oPitStartPos -= oTrack->Length();
  if (oPitEndPos > oTrack->Length())             
	  oPitEndPos -= oTrack->Length();
  if (oPitExitStartPos > oTrack->Length())       
	  oPitExitStartPos -= oTrack->Length();
  if (oPitExitEndPos > oTrack->Length())         
	  oPitExitEndPos -= oTrack->Length();

  // I'm sorry to say, but there are tracks, where the pitlane is too small,
  // to use the default values. Here we have to check our offsets.
  double AvailableWidth;
  double NeededWidth;
  int Idx0;
  int Idx1;

  // For the car we need a width including a safty marging at sides.
  // It is set to a large value, because of not driving perfectly at the
  // racingline and the thickness of a pitwall.
  float SafePassage = 4.0f + CarWidth;
  // We have to be on the outer side of a pitwall.
  NeededWidth = (SafePassage + oTrack->Width())/2.0; 
  // We assume that we could go up to the TORCS defined pitlane.
  AvailableWidth = MAX(PitLaneOffset,NeededWidth);

  // Now we compare the possible offset against our default parameters
  // If the offset would be impossible we define the available width at
  // entry and the needed at exit.
  if (LaneEntryOffset > fabs(PitLaneOffset - NeededWidth))
	LaneEntryOffset = 0;

  if (LaneExitOffset > fabs(PitLaneOffset - NeededWidth))
	LaneExitOffset = AvailableWidth - NeededWidth;

  // Splice entry/exit of pit path into the base path provided.
  // To avoid unwanted oscilliations, we have to go to a point 
  // where the direction of the racingline does not show to the 
  // opposite side!
  TLanePoint Point;                              // Data of the point
  double PreEntryStartPos = oPitEntryStartPos;
  do
  {
    PreEntryStartPos -= 1.0;                     // go back the racningline
    BasePath->GetLanePoint(PreEntryStartPos, Point); 
    S[0] = -tan(Point.Angle                      // Direction of our
	- oTrack->ForwardAngle(PreEntryStartPos));   //   basic racingline
  }                                              // Force direction to be
  while (fabs(S[0]) > 0.02f);                    // ~parallel to track!
  Y_StartEntry = Point.Offset;                   // Lateral distance

  // To avoid unwanted slow down of our car we have to go to a 
  // point, where the direction of the racing line is nearly parallel 
  // to the track
  double PostExitEndPos = oPitExitEndPos;
  do
  {
    PostExitEndPos += 1.0;
    BasePath->GetLanePoint(PostExitEndPos, Point);  
    S[8] = -tan(Point.Angle                      // Direction of the
	  - oTrack->ForwardAngle(PostExitEndPos));   // racingline at the end
  }                                              // Force direction to be
  while (fabs(S[8]) > 0.02f);                    // ~parallel to track!
    Y_EndExit = Point.Offset;                    // Lateral distance and


  // If we have the first pit, we can often use a better way into the pit 
  // driving directly to the pit
  if (Param.Pit.oUseFirstPit && FirstPit)        // If allowed and possible
  {                                              // we will use a special 
	Y_Pit = Y_StartPit = Y_StartPitlane =        // path to the
	  Sign *                                     // first pit with
	  (fabs(PitInfo->driversPits->pos.toMiddle   // TORCS/SD defined offset
	  + Param.Pit.oLatOffset));                  // and an own lateral offset
  }
  else                                           // All other pits
  {                                              // have to be reached over
    Y_StartPit = Y_StartPitlane =                // a path defined here
	  Sign * (PitLaneOffset -                    // Sign gives the side 
	  LaneEntryOffset * F[Index]);               // of the pits   

	Y_Pit = Sign *                               // Set the pit offset
	  (fabs(PitInfo->driversPits->pos.toMiddle   // compensate steering 
	  + 0.1 + Param.Pit.oLatOffset));            // LatOffset > 0 -> outer
  }

  Y_EndPitlane = Y_EndPit = Sign *               // Leaving the own pit, we 
    (PitLaneOffset                               // will go to the pitlane 
    - LaneExitOffset * F[Index]);                // and use additional offset

  // Pit entry ...
  int NSEG = oTrack->Count();                    // Nbr of sections in path
  Idx0 = (oTrack->IndexFromPos(oPitEntryEndPos) + NSEG - 1) % NSEG;
  NeededWidth = (SafePassage + oTrack->Width())/2.0; 
  if (Sign < 0) 
  {
	AvailableWidth = MAX(oPathPoints[Idx0].WPitToL,oPathPoints[Idx0].WToL) 
	  - CarWidth/2.0 - 1.0;
    Y_EndEntry = -MAX((AvailableWidth + NeededWidth) / 2.0,NeededWidth);  
  }
  else 
  {
	AvailableWidth = MAX(oPathPoints[Idx0].WPitToR,oPathPoints[Idx0].WToR) 
	  - CarWidth/2.0 - 1.0;
    Y_EndEntry = MAX((AvailableWidth + NeededWidth) / 2.0,NeededWidth);  
  }

  // In case of a small distance between the points, we set them to the 
  // same offset to avoid unwanted curves.
  if (X_StartPitlane - X_EndEntry < 10.0)
	  Y_EndEntry = Y_StartPitlane;

  // Pit exit ...
  Idx1 = (oTrack->IndexFromPos(oPitExitStartPos) + 1) % NSEG;;
  if (Sign < 0) 
  {
	AvailableWidth = MAX(oPathPoints[Idx1].WPitToL,oPathPoints[Idx1].WToL)
	  - CarWidth/2.0 - 1.0;
	Y_StartExit = -MAX((AvailableWidth + 3*NeededWidth)/4.0,NeededWidth);    
  }
  else
  {
	AvailableWidth = MAX(oPathPoints[Idx1].WPitToR,oPathPoints[Idx1].WToR)
	  - CarWidth/2.0 - 1.0;
	Y_StartExit = MAX((AvailableWidth + 3*NeededWidth)/4.0,NeededWidth);    
  }	

  // In case of a small distance between the points, we set them to the 
  // same offset to avoid unwanted curves.
  if (X_StartExit - X_EndPitlane < 10.0)
	  Y_StartExit = Y_EndPitlane;
/*
  for (int I = 0; I < 9; I++)
  {
	  LogSimplix.debug("X[%d]: %g Y[%d]: %g S[%d]: %g\n",I,X[I],I,Y[I],I,S[I]);
  }
*/
  // Calculate the splines for entry and exit of pitlane
  TCubicSpline PreSpline(NPOINTS, X, Y, S);      

  // Now we come to the more dirty details.

  // In case of smoothing we start at section with speedlimit
  //Idx0 = oTrack->IndexFromPos(oPitStartPos);     // Index to first point
  //Idx1 = oTrack->IndexFromPos(oPitEndPos);       // Index to last point
  // In case we want to use smoothing later, these points have to keep
  //for (I = Idx0; I != Idx1; I = (I + 1) % NSEG)  // Set Flag, to keep 
  //  oPathPoints[I].Fix = true;                   //   while smooting

  // Section before pit entry
  Idx0 = (1 + oTrack->IndexFromPos(PreEntryStartPos)) % NSEG;
  Idx1 = (1 + oTrack->IndexFromPos(oPitEntryStartPos)) % NSEG;;
  for (I = Idx0; I != Idx1; I = (I + 1) % NSEG)
  {
	oPathPoints[I].Offset = (float) Y_StartEntry;// Offset lateral to track
    oPathPoints[I].Point =                       // Recalculate points
	  oPathPoints[I].CalcPt();                   // from offset
  }

  // Pit entry and pit exit as calculated
  Idx0 = (1 + oTrack->IndexFromPos(oPitEntryStartPos)) % NSEG;
  Idx1 = (1 + oTrack->IndexFromPos(PostExitEndPos)) % NSEG;;

  // Change offsets to go to the pitlane based on the splines
  NeededWidth = (SafePassage + oTrack->Width())/2.0; 
  for (I = Idx0; I != Idx1; I = (I + 1) % NSEG)
  {
    double SplineY;                              // Offset lateral to track
    double TrackX =                              // Station in track coords
	  oTrack->Section(I).DistFromStart;
    double SplineX = ToSplinePos(TrackX);        // Station in spline coords
 
	if (SplineX >= X_EndExit)
	  SplineY = Y_EndExit;                       // After/At the real exit
	else
	{
 	  // Calculate offset to side depending on pit side
      // For all points along our pitlane racingline we have to check, 
      // whether the designer did a good job or did restrict the usable 
	  // offset too much.
      if (Sign < 0) 
	  {
	    AvailableWidth = MAX(oPathPoints[I].WPitToL,oPathPoints[I].WToL) 
			- CarWidth/2.0 - 0.01;
        SplineY = MAX(PreSpline.CalcOffset(SplineX),
			MIN(-AvailableWidth,-NeededWidth));
	  }
      else
	  {
	    AvailableWidth = MAX(oPathPoints[I].WPitToR,oPathPoints[I].WToR) 
			- CarWidth/2.0 - 0.01;
        SplineY = MIN(PreSpline.CalcOffset(SplineX),
			MAX(AvailableWidth,NeededWidth));
	  }
	}

	oPathPoints[I].Offset = (float) SplineY;    // Offset lateral to track
    oPathPoints[I].Point =                       // Recalculate points
	  oPathPoints[I].CalcPt();                   // from offset
	//LogSimplix.debug("Spline: %g: %g/%g %g\n",TrackX,SplineX,oPathPoints[I].Offset,SplineY);
  }

  // Prepare speed calculation with changed path (Simplix specific)
  int FwdRange = 110;
  CalcFwdAbsCrv(FwdRange);
  CalcCurvaturesXY();
  CalcCurvaturesZ();
  CalcMaxSpeeds(1);

  // Overwrite speed calculations at section with speed limit
  // Here we assume that it is a straight!
  Idx0 = (oTrack->IndexFromPos(oPitStartPos) + NSEG - 5) % NSEG;
  Idx1 = (oTrack->IndexFromPos(oPitEndPos) + 1) % NSEG;

  // Allowed speed in pitlane itself
  for (I = Idx0; I != Idx1; I = (I + 1) % NSEG)
  {
    double Speed = MIN(oPathPoints[I].Speed, PitInfo->speedLimit - 0.75);
    oPathPoints[I].MaxSpeed = oPathPoints[I].Speed = Speed;
  }

  // Save stop position
  double StopPos = Pit->pos.seg->lgfromstart     // As defined by TROCS/SD
	+ Pit->pos.toStart + oPitStopOffset;         // with offset along track

  // Normalize it to be 0 >= StopPos > track length
  if (StopPos >= oTrack->Length())               
    StopPos -= oTrack->Length();
  else if (StopPos < 0)
    StopPos += oTrack->Length();

  // Section at pit stop
  oStopIdx = Idx0 = oTrack->IndexFromPos(StopPos);
/*
  // Set speed to restart faster or at Aalborg
  for (I = 0; I < 15; I++)
  {
    Idx0 = (Idx0 + 1) % NSEG;
    oPathPoints[Idx0].MaxSpeed = 
	  oPathPoints[Idx0].Speed = 
	    PitInfo->speedLimit - 0.5;
  }
*/
  // This is where we stop in track coordinates
  oPitStopPos = oPathPoints[oStopIdx].Dist();

  // Set target speed at stop position
  oPathPoints[oStopIdx].MaxSpeed = oPathPoints[oStopIdx].Speed = 0.01;

  // Calculate braking
  PropagatePitBreaking(oStopIdx, (tdble) oPitStopPos,(tdble) Param.oCarParam.oScaleMu);

  // Look for point to decide to go to pit
  Idx0 = oTrack->IndexFromPos(oPitEntryStartPos);
  double DeltaSpeed;
  int Steps = 0;
  do
  {
    Idx0 = (Idx0 + NSEG - 1) % NSEG;
	double TrackSpeed =
	  MIN(BasePath->oPathPoints[Idx0].Speed,
	  BasePath->oPathPoints[Idx0].AccSpd);
    DeltaSpeed = TrackSpeed - oPathPoints[Idx0].Speed;
  } while ((DeltaSpeed > 1.0) && (++Steps < NSEG));

  // Distance before pit to decide for pitstop
  oPitDist = oPitStopPos - oPathPoints[Idx0].Dist();
  if (oPitDist < 0)
    oPitDist += oTrack->Length();

}
//==========================================================================*

//==========================================================================*
// Are we in Pit Section?
//--------------------------------------------------------------------------*
bool TPitLane::InPitSection(double TrackPos) const
{
  TrackPos = ToSplinePos(TrackPos);
  return oPitEntryStartPos < TrackPos 
	  && TrackPos < ToSplinePos(oPitExitEndPos);
}
//==========================================================================*

//==========================================================================*
// Check wether we can stop in pit
//--------------------------------------------------------------------------*
bool TPitLane::CanStop(double TrackPos) const
{
  double D = DistToPitStop(TrackPos, true);
  if ((D < oStoppingDist) || (oTrack->Length() - D < oStoppingDist))
    return true;
  else
    return false;
}
//==========================================================================*

//==========================================================================*
// Check wether we overrun stop pos
//--------------------------------------------------------------------------*
bool TPitLane::Overrun(double TrackPos) const
{
  double D = DistToPitStop(TrackPos, true);
  if ((D > oTrack->Length() / 2) && (oTrack->Length() - D > oStoppingDist))
	return true;
  else
    return false;
}
//==========================================================================*

//==========================================================================*
// Get Distance to Pit entry
//--------------------------------------------------------------------------*
double TPitLane::DistToPitEntry(double TrackPos) const
{
  double Dist = oPitEntryStartPos - TrackPos;
  if (Dist < 0)
	Dist += oTrack->Length();
  return Dist;
}
//==========================================================================*

//==========================================================================*
// Get Distance to Pit entry
//--------------------------------------------------------------------------*
double TPitLane::DistToPitStop(double TrackPos, bool Pitting) const
{
  double Dist;
  float DL, DW;

  if (Pitting)
  {
//	  dist = oPitStopPos - trackPos;
    RtDistToPit(oCar,oTrack->Track(),&DL,&DW);
    DL += (float)(oPitStopOffset - TRACKRES / 2);
//	  LogSimplix.debug("DistToPitStop: %g-%g=%g\n",dist,DL,dist-DL);
    Dist = DL;
  	if (Dist < 0)
	  Dist += oTrack->Length();
  }
  else
  {
	Dist = oPitStopPos - oPitEntryStartPos;
	if (Dist < 0)
	  Dist += oTrack->Length();
	  Dist += DistToPitEntry(TrackPos);
  }
  return Dist;
}
//==========================================================================*

//==========================================================================*
// Calculate local position
//--------------------------------------------------------------------------*
double TPitLane::ToSplinePos(double TrackPos) const
{
  if (TrackPos < oPitEntryStartPos)
	TrackPos += oTrack->Length();
  return TrackPos;
}
//--------------------------------------------------------------------------*
// end of file unitpit.cpp
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
