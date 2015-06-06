/*
 *      pit.cpp
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
 *      $Id: pit.cpp 4669 2012-04-07 12:48:22Z wdbee $
 * 
 */

#include "src/drivers/kilo2008/pit.h"

#include "src/drivers/kilo2008/kdriver.h"

// [m/s] safety margin to avoid pit speeding.
const double Pit::SPEED_LIMIT_MARGIN = 0.5;


Pit::Pit(const tSituation * s, KDriver * driver, const double pitoffset) {
  track_ = driver->track_ptr();
  car_ = driver->car_ptr();
  mypit_ = driver->car_ptr()->_pit;
  pitinfo_ = &track_->pits;
  pit_planned_ = in_pitlane_ = false;
  pit_timer_ = 0.0;

  if (mypit_ != NULL) {
    speed_limit_ = pitinfo_->speedLimit - SPEED_LIMIT_MARGIN;
    pit_speed_limit_sqr_ = pow(pitinfo_->speedLimit, 2);

    // Compute pit spline points along the track.
    points_[3].x = mypit_->pos.seg->lgfromstart + mypit_->pos.toStart;
//    points_[2].x = points_[3].x - pitinfo_->len;
    points_[2].x = points_[3].x - 1.5 * pitinfo_->len;
//    points_[4].x = points_[3].x + pitinfo_->len;
    points_[4].x = points_[3].x + 0.75 * pitinfo_->len;
    points_[0].x = pitinfo_->pitEntry->lgfromstart + pitoffset;
    points_[1].x = pitinfo_->pitStart->lgfromstart;
    // Use nPitSeg to respect the pit speed limit on Migrants e.a.
    points_[5].x = pitinfo_->pitStart->lgfromstart
                + pitinfo_->nPitSeg * pitinfo_->len;
    points_[6].x = pitinfo_->pitExit->lgfromstart;

    pit_entry_ = points_[0].x;
    pit_exit_ = points_[6].x;

    // Normalizing spline segments to >= 0.0.
    for (int i = 0; i < NPOINTS; i++) {
      points_[i].s = 0.0;
      points_[i].x = ToSplineCoord(points_[i].x);
    }  // for i

    // Fix broken pit exit.
    if (points_[6].x < points_[5].x)
      points_[6].x = points_[5].x + 50.0;

    // Fix point for first pit if necessary.
    if (points_[1].x > points_[2].x)
      points_[1].x = points_[2].x;

    // Fix point for last pit if necessary.
    if (points_[4].x > points_[5].x)
      points_[5].x = points_[4].x;

    double sign = (pitinfo_->side == TR_LFT) ? 1.0 : -1.0;
    points_[0].y = 0.0;
    points_[6].y = 0.0;
    for (int i = 1; i < NPOINTS - 1; i++) {
      points_[i].y = fabs(pitinfo_->driversPits->pos.toMiddle)
                      - pitinfo_->width;
      points_[i].y *= sign;
    }  // for i

//    points_[3].y = fabs(pitinfo_->driversPits->pos.toMiddle + 1.0) * sign;
    points_[3].y = fabs(pitinfo_->driversPits->pos.toMiddle + MIN(3.0,fabs(pitinfo_->width  - 0.5))) * sign;
    spline_ = new Spline(NPOINTS, points_);
  }  // if pit not null
}  // Pit::Pit


Pit::~Pit() {
  if (mypit_ != NULL)
    delete spline_;
}  // Pit::~Pit


// Transforms track coordinates to spline parameter coordinates.
double Pit::ToSplineCoord(const double x) const {
  double ret = x - pit_entry_;
  while (ret < 0.0)
    ret += track_->length;

  return ret;
}  // ToSplineCoord


// Computes offset to track middle for trajectory.
double Pit::CalcPitOffset(const double offset, double fromstart) {
  if (mypit_ != NULL) {
    if (in_pitlane() || (pit_planned() && is_between(fromstart))) {
      fromstart = ToSplineCoord(fromstart);
      return spline_->evaluate(fromstart);
    }
  }
  return offset;
}  // CalcPitOffset


// Sets the pitstop flag if we are not in the pit range.
void Pit::set_pitstop(bool pitstop) {
  if (mypit_ != NULL) {
    double fromstart = car_->_distFromStartLine;

    if (!is_between(fromstart)) {
      pit_planned_ = pitstop;
    } else {
      if (!pitstop) {
        pit_planned_ = pitstop;
        pit_timer_ = 0.0;
      }
    }
  }
}  // set_pitstop


// Check if the argument fromstart is in the range of the pit.
bool Pit::is_between(const double fromstart) const {
  bool ret = false;

  if (pit_entry_ <= pit_exit_) {
    if (fromstart >= pit_entry_ && fromstart <= pit_exit_) {
      ret = true;
    }
  } else {
    // Warning: TORCS reports sometimes negative values for "fromstart"!
    if (fromstart <= pit_exit_ || fromstart >= pit_entry_) {
      ret = true;
    }
  }  // if pitentry <= pitexit

  return ret;
}  // is_between


// Checks if we stay too long without getting captured by the pit.
// Distance is the distance to the pit along the track, when the pit is
// ahead it is > 0, if we overshoot the pit it is < 0.
bool Pit::is_timeout(const double distance) {
  bool ret = false;

  if (car_->_speed_x > 1.0 || distance > 3.0 || !pit_planned()) {
    pit_timer_ = 0.0;
  } else {
    pit_timer_ += RCM_MAX_DT_ROBOTS;
    if (pit_timer_ > 3.0) {
      pit_timer_ = 0.0;
      ret = true;
    }
  }

  return ret;
}  // is_timeout


// Update pit data and strategy.
void Pit::Update() {
  if (mypit_ != NULL) {
    if (is_between(car_->_distFromStartLine)) {
      if (pit_planned()) {
        in_pitlane_ = true;
      }
    } else {
      in_pitlane_ = false;
    }

    if (pit_planned())
      car_->_raceCmd = RM_CMD_PIT_ASKED;
  }
}  // Update
