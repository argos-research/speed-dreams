/***************************************************************************

    file                 : transmission.cpp
    created              : Sun Mar 19 00:07:19 CET 2000
    copyright            : (C) 2000 by Eric Espie
    email                : torcs@free.fr
    version              : $Id: transmission.cpp 4985 2012-10-07 16:15:40Z pouillot $

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
SimTransmissionConfig(tCar *car)
{
    void		*hdle = car->params;
    tCarElt		*carElt = car->carElt;
    //tdble		clutchI; // Never used
    tTransmission	*trans = &(car->transmission);
    tClutch		*clutch = &(trans->clutch);
    tDifferential	*differential;
    const char		*transType;
    int			i, j;
    tdble		gRatio = 0; // Avoid compiler warning: usage of possibly uninitialized variable
	tdble		gEff = 0;
    //tdble       fEff; // Never used
    char		path[256];
    tCarSetupItem	*setupGear;

    //clutchI		= GfParmGetNum(hdle, SECT_CLUTCH, PRM_INERTIA, (char*)NULL, 0.12f);
    transType		= GfParmGetStr(hdle, SECT_DRIVETRAIN, PRM_TYPE, VAL_TRANS_RWD);
    trans->gearbox.shiftTime = clutch->releaseTime = GfParmGetNum(hdle, SECT_GEARBOX, PRM_SHIFTTIME, (char*)NULL, 0.2f);

    /* Link between the differentials */
    for (j = 0; j < 2; j++) {
		trans->differential[TRANS_FRONT_DIFF].inAxis[j]  = &(car->wheel[j].feedBack);
		trans->differential[TRANS_FRONT_DIFF].outAxis[j] = &(car->wheel[j].in);
    }
    for (j = 0; j < 2; j++) {
		trans->differential[TRANS_REAR_DIFF].inAxis[j]  = &(car->wheel[2+j].feedBack);
		trans->differential[TRANS_REAR_DIFF].outAxis[j] = &(car->wheel[2+j].in);
    }
    trans->differential[TRANS_CENTRAL_DIFF].inAxis[0]  = &(trans->differential[TRANS_FRONT_DIFF].feedBack);
    trans->differential[TRANS_CENTRAL_DIFF].outAxis[0] = &(trans->differential[TRANS_FRONT_DIFF].in);

    trans->differential[TRANS_CENTRAL_DIFF].inAxis[1]  = &(trans->differential[TRANS_REAR_DIFF].feedBack);
    trans->differential[TRANS_CENTRAL_DIFF].outAxis[1] = &(trans->differential[TRANS_REAR_DIFF].in);

    if (strcmp(VAL_TRANS_RWD, transType) == 0) {
		SimDifferentialConfig(car, TRANS_REAR_DIFF);
		trans->type = TRANS_RWD;
		//fEff   = trans->differential[TRANS_REAR_DIFF].efficiency;
    } else if (strcmp(VAL_TRANS_FWD, transType) == 0) {
		SimDifferentialConfig(car, TRANS_FRONT_DIFF);
		trans->type = TRANS_FWD;
		//fEff   = trans->differential[TRANS_FRONT_DIFF].efficiency;
    } else if (strcmp(VAL_TRANS_4WD, transType) == 0) {
		SimDifferentialConfig(car, TRANS_FRONT_DIFF);
		SimDifferentialConfig(car, TRANS_REAR_DIFF);
		SimDifferentialConfig(car, TRANS_CENTRAL_DIFF);
		trans->type = TRANS_4WD;
		//fEff   = trans->differential[TRANS_FRONT_DIFF].efficiency * trans->differential[TRANS_CENTRAL_DIFF].efficiency * trans->differential[TRANS_REAR_DIFF].efficiency;
    }

    trans->gearbox.gearMax = 0;
    //printf ("engine I %f\n", car->engine.I);
    for (i = MAX_GEARS - 1; i >= 0; i--) {
		if (i<2)
			sprintf(path, "%s/%s/%s", SECT_GEARBOX, ARR_GEARS, i==0 ? "r" : "n");
		else
			sprintf(path, "%s/%s/%d", SECT_GEARBOX, ARR_GEARS, i-1);
		setupGear = &(car->carElt->setup.gearRatio[i]);
		setupGear->desired_value = setupGear->min = setupGear->max = 0.0f;
		GfParmGetNumWithLimits(hdle, path, PRM_RATIO, (char*)NULL, &(setupGear->desired_value), &(setupGear->min), &(setupGear->max));
		setupGear->changed = TRUE;
		setupGear->stepsize = 0.01f;
		gRatio = setupGear->desired_value;
		if ((trans->gearbox.gearMax == 0) && (gRatio != 0.0f)) {
			trans->gearbox.gearMax = i - 1;
		}
		if (gRatio == 0.0f) {
			carElt->priv.gearRatio[i] = trans->overallRatio[i] = 0;
			trans->freeI[i] = trans->driveI[i] = 0;
			trans->gearEff[i] = 1.0f;
			continue;
		}
		gEff = GfParmGetNum(hdle, path, PRM_EFFICIENCY, (char*)NULL, 1.0f);
		if (gEff > 1.0f) gEff = 1.0f;
		if (gEff < 0.0f) gEff = 0.0f;
		trans->gearI[i] = GfParmGetNum(hdle, path, PRM_INERTIA, (char*)NULL, 0.0f);
		//printf ("drivetrain %d = %f %f\n", i, trans->driveI[i], gearI);
		trans->gearEff[i] = gEff;
    }
    if (gRatio == 0) {
		/* no reverse */
		trans->gearbox.gearMin = 0;
		carElt->priv.gearOffset = 0;
    } else {
		trans->gearbox.gearMin = -1;
		carElt->priv.gearOffset = 1;
    }
    carElt->priv.gearNb = trans->gearbox.gearMax + 1;

    /* initial state */
    clutch->state = CLUTCH_RELEASING;
    clutch->timeToRelease = 0;

    trans->gearbox.gear = 0; /* neutral */
    trans->gearbox.gearNext = 0;
    trans->gearbox.timeToEngage = 0.0f;
    trans->curI = trans->freeI[1];
    switch(trans->type) {
    case TRANS_RWD:
		differential = &(trans->differential[TRANS_REAR_DIFF]);
		differential->outAxis[0]->I = trans->curI / 2.0f + differential->inAxis[0]->I;
		differential->outAxis[1]->I = trans->curI / 2.0f + differential->inAxis[1]->I;
		differential->outAxis[0]->Tq = 0;
		differential->outAxis[1]->Tq = 0;
		break;
    case TRANS_FWD:
		differential = &(trans->differential[TRANS_FRONT_DIFF]);
		differential->outAxis[0]->I = trans->curI / 2.0f + differential->inAxis[0]->I;
		differential->outAxis[1]->I = trans->curI / 2.0f + differential->inAxis[1]->I;
		differential->outAxis[0]->Tq = 0;
		differential->outAxis[1]->Tq = 0;
		break;
    case TRANS_4WD:
		differential = &(trans->differential[TRANS_FRONT_DIFF]);
		differential->outAxis[0]->I = trans->curI / 4.0f + differential->inAxis[0]->I;
		differential->outAxis[1]->I = trans->curI / 4.0f + differential->inAxis[1]->I;
		differential->outAxis[0]->Tq = 0;
		differential->outAxis[1]->Tq = 0;
		differential = &(trans->differential[TRANS_REAR_DIFF]);
		differential->outAxis[0]->I = trans->curI / 4.0f + differential->inAxis[0]->I;
		differential->outAxis[1]->I = trans->curI / 4.0f + differential->inAxis[1]->I;
		differential->outAxis[0]->Tq = 0;
		differential->outAxis[1]->Tq = 0;
		differential = &(trans->differential[TRANS_CENTRAL_DIFF]);
		differential->outAxis[0]->I = trans->curI / 2.0f + differential->inAxis[0]->I;
		differential->outAxis[1]->I = trans->curI / 2.0f + differential->inAxis[1]->I;
		differential->outAxis[0]->Tq = 0;
		differential->outAxis[1]->Tq = 0;
		break;
    }

}

void
SimTransmissionReConfig(tCar *car)
{/* called by SimCarReConfig() in car.cpp */
    tCarElt *carElt = car->carElt;
    tTransmission *trans = &(car->transmission);
    int  i;
    tdble fRatio = 0.0;
    tdble gRatio = 0.0;
    tCarSetupItem *setupGear;
    
    if (trans->type == TRANS_RWD) {
	SimDifferentialReConfig(car, TRANS_REAR_DIFF);
	fRatio = trans->differential[TRANS_REAR_DIFF].ratio;
    } else if (trans->type == TRANS_FWD) {
	SimDifferentialReConfig(car, TRANS_FRONT_DIFF);
	fRatio = trans->differential[TRANS_FRONT_DIFF].ratio;
    } else if (trans->type == TRANS_4WD) {
	SimDifferentialReConfig(car, TRANS_FRONT_DIFF);
	SimDifferentialReConfig(car, TRANS_REAR_DIFF);
	SimDifferentialReConfig(car, TRANS_CENTRAL_DIFF);
	fRatio = trans->differential[TRANS_CENTRAL_DIFF].ratio;
    }
    
    for (i = MAX_GEARS - 1; i >= 0; i--) {
        setupGear = &(car->carElt->setup.gearRatio[i]);
	if (setupGear->changed) {
	    gRatio = MIN(setupGear->max, MAX(setupGear->min, setupGear->desired_value));
	    setupGear->value = gRatio;
	    setupGear->changed = FALSE;
	}else {gRatio = setupGear->value;}
	if (gRatio == 0.0f) {
	    carElt->priv.gearRatio[i] = trans->overallRatio[i] = 0;
	    trans->freeI[i] = trans->driveI[i] = 0;
	    continue;
	}
	carElt->priv.gearRatio[i] = trans->overallRatio[i] = gRatio * fRatio;
	trans->driveI[i] = (car->engine.I + trans->gearI[i]) * (gRatio * gRatio * fRatio * fRatio);
	trans->freeI[i] = trans->gearI[i] * (gRatio * gRatio * fRatio * fRatio);
    }
}
void
SimGearboxUpdate(tCar *car)
{
    /* manages gear change */
    tTransmission	*trans = &(car->transmission);
    tClutch		*clutch = &(trans->clutch);
    tGearbox		*gearbox = &(trans->gearbox);
    tDifferential	*differential = NULL;

    switch(trans->type) {
    case TRANS_RWD:
		differential = &(trans->differential[TRANS_REAR_DIFF]);
		break;
    case TRANS_FWD:
		differential = &(trans->differential[TRANS_FRONT_DIFF]);
		break;
    case TRANS_4WD:
		differential = &(trans->differential[TRANS_CENTRAL_DIFF]);
		break;
    }

    trans->curI = trans->driveI[gearbox->gear + 1] * clutch->transferValue + trans->freeI[gearbox->gear +  1] * (1.0f - clutch->transferValue);
    
    if (car->features & FEAT_REALGEARCHANGE) {/* simuv4 new gear change code */
	if ( (car->ctrl->gear != gearbox->gear) && (car->ctrl->gear <= gearbox->gearMax) && (car->ctrl->gear >= gearbox->gearMin) ) {
	    /* initiate a shift, go to neutral */
	    gearbox->gearNext = car->ctrl->gear;
	    if (gearbox->timeToEngage <= 0.0f) {
		if (gearbox->gearNext == 0) {gearbox->timeToEngage = 0.0f;} /* disengaging gears happens immediately */
		else {gearbox->timeToEngage = gearbox->shiftTime * 0.67f;}
		gearbox->gear = 0;
		trans->curOverallRatio = trans->overallRatio[1];
		trans->curI = trans->driveI[1] * clutch->transferValue + trans->freeI[1] * (1.0f - clutch->transferValue);
	    }
	}
	
	if (gearbox->timeToEngage > 0.0f) {
	    gearbox->timeToEngage -= SimDeltaTime;
	    if (gearbox->timeToEngage <= 0.0f) {
		/* engage new gear */
		gearbox->gear = gearbox->gearNext;
		gearbox->gearNext = 0;
		trans->curOverallRatio = trans->overallRatio[gearbox->gear+1];
		trans->curI = trans->driveI[gearbox->gear + 1] * clutch->transferValue + trans->freeI[gearbox->gear +  1] * (1.0f - clutch->transferValue);
	    }
	}
    } else {/* old gear change code */
    	if (clutch->state == CLUTCH_RELEASING && gearbox->gear != car->ctrl->gear) { 
                /* Fast change during clutch release, re-releasing it */ 
                clutch->state = CLUTCH_RELEASED; 
    	}
    	if (clutch->state == CLUTCH_RELEASING) {
		clutch->timeToRelease -= SimDeltaTime;
		if (clutch->timeToRelease <= 0.0f) {
			clutch->state = CLUTCH_RELEASED;
		} else  {
            // If user does not engage clutch, we do it automatically.
			if (clutch->transferValue > 0.99f) {
				clutch->transferValue = 0.0f;
                trans->curI = trans->freeI[gearbox->gear + 1];

                // NOTE: Shouldn't usage of accelerator when shifting be let
                // to the user to decide? Especially when shifting down
                // in order to accelerate more, this could be annoying.
				if (car->ctrl->accelCmd > 0.1f) {
					car->ctrl->accelCmd = 0.1f;
				}
			}
		}
    	} else if ((car->ctrl->gear > gearbox->gear)) {
		if (car->ctrl->gear <= gearbox->gearMax) {
			gearbox->gear = car->ctrl->gear;
			clutch->state = CLUTCH_RELEASING;
			if (gearbox->gear != 0) {
				clutch->timeToRelease = clutch->releaseTime;
			} else {
				clutch->timeToRelease = 0;
			}
			trans->curOverallRatio = trans->overallRatio[gearbox->gear+1];
			trans->curI = trans->freeI[gearbox->gear+1];
		}
    	} else if ((car->ctrl->gear < gearbox->gear)) {
		if (car->ctrl->gear >= gearbox->gearMin) {
			gearbox->gear = car->ctrl->gear;
			clutch->state = CLUTCH_RELEASING;
			if (gearbox->gear != 0) {
				clutch->timeToRelease = clutch->releaseTime;
			} else {
				clutch->timeToRelease = 0;
			}
			trans->curOverallRatio = trans->overallRatio[gearbox->gear+1];
			trans->curI = trans->freeI[gearbox->gear+1];
		}
    	}
    }


	differential->in.I = trans->curI + differential->feedBack.I;
	differential->outAxis[0]->I = trans->curI / 2.0f + differential->inAxis[0]->I;
	differential->outAxis[1]->I = trans->curI / 2.0f + differential->inAxis[1]->I;
	if (trans->type == TRANS_4WD) {
		differential = &(trans->differential[TRANS_FRONT_DIFF]);
		differential->outAxis[0]->I = trans->curI / 4.0f + differential->inAxis[0]->I;
		differential->outAxis[1]->I = trans->curI / 4.0f + differential->inAxis[1]->I;
		differential = &(trans->differential[TRANS_REAR_DIFF]);
		differential->outAxis[0]->I = trans->curI / 4.0f + differential->inAxis[0]->I;
		differential->outAxis[1]->I = trans->curI / 4.0f + differential->inAxis[1]->I;
	}

}

void
SimTransmissionUpdate(tCar *car)
{
    tTransmission	*trans = &(car->transmission);
    tClutch		*clutch = &(trans->clutch);
    tGearbox		*gearbox = &(trans->gearbox);
    tDifferential	*differential, *differential0, *differential1;
    tdble		transfer = MIN(clutch->transferValue * 3.0f, 1.0f);

    switch(trans->type) {
    case TRANS_RWD:
		differential = &(trans->differential[TRANS_REAR_DIFF]);
		differential->in.Tq = (car->engine.Tq_response + car->engine.Tq) * trans->curOverallRatio * transfer * trans->gearEff[gearbox->gear+1];
		SimDifferentialUpdate(car, differential, 1);
		SimUpdateFreeWheels(car, 0);
		/* 	printf("s0 %f - s1 %f (%f)	inTq %f -- Tq0 %f - Tq1 %f (%f)\n", */
		/* 	       differential->outAxis[0]->spinVel, differential->outAxis[1]->spinVel, differential->outAxis[0]->spinVel - differential->outAxis[1]->spinVel, */
		/* 	       differential->in.Tq, */
		/* 	       differential->outAxis[0]->Tq, differential->outAxis[1]->Tq, differential->outAxis[0]->Tq - differential->outAxis[1]->Tq); */
		break;
    case TRANS_FWD:
		differential = &(trans->differential[TRANS_FRONT_DIFF]);
		differential->in.Tq = (car->engine.Tq_response + car->engine.Tq) * trans->curOverallRatio * transfer * trans->gearEff[gearbox->gear+1];
		SimDifferentialUpdate(car, differential, 1);
		SimUpdateFreeWheels(car, 1);
		/* 	printf("s0 %f - s1 %f (%f)	inTq %f -- Tq0 %f - Tq1 %f (%f)\n", */
		/* 	       differential->outAxis[0]->spinVel, differential->outAxis[1]->spinVel, differential->outAxis[0]->spinVel - differential->outAxis[1]->spinVel, */
		/* 	       differential->in.Tq, */
		/* 	       differential->outAxis[0]->Tq, differential->outAxis[1]->Tq, differential->outAxis[0]->Tq - differential->outAxis[1]->Tq); */
		break;
    case TRANS_4WD:
		differential = &(trans->differential[TRANS_CENTRAL_DIFF]);
		differential0 = &(trans->differential[TRANS_FRONT_DIFF]);
		differential1 = &(trans->differential[TRANS_REAR_DIFF]);

		differential->in.Tq = (car->engine.Tq_response + car->engine.Tq) * trans->curOverallRatio * transfer * trans->gearEff[gearbox->gear+1];
		differential->inAxis[0]->spinVel = (differential0->inAxis[0]->spinVel + differential0->inAxis[1]->spinVel) / 2.0f;
		differential->inAxis[1]->spinVel = (differential1->inAxis[0]->spinVel + differential1->inAxis[1]->spinVel) / 2.0f;
//		differential->inAxis[0]->Tq = (differential0->inAxis[0]->Tq + differential0->inAxis[1]->Tq) / differential->ratio;
//		differential->inAxis[1]->Tq = (differential1->inAxis[0]->Tq + differential1->inAxis[1]->Tq) / differential->ratio;
		differential->inAxis[0]->Tq = 0;
		differential->inAxis[1]->Tq = 0;
//		differential->inAxis[0]->brkTq = (differential0->inAxis[0]->brkTq + differential0->inAxis[1]->brkTq) / differential->ratio;
//		differential->inAxis[1]->brkTq = (differential1->inAxis[0]->brkTq + differential1->inAxis[1]->brkTq) / differential->ratio;
		differential->inAxis[0]->brkTq = 0;
		differential->inAxis[1]->brkTq = 0;

		SimDifferentialUpdate(car, differential, 1);
		/* 	printf("\nCentral : s0 %f - s1 %f (%f)	inTq %f -- Tq0 %f - Tq1 %f (%f)\n", */
		/* 	       differential->outAxis[0]->spinVel, differential->outAxis[1]->spinVel, differential->outAxis[0]->spinVel - differential->outAxis[1]->spinVel, */
		/* 	       differential->in.Tq, */
		/* 	       differential->outAxis[0]->Tq, differential->outAxis[1]->Tq, differential->outAxis[0]->Tq - differential->outAxis[1]->Tq); */

		differential = differential0;
		SimDifferentialUpdate(car, differential, 0);
		/* 	printf("Front   : s0 %f - s1 %f (%f)	inTq %f -- Tq0 %f - Tq1 %f (%f)\n", */
		/* 	       differential->outAxis[0]->spinVel, differential->outAxis[1]->spinVel, differential->outAxis[0]->spinVel - differential->outAxis[1]->spinVel, */
		/* 	       differential->in.Tq, */
		/* 	       differential->outAxis[0]->Tq, differential->outAxis[1]->Tq, differential->outAxis[0]->Tq - differential->outAxis[1]->Tq); */

		differential = differential1;
		SimDifferentialUpdate(car, differential, 0);
		/* 	printf("Rear    : s0 %f - s1 %f (%f)	inTq %f -- Tq0 %f - Tq1 %f (%f)\n", */
		/* 	       differential->outAxis[0]->spinVel, differential->outAxis[1]->spinVel, differential->outAxis[0]->spinVel - differential->outAxis[1]->spinVel, */
		/* 	       differential->in.Tq, */
		/* 	       differential->outAxis[0]->Tq, differential->outAxis[1]->Tq, differential->outAxis[0]->Tq - differential->outAxis[1]->Tq); */
		break;
    }
}
