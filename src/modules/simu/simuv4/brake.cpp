/***************************************************************************

    file                 : brake.cpp
    created              : Sun Mar 19 00:05:26 CET 2000
    copyright            : (C) 2000 by Eric Espie
    email                : torcs@free.fr
    version              : $Id: brake.cpp 3948 2011-10-08 07:27:25Z wdbee $

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

void 
SimBrakeConfig(void *hdle, const char *section, tBrake *brake)
{
    tdble diam, area, mu;
    
    diam     = GfParmGetNum(hdle, section, PRM_BRKDIAM, (char*)NULL, 0.2f);
    area     = GfParmGetNum(hdle, section, PRM_BRKAREA, (char*)NULL, 0.002f);
    mu       = GfParmGetNum(hdle, section, PRM_MU, (char*)NULL, 0.30f);

	// Option TCL ...
	//if (car->features & FEAT_TCLINSIMU)
	{
	    brake->TCL = 0.0f;
	}
	// ... Option TCL
	// Option ABS ...
	//if (car->features & FEAT_ABSINSIMU)
	{
		brake->ABS = 1.0f;
		brake->EnableABS
		     = GfParmGetNum(hdle, section, PRM_ABSINSIMU, (char*)NULL, 0.0f) > 0;
/*
		if (brake->EnableABS)
			fprintf(stderr,"ABS: Enabled\n");
		else
			fprintf(stderr,"ABS: Disabled\n");
*/
	}
	// ... Option ABS

	brake->coeff = (tdble) (diam * 0.5 * area * mu);

    brake->I = GfParmGetNum(hdle, section, PRM_INERTIA, (char*)NULL, 0.13f);
    brake->radius = diam/2.0f;
}

void 
SimBrakeUpdate(tCar *car, tWheel *wheel, tBrake *brake)
{
    brake->Tq = brake->coeff * brake->pressure;

	// Option ABS ...
	if (car->features & FEAT_ABSINSIMU)
	{
		if (brake->EnableABS)
			brake->Tq *= brake->ABS;
	}
	// ... Option ABS

	// Option TCL ...
	if (car->features & FEAT_TCLINSIMU)
	{
		// Brake most spinning wheel
		tdble TCL_BrakeScale = 125.0f;	// Make it be a parameter later
		brake->Tq += (tdble) MAX(0.0,MIN(5000.0,TCL_BrakeScale * brake->TCL)); // Sanity check
		brake->TCL = 0.0; // Reset for next timestep
	}
	// ... Option TCL

    brake->temp -= (tdble) (fabs(car->DynGC.vel.x) * 0.0001 + 0.0002);
    if (brake->temp < 0 ) brake->temp = 0;
    brake->temp += (tdble) (brake->pressure * brake->radius * fabs(wheel->spinVel) * 0.00000000005);
    if (brake->temp > 1.0) brake->temp = 1.0;
}

void 
SimBrakeSystemConfig(tCar *car)
{
    void *hdle = car->params;
    tCarSetupItem *setupBrkRep = &(car->carElt->setup.brakeRepartition);
    tCarSetupItem *setupBrkPress = &(car->carElt->setup.brakePressure);
    
    setupBrkRep->desired_value = setupBrkRep->min = setupBrkRep->max = 0.5;
    GfParmGetNumWithLimits(hdle, SECT_BRKSYST, PRM_BRKREP, (char*)NULL, &(setupBrkRep->desired_value), &(setupBrkRep->min), &(setupBrkRep->max));
    setupBrkRep->changed = TRUE;
    setupBrkRep->stepsize = 0.005f;
    
    setupBrkPress->desired_value = setupBrkPress->min = setupBrkPress->max = 1000000;
    GfParmGetNumWithLimits(hdle, SECT_BRKSYST, PRM_BRKPRESS, (char*)NULL, &(setupBrkPress->desired_value), &(setupBrkPress->min), &(setupBrkPress->max));
    setupBrkPress->changed = TRUE;
    setupBrkPress->stepsize = 1000;
    
    car->brkSyst.ebrake_pressure = GfParmGetNum(hdle, SECT_BRKSYST, PRM_EBRKPRESS, (char*)NULL, 0.0);
}

void 
SimBrakeSystemReConfig(tCar *car)
{/* called by SimCarReConfig() in car.cpp */
    tCarSetupItem *setupBrkRep = &(car->carElt->setup.brakeRepartition);
    tCarSetupItem *setupBrkPress = &(car->carElt->setup.brakePressure);
    
    if (setupBrkRep->changed) {
        car->brkSyst.rep = MIN(setupBrkRep->max, MAX(setupBrkRep->min, setupBrkRep->desired_value));
	setupBrkRep->value = car->brkSyst.rep;
	setupBrkRep->changed = FALSE;
    }
    
    if (setupBrkPress->changed) {
        car->brkSyst.coeff = MIN(setupBrkPress->max, MAX(setupBrkPress->min, setupBrkPress->desired_value));
	setupBrkPress->value = car->brkSyst.coeff;
	setupBrkPress->changed = FALSE;
    }
}

void 
SimBrakeSystemUpdate(tCar *car)
{
    tBrakeSyst	*brkSyst = &(car->brkSyst);

	// Option ESP ...
	if (car->features & FEAT_ESPINSIMU)
	{
		tCarElt	*carElt = car->carElt;
		tdble driftAngle = atan2(carElt->_speed_Y, carElt->_speed_X) - carElt->_yaw;
		FLOAT_NORM_PI_PI(driftAngle);                
		tdble absDriftAngle = fabs(driftAngle);
		//fprintf(stderr,"driftAngle: %.2f deg\n",driftAngle * 180/PI);

		// Make it be parameters later
		tdble driftAngleLimit = (tdble) (7.5 * PI / 180);       // 7.5 deg activation level
		tdble brakeSide = 0.0025f * driftAngle/driftAngleLimit; // Car side brake command
		tdble brakeBalance = 0.005f;                            // Front/Rear brake command 

		if (absDriftAngle > driftAngleLimit)
		{
			car->ctrl->brakeFrontRightCmd -= brakeSide;
			car->ctrl->brakeFrontLeftCmd += brakeSide;
			car->ctrl->brakeRearRightCmd -= brakeBalance + brakeSide;
			car->ctrl->brakeRearLeftCmd -= brakeBalance - brakeSide;
		}

		if (car->ctrl->singleWheelBrakeMode == 1)
		{
			// Sanity check needed
			car->ctrl->brakeFrontRightCmd = (tdble) MIN(1.0,MAX(0.0,car->ctrl->brakeFrontRightCmd));
			car->ctrl->brakeFrontLeftCmd = (tdble) MIN(1.0,MAX(0.0,car->ctrl->brakeFrontLeftCmd));
			car->ctrl->brakeRearRightCmd = (tdble) MIN(1.0,MAX(0.0,car->ctrl->brakeRearRightCmd));
			car->ctrl->brakeRearLeftCmd = (tdble) MIN(1.0,MAX(0.0,car->ctrl->brakeRearRightCmd));

			car->wheel[FRNT_RGT].brake.pressure = brkSyst->coeff * car->ctrl->brakeFrontRightCmd; 
			car->wheel[FRNT_LFT].brake.pressure = brkSyst->coeff * car->ctrl->brakeFrontLeftCmd;
			car->wheel[REAR_RGT].brake.pressure = brkSyst->coeff * car->ctrl->brakeRearRightCmd;
			car->wheel[REAR_LFT].brake.pressure = brkSyst->coeff * car->ctrl->brakeRearLeftCmd;
			//fprintf(stderr,"FR: %.2f FL: %.2f / RR: %.2f RL: %.2f\n",car->ctrl->brakeFrontRightCmd,car->ctrl->brakeFrontLeftCmd,car->ctrl->brakeRearRightCmd,car->ctrl->brakeRearLeftCmd);
		}
		else
		{
			tdble	ctrl = car->ctrl->brakeCmd;

			if (absDriftAngle > driftAngleLimit)
			{
				// Sanity check needed
				car->wheel[FRNT_RGT].brake.pressure = (tdble) MIN(1.0,MAX(0.0,ctrl - brakeSide));
				car->wheel[FRNT_LFT].brake.pressure = (tdble) MIN(1.0,MAX(0.0,ctrl + brakeSide));
				car->wheel[REAR_RGT].brake.pressure = (tdble) MIN(1.0,MAX(0.0,ctrl - brakeSide - brakeBalance));
				car->wheel[REAR_LFT].brake.pressure = (tdble) MIN(1.0,MAX(0.0,ctrl + brakeSide - brakeBalance));

				car->wheel[FRNT_RGT].brake.pressure *= brkSyst->coeff * brkSyst->rep;
				car->wheel[FRNT_LFT].brake.pressure *= brkSyst->coeff * brkSyst->rep;
				car->wheel[REAR_RGT].brake.pressure *= brkSyst->coeff * (1 - brkSyst->rep);
				car->wheel[REAR_LFT].brake.pressure *= brkSyst->coeff * (1 - brkSyst->rep);
			}
			else
			{
				ctrl *= brkSyst->coeff;
				car->wheel[FRNT_RGT].brake.pressure = car->wheel[FRNT_LFT].brake.pressure = ctrl * brkSyst->rep;
				car->wheel[REAR_RGT].brake.pressure = car->wheel[REAR_LFT].brake.pressure = ctrl * (1 - brkSyst->rep);
			}
		}
	} // ... Option ESP
	else if (car->ctrl->singleWheelBrakeMode == 1) 
	{
/*
		car->wheel[FRNT_RGT].brake.pressure = brkSyst->coeff * MIN(car->ctrl->brakeFrontRightCmd, brkSyst->rep); 
		car->wheel[FRNT_LFT].brake.pressure = brkSyst->coeff * MIN(car->ctrl->brakeFrontLeftCmd, brkSyst->rep);
		car->wheel[REAR_RGT].brake.pressure = brkSyst->coeff * MIN(car->ctrl->brakeRearRightCmd, (1-brkSyst->rep));
		car->wheel[REAR_LFT].brake.pressure = brkSyst->coeff * MIN(car->ctrl->brakeRearLeftCmd, (1-brkSyst->rep));
*/
		car->wheel[FRNT_RGT].brake.pressure = brkSyst->coeff * car->ctrl->brakeFrontRightCmd; 
		car->wheel[FRNT_LFT].brake.pressure = brkSyst->coeff * car->ctrl->brakeFrontLeftCmd;
		car->wheel[REAR_RGT].brake.pressure = brkSyst->coeff * car->ctrl->brakeRearRightCmd;
		car->wheel[REAR_LFT].brake.pressure = brkSyst->coeff * car->ctrl->brakeRearLeftCmd;
	}
	else
	{
	    tdble	ctrl = car->ctrl->brakeCmd;
		ctrl *= brkSyst->coeff;
		car->wheel[FRNT_RGT].brake.pressure = car->wheel[FRNT_LFT].brake.pressure = ctrl * brkSyst->rep;
		car->wheel[REAR_RGT].brake.pressure = car->wheel[REAR_LFT].brake.pressure = ctrl * (1 - brkSyst->rep);
	}

    if ( (car->ctrl->ebrakeCmd > 0) && (car->wheel[REAR_RGT].brake.pressure < brkSyst->ebrake_pressure) ) {
        car->wheel[REAR_RGT].brake.pressure = car->wheel[REAR_LFT].brake.pressure = brkSyst->ebrake_pressure;
    }
}
