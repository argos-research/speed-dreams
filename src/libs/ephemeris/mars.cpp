/****************************************************************************

    file                 : mars.cpp
    created              : Sun Aug 02 22:54:56 CET 2012
    copyright            : (C) 1997  Durk Talsma - (C)2012 Xavier Bertaux
    email                : bertauxx@yahoo.fr
    version              : $Id: mars.cpp 4811 2012-07-29 17:16:37Z torcs-ng $

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

#include "mars.h"

ePhMars::ePhMars(double mjd) :
  ePhCelestialBody(49.55740,  2.1108100E-5,
		1.8497,   -1.78E-8,
		286.5016,  2.9296100E-5,
		1.5236880, 0.000000,
		0.093405,  2.516E-9,
		18.60210,  0.52402077660, mjd)
{
}
ePhMars::ePhMars() :
  ePhCelestialBody(49.55740,  2.1108100E-5,
		1.8497,   -1.78E-8,
		286.5016,  2.9296100E-5,
		1.5236880, 0.000000,
		0.093405,  2.516E-9,
		18.60210,  0.52402077660)
{
}
/*************************************************************************
 * void Mars::updatePosition(double mjd, Star *ourSun)
 * 
 * calculates the current position of Mars, by calling the base class,
 * CelestialBody::updatePosition(); The current magnitude is calculated using 
 * a Mars specific equation
 *************************************************************************/
void ePhMars::updatePosition(double mjd, ePhStar *ourSun)
{
  	ePhCelestialBody::updatePosition(mjd, ourSun);
  	magnitude = -1.51 + 5*log10( r*R ) + 0.016 * FV;
}
