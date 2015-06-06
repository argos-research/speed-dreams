//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
// unittrack.cpp
//--------------------------------------------------------------------------*
// TORCS: "The Open Racing Car Simulator"
// A robot for Speed Dreams-Version 1.4.0/2.X
//--------------------------------------------------------------------------*
// Track description with variable step length
// Streckenbeschreibung mit variabler Abtastrate
// und segmentgenauen Abschnittsgrenzen
// 
// File         : unittrack.cpp 
// Created      : 2007.11.17
// Last changed : 2013.06.25
// Copyright    : © 2007-2011 Wolf-Dieter Beelitz
// eMail        : wdb@wdbee.de
// Version      : 4.00.002
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
//    eMail    : wdb@wdbee.de
//
// dem Roboter wdbee_2007
//
//    Copyright: (C) 2006-2007 Wolf-Dieter Beelitz
//    eMail    : wdb@wdbee.de
//
// dem Roboter mouse_2006
//    Copyright: (C) 2006 Tim Foden
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
// Im Übrigen gilt für die Nutzung und/oder Weitergabe die
// GNU GPL (General Public License)
// Version 2 oder nach eigener Wahl eine spätere Version.
//--------------------------------------------------------------------------*

#include <robottools.h>

#include "unitglobal.h"
#include "unitcommon.h"

#include "unittrack.h"
#include "unitdriver.h"

//==========================================================================*
// Constructor
//--------------------------------------------------------------------------*
TTrackDescription::TTrackDescription(): oSections(NULL), oTrack(NULL)
{
  oTrackRes = TRACKRES;                          // sampling rate
  oMeanSectionLen = oTrackRes;                   // First estimation
}
//==========================================================================*

//==========================================================================*
// Destructor
//--------------------------------------------------------------------------*
TTrackDescription::~TTrackDescription()
{
  delete [] oSections;                           // Free sections
}
//==========================================================================*

//==========================================================================*
// Calc position from offset
//--------------------------------------------------------------------------*
double TTrackDescription::CalcPos(tTrkLocPos& TrkPos, double Offset) const
{
  double Pos = RtGetDistFromStart2(&TrkPos) + Offset;
  return NormalizePos(Pos);                      // Normalize to >= 0.0
}
//==========================================================================*

//==========================================================================*
// Calc cars position from offset
//--------------------------------------------------------------------------*
double TTrackDescription::CalcPos(tCarElt* Car, double Offset) const
{
  double Pos = RtGetDistFromStart(Car) + Offset;
  return NormalizePos(Pos);                      // Normalize to >= 0.0
}
//==========================================================================*

//==========================================================================*
// Calc position from coordinates
//--------------------------------------------------------------------------*
double TTrackDescription::CalcPos
  (float X, float Y, const TSection* Hint, bool Sides) const
{
  tTrackSeg* Seg = oSections[0].Seg;
  if(Hint != 0)
    Seg = Hint->Seg;

  tTrkLocPos Pos;
  RtTrackGlobal2Local(Seg, X, Y, &Pos, Sides );
  return RtGetDistFromStart2(&Pos);
}
//==========================================================================*

//==========================================================================*
// Number of sections in track
//--------------------------------------------------------------------------*
int TTrackDescription::Count() const
{
  return oCount;
}
//==========================================================================*

//==========================================================================*
// Create track description
// using different track resolutions along pits
// and setting sections according to segment limmits
//--------------------------------------------------------------------------*
void TTrackDescription::Execute()
{
  int I;                                         // Looping index of sections
  int ID = 0;                                    // ID of section
  tTrackSeg* First;                              // First Segment;
  tTrackSeg* Segm;                               // Current Segment;
  float Station;                                 // Position along track
  TSection* Section;                             // Section

  int NSec;                                      // Number of sections
  float StepLen;                                 // Length of current step
  bool PitSection = false;                       // Start in pit section

  oPitEntry = -1;                                // Undefined pit entry
  oPitExit  = -1;                                //   and pit exit
  oPitSide  = oTrack->pits.side ==               // Get side of pit
	TR_LFT ? TR_SIDE_LFT : TR_SIDE_RGT;

  First = oTrack->seg;                           // First TORCS segment

  // But find segment with smallest destination to startline ...
  while (First->lgfromstart > oTrack->length / 2)//
	First = First->next;
  // ... First is new start segment for our track description!

  // Find oder of pit entry/exit ...
  Segm = First;                                  // Start near startline
  do
  {                                              // loop all segments
    if(Segm->raceInfo & TR_PITENTRY)             // if pit entry first
	  break;                                     //   startline is out
	else if(Segm->raceInfo & TR_PITEXIT)         // if pit exit is first
	{                                            //   startline is in
      PitSection = true;                         //   pit section! Start
	  break;                                     //   with pit mode!
	}
	Segm = Segm->next;                           // Next segment
  }
  while (Segm != First);                         // Loop till restart

  // ... find number of sections needed ...
  Segm = First;                                  // Start near startline
  do
  {                                              // loop all segments
    if((oPitEntry < 0)                           // If pit entry is found
	  && (Segm->raceInfo & TR_PITENTRY))         //   first time
	{                                            // save ID of section
      oPitEntry = ID;                            // and change mode of
	  PitSection = true;                         // calculating steplength
	}
    else if(Segm->raceInfo & TR_PITEXIT)         // If pit exit is found
	{                                            // we leave pit sections
      oPitExit  = ID;                            // save ID of section
	  PitSection = false;                        // reset steplength mode
	}

    ID += NbrOfSections                          // Get number of sections
	  (Segm->length,PitSection);                 // to use for this segment
    Segm = Segm->next;                           // Next torcs segment
  }
  while (Segm != First);                         // Loop till restart

  oCount = ID;                                   // Number of sections in track

  // ... estimate length of sections ...
  oMeanSectionLen = oTrack->length / oCount;     // Mean length of sections

  // .. create track description ...
  oSections = new TSection[oCount];              // Create all sections of track
  ID = 0;                                        // Start with ID 0
  oPitEntry = -1;                                // Reset Markers
  oPitExit  = -1;

  Segm = First;                                  // Start near startline
  double segDist = Segm->lgfromstart;            // Distance from start
  do
  {                                              // Loop all segments
    Station = 0;                                 // Local position

    if((oPitEntry < 0)                           // If found pit entry
	  && (Segm->raceInfo & TR_PITENTRY))         // we enter the pit sections
	{
      oPitEntry = ID;
	  PitSection = true;
	}
    else if(Segm->raceInfo & TR_PITEXIT)         // If found pit exit
	{                                            // we leave pit sections
      oPitExit  = ID;
	  PitSection = false;
	}

    NSec = NbrOfSections                         // Number of sections to
      (Segm->length,PitSection);                 // use for this segment

	if (Segm->type == TR_STR)
    {
      // Segment is straight ...
      StepLen = Segm->length / NSec;             // Steplength

      for (I = 0; I < NSec; I++)                 // Loop sections
	  {
        Section = &oSections[ID];                // next undefined section

		Section->Station = Station;              // Save local position
		Section->DistFromStart = segDist;        // Save global distance
		Section->Seg = Segm;                     // Derived from segment
		Section->WidthToLeft = Segm->width / 2;  // Save initial width to
		Section->WidthToRight = Segm->width / 2; //   left and right side
		Section->Friction = Segm->surface->kFriction;

		segDist += StepLen;                      // calculate dist from start
        Station = Station + StepLen;             // and local position
        ID++;                                    // Increment ID
      }
    }
    else
    {
	  // Segment is curve ...
      StepLen = Segm->length / NSec;             // Step length

      for (I = 0; I < NSec; I++)                 // Loop all sections
      {
        Section = &oSections[ID];                // Next undefined section

		Section->Station = Station;              // Save local position
		Section->DistFromStart = segDist;        // Save global distance
		Section->Seg = Segm;                     // Derived from segment
		Section->WidthToLeft = Segm->width / 2;  // Save initial width to
		Section->WidthToRight = Segm->width / 2; //   left and right side
		Section->Friction = Segm->surface->kFriction;

		segDist += StepLen;                      // calculate dist from start
        Station = Station + StepLen;             // and local position
        ID++;                                    // Increment ID
      }
    }
	Segm = Segm->next;                           // Next segment
    segDist = Segm->lgfromstart;                 // Distance from Start
  }
  while (Segm != First);                         // Loop all segments

  BuildPos2SecIndex();                           // Build reverse index
}
//==========================================================================*

//==========================================================================*
// Initial target speed[Index]
//--------------------------------------------------------------------------*
void TTrackDescription::InitialTargetSpeed(int Index, double TargetSpeed)
{
  oSections[Index].InitialTargetSpeed = TargetSpeed;
}
//==========================================================================*

//==========================================================================*
// Initial target speed[Index]
//--------------------------------------------------------------------------*
double TTrackDescription::InitialTargetSpeed(int Index)
{
  return oSections[Index].InitialTargetSpeed;
}
//==========================================================================*

//==========================================================================*
// Friction of section[Index]
//--------------------------------------------------------------------------*
double TTrackDescription::Friction(int Index) const
{
  return oSections[Index].Friction;
}
//==========================================================================*

//==========================================================================*
// Learn Friction of section[Index]
//--------------------------------------------------------------------------*
double TTrackDescription::LearnFriction
  (int Index, double Delta, double MinFriction)
{
  if (Delta > 0) 
  {
    for (int I = MIN(Index + 2,oCount - 1); (I > 0) && (I > Index - 3); I--)
	{
      oSections[I].Friction -= Delta;
      oSections[I].Friction =
	    MAX(oSections[I].Seg->surface->kFriction * MinFriction,oSections[I].Friction);
	}
  }
  else
  {
    oSections[Index].Friction -= Delta / 2;
    oSections[Index].Friction =
	  MIN(oSections[Index].Seg->surface->kFriction * 1.02,oSections[Index].Friction);
  }
  return oSections[Index].Friction;
}
//==========================================================================*

//==========================================================================*
// Angle
//--------------------------------------------------------------------------*
double TTrackDescription::ForwardAngle(double TrackPos) const
{
  int Index = IndexFromPos(TrackPos);
  const tTrackSeg* Seg = oSections[Index].Seg;

  double X;
  TVec3d CenterPoint;
  TVec3d Normale;
  NormalizeDir(Seg, TrackPos - Seg->lgfromstart, X, CenterPoint, Normale);

  return TUtils::VecAngXY(Normale) + PI / 2;
}
//==========================================================================*

//==========================================================================*
// Find index of section to position
//--------------------------------------------------------------------------*
int TTrackDescription::IndexFromPos(double TrackPos) const
{
  TrackPos = NormalizePos(TrackPos);             // Normalize to >= 0.0

  int Index = int(                               // Estimate index from
	floor(TrackPos/oMeanSectionLen)) % oCount;   // distance to startline
  Index = oSections[Index].PosIndex;             // Use lookup table

  // Interpolate back from estimation
  while (TrackPos < oSections[Index].DistFromStart)
  {
	if (Index > 0)
      Index--;
	else
      return 0;
  }

  // Interpolate from estimation
  while (TrackPos > oSections[Index+1].DistFromStart)
  {
    if (Index < oCount - 2)
      Index++;
	else
      return oCount - 1;
  }

  return Index;
}
//==========================================================================*

//==========================================================================*
// Build position to section index to estimate search start index
//--------------------------------------------------------------------------*
void TTrackDescription::BuildPos2SecIndex()
{
  for (int I = 0; I < oCount; I++)
  {
    double TrackPos = I * oMeanSectionLen + 0.1;
    int Index = int(floor(TrackPos / oMeanSectionLen)) % oCount;
    while (TrackPos < oSections[Index].DistFromStart)
    {
	  if (Index > 0)
        Index--;
	  else
        break;
	}

    while (TrackPos > oSections[Index + 1].DistFromStart)
 	{
      if (Index < oCount - 1)
        Index++;
	  else
        break;
	}
    oSections[I].PosIndex = Index;
  }
}
//==========================================================================*

//==========================================================================*
// Build new track description
//--------------------------------------------------------------------------*
void TTrackDescription::InitTrack
  (tTrack* Track, TCarParam& CarParam, PitSideMod* PitSideMod)
{
  if (oTrack != Track)                           // if used
  {                                              //   free old ones
    if (oSections != NULL)
	  delete [] oSections;
	oSections = NULL;
	oCount = 0;
  }

  oTrack = Track;                                // Save pointer to TORCS data

  if (PitSideMod)                                // If defined
	oPitSideMod = *PitSideMod;                   //   Set Pit side mode

  Execute();                                     // Make description

  tTrackSeg* LastSeg = oTrack->seg;              // Save last segment
  //int LastSegType = TR_STR;  // Set but bot used (see below)    // Assume straight
  tTrackSeg* Seg = LastSeg;                      // Start at last segment

  // Find usable additional width at sides of the track ...
  int N = 0;                                     // Count sections since
  {for (int I = 0; I < oCount; I++)               //   last curve
  {
	oSections[I].PitWidthToLeft = oSections[I].WidthToLeft;
	oSections[I].PitWidthToRight = oSections[I].WidthToRight;

	LastSeg = Seg;                               // Save last segment
    if (LastSeg->type != TR_STR)                 // if it was a curve
	{                                            // save last type and
	  N = 0;
	}
	else                                         // increase counter
      N++;                                       //   on straights

    Seg = oSections[I].Seg;                      // Get torcs segment

	double DistFromStart =                       // Distance from start
      oSections[I].DistFromStart;                //   of section
	double T = (DistFromStart - Seg->lgfromstart)// Part of section
	  / Seg->length;

	bool InPit = (((oPitEntry < oPitExit)         // Check if along pits
	  && (oPitEntry <= I)
	  && (I <= oPitExit))
	  || ((oPitEntry > oPitExit)
	  && ((I <= oPitExit) || (I >= oPitEntry))));

	// Selections ...
	double MIN_MU =                              // min usable mu
	  Seg->surface->kFriction * CarParam.oScaleMinMu;

	const double MAX_ROUGH =                     // max usable rough
//      MAX(0.005, Seg->surface->kRoughness * 1.2);
      MAX(0.025, Seg->surface->kRoughness * 1.2);
//      MAX(0.100, Seg->surface->kRoughness * 1.5);
	const double MAX_RESIST =                    // max usable resistance
//	  MAX(0.02, Seg->surface->kRollRes * 1.2);
	  MAX(0.05, Seg->surface->kRollRes * 1.2);
//	  MAX(0.10, Seg->surface->kRollRes * 1.5);
	const double SLOPE = Seg->Kzw;               // Slope of segment

	for (int S = 0; S < 2; S++)                  // Look at both sides
	{
	  tTrackSeg* PSide = Seg->side[S];           // Side-segment
	  if (PSide == NULL)                         // If NULL no side
		continue;                                //   go to next segment

	  double ExtraW = 0;                         // Initialize add. width
	  double ExtraWpit = 0;                      // Initialize add. width
	  bool Done = false;                         // Reset flag
	  bool PitOnly = false;                      // Reset flag
	  while(PSide)                               // Loop all side-segments
	  {
	    double Wpit = 0.0;                       // Initialize
		double WCurb = 0.0;                      // additional with
		double W = PSide->startWidth + T *       // Estimate width of section
		  (PSide->endWidth - PSide->startWidth); // at current position
		float slope = PSide->height/PSide->width;// Slope of border
		bool outer = ((S == TR_SIDE_LFT)         // Is it the outer side?
		    && (Seg->type == TR_RGT))       
		    || ((S == TR_SIDE_RGT)          
		    && (Seg->type == TR_LFT));      
		bool pitlane = (((S == oPitSideMod.side) // If side of pits
			&& (I >= oPitSideMod.start)          // and between start
			&& (I <= oPitSideMod.end)));         // and end of pitlane

		if (pitlane)                             // if side along pitlane
	      W = 0.0;                               // Keep out!

		if ((PSide->style == TR_CURB)            // On curbs with height
		  && (slope > 0.01))                     // and great slope
		{
		  Done = true;                           // Last possible to use 
          WCurb = 0.8 * W;                       // Use 80%
          //WCurb =  W;                            // Use 100%
		  if (outer)                             // If outer side
            WCurb = MIN(WCurb, 1.5);             // Keep a wheel on track
		  else
            WCurb = MIN(WCurb, 1.5);             // Keep a wheel on track

		  if (outer                              // If outer side and friction lower
			&& (PSide->surface->kFriction < Seg->surface->kFriction))
	        WCurb = MIN(WCurb, 0.15);            // use 15 cm only

		  // Don't go too far up raised curbs
		  if (slope > 0.15)                      // If more
		    WCurb = 0;                           //   keep off
          else                                   // less slope
            WCurb = MIN(WCurb,3.0/slope);        // more width used

		}
	    else if (PSide->style == TR_CURB)        // On curbs without height
		{
          WCurb = 0.8 * W;                       // Use 80%
          //WCurb = W;                             // Use 100%
		  if (pitlane)                           // if side along pitlane
		  {
            WCurb = 0.15;                         
		    //Done = true;
		  }
		  else if (outer && (PSide->surface->kFriction < Seg->surface->kFriction))
		  {
		    //WCurb = MIN(WCurb,1.5);              // Keep two wheels on track
		    //Done = true;
		  }
		  if (outer)                             // If outer side
  	        WCurb = MIN(WCurb,1.5);              // Keep two wheels on track
		  else
  	        WCurb = MIN(WCurb,1.5);              // Keep two wheels on track
	      Done = true;
		}
		else if (PSide->style == TR_PLAN)        // On plan
		{
          WCurb = 0.8 * W;                       // Use 80%
          //WCurb =  W;                            // Use 100%
		  if ((InPit && (oPitSide == S))         // Exclude pits
			|| (PSide->raceInfo & (TR_SPEEDLIMIT | TR_PITLANE)))
		  {
            Wpit = W;                            // Save it for pitlane
            WCurb = 0;                           // Use 80%
		    Done = true;
	      }
          else if (pitlane)
		  {
            Wpit = W;                            // Save it for pitlane
			if (W > 0.15)                        // Only 15 cm
			{
			  WCurb = 0.15;
			  Done = true;
			}
		  }
		  // Selections ...
		  else if ((PSide->surface->kFriction < MIN_MU)
			|| (PSide->surface->kRoughness > MAX_ROUGH)
			|| (PSide->surface->kRollRes > MAX_RESIST)
			|| (fabs(PSide->Kzw - SLOPE) > 0.005))
		  {
            WCurb = 0;                           
			Done = true;
		  }
		  else if (outer
			  && ((PSide->surface->kFriction <  MIN_MU /* Seg->surface->kFriction */ )
			  || (PSide->surface->kRoughness > MAX_ROUGH)
			  || (PSide->surface->kRollRes > MAX_RESIST)
			  || (fabs(PSide->Kzw - SLOPE) > 0.005)))
		  {
            WCurb = 0;                           
			Done = true;
		  }
		  else if (PSide->surface->kFriction < Seg->surface->kFriction)
		  {
			if (CarParam.oLimitSideUse)
			{
		      if (outer)                         // If outer side
			  {
			    WCurb = MIN(WCurb,CarParam.oLimitSideWidth);
			    W = MIN(W,CarParam.oLimitSideWidth);
			    ExtraW = MIN(ExtraW,CarParam.oLimitSideWidth);
			  }
			  else
			  {
			    WCurb = MIN(WCurb,CarParam.oLimitSideWidth/2);
			    W = MIN(W,CarParam.oLimitSideWidth);
			    ExtraW = MIN(ExtraW,CarParam.oLimitSideWidth/2);
			  }
			  Done = true;
			}
		  }
		  else if (CarParam.oLimitSideUse)
		  {
		    if (outer)                         // If outer side
		    {
			  WCurb = MIN(WCurb,CarParam.oLimitSideWidth);
			  W = MIN(W,CarParam.oLimitSideWidth);
			  ExtraW = MIN(ExtraW,CarParam.oLimitSideWidth);
			}
			else
		    {
			  WCurb = MIN(WCurb,CarParam.oLimitSideWidth/2);
			  W = MIN(W,CarParam.oLimitSideWidth);
			  ExtraW = MIN(ExtraW,CarParam.oLimitSideWidth/2);
			}
			Done = true;
		  }
		}
		else
		{
		  // Wall of some sort
		  WCurb = (PSide->style >= TR_WALL) ? -1.0 : 0.0;
		  Done = true;
		}

		ExtraWpit += Wpit;
		if (!PitOnly)
		{
		  if (Done)
  	        ExtraW += WCurb;
		  else
		    ExtraW += W;

		  if (Done)
		    PitOnly = true;
		} 

		PSide = PSide->side[S];
	  }
       
	  if (S == TR_SIDE_LFT)
	  {
		oSections[I].PitWidthToLeft = oSections[I].WidthToLeft + MAX(ExtraW,ExtraWpit);
		oSections[I].WidthToLeft += ExtraW;
	  }
	  else
	  {
		oSections[I].PitWidthToRight = oSections[I].WidthToRight + MAX(ExtraW,ExtraWpit);
		oSections[I].WidthToRight += ExtraW;
	  }
	}
	NormalizeDir(Seg, DistFromStart - Seg->lgfromstart,	
	  oSections[I].T, oSections[I].Center, oSections[I].ToRight);
  }}
  //SmoothSides(0.22);
  SmoothSides(1.0);
}
//==========================================================================*

//==========================================================================*
// Mean length of sections
//--------------------------------------------------------------------------*
double TTrackDescription::MeanSectionLen() const
{
  return oMeanSectionLen;
}
//==========================================================================*

//==========================================================================*
// To right
//--------------------------------------------------------------------------*
TVec2d TTrackDescription::Normale(double TrackPos) const
{
  //int LastPos = 0;
  int Index = IndexFromPos(TrackPos); 
  const tTrackSeg* Seg = oSections[Index].Seg;

  double Tmp;
  TVec3d CenterPoint;
  TVec3d Normale;
  NormalizeDir(Seg, TrackPos - Seg->lgfromstart, Tmp, CenterPoint, Normale);

  return Normale.GetXY();
}
//==========================================================================*

//==========================================================================*
// Keep pos in 0..Tracklength
//--------------------------------------------------------------------------*
double TTrackDescription::NormalizePos(double TrackPos) const
{
  while(TrackPos < 0)
    TrackPos += oTrack->length;
  while(TrackPos >= oTrack->length )
    TrackPos -= oTrack->length;
  return TrackPos;
}
//==========================================================================*

//==========================================================================*
// Length of track
//--------------------------------------------------------------------------*
double TTrackDescription::Length() const
{
  return oTrack->length;
}
//==========================================================================*

//==========================================================================*
// Section of index
//--------------------------------------------------------------------------*
const TSection& TTrackDescription::operator[](int Index) const
{
  return oSections[Index];
}
//==========================================================================*

//==========================================================================*
// Section of index
//--------------------------------------------------------------------------*
const TSection& TTrackDescription::Section(int Index) const
{
  return oSections[Index];
}
//==========================================================================*


//==========================================================================*
// Get TORCS track data
//--------------------------------------------------------------------------*
tTrack*	TTrackDescription::Track()
{
  return oTrack;
}
//==========================================================================*

//==========================================================================*
// Get TORCS track data as const pointer
//--------------------------------------------------------------------------*
const tTrack* TTrackDescription::Track() const
{
  return oTrack;
}
//==========================================================================*

//==========================================================================*
// Const track width
//--------------------------------------------------------------------------*
double TTrackDescription::Width() const
{
  return oTrack->width;
}
//==========================================================================*

//==========================================================================*
// Nbr of sections in segment
//--------------------------------------------------------------------------*
int TTrackDescription::NbrOfSections(double Len, bool PitSection)
 {
  double Den = oTrackRes;
  if (PitSection)
    Den = 1.0;

  int Result = (int) (Len / Den);
  if (Result > Len / Den)
    Result--;
  if (Result < 1)
    Result = 1;
  return Result;
 };
//==========================================================================*

//==========================================================================*
// Calculate Center and ToRight
//--------------------------------------------------------------------------*
void TTrackDescription::NormalizeDir(
  const tTrackSeg* Seg, double ToStart,
  double& T, TVec3d& Point, TVec3d& Normale) const
{
  T = ToStart / Seg->length;
  double Zl = Seg->vertex[TR_SL].z +
    (Seg->vertex[TR_EL].z - Seg->vertex[TR_SL].z) * T;
  double Zr = Seg->vertex[TR_SR].z +
	(Seg->vertex[TR_ER].z - Seg->vertex[TR_SR].z) * T;

  if(Seg->type == TR_STR)
  {
    TVec3d Start = (TVec3d(Seg->vertex[TR_SL]) + TVec3d(Seg->vertex[TR_SR])) / 2;
    TVec3d End = (TVec3d(Seg->vertex[TR_EL]) + TVec3d(Seg->vertex[TR_ER])) / 2;
    Point = Start + (End - Start) * T;
    Normale = -TVec3d(Seg->rgtSideNormal);
    Normale.z = (Zr - Zl) / Seg->width;
  }
  else
  {
    double VZ = Seg->type == TR_LFT ? 1 : -1;
    double DeltaAngle = VZ * ToStart / Seg->radius;
	double Ang = Seg->angle[TR_ZS] - PI / 2 + DeltaAngle;
	double Cos = cos(Ang);
	double CosRad = VZ * Cos * Seg->radius;
	double Sin = sin(Ang);
	double SinRad = VZ * Sin * Seg->radius;
	Point = TVec3d(Seg->center.x + CosRad, Seg->center.y + SinRad, (Zl + Zr) / 2);
	Normale = TVec3d(Cos, Sin, (Zr - Zl) / Seg->width);
  }
}
//==========================================================================*

//==========================================================================*
// Smooth changings of track width 
//--------------------------------------------------------------------------*
void TTrackDescription::SmoothSides(double Delta) 
{
  //for (int J = 0; J < 3; J++) 
  {
    {for (int I = oCount - 2; I > 0; --I)                 
    {
	  oSections[I].WidthToLeft = 
		MIN(oSections[I+1].WidthToLeft + Delta/2.0,oSections[I].WidthToLeft);
      oSections[I].WidthToRight = 
		MIN(oSections[I+1].WidthToRight + Delta/2.0,oSections[I].WidthToRight);
    }}
    {for (int I = 2; I < oCount; I++)                 
    {
	  oSections[I].WidthToLeft = 
		MIN(oSections[I-1].WidthToLeft + 2*Delta,oSections[I].WidthToLeft);
      oSections[I].WidthToRight = 
		MIN(oSections[I-1].WidthToRight + 2*Delta,oSections[I].WidthToRight);
    }}
  }
}
//==========================================================================*

//--------------------------------------------------------------------------*
// end of file unittrack.cpp
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
