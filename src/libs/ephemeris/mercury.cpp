/****************************************************************************

    file                 : mercury.cpp
    created              : Sun Aug 02 22:54:56 CET 2012
    copyright            : (C) 1997  Durk Talsma - (C)2012 Xavier Bertaux
    email                : bertauxx@yahoo.fr
    version              : $Id: mercury.cpp 4811 2012-07-29 17:16:37Z torcs-ng $

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

#include "mercury.h"

ePhMercury::ePhMercury(double mjd) :
  ePhCelestialBody (48.33130,   3.2458700E-5,
                  7.0047,    5.00E-8,
                  29.12410,  1.0144400E-5,
                  0.3870980, 0.000000,
                  0.205635,  5.59E-10,
                  168.6562,  4.09233443680, mjd)
{
}

ePhMercury::ePhMercury() :
  ePhCelestialBody (48.33130,   3.2458700E-5,
                  7.0047,    5.00E-8,
                  29.12410,  1.0144400E-5,
                  0.3870980, 0.000000,
                  0.205635,  5.59E-10,
                  168.6562,  4.09233443680)
{
}

void ePhMercury::updatePosition(double mjd, ePhStar *ourSun)
{
  	ePhCelestialBody::updatePosition(mjd, ourSun);
  	magnitude = -0.36 + 5*log10( r*R ) + 0.027 * FV + 2.2E-13 * pow(FV, 6); 
}
