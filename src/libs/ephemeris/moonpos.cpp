/****************************************************************************

    file                 : moonpos.cpp
    created              : Sun Aug 02 22:54:56 CET 2012
    copyright            : (C) 1997  Durk Talsma - (C)2012 Xavier Bertaux
    email                : bertauxx@yahoo.fr
    version              : $Id: moonpos.cpp 4811 2012-07-29 17:16:37Z torcs-ng $

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

#include "moonpos.h"

ePhMoonPos::ePhMoonPos(double mjd) :
  ePhCelestialBody(125.1228, -0.0529538083,
		5.1454,    0.00000,
		318.0634,  0.1643573223,
		60.266600, 0.000000,
		0.054900,  0.000000,
		115.3654,  13.0649929509, mjd)
{
}

ePhMoonPos::ePhMoonPos() :
  ePhCelestialBody(125.1228, -0.0529538083,
		5.1454,    0.00000,
		318.0634,  0.1643573223,
		60.266600, 0.000000,
		0.054900,  0.000000,
		115.3654,  13.0649929509)
{
}


ePhMoonPos::~ePhMoonPos()
{
}

void ePhMoonPos::updatePosition(double mjd, double lst, double lat, ePhStar *ourSun)
{
  double 
    eccAnom, ecl, actTime,
    xv, yv, v, r, xh, yh, zh, xg, yg, zg, xe, ye, ze,
    Ls, Lm, D, F, mpar, gclat, rho, HA, g,
    geoRa, geoDec;
  
  updateOrbElements(mjd);
  actTime = sdCalcActTime(mjd);

  ecl = ((SGD_DEGREES_TO_RADIANS * 23.4393) - (SGD_DEGREES_TO_RADIANS * 3.563E-7) * actTime);  
  eccAnom = sdCalcEccAnom(M, e);  // Calculate the eccentric anomaly
  xv = a * (cos(eccAnom) - e);
  yv = a * (sqrt(1.0 - e*e) * sin(eccAnom));
  v = atan2(yv, xv);               // the moon's true anomaly
  r = sqrt (xv*xv + yv*yv);       // and its distance

  xh = r * (cos(N) * cos (v+w) - sin (N) * sin(v+w) * cos(i));
  yh = r * (sin(N) * cos (v+w) + cos (N) * sin(v+w) * cos(i));
  zh = r * (sin(v+w) * sin(i));

  lonEcl = atan2 (yh, xh);
  latEcl = atan2(zh, sqrt(xh*xh + yh*yh));

  Ls = ourSun->getM() + ourSun->getw();
  Lm = M + w + N;
  D = Lm - Ls;
  F = Lm - N;
  
  lonEcl += SGD_DEGREES_TO_RADIANS * (-1.274 * sin (M - 2*D)
			  +0.658 * sin (2*D)
			  -0.186 * sin(ourSun->getM())
			  -0.059 * sin(2*M - 2*D)
			  -0.057 * sin(M - 2*D + ourSun->getM())
			  +0.053 * sin(M + 2*D)
			  +0.046 * sin(2*D - ourSun->getM())
			  +0.041 * sin(M - ourSun->getM())
			  -0.035 * sin(D)
			  -0.031 * sin(M + ourSun->getM())
			  -0.015 * sin(2*F - 2*D)
			  +0.011 * sin(M - 4*D)
			  );
  latEcl += SGD_DEGREES_TO_RADIANS * (-0.173 * sin(F-2*D)
			  -0.055 * sin(M - F - 2*D)
			  -0.046 * sin(M + F - 2*D)
			  +0.033 * sin(F + 2*D)
			  +0.017 * sin(2*M + F)
			  );
  r += (-0.58 * cos(M - 2*D)-0.46 * cos(2*D));

  xg = r * cos(lonEcl) * cos(latEcl);
  yg = r * sin(lonEcl) * cos(latEcl);
  zg = r *               sin(latEcl);
  
  xe = xg;
  ye = yg * cos(ecl) -zg * sin(ecl);
  ze = yg * sin(ecl) +zg * cos(ecl);

  geoRa  = atan2(ye, xe);
  geoDec = atan2(ze, sqrt(xe*xe + ye*ye));

  mpar = asin ( 1 / r);

  gclat = lat - 0.003358 * sin (2 * SGD_DEGREES_TO_RADIANS * lat );
  rho = 0.99883 + 0.00167 * cos(2 * SGD_DEGREES_TO_RADIANS * lat);
 
  if (geoRa < 0)
    geoRa += SGD_2PI;
  
  HA = lst - (3.8197186 * geoRa);
  g = atan (tan(gclat) / cos ((HA / 3.8197186)));

  rightAscension = geoRa - mpar * rho * cos(gclat) * sin(HA) / cos (geoDec);
  if (fabs(lat) > 0) 
  {
      declination = geoDec - mpar * rho * sin (gclat) * sin (g - geoDec) / sin(g);
  } else 
  {
      declination = geoDec;
  }
}
