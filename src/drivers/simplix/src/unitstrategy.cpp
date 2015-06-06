//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
// unitstrategy.cpp
//--------------------------------------------------------------------------*
// TORCS: "The Open Racing Car Simulator"
// A robot for Speed Dreams-Version 1.4.0/2.X
//--------------------------------------------------------------------------*
// Pitstop strategy
// Boxenstop-Strategie
// 
// File         : unitstrategy.cpp
// Created      : 2007.02.20
// Last changed : 2013.06.25
// Copyright    : © 2007-2013 Wolf-Dieter Beelitz
// eMail        : wdb@wdbee.de
// Version      : 4.00.002
//--------------------------------------------------------------------------*
// Teile diese Unit basieren auf dem erweiterten Robot-Tutorial bt
//
//    Copyright: (C) 2002-2004 Bernhard Wymann
//    eMail    : berniw@bluewin.ch
//
// dem Roboter delphin
//
//    Copyright: (C) 2006-2007 Wolf-Dieter Beelitz
//    eMail    : wdb@wdbee.de
//
// und dem Roboter mouse_2006
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
// Im übrigen gilt für die Nutzung und/oder Weitergabe die
// GNU GPL (General Public License)
// Version 2 oder nach eigener Wahl eine spätere Version.
//--------------------------------------------------------------------------*

#include "unitglobal.h"
#include "unitcommon.h"

#include "unitstrategy.h"

//==========================================================================*
// Konstanten
//--------------------------------------------------------------------------*
const float TSimpleStrategy::cMAX_FUEL_PER_METER = 0.0008f;
const int TSimpleStrategy::cPIT_DAMMAGE = 5000;
const short int NEEDED_MAJOR_VERSION = 1;
const short int NEEDED_MINOR_VERSION = 1;
//==========================================================================*

//==========================================================================*
// Strategie erzeugen
//--------------------------------------------------------------------------*
TSimpleStrategy::TSimpleStrategy():
	oWasInPit(false),
    oFuelChecked(false),
    oFuelPerM(0.0f),
    oLastPitFuel(0.0f),
    oLastFuel(0.0f),
    oExpectedFuelPerM(0.0f),
    oRaceDistance(0.0f),
    oRemainingDistance(0.0f),
    oReserve(0.0f),
    oTrackLength(0.0f),
    oMaxFuel(100.0f),
    oMinLaps(3)
{
}
//==========================================================================*

//==========================================================================*
// Aufräumen
//--------------------------------------------------------------------------*
TSimpleStrategy::~TSimpleStrategy()
{
  if (oPit != NULL)
	delete oPit;
}
//==========================================================================*

//==========================================================================*
// Strategie initialisieren
//--------------------------------------------------------------------------*
void TSimpleStrategy::Init(TDriver *Driver)
{
  oDriver = Driver;
  oPit = new TPit(Driver);
}
//==========================================================================*

//==========================================================================*
// Ist die Box frei?
//--------------------------------------------------------------------------*
bool TSimpleStrategy::IsPitFree()
{
    bool IsFree = RtTeamIsPitFree(oDriver->TeamIndex());
	if (IsFree)
		LogSimplix.debug("#%s pit is free (%d)\n",oDriver->GetBotName(),oDriver->TeamIndex());
	else
		LogSimplix.debug("#%s pit is locked (%d)\n",oDriver->GetBotName(),oDriver->TeamIndex());
    return IsFree;
}
//==========================================================================*

//==========================================================================*
// Entscheiden, ob Boxenstopp nötig ist
//--------------------------------------------------------------------------*
bool TSimpleStrategy::NeedPitStop()
{
  float FuelConsum;                              // Fuel consumption per m
  if (oFuelPerM == 0.0)                          // If still undefined
    FuelConsum = oExpectedFuelPerM;              //   use estimated value
  else                                           // If known
    FuelConsum = oFuelPerM;                      //   use it

  bool Result = RtTeamNeedPitStop(oDriver->TeamIndex(), FuelConsum, RepairWanted(cPIT_DAMMAGE));

  if (oDriver->oTestPitStop)                     // If defined, try
    Result = true;                               //   to stop in pit

  return Result;
};
//==========================================================================*

//==========================================================================*
// Box freigeben
//--------------------------------------------------------------------------*
void TAbstractStrategy::PitRelease()
{
  RtTeamReleasePit(oDriver->TeamIndex());
  CarRaceCmd = 0;
};
//==========================================================================*

//==========================================================================*
// Tanken
//--------------------------------------------------------------------------*
float TSimpleStrategy::PitRefuel()
{
  float FuelConsum;                              // Spritverbrauch kg/m
  float Fuel;                                    // Menge in kg

  if (oFuelPerM == 0.0)                          // Wenn kein Messwert
    FuelConsum = oExpectedFuelPerM;              //   vorliegt, schätzen
  else                                           // ansonsten
    FuelConsum = oFuelPerM;                      //   Messwert nehmen

  FuelConsum *= 1.10f;                           // ggf. ohne Windschatten!

  oRemainingDistance =                           // Restliche Strecke des
    oRaceDistance - DistanceRaced;               //   Rennens ermitteln

  Fuel =                                         // Bedarf an Treibstoff
    (oRemainingDistance + oReserve) * FuelConsum;// für restliche Strecke

  if (Fuel > oMaxFuel)                           // Wenn mehr als eine Tank-
  {                                              //   füllung benötigt wird
    if (Fuel / 2 < oMaxFuel)                     // Bei zwei Tankfüllungen
      Fuel = Fuel / 2;                           //   die Hälfte tanken
    else if (Fuel / 3 < oMaxFuel)                // Bei drei Tankfüllungen.
      Fuel = Fuel / 3;                           //   ein Drittel tanken
    else if (Fuel / 4 < oMaxFuel)                // Bei vier Tankfüllungen.
      Fuel = Fuel / 4;                           //   ein Viertel tanken
    else                                         // Bei fünf Tankfüllungen
      Fuel = Fuel / 5;                           //   ein Fünftel tanken
  };

  if (Fuel > oMaxFuel - CarFuel)                 // Menge ggf. auf freien
    Fuel = oMaxFuel - CarFuel;                   // Tankinhalt begrenzen
  else                                           // ansonsten Bedarf
    Fuel = Fuel - CarFuel;                       // abzügl. Tankinhalt

  //Fuel = MIN(Fuel,10.0);                         // NUR ZUM TEST DES TEAMMANAGERS

  oLastPitFuel = (float) MAX(Fuel,0.0);          // Wenn genug da ist = 0.0

  return oLastPitFuel;                           // Menge an TORCS melden
};
//==========================================================================*

//==========================================================================*
// Umfang der Reparaturen festlegen
//--------------------------------------------------------------------------*
int TSimpleStrategy::RepairWanted(int AcceptedDammage)
{
  if (oCar->_dammage < AcceptedDammage)
	return 0;
  else if (oRemainingDistance > 5.5 * oTrackLength)
    return oCar->_dammage;
  else if (oRemainingDistance > 4.5 * oTrackLength)
    return MAX(0,oCar->_dammage - cPIT_DAMMAGE);
  else if (oRemainingDistance > 3.5 * oTrackLength)
    return MAX(0,oCar->_dammage - cPIT_DAMMAGE - 1000);
  else if (oRemainingDistance > 2.5 * oTrackLength)
    return MAX(0,oCar->_dammage - cPIT_DAMMAGE - 2000);
  else
    return MAX(0,oCar->_dammage - cPIT_DAMMAGE - 3000);
}
//==========================================================================*

//==========================================================================*
// Umfang der Reparaturen festlegen
//--------------------------------------------------------------------------*
int TSimpleStrategy::PitRepair()
{
  oState = PIT_EXIT_WAIT;
  oWasInPit = true;
  return RepairWanted(0);
}
//==========================================================================*

//==========================================================================*
// Tankfüllung beim Start bestimmen
//--------------------------------------------------------------------------*
double TSimpleStrategy::SetFuelAtRaceStart
  (PTrack Track, PCarSettings *CarSettings, PSituation Situation, float Fuel)
{
  oTrack = Track;                                // Save TORCS pointer

  oTrackLength = oTrack->length;                 // Länge der Strecke merken
  oRaceDistance =                                // Gesamtlänge des Rennens
    oTrackLength * Situation->_totLaps;          //   berechnen
  oRemainingDistance =                           // Restliche Strecke des
    oRaceDistance + oReserve;                    //   Rennens ermitteln
  Fuel = (float) 
    (Fuel * oRemainingDistance / 100000.0);      // Gesamtbedarf in kg 

  oExpectedFuelPerM = Fuel / oRemainingDistance; // Verbrauch in kg/m

  oMaxFuel =            
	  GfParmGetNum(*CarSettings,TDriver::SECT_PRIV,         // Maximal möglicher
	PRV_MAX_FUEL,(char*) NULL,oMaxFuel);         //   Tankinhalt
  LogSimplix.debug("#oMaxFuel (private) = %.1f\n",oMaxFuel);

  oStartFuel =            
	GfParmGetNum(*CarSettings,TDriver::SECT_PRIV,         // Tankinhalt beim Start
	PRV_START_FUEL,(char*) NULL,(float) oStartFuel);          
  LogSimplix.debug("#oStartFuel (private) = %.1f\n",oStartFuel);

  if ((!TDriver::Qualification)                  // Fürs Rennen 
	  && (oStartFuel > 0))
  {
    oLastFuel = (float) oStartFuel;              // volltanken
    GfParmSetNum(*CarSettings,SECT_CAR,PRM_FUEL, // Gewünschte Tankfüllung
      (char*) NULL, oLastFuel);                  //   an TORCS melden
    return oLastFuel;    
  }

  oMinLaps = (int)           
	  GfParmGetNum(*CarSettings,TDriver::SECT_PRIV, // Mindestanzahl an Runden
	PRV_MIN_LAPS,(char*) NULL,(float) oMinLaps); //   die mit dem Tankinhalt
  LogSimplix.debug("#oMinLaps (private) = %d\n",oMinLaps);  //   möglich sein müssen

  if (Fuel == 0)                                 // Wenn nichts bekannt ist,
    Fuel = oMaxFuel;                             // Volltanken

  oLastFuel = Fuel;                              // Erforderlicher Treibstoff
  if (Fuel > oMaxFuel)                           // Wenn mehr als eine Tank-
  {                                              //   füllung benötigt wird
    if (Fuel / 2 < oMaxFuel)                     // Bei zwei Tankfüllungen
      oLastFuel = Fuel / 2;                      //   die Hälfte tanken
    else if (Fuel / 3 < oMaxFuel)                // Bei drei Tankfüllungen.
      oLastFuel = Fuel / 3;                      //   ein Drittel tanken
    else if (Fuel / 4 < oMaxFuel)                // Bei vier Tankfüllungen.
      oLastFuel = Fuel / 4;                      //   ein Viertel tanken
    else                                         // Bei fünf Tankfüllungen
      oLastFuel = Fuel / 5;                      //   ein Fünftel tanken
  };

  //oLastFuel = MIN(oLastFuel,15.0);               // NUR ZUM TEST DES TEAMMANAGERS

  oLastFuel = MIN(oLastFuel, oMaxFuel);          // Überlaufen verhindern
  GfParmSetNum(*CarSettings, SECT_CAR, PRM_FUEL, // Gewünschte Tankfüllung
    (char*) NULL, oLastFuel);                    //   an TORCS melden

  return oLastFuel;
};
//==========================================================================*

//==========================================================================*
// Go to pit
//--------------------------------------------------------------------------*
bool TSimpleStrategy::GoToPit()
{
  return ((oState >= PIT_PREPARE) && (oState <= PIT_GONE));
};
//==========================================================================*

//==========================================================================*
// Start entry procedure to pit?
//--------------------------------------------------------------------------*
bool TSimpleStrategy::StartPitEntry(float& Ratio)
{
  float DLong, DLat;                             // Dist to Pit
  RtDistToPit(oCar,oTrack,&DLong,&DLat);

  if (GoToPit() && (DLong < oDistToSwitch))
  {
    Ratio = (float) (1.0 - MAX(0.0,(DLong-100)/oDistToSwitch));
    return true;
  }
  else
    return false;
};
//==========================================================================*

//==========================================================================*
// Stop entry procedure to pit?
//--------------------------------------------------------------------------*
bool TSimpleStrategy::StopPitEntry(float Offset)
{
  float DLong, DLat;                             // Dist to Pit
  RtDistToPit(oCar,oTrack,&DLong,&DLat);
  if (oWasInPit && (DLong - oTrackLength) > -Offset)
  {
    return true;
  }
  else
  {
    oWasInPit = false;
    return false;
  }
};
//==========================================================================*

//==========================================================================*
// Update Data
//--------------------------------------------------------------------------*
void TSimpleStrategy::Update(PtCarElt Car, 
  float MinDistBack, double MinTimeSlot)
{
  double CurrentFuelConsum;                      // Current fuel consum
  float DL;                                      // Distance longitudinal
  float DW;                                      // Distance lateral  
  oMinDistBack = MinDistBack;
  oMinTimeSlot = MinTimeSlot;

  oCar = Car;                                    // Save pointer

  if (!oPit->HasPits())
    return;

  RtDistToPit                                    // Get distance to pit
   (Car,oTrack,&DL,&DW);                         

  if (DL < 0)                                    // Make DL be >= 0.0
	DL = DL + oTrack->length;                    // to have it to front

  if ((DL < oDistToSwitch) && (DL > 50) && (!oFuelChecked))
  { // We passed the line to check our fuel consume!
    if (CarLaps > 1)                             // Start at lap 2
    {                                            //   to get values
      CurrentFuelConsum =                        // Current consume =
        (oLastFuel                               // Last tank capacity
        + oLastPitFuel                           // + refueled
        - oCar->priv.fuel)                       // - current capacity
        / oTrackLength;                          // related to track length

      if (oFuelPerM == 0.0)                      // At first time we use
        oFuelPerM = (float) CurrentFuelConsum;    //   our initial estimation
      else                                       // later
        oFuelPerM = (float)                      //   we get the mean
          ((oFuelPerM*CarLaps+CurrentFuelConsum) //   of what we needed till now
		  / (CarLaps + 1));

    };

    oLastFuel = oCar->priv.fuel;                 // Capacity at this estimation
    oLastPitFuel = 0.0;                          // Refueled
    oFuelChecked = true;                         // We did the estimation in this lap

    if (!oGoToPit)                               // If decision isn't made
  	  oGoToPit = NeedPitStop();                  // Check if we should have a pitstop
  }
  else if (DL < 50)                              // I we are out of the window
  {                                              // to estimate fuel consumption
    oFuelChecked = false;                        // reset flag
  };
};
//==========================================================================*

//==========================================================================*
// State (Sequential logic system)
//--------------------------------------------------------------------------*
void TSimpleStrategy::CheckPitState(float PitScaleBrake)
{
  if (oPit == NULL)                              // No Pit no service 
    return;
  if (!oPit->HasPits())
    return;

  double TrackPos = RtGetDistFromStart(oCar);    // Distance to pit

  switch(oState)                                 // Check state
  {
	case PIT_NONE:
      // We are somewhere on the track, nothing has happend yet
	  if ((!oPit->oPitLane[0].InPitSection(TrackPos)) && oGoToPit)
	  { // if we are not parallel to the pits and get the flag,
		// let's stop in the pits.
		oState = PIT_BEFORE;
	  }
	  break;

	case PIT_BEFORE:
      // We are somewhere on the track and got the flag to go to pit
	  if (oFuelChecked && oGoToPit)
	  { // If we reache pit entry and flag is still set
	    // switch to the pitlane
		oState = PIT_PREPARE;
	  }
	  break;

	case PIT_PREPARE:
      // We are near the pit entry on the track and got the flag to go to pit
	  if (oPit->oPitLane[0].InPitSection(TrackPos) && oGoToPit)
	  { // If we reache pit entry and flag is still set
	    // switch to the pitlane
		oState = PIT_ENTER;
	  }
	  break;

	case PIT_ENTER:
      // We are on the pitlane and drive to our pit
 	  if (!oPit->oPitLane[0].CanStop(TrackPos))
	  { // We have to wait, till we reached the point to stop
	    if (oDriver->CurrSpeed() < 3)
		{
	      CarAccelCmd =                          // a little throttle
			  MAX(0.05f,CarAccelCmd);
	      CarBrakeCmd = 0.0;                     // Start braking
		  LogSimplix.debug("#PIT_ENTER: Wait %g (%g)\n",TrackPos,oDriver->CurrSpeed());
		}
		else
		  LogSimplix.debug("#PIT_ENTER: Wait %g\n",TrackPos);
		break;
	  }

	  // We reached the poit to stopp
	  oState = PIT_ASKED;
  	  LogSimplix.debug("#PIT_ENTER: %g\n",TrackPos);

 	  // falls through...

	case PIT_ASKED:
	  // We are still going to the pit
	  if (oPit->oPitLane[0].CanStop(TrackPos))
	  { // If we can stop a this position we start pit procedure
  	    LogSimplix.debug("#PIT_ASKED: CanStop %g (%g)\n",TrackPos,oDriver->CurrSpeed());
		oDriver->oStanding = true;               // For motion survey!
        oPitTicker = 0;                          // Start service timer
	    CarAccelCmd = 0;                         // release throttle
	    CarBrakeCmd = 1.0;                       // Start braking
	    CarRaceCmd = RM_CMD_PIT_ASKED;           // Tell TORCS to service us! To test oPitTicker comment out
	    oState = PIT_SERVICE;                    
	  }
	  else
	  { // We can't stop here (to early or to late)
	    if (oPit->oPitLane[0].Overrun(TrackPos))
		{ // if to late
			LogSimplix.debug("#Overrun 1: %g\n",TrackPos);
		  PitRelease();
	      oState = PIT_EXIT_WAIT;
	      // pit stop finished, need to exit pits now.
		}
		else
		{
		  LogSimplix.debug("#ToShort 1: %g\n",TrackPos);
	      if (oDriver->CurrSpeed() < 3)
		  {
	        CarAccelCmd =                        // a little throttle
			  MAX(0.05f,CarAccelCmd);
	        CarBrakeCmd = 0.0;                   // Start braking
		  }
		}
	  }
	  break;

	case PIT_SERVICE:
      // Wait to reach standstill to get service from TORCS
	  oDriver->oStanding = true;                 // Keep motion survey quiet
	  oPitTicker++;                              // Check time to start service
	  if (oPitTicker > 10)                       // Check Timer
	  { // If we have to wait
		LogSimplix.debug("#oPitTicker: %d\n",oPitTicker);
		tTeamDriver* TeamDriver = RtTeamDriverByCar(oCar); 
		short int Major = RtTeamManagerGetMajorVersion();
		short int Minor = RtTeamManagerGetMinorVersion();
		if ((TeamDriver)
		  && ((Major > NEEDED_MAJOR_VERSION) 
		    || ((Major = NEEDED_MAJOR_VERSION) && (Minor >= NEEDED_MINOR_VERSION))))
		{
			LogSimplix.debug("#Pitting issues %s\n",oDriver->GetBotName());
			LogSimplix.debug("#StillToGo : %.2f m\n",TeamDriver->StillToGo);
			LogSimplix.debug("#MoreOffset: %.2f m\n",TeamDriver->MoreOffset);
			LogSimplix.debug("#TooFastBy : %.2f m/s\n",TeamDriver->TooFastBy);

			// Learn from the response
			if (fabs(TeamDriver->StillToGo) > 0.0)
			{
	          //CarSteerCmd = 0.0;                 // Straight on
			  if (fabs(CarSpeedLong) < 1.0)
			  {
	            CarAccelCmd =                    // a little throttle
			      MAX(0.005f,CarAccelCmd);
	            CarBrakeCmd = 0.0;                 // No braking
  			    LogSimplix.debug("#Accel     : %.2f\n",CarAccelCmd);
			  }
			  else
			  {
	            CarBrakeCmd = 0.1f;              // Braking
  			    LogSimplix.debug("#Brake     : %.2f\n",CarBrakeCmd);
			  }
	          CarClutchCmd = 0.5;                // Press clutch
			  if (TeamDriver->StillToGo > 0)
	            CarGearCmd = 1;                  // First gear
			  else
	            CarGearCmd = -1;                 // reverse gear
			}
			else
			{
// 			    LogSimplix.debug("#Stopped\n");
	            CarAccelCmd = 0.0;               // Stop throttle
	            CarBrakeCmd = 1.0;               // Lock brake
	            CarClutchCmd = 0.0;              // Release clutch
	            CarGearCmd = 1;                  // First gear
			}
		}

  	    if (oPitTicker > 300)                    // Check Timer
		{ // If we have to wait too long
	      PitRelease();                          // Something went wrong, we have 
	      oState = PIT_EXIT_WAIT;                // to leave and release pit for teammate
		}
	  }
	  else if (oPit->oPitLane[0].Overrun(TrackPos))
	  { // If we couldn't stop in place
		LogSimplix.debug("#Overrun 2: %g\n",TrackPos);
	    PitRelease();                            // We have to release the pit
	    oState = PIT_EXIT_WAIT;                  // for teammate
	  }
	  else
	  { // There is nothing that hampers TORCS to service us
  	    LogSimplix.debug("#PIT_SERVICE: %g (%g)\n",TrackPos,oDriver->CurrSpeed());
		CarLightCmd = 0;                         // No lights on
        CarAccelCmd = 0;                         // No throttle
	    CarBrakeCmd = 1.0;                       // Still braking
	    CarRaceCmd = RM_CMD_PIT_ASKED;           // Tell TORCS to service us! To test oPitTicker comment out
        // oState is set to next state in PitRepair()!
		// If TORCS doesn't service us, no call to PitRepair() is done!
		// We run into timeout! (oPitTicker)
		oPitStartTicker = 600;
	  }
	  break;

	case PIT_EXIT_WAIT:
      // We are still in the box
	  oDriver->oStanding = true;                 // Keep motion survey quiet
	  if ((oMinTimeSlot < 7)                     // If start slot to short
		|| ((oMinDistBack > -7)                  // or cars aside
		&& (oMinDistBack < 5)))                  // we have to wait
	  {
        oPitStartTicker--;
        if (oPitStartTicker < 0)
		{
  		  LogSimplix.debug("#PIT_EXIT: mts%g (mdb%gm)\n",oMinTimeSlot,oMinDistBack);
	      oState = PIT_EXIT;
		}
		CarLightCmd = RM_LIGHT_HEAD2;            // Only small lights on           
		CarAccelCmd = 0.0;               
	    CarBrakeCmd = 1.0;               
	  }
	  else
	  {
		CarLightCmd = RM_LIGHT_HEAD1;             // Only big lights on           
	    oState = PIT_EXIT;
	  }
	  break;

	case PIT_EXIT:
      // We are still in the box
	  oDriver->oStanding = true;                  // Keep motion survey quiet
      oGoToPit = false;                           // Service is finished, lets go
	  CarAccelCmd = 0.5;                          // Press throttle
	  CarBrakeCmd = 0;                            // Release brake
	  PitRelease();                               // Release pit for teammate
	  if (oDriver->CurrSpeed() > 5)
	    oState = PIT_GONE;                          
	  break;

	case PIT_GONE:
      // We are on our path back to the track
	  if (!oPit->oPitLane[0].InPitSection(TrackPos))
	  { // If we reached the end of the pitlane
        CarLightCmd = RM_LIGHT_HEAD1 |           // All lights on
			RM_LIGHT_HEAD2;                      
		oState = PIT_NONE;                       // Switch to default mode 
	  }
 	  break;
  }
}
//==========================================================================*

//==========================================================================*
// Statistics
//--------------------------------------------------------------------------*
void TAbstractStrategy::Statistics()
{
};
//==========================================================================*
// end of file unitstrategy.cpp
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
