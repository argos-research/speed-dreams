/***************************************************************************

    file                 : spline.h
    created              : Wed Mai 14 19:53:00 CET 2003
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

#ifndef _SPLINE_H_
#define _SPLINE_H_

class SplinePoint {
  public:
    double x;    /* x coordinate */
    double y;    /* y coordinate */
    double s;    /* slope */
};


class Spline {
  public:
    Spline();
    
    void newSpline(int dim, SplinePoint* spl);
    double evaluate(double z);

  private:
    SplinePoint* mSpl;
    int mDim;
};

#endif // _SPLINE_H_
