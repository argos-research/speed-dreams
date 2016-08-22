/***************************************************************************

    file                 : spline.cpp
    created              : Wed Mai 14 20:10:00 CET 2003
    copyright            : (C) 2003 by Bernhard Wymann
    email                : berniw@bluewin.ch

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#include "spline.h"


Spline::Spline()
{
}


void Spline::newSpline(int dim, SplinePoint* spl)
{
  mSpl = spl;
  mDim = dim;
}


double Spline::evaluate(double z)
{
  int i, a, b;
  double t, a0, a1, a2, a3, h;

  a = 0; b = mDim-1;

  do {
    i = (a + b) / 2;
    if (mSpl[i].x <= z) {
      a = i;
    } else {
      b = i;
    }
  } while ((a + 1) != b);

  i = a;
  h = mSpl[i+1].x - mSpl[i].x;
  t = (z-mSpl[i].x) / h;
  a0 = mSpl[i].y;
  a1 = mSpl[i+1].y - a0;
  a2 = a1 - h*mSpl[i].s;
  a3 = h * mSpl[i+1].s - a1;
  a3 -= a2;
  return a0 + (a1 + (a2 + a3*t) * (t-1))*t;
}

