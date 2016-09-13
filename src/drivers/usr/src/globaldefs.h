/***************************************************************************

    file                 : globaldefs.h
    created              : Sat Aug 08 21:20:19 CET 2015
    copyright            : (C) 2015 Xavier Bertaux
    email                : bertauxx@yahoo.fr
    version              : $Id: globaldefs.h 5902 2015-03-17 22:44:51Z torcs-ng $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _GLOBALDEFS_H_
#define _GLOBALDEFS_H_

#include <stdlib.h>
#include <stdio.h>

#include <string.h>
#include <math.h>
#include <time.h>

#include <tgf.h>     // TORCS
#include <track.h>   // TORCS
#include <car.h>     // TORCS
#include <raceman.h> // TORCS

// The "USR" logger instance.
extern GfLogger* PLogUSR;
#define LogUSR (*PLogUSR)

#define RTYPE_USR      0                     // Robot type usr
#define RTYPE_USR_TRB1 1                     // Robot type usr_trb1
#define RTYPE_USR_SC   2                     // Robot type usr_sc
#define RTYPE_USR_36GP 3                     // Robot type usr_36GP
#define RTYPE_USR_MPA1 4					 // Robot type usr_mpa1
#define RTYPE_USR_MPA11 5                   // Robot type usr_mpa11
#define RTYPE_USR_MPA12 6                   // Robot type usr_mpa12
#define RTYPE_USR_LS1  7					 // Robot type usr_ls1
#define RTYPE_USR_LS2  8					 // Robot type usr_ls2
#define RTYPE_USR_MP5  9					 // Robot type usr_mp5
#define RTYPE_USR_LP1  10					 // Robot type usr_lp1
#define RTYPE_USR_REF  11					 // Robot type usr_ref
#define RTYPE_USR_SRW  12                    // Robot type usr_srw

static const int MAX_NBBOTS = 100;               // Number of drivers/robots

#define SECT_PRIVATE          "private"

// raceline values
#define PRV_CURVE_FACTOR      "curve factor"
#define PRV_ACCEL_CURVE       "accel curve"
#define PRV_BRAKE_CURVE       "brake curve"
#define PRV_ACCEL_CURVE_LIMIT "accel curve limit"
#define PRV_BRAKE_CURVE_LIMIT "brake curve limit"
#define PRV_SECURITY          "security"
#define PRV_STEER_GAIN        "steer gain"
#define PRV_STEER_SKID        "steer skid"
#define PRV_SKID_ACCEL        "skid accel"
#define PRV_OVERTAKE_CAUTION  "overtake caution"
#define PRV_SKID_CORRECTION   "skid correction"
#define PRV_MIN_CORNER_INV    "min corner inverse"
#define PRV_INC_CORNER_INV    "increase corner inverse"
#define PRV_INC_CORNER_FACTOR "increase corner factor"
#define PRV_BASE_SPEED        "base speed"
#define PRV_BASE_SPEED_X      "base speed factor"
#define PRV_AVOID_SPEED       "add avoid speed"
#define PRV_AVOID_SPEED_X     "avoid speed factor"
#define PRV_AVOID_BRAKE       "add avoid brake"
#define PRV_AVOID_BRAKE_X     "avoid brake factor"
#define PRV_INT_MARGIN        "int margin"
#define PRV_EXT_MARGIN        "ext margin"
#define PRV_RL_RIGHT_MARGIN   "rl right margin"
#define PRV_RL_LEFT_MARGIN    "rl left margin"
#define PRV_BASE_BRAKE        "base brake"
#define PRV_BASE_BRAKE_X      "base brake factor"
#define PRV_BRAKE_MOD         "brake mod"
#define PRV_BRAKE_POWER       "brake power"
#define PRV_SPEED_LIMIT       "speed limit"
#define PRV_RACELINE_DEBUG    "raceline debug"
#define PRV_ACCEL_EXIT        "accel exit"
#define PRV_BUMP_CAUTION      "bump caution"
#define PRV_SLOPE_FACTOR      "slope factor"
#define PRV_STEER_MOD         "steer mod"
#define PRV_EXIT_BOOST        "exit boost"
#define PRV_EXIT_BOOST_X      "exit boost factor"
#define PRV_AV_EXIT_BOOST     "avoid exit boost"
#define PRV_AV_EXIT_BOOST_X   "avoid exit boost factor"
#define PRV_ACCEL_REDUX_X     "accel redux factor"
#define PRV_BEGIN             "bgn"
#define PRV_END               "end"
#define PRV_OFFTRACK_ALLOWED  "offtrack allowed"
#define PRV_OFFTRACK_RLIMIT   "rough limit"
#define PRV_AVOID_OFFSET      "avoid offset"

// driver values
#define PRV_PIT_DAMAGE        "pit damage"
#define PRV_PIT_END_OFFSET    "pit end offset"
#define PRV_PIT_OFFSET        "pit offset"
#define PRV_PIT_EXIT_SPEED    "pit exit speed"
#define PRV_NO_PIT            "no pit"
#define PRV_FORCE_PIT         "force pit"
#define PRV_TURN_DECEL        "turn decel"
#define PRV_REVS_UP           "revs change up"
#define PRV_REVS_DOWN         "revs change down"
#define PRV_REVS_DOWN_MAX     "revs change down max"
#define PRV_MAX_STEER_TIME    "max steer time"
#define PRV_MIN_STEER_TIME    "min steer time"
#define PRV_STEER_CUTOFF      "steer cutoff"
#define PRV_SMOOTH_STEER      "smooth steer"
#define PRV_LOOKAHEAD         "lookahead"
#define PRV_INC_FACTOR        "inc factor"
#define PRV_SIDE_MARGIN       "side margin"
#define PRV_AV_RIGHT_MARGIN   "avoid right margin"
#define PRV_AV_LEFT_MARGIN    "avoid left margin"
#define PRV_OUT_STEER_X       "out steer factor"
#define PRV_STUCK_ACCEL       "stuck accel"
#define PRV_STUCK_ANGLE       "stuck angle"
#define PRV_FOLLOW_MARGIN     "follow margin"
#define PRV_STEER_LOOKAHEAD   "steer lookahead"
#define PRV_CORRECT_DELAY     "correct delay"
#define PRV_MIN_ACCEL         "min accel"
#define PRV_MAX_GEAR          "max gear"
#define PRV_NO_TEAM_WAITING   "no team waiting"
#define PRV_TEAM_WAIT_TIME    "team wait time"
#define PRV_STEER_DEBUG       "steer debug"
#define PRV_BRAKE_DEBUG       "brake debug"
#define PRV_OVERTAKE_DEBUG    "overtake debug"
#define PRV_FUEL_SPEEDUP      "fuel speedup"
#define PRV_TCL_SLIP          "tcl slip"
#define PRV_ABS_SLIP          "abs slip"
#define PRV_TCL_RANGE         "tcl range"
#define PRV_ABS_RANGE         "abs range"
#define PRV_OVERSTEER_ASR     "oversteer asr"
#define PRV_BRAKE_MU          "brake mu"
#define PRV_BRAKE_SCALE       "brake scale"
#define PRV_FUEL_PER_LAP      "fuel per lap"
#define PRV_FUEL_PER_SECOND   "fuel per second"
#define PRV_YAW_RATE_ACCEL    "yaw rate accel"
#define PRV_MAX_FUEL          "max fuel"
#define PRV_BRAKE_MARGIN      "brake margin"
#define PRV_ACCEL_MOD         "accel mod"

#define SECT_SKILL            "skill"
#define PRV_SKILL_LEVEL       "level"
#define PRV_SKILL_AGGRO       "aggression"

#define BT_SECT_PRIV          "private"
#define BT_ATT_FUELPERLAP     "fuelperlap"
#define BT_ATT_FUELPERSECOND  "fuelpersecond"
#define BT_ATT_MUFACTOR       "mufactor"
#define BT_ATT_PITTIME        "pittime"
#define BT_ATT_BESTLAP        "bestlap"
#define BT_ATT_WORSTLAP       "worstlap"
#define BT_ATT_TEAMMATE       "teammate"

#endif
