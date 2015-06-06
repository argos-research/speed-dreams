/****************************************************************************

    file                 : ephemeris.cpp
    created              : Sun Aug 02 22:54:56 CET 2012
    copyright            : (C) 2000  Curtis L. Olson - (C)2012 Xavier Bertaux
    email                : bertauxx@yahoo.fr
    version              : $Id: ephemeris.cpp 4811 2012-07-29 17:16:37Z torcs-ng $

 ****************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <iostream>

#include "ephemeris.h"

ePhEphemeris::ePhEphemeris( const char *path ) 
{
    our_sun = new ePhStar;
    moon = new ePhMoonPos;
    mercury = new ePhMercury;
    venus = new ePhVenus;
    mars = new ePhMars;
    jupiter = new ePhJupiter;
    saturn = new ePhSaturn;
    uranus = new ePhUranus;
    neptune = new ePhNeptune;
    
    nplanets = 7;
    for ( int i = 0; i < nplanets; ++i )
      planets[i] = SGVec3d::zeros();
    stars = new ePhStarData( path );
}

ePhEphemeris::~ePhEphemeris( void ) 
{
    delete our_sun;
    delete moon;
    delete mercury;
    delete venus;
    delete mars;
    delete jupiter;
    delete saturn;
    delete uranus;
    delete neptune;
    delete stars;
}

// Update (recalculate) the positions of all objects for the specified
// time
void ePhEphemeris::update( double mjd, double lst, double lat ) 
{
    our_sun->updatePosition( mjd );
    moon->updatePosition( mjd, lst, lat, our_sun );
    mercury->updatePosition( mjd, our_sun );
    venus->updatePosition( mjd, our_sun );
    mars->updatePosition( mjd, our_sun );
    jupiter->updatePosition( mjd, our_sun );
    saturn->updatePosition( mjd, our_sun );
    uranus->updatePosition( mjd, our_sun );
    neptune->updatePosition( mjd, our_sun );

    // update planets list
    nplanets = 7;
    mercury->getPos( &planets[0][0], &planets[0][1], &planets[0][2] );
    venus  ->getPos( &planets[1][0], &planets[1][1], &planets[1][2] );
    mars   ->getPos( &planets[2][0], &planets[2][1], &planets[2][2] );
    jupiter->getPos( &planets[3][0], &planets[3][1], &planets[3][2] );
    saturn ->getPos( &planets[4][0], &planets[4][1], &planets[4][2] );
    uranus ->getPos( &planets[5][0], &planets[5][1], &planets[5][2] );
    neptune->getPos( &planets[6][0], &planets[6][1], &planets[6][2] );
}
