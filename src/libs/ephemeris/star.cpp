/****************************************************************************

    file                 : star.cpp
    created              : Sun Aug 02 22:54:56 CET 2012
    copyright            : (C) 2000  Curtis L. Olson - (C)2012 Xavier Bertaux
    email                : bertauxx@yahoo.fr
    version              : $Id: star.cpp 4811 2012-07-29 17:16:37Z torcs-ng $

 ****************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <math.h>

#include "star.h"

ePhStar::ePhStar(double mjd) :
    ePhCelestialBody (0.000000,  0.0000000000,
		   0.0000,    0.00000,
		   282.9404,  4.7093500E-5,	
		   1.0000000, 0.000000,	
		   0.016709,  -1.151E-9,
		   356.0470,  0.98560025850, mjd)
{
    distance = 0.0;
}

ePhStar::ePhStar() :
    ePhCelestialBody (0.000000,  0.0000000000,
		   0.0000,    0.00000,
		   282.9404,  4.7093500E-5,	
		   1.0000000, 0.000000,	
		   0.016709,  -1.151E-9,
		   356.0470,  0.98560025850)
{
    distance = 0.0;
}

ePhStar::~ePhStar()
{
}

void ePhStar::updatePosition(double mjd)
{
  	double actTime, eccAnom, 
    	xv, yv, v, r,
    	xe, ecl;

  	updateOrbElements(mjd);
  
  	actTime = sdCalcActTime(mjd);
  	ecl = SGD_DEGREES_TO_RADIANS * (23.4393 - 3.563E-7 * actTime); // Angle in Radians
  	eccAnom = sdCalcEccAnom(M, e);  // Calculate the eccentric Anomaly (also known as solving Kepler's equation)
  
  	xv = cos(eccAnom) - e;
  	yv = sqrt (1.0 - e*e) * sin(eccAnom);
  	v = atan2 (yv, xv);                   // the sun's true anomaly
  	distance = r = sqrt (xv*xv + yv*yv);  // and its distance

  	lonEcl = v + w; // the sun's true longitude
  	latEcl = 0;
  	xs = r * cos (lonEcl);
  	ys = r * sin (lonEcl);
  	xe = xs;
  	ye = ys * cos (ecl);
  	ze = ys * sin (ecl);

  	rightAscension = atan2 (ye, xe);
  	declination = atan2 (ze, sqrt (xe*xe + ye*ye));
}
