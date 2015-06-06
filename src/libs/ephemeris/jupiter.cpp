/****************************************************************************

    file                 : jupiter.cpp
    created              : Sun Aug 02 22:54:56 CET 2012
    copyright            : (C) 1997  Durk Talsma - (C)2012 Xavier Bertaux
    email                : bertauxx@yahoo.fr
    version              : $Id: jupiter.cpp 4811 2012-07-29 17:16:37Z torcs-ng $

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

#include "jupiter.h"

ePhJupiter::ePhJupiter(double mjd) :
  ePhCelestialBody(100.4542,  2.7685400E-5,	
		1.3030,   -1.557E-7,
		273.8777,  1.6450500E-5,
		5.2025600, 0.000000,
		0.048498,  4.469E-9,
		19.89500,  0.08308530010, mjd)
{
}

ePhJupiter::ePhJupiter() :
  ePhCelestialBody(100.4542,  2.7685400E-5,	
		1.3030,   -1.557E-7,
		273.8777,  1.6450500E-5,
		5.2025600, 0.000000,
		0.048498,  4.469E-9,
		19.89500,  0.08308530010)
{
}

void ePhJupiter::updatePosition(double mjd, ePhStar *ourSun)
{
  	ePhCelestialBody::updatePosition(mjd, ourSun);
  	magnitude = -9.25 + 5*log10( r*R ) + 0.014 * FV;
}




