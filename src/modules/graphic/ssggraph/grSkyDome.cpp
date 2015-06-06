/***************************************************************************

    file        : grSkyDome.cpp
    copyright   : (C) 2009 by Xavier Bertaux (based on ssgasky plib code)
    web         : http://www.speed-dreams.org
    version     : $Id: grSkyDome.cpp 3162 2010-12-05 13:11:22Z pouillot $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#include <math.h>
#include "grSky.h"

// proportions of max dimensions fed to the build() routine
static const float center_elev = 1.0f;

static const float upper_radius = 0.6f;
static const float upper_elev = 0.15f;

static const float middle_radius = 0.9f;
static const float middle_elev = 0.08f;

static const float lower_radius = 1.0f;
static const float lower_elev = 0.0f;

static const float bottom_radius = 0.8f;
static const float bottom_elev = -0.1f;

static int grSkyDomePreDraw( ssgEntity *e )
{
  ssgLeaf *f = (ssgLeaf *)e;
  if ( f -> hasState () ) f->getState()->apply() ;

  glPushAttrib( GL_DEPTH_BUFFER_BIT | GL_FOG_BIT );

  glDisable( GL_DEPTH_TEST );
  glDisable( GL_FOG );

  return true;
}


static int grSkyDomePostDraw( ssgEntity *e )
{
  glPopAttrib();

  return true;
}


cGrSkyDome::cGrSkyDome( void )
{
  dome_transform = 0;
  asl = 0.0f;
}


cGrSkyDome::~cGrSkyDome( void )
{
  ssgDeRefDelete( dome_transform );
}


// initialize the sky object and connect it into our scene graph
ssgBranch * cGrSkyDome::build( double hscale, double vscale )
{
  sgVec4 color;
  double theta;
  int i;

  // clean-up previous
  ssgDeRefDelete( dome_transform );

  // create new
  dome_transform = new ssgTransform;
  dome_transform->ref();

  // set up the state
  dome_state = new ssgSimpleState();
  dome_state->setShadeModel( GL_SMOOTH );
  dome_state->disable( GL_LIGHTING );
  dome_state->disable( GL_CULL_FACE );
  dome_state->disable( GL_TEXTURE_2D );
  dome_state->enable( GL_COLOR_MATERIAL );
  dome_state->setColourMaterial( GL_AMBIENT_AND_DIFFUSE );
  dome_state->setMaterial( GL_EMISSION, 0, 0, 0, 1 );
  dome_state->setMaterial( GL_SPECULAR, 0, 0, 0, 1 );
  dome_state->disable( GL_BLEND );
  dome_state->disable( GL_ALPHA_TEST );

  // initialize arrays
  center_disk_vl = new ssgVertexArray( 14 );
  center_disk_cl = new ssgColourArray( 14 );

  upper_ring_vl = new ssgVertexArray( 26 );
  upper_ring_cl = new ssgColourArray( 26 );

  middle_ring_vl = new ssgVertexArray( 26 );
  middle_ring_cl = new ssgColourArray( 26 );

  lower_ring_vl = new ssgVertexArray( 26 );
  lower_ring_cl = new ssgColourArray( 26 );

  // initially seed to all blue
  sgSetVec4( color, 0.0, 0.0, 1.0, 1.0 );

  // generate the raw vertex data
  sgVec3 center_vertex;
  sgVec3 upper_vertex[12];
  sgVec3 middle_vertex[12];
  sgVec3 lower_vertex[12];
  sgVec3 bottom_vertex[12];

  sgSetVec3( center_vertex, 0.0, 0.0, (float)(center_elev * vscale));

  for ( i = 0; i < 12; i++ ) 
  {
    theta = (i * 30.0) * SGD_DEGREES_TO_RADIANS;

    sgSetVec3( upper_vertex[i],
      (float)(cos(theta) * upper_radius * hscale),
      (float)(sin(theta) * upper_radius * hscale),
      (float)(upper_elev * vscale));

    sgSetVec3( middle_vertex[i],
      (float)(cos(theta) * middle_radius * hscale),
      (float)(sin(theta) * middle_radius * hscale),
      (float)(middle_elev * vscale));

    sgSetVec3( lower_vertex[i],
      (float)(cos(theta) * lower_radius * hscale),
      (float)(sin(theta) * lower_radius * hscale),
      (float)(lower_elev * vscale));

    sgSetVec3( bottom_vertex[i],
      (float)(cos(theta) * bottom_radius * hscale),
      (float)(sin(theta) * bottom_radius * hscale),
      (float)(bottom_elev * vscale));
  }

  // generate the center disk vertex/color arrays
  center_disk_vl->add( center_vertex );
  center_disk_cl->add( color );
  for ( i = 11; i >= 0; i-- ) 
  {
    center_disk_vl->add( upper_vertex[i] );
    center_disk_cl->add( color );
  }
  center_disk_vl->add( upper_vertex[11] );
  center_disk_cl->add( color );

  // generate the upper ring
  for ( i = 0; i < 12; i++ ) 
  {
    upper_ring_vl->add( middle_vertex[i] );
    upper_ring_cl->add( color );

    upper_ring_vl->add( upper_vertex[i] );
    upper_ring_cl->add( color );
  }
  upper_ring_vl->add( middle_vertex[0] );
  upper_ring_cl->add( color );

  upper_ring_vl->add( upper_vertex[0] );
  upper_ring_cl->add( color );

  // generate middle ring
  for ( i = 0; i < 12; i++ ) 
  {
    middle_ring_vl->add( lower_vertex[i] );
    middle_ring_cl->add( color );

    middle_ring_vl->add( middle_vertex[i] );
    middle_ring_cl->add( color );
  }
  middle_ring_vl->add( lower_vertex[0] );
  middle_ring_cl->add( color );

  middle_ring_vl->add( middle_vertex[0] );
  middle_ring_cl->add( color );

  // generate lower ring
  for ( i = 0; i < 12; i++ ) 
  {
    lower_ring_vl->add( bottom_vertex[i] );
    lower_ring_cl->add( color );

    lower_ring_vl->add( lower_vertex[i] );
    lower_ring_cl->add( color );
  }
  lower_ring_vl->add( bottom_vertex[0] );
  lower_ring_cl->add( color );

  lower_ring_vl->add( lower_vertex[0] );
  lower_ring_cl->add( color );

  // force a repaint of the sky colors with ugly defaults
  sgVec3 fog_color;
  sgSetVec3( fog_color, 1.0, 1.0, 1.0 );
  repaint( color, fog_color, 0.0, 5000.0 );

  // build the ssg scene graph sub tree for the sky and connected
  // into the provide scene graph branch
  ssgVtxTable *center_disk, *upper_ring, *middle_ring, *lower_ring;

  center_disk = new ssgVtxTable( GL_TRIANGLE_FAN, 
  center_disk_vl, NULL, NULL, center_disk_cl );

  upper_ring = new ssgVtxTable( GL_TRIANGLE_STRIP, 
  upper_ring_vl, NULL, NULL, upper_ring_cl );

  middle_ring = new ssgVtxTable( GL_TRIANGLE_STRIP, 
  middle_ring_vl, NULL, NULL, middle_ring_cl );

  lower_ring = new ssgVtxTable( GL_TRIANGLE_STRIP, 
  lower_ring_vl, NULL, NULL, lower_ring_cl );

  center_disk->setState( dome_state );
  upper_ring->setState( dome_state );
  middle_ring->setState( dome_state );
  lower_ring->setState( dome_state );

  dome_transform->addKid( center_disk );
  dome_transform->addKid( upper_ring );
  dome_transform->addKid( middle_ring );
  dome_transform->addKid( lower_ring );

  // not entirely satisfying.  We are depending here that the first
  // thing we add to a parent is the first drawn
  center_disk->setCallback( SSG_CALLBACK_PREDRAW, grSkyDomePreDraw );
  center_disk->setCallback( SSG_CALLBACK_POSTDRAW, grSkyDomePostDraw );

  upper_ring->setCallback( SSG_CALLBACK_PREDRAW, grSkyDomePreDraw );
  upper_ring->setCallback( SSG_CALLBACK_POSTDRAW, grSkyDomePostDraw );

  middle_ring->setCallback( SSG_CALLBACK_PREDRAW, grSkyDomePreDraw );
  middle_ring->setCallback( SSG_CALLBACK_POSTDRAW, grSkyDomePostDraw );

  lower_ring->setCallback( SSG_CALLBACK_PREDRAW, grSkyDomePreDraw );
  lower_ring->setCallback( SSG_CALLBACK_POSTDRAW, grSkyDomePostDraw );

  return dome_transform;
}

bool cGrSkyDome::repositionFlat( sgVec3 p, double spin )
{
  sgMat4 T, SPIN;
  sgVec3 axis;

  // Translate to view position
  sgMakeTransMat4( T, p );

  // Rotate to proper orientation
  sgSetVec3( axis, 0.0, 0.0, 1.0 );
  sgMakeRotMat4( SPIN, (float)(spin * SGD_RADIANS_TO_DEGREES), axis );

  sgMat4 TRANSFORM;

  sgCopyMat4( TRANSFORM, T );
  sgPreMultMat4( TRANSFORM, SPIN );

  sgCoord skypos;
  sgSetCoord( &skypos, TRANSFORM );

  dome_transform->setTransform( &skypos );

  return true;
}

static void fade_to_black( sgVec4 sky_color[], float asl, int count) 
{
    const float ref_asl = 10000.0f;
    const sgVec3 space_color = {0.0f, 0.0f, 0.0f};
    float d = exp( - asl / ref_asl );
    for(int i = 0; i < count ; i++)
        sgLerpVec3( sky_color[i], sky_color[i], space_color, 1.0f - d);
}

bool cGrSkyDome::reposition( sgVec3 p, double lon, double lat, double spin )
{
  sgMat4 T, LON, LAT, SPIN;
  sgVec3 axis;

  // Translate to view position
  sgMakeTransMat4( T, p );

  // Rotate to proper orientation
  sgSetVec3( axis, 0.0, 0.0, 1.0 );
  sgMakeRotMat4( LON, (float)(lon * SGD_RADIANS_TO_DEGREES), axis );

  sgSetVec3( axis, 0.0, 1.0, 0.0 );
  sgMakeRotMat4( LAT, (float)(90.0 - lat * SGD_RADIANS_TO_DEGREES), axis );

  sgSetVec3( axis, 0.0, 0.0, 1.0 );
  sgMakeRotMat4( SPIN, (float)(spin * SGD_RADIANS_TO_DEGREES), axis );

  sgMat4 TRANSFORM;

  sgCopyMat4( TRANSFORM, T );
  sgPreMultMat4( TRANSFORM, LON );
  sgPreMultMat4( TRANSFORM, LAT );
  sgPreMultMat4( TRANSFORM, SPIN );

  sgCoord skypos;
  sgSetCoord( &skypos, TRANSFORM );

  dome_transform->setTransform( &skypos );
  asl = - skypos.xyz[2];

  return true;
}


bool cGrSkyDome::repaint( sgVec4 sky_color, sgVec4 fog_color, double sol_angle, double vis )
{
  double diff, prev_sun_angle = 999.0;
  sgVec3 outer_param, outer_amt, outer_diff;
  sgVec3 middle_param, middle_amt, middle_diff;
  int i, j;

  if (prev_sun_angle == sol_angle)
        return true;

  prev_sun_angle = sol_angle;

  //sol_angle *= SGD_RADIANS_TO_DEGREES;

  // Check for sunrise/sunset condition
  if ((sol_angle > 80.0)) 
  { 
    sgSetVec3( outer_param,
      (float)((10.0 - fabs(90.0 - sol_angle)) / 20.0),
      (float)((10.0 - fabs(90.0 - sol_angle)) / 40.0),
      (float)(-(10.0 - fabs(90.0 - sol_angle)) / 30.0));

    sgSetVec3( middle_param,
      (float)((10.0 - fabs(90.0 - sol_angle)) / 40.0),
      (float)((10.0 - fabs(90.0 - sol_angle)) / 80.0),
      0.0 );

    sgScaleVec3( outer_diff, outer_param, 1.0f / 6.0f );

    sgScaleVec3( middle_diff, middle_param, 1.0f / 6.0f );
  }
  else 
  {
    sgSetVec3( outer_param, 0.0, 0.0, 0.0 );
    sgSetVec3( middle_param, 0.0, 0.0, 0.0 );

    sgSetVec3( outer_diff, 0.0, 0.0, 0.0 );
    sgSetVec3( middle_diff, 0.0, 0.0, 0.0 );
  }

  // calculate transition colors between sky and fog
  sgCopyVec3( outer_amt, outer_param );
  sgCopyVec3( middle_amt, middle_param );

  //
  // First, recalulate the basic colors
  //

  sgVec4 center_color;
  sgVec4 upper_color[12];
  sgVec4 middle_color[12];
  sgVec4 lower_color[12];
  sgVec4 bottom_color[12];

  double vis_factor, cvf = vis;

  if (cvf > 45000)
        cvf = 45000;

  vis_factor = (vis - 1000.0) / 2000.0;
  if ( vis_factor < 0.0 ) 
  {
        vis_factor = 0.0;
  } else if ( vis_factor > 1.0) 
  {
        vis_factor = 1.0;
  }

  for ( j = 0; j < 3; j++ ) 
  {
    diff = sky_color[j] - fog_color[j];
    center_color[j] = sky_color[j]; // - (float)(diff * ( 1.0 - vis_factor ));
  }

  for ( i = 0; i < 6; i++ ) 
  {
    for ( j = 0; j < 3; j++ ) 
    {
      double saif = sol_angle/SG_PI;
      diff = sky_color[j] - fog_color[j] * (0.8 + j * 0.2) * (0.8 + saif - ((6-i)/10));

      upper_color[i][j] = sky_color[j] - (float)(diff * ( 1.0 - vis_factor * (0.7 + 0.3 * cvf/45000)));
      middle_color[i][j] = sky_color[j] - (float)(diff * ( 1.0 - vis_factor * (0.1 + 0.85 * cvf/45000)))
			 + middle_amt[j];
      lower_color[i][j] = fog_color[j] + outer_amt[j];

      if ( upper_color[i][j] > 1.0 ) { upper_color[i][j] = 1.0; }
      if ( upper_color[i][j] < 0.0 ) { upper_color[i][j] = 0.0; }
      if ( middle_color[i][j] > 1.0 ) { middle_color[i][j] = 1.0; }
      if ( middle_color[i][j] < 0.0 ) { middle_color[i][j] = 0.0; }
      if ( lower_color[i][j] > 1.0 ) { lower_color[i][j] = 1.0; }
      if ( lower_color[i][j] < 0.0 ) { lower_color[i][j] = 0.0; }
    }
    upper_color[i][3] = middle_color[i][3] = lower_color[i][3] = 1.0;

    for ( j = 0; j < 3; j++ ) 
    {
      outer_amt[j] -= outer_diff[j];
      middle_amt[j] -= middle_diff[j];
    }
  }

  sgSetVec3( outer_amt, 0.0, 0.0, 0.0 );
  sgSetVec3( middle_amt, 0.0, 0.0, 0.0 );

  for ( i = 6; i < 12; i++ ) 
  {
    for ( j = 0; j < 3; j++ ) 
    {
	  double saif = sol_angle/SG_PI;
      diff = (sky_color[j] - fog_color[j]) * (0.8 + j * 0.2) * (0.8 + saif - ((-i+12)/10));

      upper_color[i][j] = sky_color[j] - (float)(diff *
                                 ( 1.0 - vis_factor * (0.7 + 0.3 * cvf/45000)));
      middle_color[i][j] = sky_color[j] - (float)(diff *
                                 ( 1.0 - vis_factor * (0.1 + 0.85 * cvf/45000)))
								  + middle_amt[j];
      lower_color[i][j] = fog_color[j] + outer_amt[j];

      if ( upper_color[i][j] > 1.0 ) { upper_color[i][j] = 1.0; }
      if ( upper_color[i][j] < 0.0 ) { upper_color[i][j] = 0.0; }
      if ( middle_color[i][j] > 1.0 ) { middle_color[i][j] = 1.0; }
      if ( middle_color[i][j] < 0.0 ) { middle_color[i][j] = 0.0; }
      if ( lower_color[i][j] > 1.0 ) { lower_color[i][j] = 1.0; }
      if ( lower_color[i][j] < 0.0 ) { lower_color[i][j] = 0.0; }
    }
    upper_color[i][3] = middle_color[i][3] = lower_color[i][3] = 1.0;

    for ( j = 0; j < 3; j++ ) 
    {
      outer_amt[j] += outer_diff[j];
      middle_amt[j] += middle_diff[j];
    }
  }

  fade_to_black( (sgVec4 *) center_color, asl * center_elev, 1);
  fade_to_black( upper_color, (asl+0.05f) * upper_elev, 12);
  fade_to_black( middle_color, (asl+0.05f) * middle_elev, 12);
  fade_to_black( lower_color, (asl+0.05f) * lower_elev, 12);

  for ( i = 0; i < 12; i++ ) 
  {
    sgCopyVec4( bottom_color[i], fog_color );
  }

  //
  // Second, assign the basic colors to the object color arrays
  //

  float *slot;
  int counter;

  // update the center disk color arrays
  counter = 0;
  slot = center_disk_cl->get( counter++ );
  // sgVec4 red;
  // sgSetVec4( red, 1.0, 0.0, 0.0, 1.0 );
  sgCopyVec4( slot, center_color );
  for ( i = 11; i >= 0; i-- ) 
  {
    slot = center_disk_cl->get( counter++ );
    sgCopyVec4( slot, upper_color[i] );
  }
  slot = center_disk_cl->get( counter++ );
  sgCopyVec4( slot, upper_color[11] );

  // generate the upper ring
  counter = 0;
  for ( i = 0; i < 12; i++ ) 
  {
    slot = upper_ring_cl->get( counter++ );
    sgCopyVec4( slot, middle_color[i] );

    slot = upper_ring_cl->get( counter++ );
    sgCopyVec4( slot, upper_color[i] );
  }
  slot = upper_ring_cl->get( counter++ );
  sgCopyVec4( slot, middle_color[0] );

  slot = upper_ring_cl->get( counter++ );
  sgCopyVec4( slot, upper_color[0] );

  // generate middle ring
  counter = 0;
  for ( i = 0; i < 12; i++ ) 
  {
    slot = middle_ring_cl->get( counter++ );
    sgCopyVec4( slot, lower_color[i] );

    slot = middle_ring_cl->get( counter++ );
    sgCopyVec4( slot, middle_color[i] );
  }
  slot = middle_ring_cl->get( counter++ );
  sgCopyVec4( slot, lower_color[0] );

  slot = middle_ring_cl->get( counter++ );
  sgCopyVec4( slot, middle_color[0] );

  // generate lower ring
  counter = 0;
  for ( i = 0; i < 12; i++ ) 
  {
    slot = lower_ring_cl->get( counter++ );
    sgCopyVec4( slot, bottom_color[i] );

    slot = lower_ring_cl->get( counter++ );
    sgCopyVec4( slot, lower_color[i] );
  }
  slot = lower_ring_cl->get( counter++ );
  sgCopyVec4( slot, bottom_color[0] );

  slot = lower_ring_cl->get( counter++ );
  sgCopyVec4( slot, lower_color[0] );

  return true;
}
