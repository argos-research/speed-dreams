/***************************************************************************

    file                 : susp.cpp
    created              : Sun Mar 19 00:08:41 CET 2000
    copyright            : (C) 2000 by Eric Espie
    email                : torcs@free.fr
    version              : $Id: susp.cpp 3133 2010-11-17 22:00:21Z kakukri $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <stdio.h>
#include "sim.h"

/*
 * b2 and b3 calculus
 */
static void initDamper(tSuspension *susp)
{
	tDamper *damp;
	
	damp = &(susp->damper);	
	damp->bump.b2 = (damp->bump.C1 - damp->bump.C2) * damp->bump.v1 + damp->bump.b1;
	damp->rebound.b2 = (damp->rebound.C1 - damp->rebound.C2) * damp->rebound.v1 + damp->rebound.b1;
}




/*
 * get damper force
 */
static tdble damperForce(tSuspension *susp)
{
	tDamperDef *dampdef;
	tdble     f;
	tdble     av;
	tdble     v;

	v = susp->v;
	
	if (fabs(v) > 10.0f) {
		v = (float)(SIGN(v) * 10.0);
	}
	
	if (v < 0.0f) {
		/* rebound */
		dampdef = &(susp->damper.rebound);
	} else {
		/* bump */
		dampdef = &(susp->damper.bump);
	}
	
	av = fabs(v);
	if (av < dampdef->v1) {
		f = (dampdef->C1 * av + dampdef->b1);
	} else {
		f = (dampdef->C2 * av + dampdef->b2);
	}
	
	f *= (float)(SIGN(v));
	
	return f;
}




/*
 * get spring force
 */
static tdble springForce(tSuspension *susp)
{
	tSpring *spring = &(susp->spring);
	tdble f;
	
	/* K is < 0 */
	f = spring->K * (susp->x - spring->x0) + spring->F0;
	if (f < 0.0f) {
		f = 0.0f;
	}
	
	return f;
}




void SimSuspCheckIn(tSuspension *susp)
{
	/*susp->state = 0;*/
	/* note: susp->state is reset in SimWheelUpdateRide in wheel.cpp */
	if (susp->x < susp->spring.packers) {
		susp->x = susp->spring.packers;
		susp->state |= SIM_SUSP_COMP;
	}
	if (susp->x >= susp->spring.xMax) {
		susp->x = susp->spring.xMax;
		susp->state |= SIM_SUSP_EXT;
	}
	susp->x *= susp->spring.bellcrank;
}




void SimSuspUpdate(tSuspension *susp)
{
	tdble prevforce = susp->force;
	susp->force = (springForce(susp) + damperForce(susp) + susp->inertance * susp->a) * susp->spring.bellcrank;
	if (susp->force * prevforce < 0.0) {susp->force = 0.0;}
}




void SimSuspConfig(tCar *car, void *hdle, const char *section, tSuspension *susp, int index)
{
	tCarSetupItem *setupSpring, *setupBellcrank, *setupInertance;
	tCarSetupItem *setupFastBump, *setupSlowBump, *setupBumpLvel;
	tCarSetupItem *setupFastReb, *setupSlowReb, *setupRebLvel;
	tCarSetupItem *setupCourse, *setupPacker;
	
	if (index < 4) {//corner spring
		setupSpring = &(car->carElt->setup.suspSpring[index]);
		setupBellcrank = &(car->carElt->setup.suspBellcrank[index]);
		setupInertance = &(car->carElt->setup.suspInertance[index]);
		setupFastBump = &(car->carElt->setup.suspFastBump[index]);
		setupSlowBump = &(car->carElt->setup.suspSlowBump[index]);
		setupBumpLvel = &(car->carElt->setup.suspBumpLvel[index]);
		setupFastReb = &(car->carElt->setup.suspFastRebound[index]);
		setupSlowReb = &(car->carElt->setup.suspSlowRebound[index]);
		setupRebLvel = &(car->carElt->setup.suspReboundLvel[index]);
		setupCourse = &(car->carElt->setup.suspCourse[index]);
		setupPacker = &(car->carElt->setup.suspPacker[index]);
	} else {//heave spring
		setupSpring = &(car->carElt->setup.heaveSpring[index-4]);
		setupBellcrank = &(car->carElt->setup.heaveBellcrank[index-4]);
		setupInertance = &(car->carElt->setup.heaveInertance[index-4]);
		setupFastBump = &(car->carElt->setup.heaveFastBump[index-4]);
		setupSlowBump = &(car->carElt->setup.heaveSlowBump[index-4]);
		setupBumpLvel = &(car->carElt->setup.heaveBumpLvel[index-4]);
		setupFastReb = &(car->carElt->setup.heaveFastRebound[index-4]);
		setupSlowReb = &(car->carElt->setup.heaveSlowRebound[index-4]);
		setupRebLvel = &(car->carElt->setup.heaveReboundLvel[index-4]);
		setupCourse = NULL;
		setupPacker = NULL;
	}
	
	if ( index < 4 ) {
		setupSpring->desired_value = setupSpring->min = setupSpring-> max = 175000.0f;
	} else {/* default heave = 0 */
		setupSpring->desired_value = setupSpring->min = setupSpring-> max = 0.0f;
	}
	GfParmGetNumWithLimits(hdle, section, PRM_SPR, (char*)NULL, &(setupSpring->desired_value), &(setupSpring->min), &(setupSpring->max));
	setupSpring->changed = TRUE;
	setupSpring->stepsize = 1000;
	
	setupBellcrank->desired_value = setupBellcrank->min = setupBellcrank-> max = 1.0f;
	GfParmGetNumWithLimits(hdle, section, PRM_BELLCRANK, (char*)NULL, &(setupBellcrank->desired_value), &(setupBellcrank->min), &(setupBellcrank->max));
	setupBellcrank->changed = TRUE;
	setupBellcrank->stepsize = 0.1f;
	
	setupInertance->desired_value = setupInertance->min = setupInertance-> max = 0.0f;
	//Inertance is not yet used in car setup files.
	setupInertance->changed = TRUE;
	setupInertance->stepsize = 0.0;
	
	setupSlowBump->desired_value = setupSlowBump->min = setupSlowBump->max = 0.0f;
	GfParmGetNumWithLimits(hdle, section, PRM_SLOWBUMP, (char*)NULL, &(setupSlowBump->desired_value), &(setupSlowBump->min), &(setupSlowBump->max));
	setupSlowBump->changed = TRUE;
	setupSlowBump->stepsize = 100;
	
	setupSlowReb->desired_value = setupSlowReb->min = setupSlowReb->max = 0.0f;
	GfParmGetNumWithLimits(hdle, section, PRM_SLOWREBOUND, (char*)NULL, &(setupSlowReb->desired_value), &(setupSlowReb->min), &(setupSlowReb->max));
	setupSlowReb->changed = TRUE;
	setupSlowReb->stepsize = 100;
	
	setupFastBump->desired_value = setupFastBump->min = setupFastBump->max = 0.0f;
	GfParmGetNumWithLimits(hdle, section, PRM_FASTBUMP, (char*)NULL, &(setupFastBump->desired_value), &(setupFastBump->min), &(setupFastBump->max));
	setupFastBump->changed = TRUE;
	setupFastBump->stepsize = 100;
	
	setupFastReb->desired_value = setupFastReb->min = setupFastReb->max = 0.0f;
	GfParmGetNumWithLimits(hdle, section, PRM_FASTREBOUND, (char*)NULL, &(setupFastReb->desired_value), &(setupFastReb->min), &(setupFastReb->max));
	setupFastReb->changed = TRUE;
	setupFastReb->stepsize = 100;
	
	setupBumpLvel->desired_value = setupBumpLvel->min = setupBumpLvel->max = 0.5f;
	GfParmGetNumWithLimits(hdle, section, PRM_BUMPLVEL, (char*)NULL, &(setupBumpLvel->desired_value), &(setupBumpLvel->min), &(setupBumpLvel->max));
	setupBumpLvel->changed = TRUE;
	setupBumpLvel->stepsize = 0.01f;
	
	setupRebLvel->desired_value = setupRebLvel->min = setupRebLvel->max = 0.5f;
	GfParmGetNumWithLimits(hdle, section, PRM_REBOUNDLVEL, (char*)NULL, &(setupRebLvel->desired_value), &(setupRebLvel->min), &(setupRebLvel->max));
	setupRebLvel->changed = TRUE;
	setupRebLvel->stepsize = 0.01f;
	
	if (index<4) {
		setupCourse->desired_value = setupCourse->min = setupCourse->max = 0.5f;
		GfParmGetNumWithLimits(hdle, section, PRM_SUSPCOURSE, (char*)NULL, &(setupCourse->desired_value), &(setupCourse->min), &(setupCourse->max));
		setupCourse->changed = TRUE;
		setupCourse->stepsize = 0.001f;
		
		setupPacker->desired_value = setupPacker->min = setupPacker->max = 0.0f;
		GfParmGetNumWithLimits(hdle, section, PRM_PACKERS, (char*)NULL, &(setupPacker->desired_value), &(setupPacker->min), &(setupPacker->max));
		setupPacker->changed = TRUE;
		setupPacker->stepsize = 0.001f;
	}
}


void SimSuspReConfig(tCar *car, tSuspension *susp, int index, tdble F0, tdble X0)
{/* called by SimWheelReConfig() in wheel.cpp or SimAxleReConfig() in axle.cpp */
	tCarSetupItem *setupSpring, *setupBellcrank, *setupInertance;
	tCarSetupItem *setupFastBump, *setupSlowBump, *setupBumpLvel;
	tCarSetupItem *setupFastReb, *setupSlowReb, *setupRebLvel;
	tCarSetupItem *setupCourse, *setupPacker;
	bool damperchanged = FALSE;
	
	if (index < 4) {//corner springs
		setupSpring = &(car->carElt->setup.suspSpring[index]);
		setupBellcrank = &(car->carElt->setup.suspBellcrank[index]);
		setupInertance = &(car->carElt->setup.suspInertance[index]);
		setupFastBump = &(car->carElt->setup.suspFastBump[index]);
		setupSlowBump = &(car->carElt->setup.suspSlowBump[index]);
		setupBumpLvel = &(car->carElt->setup.suspBumpLvel[index]);
		setupFastReb = &(car->carElt->setup.suspFastRebound[index]);
		setupSlowReb = &(car->carElt->setup.suspSlowRebound[index]);
		setupRebLvel = &(car->carElt->setup.suspReboundLvel[index]);
		setupCourse = &(car->carElt->setup.suspCourse[index]);
		setupPacker = &(car->carElt->setup.suspPacker[index]);
	} else {//heave springs: 4 = front heave, 5 = rear heave
		setupSpring = &(car->carElt->setup.heaveSpring[index-4]);
		setupBellcrank = &(car->carElt->setup.heaveBellcrank[index-4]);
		setupInertance = &(car->carElt->setup.heaveInertance[index-4]);
		setupFastBump = &(car->carElt->setup.heaveFastBump[index-4]);
		setupSlowBump = &(car->carElt->setup.heaveSlowBump[index-4]);
		setupBumpLvel = &(car->carElt->setup.heaveBumpLvel[index-4]);
		setupFastReb = &(car->carElt->setup.heaveFastRebound[index-4]);
		setupSlowReb = &(car->carElt->setup.heaveSlowRebound[index-4]);
		setupRebLvel = &(car->carElt->setup.heaveReboundLvel[index-4]);
		setupCourse = NULL;
		setupPacker = NULL;
	}
	
	if (setupSpring->changed) {
		susp->spring.K = - MIN(setupSpring->max, MAX(setupSpring->min, setupSpring->desired_value));
		setupSpring->value = - susp->spring.K;
		setupSpring->changed = FALSE;
	}
	
	if (setupBellcrank->changed) {
		susp->spring.bellcrank = MIN(setupBellcrank->max, MAX(setupBellcrank->min, setupBellcrank->desired_value));
		setupBellcrank->value = susp->spring.bellcrank;
		setupBellcrank->changed = FALSE;
	}
	susp->spring.x0 = susp->spring.bellcrank * X0;
	susp->spring.F0 = F0 / susp->spring.bellcrank;
	
	if (setupInertance->changed) {
		susp->inertance = MIN(setupInertance->max, MAX(setupInertance->min, setupInertance->desired_value));
		setupInertance->value = susp->inertance;
		setupInertance->changed = FALSE;
	}
	
	if (setupSlowBump->changed) {
		susp->damper.bump.C1 = MIN(setupSlowBump->max, MAX(setupSlowBump->min, setupSlowBump->desired_value));
		setupSlowBump->value = susp->damper.bump.C1;
		setupSlowBump->changed = FALSE;
		damperchanged = TRUE;
	}
	
	if (setupSlowReb->changed) {
		susp->damper.rebound.C1 = MIN(setupSlowReb->max, MAX(setupSlowReb->min, setupSlowReb->desired_value));
		setupSlowReb->value = susp->damper.rebound.C1;
		setupSlowReb->changed = FALSE;
		damperchanged = TRUE;
	}
	
	if (setupFastBump->changed) {
		susp->damper.bump.C2 = MIN(setupFastBump->max, MAX(setupFastBump->min, setupFastBump->desired_value));
		setupFastBump->value = susp->damper.bump.C2;
		setupFastBump->changed = FALSE;
		damperchanged = TRUE;
	}
	
	if (setupFastReb->changed) {
		susp->damper.rebound.C2 = MIN(setupFastReb->max, MAX(setupFastReb->min, setupFastReb->desired_value));
		setupFastReb->value = susp->damper.rebound.C2;
		setupFastReb->changed = FALSE;
		damperchanged = TRUE;
	}
	
	if (setupBumpLvel->changed) {
		susp->damper.bump.v1 = MIN(setupBumpLvel->max, MAX(setupBumpLvel->min, setupBumpLvel->desired_value));
		setupBumpLvel->value = susp->damper.bump.v1;
		setupBumpLvel->changed = FALSE;
		damperchanged = TRUE;
	}
	
	if (setupRebLvel->changed) {
		susp->damper.rebound.v1 = MIN(setupRebLvel->max, MAX(setupRebLvel->min, setupRebLvel->desired_value));
		setupRebLvel->value = susp->damper.rebound.v1;
		setupRebLvel->changed = FALSE;
		damperchanged = TRUE;
	}
	
	if (damperchanged) {
		susp->damper.bump.b1 = 0.0f;
		susp->damper.rebound.b1 = 0.0f;
		
		initDamper(susp);
	}
	
	if (index<4) {
		if (setupCourse->changed) {
			susp->spring.xMax = MIN(setupCourse->max, MAX(setupCourse->min, setupCourse->desired_value));
			setupCourse->value = susp->spring.xMax;
			setupCourse->changed = FALSE;
		}
		if (setupPacker->changed) {
			susp->spring.packers = MIN(setupPacker->max, MAX(setupPacker->min, setupPacker->desired_value));
			setupPacker->value = susp->spring.packers;
			setupPacker->changed = FALSE;
		}
	}
}
