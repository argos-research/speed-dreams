/****************************************************************************

    file                 : neptune.cpp
    created              : Sun Aug 02 22:54:56 CET 2012
    copyright            : (C) 1997  Durk Talsma - (C)2012 Xavier Bertaux
    email                : bertauxx@yahoo.fr
    version              : $Id: neptune.cpp 4811 2012-07-29 17:16:37Z torcs-ng $

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

#include "neptune.h"

ePhNeptune::ePhNeptune(double mjd) :
  ePhCelestialBody(131.7806,   3.0173000E-5,
		1.7700,	   -2.550E-7,
		272.8461,  -6.027000E-6,	
		30.058260,  3.313E-8,
		0.008606,   2.150E-9,
		260.2471,   0.00599514700, mjd)
{
}
ePhNeptune::ePhNeptune() :
  ePhCelestialBody(131.7806,   3.0173000E-5,
		1.7700,	   -2.550E-7,
		272.8461,  -6.027000E-6,	
		30.058260,  3.313E-8,
		0.008606,   2.150E-9,
		260.2471,   0.00599514700)
{
}

void ePhNeptune::updatePosition(double mjd, ePhStar *ourSun)
{
  	ePhCelestialBody::updatePosition(mjd, ourSun);
  	magnitude = -6.90 + 5*log10 (r*R) + 0.001 *FV;
}
