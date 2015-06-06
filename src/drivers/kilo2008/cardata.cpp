/*
 *      cardata.cpp
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
 *      $Id: cardata.cpp 3038 2010-10-22 09:07:58Z kmetykog $
 * 
 */

#include "src/drivers/kilo2008/cardata.h"

#include <robottools.h>   // Rt*

#include <list>
#include <algorithm>      // for_each, find

#include "src/drivers/kilo2008/linalg.h"  // v2d

void SingleCardata::update() {
  trackangle = RtTrackSideTgAngleL(const_cast<tTrkLocPos*>(&(car->_trkPos)));
  speed = getSpeed(car, trackangle);
  angle = trackangle - car->_yaw;
  NORM_PI_PI(angle);
  width =
    MAX(car->_dimension_y,
    fabs(car->_dimension_x * sin(angle) +
         car->_dimension_y * cos(angle))) + 0.1;
  length =
    MAX(car->_dimension_x,
    fabs(car->_dimension_y * sin(angle) +
         car->_dimension_x * cos(angle))) + 0.1;

  for (int i = 0; i < 4; i++) {
    corner2[i].ax = corner1[i].ax;
    corner2[i].ay = corner1[i].ay;
    corner1[i].ax = car->_corner_x(i);
    corner1[i].ay = car->_corner_y(i);
  }  // for i

  lastspeed[2].ax = lastspeed[1].ax;
  lastspeed[2].ay = lastspeed[1].ay;
  lastspeed[1].ax = lastspeed[0].ax;
  lastspeed[1].ay = lastspeed[0].ay;
  lastspeed[0].ax = car->_speed_X;
  lastspeed[0].ay = car->_speed_Y;
}  // update


// compute speed component parallel to the track.
double SingleCardata::getSpeed(const tCarElt * car, const double ltrackangle) {
  v2d speed(car->_speed_X, car->_speed_Y);
  v2d dir(cos(ltrackangle), sin(ltrackangle));
  return speed * dir;
}  // getSpeed


void SingleCardata::init(const CarElt * pcar) {
  car = pcar;
  for (int i = 0; i < 4; i++) {
    corner1[i].ax = corner2[i].ax = car->_corner_x(i);
    corner1[i].ay = corner2[i].ay = car->_corner_y(i);
  }  // for i
  lastspeed[0].ax = lastspeed[1].ax = lastspeed[2].ax = car->_speed_X;
  lastspeed[0].ay = lastspeed[1].ay = lastspeed[2].ay = car->_speed_Y;
}  // init


Cardata::Cardata(tSituation * s) {
  data = new std::list<SingleCardata>(s->_ncars);
  std::list<SingleCardata>::iterator it = data->begin();
  for (int i = 0; it != data->end(); it++, i++) {
    it->init(s->cars[i]);
  }
}  // Cardata


Cardata::~Cardata() {
  delete data;
}  // ~Cardata


static inline void Cupdate(SingleCardata &a) {  // NOLINT(runtime/references)
  a.update();
}

void Cardata::update() const {
  std::for_each(data->begin(), data->end(), Cupdate);
}  // update


inline bool operator==(const SingleCardata& a, const tCarElt* b)
  {return a.thisCar(b);}

SingleCardata * Cardata::findCar(const tCarElt * car) const {
  SingleCardata *ret = NULL;

  std::list<SingleCardata>::iterator it =
    std::find(data->begin(), data->end(), car);
  if (it != data->end())
    ret = &(*it);

  return ret;
}  // findCar
