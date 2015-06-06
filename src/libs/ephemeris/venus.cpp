/****************************************************************************

    file                 : venus.cpp
    created              : Sun Aug 02 22:54:56 CET 2012
    copyright            : (C) 1997  Durk Talsma - (C)2012 Xavier Bertaux
    email                : bertauxx@yahoo.fr
    version              : $Id: venus.cpp 4811 2012-07-29 17:16:37Z torcs-ng $

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

#include "venus.h"

ePhVenus::ePhVenus(double mjd) :
  ePhCelestialBody(76.67990,  2.4659000E-5, 
		3.3946,    2.75E-8,
		54.89100,  1.3837400E-5,
		0.7233300, 0.000000,
		0.006773, -1.302E-9,
		48.00520,  1.60213022440, mjd)
{
}

ePhVenus::ePhVenus() :
  ePhCelestialBody(76.67990,  2.4659000E-5, 
		3.3946,    2.75E-8,
		54.89100,  1.3837400E-5,
		0.7233300, 0.000000,
		0.006773, -1.302E-9,
		48.00520,  1.60213022440)
{
}

void ePhVenus::updatePosition(double mjd, ePhStar *ourSun)
{
  	ePhCelestialBody::updatePosition(mjd, ourSun);
  	magnitude = -4.34 + 5*log10( r*R ) + 0.013 * FV + 4.2E-07 * pow(FV,3);
}
