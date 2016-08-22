/***************************************************************************

    file        : ClothoidPath.cpp
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

// ClothoidPath.cpp: implementation of the ClothoidPath class.
//
//////////////////////////////////////////////////////////////////////

#include "ClothoidPath.h"
#include "Utils.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

ClothoidPath::ClothoidPath()
{
}

ClothoidPath::~ClothoidPath()
{
}

void ClothoidPath::MakeSmoothPath(MyTrack* pTrack, const Options& opts)
{
  m_factor = opts.factor;

  LinePath::Initialise( pTrack, opts.maxL, opts.maxR, opts.margin );

  const int NSEG = pTrack->GetSize();

  CalcCurvaturesZ();
  int fwdRange = 110;
  CalcFwdAbsK( fwdRange );

  const int delta = 25;
  const int n = (150 + delta - 1) / delta;

  int  step = 1;
  while( step * 4 < NSEG )
    step *= 2;

  do
  {
    step = (step + 1) / 2;
    // int n = 100 * int(sqrt(step));
    for( int i = 0; i < n; i++ )
    {
      OptimisePath( step, delta, 0 );
    }
  }
  while( step > 1 );

  CalcCurvaturesZ();
}

void ClothoidPath::SmoothBetween( int step )
{
  const int NSEG = m_pTrack->GetSize();

  // now smooth the values between steps
  PathPt* l0 = 0;
  PathPt* l1 = &m_pPath[((NSEG - 1) / step) * step];
  PathPt* l2 = &m_pPath[0];
  PathPt* l3 = &m_pPath[step];

  int  j = 2 * step;
  for( int i = 0; i < NSEG; i += step )
  {
    l0 = l1;
    l1 = l2; // l1 represents m_pLines[i];
    l2 = l3;
    l3 = &m_pPath[j];

    j += step;
    if( j >= NSEG )
      j = 0;

    Vec3d p0 = l0->pt;//CalcPt();
    Vec3d p1 = l1->pt;//CalcPt();
    Vec3d p2 = l2->pt;//CalcPt();
    Vec3d p3 = l3->pt;//CalcPt();

    double k1 = Utils::CalcCurvatureXY(p0, p1, p2);
    double k2 = Utils::CalcCurvatureXY(p1, p2, p3);

    if( i + step > NSEG )
      step = NSEG - i;

    for( int k = 1; k < step; k++ )
    {
      double t;
      PathPt& l = m_pPath[(i + k) % NSEG];
      Utils::LineCrossesLineXY( l.Pt(), l.Norm(), p1, p2 - p1, t );
      l.offs = t;

      double len1 = (l.CalcPt() - p1).len();
      double len2 = (l.CalcPt() - p2).len();
      double kappa = (k1 * len2 + k2 * len1) / (len1 + len2);

      if( kappa != 0 )
      {
        double delta = 0.0001;
        double deltaK = Utils::CalcCurvatureXY(p1, l.Pt() + l.Norm() * (t + delta), p2);
        t += delta * kappa / deltaK;
      }

      const double buf = 1.0;//1.25;
      if( t < -l.Wl() + l.lBuf + buf )
        t = -l.Wl() + l.lBuf + buf;
      else if( t > l.Wr() - l.rBuf - buf )
        t = l.Wr() - l.rBuf - buf;

      if( t < -m_maxL + l.lBuf + buf )
        t = -m_maxL + l.lBuf + buf;
      else if( t > m_maxR - l.rBuf - buf )
        t = m_maxR - l.rBuf - buf;

      l.offs = t;
      l.pt = l.CalcPt();
    }
  }
}

void ClothoidPath::SetOffset(
  double   k,
  double   t,
  PathPt*   l3,
  const PathPt* l2, 
  const PathPt* l4 )
{
  double wl  = -MN(m_maxL, l3->Wl()) + m_margin;
  double wr  =  MN(m_maxR, l3->Wr()) - m_margin;
  double buf = MN(1.5, 100 * fabs(k)); // a = v*v/r;

  if( k >= 0 )// 0.00001 )
  {
    if( t < wl )
      t = wl;
    else if( t > wr - l3->rBuf - buf )
    {
      if( l3->offs > wr - l3->rBuf - buf )
        t = MN(t, l3->offs);
      else
        t = wr - l3->rBuf - buf;
      t = MN(t, wr);
    }
  }
  else //if( k < -0.00001 )
  {
    if( t > wr )
      t = wr;
    else if( t < wl + l3->lBuf + buf )
    {
      if( l3->offs < wl + l3->lBuf + buf )
        t = MX(t, l3->offs);
      else
        t = wl + l3->lBuf + buf;
      t = MX(t, wl);
    }
  }

  l3->offs = t;
  l3->pt = l3->CalcPt();
  l3->k = Utils::CalcCurvatureXY(l2->pt, l3->pt, l4->pt);
}

void ClothoidPath::Optimise(
  double   factor,
  int    idx,
  PathPt*   l3,
  const PathPt* l0,
  const PathPt* l1,
  const PathPt* l2,
  const PathPt* l4,
  const PathPt* l5,
  const PathPt* l6,
  int    bumpMod )
{
  Vec3d p0 = l0->pt;//CalcPt();
  Vec3d p1 = l1->pt;//CalcPt();
  Vec3d p2 = l2->pt;//CalcPt();
  Vec3d p3 = l3->pt;//CalcPt();
  Vec3d p4 = l4->pt;//CalcPt();
  Vec3d p5 = l5->pt;//CalcPt();
  Vec3d p6 = l6->pt;//CalcPt();

  double k1 = Utils::CalcCurvatureXY(p1, p2, p3);
  double k2 = Utils::CalcCurvatureXY(p3, p4, p5);

  double length1 = hypot(p3.x - p2.x, p3.y - p2.y);
  double length2 = hypot(p4.x - p3.x, p4.y - p3.y);

  if( k1 * k2 > 0 )
  {
    double k0 = Utils::CalcCurvatureXY(p0, p1, p2);
    double k3 = Utils::CalcCurvatureXY(p4, p5, p6);
    if( k0 * k1 > 0 && k2 * k3 > 0 )
    {
      if( fabs(k0) < fabs(k1) && fabs(k1) * 1.02 < fabs(k2) )
      {
        k1 *= factor;
        k0 *= factor;
      }
      else if( fabs(k0) > fabs(k1) * 1.02 && fabs(k1) > fabs(k2) )
      {
        k1 *= factor;
        k0 *= factor;
      }
    }
  }
  else if( k1 * k2 < 0 )
  {
    double k0 = Utils::CalcCurvatureXY(p0, p1, p2);
    double k3 = Utils::CalcCurvatureXY(p4, p5, p6);
    if( k0 * k1 > 0 && k2 * k3 > 0 )
    {
      if( fabs(k1) < fabs(k2) && fabs(k1) < fabs(k3) )
      {
        k1 = (k1 * 0.25 + k2 * 0.75);
        k0 = (k0 * 0.25 + k3 * 0.75);
      }
      else if( fabs(k2) < fabs(k1) && fabs(k2) < fabs(k0) )
      {
        k2 = (k2 * 0.25 + k1 * 0.75);
        k3 = (k3 * 0.25 + k0 * 0.75);
      }
    }
  }

  double k = (length2 * k1 + length1 * k2) / (length1 + length2);

  double maxSpdK = 0.00175;//60 / (50 * 50); // a = vv/r; r = vv/a; k = a/vv;
  if( k1 * k2 >= 0 && fabs(k1) < maxSpdK && fabs(k2) < maxSpdK )
  {
    k *= 0.9;
  }

  double t = l3->offs;
  Utils::LineCrossesLineXY( l3->Pt(), l3->Norm(), p2, p4 - p2, t );

  double delta = 0.0001;
  double deltaK = Utils::CalcCurvatureXY(p2, l3->Pt() + l3->Norm() * (t + delta), p4);

  if( bumpMod == 1 )
  {
    double f = l3->h <= 0.07 ? 1.00 :
               l3->h <= 0.10 ? 0.97 :
               l3->h <= 0.20 ? 0.90 :
               l3->h <= 0.30 ? 0.80 : 0.70;
    delta *= f;
  }

  t += delta * k / deltaK;

  SetOffset( k, t, l3, l2, l4 );
}

void ClothoidPath::OptimisePath(
  int    step,
  int    nIterations,
  int    bumpMod )
{
  const int NSEG = m_pTrack->GetSize();

  for( int j = 0; j < nIterations; j++ )
  {
    PathPt* l0 = 0;
    PathPt* l1 = &m_pPath[NSEG - 3 * step];
    PathPt* l2 = &m_pPath[NSEG - 2 * step];
    PathPt* l3 = &m_pPath[NSEG - step];
    PathPt* l4 = &m_pPath[0];
    PathPt* l5 = &m_pPath[step];
    PathPt* l6 = &m_pPath[2 * step];

    // go forwards
    int  i = 3 * step;
    int  n = (NSEG + step - 1) / step;
    for( int count = 0; count < n; count++ )
    {
      l0 = l1;
      l1 = l2;
      l2 = l3;
      l3 = l4;
      l4 = l5;
      l5 = l6;
      l6 = &m_pPath[i];

      int  idx = (i + NSEG - 3 * step) % NSEG;
      Optimise( m_factor, idx, l3, l0, l1, l2, l4, l5, l6, bumpMod );

      if( (i += step) >= NSEG )
        i = 0;//i -= m_nSegs;
    }
  }

  // now smooth the values between steps
  if( step > 1 )
    SmoothBetween( step );
}
