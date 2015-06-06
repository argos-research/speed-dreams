/****************************************************************************

    file                 : ephemeris.h
    created              : Sun Aug 02 22:54:56 CET 2012
    copyright            : (C) 2000  Curtis L. Olson - (C)2012 Xavier Bertaux
    email                : bertauxx@yahoo.fr
    version              : $Id: ephemeris.h 4811 2012-07-29 17:16:37Z torcs-ng $

 ****************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#ifndef _EPHEMERIS_H
#define _EPHEMERIS_H

#include "star.h"
#include "moonpos.h"
#include "mercury.h"
#include "venus.h"
#include "mars.h"
#include "jupiter.h"
#include "saturn.h"
#include "uranus.h"
#include "neptune.h"
#include "stardata.h"

#include <SGMath.h>

class ePhEphemeris 
{
    ePhStar *our_sun;
    ePhMoonPos *moon;
    ePhMercury *mercury;
    ePhVenus *venus;
    ePhMars *mars;
    ePhJupiter *jupiter;
    ePhSaturn *saturn;
    ePhUranus *uranus;
    ePhNeptune *neptune;

    int nplanets;
    SGVec3d planets[7];

    ePhStarData *stars;

public:
    ePhEphemeris( const char *path );
    ~ePhEphemeris( void );

    void update(double mjd, double lst, double lat);

    inline ePhStar *get_sun() const { return our_sun; }

    inline double getSunRightAscension() const 
    {
	return our_sun->getRightAscension();
    }

    inline double getSunDeclination() const 
    {
	return our_sun->getDeclination();
    }

    inline ePhMoonPos *get_moon() const { return moon; }
    inline double getMoonRightAscension() const 
    {
	return moon->getRightAscension();
    }

    inline double getMoonDeclination() const 
    {
	return moon->getDeclination();
    }

    inline int getNumPlanets() const { return nplanets; }
    inline SGVec3d *getPlanets() { return planets; }
    inline const SGVec3d *getPlanets() const { return planets; }

    inline int getNumStars() const { return stars->getNumStars(); }
    inline SGVec3d *getStars() { return stars->getStars(); }
    inline const SGVec3d *getStars() const { return stars->getStars(); }
};

#endif // _EPHEMERIS_H
