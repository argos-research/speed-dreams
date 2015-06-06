/*
 *      opponent.cpp
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
 *      $Id: opponent.cpp 3115 2010-11-08 23:42:00Z kmetykog $
 * 
 */

#include "src/drivers/kilo2008/opponent.h"

#include <list>
#include <string>
#include <algorithm>

#include "src/drivers/kilo2008/kdriver.h"
#include "src/drivers/kilo2008/util.h"   // Between

using std::string;
using std::list;

// [m] distance on the track ahead to check other cars.
const double Opponent::FRONTCOLLDIST = 200.0;
// [m] distance on the track behind to check other cars.
const double Opponent::BACKCOLLDIST = -50.0;
// [m] safety margin.
const double Opponent::LENGTH_MARGIN = 1.0;
// [m] safety margin.
const double Opponent::SIDE_MARGIN = 1.0;
// [m] if the estimated distance is smaller, compute it more accurate
const double Opponent::EXACT_DIST = 12.0;
// [s]
const double Opponent::LAP_BACK_TIME_PENALTY = -30.0;
// [s] overlaptimer must reach this time before we let the opponent pass.
const double Opponent::OVERLAP_WAIT_TIME = 5.0;
// [m/s] avoid overlapping opponents to stuck behind me.
const double Opponent::SPEED_PASS_MARGIN = 5.0;
// When to change position in the team?
const int Opponent::TEAM_DAMAGE_CHANGE_LEAD = 800;

const double Opponents::TEAM_REAR_DIST = 50.0;


// Constructor
Opponent::Opponent(tCarElt *car, SingleCardata * cardata, int index) {
  car_ = car;
  cardata_ = cardata;
  index_ = index;
  teammate_ = false;
}


void Opponent::Update(tSituation *s, KDriver *driver) {
  // Init state of opponent to ignore.
  state_ = OPP_IGNORE;

  // If this car is out of the simulation or is in pits, ignore it.
  if ((car_->_state & RM_CAR_STATE_NO_SIMU)
      || (car_->_state & RM_CAR_STATE_PIT))
    return;

  // Updating distance along the middle.
  tCarElt *mycar = driver->car_ptr();
  tTrack *track = driver->track_ptr();
  double oppToStart = car_->_trkPos.seg->lgfromstart + DistToSegStart();
  distance_ = oppToStart - mycar->_distFromStartLine;
  if (distance_ > track->length / 2.0) {
    distance_ -= track->length;
  } else if (distance_ < -track->length / 2.0) {
    distance_ += track->length;
  }

  const double SIDECOLLDIST = MAX(car_->_dimension_x, mycar->_dimension_x);

  // Is opponent in relevant range BACKCOLLDIST..FRONTCOLLDIST?
  if (BetweenStrict(distance_, BACKCOLLDIST, FRONTCOLLDIST)) {
    // Is opponent aside?
    if (BetweenStrict(distance_, -SIDECOLLDIST, SIDECOLLDIST)) {
      state_ |= OPP_SIDE;
    }

    // Is opponent in front and slower?
    if (distance_ > SIDECOLLDIST && speed() <= driver->speed()) {
      state_ |= OPP_FRONT;

      if (IsQuickerTeammate(mycar))
        state_ |= OPP_FRONT_FOLLOW;

      distance_ -= SIDECOLLDIST;
      distance_ -= LENGTH_MARGIN;

      // If the distance is small we compute it more accurate.
      if (distance_ < EXACT_DIST) {
        straight2f carFrontLine(
                      mycar->_corner_x(FRNT_LFT),
                      mycar->_corner_y(FRNT_LFT),
                      mycar->_corner_x(FRNT_RGT) -
                      mycar->_corner_x(FRNT_LFT),
                      mycar->_corner_y(FRNT_RGT) -
                      mycar->_corner_y(FRNT_LFT));

        double mindist = FLT_MAX;
        for (int i = 0; i < 4; ++i) {
          vec2f corner(car_->_corner_x(i), car_->_corner_y(i));
          double dist = carFrontLine.dist(corner);
          mindist = MIN(dist, mindist);
        }
        distance_ = MIN(distance_, mindist);
      }  // if small distance

      double side_dist = car_->_trkPos.toMiddle - mycar->_trkPos.toMiddle;
      double cardist = fabs(side_dist) - fabs(width() / 2.0) -
                        mycar->_dimension_y / 2.0;
      if (cardist < SIDE_MARGIN)
        state_ |= OPP_COLL;
    } else if (distance_ < -SIDECOLLDIST
              && speed() > driver->speed() - SPEED_PASS_MARGIN) {
      // Is opponent behind and faster.
      state_ |= OPP_BACK;
      distance_ -= SIDECOLLDIST;
      distance_ -= LENGTH_MARGIN;
    } else if (distance_ > SIDECOLLDIST && speed() > driver->speed()) {
      // Opponent is in front and faster.
      state_ |= OPP_FRONT_FAST;
      if (IsQuickerTeammate(mycar))
        state_ |= OPP_FRONT_FOLLOW;
      distance_ -= SIDECOLLDIST;
      if (distance_ < 20.0 - (speed() - driver->speed()) * 4)
        state_ |= OPP_FRONT;
    }
  }

  // Check if we should let overtake the opponent.
  UpdateOverlapTimer(s, mycar);
}  // Update


// Compute the length to the start of the segment.
inline double Opponent::DistToSegStart() const {
  double ret = (car_->_trkPos.seg->type == TR_STR)
    ? car_->_trkPos.toStart
    : car_->_trkPos.toStart * car_->_trkPos.seg->radius;
  return ret;
}  // getDistToSegStart


// Update overlaptimers of opponents.
void Opponent::UpdateOverlapTimer(tSituation * const s, tCarElt * const mycar) {
  if ((car_->race.laps > mycar->race.laps) || IsQuickerTeammate(mycar)) {
    if (HasState(OPP_BACK | OPP_SIDE)) {
      overlap_timer_ += s->deltaTime;
    } else if (HasState(OPP_FRONT)) {
      overlap_timer_ = LAP_BACK_TIME_PENALTY;
    } else {
      if (overlap_timer_ > 0.0) {
        if (HasState(OPP_FRONT_FAST)) {
          overlap_timer_ = MIN(0.0, overlap_timer_);
        } else {
          overlap_timer_ -= s->deltaTime;
        }
      } else {
        overlap_timer_ += s->deltaTime;
      }
    }
  } else {
    overlap_timer_ = 0.0;
  }

  if (overlap_timer_ > OVERLAP_WAIT_TIME)
    state_ |= OPP_LETPASS;
}  // UpdateOverlapTimer


/** 
 * Constructor
 * Initializes the list of opponents.
 * Checks and doesn't store out own car as an opponent.
 * 
 * @param s Situation provided by TORCS
 * @param driver Our own robot
 * @param c Opponent car data
 */
Opponents::Opponents(tSituation *s, KDriver *driver, Cardata *c) {
  opps_ = new list<Opponent>;
  const tCarElt *ownCar = driver->car_ptr();

  // Step through all the cars
  for (int i = 0; i < s->_ncars; ++i) {
    // If it is not our own car
    if (s->cars[i] != ownCar) {
      // Create and set up new opponent
      Opponent opp(
            s->cars[i],   // car ptr
            c->findCar(s->cars[i]),   // car data ptr
            i);  // index
      opps_->push_back(opp);   // Store it in list
    }  // if s->cars[i]
  }  // for i
}  // Opponents::Opponents


/** 
 * Update
 * Makes every opponent update its own data.
 * 
 * @param   s   Situation provided by TORCS
 * @param   driver  Our own robot
 */
void Opponents::Update(tSituation *s, KDriver *driver) {
  // for_each(begin(), end(), update);
  for (list<Opponent>::iterator it = begin(); it != end(); ++it)
    it->Update(s, driver);
}  // Update


// for find()
inline bool operator==(const Opponent& o, const std::string s)
    { return !s.compare(o.car_ptr()->_name); }
/** 
 * SetTeamMate
 * Search the opponent list for our own teammate,
 * based on the teammate name given in the config as "teammate".
 * 
 * @param car Our own car, to read its config
 */
void Opponents::SetTeamMate(const tCarElt *car) {
  string teammate(
    GfParmGetStr(car->_paramsHandle, KILO_SECT_PRIV, KILO_ATT_TEAMMATE, ""));

  list<Opponent>::iterator found = find(begin(), end(), teammate);
  if (found != end())
    found->set_teammate();
}  // SetTeamMate


/** 
 * Searches the first opponent with the given state.
 *
 * @param [in]  state: we only care for an opponent in this state
 * @return      pointer to the opponent we've found, or NULL
 */
Opponent * Opponents::GetOppByState(const int state) {
  Opponent *ret = NULL;
  for (list<Opponent>::iterator it = begin(); it != end(); ++it) {
    if (it->HasState(state)) {
      ret = &(*it);
      break;
    }
  }  // for it
  return ret;
}  // GetOppByState


/**
 * Decide if there is a car on the side we are to collide with...
 *
 * @param[in] car Our own car 
 * @return  Side collision car pointer or NULL
 */
Opponent * Opponents::GetSidecollOpp(const tCarElt *car) {
  Opponent *ret = NULL;
  for (list<Opponent>::iterator it = begin(); it != end(); ++it) {
    tCarElt *ocar = it->car_ptr();

    if (ocar->_state > RM_CAR_STATE_PIT)  // Dont care for opponents in the pit
      continue;

    // if opp is too far on side
    if (it->IsTooFarOnSide(car))
      continue;

    if (it->HasState(OPP_SIDE)) {  // If opponent is on our side
      ret = &(*it);
      break;
    }  // if OPP_SIDE
  }  // for it

  return ret;
}  // GetSidecollOpp


/**
 * Decide if there is a car behind overlapping us.
 * 
 * A1) Teammate behind with more laps should overtake.
 * A2) Slipstreaming: the less damaged teammate is also allowed to pass
 *      if on the same lap.
 *      The position change happens when the damage difference is greater
 *      than TEAM_DAMAGE_CHANGE_LEAD.
 * B) Let other, overlapping opponents get by.

 * @return  overlapping car pointer or NULL
 */
Opponent * Opponents::GetOverlappingOpp(const tCarElt *car) {
  Opponent *ret = NULL;
  double mindist = -1000.0;

  for (list<Opponent>::iterator it = begin(); it != end(); ++it) {
    tCarElt *ocar = it->car_ptr();
    double oppDistance = it->distance();

    if ((  // If teammate has more laps under his belt,
      (it->teammate() && ocar->race.laps > car->race.laps)
        ||  // or teammate is less damaged, let him go
        it->IsQuickerTeammate(car))
        && (oppDistance > -TEAM_REAR_DIST)  // if close enough
        && (oppDistance < -car->_dimension_x)) {
      // Behind, larger distances are smaller ("more negative").
      if (oppDistance > mindist) {
        mindist = oppDistance;
        ret = &(*it);
      }
    } else if (it->HasState(OPP_LETPASS)) {
      // Behind, larger distances are smaller ("more negative").
      if (oppDistance > mindist) {
        mindist = oppDistance;
        ret = &(*it);
      }
    }  // else if
  }  // for i

  return ret;
}  // GetOverlappingOpp


/**
 * Checks if opponent is too much on either side of the track,
 * (doesn't occupy center part of the segment)
 * and whether we are 5+ metres far.
 * 
 * @param [in]  ocar  the opponent car
 * @return      true if the opp. is too far on either side
*/
bool Opponent::IsTooFarOnSide(const tCarElt *mycar) const {
  bool ret = false;
  if (fabs(car_->_trkPos.toMiddle) > mycar->_trkPos.seg->width / 2 + 3.0
      && fabs(mycar->_trkPos.toMiddle - car_->_trkPos.toMiddle) >= 5.0)
    ret = true;
  return ret;
}  // IsTooFarOnSide
