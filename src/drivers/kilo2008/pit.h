/*
 *      pit.h
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
 *      $Id: pit.h 3115 2010-11-08 23:42:00Z kmetykog $
 * 
 */

#ifndef SRC_DRIVERS_KILO2008_PIT_H_
#define SRC_DRIVERS_KILO2008_PIT_H_

#include <raceman.h>    // tSituation
#include "src/drivers/kilo2008/spline.h"

class KDriver;

class Pit {
 public:
  Pit(const tSituation * s, KDriver * driver, const double PitOffset);
  ~Pit();

  inline bool pit_planned() const { return pit_planned_; }
  inline bool in_pitlane() const { return in_pitlane_; }
  inline double speed_limit() const { return speed_limit_; }
  inline double speed_limit_sqr() const { return pow(speed_limit_, 2); }

  inline double n_start() const { return points_[1].x; }
  inline double n_loc()   const { return points_[3].x; }
  inline double n_end()   const { return points_[5].x; }
  inline double n_entry() const { return points_[0].x; }

  inline double speed_limit_brake(const double speedsqr) const
    { return (speedsqr - speed_limit_sqr())
      / (pit_speed_limit_sqr_ - speed_limit_sqr()); }

  void set_pitstop(const bool pitstop);
  bool is_timeout(const double distance);
  double CalcPitOffset(const double offset, double fromstart);
  double ToSplineCoord(double x) const;
  void Update();

 private:
  bool is_between(const double fromstart) const;

  tTrack *track_;
  tCarElt *car_;
  tTrackOwnPit *mypit_;      // Pointer to my pit.
  tTrackPitInfo *pitinfo_;   // General pit info.

  enum { NPOINTS = 7 };
  SplinePoint points_[NPOINTS];   // Spline points.
  Spline *spline_;           // Spline.

  bool pit_planned_;          // Pitstop planned.
  bool in_pitlane_;           // We are still in the pit lane.
  double pit_entry_;          // Distance to start line of the pit entry.
  double pit_exit_;           // Distance to the start line of the pit exit.

  double speed_limit_;          // Pit speed limit.
  double pit_speed_limit_sqr_;  // The original speedlimit squared.

  double pit_timer_;          // Timer for pit timeouts.

  static const double SPEED_LIMIT_MARGIN;
};

#endif  // SRC_DRIVERS_KILO2008_PIT_H_
