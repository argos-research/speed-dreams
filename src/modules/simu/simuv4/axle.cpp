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

void SimAxleConfig(tCar *car, int index)
{
	void	*hdle = car->params;
	tCarSetupItem *setupArbK = &(car->carElt->setup.arbSpring[index]);
	tCarSetupItem *setupRideHeightR = &(car->carElt->setup.rideHeight[index*2]);
	tCarSetupItem *setupRideHeightL = &(car->carElt->setup.rideHeight[index*2+1]);
	tAxle *axle = &(car->axle[index]);
	
	axle->xpos = GfParmGetNum(hdle, AxleSect[index], PRM_XPOS, (char*)NULL, 0.0f);
	axle->I    = GfParmGetNum(hdle, AxleSect[index], PRM_INERTIA, (char*)NULL, 0.15f);
	
	setupRideHeightR->desired_value = setupRideHeightR->min = setupRideHeightR->max = 0.20f;
	GfParmGetNumWithLimits(hdle, WheelSect[index*2], PRM_RIDEHEIGHT, (char*)NULL, &(setupRideHeightR->desired_value), &(setupRideHeightR->min), &(setupRideHeightR->max));
	setupRideHeightR->changed = TRUE;
	setupRideHeightR->stepsize = 0.001f;
	
	setupRideHeightL->desired_value = setupRideHeightL->min = setupRideHeightL->max = 0.20f;
	GfParmGetNumWithLimits(hdle, WheelSect[index*2+1], PRM_RIDEHEIGHT, (char*)NULL, &(setupRideHeightL->desired_value), &(setupRideHeightL->min), &(setupRideHeightL->max));
	setupRideHeightL->changed = TRUE;
	setupRideHeightL->stepsize = 0.001f;
	
	if (index == 0) {
		setupArbK->desired_value = setupArbK->min = setupArbK->max = 175000.0f;
		GfParmGetNumWithLimits(hdle, SECT_FRNTARB, PRM_SPR, (char*)NULL, &(setupArbK->desired_value), &(setupArbK->min), &(setupArbK->max));
		setupArbK->changed = TRUE;
		setupArbK->stepsize = 1000;
		SimSuspConfig(car, hdle, SECT_FRNTHEAVE, &(axle->heaveSusp), 4);
	} else {
		setupArbK->desired_value = setupArbK->min = setupArbK->max = 175000.0f;
		GfParmGetNumWithLimits(hdle, SECT_REARARB, PRM_SPR, (char*)NULL, &(setupArbK->desired_value), &(setupArbK->min), &(setupArbK->max));
		setupArbK->changed = TRUE;
		setupArbK->stepsize = 1000;
		SimSuspConfig(car, hdle, SECT_REARHEAVE, &(axle->heaveSusp), 5);
	}
	
	car->wheel[index*2].feedBack.I += (tdble) (axle->I / 2.0);
	car->wheel[index*2+1].feedBack.I += (tdble) (axle->I / 2.0);
}

void SimArbReConfig(tCar *car, int index)
{
	tCarSetupItem *setupArbK = &(car->carElt->setup.arbSpring[index]);
	tSuspension *arb = &(car->axle[index].arbSusp);
	if (setupArbK->changed) {
		arb->spring.K = MIN(setupArbK->max, MAX(setupArbK->min, setupArbK->desired_value));
		setupArbK->value = arb->spring.K;
		setupArbK->changed = FALSE;
	}
}

void SimAxleReConfig(tCar *car, int index, tdble weight0)
{/* called by SimCarReConfig() in car.cpp */
	tCarSetupItem *setupRideHeightR = &(car->carElt->setup.rideHeight[index*2]);
	tCarSetupItem *setupRideHeightL = &(car->carElt->setup.rideHeight[index*2+1]);
	tAxle *axle = &(car->axle[index]);
	tdble x0r, x0l;
	
	SimArbReConfig(car, index);
	
	if (setupRideHeightR->changed) {
		x0r = MIN(setupRideHeightR->max, MAX(setupRideHeightR->min, setupRideHeightR->desired_value));
		setupRideHeightR->value = x0r;
		setupRideHeightR->changed = FALSE;
	} else {
		x0r = setupRideHeightR->value;
	}
	if (setupRideHeightL->changed) {
		x0l = MIN(setupRideHeightL->max, MAX(setupRideHeightL->min, setupRideHeightL->desired_value));
		setupRideHeightL->value = x0l;
		setupRideHeightL->changed = FALSE;
	} else {
		x0l = setupRideHeightL->value;
	}
	if (index==0) {
		SimSuspReConfig(car, &(axle->heaveSusp), 4, weight0, (tdble) (0.5*(x0r+x0l)));
	} else {
		SimSuspReConfig(car, &(axle->heaveSusp), 5, weight0, (tdble) (0.5*(x0r+x0l)));
	}
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
	axle->heaveSusp.x = (tdble) (0.5 * (stl + str));
	axle->heaveSusp.v = (tdble) (0.5 * (vtl + vtr));
	SimSuspUpdate(&(axle->heaveSusp));
	f = (tdble) (0.5 * axle->heaveSusp.force);
	car->wheel[index*2].axleFz3rd = f;
	car->wheel[index*2+1].axleFz3rd = f;
}



