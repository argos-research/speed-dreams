/***************************************************************************

    file                     : OsgMath.h
    created                  : Fri Aug 18 00:00:41 CEST 2012
    copyright                : (C) 2012 by Xavier Bertaux
    email                    : bertauxx@yahoo.fr
    version                  : $Id: OsgMath.h 4693 2012-04-13 03:12:09Z torcs-ng $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include <osg/Vec3>

#include "OsgMath.h"

void osgXformPnt3(osg::Vec3 dst, const osg::Vec3 src, const osgMat4 mat)
{
	float t0 = src[0];
	float t1 = src[1];
	float t2 = src[2];
	
	dst[0] = t0*mat[0][0] + t1*mat[1][0] + t2*mat[2][0] + mat[3][0];	
	dst[1] = t0*mat[0][1] + t1*mat[1][1] + t2*mat[2][1] + mat[3][1];	
	dst[2] = t0*mat[0][2] + t1*mat[1][2] + t2*mat[2][2] + mat[3][2];
}

void osgMakeCoordMat4 ( osgMat4 m, const float x, const float y, const float z, const float h, const float p, const float r )
{
  float ch, sh, cp, sp, cr, sr, srsp, crsp, srcp ;

  if ( h == SD_ZERO )
  {
    ch = SD_ONE ;
    sh = SD_ZERO ;
  }
  else
  {
    sh = sdSin ( h ) ;
    ch = sdCos ( h ) ;
  }

  if ( p == SD_ZERO )
  {
    cp = SD_ONE ;
    sp = SD_ZERO ;
  }
  else
  {
    sp = sdSin ( p ) ;
    cp = sdCos ( p ) ;
  }

  if ( r == SD_ZERO )
  {
    cr   = SD_ONE ;
    sr   = SD_ZERO ;
    srsp = SD_ZERO ;
    srcp = SD_ZERO ;
    crsp = sp ;
  }
  else
  {
    sr   = sdSin ( r ) ;
    cr   = sdCos ( r ) ;
    srsp = sr * sp ;
    crsp = cr * sp ;
    srcp = sr * cp ;
  }

  m[0][0] = (float)(  ch * cr - sh * srsp ) ;
  m[1][0] = (float)( -sh * cp ) ;
  m[2][0] = (float)(  sr * ch + sh * crsp ) ;
  m[3][0] =  x ;

  m[0][1] = (float)( cr * sh + srsp * ch ) ;
  m[1][1] = (float)( ch * cp ) ;
  m[2][1] = (float)( sr * sh - crsp * ch ) ;
  m[3][1] =  y ;

  m[0][2] = (float)( -srcp ) ;
  m[1][2] = (float)(  sp ) ;
  m[2][2] = (float)(  cr * cp ) ;
  m[3][2] =  z ;

  m[0][3] =  SD_ZERO ;
  m[1][3] =  SD_ZERO ;
  m[2][3] =  SD_ZERO ;
  m[3][3] =  SD_ONE ;
}



