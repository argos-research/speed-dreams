//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
// globaldefinitions.h
//--------------------------------------------------------------------------*
// Global definitions used by all other files of the robot
// 
// File         : globaldefinitions.h
// Created      : 2011.06.19
// Last changed : 2011.09.26
// Copyright    : � 2011 Wolf-Dieter Beelitz, 2013 D.Schellhammer
//--------------------------------------------------------------------------*
// V0.00.000:
//--------------------------------------------------------------------------*
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//--------------------------------------------------------------------------*
#ifndef _GLOBAL_DEFINITIONS_H_
#define _GLOBAL_DEFINITIONS_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <tgf.h>
#include <car.h>
#include <track.h>
#include <raceman.h>
#include <robot.h>
#include <robottools.h>
#include <portability.h>



//==========================================================================*
// Global constants, to be changed for different wdbee-bots
//--------------------------------------------------------------------------*
static const int MAX_NBBOTS = 100;                    // Max nbr of drivers per robot
const int cMAX_OPP = 120;                     // Max nbr of drivers per race
const int RTYPE_FRAMEWORK = 100;              // Robot type 
const float HALFFRICTION = (float) sqrt(0.5); // 1=Sqrt(2*HALFFRICTION**2) 
const float DELTA_OFFSET = 0.0001f;			  // Scale offset change
const float MAX_SCALE_FRICTION = 0.80f;		  // Limit for friction calc.
const float SLOWSPEED = 5.0f;				  // Define slow speed in m/s
const int UNSTUCK_COUNTER = 90;				  // Define drive back time
//==========================================================================*

//==========================================================================*
// Collision detection constants
//--------------------------------------------------------------------------*
const double BACKCOLLDIST = 30;                  // section of interest in m
const double FRONTCOLLDIST = 200;				 // section of interest in m
const double LMARGIN = 1.5;                      // Length margin in m
const double SMARGIN = 1.0;                      // Side margin in m
//==========================================================================*

//==========================================================================*
// Type definitions for SD pointers
//--------------------------------------------------------------------------*
typedef tTrack* PTrack;                            // SD track
typedef tCarElt* PtCarElt;                         // SD car
typedef CarElt* PCarElt;                           // SD car
typedef void* PCarHandle;                          // SD file handle
typedef void* PCarSettings;                        // SD file handle
typedef tSituation* PSituation;                    // SD situation
typedef tTrackSeg* PTrackSeg;                      // SD segment of track
//==========================================================================*


//==========================================================================*
// Shortcuts
//--------------------------------------------------------------------------*
// Shortcuts for SD commands ...
#define CarAccelCmd (oCar->_accelCmd)
#define CarBrakeCmd (oCar->_brakeCmd)
#define CarClutchCmd (oCar->_clutchCmd)
#define CarGearCmd (oCar->_gearCmd)
#define CarSteerCmd (oCar->_steerCmd)
#define CarRaceCmd (oCar->_raceCmd)
#define CarLightCmd (oCar->_lightCmd)
// ... Shortcuts for SD commands

// Shortcuts for SD ...
#define CarAccelLat (oCar->_accel_y)
#define CarAccelLong (oCar->_accel_x)
#define CarDamage (oCar->_dammage)
#define CarDistanceRaced (oCar->_distRaced)
#define CarDriverIndex (oCar->_driverIndex)
#define CarFriction (oCar->_trkPos.seg->surface->kFriction)
#define CarFuel (oCar->_fuel)
#define CarGearNbr (oCar->_gearNb)
#define CarGearOffset (oCar->_gearOffset)
#define CarGearRatio oCar->_gearRatio
#define CarCarHandle (oCar->_carHandle)
#define CarIndex (oCar->index)
#define CarLaps (oCar->_laps)
#define CarLapsBehindLeader (oCar->_lapsBehindLeader)
#define CarLength (oCar->_dimension_x)
#define CarDistFromStart (oCar->race.distFromStartLine)
#define CarPit (oCar->_pit)
#define CarPos (oCar->_trkPos)
#define CarPosAz (oCar->pub.DynGC.pos.az)
#define CarPosX (oCar->_pos_X)
#define CarPosY (oCar->_pos_Y)
#define CarPosZ (oCar->_pos_Z)
#define CarPubGlobPosX (oCar->pub.DynGCg.pos.x)
#define CarPubGlobPosY (oCar->pub.DynGCg.pos.y)
#define CarPubGlobVelX (oCar->pub.DynGCg.vel.x)
#define CarPubGlobVelY (oCar->pub.DynGCg.vel.y)
#define CarPubGlobAccX (oCar->pub.DynGCg.acc.x)
#define CarPubGlobAccY (oCar->pub.DynGCg.acc.y)
#define CarRpm (oCar->_enginerpm)
#define CarRpmLimit (oCar->_enginerpmRedLine)
#define CarSeg (oCar->_trkPos.seg)
#define CarSegWidth (oCar->_trkPos.seg->width)
#define CarSpeedLat (oCar->_speed_y)
#define CarSpeedLong (oCar->_speed_x)
#define CarSpeedX (oCar->_speed_X)
#define CarSpeedY (oCar->_speed_Y)
#define CarSteerLock (oCar->_steerLock)
#define CarState (oCar->_state)
#define CarTeamname (oCar->_teamname)
#define CarToMiddle (oCar->_trkPos.toMiddle)
#define CarToStart (oCar->_trkPos.toStart)
#define CarTrackPos (oCar->_trkPos)
#define CarWidth (oCar->_dimension_y)
#define CarYaw (oCar->_yaw)
#define CarYawRate (oCar->_yaw_rate)
#define DistanceFromStartLine (oCar->_distFromStartLine)
#define EngineRpmMaxTq (oCar->priv.enginerpmMaxTq)
#define HasDriveTrainFront (oDriveTrainType == cDT_FWD || oDriveTrainType == cDT_4WD)
#define HasDriveTrainRear (oDriveTrainType == cDT_RWD || oDriveTrainType == cDT_4WD)
#define NextGear (oCar->_gear + 1)
#define PrevGear (oCar->_gear - 1)
#define RemainingLaps (oCar->_remainingLaps)
#define TreadClutch (oClutch = 0.5)
#define NextRpm (oShift[oCar->_gear])
#define IsFullThrottle ((oAccel >= 1.0) && (oBrake <= 0.0))
#define SteerLock (oCar->_steerLock)
#define WheelRad(x) (oCar->_wheelRadius(x))
#define WheelSpinVel(x) (oCar->_wheelSpinVel(x))
#define WheelSeg(x) (oCar->_wheelSeg(x))
#define WheelSegFriction(x) (oCar->_wheelSeg(x)->surface->kFriction)
#define WheelSegRoughness(x) (oCar->_wheelSeg(x)->surface->kRoughness)
#define WheelSegRollRes(x) (oCar->_wheelSeg(x)->surface->kRollRes)
#define UsedGear (oCar->_gear)
// ... Shortcuts for SD

// Shortcuts for this robot ...
#define	DEG_TO_RAD(x) ((x) * PI / 180.0)
#define	DET(Xx,Xy,Yx,Yy,Zx,Zy) (sqrt((Xx * Xx + Xy * Xy)*(Yx * Yx + Yy * Yy)*(Zx * Zx + Zy * Zy)))
#define IsTickover (oCar->_gear <= 0)
#define MINMAX(X,Y) (MAX(-X,MIN(X,Y)))
#define	SGN(X) ((X) < 0 ? -1 : (X) > 0 ? 1 : 0)
#define XX2Y(X,Y) (X*X/(2*Y))

#define X_StartEntry X[0]
#define X_EndEntry X[1]
#define X_StartPitlane X[2]
#define X_StartPit X[3]
#define X_Pit X[4]
#define X_EndPit X[5]
#define X_EndPitlane X[6]
#define X_StartExit X[7]
#define X_EndExit X[8]

#define Y_StartEntry Y[0]
#define Y_EndEntry Y[1]
#define Y_StartPitlane Y[2]
#define Y_StartPit Y[3]
#define Y_Pit Y[4]
#define Y_EndPit Y[5]
#define Y_EndPitlane Y[6]
#define Y_StartExit Y[7]
#define Y_EndExit Y[8]

// ... Shortcuts for this robot
//==========================================================================*

//==========================================================================*
// Driver parameter names in XML files
//--------------------------------------------------------------------------*
#define PRV_FIXEDOFFSET      "fixed offset"       // Fixed offset for tests
#define PRV_FORCEPITSTOP     "force pit stop"     // Force allways pit stops
#define PRV_FUELPER100KM     "fuelper100km"       // Fuel consumpt. at 100 km
#define PRV_MAX_FUEL         "max fuel"           // Capacity 
#define PRV_QUALIFICATION    "qualifying"         // Practice as qualifying
#define PRV_RESERVE          "reserve"            // Reserve in m
#define PRV_START_FUEL       "start fuel"         // Fuel at start of race
#define PRV_USE_MAX_BRAKE    "use max brake"
#define PRV_MIN_BRAKE_RADIUS "min brake radius"
#define PRV_SCALE_MAX_BRAKE  "scale max brake"    // Scale brake power
#define PRV_SCALE_BRAKE      "scale brake"        // Scale brake
#define PRV_SCALE_BUMPS      "scale bumps"        // Scale bumps
#define PRV_SCALE_SPEED      "scale speed"        // Scale speed
//==========================================================================*

//==========================================================================*
// Status flags
//--------------------------------------------------------------------------*
const int OPP_LEFT		 = 0x000001; // You are at my left side
const int OPP_RIGHT		 = 0x000002; // or my right side
const int OPP_FRONT		 = 0x000004; // in front of me
const int OPP_REAR		 = 0x000008; // behind me 

const int OPP_AHEAD		 = 0x000010; // I see you in front of me
const int OPP_SIDE		 = 0x000020; // or looking to a side
const int OPP_BEHIND	 = 0x000040; // or in the mirror

const int OPP_FASTER     = 0x000100; // Drives faster than we
const int OPP_SLOWER     = 0x000200; // Drives slower than we
const int OPP_TRK_LEFT	 = 0x000400; // British: at the left side of track
const int OPP_TRK_RIGHT	 = 0x000800; // European: at the right side of track

const int OPP_IGNORE     = 0x000100;
const int OPP_INPIT      = 0x002000;
const int OPP_COLL       = 0x004000;

const int OPP_OVERTAKE   = 0x010000;
const int OPP_CHANGESIDE = 0x020000;
const int OPP_LETPASS    = 0x040000;

const int OPP_CATCHING	 = 0x100000;
const int OPP_CATCHING_ACC = 0x200000;

//==========================================================================*

#endif // _GLOBAL_DEFINITIONS_H_
//--------------------------------------------------------------------------*
// end of file globaldefinitions.h
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*

