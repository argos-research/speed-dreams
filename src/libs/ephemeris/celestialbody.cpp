/****************************************************************************

    file                 : celestialbody.cpp
    created              : Sun Aug 02 22:54:56 CET 2012
    copyright            : (C) 1997  Durk Talsma - (C)2012 Xavier Bertaux
    email                : bertauxx@yahoo.fr
    version              : $Id: celestialbody.cpp 4811 2012-07-29 17:16:37Z torcs-ng $

 ****************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "celestialbody.h"
#include "star.h"
 
void ePhCelestialBody::updatePosition(double mjd, ePhStar *ourSun)
{
  	double eccAnom, v, ecl, actTime, 
    	xv, yv, xh, yh, zh, xg, yg, zg, xe, ye, ze;

  	updateOrbElements(mjd);
  	actTime = sdCalcActTime(mjd);

  	// calcualate the angle bewteen ecliptic and equatorial coordinate system
  	ecl = SGD_DEGREES_TO_RADIANS * (23.4393 - 3.563E-7 *actTime);
  
  	eccAnom = sdCalcEccAnom(M, e);  //calculate the eccentric anomaly
  	xv = a * (cos(eccAnom) - e);
  	yv = a * (sqrt (1.0 - e*e) * sin(eccAnom));
  	v = atan2(yv, xv);           // the planet's true anomaly
  	r = sqrt (xv*xv + yv*yv);    // the planet's distance
  
  	// calculate the planet's position in 3D space
  	xh = r * (cos(N) * cos(v+w) - sin(N) * sin(v+w) * cos(i));
  	yh = r * (sin(N) * cos(v+w) + cos(N) * sin(v+w) * cos(i));
  	zh = r * (sin(v+w) * sin(i));

  	// calculate the ecliptic longitude and latitude
  	xg = xh + ourSun->getxs();
  	yg = yh + ourSun->getys();
  	zg = zh;

  	lonEcl = atan2(yh, xh);
  	latEcl = atan2(zh, sqrt(xh*xh+yh*yh));

  	xe = xg;
  	ye = yg * cos(ecl) - zg * sin(ecl);
  	ze = yg * sin(ecl) + zg * cos(ecl);
  	rightAscension = atan2(ye, xe);
  	declination = atan2(ze, sqrt(xe*xe + ye*ye));

  	//calculate some variables specific to calculating the magnitude 
  	//of the planet
  	R = sqrt (xg*xg + yg*yg + zg*zg);
  	s = ourSun->getDistance();

  	// It is possible from these calculations for the argument to acos
  	// to exceed the valid range for acos(). So we do a little extra
  	// checking.

  	double tmp = (r*r + R*R - s*s) / (2*r*R);
  	if ( tmp > 1.0) 
  	{ 
      		tmp = 1.0;
  	} else if ( tmp < -1.0) 
  	{
      		tmp = -1.0;
  	}

  	FV = SGD_RADIANS_TO_DEGREES * acos( tmp );
}

double ePhCelestialBody::sdCalcEccAnom(double M, double e)
{
  	double 
    	eccAnom, E0, E1, diff;
  
  	eccAnom = M + e * sin(M) * (1.0 + e * cos (M));
  	// iterate to achieve a greater precision for larger eccentricities 
  	if (e > 0.05)
    	{
      		E0 = eccAnom;
      		do
		{
	  		E1 = E0 - (E0 - e * sin(E0) - M) / (1 - e *cos(E0));
	  		diff = fabs(E0 - E1);
	  		E0 = E1;
		}
      		while (diff > (SGD_DEGREES_TO_RADIANS * 0.001));
      			return E0;
    	}
    	
  	return eccAnom;
}

ePhCelestialBody::ePhCelestialBody(double Nf, double Ns,
				    double If, double Is,
				    double wf, double ws,
				    double af, double as,
				    double ef, double es,
				    double Mf, double Ms, double mjd)
{
  	NFirst = Nf;     NSec = Ns;
  	iFirst = If;     iSec = Is;
  	wFirst = wf;     wSec = ws;
  	aFirst = af;     aSec = as;
  	eFirst = ef;     eSec = es;
  	MFirst = Mf;     MSec = Ms;
  	updateOrbElements(mjd);
}

ePhCelestialBody::ePhCelestialBody(double Nf, double Ns,
				    double If, double Is,
				    double wf, double ws,
				    double af, double as,
				    double ef, double es,
				    double Mf, double Ms)
{
  	NFirst = Nf;     NSec = Ns;
  	iFirst = If;     iSec = Is;
  	wFirst = wf;     wSec = ws;
  	aFirst = af;     aSec = as;
  	eFirst = ef;     eSec = es;
  	MFirst = Mf;     MSec = Ms;
}

void ePhCelestialBody::updateOrbElements(double mjd)
{
  	double actTime = sdCalcActTime(mjd);
   	M = SGD_DEGREES_TO_RADIANS * (MFirst + (MSec * actTime));
   	w = SGD_DEGREES_TO_RADIANS * (wFirst + (wSec * actTime));
   	N = SGD_DEGREES_TO_RADIANS * (NFirst + (NSec * actTime));
   	i = SGD_DEGREES_TO_RADIANS * (iFirst + (iSec * actTime));
   	e = eFirst + (eSec * actTime);
   	a = aFirst + (aSec * actTime);
}

/* return value: the (fractional) number of days until Jan 1, 2000.*/
/****************************************************************************/
double ePhCelestialBody::sdCalcActTime(double mjd)
{
  	return (mjd - 36523.5);
}

void ePhCelestialBody::getPos(double* ra, double* dec)
{
  	*ra  = rightAscension;
  	*dec = declination;
}

void ePhCelestialBody::getPos(double* ra, double* dec, double* magn)
{
	*ra = rightAscension;
  	*dec = declination;
  	*magn = magnitude;
}
