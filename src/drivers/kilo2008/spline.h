/*
 *      spline.h
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
 *      $Id: spline.h 3048 2010-10-23 21:35:59Z kmetykog $
 * 
 */

#ifndef SRC_DRIVERS_KILO2008_SPLINE_H_
#define SRC_DRIVERS_KILO2008_SPLINE_H_

class SplinePoint {
 public:
  double x;   // x coordinate.
  double y;   // y coordinate.
  double s;   // slope.
};


class Spline {
 public:
  Spline(int dim, SplinePoint *s);

  double evaluate(double z);

 private:
  SplinePoint *s;
  int dim;
};

#endif  // SRC_DRIVERS_KILO2008_SPLINE_H_
