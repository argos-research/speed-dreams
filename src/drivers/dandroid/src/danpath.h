/***************************************************************************

    file                 : danpath.h
    created              : 2011-11-14 07:39:00 UTC
    copyright            : (C) Daniel Schellhammer

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _DANPATH_H_
#define _DANPATH_H_

#include "torcs_or_sd.h"
#include "globaldefinitions.h"

#include <stdlib.h>
#include <math.h>
#include <vector>

#include "Vec2d.h"
#include "cubic.h"


class DanPoint {
  public:
  int line;
  int index;
  Vec2d pos;
  int type;
  double fromstart;
  double tomiddle;
  double radius;
  double yaw;
  double angletotrack;
  double curv_z; // curvature in z direction... e.g. bumps
};


class DanSector {
  public:
  int sector;
  double fromstart;
  double speedfactor;
  double time;
  double bestspeedfactor;
  double besttime;
  int learned;
};


class DanLine {
  public:
  DanLine();
  void init(PTrack t);
  void addDanPoint(DanPoint danpoint);
  bool calcParam();
  void createSectors(std::vector <DanSector>& sect);
  bool getDanPos(double fromstart, DanPoint& danpoint);
  DanPoint nextPos(DanPoint danpoint);
  DanPoint prevPos(DanPoint danpoint);
  private:
  // Data
  double MAX_RADIUS;
  PTrack mTrack;
  PTrackSeg myseg; // Needed in fromStart() because of problem with crossing tracks
  std::vector <DanPoint> mLine;
  // Functions
  DanPoint getPos(int index);
  double calcYaw(DanPoint danpoint);
  double calcTrackYaw(DanPoint danpoint, double& trackyaw);
  bool fromStart(Vec2d pos, double& fromstart);
  bool toMiddle(Vec2d pos, double& tomiddle);
  void printData();
  int getIndex(double fromstart);
  Vec2d getNearestPoint(int index, double fromstart);
  double getToMiddle(double fromstart);
  double getDistDiff(double fromstart1, double fromstart2);
};


class DanPath {
  public:
  enum PathLine{IDEAL_LINE, LEFT_LINE, RIGHT_LINE, NUM_LINES};

  DanPath();
  void init(PTrack t, double max_left, double max_right, double margin, double factor, double seglen);
  bool getDanPos(int line, double fromstart, DanPoint& danpoint);
  DanPoint nextPos(DanPoint danpoint);
  DanLine mDanLine[NUM_LINES];
  std::vector <DanSector> mSector;
  private:
  PTrack mTrack;
  double mMaxL;
  double mMaxR;
  double mMargin;
  double mClothFactor;
  double mSegLen;
  void getClothPath();
};


#endif // _DANPATH_H_
