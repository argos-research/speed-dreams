/****************************************************************************

    file                 : saturn.cpp
    created              : Sun Aug 02 22:54:56 CET 2012
    copyright            : (C) 1997  Durk Talsma - (C)2012 Xavier Bertaux
    email                : bertauxx@yahoo.fr
    version              : $Id: saturn.cpp 4811 2012-07-29 17:16:37Z torcs-ng $

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

#include "saturn.h"

ePhSaturn::ePhSaturn(double mjd) :
  ePhCelestialBody(113.6634,   2.3898000E-5,
		2.4886,	   -1.081E-7,
		339.3939,   2.9766100E-5,
		9.5547500,  0.000000,
		0.055546,  -9.499E-9,
		316.9670,   0.03344422820, mjd)
{
}

ePhSaturn::ePhSaturn() :
  ePhCelestialBody(113.6634,   2.3898000E-5,
		2.4886,	   -1.081E-7,
		339.3939,   2.9766100E-5,
		9.5547500,  0.000000,
		0.055546,  -9.499E-9,
		316.9670,   0.03344422820)
{
}

void ePhSaturn::updatePosition(double mjd, ePhStar *ourSun)
{
  	ePhCelestialBody::updatePosition(mjd, ourSun);
  
  	double actTime = sdCalcActTime(mjd);
  	double ir = 0.4897394;
  	double Nr = 2.9585076 + 6.6672E-7*actTime;
  	double B = asin (sin(declination) * cos(ir) - 
		   cos(declination) * sin(ir) *
		   sin(rightAscension - Nr));
  	double ring_magn = -2.6 * sin(fabs(B)) + 1.2 * pow(sin(B), 2);
  	magnitude = -9.0 + 5*log10(r*R) + 0.044 * FV + ring_magn;
}
