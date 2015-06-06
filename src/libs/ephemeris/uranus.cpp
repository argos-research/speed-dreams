/****************************************************************************

    file                 : uranus.cpp
    created              : Sun Aug 02 22:54:56 CET 2012
    copyright            : (C) 1997  Durk Talsma - (C)2012 Xavier Bertaux
    email                : bertauxx@yahoo.fr
    version              : $Id: uranus.cpp 4811 2012-07-29 17:16:37Z torcs-ng $

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

#include "uranus.h"

ePhUranus::ePhUranus(double mjd) :
  ePhCelestialBody(74.00050,   1.3978000E-5,
		0.7733,     1.900E-8,
		96.66120,   3.0565000E-5,
		19.181710, -1.55E-8,
		0.047318,   7.450E-9,
		142.5905,   0.01172580600, mjd)
{
}

ePhUranus::ePhUranus() :
  ePhCelestialBody(74.00050,   1.3978000E-5,
		0.7733,     1.900E-8,
		96.66120,   3.0565000E-5,
		19.181710, -1.55E-8,
		0.047318,   7.450E-9,
		142.5905,   0.01172580600)
{
}

void ePhUranus::updatePosition(double mjd, ePhStar *ourSun)
{
  	ePhCelestialBody::updatePosition(mjd, ourSun);
  	magnitude = -7.15 + 5*log10( r*R) + 0.001 * FV;
}
