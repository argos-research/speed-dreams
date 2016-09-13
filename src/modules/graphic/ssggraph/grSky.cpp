/***************************************************************************

    file        : grSky.cpp
    copyright   : (C) 2009 by Xavier Bertaux (based on ssgasky plib code)
    web         : http://www.speed-dreams.org
    version     : $Id: grSky.cpp 6152 2015-09-27 23:38:33Z torcs-ng $

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

// Used for rise/set effects (flat earth - no rotation of skydome considered here )
void calc_celestial_angles( const sgVec3 body, const sgVec3 view, double& angle, double& rotation )
{
	sgVec3 pos;
	sgSubVec3(pos, body, view);
	angle = (90*SGD_DEGREES_TO_RADIANS) - atan2(pos[2], sqrt(pos[0]*pos[0] + pos[1]*pos[1]));
	rotation = (90*SGD_DEGREES_TO_RADIANS) - atan2(pos[0], pos[1]);
}


cGrSky::cGrSky( void )
{
	dome = 0;
	sun = 0;
	moon = 0;
	planets = 0;
	stars = 0;
	pre_root = 0;
	post_root = 0;

	effective_visibility = visibility = 10000.0;

	// near cloud visibility state variables
	in_puff = false;
	puff_length = 0;
	puff_progression = 0;
	ramp_up = 0.15;
	ramp_down = 0.15;
}


cGrSky::~cGrSky( void )
{
  delete dome;
  delete sun;
  delete moon;
  delete planets;
  delete stars;
  delete pre_root;
  delete post_root;
}


void cGrSky::build( double h_radius, double v_radius,
					 double sun_size, double sun_dist,
					 double moon_size, double moon_dist,
				     int nplanets, sgdVec3 *planet_data,
		     int nstars, sgdVec3 *star_data )
{
  delete dome;
  delete sun;
  delete moon;
  delete planets;
  delete stars;
  delete pre_root;
  delete post_root;
  clouds.removeAll();

  // build new
  pre_root = new ssgRoot;
  post_root = new ssgRoot;

  pre_selector = new ssgSelector;
  post_selector = new ssgSelector;

  pre_transform = new ssgTransform;
  post_transform = new ssgTransform;

  sun_transform = new ssgTransform;
  moon_transform = new ssgTransform;
  stars_transform = new ssgTransform;

  dome = new cGrSkyDome;
  pre_transform -> addKid( dome->build( h_radius, v_radius ) );

  planets = new cGrStars;
  stars_transform -> addKid( planets->build( nplanets, planet_data, h_radius ) );

  stars = new cGrStars;
  stars_transform -> addKid( stars->build( nstars, star_data, h_radius ) );

  moon = new cGrMoon;
  moon_transform ->addKid( moon->build( moon_size));
  moon->setMoonDist( moon_dist );

  sun = new cGrSun;
  sun_transform -> addKid( sun->build( sun_size));
  sun->setSunDistance( sun_dist );

  pre_transform -> addKid( stars_transform );
  pre_transform -> addKid( moon_transform );
  pre_transform -> addKid( sun_transform );

  pre_selector->addKid( pre_transform );
  pre_selector->clrTraversalMaskBits( SSGTRAV_HOT );

  post_selector->addKid( post_transform );
  post_selector->clrTraversalMaskBits( SSGTRAV_HOT );

  pre_root->addKid( pre_selector );
  post_root->addKid( post_selector );
}


cGrCloudLayer*
cGrSky::addCloud( const char *cloud_tex_path, float span, float elevation, float thickness, float transition )
{
  cGrCloudLayer* cloud = new cGrCloudLayer;
  cloud->build ( cloud_tex_path, span, elevation, thickness, transition );
  clouds.add( cloud );
  return cloud;
}


cGrCloudLayer*
cGrSky::addCloud( ssgSimpleState *cloud_state, float span, float elevation, float thickness, float transition )
{
  cGrCloudLayer* cloud = new cGrCloudLayer;
  cloud->build ( cloud_state, span, elevation, thickness, transition );
  clouds.add( cloud );
  return cloud;
}


bool cGrSky::repositionFlat( sgVec3 view_pos, double spin, double dt )
{
	int i;
	double angle;
	double rotation;
	sgCoord pos;

	// Calc angles for rise/set effects

	sun->reposition( view_pos, 0 );
	moon->reposition( view_pos, 0 );

    sun->getSunPosition ( & pos );
    calc_celestial_angles( pos.xyz, view_pos, angle, rotation );
    sun->setSunAngle( angle );
    sun->setSunRotation( rotation );

    moon->getMoonPosition ( & pos );
    calc_celestial_angles( pos.xyz, view_pos, angle, rotation );
    moon->setMoonAngle( angle );
    moon->setMoonRotation( rotation );

	for ( i = 0; i < clouds.getNum (); i++ )
	{
		clouds.get(i)->repositionFlat( view_pos, dt );
	}

	planets->reposition( view_pos, 0 );
	stars->reposition( view_pos, 0 );
	dome->repositionFlat( view_pos, sun->getSunRotation() );

	return true;
}


bool cGrSky::reposition( sgVec3 view_pos, sgVec3 zero_elev, sgVec3 view_up, double lon, double lat, double alt, double spin, double gst, double dt )
{
  int i;

  double angle = gst * 15;	// degrees

  dome->reposition( zero_elev, lon, lat, spin );

  for ( i = 0; i < clouds.getNum (); i++ )
    clouds.get(i)->reposition( zero_elev, view_up, lon, lat, alt, dt );

  moon->reposition(view_pos, angle);
  sun->reposition( view_pos, angle);
  planets->reposition( view_pos, angle );
  stars->reposition( view_pos, angle );

  return true;
}


bool cGrSky::repaint( sgVec4 sky_color, sgVec4 fog_color, sgVec4 cloud_color, double sol_angle,
                       double moon_angle, int nplanets, sgdVec3 *planet_data,
                       int nstars, sgdVec3 *star_data )
{
  int i;

  if ( effective_visibility > 300.0 )
  {
    // turn on sky
    enable();

    dome->repaint( sky_color, fog_color, sol_angle, effective_visibility );

	moon->repaint();
	sun->repaint( sol_angle, effective_visibility );

    for ( i = 0; i < clouds.getNum (); i++ )
      clouds.get(i)->repaint( cloud_color );


    planets->repaint( sol_angle, nplanets, planet_data );
    stars->repaint( sol_angle, nstars, star_data );

  }
  else
  {
    // turn off sky
    disable();
  }

  return true;
}


void cGrSky::preDraw()
{
  ssgCullAndDraw( pre_root );
}


void cGrSky::postDraw( float alt )
{
	// Sort clouds in order of distance from alt (furthest to closest)
	int i, j;
	int num = clouds.getNum ();
	if ( num > 0 )
	{
		// Initialise cloud index
		int *index = new int [ num ];
		for ( i = 0; i < num; i++ )
		{
			index [i] = i;
		}

		// Sort cloud index
		int temp;   // holding variable
		for ( i = 0; i < ( num - 1 ); i++ )    // to represent element to be compared
		{
			for( j = ( i + 1 ); j < num; j++ )   // to represent the rest of the elements
			{
				float d1 = (float)(fabs(alt - clouds.get(i)->getElevation()));
				float d2 = (float)(fabs(alt - clouds.get(j)->getElevation()));

				if (d1 < d2)
				{
					temp = index[i];
					index[i] = index[j];
					index[j] = temp;
				}
			}
		}

		float slop = 5.0; // if we are closer than this to a cloud layer, don't draw cloud

		glDepthMask( GL_FALSE );
        glStencilFunc( GL_EQUAL, 1, 1 );
        glStencilOp( GL_KEEP, GL_KEEP, GL_KEEP );

		for ( int i = 0; i < num; i++ )
		{
			cGrCloudLayer *cloud = clouds.get(index[i]);

			float asl = cloud->getElevation();
			float thickness = cloud->getThickness();

			// draw cloud only if below or above cloud layer
			if ( alt < asl - slop || alt > asl + thickness + slop )
				cloud->draw();
		}

		glDepthMask( GL_TRUE );
        glDisable( GL_STENCIL_TEST );

		delete [] index;
	}
}

void cGrSky::modifyVisibility( float alt, float time_factor )
{
  float effvis = visibility;

  for ( int i = 0; i < clouds.getNum (); ++i )
  {
    cGrCloudLayer *cloud = clouds.get(i);

    if ( cloud->isEnabled() )
    {
      float asl = cloud->getElevation();
      float thickness = cloud->getThickness();
      float transition = cloud->getTransition();

      float ratio = 1.0;

      if ( alt < asl - transition )
      {
        // below cloud layer
        ratio = 1.0;
      }
      else if ( alt < asl )
      {
        // in lower transition
        ratio = (asl - alt) / transition;
      }
      else if ( alt < asl + thickness )
      {
        // in cloud layer
        ratio = 0.0;
      }
      else if ( alt < asl + thickness + transition )
      {
        // in upper transition
        ratio = (alt - (asl + thickness)) / transition;
      }
      else
      {
        // above cloud layer
        ratio = 1.0;
      }

      // accumulate effects from multiple cloud layers
      effvis *= ratio;

      if ( ratio < 1.0 )
      {
        if ( ! in_puff )
        {
          // calc chance of entering cloud puff
          double rnd = grRandom();
          double chance = rnd * rnd * rnd;
          if ( chance > 0.95 )
          {
            in_puff = true;
            puff_length = grRandom() * 2.0; // up to 2 seconds
            puff_progression = 0.0;
          }
        }

	if ( in_puff )
	{
	  // modify actual_visibility based on puff envelope
	  if ( puff_progression <= ramp_up )
	  {
	    double x = 0.5 * SGD_PI * puff_progression / ramp_up;
	    double factor = 1.0 - sin( x );
	    effvis = (float)(effvis * factor);
	  }
	  else if ( puff_progression >= ramp_up + puff_length )
	  {
	    double x = 0.5 * SGD_PI *
	      (puff_progression - (ramp_up + puff_length)) /
	      ramp_down;
	    double factor = sin( x );
	    effvis = (float)(effvis * factor);
	  }
	  else
  	  {
            effvis = 0.0;
          }

          puff_progression += time_factor;

	  if ( puff_progression > puff_length + ramp_up + ramp_down)
	  {
	    in_puff = false;
	  }
	}

        // never let visibility drop below 25 meters
        if ( effvis <= 25.0 )
        {
          effvis = 25.0;
        }
       }
     }
  } // for

  effective_visibility = effvis;
}

