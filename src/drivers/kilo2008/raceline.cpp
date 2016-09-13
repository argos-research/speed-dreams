/*
 *      raceline.cpp
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
 *      $Id: raceline.cpp 5880 2014-12-01 16:22:22Z wdbee $
 * 
 */

#include "src/drivers/kilo2008/raceline.h"

#include <robottools.h>   // Rt*

#include <utility>  // pair
#include <algorithm>      // for_each

#include "src/drivers/kilo2008/linalg.h"       // v2d
#include "src/drivers/kilo2008/util.h"         // Mag, sign, Between*
#include "src/drivers/kilo2008/kdriver.h"      // KILO_SECT...

////////////////////////////////////////////////////////////////////////////
// Parameters
////////////////////////////////////////////////////////////////////////////

//
// These parameters are for the computation of the path
//
static const int Iterations = 100;      // Number of smoothing operations

/////////////////////////////////////////////////////////////////////////////
// Update tx and ty arrays
/////////////////////////////////////////////////////////////////////////////
void rlSegment::UpdateTxTy(const int rl) {
  tx[rl] = tLane * txRight + (1 - tLane) * txLeft;
  ty[rl] = tLane * tyRight + (1 - tLane) * tyLeft;
}


static int g_rl;
void Nullify(rlSegment &d) {  // NOLINT(runtime/references) - non-const ref
  d.tx[g_rl] = 0.0;
  d.ty[g_rl] = 0.0;
  d.tz[g_rl] = 0.0;
  d.tRInverse = 0.0;
  d.tSpeed[g_rl] = 0.0;
  d.tMaxSpeed = 0.0;
  d.txRight = 0.0;
  d.tyRight = 0.0;
  d.txLeft = 0.0;
  d.tyLeft = 0.0;
  d.tLane = 0.0;
  d.tLaneLMargin = 0.0;
  d.tLaneRMargin = 0.0;
}


/////////////////////////////////////////////////////////////////////////////
// Set segment info
/////////////////////////////////////////////////////////////////////////////
void LRaceLine::SetSegmentInfo(const tTrackSeg * pseg,
                                const int i, const double l) {
  if (pseg) {
    std::pair<int, double> info(i, l);
    seg_info_.push_back(info);
  }
}


/////////////////////////////////////////////////////////////////////////////
// Split the track into small elements
// ??? constant width supposed
/////////////////////////////////////////////////////////////////////////////
void LRaceLine::SplitTrack(const tTrack * const ptrack,
                            const int rl,
                            const tSituation *s) {
  div_length_ = 3;   // Length of path elements in meters
  const tTrackSeg *psegCurrent = ptrack->seg;

  double dAngle = psegCurrent->angle[TR_ZS];
  double dXPos  = (psegCurrent->vertex[TR_SL].x
                    + psegCurrent->vertex[TR_SR].x) / 2;
  double dYPos  = (psegCurrent->vertex[TR_SL].y
                    + psegCurrent->vertex[TR_SR].y) / 2;

  // Determine pit start and end
  const tTrackPitInfo *pPits = &ptrack->pits;
  double dpitstart = 0.0;
  double dpitend = 0.0;
  if (pPits->type != TR_PIT_NONE) {
    dpitstart = pPits->pitEntry->lgfromstart - 50.0;
    dpitend = pPits->pitExit->lgfromstart + pPits->pitExit->length + 50.0;
    if (dpitstart > dpitend) {
      if (psegCurrent->lgfromstart >= dpitstart) {
        dpitend += ptrack->length;
      } else {
        dpitstart -= ptrack->length;
      }
    }  // if dpitstart > dpitend
  }  // if pits.type


  seg_info_.reserve(ptrack->nseg);
  unsigned int i = 0;
  do {
    int Divisions = static_cast<int>(psegCurrent->length / div_length_) + 1;
    double Step = psegCurrent->length / Divisions;

    SetSegmentInfo(psegCurrent, i, Step);

    // Take sides into account if they are useable
    double lft_margin = 0.0;
    double rgt_margin = 0.0;

    if (rl == LINE_RL) {
      for (int side = 0; side < 2; side++) {
        tTrackSeg *psegSide = psegCurrent->side[side];
        double dmargin = 0.0;

        while (psegSide) {
          // Avoid walls and fences
          if (psegSide->style == TR_WALL || psegSide->style == TR_FENCE) {
            dmargin = MAX(0.0,
                        dmargin - (psegCurrent->type == TR_STR ? 0.5 : 1.0));
          }

          // Avoid slippery, rough or not plain side surfaces
          if (  // psegSide->style != TR_PLAN ||
            psegSide->surface->kFriction <
              psegCurrent->surface->kFriction * 0.8f
            || psegSide->surface->kRoughness >
              MAX(0.02, psegCurrent->surface->kRoughness * 1.2)
            || psegSide->surface->kRollRes
              > MAX(0.005, psegCurrent->surface->kRollRes * 1.2)
            )
            break;

          // Avoid too high curbs
          if (psegSide->style == TR_CURB &&
            (psegSide->surface->kFriction >= psegCurrent->surface->kFriction * 0.9 &&
             psegSide->surface->kRoughness <= psegCurrent->surface->kRoughness + 0.05 &&
             psegSide->surface->kRollRes <= psegCurrent->surface->kRollRes * 0.03 &&
             psegSide->height <= psegSide->width / 10))
          break;

          // Do not use the side if it is the pitlane, grumpy stewards...
          if (pPits->type != TR_PIT_NONE) {
            // if we plan to use the side the pit is on, too
            if (((side == TR_SIDE_LFT && pPits->side == TR_LFT)
              || (side == TR_SIDE_RGT && pPits->side == TR_RGT))
              &&
              BetweenLoose(psegCurrent->lgfromstart, dpitstart, dpitend))
              break;
          }  // if pits.type

          // Phew, we CAN use the side
          // TODO(kilo): check not to leave the track completely
          //            if TRB rules change...
          double dSideWidth = MIN(psegSide->startWidth, psegSide->endWidth);
          if (psegSide->type == TR_STR) {
            if ((side == TR_SIDE_LFT
                  && (psegCurrent->type == TR_RGT
                  || psegCurrent->next->type != TR_LFT))
                || (side == TR_SIDE_RGT
                  && (psegCurrent->type == TR_LFT
                  || psegCurrent->next->type != TR_RGT))) {
              dSideWidth *= 0.6;
            }
          }

          dmargin += dSideWidth;
          psegSide = psegSide->side[side];
        }  // while psegSide

        dmargin = MAX(0.0, dmargin);
        if (dmargin > 0.0) {
          dmargin /= psegCurrent->width;
          if (side == TR_SIDE_LFT) {
            lft_margin += dmargin;
          } else {
            rgt_margin += dmargin;
          }
        }  // if dmargin
      }  // for side
    }  // if rl

    for (int j = Divisions; --j >= 0;) {
      double cosine = cos(dAngle);
      double sine = sin(dAngle);

      if (psegCurrent->type == TR_STR) {
        dXPos += cosine * Step;
        dYPos += sine * Step;
      } else {
        double r = psegCurrent->radius;
        double Theta = psegCurrent->arc / Divisions;
        double L = 2 * r * sin(Theta / 2);
        double x = L * cos(Theta / 2);
        double y;

        if (psegCurrent->type == TR_LFT) {
          dAngle += Theta;
          y = L * sin(Theta / 2);
        } else {
          dAngle -= Theta;
          y = -L * sin(Theta / 2);
        }

        dXPos += x * cosine - y * sine;
        dYPos += x * sine + y * cosine;
      }

      double dx = -psegCurrent->width * sin(dAngle) / 2;
      double dy = psegCurrent->width * cos(dAngle) / 2;

      if (seg_.size() <= i) {  // if it is the first run
        // Create & store new segment
        rlSegment *newSeg = new rlSegment;
        seg_.push_back(*newSeg);
        delete newSeg;
      }

      // We can be quite sure seg_[i] exists as we created it just above
      seg_[i].txLeft = dXPos + dx;
      seg_[i].tyLeft = dYPos + dy;
      seg_[i].txRight = dXPos - dx;
      seg_[i].tyRight = dYPos - dy;
      seg_[i].tLane = 0.5;
      seg_[i].tLaneLMargin = lft_margin;
      seg_[i].tLaneRMargin = rgt_margin;
      seg_[i].tFriction = psegCurrent->surface->kFriction;
      SetSegmentCamber(psegCurrent, i);
      // cerr << i << " " << psegCurrent->name << endl;

      // ??? ugly trick for dirt
      // if (seg_[i].tFriction < 1)
        // seg_[i].tFriction *= 0.90;

      seg_[i].UpdateTxTy(rl);

      i++;
    }  // for j

  psegCurrent = psegCurrent->next;
  } while (psegCurrent != ptrack->seg);

  divs_ = i - 1;  // seg_.size-1!
  width_ = psegCurrent->width;
}  // SplitTrack


/////////////////////////////////////////////////////////////////////////////
// Compute the inverse of the radius
/////////////////////////////////////////////////////////////////////////////
double LRaceLine::rinverse(const int prev,
                                const double x,
                                const double y,
                                const int next,
                                const int rl) const {
  // vector<rlSegment>::iterator itNext = seg_[next];
  double x1 = seg_[next].tx[rl] - x;
  double y1 = seg_[next].ty[rl] - y;
  double x2 = seg_[prev].tx[rl] - x;
  double y2 = seg_[prev].ty[rl] - y;
  double x3 = seg_[next].tx[rl] - seg_[prev].tx[rl];
  double y3 = seg_[next].ty[rl] - seg_[prev].ty[rl];

  double det = x1 * y2 - x2 * y1;
  double n1 = x1 * x1 + y1 * y1;
  double n2 = x2 * x2 + y2 * y2;
  double n3 = x3 * x3 + y3 * y3;
  double nnn = sqrt(n1 * n2 * n3);

  return 2 * det / nnn;
}  // rinverse


/////////////////////////////////////////////////////////////////////////////
// Change lane value to reach a given radius
/////////////////////////////////////////////////////////////////////////////
void LRaceLine::AdjustRadius(int prev, int i, int next,
                              double TargetRInverse, int rl,
                              double Security) {
  double OldLane = seg_[i].tLane;

  // Start by aligning points for a reasonable initial lane
  seg_[i].tLane = (-(seg_[next].ty[rl] - seg_[prev].ty[rl])
                    * (seg_[i].txLeft - seg_[prev].tx[rl])
                    + (seg_[next].tx[rl] - seg_[prev].tx[rl])
                    * (seg_[i].tyLeft - seg_[prev].ty[rl])) /
                    ((seg_[next].ty[rl] - seg_[prev].ty[rl])
                    * (seg_[i].txRight - seg_[i].txLeft)
                    - (seg_[next].tx[rl] - seg_[prev].tx[rl])
                    * (seg_[i].tyRight - seg_[i].tyLeft));

  if (rl == LINE_RL) {
    seg_[i].tLane = MAX(seg_[i].tLane, -1.2 - seg_[i].tLaneLMargin);
    seg_[i].tLane = MIN(seg_[i].tLane,  1.2 + seg_[i].tLaneRMargin);
  }  // if rl
  seg_[i].UpdateTxTy(rl);

  //
  // Newton-like resolution method
  //
  const double dLane = 0.0001;

  double dx = dLane * (seg_[i].txRight - seg_[i].txLeft);
  double dy = dLane * (seg_[i].tyRight - seg_[i].tyLeft);

  double dRInverse =
    rinverse(prev, seg_[i].tx[rl] + dx, seg_[i].ty[rl] + dy, next, rl);

  if (dRInverse > 0.000000001) {
    seg_[i].tLane += (dLane / dRInverse) * TargetRInverse;

    double ExtLane = MIN((ext_margin_ + Security) / width_, 0.5);
    double IntLane = MIN((int_margin_ + Security) / width_, 0.5);
    // ~ //kilo HACK E-track-1
    // ~ if(BetweenLoose(i, 1077, 1112)) //bus-stop
      // ~ {
        // ~ //ExtLane = MIN((ext_margin_ + 20.0) / width_, 0.5);
        // ~ //IntLane = MIN((int_margin_ + 0.0) / width_, 0.5);
        // ~ IntLane = 0.27;
      // ~ }
    // ~ //end kilo HACK
    // ~ //kilo HACK Alpine-2
    // ~ if(BetweenLoose(i, 1470, 1490)) //left-right bump after tunnel
      // ~ {
          // ExtLane = MIN((ext_margin_ + 20.0) / width_, 0.5);
          // IntLane = MIN((int_margin_ + 0.0) / width_, 0.5);
          // ExtLane = 4.0;
          // IntLane = 0.3;
      // ~ }
      // end kilo HACK

    if (rl == LINE_RL) {
      if (TargetRInverse >= 0.0) {
        IntLane -= seg_[i].tLaneLMargin;
        ExtLane -= seg_[i].tLaneRMargin;
      } else {
        ExtLane -= seg_[i].tLaneLMargin;
        IntLane -= seg_[i].tLaneRMargin;
      }
    }  // if rl

    if (TargetRInverse >= 0.0) {
      if (seg_[i].tLane < IntLane)
            seg_[i].tLane = IntLane;

      if (1 - seg_[i].tLane < ExtLane) {
        if (1 - OldLane < ExtLane) {
          seg_[i].tLane = MIN(OldLane, seg_[i].tLane);
        } else {
          seg_[i].tLane = 1 - ExtLane;
        }
      }
    } else {
      if (seg_[i].tLane < ExtLane) {
        if (OldLane < ExtLane) {
          seg_[i].tLane = MAX(OldLane, seg_[i].tLane);
        } else {
          seg_[i].tLane = ExtLane;
        }
      }

      if (1 - seg_[i].tLane < IntLane)
        seg_[i].tLane = 1 - IntLane;
    }
  }

  seg_[i].UpdateTxTy(rl);
}

/////////////////////////////////////////////////////////////////////////////
// Smooth path
/////////////////////////////////////////////////////////////////////////////
void LRaceLine::Smooth(const int Step, const int rl) {
  int prev = ((divs_ - Step) / Step) * Step;
  int prevprev = prev - Step;
  int next = Step;
  int nextnext = next + Step;

  for (int i = 0; i <= divs_ - Step; i += Step) {
    double ri0 = rinverse(prevprev, seg_[prev].tx[rl],
                              seg_[prev].ty[rl], i, rl);
    double ri1 = rinverse(i, seg_[next].tx[rl],
                              seg_[next].ty[rl], nextnext, rl);
    double lPrev = Mag(seg_[i].tx[rl] - seg_[prev].tx[rl],
                              seg_[i].ty[rl] - seg_[prev].ty[rl]);
    double lNext = Mag(seg_[i].tx[rl] - seg_[next].tx[rl],
                              seg_[i].ty[rl] - seg_[next].ty[rl]);

    double TargetRInverse = (lNext * ri0 + lPrev * ri1) / (lNext + lPrev);

    double Security = lPrev * lNext / (8 * security_radius_);

    if (rl == LINE_RL) {
      if (ri0 * ri1 > 0) {
        double ac1 = fabs(ri0);
        double ac2 = fabs(ri1);

        if (ac1 < ac2) {  // curve increasing
          ri0 += 0.11 * (ri1 - ri0);
        } else if (ac2 < ac1) {  // curve decreasing
          ri1 += 0.11 * (ri0 - ri1);
        }

        TargetRInverse = (lNext * ri0 + lPrev * ri1) / (lNext + lPrev);
      }
    }

    AdjustRadius(prev, i, next, TargetRInverse, rl, Security);

    prevprev = prev;
    prev = i;
    next = nextnext;
    nextnext = next + Step;
    if (nextnext > divs_ - Step)
      nextnext = 0;
  }
}  // Smooth


/////////////////////////////////////////////////////////////////////////////
// Interpolate between two control points
/////////////////////////////////////////////////////////////////////////////
void LRaceLine::StepInterpolate(int iMin, int iMax, int Step, int rl) {
  int next = (iMax + Step) % divs_;
  if (next > divs_ - Step)
    next = 0;

  int prev = (((divs_ + iMin - Step) % divs_) / Step) * Step;
  if (prev > divs_ - Step)
    prev -= Step;

  double ir0 = rinverse(prev, seg_[iMin].tx[rl],
                            seg_[iMin].ty[rl], iMax % divs_, rl);
  double ir1 = rinverse(iMin, seg_[iMax % divs_].tx[rl],
                            seg_[iMax % divs_].ty[rl], next, rl);
  double range = static_cast<double>(iMax - iMin);
  for (int k = iMax; --k > iMin;) {
    double x = static_cast<double>(k - iMin) / range;
    double TargetRInverse = x * ir1 + (1 - x) * ir0;
    AdjustRadius(iMin, k, iMax % divs_, TargetRInverse, rl);
  }
}


/////////////////////////////////////////////////////////////////////////////
// Calls to StepInterpolate for the full path
/////////////////////////////////////////////////////////////////////////////
void LRaceLine::Interpolate(int Step, int rl) {
  if (Step > 1) {
    int i;
    for (i = Step; i <= divs_ - Step; i += Step)
      StepInterpolate(i - Step, i, Step, rl);
    StepInterpolate(i - Step, divs_, Step, rl);
  }
}


void LRaceLine::InitTrack(const tTrack * const track, void **parm_handle,
    const tSituation *s, const double filterSideSkill) {
  min_corner_inverse_ = GfParmGetNum(*parm_handle, KILO_SECT_PRIV,
                                      KILO_ATT_MINCORNER, NULL, 0.002f);
  corner_speed_ = GfParmGetNum(*parm_handle, KILO_SECT_PRIV,
                                      KILO_ATT_CORNERSP, NULL, 15.0);
  avoid_speed_adjust_ = GfParmGetNum(*parm_handle, KILO_SECT_PRIV,
                                      KILO_ATT_AVOIDSP, NULL, 2.0);
  corner_accel_ = GfParmGetNum(*parm_handle, KILO_SECT_PRIV,
                                      KILO_ATT_CORNERACC, NULL, 1.0);
  int_margin_ = GfParmGetNum(*parm_handle, KILO_SECT_PRIV,
                                      KILO_ATT_INTMARG, NULL, 0.5);
  ext_margin_ = GfParmGetNum(*parm_handle, KILO_SECT_PRIV,
                                      KILO_ATT_EXTMARG, NULL, 1.0);
  brake_delay_ = GfParmGetNum(*parm_handle, KILO_SECT_PRIV,
                                      KILO_ATT_BRDELAY, NULL, 10.0);
  security_radius_ = GfParmGetNum(*parm_handle, KILO_SECT_PRIV,
                                      KILO_ATT_SECRAD, NULL, 100.0);

  if (s->_raceType != RM_TYPE_PRACTICE) {
    ext_margin_ *= filterSideSkill;
    int_margin_ *= filterSideSkill;
  }  // if not practice

  // split track
  for (int rl = LINE_MID; rl <= LINE_RL; rl++) {
    g_rl = rl;
    std::for_each(seg_.begin(), seg_.end(), Nullify);

    SplitTrack(track, rl, s);

    // Smoothing loop
    int Iter = (rl == LINE_MID ? Iterations / 4 : Iterations);
    for (int Step = 128; (Step /= 2) > 0;) {
      for (int i = Iter * static_cast<int>(sqrt(static_cast<double>(Step)));
          --i >= 0; ) {
        Smooth(Step, rl);
      }
      Interpolate(Step, rl);
    }

    // Compute curvature and speed along the path
    for (int i = divs_; --i >= 0;) {
      double TireAccel = corner_speed_ * seg_[i].tFriction;
      if (rl == LINE_MID)
        TireAccel += avoid_speed_adjust_;
      int next = (i + 1) % divs_;
      int prev = (i - 1 + divs_) % divs_;

      double rInverse = rinverse(prev,
                                    seg_[i].tx[rl],
                                    seg_[i].ty[rl],
                                    next,
                                    rl);
      seg_[i].tRInverse = rInverse;

      double MaxSpeed = 10000.0;
      double dAbsInverse = fabs(rInverse);
      if (dAbsInverse > min_corner_inverse_ * 1.01)
        MaxSpeed = sqrt(TireAccel / (dAbsInverse - min_corner_inverse_));

      // Increase or decrease speed depending on track camber
      if (dAbsInverse > 0.002) {
        double camber = seg_[i].dCamber;

        if (camber < -0.02) {  // bad camber. slow us down.
          MaxSpeed -= MIN(MaxSpeed/4, fabs(camber) * 20);
        } else if (camber > 0.02) {  // good camber, speed us up.
          MaxSpeed += camber * 10;
        }
      }  // if dAbsInverse

      // TODO(kilo): increase or decrease speed depending on track slope

      // TODO(kilo): increase or decrease speed depending on approaching bumps

      seg_[i].tSpeed[rl] = seg_[i].tMaxSpeed = MaxSpeed;
    }  // for i

    // Anticipate braking
    for (int j = 32; --j >= 0;) {
      for (int i = divs_; --i >= 0;) {
        double TireAccel = corner_speed_ * seg_[i].tFriction;
        int prev = (i - 1 + divs_) % divs_;

        double dx = seg_[i].tx[rl] - seg_[prev].tx[rl];
        double dy = seg_[i].ty[rl] - seg_[prev].ty[rl];

        double dist = Mag(dx, dy);
        double Speed = (seg_[i].tSpeed[rl] + seg_[prev].tSpeed[rl]) / 2;

        double LatA = seg_[i].tSpeed[rl] * seg_[i].tSpeed[rl] *
            (fabs(seg_[prev].tRInverse) + fabs(seg_[i].tRInverse)) / 2;

        double TanA = TireAccel * TireAccel +
            min_corner_inverse_ * Speed * Speed - LatA * LatA;
        double brakedelay =
            brake_delay_ + (rl == LINE_MID ? avoid_speed_adjust_ : 0.0);
        TanA = MAX(TanA, 0.0);
        TanA = MIN(TanA, brakedelay * seg_[i].tFriction);

        double Time = dist / Speed;
        double MaxSpeed = seg_[i].tSpeed[rl] + TanA * Time;
        seg_[prev].tSpeed[rl] = MIN(MaxSpeed, seg_[prev].tMaxSpeed);
      }  // for i
    }  // for j
  }  // for rl
}  // InitTrack


////////////////////////////////////////////////////////////////////////////
// New race
////////////////////////////////////////////////////////////////////////////
void LRaceLine::NewRace() {
  const tPrivCar * const cp = &car_->priv;
  wheel_base_ = (cp->wheel[FRNT_RGT].relPos.x +
                cp->wheel[FRNT_LFT].relPos.x -
                cp->wheel[REAR_RGT].relPos.x -
                cp->wheel[REAR_LFT].relPos.x) / 2;

  wheel_track_ = (cp->wheel[FRNT_LFT].relPos.y +
                cp->wheel[REAR_LFT].relPos.y -
                cp->wheel[FRNT_RGT].relPos.y -
                cp->wheel[REAR_RGT].relPos.y) / 2;
}


////////////////////////////////////////////////////////////////////////////
// Car control
////////////////////////////////////////////////////////////////////////////
void
LRaceLine::GetRaceLineData(const tSituation * const s,
                v2d *target,
                double *speed,
                double *avspeed,
                double *raceoffset,
                double *lookahead,
                double *racesteer) {
  // Find index in data arrays
  tTrackSeg *pSeg = car_->_trkPos.seg;
  double dist = MAX(car_->_trkPos.toStart, 0.0);
  if (pSeg->type != TR_STR)
    dist *= pSeg->radius;
  int Index = seg_info_[pSeg->id].first
              + static_cast<int>(dist / seg_info_[pSeg->id].second);
  this_ = Index;

  Index = (Index + divs_ - 5) % divs_;
  static const double Time = s->deltaTime * 9 + corner_accel_ / 80.0;
  // ??? 0.50 + corner_accel_/80;
  double X = car_->_pos_X + car_->_speed_X * Time / 2;
  double Y = car_->_pos_Y + car_->_speed_Y * Time / 2;
  // *lookahead = 0.0;

  while (1) {
    next_ = (Index + 1) % divs_;
    double dx = seg_[next_].tx[LINE_RL] - car_->_pos_X;
    double dy = seg_[next_].ty[LINE_RL] - car_->_pos_Y;
    *lookahead = Mag(dx, dy);
    if (*lookahead > 10.0 &&
        (seg_[next_].tx[LINE_RL] - seg_[Index].tx[LINE_RL])
          * (X - seg_[next_].tx[LINE_RL]) +
        (seg_[next_].ty[LINE_RL] - seg_[Index].ty[LINE_RL])
          * (Y - seg_[next_].ty[LINE_RL]) < 0.1)
      break;
    Index = next_;
  }

  double toMiddle = car_->_trkPos.toMiddle;
  if ((seg_[next_].tRInverse > 0.0 && toMiddle < 0.0)
      || (seg_[next_].tRInverse < 0.0 && toMiddle > 0.0)) {
    *lookahead *= MIN(4.0, 1.5 + fabs(toMiddle * 0.3));
  } else {
    *lookahead *= MAX(0.7, 1.5 - fabs(toMiddle * 0.2));
  }

  if ((seg_[next_].tRInverse < 0.0 && toMiddle > 0.0)
     || (seg_[next_].tRInverse > 0.0 && toMiddle < 0.0)) {
    *lookahead *= MAX(1.0, MIN(3.6, 1.0 +
                    (MIN(2.6, fabs(toMiddle) / (pSeg->width / 2)) / 2)
                    * (1.0 + fabs(seg_[next_].tRInverse) * 65.0 +
                    car_->_speed_x / 120.0)));
  } else if ((seg_[next_].tRInverse < 0.0 && car_->_trkPos.toRight < 5.0)
            || (seg_[next_].tRInverse > 0.0 && car_->_trkPos.toLeft < 5.0)) {
    *lookahead *= MAX(0.8, MIN(1.0, 1.0 -
        fabs(seg_[next_].tRInverse) * 200.0 *
        ((5.0 - MIN(car_->_trkPos.toRight, car_->_trkPos.toLeft)) / 5.0)));
  }

  target->x = seg_[next_].tx[LINE_RL];
  target->y = seg_[next_].ty[LINE_RL];

  //
  // Find target speed
  //
  double c0 =
    (seg_[next_].tx[LINE_RL] - seg_[Index].tx[LINE_RL])
      * (seg_[next_].tx[LINE_RL] - X) +
    (seg_[next_].ty[LINE_RL] - seg_[Index].ty[LINE_RL])
      * (seg_[next_].ty[LINE_RL] - Y);
  double c1 =
    (seg_[next_].tx[LINE_RL] - seg_[Index].tx[LINE_RL])
      * (X - seg_[Index].tx[LINE_RL]) +
    (seg_[next_].ty[LINE_RL] - seg_[Index].ty[LINE_RL])
      * (Y - seg_[Index].ty[LINE_RL]);

  double sum = c0 + c1;
  c0 /= sum;
  c1 /= sum;

  target_speed_ =
    (1 - c0) * seg_[next_].tSpeed[LINE_RL] + c0 * seg_[Index].tSpeed[LINE_RL];
  *avspeed = MAX(10.0, seg_[next_].tSpeed[LINE_MID]);
  *speed = MAX(*avspeed, target_speed_);

  if ((seg_[next_].tRInverse > 0.0 && seg_[next_].tLane > seg_[Index].tLane
      && car_->_trkPos.toLeft <= seg_[next_].tLane * width_ + 1.0)
     || (seg_[next_].tRInverse < 0.0 && seg_[next_].tLane < seg_[Index].tLane
     && car_->_trkPos.toLeft >= seg_[next_].tLane * width_ - 1.0)) {
    *avspeed = MAX(*speed, *avspeed);
  } else if ((seg_[next_].tRInverse > 0.001
            && seg_[next_].tLane < seg_[Index].tLane
            && car_->_trkPos.toLeft < seg_[next_].tLane * width_ - 1.0)
        || (seg_[next_].tRInverse < -0.001
          && seg_[next_].tLane > seg_[Index].tLane
          && car_->_trkPos.toLeft > seg_[next_].tLane * width_ + 1.0)) {
    *avspeed *= MAX(0.7, 1.0 - fabs(seg_[next_].tRInverse) * 100);
  }

  double laneoffset = width_ / 2 - (seg_[next_].tLane * width_);
  *raceoffset = laneoffset;

  //
  // Find target curveture (for the inside wheel)
  //
  double TargetCurvature = (1 - c0) * seg_[next_].tRInverse
                            + c0 * seg_[Index].tRInverse;
  if (fabs(TargetCurvature) > 0.01) {
    double r = 1 / TargetCurvature;
    if (r > 0) {
      r -= wheel_track_ / 2;
    } else {
      r += wheel_track_ / 2;
    }
    TargetCurvature = 1 / r;
  }

  //
  // Steering control
  //
  double Error = 0;
  double VnError = 0;
  double carspeed = Mag(car_->_speed_X, car_->_speed_Y);

  //
  // Ideal value
  //
  double steer = atan(wheel_base_ * TargetCurvature) / car_->_steerLock;

  //
  // Servo system to stay on the pre-computed path
  //
  {
    double dx = seg_[next_].tx[LINE_RL] - seg_[Index].tx[LINE_RL];
    double dy = seg_[next_].ty[LINE_RL] - seg_[Index].ty[LINE_RL];
    Error =
      (dx * (Y - seg_[Index].ty[LINE_RL]) -
      dy * (X - seg_[Index].tx[LINE_RL])) / Mag(dx, dy);
  }

  int Prev = (Index + divs_ - 1) % divs_;
  int NextNext = (next_ + 1) % divs_;
  double Prevdx = seg_[next_].tx[LINE_RL] - seg_[Prev].tx[LINE_RL];
  double Prevdy = seg_[next_].ty[LINE_RL] - seg_[Prev].ty[LINE_RL];
  double Nextdx = seg_[NextNext].tx[LINE_RL] - seg_[Index].tx[LINE_RL];
  double Nextdy = seg_[NextNext].ty[LINE_RL] - seg_[Index].ty[LINE_RL];
  double dx = c0 * Prevdx + (1 - c0) * Nextdx;
  double dy = c0 * Prevdy + (1 - c0) * Nextdy;
  double n = Mag(dx, dy);
  dx /= n;
  dy /= n;

  double sError =
    (dx * car_->_speed_Y - dy * car_->_speed_X) / (carspeed + 0.01);
  double cError =
    (dx * car_->_speed_X + dy * car_->_speed_Y) / (carspeed + 0.01);
  VnError = asin(sError);
  if (cError < 0)
    VnError = PI - VnError;

  steer -= (atan(Error * (300 / (carspeed + 300)) / 15) + VnError)
            / car_->_steerLock;

  //
  // Steer into the skid
  //
  double vx = car_->_speed_X;
  double vy = car_->_speed_Y;
  double dirx = cos(car_->_yaw);
  double diry = sin(car_->_yaw);
  double Skid = (dirx * vy - vx * diry) / (carspeed + 0.1);
  if (Skid > 0.9) Skid = 0.9;
  if (Skid < -0.9) Skid = -0.9;
  steer += (asin(Skid) / car_->_steerLock) * 0.9;

  double yr = carspeed * TargetCurvature;
  double diff = car_->_yaw_rate - yr;
  steer -= (0.06 * (100 / (carspeed + 100)) * diff) / car_->_steerLock;

  double trackangle = RtTrackSideTgAngleL(&(car_->_trkPos));
  double angle = trackangle - car_->_yaw;
  NORM_PI_PI(angle);
  angle = -angle;

  if (fabs(angle) > 1.0) {
    if ((angle > 0.0 && steer > 0.0) || (angle < 0.0 && steer < 0.0))
      steer = -steer;
  }

  // We're facing the wrong way.
  // Set it to full steer in whatever direction for now ...
  if (fabs(angle) > 1.6)
    steer = sign(steer);

  *racesteer = steer;
}


bool LRaceLine::isOnLine() const {
  bool ret = false;

  double lane2left = seg_[next_].tLane * width_;
  if (fabs(car_->_trkPos.toLeft - lane2left) <
        MAX(0.1, 1.0 - (car_->_speed_x * (car_->_speed_x / 10)) / 600))
    ret = true;

  return ret;
}


void LRaceLine::GetPoint(const double offset, const double lookahead,
                          vec2f * const rt) const {
  double dLane = (width_ / 2.0 - offset) / width_;
  vec2f last;
  last.x = (tdble) (dLane * seg_[this_].txRight + (1.0 - dLane) * seg_[this_].txLeft);
  last.y = (tdble) (dLane * seg_[this_].tyRight + (1.0 - dLane) * seg_[this_].tyLeft);

  int ndiv = next_;
  double dLength = 0.0;
  double la = static_cast<double>(lookahead)
                * MIN(1.0, MAX(0.8, car_->_speed_x / target_speed_));
  int iLookaheadLimit = static_cast<int>(la / div_length_);
  for (int count = 0; count < iLookaheadLimit && dLength < la; count++) {
    rt->x = (tdble) (dLane * seg_[ndiv].txRight + (1 - dLane) * seg_[ndiv].txLeft);
    rt->y = (tdble) (dLane * seg_[ndiv].tyRight + (1 - dLane) * seg_[ndiv].tyLeft);
    vec2f d = (*rt) - last;
    dLength += Mag(d.x, d.y);

    ndiv = (ndiv + 1) % divs_;
    last = (*rt);
  }  // for
}  // GetPoint


// this returns true if we're approaching a corner & are significantly
// inside the ideal racing line.  The idea is to prevent a sudden outwards
// movement at a time when we should be looking to turn in.
double LRaceLine::CorrectLimit(void) const {
  double toLeft = car_->_trkPos.toLeft;
  double nlane2left = seg_[next_].tLane * width_;

  if ((seg_[next_].tRInverse > 0.001 && toLeft < nlane2left - 2.0)
      || (seg_[next_].tRInverse < -0.001 && toLeft > nlane2left + 2.0))
    return MAX(0.2, MIN(1.0, 1.0 - fabs(seg_[next_].tRInverse) * 100.0));

  int nnext = (next_ + static_cast<int>(car_->_speed_x / 3)) % divs_;
  double nnlane2left = seg_[nnext].tLane * width_;
  if ((seg_[nnext].tRInverse > 0.001 && toLeft < nnlane2left - 2.0)
      || (seg_[nnext].tRInverse < -0.001 && toLeft > nnlane2left + 2.0))
    return MAX(0.3, MIN(1.0, 1.0 - fabs(seg_[nnext].tRInverse) * 40.0));

  // ok, we're not inside the racing line.  Check and see
  // if we're outside it and turning into a corner,
  // in which case we want to correct more to try
  // and get closer to the apex.
  if ((seg_[next_].tRInverse > 0.001
        && seg_[next_].tLane <= seg_[this_].tLane
        && toLeft > nlane2left + 2.0)
      || (seg_[next_].tRInverse < -0.001
        && seg_[next_].tLane >= seg_[this_].tLane
        && toLeft < nlane2left - 2.0))
    return MAX(1.0, MIN(1.5, 1.0 + fabs(seg_[next_].tRInverse)));

  return 1.0;
}


void LRaceLine::SetSegmentCamber(const tTrackSeg *seg, const int div) {
  double dDistRatio = 0.7;
  double dCamberStart = seg->vertex[TR_SR].z - seg->vertex[TR_SL].z;
  double dCamberEnd = seg->vertex[TR_ER].z - seg->vertex[TR_EL].z;
  double dCamber = dCamberStart * dDistRatio + dCamberEnd * (1.0 - dDistRatio);

  dCamberStart /= seg->width;
  dCamberEnd /= seg->width;
  dCamber /= seg->width;

  if (seg_[div].tRInverse < 0.0) {
    dCamber       *= -1.0;
    dCamberStart  *= -1.0;
    dCamberEnd    *= -1.0;
  }

  if (dCamberEnd < dCamberStart) {
    dCamber -= (dCamberStart - dCamberEnd) * 3.0;
  } else if (dCamberEnd > dCamberStart) {
    dCamber += (dCamberEnd - dCamberStart) * 0.4;
  }

  seg_[div].dCamber = dCamber;
}  // SetSegmentCamber

