/***************************************************************************

    file        : grMoon.cpp
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



#include "grSky.h"
#include "grSphere.h"

static int grMoonOrbPreDraw( ssgEntity *e )
{
    ssgLeaf *f = (ssgLeaf *)e;
    if ( f -> hasState () ) f->getState()->apply() ;

    glPushAttrib( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT );
    glDisable( GL_DEPTH_TEST );
    glDisable( GL_FOG );
    glBlendFunc ( GL_SRC_ALPHA, GL_ONE );
    return true;
}

static int grMoonOrbPostDraw( ssgEntity *e )
{
    glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glPopAttrib();
    return true;
}

// Constructor
cGrMoon::cGrMoon( void )
{
	moon_transform = 0;
	prev_moon_angle= 9999;
}

// Destructor
cGrMoon::~cGrMoon( void )
{
	ssgDeRefDelete( moon_transform );
}

// build the moon object
ssgBranch * cGrMoon::build( double moon_size )
{
	ssgDeRefDelete( moon_transform );
    moon_transform = new ssgTransform;
	moon_transform->ref();

    moon_cl = new ssgColourArray( 1 );
    sgVec4 color;
    sgSetVec4( color, 1.0, 1.0, 1.0, 1.0 );
    moon_cl->add( color );

    moon_state = new ssgSimpleState();
    moon_state->setTexture( "data/textures/moon.rgba" );
    moon_state->setShadeModel( GL_SMOOTH );
    moon_state->enable( GL_LIGHTING );
    moon_state->enable( GL_CULL_FACE );
    moon_state->enable( GL_TEXTURE_2D );
    moon_state->enable( GL_COLOR_MATERIAL );
    moon_state->setColourMaterial( GL_DIFFUSE );
    moon_state->setMaterial( GL_AMBIENT, 0, 0, 0, 1 );
    moon_state->setMaterial( GL_EMISSION, 0, 0, 0, 1 );
    moon_state->setMaterial( GL_SPECULAR, 0, 0, 0, 1 );
    moon_state->enable( GL_BLEND );
    moon_state->enable( GL_ALPHA_TEST );
    moon_state->setAlphaClamp( 0.01f );

    ssgBranch *moon = grMakeSphere( moon_state, moon_cl, (float) moon_size, 15, 15,
				    grMoonOrbPreDraw, grMoonOrbPostDraw );

    moon_transform->addKid( moon );
	repaint( 0.0 );

    return moon_transform;
}

bool cGrMoon::repaint( double angle )
{
    if (prev_moon_angle != angle)
	{
        prev_moon_angle = angle;

        double moon_factor = 4 * cos(angle);

        if (moon_factor > 1) moon_factor = 1.0;
        if (moon_factor < -1) moon_factor = -1.0;
        moon_factor = (moon_factor / 2) + 0.5f;

        sgVec4 color;
        color[1] = (float) (sqrt(moon_factor));
        color[0] = (float) (sqrt(color[1]));
        color[2] = (float) (moon_factor * moon_factor);
        color[2] *= color[2];
        color[3] = 1.0;
		//color[0] = (float)pow(moon_factor, 0.25);
		//color[1] = (float)pow(moon_factor, 0.50);
		//color[2] = (float)pow(moon_factor, 4.0);
		//color[3] = 1.0;

        grGammaCorrectRGB( color );

        float *ptr;
        ptr = moon_cl->get( 0 );
        sgCopyVec4( ptr, color );
    }

    return true;
}

bool cGrMoon::reposition(sgVec3 p, double angle, double moonrightAscension, double moondeclination, double moon_dist)
{
    sgMat4 T1, T2, GST, RA, DEC;
    sgVec3 axis;
    sgVec3 v;

    sgMakeTransMat4( T1, p );

    sgSetVec3( axis, 0.0, 0.0, -1.0 );
    sgMakeRotMat4( GST, (float) angle, axis );
    sgSetVec3( axis, 0.0, 0.0, 1.0 );
    sgMakeRotMat4( RA, (float) ((moonrightAscension * SGD_RADIANS_TO_DEGREES) - 90.0), axis );
    sgSetVec3( axis, 1.0, 0.0, 0.0 );
    sgMakeRotMat4( DEC, (float) (moondeclination * SGD_RADIANS_TO_DEGREES), axis );
    sgSetVec3( v, 0.0, (float) moon_dist, 0.0 );
    sgMakeTransMat4( T2, v );

    sgMat4 TRANSFORM;
    sgCopyMat4( TRANSFORM, T1 );
    sgPreMultMat4( TRANSFORM, GST );
    sgPreMultMat4( TRANSFORM, RA );
    sgPreMultMat4( TRANSFORM, DEC );
    sgPreMultMat4( TRANSFORM, T2 );

    sgCoord skypos;
    sgSetCoord( &skypos, TRANSFORM );

    moon_transform->setTransform( &skypos );

    return true;
}
