/*
 *      strategy.h
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
 *      $Id: strategy.h 3115 2010-11-08 23:42:00Z kmetykog $
 * 
 */

/*
    Pit strategy for drivers.
*/

#ifndef SRC_DRIVERS_KILO2008_STRATEGY_H_
#define SRC_DRIVERS_KILO2008_STRATEGY_H_

#include <track.h>    // tTrack
#include <car.h>      // tCarElt
#include <raceman.h>  // tSituation

#include <deque>

class KStrategy {
 public:
  KStrategy();
  ~KStrategy() {delete last_damages_;}

  // Interface
  void Update();
  bool NeedPitstop() const;
  int PitRepair() const;
  double PitRefuel();
  void SetFuelAtRaceStart(const tTrack * const t,
                            void ** const carParmHandle,
                            const tSituation * const s,
                            const int index);
  void set_car(const tCarElt * const car) {this->car_ = car;}

 protected:
  bool IsPitFree() const;
  int GetAvgDamage() const;
  inline int LapsToGo() const
      {return car_->_remainingLaps - car_->_lapsBehindLeader;}
  void UpdateFuelStrategy();
  void ComputeBestNumberOfPits(const double tankCapacity,
                                const double requiredFuel,
                                const int remainingLaps,
                                const bool preRace);

  const tCarElt * car_;
  int laps_;
  std::deque<int> *last_damages_;
  int remaining_stops_;
  double fuel_per_stint_;
  double pittime_;            // Expected additional time for pit stop.
  double best_lap_;           // Best possible lap, empty tank and alone.
  double worst_lap_;          // Worst possible lap, full tank and alone.
  bool fuel_checked_;         // Fuel statistics updated.
  double fuel_per_lap_;       // Maximum amount of fuel we needed for a lap.
  double last_pit_fuel_;      // Amount refueled, special case when we refuel.
  double last_fuel_;          // Fuel available when we cross the start lane.
  double expected_fuel_per_lap_;  // Expected fuel per lap (may be inaccurate).
  double fuel_sum_;           // All the fuel used.

  static const double MAX_FUEL_PER_METER;   // [kg/m] fuel consumtion.
  static const int PIT_DAMAGE;  // If damage > we request a pit stop.
  static const double SAFE_LAPS;   // Can go this # of laps before req. refuel.
  static const int LAST_LAPS;   // Store this count of last laps' damage datae
};

#endif  // SRC_DRIVERS_KILO2008_STRATEGY_H_
