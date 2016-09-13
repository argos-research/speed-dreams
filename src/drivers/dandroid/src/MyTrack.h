/***************************************************************************

    file        : MyTrack.h
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


#ifndef _MYTRACK_H_
#define _MYTRACK_H_

#include <track.h>
#include <car.h>

#include "Seg.h"


class MyTrack
{
public:
  MyTrack();
  ~MyTrack();

  void NewTrack( tTrack* pNewTrack, double seg_len );

  double GetLength() const;
  int  GetSize() const;
  double GetWidth() const;

  double NormalisePos( double trackPos ) const;
  int  IndexFromPos( double trackPos ) const;

  const Seg& operator[]( int index ) const;
  const Seg& GetAt( int index ) const;

  double GetDelta() const;
  double CalcPos( tTrkLocPos& trkPos, double offset = 0 ) const;
  double CalcPos( tCarElt* car, double offset = 0 ) const;
  double CalcPos( double x, double y, const Seg* hint = 0, bool sides = false ) const;

  double CalcForwardAngle( double trackPos ) const;
  Vec2d CalcNormal( double trackPos ) const;

private:
  void CalcPtAndNormal( const tTrackSeg* pSeg, double toStart, double& t, Vec3d& pt, Vec3d& norm ) const;

private:
  int  NSEG;
  double m_delta;
  Seg* m_pSegs;
  tTrack* m_pCurTrack;
};

#endif // _MYTRACK_H_
