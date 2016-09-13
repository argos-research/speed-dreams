/***************************************************************************

    file        : grStars.cpp
    copyright   : (C) 2009 by Xavier Bertaux (based on ssgasky plib code)
    web         : http://www.speed-dreams.org
    version     : $Id: grStars.cpp 6171 2015-10-17 02:43:08Z torcs-ng $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "grSky.h"

#define SD_2PI   6.28318530717958647692
#define SD_PI_2  1.57079632679489661923

static int grStarPreDraw( ssgEntity *e )
{
  ssgLeaf *f = (ssgLeaf *)e;
  if ( f -> hasState () ) f->getState()->apply() ;

  glPushAttrib( GL_DEPTH_BUFFER_BIT | GL_FOG_BIT );

  glDisable( GL_FOG );

  return true;
}


static int grStarPostDraw( ssgEntity *e )
{
    glPopAttrib();

    return true;
}


cGrStars::cGrStars( void ) :
  stars_transform(0),
  old_phase(-1)
{
}

  
cGrStars::~cGrStars( void )
{
  ssgDeRefDelete( stars_transform );
}


ssgBranch * cGrStars::build( int num, sgdVec3 *star_data, double star_dist )
{
  sgVec4 color;

  // clean-up previous
  ssgDeRefDelete( stars_transform );

  // create new
  stars_transform = new ssgTransform;
  stars_transform->ref();

  if ( star_data == NULL )
  {
	  if (num > 0)
		  ulSetError(UL_WARNING, "null star data passed to cGrStars::build()");
	  else
		  return stars_transform;
  }

  // set up the orb state
  state = new ssgSimpleState();
  state->disable( GL_LIGHTING );
  state->disable( GL_CULL_FACE );
  state->disable( GL_TEXTURE_2D );
  state->enable( GL_COLOR_MATERIAL );
  state->setColourMaterial( GL_AMBIENT_AND_DIFFUSE );
  state->setMaterial( GL_EMISSION, 0, 0, 0, 1 );
  state->setMaterial( GL_SPECULAR, 0, 0, 0, 1 );
  state->enable( GL_BLEND );
  state->disable( GL_ALPHA_TEST );

  vl = new ssgVertexArray( num );
  cl = new ssgColourArray( num );
  // cl = new ssgColourArray( 1 );
  // sgSetVec4( color, 1.0, 1.0, 1.0, 1.0 );
  // cl->add( color );

  // Build ssg structure
  sgVec3 p;
  for ( int i = 0; i < num; ++i ) 
  {
    // position seeded to arbitrary values
    sgSetVec3( p, 
      (float)( star_dist * cos( star_data[i][0] )
        * cos( star_data[i][1] )),
      (float)( star_dist * sin( star_data[i][0] )
        * cos( star_data[i][1] )),
      (float)( star_dist * sin( star_data[i][1] )));
    vl->add( p );

    // color (magnitude)
    sgSetVec4( color, 1.0, 1.0, 1.0, 1.0 );
    cl->add( color );
  }

  ssgLeaf *stars_obj = 
    new ssgVtxTable ( GL_POINTS, vl, NULL, NULL, cl );
  stars_obj->setState( state );
  stars_obj->setCallback( SSG_CALLBACK_PREDRAW, grStarPreDraw );
  stars_obj->setCallback( SSG_CALLBACK_POSTDRAW, grStarPostDraw );

  stars_transform->addKid( stars_obj );

  return stars_transform;
}


bool cGrStars::reposition( sgVec3 p, double angle )
{
  sgMat4 T1, GST;
  sgVec3 axis;

  sgMakeTransMat4( T1, p );

  sgSetVec3( axis, 0.0, 0.0, -1.0 );
  sgMakeRotMat4( GST, (float)angle, axis );

  sgMat4 TRANSFORM;
  sgCopyMat4( TRANSFORM, T1 );
  sgPreMultMat4( TRANSFORM, GST );

  sgCoord skypos;
  sgSetCoord( &skypos, TRANSFORM );

  stars_transform->setTransform( &skypos );

  return true;
}


bool cGrStars::repaint( double sol_angle, int num, sgdVec3 *star_data )
{
  double mag, nmag, alpha, factor, cutoff;
  float *color;

  int phase;

  // determine which star structure to draw
  if ( sol_angle > ( SD_PI_2 + 10.0 * SGD_DEGREES_TO_RADIANS )) 
  {
    // deep night
    factor = 1.0;
    cutoff = 4.5;
    phase = 0;
  }
  else if ( sol_angle > ( SD_PI_2 + 8.8 * SGD_DEGREES_TO_RADIANS )) 
  {
    factor = 1.0;
    cutoff = 3.8;
    phase = 1;
  }
  else if ( sol_angle > ( SD_PI_2 + 7.5 * SGD_DEGREES_TO_RADIANS )) 
  {
    factor = 0.95;
    cutoff = 3.1;
    phase = 2;
  }
  else if ( sol_angle > ( SD_PI_2 + 7.0 * SGD_DEGREES_TO_RADIANS )) 
  {
    factor = 0.9;
    cutoff = 2.4;
    phase = 3;
  }
  else if ( sol_angle > ( SD_PI_2 + 6.5 * SGD_DEGREES_TO_RADIANS )) 
  {
    factor = 0.85;
    cutoff = 1.8;
    phase = 4;
  }
  else if ( sol_angle > ( SD_PI_2 + 6.0 * SGD_DEGREES_TO_RADIANS )) 
  {
    factor = 0.8;
    cutoff = 1.2;
    phase = 5;
  }
  else if ( sol_angle > ( SD_PI_2 + 5.5 * SGD_DEGREES_TO_RADIANS )) 
  {
    factor = 0.75;
    cutoff = 0.6;
    phase = 6;
  }
  else 
  {
    // early dusk or late dawn
    factor = 0.7;
    cutoff = 0.0;
    phase = 7;
  }

  if( phase != old_phase ) 
  {
    old_phase = phase;
    for ( int i = 0; i < num; ++i ) 
    {
      // if ( star_data[i][2] < min ) { min = star_data[i][2]; }
      // if ( star_data[i][2] > max ) { max = star_data[i][2]; }

      // magnitude ranges from -1 (bright) to 4 (dim).  The
      // range of star and planet magnitudes can actually go
      // outside of this, but for our purpose, if it is brighter
      // that -1, we'll color it full white/alpha anyway and 4
      // is a convenient cutoff point which keeps the number of
      // stars drawn at about 500.

      // color (magnitude)
      mag = star_data[i][2];
      if ( mag < cutoff ) 
      {
        nmag = ( 4.5 - mag ) / 5.5;		// translate to 0 ... 1.0 scale
        // alpha = nmag * 0.7 + 0.3;	// translate to a 0.3 ... 1.0 scale
        alpha = nmag * 0.85 + 0.15;		// translate to a 0.15 ... 1.0 scale
        alpha *= factor;				// dim when the sun is brighter
      }
      else 
      {
        alpha = 0.0;
      }

      if (alpha > 1.0) { alpha = 1.0; }
      if (alpha < 0.0) { alpha = 0.0; }

      color = cl->get( i );
      sgSetVec4( color, 1.0, 1.0, 1.0, (float)alpha );
    }
  }

  return true;
}
