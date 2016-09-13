/***************************************************************************

    file        : LinePath.h
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


#ifndef _LINEPATH_H_
#define _LINEPATH_H_


#include "MyTrack.h"


class LinePath
{
public:
  struct PathPt
  {
    const Seg* pSeg; // track seg that contains this Seg.
    double k; // curvature in xy.
    double kz;	 // curvature in z direction... e.g. bumps.
    double offs; // offs from centre point.
    Vec3d pt; // actual pt (same as CalcPt())
    double h; // predicted height of car above track (flying).
    double lBuf; // buffer from left for safety.
    double rBuf; // buffer from right for safety.
    double fwdK;

    double Wl() const { return pSeg->wl; }
    double Wr() const { return pSeg->wr; }
    const Vec3d&Pt() const { return pSeg->pt; }
    const Vec3d&Norm() const { return pSeg->norm; }
    Vec3d CalcPt() const { return pSeg->pt + pSeg->norm * offs; }
  };

public:
  LinePath();
  virtual ~LinePath();

  void	Initialise( MyTrack* pTrack, double maxL = 999, double maxR = 999, double margin = 1.0 );

  const PathPt&	GetAt( int idx ) const;

  void CalcCurvaturesXY( int start, int len, int step = 1 );
  void CalcCurvaturesZ( int start, int len, int step = 1 );

  void CalcCurvaturesXY( int step = 1 );
  void CalcCurvaturesZ( int step = 1 );
  void CalcFwdAbsK( int range );

protected:
  MyTrack* m_pTrack;
  PathPt* m_pPath;

  double m_maxL;
  double m_maxR;
  double m_margin;
};

#endif // _LINEPATH_H_
