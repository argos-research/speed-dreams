//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
// unitfixcarparam.h
//--------------------------------------------------------------------------*
// A robot for Speed Dreams-Version 2.X simuV4
//--------------------------------------------------------------------------*
// Unchanged parameters of car
// Unveränderliche Parameter des Fahrzeugs
//
// File         : unitfixcarparam.h
// Created      : 2007.11.25
// Last changed : 2014.11.29
// Copyright    : © 2007-2014 Wolf-Dieter Beelitz
// eMail        : wdbee@users.sourceforge.net
// Version      : 4.05.000
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
#ifndef _UNITFIXCARPARAM_H_
#define _UNITFIXCARPARAM_H_

#include "unitglobal.h"
#include "unitcommon.h"

#include "unittmpcarparam.h"
#include "unitcarparam.h"

//==========================================================================*
// Deklaration der Klasse TFixCarParam
//--------------------------------------------------------------------------*
class TFixCarParam  
{
  private:
    PDriver oDriver;

  public:
    PtCarElt oCar;                               // Pointer to TORCS data of car
    TTmpCarParam* oTmpCarParam;                  // Variable car parameters									    

	TFixCarParam();                              // Default constructor 
	~TFixCarParam();                             // Destructor

	double CalcAcceleration
	  (									    
	   double Crv0, double Crvz0, 
	   double Crv1, double Crvz1,
	   double Speed, double Dist, 
	   double Friction,
	   double TrackRollAngle,
       double TrackTiltAngle) const;                

	double CalcBraking
	  (TCarParam& CarParam,									    
	   double Crv0, double Crvz0, 
	   double Crv1, double Crvz1,
	   double Speed, double Dist, 
	   double Friction,
	   double TrackRollAngle,
	   double TrackTiltAngle) const;

	double CalcBrakingPit
	  (TCarParam& CarParam,									    
	   double Crv0, double Crvz0, 
	   double Crv1, double Crvz1,
	   double Speed, double Dist, 
	   double Friction,
	   double TrackRollAngle,
       double TrackTiltAngle) const;                

	double CalcMaxSpeed
	  (TCarParam& CarParam,									    
	   double Crv0, 
	   double Crv1,
	   double Crvz, 
	   double KFriction,
	   double TrackRollAngle,
       double TrackTiltAngle) const;                

	double CalcMaxLateralF
	  (									    
       double Speed, 
	   double Friction, 
	   double Crvz = 0.0) const;

	double CalcMaxSpeedCrv() const;

	void Initialize
	  (PDriver Driver, PtCarElt Car);

  public:
	double oBorderInner;                         // Const. Buffer to inner
	double oBorderOuter;                         // Const. Buffer to outer
	double oMaxBorderInner;                      // Max var. Buffer to inner
	double oBorderScale;                         // Scale var. Buffer to inner
	double oCa;                                  // Aerodynamic downforce constant
	double oCaFrontWing;                         // Aerod. d. const. front wing
	double oCaFrontGroundEffect;                 // Aerod. d. const. ground effect front
	double oCaRearGroundEffect;                  // Aerod. d. const. ground effect rear
	double oCaRearWing;                          // Aerod. d. const. rear wing
	double oCdBody;                              // Aerodynamic drag constant car body
	double oCdWing;	                             // Aerod. drag const. wings
	double oEmptyMass;                           // Mass of car.without fuel
	double oLength;                              // Length of car (m)
	double oTyreMu;	                             // Mu of tyres
	double oTyreMuFront;                         // Mu of front tyres
	double oTyreMuRear;                          // Mu of rear tyres
	double oWidth;                               // Width of car (m)

	double oPitBrakeDist;
	double oPitMinEntrySpeed;                    // Min speed while entry to pitlane
	double oPitMinExitSpeed;                     // Min speed while exit of pitlane
    PSimpleStrategy oStrategy;                   // Pit strategy

};
//==========================================================================*
#endif // _UNITFIXCARPARAM_H_
//--------------------------------------------------------------------------*
// end of file unitfixcarparam.h
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
