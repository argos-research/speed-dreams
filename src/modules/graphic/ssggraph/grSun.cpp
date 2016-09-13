/***************************************************************************

    file        : grSun.cpp
    copyright   : (C) 2009 by Xavier Bertaux (based on ssgasky plib code)
    web         : http://www.speed-dreams.org
    version     : $Id: grSky.cpp 3162 2010-12-05 13:11:22Z pouillot $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <plib/sg.h>
#include <plib/ssg.h>
#include "grSky.h"
#include "grSphere.h"

static float sun_exp2_punch_through;
//static double visibility; // Never used.

// Set up sun rendering call backs
static int grSunPreDraw( ssgEntity *e )
{
    ssgLeaf *f = (ssgLeaf *)e;
    if ( f -> hasState () ) f->getState()->apply() ;

    glPushAttrib( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_FOG_BIT );
    glDisable( GL_DEPTH_TEST );
    glDisable( GL_FOG );
    glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA ) ;
    return true;
}

static int grSunPostDraw( ssgEntity *e )
{
    glPopAttrib();
    return true;
}

static int grSunHaloPreDraw( ssgEntity *e )
{
    ssgLeaf *f = (ssgLeaf *)e;
    if ( f -> hasState () ) f->getState()->apply();

    glPushAttrib( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_FOG_BIT );
    glDisable( GL_DEPTH_TEST );
    glFogf (GL_FOG_DENSITY, sun_exp2_punch_through);
    glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA ) ;

    return true;
}

static int grSunHaloPostDraw( ssgEntity *e )
{
    glPopAttrib();
    return true;
}

// Constructor
cGrSun::cGrSun( void )
{
	sun_transform = 0;
    prev_sun_angle = -9999.0;
    visibility = -9999.0;
}

// Destructor
cGrSun::~cGrSun( void )
{
	ssgDeRefDelete( sun_transform );
}

ssgBranch * cGrSun::build( double sun_size )
{
    ssgDeRefDelete( sun_transform );

	sun_transform = new ssgTransform;
	sun_transform->ref();

	sgVec4 color;
    sgSetVec4( color, 1.0, 1.0, 1.0, 1.0 );

    sun_cl = new ssgColourArray( 1 );
    sun_cl->add( color );

    ihalo_cl = new ssgColourArray( 1 );
    ihalo_cl->add( color );

    ohalo_cl = new ssgColourArray( 1 );
    ohalo_cl->add( color );

    //repaint( 0.0, 1.0 );

    // set up the sun-state
    sun_state = new ssgSimpleState();
	sun_state->setShadeModel( GL_SMOOTH );
	sun_state->disable( GL_LIGHTING );
	sun_state->enable( GL_CULL_FACE );
	sun_state->disable( GL_TEXTURE_2D );
	sun_state->enable( GL_COLOR_MATERIAL );
	sun_state->setColourMaterial( GL_AMBIENT_AND_DIFFUSE );
	sun_state->setMaterial( GL_EMISSION, 0, 0, 0, 1 );
	sun_state->setMaterial( GL_SPECULAR, 0, 0, 0, 1 );
	sun_state->disable( GL_BLEND );
    sun_state->setAlphaClamp( 0.01f );
	sun_state->disable( GL_ALPHA_TEST );

    ssgBranch *sun = grMakeSphere( sun_state, sun_cl, (float) sun_size, 15, 15,
				    grSunPreDraw, grSunPostDraw );

    repaint( 0.0, 10000.0 );

	ihalo_state = new ssgSimpleState();
	ihalo_state->setTexture( "data/textures/inner_halo.png" );
	ihalo_state->enable( GL_TEXTURE_2D );
	ihalo_state->disable( GL_LIGHTING );
	ihalo_state->setShadeModel( GL_SMOOTH );
	ihalo_state->disable( GL_CULL_FACE );
	ihalo_state->enable( GL_COLOR_MATERIAL );
	ihalo_state->setColourMaterial( GL_AMBIENT_AND_DIFFUSE );
	ihalo_state->setMaterial( GL_EMISSION, 0, 0, 0, 1 );
	ihalo_state->setMaterial( GL_SPECULAR, 0, 0, 0, 1 );
	ihalo_state->enable( GL_ALPHA_TEST );
	ihalo_state->setAlphaClamp(0.01f);
	ihalo_state->enable ( GL_BLEND ) ;

    float ihalo_size = (float) (sun_size * 2.0);
    sgVec3 vc;
    ihalo_vl = new ssgVertexArray;
    sgSetVec3( vc, -ihalo_size, 0.0, -ihalo_size );
    ihalo_vl->add( vc );
    sgSetVec3( vc, ihalo_size, 0.0, -ihalo_size );
    ihalo_vl->add( vc );
    sgSetVec3( vc, -ihalo_size, 0.0,  ihalo_size );
    ihalo_vl->add( vc );
    sgSetVec3( vc, ihalo_size, 0.0,  ihalo_size );
    ihalo_vl->add( vc );

    sgVec2 vd;
    ihalo_tl = new ssgTexCoordArray;
    sgSetVec2( vd, 0.0f, 0.0f );
    ihalo_tl->add( vd );
    sgSetVec2( vd, 1.0, 0.0 );
    ihalo_tl->add( vd );
    sgSetVec2( vd, 0.0, 1.0 );
    ihalo_tl->add( vd );
    sgSetVec2( vd, 1.0, 1.0 );
    ihalo_tl->add( vd );

    ssgLeaf *ihalo = new ssgVtxTable ( GL_TRIANGLE_STRIP, ihalo_vl, NULL, ihalo_tl, ihalo_cl );
    ihalo->setState( ihalo_state );

    ohalo_state = new ssgSimpleState();
    ohalo_state->setTexture( "data/textures/halo.png" );
    ohalo_state->enable( GL_TEXTURE_2D );
    ohalo_state->disable( GL_LIGHTING );
    ohalo_state->setShadeModel( GL_SMOOTH );
    ohalo_state->disable( GL_CULL_FACE );
    ohalo_state->enable( GL_COLOR_MATERIAL );
    ohalo_state->setColourMaterial( GL_AMBIENT_AND_DIFFUSE );
    ohalo_state->setMaterial( GL_EMISSION, 0, 0, 0, 1 );
    ohalo_state->setMaterial( GL_SPECULAR, 0, 0, 0, 1 );
    ohalo_state->enable( GL_ALPHA_TEST );
    ohalo_state->setAlphaClamp(0.01f);
    ohalo_state->enable ( GL_BLEND ) ;

    float ohalo_size = (float)(sun_size * 10.0);
    sgVec3 ve;
    ohalo_vl = new ssgVertexArray;
    sgSetVec3( ve, -ohalo_size, 0.0, -ohalo_size );
    ohalo_vl->add( ve );
    sgSetVec3( ve, ohalo_size, 0.0, -ohalo_size );
    ohalo_vl->add( ve );
    sgSetVec3( ve, -ohalo_size, 0.0,  ohalo_size );
    ohalo_vl->add( ve );
    sgSetVec3( ve, ohalo_size, 0.0,  ohalo_size );
    ohalo_vl->add( ve );

    sgVec2 vf;
    ohalo_tl = new ssgTexCoordArray;
    sgSetVec2( vf, 0.0f, 0.0f );
    ohalo_tl->add( vf );
    sgSetVec2( vf, 1.0, 0.0 );
    ohalo_tl->add( vf );
    sgSetVec2( vf, 0.0, 1.0 );
    ohalo_tl->add( vf );
    sgSetVec2( vf, 1.0, 1.0 );
    ohalo_tl->add( vf );

    ssgLeaf *ohalo = new ssgVtxTable ( GL_TRIANGLE_STRIP, ohalo_vl, NULL, ohalo_tl, ohalo_cl );
    ohalo->setState( ohalo_state );

    ihalo->setCallback( SSG_CALLBACK_PREDRAW, grSunHaloPreDraw );
    ihalo->setCallback( SSG_CALLBACK_POSTDRAW, grSunHaloPostDraw );
    ohalo->setCallback( SSG_CALLBACK_PREDRAW, grSunHaloPreDraw );
    ohalo->setCallback( SSG_CALLBACK_POSTDRAW, grSunHaloPostDraw );

    sun_transform->addKid( ohalo );
    sun_transform->addKid( ihalo );
    sun_transform->addKid( sun );

    return sun_transform;
}

bool cGrSun::repaint( double sun_angle, double new_visibility )
{
	if ( visibility != new_visibility )
	{
		if (new_visibility < 100.0) new_visibility = 100.0;
        else if (new_visibility > 45000.0) new_visibility = 45000.0;
        visibility = (float) new_visibility;


        static const float sqrt_m_log01 = (float) (sqrt( -log( 0.01 ) ));
        sun_exp2_punch_through = sqrt_m_log01 / ( visibility * 15 );
		//sun_exp2_punch_through = 2.0 /log( visibility );
    }

    if ( prev_sun_angle != sun_angle )
	{
        prev_sun_angle = sun_angle;

		float aerosol_factor;
		if ( visibility < 100 )
		{
			aerosol_factor = 8000;
		}
		else
		{
        	aerosol_factor = (float) (80.5 / log( visibility / 100 ));
		}

		float rel_humidity, density_avg;

		/*if ( !env_node )
		{*/
			rel_humidity = 0.5f;
			density_avg = 0.7f;
		/*}
		else
		{
			//rel_humidity = env_node->getFloatValue( "relative-humidity" );
			//density_avg =  env_node->getFloatValue( "atmosphere/density-tropo-avg" );
		}*/

		sgVec4 i_halo_color, o_halo_color, sun_color;
		float green_scat_f;

		float red_scat_f = (float) (( aerosol_factor * path_distance * density_avg ) / 5E+07);
		sun_color[0] = 1 - red_scat_f;
		i_halo_color[0] = 1 - ( 1.1f * red_scat_f );
		o_halo_color[0] = 1 - ( 1.4f * red_scat_f );

		if (sun_declination > 5.0 || sun_declination < 2.0)
		{
			green_scat_f = (float) (( aerosol_factor * path_distance * density_avg ) / 5E+07);
		}
		else
			green_scat_f = (float) (( aerosol_factor * path_distance * density_avg ) / 8.8938E+06);

		sun_color[1] = 1 - green_scat_f;
		i_halo_color[1] = 1 - ( 1.1f * green_scat_f );
		o_halo_color[1] = 1 - ( 1.4f * green_scat_f );

		// Blue - 435.8 nm
		float blue_scat_f = (float) (( aerosol_factor * path_distance * density_avg ) / 3.607E+06);
		sun_color[2] = 1 - blue_scat_f;
		i_halo_color[2] = 1 - ( 1.1f * blue_scat_f );
		o_halo_color[2] = 1 - ( 1.4f * blue_scat_f );

		// Alpha
		sun_color[3] = 1;
		i_halo_color[3] = 1;

		o_halo_color[3] = blue_scat_f;
		if ( ( new_visibility < 10000 ) &&  ( blue_scat_f > 1 ))
		{
			o_halo_color[3] = 2 - blue_scat_f;
		}

		float saturation = 1 - ( rel_humidity / 200 );
		sun_color[1] += (( 1 - saturation ) * ( 1 - sun_color[1] ));
		sun_color[2] += (( 1 - saturation ) * ( 1 - sun_color[2] ));

		i_halo_color[1] += (( 1 - saturation ) * ( 1 - i_halo_color[1] ));
		i_halo_color[2] += (( 1 - saturation ) * ( 1 - i_halo_color[2] ));

		o_halo_color[1] += (( 1 - saturation ) * ( 1 - o_halo_color[1] ));
		o_halo_color[2] += (( 1 - saturation ) * ( 1 - o_halo_color[2] ));

		// just to make sure we're in the limits
		if ( sun_color[0] < 0 ) sun_color[0] = 0;
		else if ( sun_color[0] > 1) sun_color[0] = 1;
		if ( i_halo_color[0] < 0 ) i_halo_color[0] = 0;
		else if ( i_halo_color[0] > 1) i_halo_color[0] = 1;
		if ( o_halo_color[0] < 0 ) o_halo_color[0] = 0;
		else if ( o_halo_color[0] > 1) o_halo_color[0] = 1;

		if ( sun_color[1] < 0 ) sun_color[1] = 0;
		else if ( sun_color[1] > 1) sun_color[1] = 1;
		if ( i_halo_color[1] < 0 ) i_halo_color[1] = 0;
		else if ( i_halo_color[1] > 1) i_halo_color[1] = 1;
		if ( o_halo_color[1] < 0 ) o_halo_color[1] = 0;
		else if ( o_halo_color[0] > 1) o_halo_color[1] = 1;

		if ( sun_color[2] < 0 ) sun_color[2] = 0;
		else if ( sun_color[2] > 1) sun_color[2] = 1;
		if ( i_halo_color[2] < 0 ) i_halo_color[2] = 0;
		else if ( i_halo_color[2] > 1) i_halo_color[2] = 1;
		if ( o_halo_color[2] < 0 ) o_halo_color[2] = 0;
		else if ( o_halo_color[2] > 1) o_halo_color[2] = 1;
		if ( o_halo_color[3] < 0 ) o_halo_color[3] = 0;
		else if ( o_halo_color[3] > 1) o_halo_color[3] = 1;

        grGammaCorrectRGB( sun_color );
		grGammaCorrectRGB( i_halo_color );
		grGammaCorrectRGB( o_halo_color );

        float *ptr;
        ptr = sun_cl->get( 0 );
        sgCopyVec4( ptr, sun_color );
		ptr = ihalo_cl->get( 0 );
		sgCopyVec4( ptr, i_halo_color );
		ptr = ohalo_cl->get( 0 );
		sgCopyVec4( ptr, o_halo_color );
    }

    return true;
}

bool cGrSun::reposition( sgVec3 p, double angle, double rightAscension, double declination,	double sun_dist )
{
    sgMat4 T1, T2, GST, RA, DEC;
    sgVec3 axis;
    sgVec3 v;

    sgMakeTransMat4( T1, p );
    sgSetVec3( axis, 0.0, 0.0, -1.0 );
    sgMakeRotMat4( GST, (float) angle, axis );
    sgSetVec3( axis, 0.0, 0.0, 1.0 );
    sgMakeRotMat4( RA, (float) (rightAscension * SGD_RADIANS_TO_DEGREES - 90.0), axis );
    sgSetVec3( axis, 1.0, 0.0, 0.0 );
    sgMakeRotMat4( DEC, (float)(declination * SGD_RADIANS_TO_DEGREES), axis );

    sgSetVec3( v, 0.0, (float) sun_dist, 0.0 );
    sgMakeTransMat4( T2, v );

    sgMat4 TRANSFORM;
    sgCopyMat4( TRANSFORM, T1 );
    sgPreMultMat4( TRANSFORM, GST );
    sgPreMultMat4( TRANSFORM, RA );
    sgPreMultMat4( TRANSFORM, DEC );
    sgPreMultMat4( TRANSFORM, T2 );

    sgCoord skypos;
    sgSetCoord( &skypos, TRANSFORM );

    sun_transform->setTransform( &skypos );

    if ( prev_sun_angle != sun_angle )
	{
		if ( sun_angle == 0 ) sun_angle = 0.1;
        const double r_earth_pole = 6356752.314;
        const double r_tropo_pole = 6356752.314 + 8000;
        const double epsilon_earth2 = 6.694380066E-3;
        const double epsilon_tropo2 = 9.170014946E-3;

        double r_tropo = r_tropo_pole / sqrt ( 1 - ( epsilon_tropo2 * pow ( cos( 0.0 ), 2 )));
        double r_earth = r_earth_pole / sqrt ( 1 - ( epsilon_earth2 * pow ( cos( 0.0 ), 2 )));

        double position_radius = r_earth;

        double gamma =  SG_PI - sun_angle;
        double sin_beta =  ( position_radius * sin ( gamma )  ) / r_tropo;
        double alpha =  SG_PI - gamma - asin( sin_beta );

		path_distance = sqrt( pow( position_radius, 2 ) + pow( r_tropo, 2 )
                        - ( 2 * position_radius * r_tropo * cos( alpha ) ));

        double alt_half = sqrt( pow ( r_tropo, 2 ) + pow( path_distance / 2, 2 ) - r_tropo * path_distance * cos( asin( sin_beta )) ) - r_earth;

        if ( alt_half < 0.0 ) alt_half = 0.0;

        /*if ( env_node )
		{
			//env_node->setDoubleValue( "atmosphere/altitude-troposphere-top", r_tropo - r_earth );
			//env_node->setDoubleValue( "atmosphere/altitude-half-to-sun", alt_half );
		}*/
    }

    return true;
}
