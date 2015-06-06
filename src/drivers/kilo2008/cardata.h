/*
 *      cardata.h
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
 *      $Id: cardata.h 3038 2010-10-22 09:07:58Z kmetykog $
 * 
 */

/*
    This class holds global facts about cars, therefore no data relative to
    each other (for that is the class Opponents/Opponent responsible).
*/

#ifndef SRC_DRIVERS_KILO2008_CARDATA_H_
#define SRC_DRIVERS_KILO2008_CARDATA_H_

#include <car.h>        // tCarElt
#include <raceman.h>    // tSituation
#include <list>         // std::list

class SingleCardata {
 public:
  void init(const tCarElt * car);

  inline double getSpeedInTrackDirection() const { return speed; }
  inline double getWidthOnTrack() const { return width; }
  inline double getLengthOnTrack() const { return length; }
  inline double getTrackangle() const { return trackangle; }
  inline double getCarAngle() const { return angle; }
  inline bool thisCar(const tCarElt * car) const { return (car == this->car); }
  inline tPosd *getCorner1() { return corner1; }
  inline tPosd *getCorner2() { return corner2; }
  inline tPosd *getLastSpeed() { return lastspeed;}

  void update();
  // void operator() (void) {this->update();}

 protected:
  static double getSpeed(const tCarElt * car, const double trackangle);

  double speed;          // speed in direction of the track.
  double width;          // the cars needed width on the track.
  double length;         // the cars needed length on the track.
  double trackangle;     // Track angle at the opponents position.
  double angle;          // The angle of the car relative to the track tangent.

  tPosd corner1[4];
  tPosd corner2[4];
  tPosd lastspeed[3];

  const tCarElt *car;         // For identification.
};


// TODO(kilo): use singleton pattern.
class Cardata {
 public:
  explicit Cardata(tSituation * s);
  ~Cardata();

  void update() const;
  SingleCardata *findCar(const tCarElt * car) const;

 protected:
  std::list<SingleCardata> *data;  // List with car data.
};

#endif  // SRC_DRIVERS_KILO2008_CARDATA_H_
