/*
 *      spline.cpp
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
 *      $Id: spline.cpp 3048 2010-10-23 21:35:59Z kmetykog $
 * 
 */

#include "src/drivers/kilo2008/spline.h"

Spline::Spline(int dim, SplinePoint *s) {
  this->s = s;
  this->dim = dim;
}


double Spline::evaluate(double z) {
  int i, a, b;
  double t, a0, a1, a2, a3, h;

  // Binary search for interval.
  a = 0;
  b = dim - 1;
  do {
    i = (a + b) / 2;
    if (s[i].x <= z) {
      a = i;
    } else {
      b = i;
    }
  } while ((a + 1) != b);

  // Evaluate.
  i = a;
  h = s[i+1].x - s[i].x;
  t = (z-s[i].x) / h;
  a0 = s[i].y;
  a1 = s[i+1].y - a0;
  a2 = a1 - h*s[i].s;
  a3 = h * s[i+1].s - a1;
  a3 -= a2;
  return a0 + (a1 + (a2 + a3*t) * (t-1))*t;
}

