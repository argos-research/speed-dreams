/***************************************************************************

    file                 : car.h
    created              : Sun Jan 30 12:00:15 CET 2000
    copyright            : (C) 2000 by Eric Espie
    email                : torcs@free.fr
    version              : $Id: car.h 5674 2013-08-10 21:42:15Z torcs-ng $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

/** @file
    		This is the car structure.
    @author	<a href=mailto:torcs@free.fr>Eric Espie</a>
    @version	$Id: car.h 5674 2013-08-10 21:42:15Z torcs-ng $
    @ingroup	carstruct
    @note	Short cuts are to be used with the carElt structure.
*/


#ifndef __CARV1_H__
#define __CARV1_H__

#include <track.h>
#include <plib/sg.h>

#define CAR_IDENT	0

#define MAX_NAME_LEN	32

/* designation */
#define FRNT_RGT	0	/**< front right */
#define FRNT_LFT	1	/**< front left */
#define REAR_RGT	2	/**< rear right */
#define REAR_LFT	3	/**< rear left */
#define FRNT		0	/**< front */
#define REAR		1	/**< rear */
#define RIGHT		0	/**< right */
#define LEFT		1	/**< left */


/** Wheels Specifications */
typedef struct
{
    tdble	rimRadius;	/**< Rim radius */
    tdble	tireHeight;	/**< Tire height  */
    tdble	tireWidth;	/**< Tire width */
    tdble	brakeDiskRadius; /**< Brake disk radius */
    tdble	wheelRadius;	/**< Overall wheel radius */
} tWheelSpec;
/* structure access short cuts */
#define _rimRadius(i)		info.wheel[i].rimRadius 	/**< short cut to tWheelSpec#rimRadius */
#define _tireHeight(i)		info.wheel[i].tireHeight	/**< short cut to tWheelSpec#tireHeight */
#define _tireWidth(i)		info.wheel[i].tireWidth		/**< short cut to tWheelSpec#tireWidth */
#define _brakeDiskRadius(i)	info.wheel[i].brakeDiskRadius	/**< short cut to tWheelSpec#brakeDiskRadius */
#define _wheelRadius(i)		info.wheel[i].wheelRadius	/**< short cut to tWheelSpec#wheelRadius */

/** Static visual attributes */
typedef struct {
    int		exhaustNb;	/**< Number of exhaust pipes (max 2) */
    t3Dd	exhaustPos[2];	/**< Position of exhaust pipes */
    tdble	exhaustPower;	/**< Power of the flames (from 1 to 3) */
} tVisualAttributes;

/** Static Public info */
typedef struct {
    char	name[MAX_NAME_LEN];	/**< Driver's name */
    char	sname[MAX_NAME_LEN];	/**< Driver's short name */
    char	codename[3+1];	/**< Driver's 3-letter codename */
	char	teamname[MAX_NAME_LEN];	/**< Team name */
    char	carName[MAX_NAME_LEN];	/**< Car object name */
    char	category[MAX_NAME_LEN];	/**< Car's category */
    int		raceNumber;		/**< Car's race number */
    int		startRank;		/**< Car's starting position */
    int		driverType;		/**< Driver type */
    int		networkplayer;		/**< Network player */
    int		skillLevel;		/**< Driver's skill level (0=rookie -> 3=pro) */
    tdble	iconColor[3];		/**< Car color in leaders board */
    t3Dd	dimension;		/**< Car's mesures */
    t3Dd	drvPos;			/**< Driver's position */
    t3Dd	bonnetPos;		/**< Bonnet's position */
    tdble	tank;			/**< Fuel tank capa */
    tdble	steerLock;		/**< Steer lock angle */
    t3Dd	statGC;			/**< Static pos of GC (should be the origin of car axis) */
    tWheelSpec	wheel[4];		/**< Wheels specifications */
    tVisualAttributes visualAttr; 	/**< Visual attributes */
    char	masterModel[MAX_NAME_LEN];	/**< Master 3D model car name (the exact folder name) */
    char	skinName[MAX_NAME_LEN];	/**< Custom skin name, if any */
    int		skinTargets;	    	/**< Target objects for the custom skinning
				   <br>The possible targets are :
				   - RM_CAR_SKIN_TARGET_WHOLE_LIVERY
				   - RM_CAR_SKIN_TARGET_3D_WHEELS
				   - RM_CAR_SKIN_TARGET_INTERIOR
				   - RM_CAR_SKIN_TARGET_BOARD
				   - RM_CAR_SKIN_TARGET_DRIVER
				   - RM_CAR_SKIN_TARGET_PIT_DOOR
*/
#define RM_CAR_SKIN_TARGET_WHOLE_LIVERY	 	0x00000001	/**< The whole car external livery */
#define RM_CAR_SKIN_TARGET_3D_WHEELS 	0x00000002	/**< The 3D wheels */
#define RM_CAR_SKIN_TARGET_INTERIOR	 	0x00000010	/**< The car interior */
#define RM_CAR_SKIN_TARGET_BOARD 	0x00000020	/**< The interior instrument board */
#define RM_CAR_SKIN_TARGET_DRIVER 	0x00000040	/**< The driver */
#define RM_CAR_SKIN_TARGET_PIT_DOOR 	0x00000100	/**< The pit door logo */
} tInitCar;

/* structure access shortcuts */
#define _name		info.name			/**< short cut to tInitCar#name */
#define _sname		info.sname			/**< short cut to tInitCar#sname */
#define _cname		info.codename		/**< short cut to tInitCar#codename */
#define _teamname	info.teamname		/**< short cut to tInitCar#teamname */
#define _carName	info.carName			/**< short cut to tInitCar#carName */
#define _category	info.category			/**< short cut to tInitCar#category */
#define _driverType	info.driverType			/**< short cut to tInitCar#driverType */
#define _networkPlayer info.networkplayer   /**< short cut to tInitCar#networkplayer*/
#define _skillLevel	info.skillLevel			/**< short cut to tInitCar#skillLevel */
#define _raceNumber	info.raceNumber			/**< short cut to tInitCar#raceNumber */
#define _startRank	info.startRank			/**< short cut to tInitCar#startRank */
#define _dimension	info.dimension			/**< short cut to tInitCar#dimension */
#define _dimension_x	info.dimension.x		/**< short cut to tInitCar#dimension.x */
#define _dimension_y	info.dimension.y		/**< short cut to tInitCar#dimension.y */
#define _dimension_z	info.dimension.z		/**< short cut to tInitCar#dimension.z */
#define _drvPos_x	info.drvPos.x			/**< short cut to tInitCar#drvPos.x */
#define _drvPos_y	info.drvPos.y			/**< short cut to tInitCar#drvPos.y */
#define _drvPos_z	info.drvPos.z			/**< short cut to tInitCar#drvPos.z */
#define _bonnetPos_x	info.bonnetPos.x		/**< short cut to tInitCar#bonnetPos.x */
#define _bonnetPos_y	info.bonnetPos.y		/**< short cut to tInitCar#bonnetPos.y */
#define _bonnetPos_z	info.bonnetPos.z		/**< short cut to tInitCar#bonnetPos.z */
#define _statGC		info.statGC			/**< short cut to tInitCar#statGC */
#define _statGC_x	info.statGC.x			/**< short cut to tInitCar#statGC.x */
#define _statGC_y	info.statGC.y			/**< short cut to tInitCar#statGC.y */
#define _statGC_z	info.statGC.z			/**< short cut to tInitCar#statGC.z */
#define _iconColor	info.iconColor			/**< short cut to tInitCar#iconColor */
#define _tank		info.tank			/**< short cut to tInitCar#tank */
#define _steerLock	info.steerLock			/**< short cut to tInitCar#steerLock */
#define _exhaustNb	info.visualAttr.exhaustNb	/**< short cut to tVisualAttributes#exhaustNb */
#define _exhaustPos	info.visualAttr.exhaustPos	/**< short cut to tVisualAttributes#exhaustPos */
#define _exhaustPower	info.visualAttr.exhaustPower	/**< short cut to tVisualAttributes#exhaustPower */
#define _masterModel	info.masterModel	 	/**< short cut to tInitCar#masterModel */
#define _skinName	info.skinName			/**< short cut to tInitCar#skinName */
#define _skinTargets	info.skinTargets		/**< short cut to tInitCar#skinTargets */

#define RM_DRV_HUMAN	1
#define RM_DRV_ROBOT	2


#define RM_PENALTY_DRIVETHROUGH	1
#define RM_PENALTY_STOPANDGO	2
#define RM_PENALTY_10SEC_STOPANDGO	3
#define RM_PENALTY_DISQUALIFIED	4

/** One penalty */
typedef struct CarPenalty
{
    int penalty;	/**< penalty type */
    int lapToClear;	/**< the lap before the penalty has to be cleared */
    GF_TAILQ_ENTRY(struct CarPenalty) link;
} tCarPenalty;

GF_TAILQ_HEAD(CarPenaltyHead, struct CarPenalty);

/** Race Administrative info */
typedef struct {
    double		bestLapTime;
    double*		bestSplitTime;
    double		deltaBestLapTime;
    double		curLapTime;
    double*		curSplitTime;
    double		lastLapTime;
    double		curTime;
    tdble		topSpeed;
    int			laps;
    int			nbPitStops;
    int			remainingLaps;
    int			pos;
    double		timeBehindLeader;
    int			lapsBehindLeader;
    double		timeBehindPrev;
    double		timeBeforeNext;
    tdble		distRaced;
    tdble		distFromStartLine;
    int			currentSector;
    double		scheduledEventTime;
    tTrackOwnPit 	*pit;
    int			event;
    tCarPenaltyHead	penaltyList;	/**< List of current penalties */
    tdble		prevFromStartLine;
    double		wrongWayTime;
} tCarRaceInfo;
/* structure access */
#define _bestLapTime		race.bestLapTime
#define _bestSplitTime		race.bestSplitTime
#define _deltaBestLapTime	race.deltaBestLapTime
#define _curLapTime		race.curLapTime
#define _curSplitTime		race.curSplitTime
#define _lastLapTime		race.lastLapTime
#define _curTime		race.curTime
#define _topSpeed		race.topSpeed
#define _laps			race.laps
#define _nbPitStops		race.nbPitStops
#define _remainingLaps		race.remainingLaps
#define _pos			race.pos
#define _timeBehindLeader	race.timeBehindLeader
#define _lapsBehindLeader	race.lapsBehindLeader
#define _timeBehindPrev		race.timeBehindPrev
#define _timeBeforeNext		race.timeBeforeNext
#define _distRaced		race.distRaced
#define _distFromStartLine	race.distFromStartLine
#define _currentSector		race.currentSector
#define _pit			race.pit
#define _scheduledEventTime	race.scheduledEventTime
#define _event			race.event
#define _penaltyList		race.penaltyList
#define _prevFromStartLine	race.prevFromStartLine
#define _wrongWayTime		race.wrongWayTime

/** Public info on the cars */
typedef struct {
    tDynPt	DynGC;		/**< GC data (car axis) */    
    tDynPt	DynGCg;		/**< GC data (world axis) */    
    sgMat4	posMat;		/**< position matrix */
    tTrkLocPos	trkPos;		/**< current track position. The segment is the track segment (not sides)*/
    int		state;	    	/**< state of the car.
				   <br>The states are:
				   - RM_CAR_STATE_FINISH
				   - RM_CAR_STATE_PIT
				   - RM_CAR_STATE_DNF
				   - RM_CAR_STATE_PULLUP
				   - RM_CAR_STATE_PULLSIDE
				   - RM_CAR_STATE_PULLDN
				   - RM_CAR_STATE_OUT
				   - RM_CAR_STATE_NO_SIMU
				   - RM_CAR_STATE_BROKEN
				   - RM_CAR_STATE_OUTOFGAS
				   - RM_CAR_STATE_ELIMINATED
				   - RM_CAR_STATE_ENDRACE_CALLED
				   - RM_CAR_STATE_SIMU_NO_MOVE
*/
#define RM_CAR_STATE_FINISH	 	0x00000100				/**< Car having passed the finish line */
#define RM_CAR_STATE_PIT	 	0x00000001				/**< Car currently stopped in pits */
#define RM_CAR_STATE_DNF	 	0x00000002				/**< Car did not finish */
#define RM_CAR_STATE_PULLUP	 	0x00000004				/**< Car pulled out in the air */
#define RM_CAR_STATE_PULLSIDE	 	0x00000008				/**< Car pulled out in the air */
#define RM_CAR_STATE_PULLDN	 	0x00000010				/**< Car pulled out in the air */
#define RM_CAR_STATE_OUT		(RM_CAR_STATE_DNF | RM_CAR_STATE_FINISH)/**< Car out of race */
#define RM_CAR_STATE_NO_SIMU	 	0x000000FF				/**< Do not simulate the car */
#define RM_CAR_STATE_BROKEN	 	0x00000200				/**< Engine no more working */
#define RM_CAR_STATE_OUTOFGAS	 	0x00000400				/**< Out of Gas */
#define RM_CAR_STATE_ELIMINATED	 	0x00000800				/**< Eliminated due to rules infringement */
#define RM_CAR_STATE_ENDRACE_CALLED	0x00001000				/**< Endrace called so robot freed a part of its data */
#define RM_CAR_STATE_SIMU_NO_MOVE	0x00010000 				/**< Simulation without car move (i.e. clutch applied and no wheel move)  */
    tPosd	corner[4];
    tdble       glance;         /* Glance angle degrees, left -ve, right +ve */
    tdble       oldglance;
} tPublicCar;
/* structure access */
#define _DynGC		pub.DynGC
#define _pos_X		pub.DynGC.pos.x
#define _pos_Y		pub.DynGC.pos.y
#define _pos_Z		pub.DynGC.pos.z
#define _roll		pub.DynGC.pos.ax
#define _pitch		pub.DynGC.pos.ay
#define _yaw		pub.DynGC.pos.az
#define _yaw_rate	pub.DynGC.vel.az
#define _speed_x	pub.DynGC.vel.x
#define _speed_y	pub.DynGC.vel.y
#define _speed_z	pub.DynGC.vel.z
#define _speed_xy	pub.DynGC.vel.xy
#define _accel_x	pub.DynGC.acc.x
#define _accel_y	pub.DynGC.acc.y
#define _accel_z	pub.DynGC.acc.z
#define _state		pub.state
#define _trkPos		pub.trkPos
#define _speed_X	pub.DynGCg.vel.x
#define _speed_Y	pub.DynGCg.vel.y
#define _corner_x(i)	pub.corner[i].ax
#define _corner_y(i)	pub.corner[i].ay
#define _posMat		pub.posMat
#define _glance		pub.glance
#define _oldglance	pub.oldglance

/** Dynamic wheel information */
typedef struct {
    tPosd	relPos;		/**< position relative to GC */
    tdble	spinVel;	/**< spin velocity rad/s */
    tdble	brakeTemp;	/**< brake temperature from 0 (cool) to 1.0 (hot) */
    int		state;		/**< wheel state */
    tTrackSeg	*seg;		/**< Track segment where the wheel is */
    tdble rollRes;              /**< rolling resistance, useful for sound */
	tdble   temp_in, temp_mid, temp_out;
	tdble   condition;
	tdble slipNorm; /* normalized slip, the variable of Magic Formula */
	tdble slipSide;
	tdble slipAccel;
	tdble Fx;
	tdble Fy;
	tdble Fz;
	tdble effectiveMu;
} tWheelState;
/* structure access */
#define _ride(i)	priv.wheel[i].relPos.z
#define _brakeTemp(i)	priv.wheel[i].brakeTemp
#define _wheelSpinVel(i) priv.wheel[i].spinVel
#define _wheelSeg(i)	priv.wheel[i].seg
#define _wheelSlipNorm(i) priv.wheel[i].slipNorm
#define _wheelSlipSide(i) priv.wheel[i].slipSide
#define _wheelSlipAccel(i) priv.wheel[i].slipAccel
#define _wheelFx(i) priv.wheel[i].Fx
#define _wheelFy(i) priv.wheel[i].Fy
#define _wheelFz(i) priv.wheel[i].Fz
#define _tyreT_in(i) priv.wheel[i].temp_in
#define _tyreT_mid(i) priv.wheel[i].temp_mid
#define _tyreT_out(i) priv.wheel[i].temp_out
#define _tyreCondition(i) priv.wheel[i].condition
#define _tyreEffMu(i) priv.wheel[i].effectiveMu

#define MAX_GEARS	10	/* including reverse and neutral */

typedef struct tCollisionState_ {
	int collision_count;
	sgVec3 pos;
	sgVec3 force;
} tCollisionState;
/* structure access */
#define _collCount	priv.collision_state.collision_count
#define _collPos	priv.collision_state.pos
#define _collForce	priv.collision_state.force

typedef struct MemoryPoolItem tMemoryPoolItem;
typedef tMemoryPoolItem* tMemoryPool;

typedef struct MemPoolCar 
{
	tMemoryPool newTrack;
	tMemoryPool newRace;
	tMemoryPool endRace;
	tMemoryPool shutdown;
} tMemPoolCar;

/** Data known only by the driver */
typedef struct {
    void	*paramsHandle;	/**< accessible parameters for modules */
    void	*carHandle;	/**< parameters for car caracteristics */
    int		driverIndex;	/**< index when multiple drivers are in the same dll */
	int	moduleIndex;	/**< index which is the same as the one in the files */
    char	modName[MAX_NAME_LEN];	/**< dll name */
    tWheelState	wheel[4];
    tPosd	corner[4];	/**< car's corners position */
    int		gear;	    	/**< current gear */
    tdble	fuel;	    	/**< remaining fuel (liters) */
	tdble   fuel_consumption_total; // l
	tdble   fuel_consumption_instant; // l/100km (>100 means infinity)
    tdble	enginerpm;
    tdble	enginerpmRedLine;
    tdble	enginerpmMax;
    tdble	enginerpmMaxTq;
    tdble	enginerpmMaxPw;
    tdble	engineMaxTq;
    tdble	engineMaxPw;
    tdble	gearRatio[MAX_GEARS];	/**< including final drive */
    int		gearNb;			/**< incl reverse and neutral */
    int		gearOffset;		/**< gearRatio[gear + gearOffset] is the ratio for gear */
    tdble	skid[4];		/**< skid intensity */
    tdble	reaction[4];    /**< reaction on wheels */
    int		collision;
	float   smoke;
    t3Dd	normal;
    t3Dd	collpos;        /**< Collision position, useful for sound ; Simu V2 only */
    int		dammage;
    int		debug;
	tCollisionState collision_state; /**< collision state ; Simu V3 only  */
    tMemPoolCar	memoryPool;
    tdble       driveSkill;           /**< Skill level for robots: 0.0 means as fast as possible; 10.0 means at a slower speed so players can easier win */
    tdble       steerTq;        /**< torqu on steering wheel for force feedback */
} tPrivCar;
/* structure access */
#define _fuelTotal priv.fuel_consumption_total
#define _fuelInstant priv.fuel_consumption_instant
#define _driverIndex	priv.driverIndex
#define _moduleIndex	priv.moduleIndex
#define _paramsHandle	priv.paramsHandle
#define _carHandle	priv.carHandle
#define _modName	priv.modName
#define _enginerpm	priv.enginerpm
#define _enginerpmRedLine	priv.enginerpmRedLine
#define _enginerpmMax	priv.enginerpmMax
#define _enginerpmMaxTq	priv.enginerpmMaxTq
#define _enginerpmMaxPw	priv.enginerpmMaxPw
#define _engineMaxTq	priv.engineMaxTq
#define _engineMaxPw	priv.engineMaxPw
#define _gearRatio	priv.gearRatio
#define _gearNb		priv.gearNb
#define _gearOffset	priv.gearOffset
#define _fuel		priv.fuel
#define _gear		priv.gear
#define _debug		priv.debug
#define _skid		priv.skid
#define _reaction	priv.reaction
#define _dammage	priv.dammage
#define _driveSkill	priv.driveSkill
#define _collision	priv.collision
#define _smoke		priv.smoke
#define _normal		priv.normal
#define _coll2Pos	priv.collpos
#define _steerTq	priv.steerTq

#define _newTrackMemPool	priv.memoryPool.newTrack
#define _newRaceMemPool		priv.memoryPool.newRace
#define _endRaceMemPool		priv.memoryPool.endRace
#define _shutdownMemPool	priv.memoryPool.shutdown

/** Info returned by driver during the race */
/** New order to get better alignment, additional parameters for new features */
typedef struct {
    tdble	steer;	            /**< Steer command       [-1.0, 1.0]  */
    tdble	accelCmd;           /**< Accelerator command [ 0.0, 1.0] */
    tdble	brakeCmd;           /**< Brake command       [ 0.0, 1.0] */
    tdble	clutchCmd;          /**< Clutch command      [ 0.0, 1.0] */

	// New commands for single wheel braking
	tdble	brakeFrontLeftCmd;  /**< Brake front left command  [0.0, 1.0] 0: no brake; 1: max brake */
    tdble	brakeFrontRightCmd; /**< Brake front right command [0.0, 1.0] 0: no brake; 1: max brake */
    tdble	brakeRearLeftCmd;   /**< Brake rear left command   [0.0, 1.0] 0: no brake; 1: max brake */
    tdble	brakeRearRightCmd;  /**< Brake rear right command  [0.0, 1.0] 0: no brake; 1: max brake */

	// New commands for variable wings (incl. airbrake)
    tdble	wingFrontCmd;       /**< Wing angle of attack front [0, PI/4: angle in rad] */
    tdble	wingRearCmd;        /**< Wing angle of attack rear  [0, PI/4: angle in rad] */
    tdble	reserved1;          /**< reserved for future use */
    tdble	reserved2;          /**< reserved for future use */

	int		gear;  	            /**< [-1,MAX_GEARS - 2] for gear selection */
    int		raceCmd;            /**< command issued by the driver */
#define RM_CMD_NONE		0	    /**< No race command */
#define RM_CMD_PIT_ASKED	1	/**< Race command: Pit asked */
#define RM_CMD_MAX_MSG_SIZE 32

	int		lightCmd;           /**< Lights command */
#define RM_LIGHT_HEAD1		0x00000001	/**< head light 1 */
#define RM_LIGHT_HEAD2		0x00000002	/**< head light 2 */

	int		ebrakeCmd;          /**< Parking/Emergency Brake command */

	int	    wingControlMode;        /**< 0: No wing control; 1 fixed wing angles; 2 variable wing angles */
	int	    singleWheelBrakeMode;   /**< 0: No single wheel brake; 1 single wheel brake mode */
	int	    switch3;                /**< reserved for future use */
	int	    telemetryMode;          /**< Define mode for telemetry output: 0: no output; 1: full output; ... see car.cpp SimCarUpdate2 */

	char	msg[4][RM_CMD_MAX_MSG_SIZE];     /**< 4 lines of 31 characters : 0-1 from car, 2-3 from race engine */
#define RM_MSG_LEN	31

	float	msgColor[4]; /**< RGBA of text */
} tCarCtrl;
#define _steerCmd	ctrl.steer
#define _accelCmd	ctrl.accelCmd
#define _brakeCmd	ctrl.brakeCmd
#define _clutchCmd	ctrl.clutchCmd

#define _gearCmd	ctrl.gear
#define _raceCmd	ctrl.raceCmd
#define _lightCmd	ctrl.lightCmd
#define _ebrakeCmd	ctrl.ebrakeCmd

#define _brakeFLCmd	ctrl.brakeFrontLeftCmd
#define _brakeFRCmd	ctrl.brakeFrontRightCmd
#define _brakeRLCmd	ctrl.brakeRearLeftCmd
#define _brakeRRCmd	ctrl.brakeRearRightCmd

#define _wingFCmd	ctrl.wingFrontCmd
#define _wingRCmd	ctrl.wingRearCmd

#define _msgCmd		ctrl.msg
#define _msgColorCmd	ctrl.msgColor

#define _telemetryMode ctrl.telemetryMode
#define _singleWheelBrakeMode ctrl.singleWheelBrakeMode

struct RobotItf;

/** Command issued by the car during pit stop */
typedef struct CarPitCmd
{
    tdble		fuel;
    int			repair;
#define RM_PIT_REPAIR		0
#define RM_PIT_STOPANDGO	1
    int			stopType;
} tCarPitCmd;
/* structure access */
#define _pitFuel	pitcmd.fuel
#define _pitRepair	pitcmd.repair
#define _pitStopType	pitcmd.stopType


/** Car structure (tCarElt).
    This is the main car structure, used everywhere in the code.
*/
typedef struct CarElt
{
    int			index;	/**< car index */
    tInitCar		info;	/**< public */
    tPublicCar		pub;	/**< public */
    tCarRaceInfo	race;	/**< public */
    tPrivCar		priv;	/**< private */
    tCarCtrl		ctrl;	/**< private */
    tCarPitCmd		pitcmd;	/**< private */
    struct RobotItf	*robot;	/**< private */
    struct CarElt	*next;
} tCarElt;

/* Sections in XML description files */

#define SECT_SIMU_SETTINGS			"Simulation Options"
#define SECT_CAR					"Car"
#define SECT_FRNT					"Front"
#define SECT_FRNTWING				"Front Wing"
#define SECT_FRNTAXLE				"Front Axle"
#define SECT_FRNTARB				"Front Anti-Roll Bar"
#define SECT_FRNTHEAVE				"Front Heave Spring"
#define SECT_FRNTRGTWHEEL			"Front Right Wheel"
#define SECT_FRNTLFTWHEEL			"Front Left Wheel"
#define SECT_FRNTRGTSUSP			"Front Right Suspension"
#define SECT_FRNTLFTSUSP			"Front Left Suspension"
#define SECT_FRNTRGTBRAKE			"Front Right Brake"
#define SECT_FRNTLFTBRAKE			"Front Left Brake"
#define SECT_FRNTDIFFERENTIAL		"Front Differential"
#define SECT_REAR					"Rear"
#define SECT_REARWING				"Rear Wing"
#define SECT_REARAXLE				"Rear Axle"
#define SECT_REARARB				"Rear Anti-Roll Bar"
#define SECT_REARHEAVE				"Rear Heave Spring"
#define SECT_REARRGTWHEEL			"Rear Right Wheel"
#define SECT_REARLFTWHEEL			"Rear Left Wheel"
#define SECT_REARRGTSUSP			"Rear Right Suspension"
#define SECT_REARLFTSUSP			"Rear Left Suspension"
#define SECT_REARRGTBRAKE			"Rear Right Brake"
#define SECT_REARLFTBRAKE			"Rear Left Brake"
#define SECT_REARDIFFERENTIAL		"Rear Differential"
#define SECT_CENTRALDIFFERENTIAL	"Central Differential"
#define SECT_STEER			"Steer"
#define SECT_BRKSYST		"Brake System"
#define SECT_AERODYNAMICS	"Aerodynamics"
#define SECT_ENGINE			"Engine"
#define SECT_CLUTCH			"Clutch"
#define SECT_DRIVETRAIN		"Drivetrain"
#define SECT_GEARBOX		"Gearbox"
#define SECT_DRIVER			"Driver"
#define SECT_BONNET			"Bonnet"
#define SECT_GROBJECTS		"Graphic Objects"
#define SECT_EXHAUST		"Exhaust"
#define SECT_LIGHT			"Light"
#define SECT_FEATURES		"Features"

/* Parameter names */
#define PRM_CATEGORY		"category"
#define PRM_LEN				"body length"
#define PRM_WIDTH			"body width"
#define PRM_OVERALLLEN		"overall length"
#define PRM_OVERALLWIDTH	"overall width"
#define PRM_HEIGHT			"body height"
#define PRM_MASS			"mass"
#define PRM_FRWEIGHTREP		"front-rear weight repartition"
#define PRM_FRLWEIGHTREP	"front right-left weight repartition"
#define PRM_RRLWEIGHTREP	"rear right-left weight repartition"
#define PRM_GCHEIGHT		"GC height"
#define PRM_TANK			"fuel tank"
#define PRM_FUEL			"initial fuel"
#define PRM_CENTR			"mass repartition coefficient"
#define PRM_INERTIA			"inertia"
#define PRM_EFFICIENCY		"efficiency"
#define PRM_TYPE			"type"
#define PRM_SIZE			"size"

/* Tires */
#define PRM_MU					"mu"
#define PRM_RIMDIAM				"rim diameter"
#define PRM_TIREWIDTH			"tire width"
#define PRM_TIREHEIGHT			"tire height"
#define PRM_TIRERATIO			"tire height-width ratio"
#define PRM_RIDEHEIGHT			"ride height"
#define PRM_ROLLINGRESIST		"rolling resistance"
#define PRM_TOE					"toe"
#define PRM_CAMBER				"camber"
#define PRM_CA					"stiffness"
#define PRM_RFACTOR				"dynamic friction"
#define PRM_EFACTOR				"elasticity factor"
#define PRM_PRESSURE			"pressure"
#define PRM_LOADFMAX			"load factor max"
#define PRM_LOADFMIN			"load factor min"
#define PRM_OPLOAD				"operating load"
#define PRM_OPTTEMP				"optimal temperature"
#define PRM_INITTEMP			"initial temperature"
#define PRM_COLDMUFACTOR		"cold mu factor"
#define PRM_HEATINGMULT			"heating multiplier"
#define PRM_AIRCOOLINGMULT		"air cooling multiplier"
#define PRM_SPEEDCOOLINGMULT	"speed cooling multiplier"
#define PRM_WEARRATE			"wear rate multiplier"
#define PRM_FALLOFFTREADDEPTH	"falloff tread depth"
#define PRM_FALLOFFGRIPMULT		"falloff grip multiplier"
#define PRM_REMAININGGRIPMULT	"remaining grip multiplier"

#define PRM_SPR				"spring"
#define PRM_SUSPCOURSE		"suspension course"
#define PRM_BELLCRANK		"bellcrank"
#define PRM_PACKERS			"packers"
#define PRM_SLOWBUMP		"slow bump"
#define PRM_SLOWREBOUND		"slow rebound"
#define PRM_FASTBUMP		"fast bump"
#define PRM_FASTREBOUND		"fast rebound"
#define PRM_BUMPLVEL		"bump limit velocity"
#define PRM_REBOUNDLVEL		"rebound limit velocity"

#define PRM_XPOS			"xpos"
#define PRM_YPOS			"ypos"
#define PRM_ZPOS			"zpos"

#define PRM_STEERLOCK		"steer lock"
#define PRM_STEERSPD		"max steer speed"

#define PRM_BRKDIAM			"disk diameter"
#define PRM_BRKAREA			"piston area"
#define PRM_BRKREP			"front-rear brake repartition"
#define PRM_BRKPRESS		"max pressure"
#define PRM_EBRKPRESS		"emergency brake pressure"
#define PRM_BRKCOR_LR		"brake corr lr"
#define PRM_BRKCOR_FR		"brake corr fr"

#define PRM_CX				"Cx"
#define PRM_FCL				"front Clift"
#define PRM_RCL				"rear Clift"
#define PRM_FRNTAREA		"front area"
#define PRM_WINGAREA		"area"
#define PRM_WINGANGLE		"angle"
#define PRM_WINGANGLEVARIABLE "variable"

#define PRM_WINGTYPE		"wing type"
#define PRM_AOAATMAX		"aoa at max"
#define PRM_AOAATZERO		"aoa at zero"
#define PRM_AOAOFFSET		"aoa offset"

#define PRM_CLMAX			"clift max"
#define PRM_CLATZERO		"clift at zero"
#define PRM_CLASYMP			"clift asymptotic"
#define PRM_DELAYDECREASE	"clift delay"
#define PRM_CURVEDECREASE	"clift curve"

#define PRM_ANGLEOFSTALL	"angle of stall"
#define PRM_STALLWIDTH		"stall width"
#define PRM_ASPECTRATIO		"aspect ratio"

/* Engine */
#define PRM_REVSLIM			"revs limiter"
#define PRM_REVSMAX			"revs maxi"
#define PRM_TICKOVER		"tickover"
#define PRM_RPM				"rpm"
#define PRM_TQ				"Tq"
#define ARR_DATAPTS			"data points"
#define PRM_FUELCONS		"fuel cons factor"
#define PRM_ENGBRKCOEFF		"brake coefficient"
#define PRM_ENGBRKLINCOEFF	"brake linear coefficient"
#define PRM_POWER			"power"
#define PRM_TURBO			"turbo"
#define PRM_TURBO_RPM		"turbo rpm"
#define PRM_TURBO_FACTOR	"turbo factor"
#define PRM_TURBO_LAG		"turbo lag"
#define PRM_CAPACITY		"capacity"
#define PRM_CYLINDERS		"cylinders"
#define PRM_ENGSHAPE		"shape"
#define PRM_ENGPOS			"position"

#define PRM_RATIO			"ratio"
#define PRM_BIAS			"bias"

#define ARR_GEARS			"gears"
#define PRM_SHIFTTIME		"shift time"

#define	PRM_ROLLCENTER		"roll center height"

#define LST_RANGES			"Ranges"
#define PRM_THRESHOLD		"threshold"
#define PRM_CAR				"car"
#define PRM_WHEELSON		"wheels"
#define PRM_TEMPLATE		"template"
#define PRM_ENV				"env"
#define PRM_BONNET			"bonnet"
#define PRM_WHEEL_TEXTURE	"wheel texture"
#define PRM_SHADOW_TEXTURE	"shadow texture"

#define SECT_STEERWHEEL	    "Steer Wheel"
#define PRM_SW_MODEL        "model"
#define PRM_SW_MODELHR      "hi res model"
#define PRM_SW_ANGLE        "angle"
#define PRM_SW_MOVT         "movt"

#define LST_DRIVER          "Driver"
#define PRM_DRIVERSTEER     "steer"
#define PRM_DRIVERMODEL     "driver"

#define LST_REARWING        "RearWing"
#define PRM_REARWINGANGLE   "angle"
#define PRM_REARWINGMODEL   "rearwing"

#define SECT_COCKPIT		"cockpit"
#define PRM_MODELCOCKPIT	"model cockpit"

#define SECT_WING_MODEL		"wing model"
#define PRM_WING_1			"model 1"
#define PRM_WING_2			"model 2"

#define PRM_MIN_TQ_BIAS		"min torque bias"
#define PRM_MAX_TQ_BIAS		"max torque bias"
#define PRM_MAX_SLIP_BIAS	"max slip bias"
#define PRM_LOCKING_TQ		"locking input torque"
#define PRM_VISCOSITY_FACTOR	"viscosity factor"


#define VAL_DIFF_NONE		"NONE"
#define VAL_DIFF_SPOOL		"SPOOL"
#define VAL_DIFF_FREE		"FREE"
#define VAL_DIFF_LIMITED_SLIP	"LIMITED SLIP"
#define VAL_DIFF_LOCKING     	"LOCKING"
#define VAL_DIFF_VISCOUS_COUPLER "VISCOUS COUPLER"

#define VAL_TRANS_RWD		"RWD"
#define VAL_TRANS_FWD		"FWD"
#define VAL_TRANS_4WD		"4WD"

#define VAL_ENGSHAPE_V		"v"
#define VAL_ENGSHAPE_L		"l"
#define VAL_ENGSHAPE_H		"h"
#define VAL_ENGSHAPE_W		"w"

#define VAL_ENGPOS_FRONT		"front"
#define VAL_ENGPOS_FRONTMID		"front-mid"
#define VAL_ENGPOS_MID		"mid"
#define VAL_ENGPOS_REARMID		"rear-mid"
#define VAL_ENGPOS_REAR		"rear"

/* graphic */
#define PRM_TACHO_TEX		"tachometer texture"
#define PRM_TACHO_XSZ		"tachometer width"
#define PRM_TACHO_YSZ		"tachometer height"
#define PRM_TACHO_XPOS		"tachometer x pos"
#define PRM_TACHO_YPOS		"tachometer y pos"
#define PRM_TACHO_NDLXSZ	"tachometer needle width"
#define PRM_TACHO_NDLYSZ	"tachometer needle height"
#define PRM_TACHO_XCENTER	"tachometer needle x center"
#define PRM_TACHO_YCENTER	"tachometer needle y center"
#define PRM_TACHO_XDIGITCENTER	"tachometer digit x center"
#define PRM_TACHO_YDIGITCENTER	"tachometer digit y center"
#define PRM_TACHO_MINVAL	"tachometer min value"
#define PRM_TACHO_MAXVAL	"tachometer max value"
#define PRM_TACHO_MINANG	"tachometer min angle"
#define PRM_TACHO_MAXANG	"tachometer max angle"

#define PRM_SPEEDO_TEX		"speedometer texture"
#define PRM_SPEEDO_XSZ		"speedometer width"
#define PRM_SPEEDO_YSZ		"speedometer height"
#define PRM_SPEEDO_XPOS		"speedometer x pos"
#define PRM_SPEEDO_YPOS		"speedometer y pos"
#define PRM_SPEEDO_NDLXSZ	"speedometer needle width"
#define PRM_SPEEDO_NDLYSZ	"speedometer needle height"
#define PRM_SPEEDO_XCENTER	"speedometer needle x center"
#define PRM_SPEEDO_YCENTER	"speedometer needle y center"
#define PRM_SPEEDO_XDIGITCENTER	"speedometer digit x center"
#define PRM_SPEEDO_YDIGITCENTER	"speedometer digit y center"
#define PRM_SPEEDO_MINVAL	"speedometer min value"
#define PRM_SPEEDO_MAXVAL	"speedometer max value"
#define PRM_SPEEDO_MINANG	"speedometer min angle"
#define PRM_SPEEDO_MAXANG	"speedometer max angle"
#define PRM_SPEEDO_DIGITAL      "speedometer digital"

#define PRM_NEEDLE_RED		"needle red"
#define PRM_NEEDLE_GREEN	"needle green"
#define PRM_NEEDLE_BLUE		"needle blue"
#define PRM_NEEDLE_ALPHA	"needle alpha"

#define PRM_WHEEL_3D            "3d wheel"
#define PRM_FRONT_WHEEL_3D      "3d front wheel"
#define PRM_REAR_WHEEL_3D       "3d rear wheel"

/* Lights */
#define	VAL_LIGHT_HEAD1		"head1"
#define	VAL_LIGHT_HEAD2		"head2"
#define	VAL_LIGHT_BRAKE		"brake"
#define	VAL_LIGHT_BRAKE2	"brake2"
#define	VAL_LIGHT_REVERSE	"reverse"
#define	VAL_LIGHT_REAR		"rear"

/* Simulation Options */
#define PRM_DAMAGE_TYRES "damage/tyres"
#define PRM_DAMAGE_SUSPENSION "damage/suspension"
#define PRM_DAMAGE_ALIGNMENT "damage/alignment"
#define PRM_DAMAGE_AERO "damage/aero"
#define PRM_MODEL_AEROFLOW "model/aero/flow"
#define PRM_MODEL_AERO_FACTOR "model/aero/factor"
#define PRM_MODEL_TYRE_TEMPERATURE "model/tyre/temperature"

// Collision constants.
#define SEM_COLLISION			0x01
#define SEM_COLLISION_XYSCENE	0x02
#define SEM_COLLISION_CAR		0x04
#define SEM_COLLISION_Z			0x08
#define SEM_COLLISION_Z_CRASH	0x10

/* features */
#define PRM_AEROTOCG		"shifting aero coordinates"
#define PRM_SLOWGRIP		"fixed low speed grip"
#define PRM_REALGEARCHANGE	"realistic gear change"
#define PRM_REVLIMIT		"realistic rev limiter"
#define PRM_TIRETEMPDEG		"tire temperature and degradation"
#define PRM_FIXEDWHEELFORCE	"fixed wheel force"
#define VAL_YES			"yes"
#define VAL_NO			"no"
#define FEAT_AEROTOCG		0x01 //shift aero coordinates with the CG
#define FEAT_SLOWGRIP		0x02 //fix magic formula divergence at low speeds
#define FEAT_REALGEARCHANGE	0x04 //neutral between old and new gears without autoclutch
#define FEAT_REVLIMIT		0x08 //rev drop when hitting the limit
#define FEAT_TIRETEMPDEG	0x10 //tire temperature and degradation
#define FEAT_FIXEDWHEELFORCE	0x20 //fixed wheel force in SimWheelUpdateForce

#endif /* __CARV1_H__ */ 



