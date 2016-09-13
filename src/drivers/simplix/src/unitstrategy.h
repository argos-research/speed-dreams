//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
// unitstrategy.h
//--------------------------------------------------------------------------*
// A robot for Speed Dreams-Version 2.X simuV4
//--------------------------------------------------------------------------*
// Pitstop strategy
// Boxenstop-Strategie
//
// File         : unitstrategy.h
// Created      : 2007.02.20
// Last changed : 2014.11.29
// Copyright    : © 2007-2014 Wolf-Dieter Beelitz
// eMail        : wdbee@users.sourceforge.net
// Version      : 4.05.000
//--------------------------------------------------------------------------*
// Teile diese Unit basieren auf dem erweiterten Robot-Tutorial bt
//
//    Copyright: (C) 2002-2004 Bernhard Wymann
//    eMail    : berniw@bluewin.ch
//
// dem Roboter delphin
//
//    Copyright: (C) 2006-2007 Wolf-Dieter Beelitz
//    eMail    : wdbee@users.sourceforge.net
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
#ifndef _UNITSTRATEGY_H_
#define _UNITSTRATEGY_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <tgf.h>
#include <track.h>
#include <car.h>
#include <raceman.h>
#include <robottools.h>
#include <robot.h>

#include "unitglobal.h"
#include "unitcommon.h"
#include "unitdriver.h"
#include "unitpit.h"

//==========================================================================*
// Basisklasse für Strategien zum Boxenstopp
//--------------------------------------------------------------------------*
class TAbstractStrategy
{
  protected:
	enum
	{
	  PIT_NONE,
	  PIT_BEFORE,
	  PIT_PREPARE,
	  PIT_ENTER,
	  PIT_ASKED,
	  PIT_SERVICE,
	  PIT_EXIT_WAIT,
	  PIT_EXIT,
	  PIT_GONE
	};
	int	oState;		                             // Current pitting state
    bool oGoToPit;                               // Pit stop needed

  public:
    PtCarElt oCar;                               // TORCS-Daten
	PTrack oTrack;                               // Track-Daten
	PPit oPit;
	PDriver oDriver;

	double oDistToSwitch;                        // Dist to Pit
	double oStartFuel;                           // Fuel at start
	double oTireLimitFront;
	double oTireLimitRear;
	double oDegradationPerLap;
	int oLaps;

	TAbstractStrategy()
	  :oState(PIT_NONE),oGoToPit(false),oCar(NULL),oTrack(NULL),oPit(NULL),oDriver(NULL),oDistToSwitch(100),oStartFuel(-1){};
	virtual ~TAbstractStrategy(){};

	virtual bool IsPitFree() = 0;
	virtual bool NeedPitStop() = 0;
	virtual float PitRefuel() = 0;
	virtual int PitRepair() = 0;
	virtual int RepairWanted
	  (int AcceptedDammage) = 0;
	virtual double SetFuelAtRaceStart
	  (PTrack Track, PCarSettings *CarSettings, PSituation Situation, float Fuel) = 0;
	virtual void Update
	  (PtCarElt Car, float MinDistBack, double MinTimeSlot) = 0;
	void PitIsFree();

    virtual void CheckPitState(float PitScaleBrake) = 0;
	virtual bool GoToPit() = 0;
	virtual bool StartPitEntry(float& Ratio) = 0;
	virtual bool StopPitEntry(float Offset) = 0;
    void Statistics();

	void PitRelease();
	int GetState(){return oState;};
	bool OutOfPitlane(){return oState < PIT_ENTER;};
};
//==========================================================================*

//==========================================================================*
// Einfache Strategie für Boxenstopps (Tanken und Reparieren)
//--------------------------------------------------------------------------*
class TSimpleStrategy
  : public TAbstractStrategy
{
  private:

  public:
	bool oWasInPit;             // Was in pit?
    bool oFuelChecked;          // Treibstoffverbrauchsstatistik berechnet?
    float oFuelPerM;            // Mittlerer Treibstoffverbrauch in kg pro m
    float oLastPitFuel;         // Getankte Menge an Treibstoff
    float oLastFuel;            // Tankinhalt beim Überqueren der Startlinie
    float oExpectedFuelPerM;    // Gesch. Treibst.bedarf fürs Rennen in kg/m
	int oPitTicker;             // Overrunner
	int oPitStartTicker;        // Blockadebrecher
    float oRaceDistance;        // Gesamtlänge des Rennens in m
    float oRemainingDistance;   // Verbeleibende Länge des Rennens in m
    float oReserve;             // Reserve in m
    float oTrackLength;         // Länge der Rennstrecke in m
    float oMaxFuel;             // Maximaler Tankinhalt in kg
	int oMinLaps;               // Mindestanzahl von Runden mit Tankinhalt
    float oMinDistBack;
    double oMinTimeSlot;

	TSimpleStrategy();
	~TSimpleStrategy();

    void Init(PDriver Driver);

	bool IsPitFree();
	bool NeedPitStop();
	float PitRefuel();
	int PitRepair();
	int RepairWanted
	  (int AcceptedDammage);
	double SetFuelAtRaceStart
	  (PTrack Track, PCarSettings *CarSettings, PSituation Situation, float Fuel);
	void Update
	  (PtCarElt Car, float MinDistBack, double MinTimeSlot);

    void CheckPitState(float PitScaleBrake);
	bool GoToPit();
	bool StartPitEntry(float& Ratio);
	bool StopPitEntry(float Offset);

	static const float cMAX_FUEL_PER_METER;	// [kg/m] Geschätzter Spritverbrauch
	static const int cPIT_DAMMAGE;			// Grenzwert für Schäden
};
//==========================================================================*
#endif // _UNITSTRATEGY_H_
//--------------------------------------------------------------------------*
// end of file unitstrategy.h
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
