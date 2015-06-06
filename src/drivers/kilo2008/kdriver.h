/*
 *      kdriver.h
 *      
 *      Copyright 2009 kilo aka Gabor Kmetyko <kg.kilo@gmail.com>
 *      Based on work by Bernhard Wymann and Andrew Sumner.
 *      
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *      
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *      
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *      MA 02110-1301, USA.
 * 
 *      $Id: kdriver.h 4635 2012-03-31 16:32:22Z wdbee $
 * 
 */

#ifndef SRC_DRIVERS_KILO2008_KDRIVER_H_
#define SRC_DRIVERS_KILO2008_KDRIVER_H_

#include <car.h>  // tCarElt
#include <raceman.h>  // tSituation

#include <string>

#include "src/drivers/kilo2008/cardata.h"
#include "src/drivers/kilo2008/linalg.h"  // v2d

class Opponents;
class Opponent;
class Pit;
class KStrategy;
class LRaceLine;

// Custom setup features
#define KILO_SECT_PRIV      "KiloPrivate"
#define KILO_ATT_FUELPERLAP "FuelPerLap"
#define KILO_ATT_MUFACTOR   "MuFactor"
#define KILO_ATT_PITTIME    "PitTime"
#define KILO_ATT_BESTLAP    "BestLap"
#define KILO_ATT_WORSTLAP   "WorstLap"
#define KILO_ATT_TEAMMATE   "Teammate"
#define KILO_ATT_MINCORNER  "MinCornerInverse"
#define KILO_ATT_CORNERSP   "CornerSpeed"
#define KILO_ATT_AVOIDSP    "AvoidSpeedAdjust"
#define KILO_ATT_CORNERACC  "CornerAccel"
#define KILO_ATT_INTMARG    "IntMargin"
#define KILO_ATT_EXTMARG    "ExtMargin"
#define KILO_ATT_BRDELAY    "BrakeDelay"
#define KILO_ATT_SECRAD     "SecurityRadius"
#define KILO_ATT_PITOFFSET  "PitOffset"
#define KILO_SECT_SKILL     "skill"
#define KILO_SKILL_LEVEL    "level"
#define KILO_SKILL_AGGRO    "aggression"
#define KILO_FORCE_PITSTOP  "force pit"


enum { NORMAL = 1, AVOIDING, CORRECTING, PITTING, BEING_OVERLAPPED };
enum { TEAM_FRIEND = 1, TEAM_FOE };
enum { AVOIDLEFT = 1, AVOIDRIGHT = 2, AVOIDSIDE = 4 };


class KDriver {
 public:
  explicit KDriver(int index);
  virtual ~KDriver();

  // Callback functions for the sim
  void drive(tSituation * s);
  int pitCommand(tSituation * s);
  void initTrack(tTrack * t, void *carHandle, void **carParmHandle,
                  tSituation * s);
  void newRace(tCarElt * car, tSituation * s);
  void endRace(tSituation * s);
  std::string bot;  // to make it possible to differentiate between Kilo & Dots

  // Used by Opponents & Pit
  tCarElt *car_ptr() const { return car_;}
  tTrack *track_ptr() const { return track_;}
  double speed() const { return my_cardata_->getSpeedInTrackDirection(); }

 protected:
  // Initialize
  void InitCa();
  void InitCw();
  void InitTireMu();
  void InitTCLFilter();
  double InitSkill(tSituation * s);

  // Driving aids
  double FilterTCL_RWD();
  double FilterTCL_FWD();
  double FilterTCL_4WD();
  double FilterTCL(double accel);
  double FilterTrk(double accel);
  double FilterABS(double brake);
  double FilterBPit(double brake);

  // Steering
  double GetSteer(tSituation * s);
  double CalcAvoidSteer(const double targetAngle);
  double CorrectSteering(double avoidsteer, double racesteer);
  double SmoothSteering(double steercmd);
  double GetOffset();
  vec2f  TargetPoint();

  // Throttle/brake handling
  void   CalcSpeed();
  double GetAccel();
  double GetBrake();
  double BrakeDist(double allowedspeed, double mu) const;
  double FilterBrakeSpeed(double brake);
  double FilterBColl(double brake);
  double FilterOverlap(double accel);
  double FilterAccel(const double accel);

  // Gear/clutch
  int GetGear();
  double GetClutch();

  // 'own' utilities
  void Update(tSituation * s);
  bool IsStuck();
  void Unstuck();
  inline double GetDistToSegEnd() const;
  void CheckPitStatus(tSituation *s);
  void * LoadDefaultSetup() const;
  void MergeCarSetups(void **oldHandle, void *newHandle) const;
  inline void SetAvoidRight() { avoid_mode_ |= AVOIDRIGHT; }
  inline void SetAvoidLeft()  { avoid_mode_ |= AVOIDLEFT; }
  inline double width() const  { return my_cardata_->getWidthOnTrack(); }
  inline double mass() const   { return CARMASS + car_->_fuel; }
  double current_speed_sqr() const { return car_->_speed_x * car_->_speed_x; }
  void SetMode(int newmode);

  // Opponent handling
  Opponent * GetTakeoverOpp();
  double FilterOverlappedOffset(const Opponent *o);
  double FilterTakeoverOffset(const Opponent *o);
  double FilterSidecollOffset(const Opponent *o, const double incfactor);


  // Constants.
  enum { NORMAL = 1, AVOIDING, CORRECTING, PITTING, OVERLAPPED };
  enum { TEAM_FRIEND = 1, TEAM_FOE };
  enum { AVOIDLEFT = 1, AVOIDRIGHT = 2, AVOIDSIDE = 4 };

  // Pointers to other classes
  tCarElt *car_;          // Pointer to tCarElt struct.
  LRaceLine *raceline_;   // The racing line
  Opponents *opponents_;  // The container for opponents.
  Pit *pit_;              // Pointer to the pit instance.
  KStrategy *strategy_;   // Pit stop strategy.
  tTrack *track_;         // Track variables.

  static Cardata *cardata_;     // Data about all cars shared by all instances.
  SingleCardata *my_cardata_;   // Pointer to "global" data about my car.

  static double current_sim_time_;  // Store time to avoid useless updates.

  // Per robot global data
  int car_index_;
  std::string car_type_;

  // Driving modes
  int mode_;
  int avoid_mode_;
  int last_mode_;

  // Timers
  double sim_time_;         // how long since the race started
  double avoid_time_;       // how long since we began avoiding
  double correct_timer_;    // how long we've been correcting
  double correct_limit_;    // level of divergence with raceline steering
  double clutch_time_;      // clutch timer
  int stuck_counter_;       // to determine if we are stuck

  // Car state
  double angle_;
  double speed_angle_;      // the angle of the speed vector
                            //  relative to trackangle, > 0.0 points to right.
  double my_offset_;        // Offset to the track middle.

  // Raceline data
  double old_lookahead_;    // Lookahead for steering in the previous step.
  double race_offset_;      // offset from middle of track towards which
                            //  raceline is steering
  double race_speed_;       // how fast raceline code says we should be going
  double avoid_speed_;      // how fast we should go if avoiding
  double brake_delay_;      // configurable, brake a wee bit late
  double pit_offset_;       // configurable, pit lane can start late/early

  // Commands
  double accel_cmd_;        // throttle pedal command [0..1]
  double brake_cmd_;        // brake pedal command [0..1]
  double race_steer_;       // steer command to get to raceline
  double last_steer_;       // previous steer command
  double last_accel_;       // previous accel command
  double last_nsa_steer_;

  // Handling traffic
  double min_catch_dist_;     // used in considering takeover
  double avoid_lft_offset_;   // closest opponent on the left
  double avoid_rgt_offset_;   // closest opponent on the right
  double rgt_inc_;
  double lft_inc_;
  double max_offset_;
  double min_offset_;
  double r_inverse_;

  // Skilling
  double skill_;
  double brake_skill_;
  double accel_skill_;
  double lookahead_skill_;
  double side_skill_;

  // Data that should stay constant after first initialization.
  int MAX_UNSTUCK_COUNT;
  int INDEX;
  double CARMASS;            // Mass of the car only [kg].
  double CA;                 // Aerodynamic downforce coefficient.
  double CW;                 // Aerodynamic drag coefficient.
  double TIREMU;             // Friction coefficient of tires.
  double OVERTAKE_OFFSET_INC;    // [m/timestep]
  double MU_FACTOR;          // [-]
  double (KDriver::*GET_DRIVEN_WHEEL_SPEED)();

  // DEBUG
  bool forcePitStop;

  // Class constants.
  static const double MAX_UNSTUCK_ANGLE;
  static const double UNSTUCK_TIME_LIMIT;
  static const double MAX_UNSTUCK_SPEED;
  static const double MIN_UNSTUCK_DIST;
  static const double G;
  static const double SHIFT;
  static const double SHIFT_MARGIN;
  static const double ABS_SLIP;
  static const double ABS_RANGE;
  static const double ABS_MINSPEED;
  static const double TCL_SLIP;
  static const double LOOKAHEAD_CONST;
  static const double LOOKAHEAD_FACTOR;
  static const double WIDTHDIV;
  static const double BORDER_OVERTAKE_MARGIN;
  static const double SIDECOLL_MARGIN;
  static const double OVERTAKE_OFFSET_SPEED;
  static const double PIT_LOOKAHEAD;
  static const double PIT_BRAKE_AHEAD;
  static const double PIT_MU;
  static const double MAX_SPEED;
  static const double TCL_RANGE;
  static const double CLUTCH_SPEED;
  static const double DISTCUTOFF;
  static const double MAX_INC_FACTOR;
  static const double CATCH_FACTOR;
  static const double TEAM_REAR_DIST;
  static const double LET_OVERTAKE_FACTOR;
};

#endif  // SRC_DRIVERS_KILO2008_KDRIVER_H_
