/***************************************************************************

    file        : grSphere.cpp
    copyright   : (C) 2009 by Xavier Bertaux (based on ssgasky plib code)
    web         : http://www.speed-dreams.org
    version     : $Id: grSphere.cpp 3162 2010-12-05 13:11:22Z pouillot $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#include "plib/ul.h"
#include "grSphere.h"

#define SGD_2PI      6.28318530717958647692

ssgBranch *grMakeSphere(
  ssgSimpleState *state, ssgColourArray *cl,
  float radius, int slices, int stacks,
  ssgCallback predraw, ssgCallback postdraw )
{
  double rho, drho, theta, dtheta;
  float x, y, z;
  float s, t, ds, dt;
  int i, j, imin, imax;
  float nsign = 1.0;
  ssgBranch *sphere = new ssgBranch;
  sgVec2 vec2;
  sgVec3 vec3;

  drho = SGD_PI / (float)stacks;
  dtheta = (2.0 * SGD_PI) / (float)slices;

  /* texturing: s goes from 0.0/0.25/0.5/0.75/1.0 at +y/+x/-y/-x/+y
     axis t goes from -1.0/+1.0 at z = -radius/+radius (linear along
     longitudes) cannot use triangle fan on texturing (s coord. at
     top/bottom tip varies) */

  ds = 1.0f / slices;
  dt = 1.0f / stacks;
  t = 1.0;  /* because loop now runs from 0 */
  imin = 0;
  imax = stacks;

  /* build slices as quad strips */
  for ( i = imin; i < imax; i++ ) 
  {
    ssgVertexArray   *vl = new ssgVertexArray();
    ssgNormalArray   *nl = new ssgNormalArray();
    ssgTexCoordArray *tl = new ssgTexCoordArray();

    rho = i * drho;
    s = 0.0;
    for ( j = 0; j <= slices; j++ ) 
    {
      theta = (j == slices) ? 0.0 : j * dtheta;
      x = (float)(-sin(theta) * sin(rho));
      y = (float)(cos(theta) * sin(rho));
      z = (float)(nsign * cos(rho));

      sgSetVec3( vec3, x*nsign, y*nsign, z*nsign );
      sgNormalizeVec3( vec3 );
      nl->add( vec3 );

      sgSetVec2( vec2, s, t );
      tl->add( vec2 );

      sgSetVec3( vec3, x*radius, y*radius, z*radius );
      vl->add( vec3 );

      x = (float)(-sin(theta) * sin(rho+drho));
      y = (float)(cos(theta) * sin(rho+drho));
      z = (float)(nsign * cos(rho+drho));

      sgSetVec3( vec3, x*nsign, y*nsign, z*nsign );
      sgNormalizeVec3( vec3 );
      nl->add( vec3 );

      sgSetVec2( vec2, s, t-dt );
      tl->add( vec2 );
      s += ds;

      sgSetVec3( vec3, x*radius, y*radius, z*radius );
      vl->add( vec3 );
    }

    ssgLeaf *slice = 
      new ssgVtxTable ( GL_TRIANGLE_STRIP, vl, nl, tl, cl );

    if ( vl->getNum() != nl->getNum() ) 
    {
      ulSetError(UL_FATAL, "bad sphere1\n");
      exit(-1);
    }
    if ( vl->getNum() != tl->getNum() ) 
    {
      ulSetError(UL_FATAL, "bad sphere2\n");
      exit(-1);
    }
    slice->setState( state );
    slice->setCallback( SSG_CALLBACK_PREDRAW, predraw );
    slice->setCallback( SSG_CALLBACK_POSTDRAW, postdraw );

    sphere->addKid( slice );

    t -= dt;
  }

  return sphere;
}

