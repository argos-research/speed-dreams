/***************************************************************************

    file                 : danpath.cpp
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


#include "danpath.h"

#include "ClothoidPath.h"
#include "Utils.h"

//#define DANPATH_VERBOSE


DanLine::DanLine()
{
}


void DanLine::init(PTrack t)
{
  MAX_RADIUS = 1000.0;
  mTrack = t;
  myseg = mTrack->seg;
}


void DanLine::addDanPoint(DanPoint danpoint)
{
  mLine.push_back(danpoint);
}


bool DanLine::calcParam()
{
  int i;
  for (i = 0; i < (int)mLine.size(); i++) {
    if (fromStart(mLine[i].pos, mLine[i].fromstart)) {
      if (!toMiddle(mLine[i].pos, mLine[i].tomiddle)) {
        return false;
      }
    } else {
      return false;
    }
  }
  for (i = 0; i < (int)mLine.size(); i++) {
    mLine[i].yaw = calcYaw(mLine[i]);
    double trackyaw;
    if (!calcTrackYaw(mLine[i], trackyaw)) {
      return false;
    }
    mLine[i].angletotrack = mLine[i].yaw - trackyaw;
    NORM_PI_PI(mLine[i].angletotrack);
  }
  for (i = 0; i < (int)mLine.size(); i++) {
    if (fabs(mLine[i].radius) < MAX_RADIUS) {
      mLine[i].type = (SIGN(mLine[i].radius) > 0) ? TR_LFT : TR_RGT;
    } else {
      mLine[i].type = TR_STR;
    }
  }
  return true;
}


void DanLine::createSectors(std::vector <DanSector>& sect)
{
  int sector = 0;
  DanSector dansect;
  dansect.sector = sector;
  dansect.fromstart = 0.0;
  dansect.speedfactor = 0.6;
  dansect.time = 0.0;
  dansect.bestspeedfactor = 0.6;
  dansect.besttime = 10000.0;
  dansect.learned = 0;
  sect.push_back(dansect);
  double lastfromstart = dansect.fromstart;
  bool newsector = false;
  bool largeradius = true;
  for (int i = 1 ; i < (int)mLine.size(); i++) {
    // Find sector
    if (fabs(mLine[i].radius) < 150.0) {
      largeradius = false;
    }
    if (fabs(mLine[i].radius) > 200.0) {
      if (!largeradius) {
        newsector = true;
      }
      largeradius = true;
    }
    // Store sector
    if (newsector) {
      if (mLine[mLine.size()-1].fromstart - mLine[i].fromstart > 400.0) {
        if (mLine[i].fromstart < 200.0) {
          // Do nothing
        } else if (mLine[i].fromstart - lastfromstart > 200.0) {
          sector++;
          dansect.sector = sector;
          dansect.fromstart = mLine[i].fromstart;
          lastfromstart = dansect.fromstart;
          sect.push_back(dansect);
          //GfOut("fs:%g radius:%g\n", mLine[i].fromstart, fabs(mLine[i].radius));
        } else {
          sect[sector].fromstart = mLine[i].fromstart;
          lastfromstart = mLine[i].fromstart;
          //GfOut("overwrite fs:%g radius:%g\n", mLine[i].fromstart, fabs(mLine[i].radius));
        }
      }
      newsector = false;
    }
  }
  printData();
}


void DanLine::printData()
{
#ifdef DANPATH_VERBOSE
  if (mLine[0].line == 0) {
    for (int i = 0; i < (int)mLine.size(); i++) {
      GfOut("ind:%d fs:%g r:%g curv_z:%g\n", i, mLine[i].fromstart, mLine[i].radius, mLine[i].curv_z);
    }
    for (int i = 0; i < (int)mSector.size(); i++) {
      GfOut("sector:%d fs:%g speedfactor:%g\n", mSector[i].sector, mSector[i].fromstart, mSector[i].speedfactor);
    }
  }
#endif
}


bool DanLine::getDanPos(double fromstart, DanPoint& danpoint)
{
  if (!mLine.size()) {
    return false;
  }
  int index = getIndex(fromstart);
  danpoint = mLine[index];
  
  // Calculate radius
  double radius = mLine[index].radius;
  double nextradius = nextPos(mLine[index]).radius;
  if (SIGN(radius) != SIGN(nextradius)) {
    danpoint.radius = 100000.0;
  } else {
    // Interpolate radius linear
    double seglength = getDistDiff(mLine[index].fromstart, nextPos(mLine[index]).fromstart);
    double poslength = getDistDiff(mLine[index].fromstart, fromstart);
    double invradius = 1.0 / radius + (poslength / seglength) * (1.0 / nextradius - 1.0 / radius);
    danpoint.radius = 1.0 / invradius;
  }

  danpoint.tomiddle = getToMiddle(fromstart); // Interpolate cubic toMiddle
  danpoint.pos = getNearestPoint(danpoint.index, fromstart); // position (straight interpolation)
  danpoint.fromstart = fromstart;
  //danpoint.yaw = calcYaw(danpoint); // useless without interpolation
  return true;
}


DanPoint DanLine::nextPos(DanPoint danpoint)
{
  danpoint.index++;
  return getPos(danpoint.index);
}


DanPoint DanLine::prevPos(DanPoint danpoint)
{
  danpoint.index--;
  return getPos(danpoint.index);
}


DanPoint DanLine::getPos(int index)
{
  if (index < 0) {
    return mLine[mLine.size() - 1];
  } else if (index >= (int)mLine.size()) {
    return mLine[0];
  } else {
    return mLine[index];
  }
}


double DanLine::calcYaw(DanPoint danpoint)
{
  Vec2d prev = danpoint.pos - prevPos(danpoint).pos;
  Vec2d next = nextPos(danpoint).pos - danpoint.pos;
  return Utils::VecAngle(prev + next);
}


double DanLine::calcTrackYaw(DanPoint danpoint, double& trackyaw)
{
  tTrkLocPos locpos;
  RtTrackGlobal2Local(myseg, (tdble) danpoint.pos.x, (tdble) danpoint.pos.y, &locpos, TR_LPOS_MAIN);
  myseg = locpos.seg;
  trackyaw = RtTrackSideTgAngleL(&locpos);
  return true;
}


bool DanLine::fromStart(Vec2d pos, double& fromstart)
{
  tTrkLocPos locpos;
  RtTrackGlobal2Local(myseg, (tdble) pos.x, (tdble) pos.y, &locpos, TR_LPOS_MAIN);
  myseg = locpos.seg;
  fromstart = RtGetDistFromStart2(&locpos);
  return true;
}


bool DanLine::toMiddle(Vec2d pos, double& tomiddle)
{
  tTrkLocPos locpos;
  RtTrackGlobal2Local(myseg, (tdble) pos.x, (tdble) pos.y, &locpos, TR_LPOS_MAIN);
  myseg = locpos.seg;
  tomiddle = locpos.toMiddle;
  return true;
}


// Find nearest section on mLine
int DanLine::getIndex(double fromstart)
{
  double estpos = fromstart / mTrack->length;
  int i = (int)floor(estpos * mLine.size());
  while (true) {
    if (i < 0) {
      i = mLine.size() - 1;
    } else if (i >= (int)mLine.size()) {
      i = 0;
    }
    double sectlen = getDistDiff(getPos(i).fromstart, getPos(i + 1).fromstart);
//    double poslen = getDistDiff(getPos(i).fromstart, fromstart);
    double poslen = getDistDiff(getPos(i).fromstart, fromstart+0.001);
    if (poslen >= 0.0 && poslen <= sectlen) {
      //GfOut("poslen:%g sectlen:%g", poslen, sectlen);
      break;
    }
    i += (int)SIGN(poslen);
  }
  return i;
}


Vec2d DanLine::getNearestPoint(int index, double fromstart)
{
  Vec2d straight = getPos(index+1).pos - mLine[index].pos;
  double straightlen = getDistDiff(mLine[index].fromstart, getPos(index+1).fromstart);
  double poslen = getDistDiff(mLine[index].fromstart, fromstart);
  Vec2d pointonStraight = mLine[index].pos + straight * (poslen / straightlen);
  return pointonStraight;
}


double DanLine::getToMiddle(double fromstart)
{
  int index = getIndex(fromstart);
  TCubic ccurve(mLine[index].fromstart, mLine[index].tomiddle, mLine[index].angletotrack, nextPos(mLine[index]).fromstart, nextPos(mLine[index]).tomiddle, nextPos(mLine[index]).angletotrack);
  return ccurve.CalcOffset(fromstart);
}


double DanLine::getDistDiff(double fromstart1, double fromstart2)
{
  double diff = fromstart2 - fromstart1;
  diff = (diff >= 0.0) ? diff : diff + mTrack->length;
  return (diff <= mTrack->length / 2.0) ? diff : diff - mTrack->length;
}




DanPath::DanPath()
{
}


void DanPath::init(PTrack t, double max_left, double max_right, double margin, double factor, double seglen)
{
  mTrack = t;
  mMaxL = max_left;
  mMaxR = max_right;
  mMargin = margin;
  mClothFactor = factor;
  mSegLen = seglen;

  for (int i=0; i < NUM_LINES; i++) {
    mDanLine[i].init(t);
  }

  getClothPath();

  for (int i=0; i < NUM_LINES; i++) {
    if (!mDanLine[i].calcParam()) {
      GfOut("Error danpath: calcParam() failed\n");
    }
  }
  
  mDanLine[IDEAL_LINE].createSectors(mSector);
}


bool DanPath::getDanPos(int line, double fromstart, DanPoint& danpoint)
{
  return mDanLine[line].getDanPos(fromstart, danpoint);
}


DanPoint DanPath::nextPos(DanPoint danpoint)
{
  return mDanLine[danpoint.line].nextPos(danpoint);
}


void DanPath::getClothPath()
{
  Vec2d point(0, 0);

  MyTrack track;
  track.NewTrack(mTrack, mSegLen);
  for (int l = 0; l < NUM_LINES; l++) {
    ClothoidPath clpath;
    if (l == IDEAL_LINE ) {
      clpath.MakeSmoothPath(&track, ClothoidPath::Options(mMaxL, mMaxR, mMargin, mClothFactor));
    }
    if (l == LEFT_LINE ) {
      clpath.MakeSmoothPath(&track, ClothoidPath::Options(mMaxL, -1.0, mMargin, mClothFactor));
    }
    if (l == RIGHT_LINE) {
      clpath.MakeSmoothPath(&track, ClothoidPath::Options(-1.0, mMaxR, mMargin, mClothFactor));
    }
    LinePath::PathPt pathpoint;
    int index = 0;
    for (int j = 0; j < track.GetSize(); j++) {
      pathpoint = clpath.GetAt(j);
      point.x = pathpoint.pt.x;
      point.y = pathpoint.pt.y;
      DanPoint p;
      p.line = l;
      p.index = index++;
      p.pos = point;
      p.type = 0;
      p.fromstart = 0;
      p.tomiddle = 0;
      p.radius = 1/pathpoint.k;
      p.yaw = 0;
      p.angletotrack = 0;
      p.curv_z = pathpoint.kz;
      mDanLine[l].addDanPoint(p);
    }
  }
}
