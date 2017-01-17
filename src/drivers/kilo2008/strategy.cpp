/*
 *      strategy.cpp
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
 *      $Id: strategy.cpp 5880 2014-12-01 16:22:22Z wdbee $
 * 
 */

#include "src/drivers/kilo2008/strategy.h"

#include <deque>

#include "src/drivers/kilo2008/kdriver.h"   // KILO_ATT, KILO_SECT

#define MAXFUEL_FOR_THIS_RACE 60.0
// #define STRAT_DEBUG

// [kg/m] fuel consumption.
const double KStrategy::MAX_FUEL_PER_METER = 0.0006;
// [-] Repair needed if damage is beyond this value
const int   KStrategy::PIT_DAMAGE = 5000;
// [laps] how many laps' fuel to leave in tank before deciding on refuel?
const double KStrategy::SAFE_LAPS = 2.0;
// [-] Store this count of laps' damages
const int   KStrategy::LAST_LAPS = 10;


KStrategy::KStrategy() {
  last_damages_ = new std::deque<int>;

  laps_ = 0;
  fuel_checked_ = false;
  fuel_per_lap_ = 0.0;
  last_pit_fuel_ = 0.0;
  fuel_sum_ = 0.0;
}  // KStrategy


/** 
 * SetFuelAtRaceStart
 * 
 * @param t the track
 * @param carParmHandle handle for car parameters
 * @param s current situation, provided by TORCS
 * @param index index of car in the team
 */
void KStrategy::SetFuelAtRaceStart(const tTrack * const t,
                                    void ** const carParmHandle,
                                    const tSituation * const s,
                                    const int index) {
  // Load and set parameters.
  const tdble fuel_cons_factor =
    GfParmGetNum(*carParmHandle, SECT_ENGINE, PRM_FUELCONS, NULL, 1.0f);
  const double fuel =
    GfParmGetNum(*carParmHandle, KILO_SECT_PRIV, KILO_ATT_FUELPERLAP,
         NULL, (tdble) (t->length * MAX_FUEL_PER_METER * fuel_cons_factor));
  expected_fuel_per_lap_ = fuel;
  // Pittime is pittime without refuel.
  pittime_ = GfParmGetNum(*carParmHandle, KILO_SECT_PRIV,
                            KILO_ATT_PITTIME, NULL, 25.0);
  best_lap_ = GfParmGetNum(*carParmHandle, KILO_SECT_PRIV,
                            KILO_ATT_BESTLAP, NULL, 87.0);
  worst_lap_ = GfParmGetNum(*carParmHandle, KILO_SECT_PRIV,
                            KILO_ATT_WORSTLAP, NULL, 87.0);
  // Fuel tank capacity
  const double maxfuel = GfParmGetNum(*carParmHandle, SECT_CAR,
                                      PRM_TANK, NULL, 100.0);

  // Fuel for the whole race. A race needs one more lap - why???
  const double fuelForRace = (s->_raceType == RM_TYPE_RACE)
    ? (s->_totLaps + 1.0) * fuel
    : s->_totLaps * fuel;

  // Compute race times for min to min + 9 pit stops.
  ComputeBestNumberOfPits(maxfuel, fuelForRace, s->_totLaps, true);
  last_fuel_ = fuel_per_stint_;

  // If the setup defines initial fuel amount, use that value in races.
  // Otherwise use computed amount.
  const double initial_fuel = GfParmGetNum(*carParmHandle, SECT_CAR,
                                            PRM_FUEL, NULL, 0.0);
  if (s->_raceType == RM_TYPE_RACE) {
    if (initial_fuel) {
      GfParmSetNum(*carParmHandle, SECT_CAR, PRM_FUEL, NULL, (tdble) initial_fuel);
    } else {
      // Add fuel dependent on index to avoid fuel stop in the same lap.
      GfParmSetNum(*carParmHandle, SECT_CAR, PRM_FUEL, NULL,
                    (tdble) (last_fuel_ + index * expected_fuel_per_lap_));
    }
  } else {
    // Use fuel for whole 'race', ie qualy or practice N laps.
      GfParmSetNum(*carParmHandle, SECT_CAR, PRM_FUEL, NULL, (tdble) fuelForRace);
  }
}   // SetFuelAtRaceStart


/** 
 * updateFuelStrategy
 * 
 * Computes the fuel amount required to finish the race.
 * If there is not enough fuel in the tank, re-runs pit computations.
 * 
 * @param: -
 * @return void
 */
void KStrategy::UpdateFuelStrategy() {
  // Required additional fuel for the rest of the race.
  // +1 because the computation happens right after crossing the start line.
  double fuelperlap = MAX(fuel_per_lap_, 2.5);  // average
  double required_fuel = ((LapsToGo() + 1) - ceil(car_->_fuel / fuelperlap))
                            * fuelperlap;

  // We don't have enough fuel to end the race, need at least one stop.
  if (required_fuel >= 0.0) {
    // Compute race times for different pit strategies.
    ComputeBestNumberOfPits(car_->_tank, required_fuel, LapsToGo(), FALSE);
  }
}  // UpdateFuelStrategy


/** 
 * pitRefuel
 * @return amount of fuel requested from race engine.
 */
double KStrategy::PitRefuel() {
  UpdateFuelStrategy();

  double fuel;
  if (remaining_stops_ > 1) {
    fuel = MIN(MAX(fuel_per_stint_, MAXFUEL_FOR_THIS_RACE),
                car_->_tank - car_->_fuel);  // !!!
    --remaining_stops_;
  } else {
    double cmpfuel = (fuel_per_lap_ == 0.0)
      ? expected_fuel_per_lap_
      : fuel_per_lap_;
    fuel = MAX(MIN((LapsToGo() + 1.0) * cmpfuel - car_->_fuel,
                car_->_tank - car_->_fuel), 0.0);
  }

  last_pit_fuel_ = fuel;
  return fuel;
}   // pitRefuel


/*
 * Based on fuel tank capacity, remaining laps and current fuel situtation,
 * decides the optimum number of pits to complete the race the fastest way.
 * Ie: it may be advantegous to pit more and refill less each time.
 * param preRace = is it the pre-race, first time calculation?
 */
void KStrategy::ComputeBestNumberOfPits(const double tankCapacity,
                                        const double requiredFuel,
                                        const int remainingLaps,
                                        const bool preRace) {
  int pitstopMin = static_cast<int>(ceil(requiredFuel / tankCapacity));
  double mintime = DBL_MAX;
  int beststops = pitstopMin;
  for (int i = 0; i < (preRace ? 5 : 4); ++i) {
    double stintFuel = requiredFuel / (pitstopMin + i);   // + 1);
    double fillratio = stintFuel / tankCapacity;
    double avglapest = best_lap_ + (worst_lap_ - best_lap_) * fillratio;
    double racetime = (pitstopMin + i) * (pittime_ + stintFuel / 8.0) +
              remainingLaps * avglapest;
    if (mintime > racetime) {
      mintime = racetime;
      beststops = pitstopMin + i - (preRace ? 1 : 0);
      fuel_per_stint_ = stintFuel;
    }
  }
  remaining_stops_ = beststops;
}   // ComputeBestNumberOfPits


/**
 * Update
 * 
 * Stores last N laps' damage values.
 * Updates best & worst lap times.
 * Then resumes fuel statistics updates.
 *
 * @param: -
 * @return: -
 */
void KStrategy::Update() {
  if (car_->_laps > laps_) {   // If a new lap has been finished
    laps_ = car_->_laps;
    last_damages_->push_front(car_->_dammage);  // store last lap's damage
    if (static_cast<int>(last_damages_->size()) > LAST_LAPS)
      last_damages_->pop_back();   // and keep deque size at limit

#ifdef STRAT_DEBUG
    // print damage values in reverse order
    cerr << car->_name << ": damages";
    for (deque<int>::reverse_iterator rit = last_damages_->rbegin();
          rit < last_damages_->rend();
          ++rit)
      cerr << " " << *rit;
    cerr << endl;
#endif
  }

  // Update best & worst lap times
  best_lap_ = MIN((best_lap_ == 0.0 ? car_->_lastLapTime : best_lap_),
                  car_->_lastLapTime);
  worst_lap_ = MAX(worst_lap_, car_->_lastLapTime);


  // Fuel statistics update.
  int id = car_->_trkPos.seg->id;
  // Range must include enough segments to be executed once guaranteed.
  if (id >= 0 && id < 5 && !fuel_checked_) {
    if (car_->race.laps > 1) {
      fuel_sum_ += (last_fuel_ + last_pit_fuel_ - car_->priv.fuel);
      fuel_per_lap_ = (fuel_sum_ / (car_->race.laps - 1));
      // This is here for adding strategy decisions, otherwise
      // it could be moved to pitRefuel for efficiency.
      UpdateFuelStrategy();
    }
    last_fuel_ = car_->priv.fuel;
    last_pit_fuel_ = 0.0;
    fuel_checked_ = true;
  } else if (id > 5) {
    fuel_checked_ = false;
  }
}  // update


/**
 * NeedPitstop
 * 
 * Checks if we need to pit.
 * We need a pit stop if fuel is low, or if damage is over the limit.
 * In the last 5 laps we want to pit only if the predicted damage value
 * would go beyond the dreaded 10k value.
 *
 * @return True if we need to visit the pit
 */
bool KStrategy::NeedPitstop() const {
  bool ret = false;

  // Question makes sense only if there is a pit.
  if (car_->_pit != NULL) {
    // Ideally we shouldn't pit on the last lap for any reason,
    // just get to the finish line somehow.
    if (LapsToGo() > 0) {
      // Do we need to refuel?
      double cmpfuel = (fuel_per_lap_ == 0.0)
        ? expected_fuel_per_lap_
        : fuel_per_lap_;

#ifdef STRAT_DEBUG
      if (strcmp(car->_name, "Kilo 1") == 0 && car->_fuel < 5.0)
        cerr << car->_name
            << " lapsToGo:" << LapsToGo(car)
            << " dam_avg:" << GetAvgDamage()
            << " fuel:" << car->_fuel
            << " cmpfuel:" << cmpfuel
            << " SAFE:"
            << static_cast<bool>(car->_fuel < SAFE_LAPS * cmpfuel)
            << " cf<ltg(c)*cmp:"
            << static_cast<bool>(car->_fuel < LapsToGo(car) * cmpfuel)
            << endl;
#endif
      cmpfuel *= MIN(SAFE_LAPS, LapsToGo());
      if (car_->_fuel < cmpfuel) {
#ifdef STRAT_DEBUG
        cerr << car->_name << " REFUEL" << endl;
#endif
        ret = true;
      } else {
        // Do we need to repair and is the pit free?
        if (car_->_dammage > PIT_DAMAGE) {
          // Let's see if we can make it somehow onto the finish line
          // BEWARE doesnt check for limits, works for races > 5 laps!!!
          if (LapsToGo() <= LAST_LAPS) {
            // If prediction shows we would top the damage limit,
            // let's visit the pit
            if (car_->_dammage + GetAvgDamage() * LapsToGo() >= 10000) {
              ret = IsPitFree();
            }
          } else {
            ret = IsPitFree();
          }
        }  // if damage > PIT_DAMAGE
      }  // else fuel decides
    }  // if LapsToGo > 0
  }  // if pit != NULL

  return ret;
}  // NeedPitstop


/**
 * GetAvgDamage
 * 
 * Computes last N laps' average damage increment
 *
 * @return average damage increment
 */
int KStrategy::GetAvgDamage(void) const {
  return (last_damages_->front() - last_damages_->back())
          / MAX(last_damages_->size(), 1);
}  // GetAvgDamage


/**
 * PitRepair
 * 
 * Tells TORCS how much damage we ask to be repaired.
 * On the last N laps, we push fast pitstops with minimal repairs.
 * Otherwise, let's repair all damage.
 *
 * @return how much damage to repair
 */
int KStrategy::PitRepair() const {
  int ret = (LapsToGo() <= LAST_LAPS)   // In the last N laps
    ? GetAvgDamage() * LapsToGo()       // repair only as much as really needed.
    : car_->_dammage;                  // Otherwise repair everything.

#ifdef STRAT_DEBUG
  cerr << car->_name
    << " pitRepair:" << ret
    << endl;
#endif

  // Clear buffer
  last_damages_->clear();

  return ret;
}  // pitRepair


bool KStrategy::IsPitFree() const {
  if (car_->_pit != NULL && car_->_pit->pitCarIndex == TR_PIT_STATE_FREE)
    return true;
  else
    return false;
}  // IsPitFree
