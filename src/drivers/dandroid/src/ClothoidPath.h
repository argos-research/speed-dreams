/***************************************************************************

    file        : ClothoidPath.h
    created     : 9 Apr 2006
    copyright   : (C) 2006 Tim Foden, 2013 D.Schellhammer

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

// ClothoidPath.h: interface for the ClothoidPath class.
//
//////////////////////////////////////////////////////////////////////

#ifndef _CLOTHOIDPATH_H_
#define _CLOTHOIDPATH_H_

#include "LinePath.h"
#include "MyTrack.h"

class ClothoidPath : public LinePath
{
public:
  struct Options
  {
    double maxL;
    double maxR;
    double margin;
    double factor;
    
    Options() : maxL(999), maxR(999), margin(1.0), factor(1.005) {}
    Options( double ml /* = 999 */, double mr = 999, double ma = 1.0, double fa = 1.005 ) : maxL(ml), maxR(mr), margin(ma), factor(fa) {}
  };

public:
  ClothoidPath();
  virtual ~ClothoidPath();

  void MakeSmoothPath( MyTrack* pTrack, const Options& opts );

private:
  void SmoothBetween( int step );
  void SetOffset( double k, double t, PathPt* l3, const PathPt* l2, const PathPt* l4 );
  void Optimise(double factor,int idx, PathPt* l3,const PathPt* l0, const PathPt* l1,const PathPt* l2, const PathPt* l4,const PathPt* l5, const PathPt* l6,int bumpMod );
  void OptimisePath(int step, int nIterations, int bumpMod );

private:
  double m_factor;
};

#endif // _CLOTHOIDPATH_H_
