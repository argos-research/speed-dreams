/***************************************************************************

    file                 : driver.h
    created              : Thu Dec 20 01:20:19 CET 2002
    copyright            : (C) 2002-2004 Bernhard Wymann
    email                : berniw@bluewin.ch
    version              : $Id: driver.h 6066 2015-08-09 18:11:28Z torcs-ng $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _DRIVER_H_
#define _DRIVER_H_

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
#include <portability.h>

#include "opponent.h"
#include "pit.h"
#include "strategy.h"
#include "cardata.h"
#include "raceline.h"
#include "mod.h"
#include "globaldefs.h"

class Opponents;
class Opponent;
class Pit;
class AbstractStrategy;
class SimpleStrategy;

enum { TEAM_FRIEND=1, TEAM_FOE };
enum { avoidleft=1, avoidright=2, avoidside=4, avoidsideclosing=8, avoidback=16 };
enum { debug_steer=1, debug_overtake=2, debug_brake=4 };

class Driver
{
public:
    Driver(int index);
    ~Driver();

    void SetBotName(void* RobotSettings, char* Value);

    // Callback functions called from Speed Dreams / TORCS.
    void initTrack(tTrack* t, void *carHandle, void **carParmHandle, tSituation *s);
    void newRace(tCarElt* car, tSituation *s);
    void drive(tSituation *s);
    int pitCommand(tSituation *s);
    void endRace(tSituation *s);
    void shutdown();

    tCarElt *getCarPtr() { return car; }
    tTrack *getTrackPtr() { return track; }
    float getSpeed() { return mycardata->getSpeedInTrackDirection(); /*speed;*/ }
    float getSpeedDeltaX() { return (float)mycardata->getSpeedDeltaX(); }
    float getSpeedDeltaY() { return (float)mycardata->getSpeedDeltaY(); }
    float getTrueSpeed() { return car->_speed_x; /* mycardata->getTrueSpeed(); */ }
    float evalTrueSpeed() { truespeed = mycardata->getTrueSpeed(); return truespeed; }
    float getNextLeft() { return car->_trkPos.toLeft + (car->_trkPos.toLeft - prevleft); }
    double getRInverse() { return rldata->rInverse; }
    double getRaceLane() { return rldata->lane; }
    tPosd *getCorner1() { return mycardata->getCorner1(); }
    tPosd *getCorner2() { return mycardata->getCorner2(); }
    float getAngle() { return angle; }
    float getSpeedAngle() { return speedangle; }
    int getAlone() { return alone; }
    int getCarefulBrake() { return raceline->getCarefulBrake(); }
    double speedAngleChange() { return speedangle - prevspeedangle; }
    void GetSteerPoint( double lookahead, vec2f *rt, double offset=-100.0, double time=-1.0 );
    void GetRLSteerPoint( vec2f *rt, double *offset, double time ) { return raceline->GetRLSteerPoint( rt, offset, time ); }
    int GetMode() { return mode; }
    float getWidth() { return mycardata->getWidthOnTrack(); }
    double getBrakeMargin() { return brakemargin; }

    double TyreConditionFront();
    double TyreConditionRear();
    double TyreTreadDepthFront();
    double TyreTreadDepthRear();

    // Per robot global data.
    static int NBBOTS;									// Nbr of cars
    double CurrSimTime;									// Current simulation time
    static const char* MyBotName;						// Name of this bot
    static const char* ROBOT_DIR;						// Sub path to dll
    static const char* SECT_PRIV;						// Private section
    static const char* DEFAULTCARTYPE;					// Default car type

    static int		RobotType;
    static bool		AdvancedParameters;
    static bool		UseOldSkilling;
    static bool		UseSCSkilling;
    static bool		UseMPA1Skilling;
    static float	SkillingFactor;
    static bool		UseBrakeLimit;
    static bool		UseGPBrakeLimit;
    static bool		UseRacinglineParameters;
    static bool		UseWingControl;
    static float	BrakeLimit;
    static float	BrakeLimitScale;
    static float	BrakeLimitBase;
    static float	SpeedLimitScale;
    static float	SpeedLimitBase;
    static bool		FirstPropagation;
    static bool		Learning;

    static double	LengthMargin;						// Length margin
    static bool		Qualification;						// Flag qualifying

    int         m_Index;
    int         m_Extended;

    bool    HasABS;
    bool    HasESP;
    bool    HasTCL;
    bool    HasTYC;

private:
    // Utility functions.
    bool isStuck();
    void update(tSituation *s);
    float getAccel();
    float getDistToSegEnd();
    float getBrake();
    int getGear();
    float getSteer(tSituation *s);
    float getClutch();
    vec2f getTargetPoint(bool use_lookahead, double targetoffset = -100.0);
    float getOffset();
    float brakedist(float allowedspeed, float mu);
    float smoothSteering( float steercmd );
    float correctSteering( float avoidsteer, float racesteer );
    double calcSteer( double targetAngle, int rl );
    void setMode( int newmode );
    void calcSpeed();
    float adjustOffset( float offset );
    bool canOvertake( Opponent *o, double *mincatchdist, bool outside, bool lenient );
    bool canOvertake2( Opponent *o, int avoidingside );
    double getFollowDistance();

    float filterTeam(float accel);
    float filterOverlap(float accel);
    float filterBColl(float brake);
    float filterABS(float brake);
    float filterBPit(float brake);
    float filterBrakeSpeed(float brake);
    float filterTurnSpeed(float brake);

    float filterTCL(float accel);
    float filterTrk(float accel);

    float filterTCL_RWD();
    float filterTCL_FWD();
    float filterTCL_4WD();
    void initTCLfilter();

    void initWheelPos();
    void initCa();
    void initCw();
    void initTireMu();
    int checkSwitch( int side, Opponent *o, tCarElt *ocar );
    int checkFlying();
    void calcSkill();
    int rearOffTrack();
    float GetSafeStuckAccel();

    void LoadDAT( tSituation *s, char *carname, char *trackname );

    void computeRadius(float *radius);
    int isAlone();
    void SetRandomSeed( unsigned int seed );
    unsigned int getRandom();
    int getWeather();
    void Meteorology();

    float stuckSteering( float steercmd );

    char* CarType;                              // Type name of own car

    int NoTeamWaiting;
    float TeamWaitTime;
    float truespeed;
    float deltaTime;
    float FuelSpeedUp;
    float TclSlip;
    float TclRange;
    float AbsSlip;
    float AbsRange;
    float OversteerASR;
    float BrakeMu;
    float BrakeScale;
    float YawRateAccel;
    int AccelMod;
    unsigned int random_seed;
    int DebugMsg;
    int racetype;
    int mode;
    int avoidmode;
    int lastmode;
    int allow_stuck;
    int stuck;
    int stuckcheck;
    float stuck_timer;
    float last_stuck_time;
    int prefer_side;
    int allowcorrecting;
    int pitpos;

    float prevspeedangle;				// the angle of the speed vector relative to trackangle, > 0.0 points to right.
    float speedangle;					// the angle of the speed vector relative to trackangle, > 0.0 points to right.
    float angle;
    float mass;							// Mass of car + fuel.
    float maxfuel;
    float myoffset, pitoffset;			// Offset to the track middle.
    float laststeer, lastbrake, lastaccel;
    float lastNSasteer, lastNSksteer;
    float avgaccel_x;
    double wheelz[4];

    char        *m_BotName;				// Name of driver
    const char  *m_TeamName;            // Name of team
    int         m_RaceNumber;           // Race number

    tCarElt *car;						// Pointer to tCarElt struct.
    LRaceLine *raceline;				// pointer to the raceline instance

    Opponents *opponents;				// The container for opponents.
    Opponent *opponent;					// The array of opponents.

    Pit *pit;							// Pointer to the pit instance.
    SimpleStrategy *strategy;			// Pit stop strategy.

    SingleCardata *mycardata;			// Pointer to "global" data about my car.
    LRLMod *tLftMargin;
    LRLMod *tRgtMargin;
    LRLMod *tYawRateAccel;

    double simtime;						// how long since the race started
    double avoidtime;					// how long since we began avoiding
    double frontavoidtime;
    double correcttimer;				// how long we've been correcting
    double correctlimit;				// level of divergence with raceline steering
    double aligned_timer;
    double stopped_timer;
    double brakedelay;
    double brakeratio;
    double deltamult;
    double nextCRinverse;
    double sideratio;
    double laststeer_direction;
    double steerLock;
    float currentspeedsqr;				// Square of the current speed_x.
    float currentspeed;
    float clutchtime;					// Clutch timer.
    float oldlookahead;					// Lookahead for steering in the previous step.
    float oldtime_mod;					// Lookahead for steering in the previous step.
    float racesteer;					// steer command to get to raceline
    float stucksteer;
    float prevleft;

    LRaceLineData *rldata;				// info queried from raceline.cpp

    float avoidlftoffset;				// closest opponent on the left
    float avoidrgtoffset;				// closest opponent on the right
    float accelcmd, brakecmd;
    float faccelcmd, fbrakecmd;
    float TurnDecel;
    float PitOffset;
    float PitExitSpeed;
    float RevsChangeDown;
    float RevsChangeUp;
    float RevsChangeDownMax;
    float MaxSteerTime;
    float MinSteerTime;
    float SteerCutoff;
    float SmoothSteer;
    float LookAhead;
    float IncFactor;
    float SideMargin;
    float OutSteerFactor;
    float StuckAccel;
    float StuckAngle;
    float FollowMargin;
    float SteerLookahead;
    float CorrectDelay;
    double SteerMaxRI;
    double SkidSteer;
    double MinAccel;
    float lookahead;
    float brakemargin;
    int MaxGear;
    int NoPit;

    float *radius;
    int alone;
    int carindex;
    int teamindex;
    float collision;

    float global_skill;
    float driver_aggression;
    float skill;
    double skill_adjust_limit;
    double skill_adjust_timer;
    double decel_adjust_targ;
    double decel_adjust_perc;
    double brake_adjust_targ;
    double brake_adjust_perc;

    float fuelperlap;
#ifdef SPEED_DREAMS
    int teamIndex;
    bool pitStopChecked;
#endif

    // Data that should stay constant after first initialization.
    int MAX_UNSTUCK_COUNT;
    int INDEX;
    float CARMASS;						// Mass of the car only [kg].
    float CA;							// Aerodynamic downforce coefficient.
    float CW;							// Aerodynamic drag coefficient.
    float TIREMU;						// Friction coefficient of tires.
    float OVERTAKE_OFFSET_INC;			// [m/timestep]
    float MU_FACTOR;					// [-]

    float GearRevsChangeDown[6];
    float GearRevsChangeUp[6];
    float GearRevsChangeDownMax[6];

    // Track variables.
    tTrack* track;

    int skipcount;
    float cmd_accel;
    float cmd_brake;
    float cmd_steer;
    int   cmd_gear;
    float cmd_clutch;
    float cmd_light;

    int mRain;

    float (Driver::*GET_DRIVEN_WHEEL_SPEED)();
    static Cardata *cardata;		// Data about all cars shared by all instances.

    // Class constants.
    static const float MAX_UNSTUCK_ANGLE;
    static const float UNSTUCK_TIME_LIMIT;
    static const float MAX_UNSTUCK_SPEED;
    static const float MIN_UNSTUCK_DIST;
    static const float G;
    static const float FULL_ACCEL_MARGIN;
    static const float SHIFT;
    static const float SHIFT_MARGIN;
    static const float ABS_MINSPEED;
    static const float LOOKAHEAD_CONST;
    static const float LOOKAHEAD_FACTOR;
    static const float WIDTHDIV;
    static const float SIDECOLL_MARGIN;
    static const float BORDER_OVERTAKE_MARGIN;
    static const float OVERTAKE_OFFSET_SPEED;
    static const float PIT_LOOKAHEAD;
    static const float PIT_BRAKE_AHEAD;
    static const float PIT_MU;
    static const float MAX_SPEED;
    static const float MAX_FUEL_PER_METER;
    static const float CLUTCH_SPEED;
    static const float CENTERDIV;
    static const float DISTCUTOFF;
    static const float MAX_INC_FACTOR;
    static const float CATCH_FACTOR;
    static const float CLUTCH_FULL_MAX_TIME;
    static const float USE_LEARNED_OFFSET_RANGE;

    static const float TEAM_REAR_DIST;
    static const int TEAM_DAMAGE_CHANGE_LEAD;
};

#endif // _DRIVER_H_

