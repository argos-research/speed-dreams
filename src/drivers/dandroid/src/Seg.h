/***************************************************************************

    file        : Seg.h
    created     : 9 Apr 2006
    copyright   : (C) 2006 Tim Foden

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#ifndef _SEG_H_
#define _SEG_H_

#include <track.h>
#include "Vec3d.h"

class Seg
{
public:
  Seg() {};
  ~Seg() {};

public:
  double segDist;
  tTrackSeg* pSeg; // main track segment.
  double wl; // width to left.
  double wr; // width to right.
  double midOffs; // offset to "mid" (nominal centre -- e.g. pitlane)
  double t; // relative position of pt within trackSeg [0..1]
  Vec3d pt; // centre point.
  Vec3d norm; // normal left to right (unit vector in xy, slope in z).
};

#endif // _SEG_H_
