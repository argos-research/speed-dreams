/***************************************************************************

    file                 : axle.cpp
    created              : Sun Mar 19 00:05:09 CET 2000
    copyright            : (C) 2000 by Eric Espie
    email                : torcs@free.fr
    version              : $Id: axle.cpp 3948 2011-10-08 07:27:25Z wdbee $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "sim.h"

static const char *AxleSect[2] = {SECT_FRNTAXLE, SECT_REARAXLE};
static const char *WheelSect[4] = {SECT_FRNTRGTWHEEL, SECT_FRNTLFTWHEEL, SECT_REARRGTWHEEL, SECT_REARLFTWHEEL};

void SimAxleConfig(tCar *car, int index, tdble weight0)
{
	void	*hdle = car->params;
	tdble	rollCenter, x0r, x0l;
	
	tAxle *axle = &(car->axle[index]);
	
	axle->xpos = GfParmGetNum(hdle, AxleSect[index], PRM_XPOS, (char*)NULL, 0.0f);
	axle->I    = GfParmGetNum(hdle, AxleSect[index], PRM_INERTIA, (char*)NULL, 0.15f);
	x0r        = GfParmGetNum(hdle, WheelSect[index*2], PRM_RIDEHEIGHT, (char*)NULL, 0.20f);
	x0l        = GfParmGetNum(hdle, WheelSect[index*2+1], PRM_RIDEHEIGHT, (char*)NULL, 0.20f);
	rollCenter = GfParmGetNum(hdle, AxleSect[index], PRM_ROLLCENTER, (char*)NULL, 0.15f);
	car->wheel[index*2].rollCenter = car->wheel[index*2+1].rollCenter = rollCenter;
	
	if (index == 0) {
		SimSuspConfig(hdle, SECT_FRNTARB, &(axle->arbSusp), 0, 0);
		axle->arbSusp.spring.K = -axle->arbSusp.spring.K;
		SimSuspConfig(hdle, SECT_FRNTHEAVE, &(axle->heaveSusp), weight0, 0.5*(x0r+x0l));
	} else {
		SimSuspConfig(hdle, SECT_REARARB, &(axle->arbSusp), 0, 0);
		axle->arbSusp.spring.K = -axle->arbSusp.spring.K;
		SimSuspConfig(hdle, SECT_REARHEAVE, &(axle->heaveSusp), weight0, 0.5*(x0r+x0l));
	}
	
	car->wheel[index*2].feedBack.I += (tdble) (axle->I / 2.0);
	car->wheel[index*2+1].feedBack.I += (tdble) (axle->I / 2.0);
}




void SimAxleUpdate(tCar *car, int index)
{
	tAxle *axle = &(car->axle[index]);
	tdble str, stl, sgn, vtl, vtr;
	
	str = car->wheel[index*2].susp.x;
	stl = car->wheel[index*2+1].susp.x;
	vtr = car->wheel[index*2].susp.v;
	vtl = car->wheel[index*2+1].susp.v;
	
	sgn = (tdble) (SIGN(stl - str));
	axle->arbSusp.x = fabs(stl - str);		
	tSpring *spring = &(axle->arbSusp.spring);

	// To save CPU power we compute the force here directly. Just the spring
	// is considered.
	tdble f;
	f = spring->K * axle->arbSusp.x;
	
	// right
	car->wheel[index*2].axleFz =  + sgn * f;
	// left
	car->wheel[index*2+1].axleFz = - sgn * f;
	
	/* heave/center spring */
	axle->heaveSusp.x = 0.5 * (stl + str);
	axle->heaveSusp.v = 0.5 * (vtl + vtr);
	SimSuspUpdate(&(axle->heaveSusp));
	f = 0.5 * axle->heaveSusp.force;
	car->wheel[index*2].axleFz3rd = f;
	car->wheel[index*2+1].axleFz3rd = f;
}



