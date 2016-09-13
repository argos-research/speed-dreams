/***************************************************************************

    file                 : wheel.cpp
    created              : Sun Mar 19 00:09:06 CET 2000
    copyright            : (C) 2000 by Eric Espie
    email                : torcs@free.fr
    version              : $Id: wheel.cpp 4983 2012-10-07 13:53:17Z pouillot $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <tgf.h>
#include "sim.h"

static const char *WheelSect[4] = {SECT_FRNTRGTWHEEL, SECT_FRNTLFTWHEEL, SECT_REARRGTWHEEL, SECT_REARLFTWHEEL};
static const char *SuspSect[4] = {SECT_FRNTRGTSUSP, SECT_FRNTLFTSUSP, SECT_REARRGTSUSP, SECT_REARLFTSUSP};
static const char *BrkSect[4] = {SECT_FRNTRGTBRAKE, SECT_FRNTLFTBRAKE, SECT_REARRGTBRAKE, SECT_REARLFTBRAKE};

tdble Tair = 273; //air temperature in K
tdble Ttrack = 283; //track temperature in K

void
SimWheelConfig(tCar *car, int index)
{
	void *hdle = car->params;
	tCarElt *carElt = car->carElt;
	tWheel *wheel = &(car->wheel[index]);
	tdble rimdiam, tirewidth, tireratio, tireheight;
	tdble Ca, RFactor, EFactor;
	tCarSetupItem *setupToe = &(car->carElt->setup.toe[index]);
	tCarSetupItem *setupCamber = &(car->carElt->setup.camber[index]);
	tCarSetupItem *setupPressure = &(car->carElt->setup.tirePressure[index]);
	tCarSetupItem *setupOpLoad = &(car->carElt->setup.tireOpLoad[index]);
		
	/* Note: ride height is already read in SimAxleConfig() */
	
	setupToe->desired_value = setupToe->min = setupToe->max = 0.0f;
	GfParmGetNumWithLimits(hdle, WheelSect[index], PRM_TOE, (char*)NULL, &(setupToe->desired_value), &(setupToe->min), &(setupToe->max));
	setupToe->changed = TRUE;
	setupToe->stepsize = (tdble) DEG2RAD(0.1);
	
	setupCamber->desired_value = setupCamber->min = setupCamber->max = 0.0f;
	GfParmGetNumWithLimits(hdle, WheelSect[index], PRM_CAMBER, (char*)NULL, &(setupCamber->desired_value), &(setupCamber->min), &(setupCamber->max));
	setupCamber->changed = TRUE;
	setupCamber->stepsize = (tdble) DEG2RAD(0.1);

	setupPressure->desired_value = setupPressure->min = setupPressure->max = 275600;
	GfParmGetNumWithLimits(hdle, WheelSect[index], PRM_PRESSURE, (char*)NULL, &(setupPressure->desired_value), &(setupPressure->min), &(setupPressure->max));
	setupPressure->changed = TRUE;
	setupPressure->stepsize = 10000;
	
	setupOpLoad->desired_value = setupOpLoad->min = setupOpLoad->max = wheel->weight0 * 1.2f;
	GfParmGetNumWithLimits(hdle, WheelSect[index], PRM_OPLOAD, (char*)NULL, &(setupOpLoad->desired_value), &(setupOpLoad->min), &(setupOpLoad->max));
	setupOpLoad->changed = TRUE;
	setupOpLoad->stepsize = 100;
	
	rimdiam               = GfParmGetNum(hdle, WheelSect[index], PRM_RIMDIAM, (char*)NULL, 0.33f);
	tirewidth             = GfParmGetNum(hdle, WheelSect[index], PRM_TIREWIDTH, (char*)NULL, 0.145f);
	tireheight            = GfParmGetNum(hdle, WheelSect[index], PRM_TIREHEIGHT, (char*)NULL, -1.0f);
	tireratio             = GfParmGetNum(hdle, WheelSect[index], PRM_TIRERATIO, (char*)NULL, 0.75f);
	wheel->mu             = GfParmGetNum(hdle, WheelSect[index], PRM_MU, (char*)NULL, 1.0f);
	wheel->I              = GfParmGetNum(hdle, WheelSect[index], PRM_INERTIA, (char*)NULL, 1.5f);
	//BUG: the next line should go after SimBrakeConfig to have an effect
	wheel->I += wheel->brake.I; // add brake inertia
	wheel->staticPos.y    = GfParmGetNum(hdle, WheelSect[index], PRM_YPOS, (char*)NULL, 0.0f);
	Ca                    = GfParmGetNum(hdle, WheelSect[index], PRM_CA, (char*)NULL, 30.0f);
	RFactor               = GfParmGetNum(hdle, WheelSect[index], PRM_RFACTOR, (char*)NULL, 0.8f);
	EFactor               = GfParmGetNum(hdle, WheelSect[index], PRM_EFACTOR, (char*)NULL, 0.7f);
	wheel->lfMax          = GfParmGetNum(hdle, WheelSect[index], PRM_LOADFMAX, (char*)NULL, 1.6f);
	wheel->lfMin          = GfParmGetNum(hdle, WheelSect[index], PRM_LOADFMIN, (char*)NULL, 0.8f);
	wheel->mass           = GfParmGetNum(hdle, WheelSect[index], PRM_MASS, (char*)NULL, 20.0f);

	wheel->lfMin = MIN(0.9f, wheel->lfMin);
	wheel->lfMax = MAX(1.1f, wheel->lfMax);

	RFactor = MIN(1.0f, RFactor);
	RFactor = MAX(0.1f, RFactor);
	EFactor = MIN(1.0f, EFactor);

	if (tireheight > 0.0)
		wheel->radius = rimdiam / 2.0f + tireheight;
	else
		wheel->radius = rimdiam / 2.0f + tirewidth * tireratio;
	wheel->relPos.x = wheel->staticPos.x = car->axle[index/2].xpos;
	wheel->relPos.y = wheel->staticPos.y;
	/* BUG? susp.spring.x0 is still 0 here, maybe move after SimSuspReConfig in SimWheelReConfig? */
	wheel->relPos.z = wheel->radius - wheel->susp.spring.x0;
	wheel->relPos.ay = wheel->relPos.az = 0.0f;
	wheel->steer = 0.0f;
	
	/* temperature and degradation */
	wheel->Tinit = GfParmGetNum(hdle, WheelSect[index], PRM_INITTEMP, (char*)NULL, Tair);
	wheel->Ttire = wheel->Tinit;
	wheel->treadDepth = 1.0;
	wheel->Topt = GfParmGetNum(hdle, WheelSect[index], PRM_OPTTEMP, (char*)NULL, 350.0f);
	tdble coldmufactor = GfParmGetNum(hdle, WheelSect[index], PRM_COLDMUFACTOR, (char*)NULL, 1.0f);
	coldmufactor = MIN(MAX(coldmufactor, 0.0f), 1.0f);
	wheel->muTmult = (1 - coldmufactor) / ((wheel->Topt - Tair) * (wheel->Topt - Tair));
	wheel->heatingm = GfParmGetNum(hdle, WheelSect[index], PRM_HEATINGMULT, (char*)NULL, (tdble) 6e-5);
	wheel->aircoolm = GfParmGetNum(hdle, WheelSect[index], PRM_AIRCOOLINGMULT, (char*)NULL, (tdble) 12e-4);
	wheel->speedcoolm = GfParmGetNum(hdle, WheelSect[index], PRM_SPEEDCOOLINGMULT, (char*)NULL, (tdble) 0.25);
	wheel->wearrate = GfParmGetNum(hdle, WheelSect[index], PRM_WEARRATE, (char*)NULL, (tdble) 1.5e-8);
	wheel->wearrate = MIN(MAX(wheel->wearrate, 0.0f), 0.1f);
	wheel->critTreadDepth = GfParmGetNum(hdle, WheelSect[index], PRM_FALLOFFTREADDEPTH, (char*)NULL, (tdble) 0.03);
	wheel->critTreadDepth = MIN(MAX(wheel->critTreadDepth, 0.0001f), 0.9999f);
	wheel->muTDoffset[0] = GfParmGetNum(hdle, WheelSect[index], PRM_REMAININGGRIPMULT, (char*)NULL, (tdble) 0.5);
	wheel->muTDoffset[0] = MIN(MAX(wheel->muTDoffset[0], 0.1f), 1.0f);
	wheel->muTDoffset[1] = GfParmGetNum(hdle, WheelSect[index], PRM_FALLOFFGRIPMULT, (char*)NULL, (tdble) 0.85);
	wheel->muTDoffset[1] = MIN(MAX(wheel->muTDoffset[1], 0.1f), 1.0f);
	/* calculate parameters for straight mu(TreadDepth) sections */
	wheel->muTDmult[0] = (wheel->muTDoffset[1] - wheel->muTDoffset[0]) / wheel->critTreadDepth;
	wheel->muTDmult[1] = (tdble) ((1.0 - wheel->muTDoffset[1]) / (1.0 - wheel->critTreadDepth));
	wheel->muTDoffset[1] = wheel->muTDoffset[1] - wheel->muTDmult[1] * wheel->critTreadDepth;

	/* components */
	SimSuspConfig(car, hdle, SuspSect[index], &(wheel->susp), index);
	SimBrakeConfig(hdle, BrkSect[index], &(wheel->brake));

	carElt->_rimRadius(index) = rimdiam / 2.0f;
	if (tireheight > 0.0)
		carElt->_tireHeight(index) = tireheight;
	else
		carElt->_tireHeight(index) = tirewidth * tireratio;
	carElt->_tireWidth(index) = tirewidth;
	carElt->_brakeDiskRadius(index) = wheel->brake.radius;
	carElt->_wheelRadius(index) = wheel->radius;
    if (car->features & FEAT_TIRETEMPDEG)
    {
        // Assume new wheels
        carElt->_tyreCondition(index) = 1.0;
		car->carElt->_tyreTreadDepth(index) = wheel->treadDepth;
		car->carElt->_tyreCritTreadDepth(index) = wheel->critTreadDepth;
    }

	wheel->mfC = (tdble)(2.0 - asin(RFactor) * 2.0 / PI);
	wheel->mfB = Ca / wheel->mfC;
	wheel->mfE = EFactor;

	wheel->lfK = log((1.0f - wheel->lfMin) / (wheel->lfMax - wheel->lfMin));

	wheel->feedBack.I += wheel->I;
	wheel->feedBack.spinVel = 0.0f;
	wheel->feedBack.Tq = 0.0f;
	wheel->feedBack.brkTq = 0.0f;
	wheel->torques.x = wheel->torques.y = wheel->torques.z = 0.0f;
	
	/* calculate optimal slip value */
	tdble s, Bx, low, high;
	int i;
	//wheel->mfC * atan(Bx * (1.0f - wheel->mfE) + wheel->mfE * atan(Bx)) == PI/2
	low = 0.0;
	high = wheel->mfB;
	
	if (wheel->mfC * atan(high * (1.0f - wheel->mfE) + wheel->mfE * atan(high)) < PI_2) {
		/* tire parameters are unphysical*/
		s = 1.0;
		GfLogWarning("Tire magic curve parameters are unphysical!");
	} else {
		for (i = 0; i < 32; i++) {
			Bx = (tdble)(0.5 * (low + high));
			if (wheel->mfC * atan(Bx * (1.0f - wheel->mfE) + wheel->mfE * atan(Bx)) < PI_2) {
				low = Bx;
			} else {
				high = Bx;
			}
		}
		s = (tdble)(0.5 * (low + high) / wheel->mfB);
	}
	car->carElt->_wheelSlipOpt(index) = s;
}


void
SimWheelReConfig(tCar *car, int index)
{/* called by SimCarReConfig in car.cpp */
	tCarElt *carElt = car->carElt;
	tWheel *wheel = &(car->wheel[index]);
	tdble x0;
	
	tCarSetupItem *setupRideHeight = &(car->carElt->setup.rideHeight[index]);
	tCarSetupItem *setupToe = &(car->carElt->setup.toe[index]);
	tCarSetupItem *setupCamber = &(car->carElt->setup.camber[index]);
	tCarSetupItem *setupPressure = &(car->carElt->setup.tirePressure[index]);
	tCarSetupItem *setupOpLoad = &(car->carElt->setup.tireOpLoad[index]);
	tdble pressure, patchLen;
	
	if (setupToe->changed) {
		wheel->staticPos.az = MIN(setupToe->max, MAX(setupToe->min, setupToe->desired_value));
		setupToe->value = wheel->staticPos.az;
		setupToe->changed = FALSE;
	}
	
	if (setupCamber->changed) {
		wheel->staticPos.ax = MIN(setupCamber->max, MAX(setupCamber->min, setupCamber->desired_value));
		if (index % 2) {
			wheel->relPos.ax = -wheel->staticPos.ax;
		} else {
			wheel->relPos.ax = wheel->staticPos.ax;
		}
		wheel->cosax = cos(wheel->relPos.ax);
		wheel->sinax = sin(wheel->relPos.ax);
		setupCamber->value = wheel->staticPos.ax;
		setupCamber -> changed = FALSE;
	}
	
	if ( setupPressure->changed ||car->carElt->setup.FRWeightRep.changed ) {
		/* NOTE: these variables are unused as of 2015.11.14. */
		pressure = MIN(setupPressure->max, MAX(setupPressure->min, setupPressure->desired_value));
		patchLen = wheel->weight0 / (carElt->_tireWidth(index) * pressure);
		wheel->tireSpringRate = wheel->weight0 / (wheel->radius * (1.0f - cos(asin(patchLen / (2.0f * wheel->radius)))));
		setupPressure->value = pressure;
		setupPressure->changed = FALSE;
	}
	
	if (setupOpLoad->changed) {
		wheel->opLoad = MIN(setupOpLoad->max, MAX(setupOpLoad->min, setupOpLoad->desired_value));
		setupOpLoad->value = wheel->opLoad;
		setupOpLoad->changed = FALSE;
	}
	
	x0 = setupRideHeight->value;
	SimSuspReConfig(car, &(wheel->susp), index, wheel->weight0, x0);
}


void SimWheelUpdateRide(tCar *car, int index)
{
	tWheel *wheel = &(car->wheel[index]);
	tdble Zroad;

	// compute suspension travel
	RtTrackGlobal2Local(car->trkPos.seg, wheel->pos.x, wheel->pos.y, &(wheel->trkPos), TR_LPOS_SEGMENT);
	wheel->zRoad = Zroad = RtTrackHeightL(&(wheel->trkPos));

	// Wheel susp.x is not the wheel movement, look at SimSuspCheckIn, it becomes there scaled with
	// susp->spring.bellcrank, so we invert this here.

	tdble new_susp_x = (wheel->susp.x - wheel->susp.v * SimDeltaTime) / wheel->susp.spring.bellcrank;
    tdble max_extend =  wheel->pos.z - Zroad;
	wheel->rideHeight = max_extend;

	if (max_extend > new_susp_x + 0.01) {
		wheel->susp.state = SIM_WH_INAIR;
	} else {wheel->susp.state = 0;}
	
	if (max_extend < new_susp_x) {
		new_susp_x = max_extend;
	}

	tdble prex = wheel->susp.x;
	tdble prev = wheel->susp.v;
	wheel->susp.x = new_susp_x;
	
	// verify the suspension travel, beware, wheel->susp.x will be scaled by SimSuspCheckIn
	SimSuspCheckIn(&(wheel->susp));
	wheel->susp.v = (prex - wheel->susp.x) / SimDeltaTime;
	wheel->susp.a = (prev - wheel->susp.v) / SimDeltaTime;
	
	// update wheel brake
	SimBrakeUpdate(car, wheel, &(wheel->brake));

	// Option TCL ...
	if (car->features & FEAT_TCLINSIMU)
	{
		if (index == 3) 
		{	// After using the values for the last wheel
			tEngine	*engine = &(car->engine);
			engine->TCL = 1.0;			// Reset the TCL accel command
		}
	}
	// ... Option TCL
}




void SimWheelUpdateForce(tCar *car, int index)
{
	tWheel *wheel = &(car->wheel[index]);
	tdble axleFz = wheel->axleFz;
	tdble vt, v, v2, wrl; // wheel related velocity
	tdble Fn, Ft;
	tdble waz;
	tdble CosA, SinA;
	tdble s, sa, sx, sy; // slip vector
	tdble stmp, F, Bx;
	tdble mu;
	tdble tireCond = 1.0;
	tdble reaction_force = 0.0f;
	wheel->state = 0;

	// VERTICAL STUFF CONSIDERING SMALL PITCH AND ROLL ANGLES
	// update suspension force
	SimSuspUpdate(&(wheel->susp));
	// check suspension state
	wheel->state |= wheel->susp.state;
	if ( ((wheel->state & SIM_SUSP_EXT) == 0) && ((wheel->state & SIM_WH_INAIR) == 0) ) {
		wheel->forces.z = axleFz + wheel->susp.force + wheel->axleFz3rd;
		if (car->features & FEAT_FIXEDWHEELFORCE) {
			wheel->susp.v -= wheel->susp.spring.bellcrank * SimDeltaTime * wheel->forces.z / wheel->mass;
		} else {
			wheel->susp.v -= wheel->susp.spring.bellcrank * SimDeltaTime * wheel->susp.force / wheel->mass;
		}
		if (wheel->forces.z < 0.0f) {
			wheel->forces.z = 0.0f;
		}
	} else {
        if (wheel->state & SIM_SUSP_EXT) {
			/* calculate the force needed to reach susp->spring.xMax 
			 * it becomes 0 from the 2. time step being extended 
			 * works even if both SIM_SUSP_EXT and SIM_WH_INAIR is set */
			wheel->forces.z = -wheel->susp.a * wheel->mass / wheel->susp.spring.bellcrank;
			wheel->susp.v = 0.0f;
		} else { //SIM_WH_INAIR is set, but SIM_SUSP_EXT is not
			wheel->forces.z = axleFz + wheel->susp.force + wheel->axleFz3rd;
			if (car->features & FEAT_FIXEDWHEELFORCE) {
			    wheel->susp.v -= wheel->susp.spring.bellcrank * SimDeltaTime * wheel->forces.z / wheel->mass;
		    } else {
			    wheel->susp.v -= wheel->susp.spring.bellcrank * SimDeltaTime * wheel->susp.force / wheel->mass;
		    }
		    wheel->forces.z = 0.0f; /* zero for zero grip and prevent getting into the air */
		}
	}
	reaction_force = wheel->forces.z;

	// update wheel coord, center relative to GC
	wheel->relPos.z = - wheel->susp.x / wheel->susp.spring.bellcrank + wheel->radius;

	// HORIZONTAL FORCES
	waz = wheel->steer + wheel->staticPos.az;
	CosA = cos(waz);
	SinA = sin(waz);

	// tangent velocity.
	vt = wheel->bodyVel.x * CosA + wheel->bodyVel.y * SinA;
	v2 = wheel->bodyVel.x * wheel->bodyVel.x + wheel->bodyVel.y * wheel->bodyVel.y;
	v = sqrt(v2);

	// slip angle
	if (v < 0.000001f) {
		sa = 0.0f;
	} else {
		sa = atan2(wheel->bodyVel.y, wheel->bodyVel.x) - waz;
	}
	FLOAT_NORM_PI_PI(sa);

	wrl = wheel->spinVel * wheel->radius;
	if ((wheel->state & SIM_SUSP_EXT) != 0) {
		sx = sy = 0.0f;
	} else if (v < 0.000001f) {
		if (car->features & FEAT_SLOWGRIP) {
			sx = -wrl;
		} else {
			sx = wrl;
		}
		sy = 0.0f;
	} else {
		if (car->features & FEAT_SLOWGRIP) {
			sx = (vt - wrl) / MAX(fabs(vt), 1.0f); //avoid divergence
			sy = sin(sa);
		} else {
			sx = (vt - wrl) / fabs(vt);
			sy = sin(sa);
		}
	}

	Ft = 0.0f;
	Fn = 0.0f;
	s = sqrt(sx*sx+sy*sy);

	{
		// calculate _skid and _reaction for sound.
		if (v < 2.0f) {
			car->carElt->_skid[index] = 0.0f;
		} else {
			car->carElt->_skid[index] =  MIN(1.0f, (s*reaction_force*0.0002f));
		}
		car->carElt->_reaction[index] = reaction_force;
	}

	stmp = MIN(s, 150.0f);

	// MAGIC FORMULA
	Bx = wheel->mfB * stmp;
	F = sin(wheel->mfC * atan(Bx * (1.0f - wheel->mfE) + wheel->mfE * atan(Bx))) * (1.0f + stmp * simSkidFactor[car->carElt->_skillLevel]);

	// load sensitivity
	mu = wheel->mu * (wheel->lfMin + (wheel->lfMax - wheel->lfMin) * exp(wheel->lfK * wheel->forces.z / wheel->opLoad));
	
	//temperature and degradation
	if (car->features & FEAT_TIRETEMPDEG) {
		tireCond = 1 - wheel->muTmult * (wheel->Ttire - wheel->Topt)*(wheel->Ttire - wheel->Topt);
		if(wheel->treadDepth > wheel->critTreadDepth) {
			tireCond *= wheel->muTDmult[1] * wheel->treadDepth + wheel->muTDoffset[1];
		} else {
			tireCond *= wheel->muTDmult[0] * wheel->treadDepth + wheel->muTDoffset[0];
		}
		tireCond = (tdble) MAX(tireCond, 0.1);
		mu *= tireCond;
	}

	F *= wheel->forces.z * mu * wheel->trkPos.seg->surface->kFriction;	/* coeff */

	// For debugging weather simultation on some tracks
	#ifdef SD_DEBUG
        //GfLogDebug("Simu v2.1 kFriction : %f   ", wheel->trkPos.seg->surface->kFriction);
	#endif

	wheel->rollRes = wheel->forces.z * wheel->trkPos.seg->surface->kRollRes;
    	car->carElt->priv.wheel[index].rollRes = wheel->rollRes;

	if (s > 0.000001f) {
		// wheel axis based
		Ft -= F * sx / s;
		Fn -= F * sy / s;
	} else {
		Ft -=F;
	}
	
	if ( !(car->features & FEAT_SLOWGRIP) ) {
		FLOAT_RELAXATION2(Fn, wheel->preFn, 50.0f);
		FLOAT_RELAXATION2(Ft, wheel->preFt, 50.0f);
	}

	wheel->relPos.az = waz;

	wheel->forces.x = Ft * CosA - Fn * SinA;
	wheel->forces.y = Ft * SinA + Fn * CosA;
	wheel->spinTq = Ft * wheel->radius;
	wheel->sa = sa;
	wheel->sx = sx;

	wheel->feedBack.spinVel = wheel->spinVel;
	wheel->feedBack.Tq = wheel->spinTq;
	wheel->feedBack.brkTq = wheel->brake.Tq;

	car->carElt->_wheelSlipNorm(index) = stmp;
	car->carElt->_wheelSlipSide(index) = sy*v;
	car->carElt->_wheelSlipAccel(index) = sx*v;
	car->carElt->_reaction[index] = reaction_force;
	car->carElt->_tyreEffMu(index) = mu;
	
	tdble Work = 0.0;
	/* update tire temperature and degradation */
	if (car->features & FEAT_TIRETEMPDEG) {
		//heat from the work of friction
		Work = (wheel->forces.x * (wrl * CosA - wheel->bodyVel.x)
				+ wheel->forces.y * (wrl * SinA - wheel->bodyVel.y)) * SimDeltaTime;
		wheel->Ttire += Work * wheel->heatingm;
		//air cooling
		wheel->Ttire -= wheel->aircoolm * (1 + wheel->speedcoolm * v) * (wheel->Ttire - Tair) * SimDeltaTime;
		//tire wear
		if(wheel->treadDepth > 0.0) {wheel->treadDepth -= wheel->wearrate * Work;}
		else {wheel->treadDepth = 0.0;} //note: lets it go to slightly negative for one cycle
		//filling carElt
		car->carElt->_tyreT_in(index) = wheel->Ttire;
		car->carElt->_tyreT_mid(index) = wheel->Ttire;
		car->carElt->_tyreT_out(index) = wheel->Ttire;
		car->carElt->_tyreCondition(index) = tireCond;
		car->carElt->_tyreTreadDepth(index) = wheel->treadDepth;
		car->carElt->_tyreCritTreadDepth(index) = wheel->critTreadDepth;
	}

	// Option TCL ...
	if (car->features & FEAT_TCLINSIMU)
	{
		tdble TCL_SlipScale = 1.00f;	// Make it be a parameter later
		tdble TCL_AccelScale = 0.9f;	// Make it be a parameter later

		tEngine	*engine = &(car->engine); // Get engine
		if (sx < -TCL_SlipScale)          // Slip is over our limit
		{	// Store the TCL_Brake command for this wheel
			wheel->brake.TCL = -sx;
			// Store the minimum TCL_Accel command for the engine
			engine->TCL = (tdble) MIN(TCL_AccelScale * wheel->brake.TCL,engine->TCL);
			// fprintf(stderr,"sx: %.1f TCL: %.3f %%\n",sx,wheel->brake.TCL);
		};
	}
    // ... Option TCL

	// Option ABS ...
	if (car->features & FEAT_ABSINSIMU)
	{
		tdble ABS_SlipScale = 0.1f;		// Make it be a parameter later
		tdble ABS_BrakeScale = 1.0f;	// Make it be a parameter later

		// If slip is over the limit, reduce brake command for this wheel
		if (sx > ABS_SlipScale)			
            wheel->brake.ABS = (tdble) MAX(0.0,MIN(1.0,1 - ABS_BrakeScale * sx));
		else
			wheel->brake.ABS = 1.0f;
	}
    // ... Option ABS
}


void
SimWheelUpdateRotation(tCar *car)
{
	int i;
	tWheel *wheel;
	tdble deltan;
	tdble cosaz, sinaz;

	tdble maxslip = 0.0;

	for (i = 0; i < 4; i++) {
		wheel = &(car->wheel[i]);
		/*calculate gyroscopic forces*/
		cosaz = cos(wheel->relPos.az);
		sinaz = sin(wheel->relPos.az);
		if( (i == 0) || (i == 1) ){
			wheel->torques.y = wheel->torques.x * sinaz;
			wheel->torques.x = wheel->torques.x * cosaz;
		} else {
			wheel->torques.x = wheel->torques.y =0.0;
		}
		deltan = -(wheel->in.spinVel - wheel->prespinVel) * wheel->I / SimDeltaTime;
		wheel->torques.x -= deltan * wheel->cosax *sinaz;
		wheel->torques.y += deltan * wheel->cosax *cosaz;
		wheel->torques.z = deltan * wheel->sinax;
		/*update rotation*/
		wheel->spinVel = wheel->in.spinVel;
		
		if ( (car->features & FEAT_SLOWGRIP) && (wheel->brake.Tq <= 1.0) && (car->ctrl->accelCmd * car->transmission.clutch.transferValue < 0.05) ) {
			/* prevent wheelspin value oscillating around wheel tangential velocity */
			tdble waz = wheel->steer + wheel->staticPos.az;
			tdble vt = wheel->bodyVel.x * cos(waz) + wheel->bodyVel.y * sin(waz);
			tdble wrl = wheel->spinVel * wheel->radius;
			tdble oldwrl = wheel->prespinVel * wheel->radius;
			if( (vt-wrl)*(vt-oldwrl) < 0.0 ) {
				wheel->spinVel = vt / wheel->radius;
			}
			wheel->prespinVel = wheel->spinVel;
		} else {
			FLOAT_RELAXATION2(wheel->spinVel, wheel->prespinVel, 50.0f);
		}

		wheel->relPos.ay += wheel->spinVel * SimDeltaTime;
		FLOAT_NORM_PI_PI(wheel->relPos.ay);
		car->carElt->_wheelSpinVel(i) = wheel->spinVel;

		// Option TCL ...
		if (car->features & FEAT_TCLINSIMU)
		{

			if (maxslip < wheel->brake.TCL)
				maxslip = wheel->brake.TCL;
		}
		// ... Option TCL
	}

	// Option TCL ...
	if (maxslip > 0.0)	
	{
		for (i = 0; i < 4; i++) 
		{
			wheel = &(car->wheel[i]);
			if (wheel->brake.TCL != maxslip)
				wheel->brake.TCL = 0.0;
		}
	}
	// ... Option TCL
}


void
SimUpdateFreeWheels(tCar *car, int axlenb)
{
	int i;
	tWheel *wheel;
	tdble BrTq;		// brake torque
	tdble ndot;		// rotation acceleration
	tdble I;

	for (i = axlenb * 2; i < axlenb * 2 + 2; i++) {
		wheel = &(car->wheel[i]);

        I = wheel->I + car->axle[axlenb].I / 2.0f;

		ndot = SimDeltaTime * wheel->spinTq / I;
		wheel->spinVel -= ndot;

		BrTq = (tdble)(- SIGN(wheel->spinVel) * wheel->brake.Tq);
		ndot = SimDeltaTime * BrTq / I;

        if (fabs(ndot) > fabs(wheel->spinVel)) {
			ndot = -wheel->spinVel;
		}

		wheel->spinVel += ndot;
		wheel->in.spinVel = wheel->spinVel;
	}
}
