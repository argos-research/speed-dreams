/*
 *      kdriver.cpp
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
 *      $Id: kdriver.cpp 6198 2015-11-04 15:18:41Z wdbee $
 * 
 */

#include "src/drivers/kilo2008/kdriver.h"

#include <portability.h>  // snprintf under MSVC
#include <robottools.h>  // Rt*
#include <robot.h>  // ROB_PIT_IM

#include <sstream>
#include <list>
#include <string>

// #define KDRIVER_DEBUG

#include "src/drivers/kilo2008/opponent.h"
#include "src/drivers/kilo2008/strategy.h"
#include "src/drivers/kilo2008/pit.h"
#include "src/drivers/kilo2008/raceline.h"
#include "src/drivers/kilo2008/util.h"

using ::std::string;
using ::std::stringstream;
using ::std::list;

// "I AM DEATH, NOT TAXES.  *I* TURN UP ONLY ONCE."  --  Death
// Fear was theirs, not yers.

static int
  pitstatus[128] = { 0 };
#if 0
static double colour[] = {1.0, 0.0, 0.0, 0.0};
#endif

// Constants

// [radians] If the angle of the car on the track is smaller,
// we assume we are not stuck.
const double KDriver::MAX_UNSTUCK_ANGLE = 15.0 / 180.0 * PI;
// [s] We try to get unstuck after this time.
const double KDriver::UNSTUCK_TIME_LIMIT = 2.0;
// [m/s] Below this speed we consider being stuck.
const double KDriver::MAX_UNSTUCK_SPEED = 5.0;
// [m] If we are closer to the middle we assume to be not stuck.
const double KDriver::MIN_UNSTUCK_DIST = 3.0;
// [m/(s*s)] Welcome on Earth.
const double KDriver::G = 9.81;
// [-] (% of rpmredline) When do we like to shift gears.
const double KDriver::SHIFT = 0.95;
// [m/s] Avoid oscillating gear changes.
const double KDriver::SHIFT_MARGIN = 4.4;
// [m/s] range [0..10]
const double KDriver::ABS_SLIP = 2.5;
// [m/s] range [0..10]
const double KDriver::ABS_RANGE = 5.0;
// [m/s] Below this speed the ABS is disabled
// (numeric, division by small numbers).
const double KDriver::ABS_MINSPEED = 3.0;
// [m/s] range [0..10]
const double KDriver::TCL_SLIP = 2.0;
// [m/s] range [0..10]
const double KDriver::TCL_RANGE = 10.0;
// [m]
const double KDriver::LOOKAHEAD_CONST = 18.0;
const double KDriver::LOOKAHEAD_FACTOR = 0.33;
// [-] Defines the percentage of the track to use (2/WIDTHDIV).
const double KDriver::WIDTHDIV = 2.0;
// [m]
const double KDriver::BORDER_OVERTAKE_MARGIN = 1.0;

const double KDriver::SIDECOLL_MARGIN = 2.0;

// [m/s] Offset change speed.
const double KDriver::OVERTAKE_OFFSET_SPEED = 5.0;
// [m] Lookahead to stop in the pit.
const double KDriver::PIT_LOOKAHEAD = 6.0;
// [m] Workaround for "broken" pitentries.
const double KDriver::PIT_BRAKE_AHEAD = 200.0;
// [-] Friction of pit concrete.
const double KDriver::PIT_MU = 0.4;
// [m/s] Speed to compute the percentage of brake to apply., 350 km/h
const double KDriver::MAX_SPEED = 350.0 / 3.6;
// [m/s]
const double KDriver::CLUTCH_SPEED = 5.0;
// [m] How far to look, terminate 'while'' loops.
const double KDriver::DISTCUTOFF = 400.0;
// [m] Increment faster if speed is slow [1.0..10.0].
const double KDriver::MAX_INC_FACTOR = 8.0;
// [-] select MIN(catchdist, dist*CATCH_FACTOR) to overtake.
const double KDriver::CATCH_FACTOR = 10.0;
// Reduce speed with this factor when being overlapped
const double KDriver::LET_OVERTAKE_FACTOR = 0.6;

// Static variables.
Cardata *KDriver::cardata_ = NULL;
double KDriver::current_sim_time_;
static char const *WheelSect[4] = { SECT_FRNTRGTWHEEL, SECT_FRNTLFTWHEEL,
                        SECT_REARRGTWHEEL, SECT_REARLFTWHEEL };
static int current_light = RM_LIGHT_HEAD1 | RM_LIGHT_HEAD2;

#define DEFAULTCARTYPE "nogood"   // "trb1-cavallo-360rb"

#define SLOW_TRACK_LIMIT 2.4
#define FAST_TRACK_LIMIT 4.0

#define START_TIME 2.0


KDriver::KDriver(int index)
  : mode_(NORMAL) {
  INDEX = index;
  forcePitStop = false;

}


KDriver::~KDriver() {
  delete raceline_;
  delete opponents_;
  delete pit_;
  delete strategy_;
  if (cardata_ != NULL) {
    delete cardata_;
    cardata_ = NULL;
  }
}


/**
 * Drive during the race.
 * 
 * @param[in] s Situation provided by the sim.
 */
void KDriver::drive(tSituation * s) {
  /*
   * Update
   * if isStuck
   *  Unstuck
   * else
   *  Drive
   */
  memset(&car_->ctrl, 0, sizeof(tCarCtrl));
  Update(s);
  // Situation-dependent light commands
  car_->_lightCmd = current_light;

  if (IsStuck()) {
    Unstuck();
  } else {
    car_->_steerCmd = (tdble) GetSteer(s);
    car_->_gearCmd = GetGear();
    CalcSpeed();
    car_->_brakeCmd = (tdble) (FilterABS(
                        FilterBrakeSpeed(
                          FilterBColl(
                            FilterBPit(
                              GetBrake())))));

    if (car_->_brakeCmd == 0.0) {
      car_->_accelCmd = (tdble) (FilterAccel(
                          FilterTCL(
                            FilterTrk(
                              FilterOverlap(
                                GetAccel())))));
    } else {
      car_->_accelCmd = 0.0;
    }
    car_->_clutchCmd = (tdble) (GetClutch());
  }  // if IsStuck

#if 0
  snprintf(car_->_msgCmd[0], RM_MSG_LEN, "%s", sMsg.c_str());
  memcpy(car_->_msgColorCmd, colour, sizeof(car_->_msgColorCmd));
#endif

  last_steer_ = car_->_steerCmd;
  last_accel_ = car_->_accelCmd;
  last_mode_ = mode_;
}  // drive


void KDriver::endRace(tSituation * s) {
  // Nothing for now.
}  // endRace


/**
 * Checks if I'm stuck.
 * 
 * @return true if stuck
 */
bool KDriver::IsStuck() {
  bool ret = false;

  if (fabs(my_cardata_->getCarAngle()) > MAX_UNSTUCK_ANGLE
            && car_->_speed_x < MAX_UNSTUCK_SPEED
            && fabs(car_->_trkPos.toMiddle) > MIN_UNSTUCK_DIST) {
    if (stuck_counter_ > MAX_UNSTUCK_COUNT
        && car_->_trkPos.toMiddle * my_cardata_->getCarAngle() < 0.0) {
        ret = true;
    } else {
      stuck_counter_++;
    }
  } else {
    stuck_counter_ = 0;
  }

  return ret;
}  // IsStuck


/**
 * Reduces the brake value such that it fits the speed
 * (more downforce -> more braking).
 * 
 * @param[in] brake Original braking value
 * @return  Modified braking value
 */
double KDriver::FilterBrakeSpeed(double brake) {
  double weight = mass() * G;
  double maxForce = weight + CA * pow(MAX_SPEED, 2);
  double force = weight + CA * current_speed_sqr();
  return brake * force / maxForce;
}  // FilterBrakeSpeed


/**
 * Compute offset to normal target point
 * for overtaking or let pass an opponent.
 * 
 * @return  new offset. Equals member 'my_offset_'
 */
double KDriver::GetOffset() {
  min_catch_dist_ = 500.0;
  Opponent *o = NULL;

  my_offset_ = car_->_trkPos.toMiddle;
  avoid_mode_ = 0;
  avoid_lft_offset_ = MAX(my_offset_,   car_->_trkPos.seg->width / 2.0 - 1.5);
  avoid_rgt_offset_ = MIN(my_offset_, -(car_->_trkPos.seg->width / 2.0 - 1.5));

  // Increment speed dependent.
  r_inverse_ = raceline_->rinverse();
  double incspeed = MIN(60.0, MAX(45.0, speed())) - 18.0;
  double incfactor = (MAX_INC_FACTOR - MIN(fabs(incspeed) / MAX_INC_FACTOR,
                                            (MAX_INC_FACTOR - 1.0)))
                      * (12.0 + MAX(0.0, (CA-1.9) * 14));
  rgt_inc_ = incfactor * MIN(1.3, MAX(0.4,
                    1.0 + r_inverse_ * (r_inverse_ < 0.0 ? 20 : 80)));
  lft_inc_ = incfactor * MIN(1.3, MAX(0.4,
                    1.0 - r_inverse_ * (r_inverse_ > 0.0 ? 20 : 80)));

  int offlft = my_offset_ > car_->_trkPos.seg->width / 2 - 1.0;
  int offrgt = my_offset_ < -(car_->_trkPos.seg->width / 2 - 1.0);

  if (offlft) {
    my_offset_ -= OVERTAKE_OFFSET_INC * rgt_inc_ / 2;
  } else if (offrgt) {
    my_offset_ += OVERTAKE_OFFSET_INC * lft_inc_ / 2;
  }

  avoid_lft_offset_ = MAX(avoid_lft_offset_,
                          my_offset_ - OVERTAKE_OFFSET_INC * rgt_inc_
                          * (offlft ? 6 : 2));
  avoid_rgt_offset_ = MIN(avoid_rgt_offset_,
                          my_offset_ + OVERTAKE_OFFSET_INC * lft_inc_
                          * (offrgt ? 6 : 2));

  // limit to the left
  max_offset_ = track_->width / 2 - car_->_dimension_y;
  // limit to the right
  // min_offset_ = -(track_->width / 2 - car_->_dimension_y);
  min_offset_ = -max_offset_;

  if (my_offset_ < min_offset_) {
    // we're already outside right limit, bring us back towards track
    min_offset_ = my_offset_ + OVERTAKE_OFFSET_INC * lft_inc_;
    max_offset_ = MIN(max_offset_,
                        my_offset_ + OVERTAKE_OFFSET_INC * lft_inc_ * 2);
  } else if (my_offset_ > max_offset_) {
    // outside left limit, bring us back
    max_offset_ = my_offset_ - OVERTAKE_OFFSET_INC * rgt_inc_;
    min_offset_ = MAX(min_offset_,
                        my_offset_ - OVERTAKE_OFFSET_INC * rgt_inc_ * 2);
  } else {
    // set tighter limits based on how far we're allowed to move
    max_offset_ = MIN(max_offset_,
                        my_offset_ + OVERTAKE_OFFSET_INC * lft_inc_ * 2);
    min_offset_ = MAX(min_offset_,
                        my_offset_ - OVERTAKE_OFFSET_INC * rgt_inc_ * 2);
  }

  // Check for side collision
  o = opponents_->GetSidecollOpp(car_);
  if (o != NULL) {
    SetMode(AVOIDING);
    return FilterSidecollOffset(o, incfactor);
  }


  // If we have someone to take over, let's try it
  o = GetTakeoverOpp();
  if (o != NULL)
    return FilterTakeoverOffset(o);


  // If there is someone overlapping, move out of the way
  o = opponents_->GetOverlappingOpp(car_);
  if (o != NULL)
    return FilterOverlappedOffset(o);


  // no-one to avoid, work back towards raceline
  if (sim_time_ > START_TIME * 4
      && mode_ != NORMAL
      && fabs(my_offset_ - race_offset_) > 1.0) {
    if (my_offset_ > race_offset_ + OVERTAKE_OFFSET_INC * rgt_inc_ / 4) {
      my_offset_ -= OVERTAKE_OFFSET_INC * rgt_inc_ / 4;
    } else if (my_offset_ < race_offset_ + OVERTAKE_OFFSET_INC * lft_inc_ / 4) {
      my_offset_ += OVERTAKE_OFFSET_INC * lft_inc_ / 4;
    }
  }  // if mode_

  if (sim_time_ > START_TIME) {
    if (my_offset_ > race_offset_) {
      my_offset_ = MAX(race_offset_,
                        my_offset_ - OVERTAKE_OFFSET_INC * incfactor / 2);
    } else {
      my_offset_ = MIN(race_offset_,
                        my_offset_ + OVERTAKE_OFFSET_INC * incfactor / 2);
    }
  }  // if sim_time_

  my_offset_ = MIN(max_offset_, MAX(min_offset_, my_offset_));
  return my_offset_;
}  // GetOffset


/**
 * Modifies the member 'my_offset_' so that the car moves out of the way
 * of the overlapping opponent.
 * 
 * @param [in] o: the opponent we should let go
 * @return    new offset. Equals member 'my_offset_'
 * 
 */
double KDriver::FilterOverlappedOffset(const Opponent *o) {
  double w = car_->_trkPos.seg->width / WIDTHDIV - BORDER_OVERTAKE_MARGIN;

  if (o->IsOnRight(car_->_trkPos.toMiddle)) {
    if (my_offset_ < w) {
      my_offset_ += OVERTAKE_OFFSET_INC * lft_inc_ / 1;  // 2;
    }
  } else {
    if (my_offset_ > -w) {
      my_offset_ -= OVERTAKE_OFFSET_INC * rgt_inc_ / 1;  // 2;
    }
  }
  SetMode(BEING_OVERLAPPED);

  my_offset_ = MIN(avoid_lft_offset_, MAX(avoid_rgt_offset_, my_offset_));
  return my_offset_;
}  // FilterOverlappedOffset


/** 
 * If there is an opponent overlapping us, reduce accelerator.
 *
 * @param [in]  accel: original acceleration value
 * @return      possibly reduced acceleration value
 */
double KDriver::FilterOverlap(double accel) {
  return (opponents_->GetOppByState(OPP_LETPASS)
    ? MIN(accel, LET_OVERTAKE_FACTOR)
    : accel);
}  // FilterOverlap


/**
 * Decide if there is a car ahead we can take over.
 * 
 * @return  Overlap car pointer or NULL
 */
Opponent* KDriver::GetTakeoverOpp() {
  Opponent *ret = NULL;

  min_catch_dist_ = MAX(30.0, 1500.0 - fabs(r_inverse_) * 10000);
  int otrySuccess = 0;

  for (int otry = 0; otry <= 1; otry++) {
    for (list<Opponent>::iterator it = opponents_->begin();
          it != opponents_->end();
          ++it) {
      tCarElt *ocar = it->car_ptr();

      // If opponent is clearly ahead of us, we don't care
      if (it->HasState(OPP_FRONT_FOLLOW))
        continue;

      if (it->IsTooFarOnSide(car_))
        continue;

      // If opponent is in pit, let him be ;)
      if (ocar->_state > RM_CAR_STATE_PIT)
        continue;

      // If opponent is ahead, and is not a quicker teammate of ours
      if ((it->HasState(OPP_FRONT))
          && !it->IsQuickerTeammate(car_)) {
        double otry_factor = otry
            ? (0.2 + (1.0 - ((current_sim_time_ - avoid_time_) / 7.0)) * 0.8)
            : 1.0;
        // How far ahead is he?
        double distance = it->distance() * otry_factor;
        double my_speed = MIN(avoid_speed_,
                            speed() + MAX(0.0, 10.0 - distance));
        double ospeed = it->speed();  // opponent's speed
        // When will we reach up to the opponent?
        double catchdist = MIN(my_speed * distance / (my_speed - ospeed),
                          distance * CATCH_FACTOR) * otry_factor;

        // If we are close enough,
        // check again with avoidance speed taken into account
        if (catchdist < min_catch_dist_
            && distance < fabs(my_speed - ospeed) * 2) {
          min_catch_dist_ = catchdist;
          ret = &(*it);  // This is the guy we need to take over
          otrySuccess = otry;
        }
      }  // if it state
    }  // for it
    if (ret) break;
    if (mode_ != AVOIDING) break;
  }  // for otry

  if (ret != NULL && otrySuccess == 0)
    avoid_time_ = current_sim_time_;

  return ret;
}  // GetTakeoverOpp


/**
 * Change offset value if we are to overtake a car.
 *
 * @param [in]  o the opponent
 * @return      new offset. Equals member 'my_offset_'
 */
double KDriver::FilterTakeoverOffset(const Opponent *o) {
  SetMode(AVOIDING);
  tCarElt *ocar = o->car_ptr();

  // Compute the opponent's distance to the middle.
  double otm = ocar->_trkPos.toMiddle;
  double sidemargin = o->width() + width() + SIDECOLL_MARGIN;
  double sidedist = fabs(ocar->_trkPos.toLeft - car_->_trkPos.toLeft);

  // Avoid more if on the outside of opponent on a bend.
  // Stops us from cutting in too much and colliding...
  if ((otm < -(ocar->_trkPos.seg->width - 5.0) && r_inverse_ < 0.0)
      || (otm > (ocar->_trkPos.seg->width - 5.0) && r_inverse_ > 0.0))
    sidemargin += fabs(r_inverse_) * 150;

  if (otm > (ocar->_trkPos.seg->width - 5.0)
      || (car_->_trkPos.toLeft > ocar->_trkPos.toLeft
          && (sidedist < sidemargin || o->HasState(OPP_COLL)))) {
    my_offset_ -= OVERTAKE_OFFSET_INC * rgt_inc_;
    SetAvoidLeft();
  } else if (otm < -(ocar->_trkPos.seg->width - 5.0)
              || (car_->_trkPos.toLeft < ocar->_trkPos.toLeft
                  && (sidedist < sidemargin || o->HasState(OPP_COLL)))) {
    my_offset_ += OVERTAKE_OFFSET_INC * lft_inc_;
    SetAvoidRight();
  } else {
    // If the opponent is near the middle we try to move the offset toward
    // the inside of the expected turn.
    // Try to find out the characteristic of the track up to catchdist.
    tTrackSeg *seg = car_->_trkPos.seg;
    double length = GetDistToSegEnd();
    double oldlen, seglen = length;
    double lenright = 0.0, lenleft = 0.0;
    min_catch_dist_ = MIN(min_catch_dist_, DISTCUTOFF);

    do {
      switch (seg->type) {
        case TR_LFT:
          lenleft += seglen;
          break;
        case TR_RGT:
          lenright += seglen;
          break;
        default:
          // Do nothing.
          break;
      }  // switch seg->type
      seg = seg->next;
      seglen = seg->length;
      oldlen = length;
      length += seglen;
    } while (oldlen < min_catch_dist_);

    // If we are on a straight look for the next turn.
    if (lenleft == 0.0 && lenright == 0.0) {
      while (seg->type == TR_STR)
        seg = seg->next;

      // Assume: left or right if not straight.
      if (seg->type == TR_LFT)
        lenleft = 1.0;
      else
        lenright = 1.0;
    }  // if lenleft/lenright == 0

    // Because we are inside we can go to the limit.
    if ((lenleft > lenright && r_inverse_ < 0.0)
         || (lenleft <= lenright && r_inverse_ > 0.0)) {
        // avoid more if on the outside of opponent on a bend.  Stops us
        // from cutting in too much and colliding...
        sidemargin += fabs(r_inverse_) * 150;
    }

    if (sidedist < sidemargin || o->HasState(OPP_COLL)) {
      if (lenleft > lenright) {
        my_offset_ += OVERTAKE_OFFSET_INC * lft_inc_;  // * 0.7;
        SetAvoidRight();
      } else {
        my_offset_ -= OVERTAKE_OFFSET_INC * rgt_inc_;  // * 0.7;
        SetAvoidLeft();
      }  // if lenleft > lenright
    }  // if sidedist
  }  // if opp near middle

  my_offset_ = MIN(avoid_lft_offset_, MAX(avoid_rgt_offset_, my_offset_));
  my_offset_ = MIN(max_offset_, MAX(min_offset_, my_offset_));
  return my_offset_;
}  // FilterTakeoverOffset


/**
 * Change offset value if we are at danger of side collision.
 *
 * @param [in]  o the opponent
 * @param [in]  incfactor
 * @return      new offset. Equals member 'my_offset_'
 */
double KDriver::FilterSidecollOffset(const Opponent *o,
                                      const double incfactor) {
  double myToLeft = car_->_trkPos.toLeft;
  double oppToLeft = o->car_ptr()->_trkPos.toLeft;
  double sidedist = fabs(oppToLeft - myToLeft);
  double sidemargin = o->width() + width() + SIDECOLL_MARGIN;
  bool oppOnRight = o->IsOnRight(car_->_trkPos.toMiddle);

  // Avoid more if on the outside of opponent on a bend.
  // Stops us from cutting in too much and colliding...
  if ((oppOnRight && r_inverse_ < 0.0)
      || (!oppOnRight && r_inverse_ > 0.0))
    sidemargin += fabs(r_inverse_) * 150;

  if (oppOnRight) {
    sidemargin -= MIN(0.0, r_inverse_ * 100);
  } else {
    sidemargin += MAX(0.0, r_inverse_ * 100);
  }
  sidedist = MIN(sidedist, sidemargin);

  if (sidedist < sidemargin) {
    double sdiff = 3.0 - (sidemargin - sidedist) / sidemargin;

    if (oppOnRight) {  // He is on the right, we must move to the left
      my_offset_ += OVERTAKE_OFFSET_INC * lft_inc_ * MAX(0.2, MIN(1.0, sdiff));
    } else {           // He is on the left, we must move to the right
      my_offset_ -= OVERTAKE_OFFSET_INC * rgt_inc_ * MAX(0.2, MIN(1.0, sdiff));
    }
  } else if (sidedist > sidemargin + 3.0) {
    if (race_offset_ > my_offset_ + OVERTAKE_OFFSET_INC * incfactor) {
      my_offset_ += OVERTAKE_OFFSET_INC * lft_inc_ / 4;
    } else if (race_offset_ < my_offset_ - OVERTAKE_OFFSET_INC * incfactor) {
      my_offset_ -= OVERTAKE_OFFSET_INC * rgt_inc_ / 4;
    }
  }

  oppOnRight ? SetAvoidRight() : SetAvoidLeft();

  my_offset_ = MIN(max_offset_, MAX(min_offset_, my_offset_));
  return my_offset_;
}  // FilterSidecollOffset


/**
 * Initialize the robot on a track.
 * For this reason it looks up any setup files.
 * 
 * Setup files are in a directory path like:
 * drivers/kilo
 *          |- default.xml  (skill enable)
 *          tracks
 *          | |-<trackname>.xml   (track-specific parameters)
 *          |
 *          |
 *          <carname>
 *            |-<trackname>.xml   (setup for the given track)
 *            |-def-slow.xml      (setup for undefined, slow tracks)
 *            |-def-norm.xml      (setup for undefined, normal tracks)
 *            |-def-fast.xml      (setup for undefined, fast tracks)
 * 
 * @param [in]  t the track
 * @param [out] carHandle
 * @param [out] carParmHandle
 * @param [in]  s Situation provided by the sim
 */
void KDriver::initTrack(tTrack * t, void *carHandle,
                        void **carParmHandle, tSituation * s) {
  InitSkill(s);   // Read & calculate skilling info

  stringstream buf;

  // Load a custom setup if one is available.
  track_ = t;
  // Ptr to the track filename
  char *trackname = strrchr(track_->filename, '/') + 1;
  stringstream botPath;

  // Try to load the default setup
  botPath << "drivers/" << bot << "/";  // << INDEX << "/";
  buf << botPath.str() << "default.xml";
  *carParmHandle = GfParmReadFile(buf.str().c_str(), GFPARM_RMODE_STD);
#ifdef KDRIVER_DEBUG
  GfLogInfo("KILO Default: %s\n", buf.str().c_str());
  if (carParmHandle)
    GfLogDebug("KILO default xml loaded\n");
#endif

  // Try to load the track-based informations
  buf.str(string());
  buf << botPath.str() << "tracks/" << trackname;
  void *newhandle = GfParmReadFile(buf.str().c_str(), GFPARM_RMODE_STD);
#ifdef KDRIVER_DEBUG
  GfLogInfo("KILO track-based: %s\n", buf.str().c_str());
  if (newhandle)
    GfLogDebug("KILO track-based XML loaded\n");
  else
    GfLogDebug("KILO no track-based XML present\n");
#endif

  MergeCarSetups(carParmHandle, newhandle);

  // Discover somehow the name of the car used
  if (car_type_.empty()) {
    stringstream ssSection;
    ssSection << ROB_SECT_ROBOTS << "/" << ROB_LIST_INDEX << "/" << INDEX;
    car_type_ = GfParmGetStr(carHandle, ssSection.str().c_str(),
        ROB_ATTR_CAR, DEFAULTCARTYPE);

    // Fallback mechanism
    if (car_type_ == DEFAULTCARTYPE) {
      char indexstr[32];
      RtGetCarindexString(INDEX, "kilo2008", true, indexstr, 32);
      car_type_ = indexstr;
      }
    }  // if carType empty
#ifdef KDRIVER_DEBUG
  GfLogInfo("KILO car type: %s\n", car_type_.c_str());
#endif

  // Load setup tailored for car+track
  buf.str(string());
  buf << botPath.str() << car_type_ << "/" << trackname;
  newhandle = GfParmReadFile(buf.str().c_str(), GFPARM_RMODE_STD);
#ifdef KDRIVER_DEBUG
  GfLogInfo("KILO custom setup: %s\n", buf.str().c_str());
  if (newhandle)
    GfLogDebug("KILO car+track XML loaded\n");
  else
    GfLogDebug("KILO no car+track XML present\n");
#endif

  // If there is no tailored setup, let's load a default one
  // based on the track charateristics.
  if (!newhandle)
    newhandle = LoadDefaultSetup();
#ifdef KDRIVER_DEBUG
  if (newhandle)
    GfLogDebug("KILO default setup XML loaded\n");
  else
    GfLogError("KILO no default setup loaded???\n");
#endif

  // Merge the above two setups
  MergeCarSetups(carParmHandle, newhandle);

  // Load and set parameters.
  MU_FACTOR = GfParmGetNum(*carParmHandle, KILO_SECT_PRIV,
                            KILO_ATT_MUFACTOR, NULL, 0.69f);
  pit_offset_ = GfParmGetNum(*carParmHandle, KILO_SECT_PRIV,
                            KILO_ATT_PITOFFSET, NULL, 10.0);
  brake_delay_ = GfParmGetNum(*carParmHandle, KILO_SECT_PRIV,
                            KILO_ATT_BRDELAY, NULL, 10.0);
  forcePitStop = GfParmGetNum(*carParmHandle, KILO_SECT_PRIV,
                            KILO_FORCE_PITSTOP, NULL, 0) == 1;

  // Create a pit stop strategy object & initialize fuel.
  strategy_ = new KStrategy();
  strategy_->SetFuelAtRaceStart(t, carParmHandle, s, INDEX);

  // Create a raceline object & initialize it for this track
  raceline_ = new LRaceLine;
  raceline_->InitTrack(track_, carParmHandle, s, side_skill_);
}  // InitTrack


/**
 * Update own private data on every timestep.
 * 
 * @param [in]  s situation provided by the sim
 */
void KDriver::Update(tSituation * s) {
  // Update global car data (shared by all instances) just once per timestep.
  if (current_sim_time_ != s->currentTime) {
    current_sim_time_ = s->currentTime;
    cardata_->update();
  }

  // Update the rest of local data
  speed_angle_ = -(my_cardata_->getTrackangle() -
                    atan2(car_->_speed_Y, car_->_speed_X));
  NORM_PI_PI(speed_angle_);

  opponents_->Update(s, this);
  strategy_->Update();

  CheckPitStatus(s);
  pit_->Update();
  sim_time_ = s->currentTime;

  double trackangle = RtTrackSideTgAngleL(&(car_->_trkPos));
  angle_ = trackangle - car_->_yaw;
  NORM_PI_PI(angle_);
  angle_ = -angle_;
}  // Update


/**
 * Checks if we need to plan a pitstop.
 * If yes, checks availability of the pit,
 * is it free or occupied by teammate.
 * 
 * @param [in]  s Situation provided by the sim
 */
void KDriver::CheckPitStatus(tSituation *s) {
  if (car_->_state <= RM_CAR_STATE_PIT) {  // If our car is still racing
    if (!pit_->pit_planned()) {  // if no pit is planned yet
      // and we are not in the pit range
      // or we are very low on fuel
      // then we can check if pitting is needed by strategy.
      if ((car_->_distFromStartLine < pit_->n_entry()
                || car_->_distFromStartLine > pit_->n_end())
                || car_->_fuel < 5.0) {
          pit_->set_pitstop(strategy_->NeedPitstop() || forcePitStop);
      }
    }  // if no pit planned

    if (pit_->pit_planned() && car_->_pit) {  // if pitting is planned
      pitstatus[car_index_] = 1;  // flag our car's pit as occupied

      for (list<Opponent>::iterator it = opponents_->begin();
            it != opponents_->end();
            ++it) {
        tCarElt *ocar = it->car_ptr();
        // If the other car is our teammate, still in the race
        if (it->teammate() && ocar->_state <= RM_CAR_STATE_PIT) {
          int idx = it->index();
          if (pitstatus[idx] == 1
              || ((pitstatus[idx]
                  || (ocar-> _fuel < car_->_fuel - 1.0
                      && car_->_dammage < 5000))
                  && fabs(car_->_trkPos.toMiddle)
                      <= car_->_trkPos.seg->width / 2)) {
            pit_->set_pitstop(false);
            pitstatus[car_index_] = 0;
          }
          break;
        }  // if our teammate
      }  // for it
    } else if (!pit_->in_pitlane()) {  // If we are not in the pitlane
      pitstatus[car_index_] = 0;  // sign the pit as free
    } else {
      pitstatus[car_index_] = 0;
    }
  }
}  // CheckPitStatus


/**
 * Brake filter for collision avoidance.
 * If there is an opponent we are to collide, brake brake brake!
 * 
 * @param [in]  brake Original brake value
 * @return  Possibly modified brake value
 */
double KDriver::FilterBColl(const double brake) {
  double ret = brake;

  if (sim_time_ >= START_TIME) {
    double mu = car_->_trkPos.seg->surface->kFriction;
    Opponent *o = opponents_->GetOppByState(OPP_COLL);
    if (o != NULL) {  // Endangered species nearby
//      if (BrakeDist(o->speed(), mu)
      if (BrakeDist(o->speed(), mu)
          + MIN(1.0, 0.5 + MAX(0.0, (speed() - o->speed()) / 4.0))
          > o->distance()) {  // Damn, too close, brake hard!!!
        accel_cmd_ = 0.0;
        ret = 1.0;
      }   // if BrakeDist
    }   // if o
  }   // if sim_time_

  return ret;
}  // FilterBColl


/**
 * Set pitstop commands.
 * 
 * @param [in]  s Situation provided by the sim
 * @return        Request pit immediately
 */
int KDriver::pitCommand(tSituation * s) {
  car_->_pitRepair = strategy_->PitRepair();
  car_->_pitFuel = (tdble) (strategy_->PitRefuel());
  // This should be the only place where the pit stop is set to false!
  pit_->set_pitstop(false);
  return ROB_PIT_IM;        // return immediately.
}  // pitCommand


/**
 * Initialize a new race.
 * 
 * @param [in]  car Our own car
 * @param [in]  s Situation provided by the sim
 */
void KDriver::newRace(tCarElt * car, tSituation * s) {
  strategy_->set_car(car);

  double deltaTime = static_cast<double>(RCM_MAX_DT_ROBOTS);
  MAX_UNSTUCK_COUNT = static_cast<int>(UNSTUCK_TIME_LIMIT / deltaTime);
  OVERTAKE_OFFSET_INC = OVERTAKE_OFFSET_SPEED * deltaTime;
  stuck_counter_ = 0;
  clutch_time_ = 0.0;
  old_lookahead_ = last_steer_ = last_nsa_steer_ = 0.0;
  last_accel_ = 0.0;
  car_ = car;
  CARMASS = GfParmGetNum(car_->_carHandle, SECT_CAR,
                          PRM_MASS, NULL, 1000.0);
  rgt_inc_ = lft_inc_ = 0.0;
  min_offset_ = max_offset_ = 0.0;
  r_inverse_ = 0.0;
  my_offset_ = 0.0;
  sim_time_ = correct_timer_ = 0.0;
  correct_limit_ = 1000.0;
  InitCa();
  InitCw();
  InitTireMu();
  InitTCLFilter();

  // Create just one instance of cardata shared by all drivers.
  if (cardata_ == NULL)
    cardata_ = new Cardata(s);
  my_cardata_ = cardata_->findCar(car_);
  current_sim_time_ = s->currentTime;

  // initialize the list of opponents.
  opponents_ = new Opponents(s, this, cardata_);
  opponents_->SetTeamMate(car_);

  // create the pit object.
  pit_ = new Pit(s, this, pit_offset_);

  // set initial mode
  // we set it to CORRECTING so the robot will steer towards the raceline
  SetMode(CORRECTING);
  last_mode_ = CORRECTING;

  // search for this car's index
  for (car_index_ = 0; car_index_ < s->_ncars; ++car_index_) {
    if (s->cars[car_index_] == car_)
      break;
  }

  raceline_->set_car(car_);
  raceline_->NewRace();
}  // newRace


/**
 * Calculates target speed dependent on the traffic situation.
 * 
 */
void KDriver::CalcSpeed() {
  accel_cmd_ = brake_cmd_ = 0.0;
  double speed;

  switch (mode_) {
    case AVOIDING:
    case BEING_OVERLAPPED:
      speed = avoid_speed_;
      break;

    case CORRECTING:
      speed = race_speed_ -
              (race_speed_ - avoid_speed_)
                * MAX(0.0, (correct_timer_ - sim_time_) / 7.0);
      break;

    default:
      speed = race_speed_;
  }  // switch mode_

  double x = (10 + car_->_speed_x) * (speed - car_->_speed_x) / 200;

  // Let's see if we must accelerate or brake a bit.
  if (x > 0.0)
    accel_cmd_ = x;
  else
    brake_cmd_ = MIN(1.0, -(MAX(10.0, brake_delay_ * 0.7)) * x);
}  // CalcSpeed


/**
 * Decides the character of the track and chooses 1 of 3 default setups.
 * Loads the appropriate setup file and returns it's handler.
 * 
 * @return  Handler to the loaded default setup
 */
void* KDriver::LoadDefaultSetup() const {
  void *ret = NULL;

  double dLength = 0.0;
  double dCurves = 0.0;

  // Count length and degrees of all turns
  tTrackSeg *pSeg = track_->seg;
  do {
    if (pSeg->type == TR_STR) {
      dLength += pSeg->length;
    } else {
      dLength += pSeg->radius * pSeg->arc;
      dCurves += RAD2DEG(pSeg->arc);
    }

    pSeg = pSeg->next;
  } while (pSeg != track_->seg);

  double dRatio = dLength / dCurves;

#ifdef KDRIVER_DEBUG
  GfLogInfo("KILO Track %s, length %.2f, curves %.2f, ratio %.2f\n",
    track_->name, dLength, dCurves, dRatio);
#endif

  stringstream buf;
  buf << "drivers/" << bot << "/" << car_type_;

  if (dRatio < SLOW_TRACK_LIMIT) {
    // Slow track
    buf << "/def-slow.xml";
  } else if (dRatio < FAST_TRACK_LIMIT) {
    // Normal track
    buf << "/def-norm.xml";
  } else {
    // Fast track
    buf << "/def-fast.xml";
  }  // if dRatio

  ret = GfParmReadFile(buf.str().c_str(), GFPARM_RMODE_STD);
#ifdef KDRIVER_DEBUG
  GfLogInfo("KILO Decision of setup: %s\n", buf.str().c_str());
  if (ret)
    GfLogDebug("KILO Def-XXX xml loaded\n");
#endif

  return ret;
}  // LoadDefaultSetup


/**
 * Merges 2 car setup infos, possibly read from different setup files.
 * 
 * @param [in,out]  oldHandle setup #1, output as the merged setups.
 * @param [in]      newHandle setup #2
 */
void KDriver::MergeCarSetups(void **oldHandle, void *newHandle) const {
  if (newHandle) {
    if (*oldHandle)
      *oldHandle = GfParmMergeHandles(*oldHandle, newHandle,
          (GFPARM_MMODE_SRC | GFPARM_MMODE_DST |
          GFPARM_MMODE_RELSRC | GFPARM_MMODE_RELDST));
    // Otherwise no need to merge
    else
      *oldHandle = newHandle;
  }  // if newHandle
}  // MergeCarSetups


/**
 * Computes CA (aerodynamic downforce coefficient).
 * 
 */
void KDriver::InitCa() {
  double rear_wing_area =
    GfParmGetNum(car_->_carHandle, SECT_REARWING, PRM_WINGAREA, NULL, 0.0);
  double rear_wing_angle =
    GfParmGetNum(car_->_carHandle, SECT_REARWING, PRM_WINGANGLE, NULL, 0.0);
  double front_wing_area =
    GfParmGetNum(car_->_carHandle, SECT_FRNTWING, PRM_WINGAREA, NULL, 0.0);
  double front_wing_angle =
    GfParmGetNum(car_->_carHandle, SECT_FRNTWING, PRM_WINGANGLE, NULL, 0.0);

  front_wing_area *= sin(front_wing_angle);
  rear_wing_area *= sin(rear_wing_angle);
  double wing_ca = 1.23 * (front_wing_area + rear_wing_area);

  double cl =
    GfParmGetNum(car_->_carHandle, SECT_AERODYNAMICS, PRM_FCL, NULL, 0.0) +
    GfParmGetNum(car_->_carHandle, SECT_AERODYNAMICS, PRM_RCL, NULL, 0.0);

  double h = 0.0;
  for (int i = 0; i < 4; ++i)
    h += GfParmGetNum(car_->_carHandle, WheelSect[i],
                        PRM_RIDEHEIGHT, NULL, 0.2f);
  h *= 1.5;
  h = pow(h, 4);
  h = 2.0 * exp(-3.0 * h);
  CA = h * cl + 4.0 * wing_ca;
  // TODO(kilo): ground effect (h * cl)
}  // InitCa


/**
 * Computes CW (aerodynamic drag coefficient).
 * 
 */
void KDriver::InitCw() {
  double cx = GfParmGetNum(car_->_carHandle, SECT_AERODYNAMICS,
                            PRM_CX, NULL, 0.0);
  double front_area = GfParmGetNum(car_->_carHandle, SECT_AERODYNAMICS,
                                  PRM_FRNTAREA, NULL, 0.0);

  CW = 0.645 * cx * front_area;
}  // InitCw


/**
 * Init the friction coefficient of the tires.
 * 
 */
void KDriver::InitTireMu() {
  double tm = DBL_MAX;
  for (int i = 0; i < 4; i++)
    tm = MIN(tm, GfParmGetNum(car_->_carHandle, WheelSect[i],
                                PRM_MU, NULL, 1.0));

  TIREMU = tm;
}  // InitTireMu


// Traction Control (TCL) setup.
void KDriver::InitTCLFilter() {
  const string traintype = GfParmGetStr(car_->_carHandle,
                                        SECT_DRIVETRAIN, PRM_TYPE,
                                        VAL_TRANS_RWD);

  if (traintype == VAL_TRANS_RWD)
    GET_DRIVEN_WHEEL_SPEED = &KDriver::FilterTCL_RWD;
  else if (traintype == VAL_TRANS_FWD)
    GET_DRIVEN_WHEEL_SPEED = &KDriver::FilterTCL_FWD;
  else if (traintype == VAL_TRANS_4WD)
    GET_DRIVEN_WHEEL_SPEED = &KDriver::FilterTCL_4WD;
}  // InitTCLFilter


// TCL filter plugin for rear wheel driven cars.
double KDriver::FilterTCL_RWD() {
  return (car_->_wheelSpinVel(REAR_RGT) + car_->_wheelSpinVel(REAR_LFT)) *
    car_->_wheelRadius(REAR_LFT) / 2.0;
}


// TCL filter plugin for front wheel driven cars.
double KDriver::FilterTCL_FWD() {
  return (car_->_wheelSpinVel(FRNT_RGT) + car_->_wheelSpinVel(FRNT_LFT)) *
    car_->_wheelRadius(FRNT_LFT) / 2.0;
}


// TCL filter plugin for all wheel driven cars.
double KDriver::FilterTCL_4WD() {
  return ((car_->_wheelSpinVel(FRNT_RGT) + car_->_wheelSpinVel(FRNT_LFT)) *
      car_->_wheelRadius(FRNT_LFT) +
      (car_->_wheelSpinVel(REAR_RGT) + car_->_wheelSpinVel(REAR_LFT)) *
      car_->_wheelRadius(REAR_LFT)) / 4.0;
}


// TCL filter for accelerator pedal.
double KDriver::FilterTCL(const double accel) {
  double ret = accel;

  if (sim_time_ >= START_TIME) {
    ret = MIN(1.0, accel);
    double accel1 = ret, accel2 = ret, accel3 = ret;

    if (car_->_speed_x > 10.0) {
      tTrackSeg *seg = car_->_trkPos.seg;
      tTrackSeg *wseg0 = car_->_wheelSeg(0);
      tTrackSeg *wseg1 = car_->_wheelSeg(1);
      int count = 0;

      if (
        wseg0->surface->kRoughness > MAX(0.02, seg->surface->kRoughness * 1.2)
        || wseg0->surface->kFriction < seg->surface->kFriction * 0.8
        || wseg0->surface->kRollRes > MAX(0.005, seg->surface->kRollRes * 1.2))
        count++;

      if (
        wseg1->surface->kRoughness > MAX(0.02, seg->surface->kRoughness * 1.2)
        || wseg1->surface->kFriction < seg->surface->kFriction * 0.8
        || wseg1->surface->kRollRes > MAX(0.005, seg->surface->kRollRes * 1.2))
        count++;

      if (count) {
        if (mode_ != NORMAL
          && ((seg->type == TR_RGT && seg->radius <= 200.0
            && car_->_trkPos.toLeft < 3.0)
            || (seg->type == TR_LFT && seg->radius <= 200.0
            && car_->_trkPos.toRight < 3.0)))
          count++;

          accel1 = MAX(0.0, MIN(accel1, (1.0 - (0.25 * count)) -
                            MAX(0.0, (speed() - car_->_speed_x) / 10.0)));
          }  // if count

      if (fabs(angle_) > 1.0)
        accel1 = MIN(accel1, 1.0 - (fabs(angle_) - 1.0) * 1.3);
      }  // if car_->_speed_x

    if (fabs(car_->_steerCmd) > 0.02) {
      double decel = ((fabs(car_->_steerCmd) - 0.02) *
          (1.0 + fabs(car_->_steerCmd)) * 0.7);
      accel2 = MIN(accel2, MAX(0.45, 1.0 - decel));
    }  // if car_->_steerCmd

    double slip = (this->*GET_DRIVEN_WHEEL_SPEED) () - car_->_speed_x;
    if (slip > TCL_SLIP)
      accel3 = accel3 - MIN(accel3, (slip - TCL_SLIP) / TCL_RANGE);

    ret = MIN(accel1, MIN(accel2, accel3));
  }  // if sim_time_

  return ret;
}  // FilterTCL


// Hold car on the track.
double KDriver::FilterTrk(double accel) {
  double ret = accel;

  tTrackSeg *seg = car_->_trkPos.seg;

  if (car_->_speed_x >= MAX_UNSTUCK_SPEED  // Not too slow
        && !pit_->in_pitlane()  // Not on pit stop
        && car_->_trkPos.toMiddle * -speed_angle_ <= 0.0) {
    // Speedvector points to the outside of the turn
    if (seg->type == TR_STR) {
      double tm = fabs(car_->_trkPos.toMiddle);
      double w = (seg->width - car_->_dimension_y) / 2.0;
      ret = (tm > w) ? 0.0 : accel;
    } else {
      double sign = (seg->type == TR_RGT) ? -1.0 : 1.0;
      if (car_->_trkPos.toMiddle * sign > 0.0) {
        ret = accel;
      } else {
        double tm = fabs(car_->_trkPos.toMiddle);
        double w = seg->width / WIDTHDIV;
        ret = (tm > w) ? 0.0 : accel;
      }
    }  // if seg->type
  }  // if not too slow
  return ret;
}  // FilterTrk


// Compute the needed distance to brake.
double KDriver::BrakeDist(double allowedspeed, double mu) const {
  double c = mu * G;
  double d = (CA * mu + CW) / mass();
  double v1sqr = current_speed_sqr();
  double v2sqr = pow(allowedspeed, 2);
  double ret = -log((c + v2sqr * d) / (c + v1sqr * d)) / (2.0 * d);
  ret /= brake_skill_;  // skilling
  return ret;
}  // BrakeDist


// Antilocking filter for brakes.
double KDriver::FilterABS(double brake) {
  double ret = brake;

  if (car_->_speed_x >= ABS_MINSPEED) {
    double origbrake = brake;
    double rearskid = MAX(0.0, MAX(car_->_skid[2], car_->_skid[3]) -
                    MAX(car_->_skid[0], car_->_skid[1]));

    double slip = 0.0;
    for (int i = 0; i < 4; i++)
      slip += car_->_wheelSpinVel(i) * car_->_wheelRadius(i);

    slip *=
      1.0 + MAX(rearskid, MAX(fabs(car_->_yaw_rate) / 5,
                                fabs(angle_) / 6));
    slip = car_->_speed_x - slip / 4.0;

    if (slip > ABS_SLIP)
      ret = brake - MIN(brake, (slip - ABS_SLIP) / ABS_RANGE);

    ret = MAX(ret, MIN(origbrake, 0.1f));
  }

  return ret;
}  // FilterABS


// Brake filter for pit stop.
double KDriver::FilterBPit(double brake) {
  double mu = car_->_trkPos.seg->surface->kFriction * TIREMU * PIT_MU;

  if (pit_->pit_planned() && !pit_->in_pitlane()) {
    tdble dl, dw;
    RtDistToPit(car_, track_, &dl, &dw);
    if (dl < PIT_BRAKE_AHEAD) {
//      if (BrakeDist(0.0, mu) > dl)
      if (BrakeDist(0.0, 0.5f * mu) > dl)
        return 1.0;
    }
  }

  if (pit_->in_pitlane()) {
    double s = pit_->ToSplineCoord(car_->_distFromStartLine);

    // Pit entry.
    if (pit_->pit_planned()) {
      if (s < pit_->n_start()) {
        // Brake to pit speed limit.
        double dist = pit_->n_start() - s;
        if (BrakeDist(pit_->speed_limit(), mu) > dist)
          return 1.0;
      } else {
        // Hold speed limit.
        if (current_speed_sqr() > pit_->speed_limit_sqr())
          return pit_->speed_limit_brake(current_speed_sqr());
      }  // if s

      // Brake into pit (speed limit 0.0 to stop)
      double dist = pit_->n_loc() - s;
      if (pit_->is_timeout(dist)) {
        pit_->set_pitstop(false);
        return 0.0;
      } else {
        if (BrakeDist(0.0, mu) > dist)
          return 1.0;
        else if (s > pit_->n_loc())
          return 1.0;  // Stop in the pit.
      }  // if pit timeout
    } else {
      // Pit exit.
      if (s < pit_->n_end()) {
        // Pit speed limit.
        if (current_speed_sqr() > pit_->speed_limit_sqr())
          return pit_->speed_limit_brake(current_speed_sqr());
      }
    }
  }

  return brake;
}  // FilterBPit


double KDriver::CalcAvoidSteer(const double targetAngle) {
  double rearskid = MAX(0.0,
                        MAX(car_->_skid[2], car_->_skid[3])
                        - MAX(car_->_skid[0], car_->_skid[1]))
                        + MAX(car_->_skid[2], car_->_skid[3])
                          * fabs(angle_) * 0.9;

  double factor = (mode_ == CORRECTING ? 1.1f : 1.2f);
  double angle_correction = MAX(angle_,
                            MIN(0.0, angle_ / 2.0)
                              / MAX(15.0, 70.0 - car_->_speed_x) * factor);
  angle_correction *= angle_ < 0.0 ? 1.0 : -1.0;
  double steer = targetAngle - car_->_yaw + angle_correction;
  NORM_PI_PI(steer);

  if (car_->_speed_x > 10.0) {
    double speedsteer = (80.0 - MIN(70.0, MAX(40.0, speed()))) /
          ((185.0 * MIN(1.0, car_->_steerLock / 0.785)) +
          (185.0 * (MIN(1.3, MAX(1.0, 1.0 + rearskid))) - 185.0));
    if (fabs(steer) > speedsteer)
      steer = MAX(-speedsteer, MIN(speedsteer, steer));
  }

  steer /= car_->_steerLock;

  // smooth steering. check for separate function for this!
  if (mode_ != PITTING) {
    double minspeedfactor = (((105.0 -
        MAX(40.0, MIN(70.0, speed() + MAX(0.0, car_->_accel_x * 5.0))))
         / 300.0) * (5.0 + MAX(0.0, (CA-1.9) * 20.0)));
    double maxspeedfactor = minspeedfactor;
    double rInverse = raceline_->rinverse();

    // TODO(kilo): check for +/- rInverse!!!
    if (rInverse > 0.0) {
      minspeedfactor = MAX(minspeedfactor / 3, minspeedfactor - rInverse * 80);
      maxspeedfactor = MAX(maxspeedfactor / 3, maxspeedfactor + rInverse * 20);
    } else {
      minspeedfactor = MAX(minspeedfactor / 3, minspeedfactor + rInverse * 20);
      maxspeedfactor = MAX(maxspeedfactor / 3, maxspeedfactor - rInverse * 80);
    }  // if rInverse

    steer = MAX(last_nsa_steer_ - minspeedfactor,
                MIN(last_nsa_steer_ + maxspeedfactor, steer));
  }  // if mode != PITTING

  last_nsa_steer_ = steer;

  if (fabs(angle_) > fabs(speed_angle_)) {
    // steer into the skid
    double sa = MAX(-0.3, MIN(0.3, speed_angle_ / 3));
    double anglediff = (sa - angle_)
                        * (0.7 - MAX(0.0, MIN(0.3, car_->_accel_x / 100)));
    // anglediff += raceline_->rinverse() * 10;
    steer += anglediff * 0.7;
  }

  if (fabs(angle_) > 1.2) {
    steer = sign(steer);
  } else if (fabs(car_->_trkPos.toMiddle)
              - car_->_trkPos.seg->width / 2 > 2.0) {
    steer = MIN(1.0,
              MAX(-1.0,
                  steer * (1.0 + (fabs(car_->_trkPos.toMiddle)
                    - car_->_trkPos.seg->width / 2) / 14
                    + fabs(angle_) / 2)));
  }

  if (mode_ != PITTING) {
    // limit how far we can steer against raceline
    double limit = (90.0 - MAX(40.0, MIN(60.0, car_->_speed_x))) /
                                    (50 + fabs(angle_) * fabs(angle_) * 3);
    steer = MAX(race_steer_ - limit, MIN(race_steer_ + limit, steer));
  }

  return steer;
}  // CalcAvoidSteer


double KDriver::CorrectSteering(double avoidsteer, double racesteer) {
  if (sim_time_ < 15.0 && car_->_speed_x < 20.0)
    return avoidsteer;

  double steer = avoidsteer;
  // double accel = MIN(0.0, car_->_accel_x);
  double act_speed = MAX(50.0, speed());
  // double changelimit = MIN(1.0, raceline_->correctLimit());
  double changelimit = MIN(raceline_->CorrectLimit(),
        (((120.0 - speed()) / 6000)
        * (0.5 + MIN(fabs(avoidsteer), fabs(racesteer)) / 10)));

  // TODO(kilo): check if START_TIME is enough
  if (mode_ == CORRECTING /*&& sim_time_ > START_TIME*/) {
    // move steering towards racesteer...
    if (correct_limit_ < 900.0) {
      if (steer < racesteer) {
        steer = (correct_limit_ >= 0.0)
          ? racesteer
          : MIN(racesteer, MAX(steer, racesteer + correct_limit_));
      } else {
        steer = (correct_limit_ <= 0.0)
          ? racesteer
          : MAX(racesteer, MIN(steer, racesteer + correct_limit_));
      }   // if steer
    }   // if correct_limit_

    act_speed -= car_->_accel_x / 10;
    act_speed = MAX(55.0, MIN(150.0, act_speed + (pow(act_speed , 2) / 55.0)));
    double rInverse = raceline_->rinverse() *
          (car_->_accel_x < 0.0
          ? 1.0 + fabs(car_->_accel_x) / 10.0
          : 1.0);
    double correctspeed = 0.5;
    if ((rInverse > 0.0 && racesteer > steer)
        || (rInverse < 0.0 && racesteer < steer))
      correctspeed += rInverse * 110.0;

    steer = (racesteer > steer)
      ? MIN(racesteer, steer + changelimit)
      : MAX(racesteer, steer - changelimit);
    correct_limit_ = (steer - racesteer);  // * 1.08;
  }  // if mode=correcting

  return steer;
}  // CorrectSteering


// try to limit sudden changes in steering
// to avoid loss of control through oversteer.
double KDriver::SmoothSteering(double steercmd) {
  double speedfactor = (((60.0 -
       (MAX(40.0, MIN(70.0, speed() + MAX(0.0, car_->_accel_x * 5)))
        - 25)) / 300) * 2.5) / 0.585;
  // double rearskid = MAX(0.0,
  //                        MAX(car_->_skid[2], car_->_skid[3])
  //                        - MAX(car_->_skid[0], car_->_skid[1]));

  if (fabs(steercmd) < fabs(last_steer_)
      && fabs(steercmd) <= fabs(last_steer_ - steercmd))
    speedfactor *= 2;

  double lftspeedfactor = speedfactor - MIN(0.0f, car_->_yaw_rate / 10.0);
  double rgtspeedfactor = speedfactor + MAX(0.0f, car_->_yaw_rate / 10.0);

  steercmd = MAX(last_steer_ - rgtspeedfactor,
                        MIN(last_steer_ + lftspeedfactor, steercmd));
  return steercmd;
}  // SmoothSteering


/**
 * GetSteer
 * Computes steer value.
 * 
 * @param[in] s global situation
 * @return    steer value range -1...1
 */
double KDriver::GetSteer(tSituation *s) {
  double steer = 0.0;
  v2d    race_target;      // the 2d point the raceline is driving at.
  double look_ahead;       // how far ahead on the track we look for steering

  raceline_->GetRaceLineData(s, &race_target,
                              &race_speed_, &avoid_speed_,
                              &race_offset_, &look_ahead,
                              &race_steer_);
  vec2f target = TargetPoint();

  double targetAngle =
    atan2(target.y - car_->_pos_Y, target.x - car_->_pos_X);
  double avoidsteer = CalcAvoidSteer(targetAngle);

  if (mode_ == PITTING)
    return avoidsteer;

  targetAngle =
    atan2(race_target.y - car_->_pos_Y, race_target.x - car_->_pos_X);

  if (mode_ == AVOIDING &&
     (!avoid_mode_
      || (avoid_mode_ == AVOIDRIGHT
          && race_offset_ >= my_offset_
          && race_offset_ < avoid_lft_offset_)
      || (avoid_mode_ == AVOIDLEFT
          && race_offset_ <= my_offset_
          && race_offset_ > avoid_rgt_offset_))) {
    // we're avoiding, but trying to steer somewhere the raceline takes us.
    // hence we'll just correct towards the raceline instead.
    SetMode(CORRECTING);
  }

  bool angle_ok = fabs(angle_) < 0.2f && fabs(race_steer_) < 0.4f;
  bool steer_ok = (race_steer_ < last_steer_ + 0.05 &&
                    race_steer_ > last_steer_ - 0.05);
  // fabs(last_steer_ - race_steer_) < 0.05
  double skid = (car_->_skid[0] + car_->_skid[1]
                  + car_->_skid[2] + car_->_skid[3]) / 2;
  if (mode_ == CORRECTING
      && (last_mode_ == NORMAL
          || (angle_ok
              && (sim_time_ > 15.0 || car_->_speed_x > 20)
              && skid < 0.1
              && steer_ok
              && ((fabs(car_->_trkPos.toMiddle)
                < car_->_trkPos.seg->width / 2 - 1.0)
                || car_->_speed_x < 10.0)
              && raceline_->isOnLine()
              )  // angle_ok
          )  // last_mode
        ) {  // mode
    // we're CORRECTING & are now close enough to the raceline to
    // switch back to 'normal' mode...
    SetMode(NORMAL);
    steer = race_steer_;
  }

  if (mode_ == NORMAL) {
    // if (car_->_distRaced < 100.0 && car_->_pos == 1) {
      // steer = race_steer_;//race_steer_ * 0.5;
      // last_nsa_steer_ = race_steer_ * 0.8;
    // } else {
      steer = race_steer_;
      last_nsa_steer_ = race_steer_ * 0.8;
    // }
  } else {
    if (mode_ != CORRECTING) {
      correct_limit_ = 1000.0;
      correct_timer_ = sim_time_ + 7.0;
      steer = SmoothSteering(avoidsteer);
    } else {
      steer = SmoothSteering(CorrectSteering(avoidsteer, race_steer_));
    }  // mode != CORRECTING

    // TODO(kilo): Isnt the same limiting code used
    // in raceline::GetRacelinedata???
    if (fabs(angle_) >= 1.6)
      steer = sign(steer);
  }  // mode != NORMAL

#if 0
  fprintf(stderr,
      "%s %d: %c%c %.3f (a%.3f k%.3f) cl=%.3f ct=%.1f myof=%.3f->%.3f\n",
      car_->_name, car_->_dammage,
      (mode_ ==
       NORMAL ? 'n' : (mode_ ==
               AVOIDING ? 'a' : (mode_ ==
                         CORRECTING ? 'c' : 'p'))),
      (avoid_mode_ ==
       AVOIDLEFT ? 'l' : (avoid_mode_ ==
                  AVOIDRIGHT ? 'r' : (avoid_mode_ ==
                          (AVOIDLEFT +
                           AVOIDRIGHT) ? 'b' :
                          ' '))), steer,
      avoidsteer, race_steer_, correct_limit_, (correct_timer_ - sim_time_),
      car_->_trkPos.toMiddle, my_offset_);
  fflush(stderr);
#endif
  return steer;
}  // GetSteer


/**
 * TargetPoint
 * Computes target point for steering.
 * 
 * @return 2D coords of the target
 */
vec2f KDriver::TargetPoint() {
  double lookahead;

  if (pit_->in_pitlane()) {
    // To stop in the pit we need special lookahead values.
    lookahead = PIT_LOOKAHEAD;
    if (current_speed_sqr() > pit_->speed_limit_sqr())
      lookahead += car_->_speed_x * LOOKAHEAD_FACTOR;
  } else {
    // Usual lookahead.
    double my_speed = MAX(20.0, speed());  // + MAX(0.0, car_->_accel_x));
    lookahead = LOOKAHEAD_CONST * 1.2 + my_speed * 0.60;
    lookahead = MIN(lookahead, LOOKAHEAD_CONST + (pow(my_speed, 2) / 7 * 0.15));

    // Prevent "snap back" of lookahead on harsh braking.
    double cmplookahead = old_lookahead_
                          - car_->_speed_x * RCM_MAX_DT_ROBOTS;
    lookahead = MAX(cmplookahead, lookahead);
  }  // if in_pitlane

  lookahead *= lookahead_skill_;

  old_lookahead_ = lookahead;

  // Search for the segment containing the target point.
  tTrackSeg *seg = car_->_trkPos.seg;
  double length = GetDistToSegEnd();
  while (length < lookahead) {
    seg = seg->next;
    length += seg->length;
  }

  length = lookahead - length + seg->length;
  double fromstart = seg->lgfromstart + length;

  // Compute the target point.
  double offset = GetOffset();
  double pitoffset = pit_->CalcPitOffset(-100.0, fromstart);
  if ((pit_->pit_planned() || pit_->in_pitlane()) && pitoffset != -100.0) {
    SetMode(PITTING);
    offset = my_offset_ = pitoffset;
  } else if (mode_ == PITTING) {
    SetMode(CORRECTING);
  }

  vec2f s;
  if (mode_ != PITTING) {
    raceline_->GetPoint(offset, lookahead, &s);
    return s;
  }

  s.x = (tdble) ((seg->vertex[TR_SL].x + seg->vertex[TR_SR].x) / 2.0);
  s.y = (tdble) ((seg->vertex[TR_SL].y + seg->vertex[TR_SR].y) / 2.0);

  if (seg->type == TR_STR) {
    vec2f n((seg->vertex[TR_EL].x - seg->vertex[TR_ER].x) / seg->length,
      (seg->vertex[TR_EL].y - seg->vertex[TR_ER].y) / seg->length);
    n.normalize();
    vec2f d((seg->vertex[TR_EL].x - seg->vertex[TR_SL].x) / seg->length,
      (seg->vertex[TR_EL].y - seg->vertex[TR_SL].y) / seg->length);
    return (s + d * (float) length + static_cast<float>((float) offset) * n);
  } else {
    vec2f c(seg->center.x, seg->center.y);
    double arc = length / seg->radius;
    double arcsign = (seg->type == TR_RGT) ? -1.0 : 1.0;
    arc = arc * arcsign;
    s = s.rotate(c, (float) arc);

    vec2f n, t, rt;
    n = c - s;
    n.normalize();
    t = s + static_cast<float>(arcsign) * static_cast<float>(offset) * n;

    // TODO(kilo): unreachable code, mode_ != PITTING returns before
    if (mode_ != PITTING) {
      // bugfix - calculates target point a different way, thus
      // bypassing an error in the BT code that sometimes made
      // the target closer to the car than lookahead...
      raceline_->GetPoint(offset, lookahead, &rt);
      double dx = t.x - car_->_pos_X;
      double dy = t.y - car_->_pos_Y;
      double dist1 = Mag(dx, dy);
      dx = rt.x - car_->_pos_X;
      dy = rt.y - car_->_pos_Y;
      double dist2 = Mag(dx, dy);
      if (dist2 > dist1)
        t = rt;
    }  // if !PITTING

    return t;
  }  // if seg->type
}  // TargetPoint


/** 
 * GetDistToSegEnd
 * Computes the distance to the end of the segment.
 * 
 * @return distance in meter.
 */
inline double KDriver::GetDistToSegEnd() const {
  const tTrackSeg *seg = car_->_trkPos.seg;
  double ret = (seg->type == TR_STR)
    ? seg->length - car_->_trkPos.toStart  // simple when straight
    : (seg->arc - car_->_trkPos.toStart) * seg->radius;  // uhm, arc
  return ret;
}  // GetDistToSegEnd


void KDriver::SetMode(int newmode) {
  // if (car_->_distRaced < 30.0 && car_->_pos == 1)
    // newmode = NORMAL;
#ifdef KDRIVER_DEBUG
  GfLogInfo("KILO mode: %d newmode: %d\n", mode_, newmode);
#endif

  if (mode_ != newmode) {
    if (mode_ == NORMAL || mode_ == PITTING) {
      correct_timer_ = sim_time_ + 7.0;
      correct_limit_ = 1000.0;
    }
    mode_ = newmode;

    switch (newmode) {
      case PITTING:
        current_light = RM_LIGHT_HEAD2;
        break;
      case AVOIDING:
        if (static_cast<int>(current_sim_time_ * 2.0) % 2 == 0)
          current_light = RM_LIGHT_HEAD1 | RM_LIGHT_HEAD2;
        else
          current_light = RM_LIGHT_HEAD1;
        break;
      default:
        current_light = RM_LIGHT_HEAD1;
        break;
    }
  }  // mode_ != newmode
}  // SetMode


/** 
 * GetAccel
 * Computes fitting acceleration.
 * 
 * @return Acceleration scaled 0-1.
 */
double KDriver::GetAccel() {
  double ret = 1.0;

  if (car_->_gear > 0) {
    accel_cmd_ = MIN(1.0, accel_cmd_);
    if (fabs(angle_) > 0.8 && speed() > 10.0) {
      accel_cmd_ = MAX(0.0, MIN(accel_cmd_,
                                1.0 - speed() / 100.0 * fabs(angle_)));
    }
    // Even the most unskilled driver can push the throttle 100% at first gear
    accel_cmd_ *= (car_->_gear > 1) ? accel_skill_ : 1.0;
    ret = accel_cmd_;
  }  // if car_->_gear

  return ret;
}  // GetAccel


/**
 * GetBrake
 * Computes initial brake value.
 * 
 * @return brake scaled 0-1
 */
double KDriver::GetBrake() {
  double ret;

  if (car_->_speed_x < -MAX_UNSTUCK_SPEED) {
    ret = 1.0;  // Car drives backward, brake
  } else {
    brake_cmd_ *= brake_skill_;   // Brake earlier if less skilled.
    ret = brake_cmd_;   // Car drives forward, normal braking.
  }

  return ret;
}  // GetBrake


// Compute gear.
int KDriver::GetGear() {
  int ret = car_->_gear <= 0 ? 1 : car_->_gear;

  if (car_->_gear > 0) {
    double gr_up =
            car_->_gearRatio[car_->_gear + car_->_gearOffset];
    double omega = car_->_enginerpmRedLine / gr_up;
    double wr = car_->_wheelRadius(2);

    if (omega * wr * SHIFT < car_->_speed_x) {
      ret = car_->_gear + 1;
    } else {
      double gr_down =
            car_->_gearRatio[car_->_gear + car_->_gearOffset - 1];
      omega = car_->_enginerpmRedLine / gr_down;
      if (car_->_gear > 1
          && omega * wr * SHIFT > car_->_speed_x + SHIFT_MARGIN)
        ret = car_->_gear - 1;
    }
  }  // if gear > 0

  return ret;
}  // GetGear


// Compute the clutch value.
double KDriver::GetClutch() {
  // ???
  if (1 || car_->_gearCmd > 1) {
    double maxtime = MAX(0.06, 0.32 - (car_->_gearCmd / 65.0));
    if (car_->_gear != car_->_gearCmd)
      clutch_time_ = maxtime;
    if (clutch_time_ > 0.0)
      clutch_time_ -= (RCM_MAX_DT_ROBOTS *
         (0.02 + (car_->_gearCmd / 8.0)));
    return 2.0 * clutch_time_;
  } else {  // unreachable code???
    double drpm = car_->_enginerpm - car_->_enginerpmRedLine / 2.0;
    double ctlimit = 0.9;
    if (car_->_gearCmd > 1)
      ctlimit -= 0.15 + car_->_gearCmd / 13.0;
    clutch_time_ = MIN(ctlimit, clutch_time_);
    if (car_->_gear != car_->_gearCmd)
      clutch_time_ = 0.0;
    double clutcht = (ctlimit - clutch_time_) / ctlimit;
    if (car_->_gear == 1 && car_->_accelCmd > 0.0)
      clutch_time_ += RCM_MAX_DT_ROBOTS;

    if (car_->_gearCmd == 1 || drpm > 0) {
      double speedr;
      if (car_->_gearCmd == 1) {
        // Compute corresponding speed to engine rpm.
        double omega =
          car_->_enginerpmRedLine / car_->_gearRatio[car_->_gear +
                       car_->_gearOffset];
        double wr = car_->_wheelRadius(2);
        speedr = (CLUTCH_SPEED +
                  MAX(0.0, car_->_speed_x)) / fabs(wr * omega);
        double clutchr = MAX(0.0,
            (1.0 - speedr * 2.0 * drpm /
             car_->_enginerpmRedLine)) *
            (car_->_gearCmd ==
              1 ? 0.95 : (0.7 - (car_->_gearCmd) / 30.0));
        return MIN(clutcht, clutchr);
      } else {
        // For the reverse gear.
        clutch_time_ = 0.0;
        return 0.0;
      }
    } else {
      return clutcht;
    }
  }
}  // GetClutch


/** 
 * InitSkill
 * Reads the global and the driver-specific skill values.
 * 
 * @return The accumulated skill value
 */
double KDriver::InitSkill(tSituation * s) {
  double globalSkill = skill_ = 0.0;
  brake_skill_ = 1.0;
  accel_skill_ = 1.0;
  lookahead_skill_ = 1.0;
  side_skill_ = 1.0;

  if (s->_raceType != RM_TYPE_PRACTICE) {
    stringstream buf;
    // load the global skill level, range 0 - 10
    buf << GetLocalDir() << "config/raceman/extra/skill.xml";
    void *skillHandle = GfParmReadFile(buf.str().c_str(), GFPARM_RMODE_REREAD);
    if (!skillHandle) {
      buf.str(string());
      buf << GetDataDir() << "config/raceman/extra/skill.xml";
      skillHandle = GfParmReadFile(buf.str().c_str(), GFPARM_RMODE_REREAD);
    }  // if !skillHandle

    if (skillHandle) {
      globalSkill = GfParmGetNum(skillHandle, KILO_SECT_SKILL,
                                 KILO_SKILL_LEVEL, NULL, 0.0);
      globalSkill = MIN(10.0, MAX(0.0, globalSkill));
    }

    // load the driver skill level, range 0 - 1
    double driverSkill = 0.0;
    buf.str(string());
    buf << "drivers/" << bot << "/" << INDEX << "/skill.xml";
    skillHandle = GfParmReadFile(buf.str().c_str(), GFPARM_RMODE_STD);
    if (skillHandle) {
      driverSkill = GfParmGetNum(skillHandle, KILO_SECT_SKILL,
                                 KILO_SKILL_LEVEL, NULL, 0.0);
      // driver_aggression = GfParmGetNum(skillHandle, SECT_SKILL,
      //                                  KILO_SKILL_AGGRO, NULL, 0.0);
      driverSkill = MIN(1.0, MAX(0.0, driverSkill));
    }

    // limits: 0.0 - 24.0
    skill_ = (globalSkill + driverSkill * 2.0) * (1.0 + driverSkill);

    // Set up different handicap values
    brake_skill_ = MAX(0.85, 1.0 - skill_ / 24.0 * 0.15);
    accel_skill_ = MAX(0.80, 1.0 - skill_ / 24.0 * 0.20);
    lookahead_skill_ = 1.0 / (1.0 + skill_ / 24.0);
    side_skill_ = 1.0 + skill_ / 24.0;
  }   // if not practice

#ifdef KDRIVER_DEBUG
  GfLogDebug("Skill: %.2f, brake: %.2f, accel: %.2f, lookA: %.2f, sides: %.2f\n",
    skill_, brake_skill_, accel_skill_, lookahead_skill_, side_skill_);
#endif

  return skill_;
}   // InitSkill


void KDriver::Unstuck() {
  car_->_steerCmd = (tdble) (- my_cardata_->getCarAngle() / car_->_steerLock);
  car_->_gearCmd = -1;     // Reverse gear.
  car_->_accelCmd = 1.0;   // 100% accelerator pedal.
  car_->_brakeCmd = 0.0;   // No brakes.
  car_->_clutchCmd = 0.0;  // Full clutch (gearbox connected with engine).
}


/**
 * FilterAccel
 * Avoids too hard throttle surges.
 *
 * @param[in] accel Original throttle command
 * @return Limited throttle value
 */
double KDriver::FilterAccel(const double accel) {
  double ret = accel;
  if (accel > last_accel_ + 0.05)
    ret = MIN(1.0, last_accel_ + 0.05);
  return ret;
}   // FilterAccel

