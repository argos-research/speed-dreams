/*
 *      raceline.h
 *      
 *      Copyright 2009 kilo aka Gabor Kmetyko <kg.kilo@gmail.com>
 *      Based on work by Bernhard Wymann, Andrew Sumner, Remi Coulom.
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
 *      $Id: raceline.h 3115 2010-11-08 23:42:00Z kmetykog $
 * 
 */

#ifndef SRC_DRIVERS_KILO2008_RACELINE_H_
#define SRC_DRIVERS_KILO2008_RACELINE_H_

#include <raceman.h>    // tSituation
#include <track.h>      // tTrack

#ifdef KILO_DEBUG
#include <iostream>     // ostream  // NOLINT(readability/streams) only debug
#endif
#include <vector>       // vector
#include <utility>      // pair

class v2d;

enum { LINE_MID = 0, LINE_RL };

class rlSegment {
 public:
  ~rlSegment() {}

  void UpdateTxTy(const int rl);
  friend void Nullify(rlSegment &d);  // NOLINT(runtime/references)
  // The above function is called in an STL for_each operation
#ifdef KILO_DEBUG
  friend std::ostream &operator<<(std::ostream &output, const rlSegment &s) {
    output << s.tx[0] << ' ' << s.tx[1] << ' '
        << s.ty[0] << ' ' << s.ty[1] << ' '
        << s.tz[0] << ' ' << s.tz[1] << ' '
        << s.tRInverse << ' ' << s.tMaxSpeed << ' '
        << s.tSpeed[0] << ' ' << s.tSpeed[1] << ' '
        << s.txLeft << ' ' << s.tyLeft << ' '
        << s.txRight << ' ' << s.tyRight << ' '
        << s.tLane << ' ' << s.tLaneLMargin << ' ' << s.tLaneRMargin << ' '
        << s.tFriction << ' ' << s.dCamber << std::endl;

    return output;
  }
#endif

 public:
  double tx[2];
  double ty[2];
  double tz[2];
  double tRInverse;
  double tMaxSpeed;
  double tSpeed[2];
  double txLeft;
  double tyLeft;
  double txRight;
  double tyRight;
  double tLane;
  double tLaneLMargin;
  double tLaneRMargin;
  double tFriction;
  double dCamber;
};


class LRaceLine {
 public:
  LRaceLine() {}
  virtual ~LRaceLine() {}

  inline double rinverse(void) const { return seg_[next_].tRInverse; }
  inline double rinverse(const double distance) const {
    int d = ((next_ + static_cast<int>(distance / div_length_)) % divs_);
    return seg_[d].tRInverse; }
  inline void set_car(tCarElt * const car) {car_ = car;}

  void InitTrack(const tTrack * const track, void **carParmHandle,
                  const tSituation *s, const double filterSideSkill);
  void NewRace();
  void GetRaceLineData(const tSituation * const s, v2d * target,
                double *speed, double *avspeed, double *raceoffset,
                double *lookahead, double *racesteer);
  void GetPoint(const double offset, const double lookahead,
                  vec2f * const rt) const;
  bool isOnLine(void) const;
  double CorrectLimit(void) const;

 private:
  double rinverse(const int prev, const double x, const double y,
                      const int next, const int rl) const;
  void SetSegmentInfo(const tTrackSeg * pseg, const int i, const double l);
  void SetSegmentCamber(const tTrackSeg *seg, const int div);
  void SplitTrack(const tTrack * const ptrack, const int rl,
                      const tSituation *s);
  void AdjustRadius(int prev, int i, int next, double TargetRInverse, int rl,
                      double Security = 0);
  void Smooth(const int Step, const int rl);
  void StepInterpolate(int iMin, int iMax, int Step, int rl);
  void Interpolate(int Step, int rl);

 private:
  tCarElt *car_;
  double min_corner_inverse_;
  double corner_speed_;
  double corner_accel_;
  double brake_delay_;
  double int_margin_;
  double ext_margin_;
  double avoid_speed_adjust_;
  double security_radius_;
  double wheel_base_;
  double wheel_track_;

  int divs_;
  int div_length_;
  double target_speed_;
  double width_;

  // first: segment index, second: elem length
  std::vector< std::pair<int, double> > seg_info_;

  std::vector<rlSegment> seg_;

  int next_;
  int this_;
};

#endif  // SRC_DRIVERS_KILO2008_RACELINE_H_
