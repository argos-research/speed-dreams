/*
 *      opponent.h
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
 *      $Id: opponent.h 3401 2011-02-27 10:36:03Z pouillot $
 * 
 */

#ifndef SRC_DRIVERS_KILO2008_OPPONENT_H_
#define SRC_DRIVERS_KILO2008_OPPONENT_H_

#include <car.h>        // tCarElt
#include <raceman.h>    // tSituation

#include <list>

#include "src/drivers/kilo2008/cardata.h"

class KDriver;

#define OPP_IGNORE          0
#define OPP_FRONT           (1<<0)
#define OPP_BACK            (1<<1)
#define OPP_SIDE            (1<<2)
#define OPP_COLL            (1<<3)
#define OPP_LETPASS         (1<<4)
#define OPP_FRONT_FAST      (1<<5)
#define OPP_FRONT_FOLLOW    (1<<6)


// Opponent maintains the data for one opponent RELATIVE to the driver's car.
class Opponent {
 public:
  Opponent(tCarElt *car, SingleCardata *cardata, int index);


  // Accessors
  inline tCarElt *car_ptr()  const { return car_; }
  inline double distance()   const { return distance_; }
  inline double width()      const { return cardata_->getWidthOnTrack(); }
  inline double speed() const { return cardata_->getSpeedInTrackDirection(); }
  inline int index()         const { return index_; }
  inline bool teammate()     const { return teammate_; }

  // State queries
  inline bool HasState(const int state) const
  { return (state_ & state) ? true : false; }
  inline bool IsQuickerTeammate(const tCarElt *mycar) const
        { return (teammate_
            && (mycar->_dammage - car_->_dammage > TEAM_DAMAGE_CHANGE_LEAD));}
  inline bool IsOnRight(const double dMiddle) const
        { return (dMiddle > car_->_trkPos.toMiddle) ? true : false; }
  bool IsTooFarOnSide(const tCarElt *mycar) const;

  // Mutators
  inline void set_teammate() {teammate_ = true;}
  void Update(tSituation *s, KDriver *driver);


 private:
  // distance minus opponent car length
  inline double brake_distance() const {return distance_ - car_->_dimension_x;}
  inline double DistToSegStart() const;
  void UpdateOverlapTimer(tSituation * const s, tCarElt * const mycar);

  // approximation of the real distance, negative if the opponent is behind.
  double distance_;

  int state_;   // State describes the relation to the opp, eg. opp is behind
  int index_;
  double overlap_timer_;
  tCarElt *car_;
  SingleCardata *cardata_;   // Pointer to global data about this opponent.
  bool teammate_;            // Is this opponent a team mate of me

  // constants.
  static const double FRONTCOLLDIST;
  static const double BACKCOLLDIST;
  static const double LENGTH_MARGIN;
  static const double SIDE_MARGIN;
  static const double EXACT_DIST;
  static const double LAP_BACK_TIME_PENALTY;
  static const double OVERLAP_WAIT_TIME;
  static const double SPEED_PASS_MARGIN;
  static const int TEAM_DAMAGE_CHANGE_LEAD;
};


// The Opponents class holds a list of all Opponents.
// Uses STL's list template.
class Opponents {
 public:
  Opponents(tSituation *s, KDriver *driver, Cardata *cardata);
  ~Opponents() { if (opps_ != NULL) delete opps_;}

  void Update(tSituation *s, KDriver *driver);
  void SetTeamMate(const tCarElt *car);
  Opponent *GetOppByState(const int state);
  Opponent *GetSidecollOpp(const tCarElt *car);
  Opponent *GetOverlappingOpp(const tCarElt *car);

  inline std::list<Opponent>::iterator begin() {return opps_->begin();}
  inline std::list<Opponent>::iterator end() {return opps_->end();}

 private:
  std::list<Opponent> *opps_;
  static const double TEAM_REAR_DIST;
};


#endif  // SRC_DRIVERS_KILO2008_OPPONENT_H_
