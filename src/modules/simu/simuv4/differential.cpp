/***************************************************************************

    file                 : differential.cpp
    created              : Sun Mar 19 00:06:33 CET 2000
    copyright            : (C) 2000 by Eric Espie
    email                : torcs@free.fr
    version              : $Id: differential.cpp 3945 2011-10-07 13:38:15Z wdbee $

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
SimDifferentialConfig(tCar *car, int index)
{
    void *hdle = car->params;
    const char *type;
    const char *section;

    switch (index) {
        case TRANS_FRONT_DIFF:
            section = SECT_FRNTDIFFERENTIAL;
            break;
        case TRANS_REAR_DIFF:
            section = SECT_REARDIFFERENTIAL;
            break;
        case TRANS_CENTRAL_DIFF:
            section = SECT_CENTRALDIFFERENTIAL;
            break;
        default: 
            GfLogWarning("No differential indexed %d exists, returning without configuration.", index);
            return;
    }
    tDifferential *differential = &(car->transmission.differential[index]);
    tCarSetupItem *setupDRatio = &(car->carElt->setup.differentialRatio[index]);
    tCarSetupItem *setupDMinTB = &(car->carElt->setup.differentialMinTqBias[index]);
    tCarSetupItem *setupDMaxTB = &(car->carElt->setup.differentialMaxTqBias[index]);
    tCarSetupItem *setupDVisc = &(car->carElt->setup.differentialViscosity[index]);
    tCarSetupItem *setupDLT = &(car->carElt->setup.differentialLockingTq[index]);
    tCarSetupItem *setupDMaxSB = &(car->carElt->setup.differentialMaxSlipBias[index]);
    tCarSetupItem *setupDCMaxSB = &(car->carElt->setup.differentialCoastMaxSlipBias[index]);
    
    differential->I     = GfParmGetNum(hdle, section, PRM_INERTIA, (char*)NULL, 0.1f);
    differential->efficiency    = GfParmGetNum(hdle, section, PRM_EFFICIENCY, (char*)NULL, 1.0f);
    //differential->bias is unused as of 2015.11.15.
    differential->bias      = GfParmGetNum(hdle, section, PRM_BIAS, (char*)NULL, 0.1f);
    
    setupDRatio->desired_value = setupDRatio->min = setupDRatio->max = 1.0f;
    GfParmGetNumWithLimits(hdle, section, PRM_RATIO, (char*)NULL, &(setupDRatio->desired_value), &(setupDRatio->min), &(setupDRatio->max));
    setupDRatio->changed = TRUE;
    setupDRatio->stepsize = 0.1f;
    
    setupDMinTB->desired_value = setupDMinTB->min = setupDMinTB->max = 0.05f;
    GfParmGetNumWithLimits(hdle, section, PRM_MIN_TQ_BIAS, (char*)NULL, &(setupDMinTB->desired_value), &(setupDMinTB->min), &(setupDMinTB->max));
    setupDMinTB->changed = TRUE;
    setupDMinTB->stepsize = 0.01f;
    
    setupDMaxTB->desired_value = setupDMaxTB->min = setupDMaxTB->max = 0.80f;
    GfParmGetNumWithLimits(hdle, section, PRM_MAX_TQ_BIAS, (char*)NULL, &(setupDMaxTB->desired_value), &(setupDMaxTB->min), &(setupDMaxTB->max));
    setupDMaxTB->changed = TRUE;
    setupDMaxTB->stepsize = 0.01f;
    
    setupDVisc->desired_value = setupDVisc->min = setupDVisc->max = 2.0f;
    GfParmGetNumWithLimits(hdle, section, PRM_VISCOSITY_FACTOR, (char*)NULL, &(setupDVisc->desired_value), &(setupDVisc->min), &(setupDVisc->max));
    setupDVisc->changed = TRUE;
    setupDVisc->stepsize = 0.1f;
    
    setupDLT->desired_value = setupDLT->min = setupDLT->max = 300.0f;
    GfParmGetNumWithLimits(hdle, section, PRM_LOCKING_TQ, (char*)NULL, &(setupDLT->desired_value), &(setupDLT->min), &(setupDLT->max));
    setupDLT->changed = TRUE;
    setupDLT->stepsize = 10.0f;
    
    setupDMaxSB->desired_value = setupDMaxSB->min = setupDMaxSB->max = 0.75f;
    GfParmGetNumWithLimits(hdle, section, PRM_MAX_SLIP_BIAS, (char*)NULL, &(setupDMaxSB->desired_value), &(setupDMaxSB->min), &(setupDMaxSB->max));
    setupDMaxSB->changed = TRUE;
    setupDMaxSB->stepsize = 0.01f;
    
    setupDCMaxSB->desired_value = setupDCMaxSB->min = setupDCMaxSB->max = setupDMaxSB->desired_value;
    GfParmGetNumWithLimits(hdle, section, PRM_COAST_MAX_SLIP_BIAS, (char*)NULL, &(setupDCMaxSB->desired_value), &(setupDCMaxSB->min), &(setupDCMaxSB->max));
    setupDCMaxSB->changed = TRUE;
    setupDCMaxSB->stepsize = 0.01f;

    type = GfParmGetStr(hdle, section, PRM_TYPE, VAL_DIFF_NONE);
    if (strcmp(type, VAL_DIFF_LIMITED_SLIP) == 0) {
        differential->type = DIFF_LIMITED_SLIP; 
    } else if (strcmp(type, VAL_DIFF_VISCOUS_COUPLER) == 0) {
        differential->type = DIFF_VISCOUS_COUPLER;
    } else if (strcmp(type, VAL_DIFF_SPOOL) == 0) {
        differential->type = DIFF_SPOOL;
    }  else if (strcmp(type, VAL_DIFF_FREE) == 0) {
        differential->type = DIFF_FREE;
    }  else if (strcmp(type, VAL_DIFF_15WAY_LSD) == 0) {
        differential->type = DIFF_15WAY_LSD;
    }  else if (strcmp(type, VAL_DIFF_ELECTRONIC_LSD) == 0) {
        differential->type = DIFF_ELECTRONIC_LSD;
    } else {
        differential->type = DIFF_NONE; 
    }
    car->carElt->setup.differentialType[index] = differential->type;
    //TODO: get allowed differential types from xml and store them

    if (differential->efficiency > 1.0f) {differential->efficiency = 1.0f;}
    if (differential->efficiency < 0.0f) {differential->efficiency = 0.0f;}

    differential->feedBack.I = differential->I * differential->ratio * differential->ratio +
        (differential->inAxis[0]->I + differential->inAxis[1]->I);
}


void 
SimDifferentialReConfig(tCar *car, int index)
{/* called by SimTransmissionReConfig() in transmission.cpp */
    tDifferential *differential = &(car->transmission.differential[index]);
    tCarSetupItem *setupDRatio = &(car->carElt->setup.differentialRatio[index]);
    tCarSetupItem *setupDMinTB = &(car->carElt->setup.differentialMinTqBias[index]);
    tCarSetupItem *setupDMaxTB = &(car->carElt->setup.differentialMaxTqBias[index]);
    tCarSetupItem *setupDVisc = &(car->carElt->setup.differentialViscosity[index]);
    tCarSetupItem *setupDLT = &(car->carElt->setup.differentialLockingTq[index]);
    tCarSetupItem *setupDMaxSB = &(car->carElt->setup.differentialMaxSlipBias[index]);
    tCarSetupItem *setupDCMaxSB = &(car->carElt->setup.differentialCoastMaxSlipBias[index]);
    
    //TODO: check if type is available
    differential->type = car->carElt->setup.differentialType[index];
    
    if (setupDRatio->changed) {
        differential->ratio = MIN(setupDRatio->max, MAX(setupDRatio->min, setupDRatio->desired_value));
        setupDRatio->value = differential->ratio;
        setupDRatio->changed = FALSE;
    }
    
    if (setupDMinTB->changed) {
        differential->dTqMin = MIN(setupDMinTB->max, MAX(setupDMinTB->min, setupDMinTB->desired_value));
        setupDMinTB->value = differential->dTqMin;
        setupDMinTB->changed = FALSE;
    }
    
    if (setupDMaxTB->changed) {
        differential->dTqMax = MIN(setupDMaxTB->max, MAX(setupDMaxTB->min, setupDMaxTB->desired_value));
        setupDMaxTB->value = differential->dTqMax;
        setupDMaxTB->changed = FALSE;
    }
    
    if (setupDVisc->changed) {
        differential->viscosity = MIN(setupDVisc->max, MAX(setupDVisc->min, setupDVisc->desired_value));
        setupDVisc->value = differential->viscosity;
        setupDVisc->changed = FALSE;
        differential->viscomax  = 1 - exp(-differential->viscosity);
    }
    
    if (setupDLT->changed) {
        differential->lockInputTq = MIN(setupDLT->max, MAX(setupDLT->min, setupDLT->desired_value));
        setupDLT->value = differential->lockInputTq;
        setupDLT->changed = FALSE;
    }
    
    if (setupDMaxSB->changed) {
        differential->dSlipMax = MIN(setupDMaxSB->max, MAX(setupDMaxSB->min, setupDMaxSB->desired_value));
        setupDMaxSB->value = differential->dSlipMax;
        setupDMaxSB->changed = FALSE;
    }
    
     if (setupDCMaxSB->changed) {
        differential->dCoastSlipMax = MIN(setupDCMaxSB->max, MAX(setupDCMaxSB->min, setupDCMaxSB->desired_value));
        setupDCMaxSB->changed = FALSE;
    }
    if ( (differential->type != DIFF_15WAY_LSD) && (differential->type != DIFF_ELECTRONIC_LSD) ) {
        differential->dCoastSlipMax = differential->dSlipMax;
    }
    setupDCMaxSB->value = differential->dCoastSlipMax;
}


static void
updateSpool(tCar *car, tDifferential *differential, int first)
{
    tdble   DrTq;
    tdble   ndot;
    tdble   spinVel;
    tdble   BrTq;
    tdble   engineReaction;
    tdble   I;
    tdble   inTq, brkTq;
    
    DrTq = differential->in.Tq * differential->efficiency;

    I = differential->outAxis[0]->I + differential->outAxis[1]->I;
    inTq = differential->inAxis[0]->Tq + differential->inAxis[1]->Tq;
    brkTq = differential->inAxis[0]->brkTq + differential->inAxis[1]->brkTq;

    ndot = SimDeltaTime * (DrTq - inTq) / I;
    spinVel = differential->inAxis[0]->spinVel + ndot;
    
    BrTq = (tdble) (- SIGN(spinVel) * brkTq);
    ndot = SimDeltaTime * BrTq / I;
    
    if (((ndot * spinVel) < 0.0) && (fabs(ndot) > fabs(spinVel))) {
        ndot = -spinVel;
    }
    if ((spinVel == 0.0) && (ndot < 0.0)) ndot = 0;
    
    spinVel += ndot;
    if (first) {
        engineReaction = SimEngineUpdateRpm(car, spinVel);
        if (engineReaction != 0.0) {
            spinVel = engineReaction;
        }
    }
    differential->outAxis[0]->spinVel = differential->outAxis[1]->spinVel = spinVel;

    differential->outAxis[0]->Tq = (differential->outAxis[0]->spinVel - differential->inAxis[0]->spinVel) / SimDeltaTime * differential->outAxis[0]->I;
    differential->outAxis[1]->Tq = (differential->outAxis[1]->spinVel - differential->inAxis[1]->spinVel) / SimDeltaTime * differential->outAxis[1]->I;
}


void 
SimDifferentialUpdate(tCar *car, tDifferential *differential, int first)
{
    tdble   DrTq, DrTq0, DrTq1;
    tdble   ndot0, ndot1;
    tdble   spinVel0, spinVel1;
    tdble   inTq0, inTq1;
    tdble   spdRatio/*, spdRatioMax*/;
    tdble   /*deltaSpd,*/ deltaTq;
    tdble   BrTq;
    tdble   engineReaction;
    tdble   meanv;

    if (differential->type == DIFF_SPOOL) {
        updateSpool(car, differential, first);
        return;
    }

    DrTq = differential->in.Tq * differential->efficiency;

    spinVel0 = differential->inAxis[0]->spinVel;
    spinVel1 = differential->inAxis[1]->spinVel;
    
    inTq0 = differential->inAxis[0]->Tq;
    inTq1 = differential->inAxis[1]->Tq;


    spdRatio = fabs(spinVel0 + spinVel1);
    if (spdRatio != 0) {
        spdRatio = fabs(spinVel0 - spinVel1) / spdRatio;

        switch (differential->type) {
        case DIFF_FREE:
            // I would think that the following is what a FREE
            // differential should look like, with both wheels
            // independent and linked through a spider gear.
            //
            // The reaction from each wheel is transmitted back to the
            // spider gear. If both wheels react equally, then the
            // spider gear does not turn. If one of the wheel is
            // immobile, so that DrTq/2=inTq0 for example, then the
            // reaction does not act against the drivetrain, but since
            // the spider gear can turn freely, it acts on the other wheel.
            // 
            // This system is equivalent to a rotating gear attached
            // in between two parallel surfaces, with DrTq being
            // equivalent to a force acting in the center of the
            // gear. If one surface is fixed, only the other surface
            // moves and all the force is 'transferred' to the moving
            // surface. Or, the way I like to think of it, the
            // immobile surface reacts with an equal and opposite
            // force[1] that cancels DrTq/2 exactly and which is
            // transmitted directly with the rotating gear to the
            // other, free, surface.
            //
            //
            // A lot of explanation for 3 lines of code..  TODO: Check
            // what bias would mean in such a system. Would it be
            // implemented between the spider and the wheels?  Or
            // between the spider and the drivetrain? If the latter
            // then it meanst the spider would always be turning, even
            // under an even load. I think in this case it is safest
            // to ignore it completely because it is frequently used
            // in cars with just FWD or RWD, and very frequently in
            // just the front part of 4WD cars, while the default
            // differential bias setting is 0.1...
            //
            // [1] For an object to remain at rest, all forces acting
            // on it must sum to 0.
            
            {
                float spiderTq = inTq1 - inTq0;
                DrTq0 = DrTq*0.5f + spiderTq;
                DrTq1 = DrTq*0.5f - spiderTq;
            }
            break;

                       
        case DIFF_LIMITED_SLIP:
            // Limited slip differential with:
            // - Gradual frictive locking
            // - Open differential behaviour when not locked
            //
            // The spider gear transfers torque between the two axles
            // When DrTq=lockInputTq, then the locking is at 66% (and
            // almost 100% at double the torque).  When the
            // differential is locked, there is a pressure applied due
            // to the different amount of spin of each wheel.  This
            // pressure creates a torque bias at the input, limited by
            // dSlipMax.
            // So the user should use lockInputTq to regulate how fast
            // the differential locks and dSlipMax to regulate how much
            // more torque should go to the slower moving wheel.
            {
                float spiderTq = inTq1 - inTq0; 
                float propTq = DrTq/differential->lockInputTq;
                float rate = 0.0f;
                if (propTq > 0.0f) {
                    rate = 1.0f - exp(-propTq*propTq);
                }

                float pressure = tanh(rate*(spinVel1-spinVel0));
                float bias = differential->dSlipMax * 0.5f* pressure;
                float open = 1.0f;// - rate;
                DrTq0 = DrTq*(0.5f+bias) + spiderTq*open;
                DrTq1 = DrTq*(0.5f-bias) - spiderTq*open;
            }
            break;

        case DIFF_ELECTRONIC_LSD: ;
        case DIFF_15WAY_LSD:
            //Similar to DIFF_LIMITED_SLIP, 
            //but has different dSlipMax for power (acceleration) 
            //and coast (deceleration), instead working as a free
            //differential in coast direction.
            //Electronic LSD has the same working, but its parameters
            //can be changed during driving.
            {
                float spiderTq = inTq1 - inTq0; 
                float propTq = DrTq/differential->lockInputTq;
                float rate = 0.0f;
                rate = 1.0f - exp(-propTq*propTq);

                float pressure = tanh(rate*(spinVel1-spinVel0));
                float bias = (DrTq >= 0 ? differential->dSlipMax : differential->dCoastSlipMax) * 0.5f* pressure;
                float open = 1.0f;// - rate;
                DrTq0 = DrTq*(0.5f+bias) + spiderTq*open;
                DrTq1 = DrTq*(0.5f-bias) - spiderTq*open;
            }
            break;
        
        case DIFF_VISCOUS_COUPLER:
            if (spinVel0 >= spinVel1) {
                DrTq0 = DrTq * differential->dTqMin;
                DrTq1 = DrTq * (1 - differential->dTqMin);
            } else {
                deltaTq = (tdble) (differential->dTqMin + (1.0 - exp(-fabs(differential->viscosity * spinVel0 - spinVel1))) /
                    differential->viscomax * differential->dTqMax);
                DrTq0 = DrTq * deltaTq;
                DrTq1 = DrTq * (1 - deltaTq);
            }
    
            break;
        default: /* NONE ? */
            DrTq0 = DrTq1 = 0;
            break;
        }
    } else {
        DrTq0 = (tdble) (DrTq / 2.0);
        DrTq1 = (tdble) (DrTq / 2.0);
    }


    ndot0 = SimDeltaTime * (DrTq0 - inTq0) / differential->outAxis[0]->I;
    spinVel0 += ndot0;
    ndot1 = SimDeltaTime * (DrTq1 - inTq1) / differential->outAxis[1]->I;
    spinVel1 += ndot1;

    BrTq = (tdble) (- SIGN(spinVel0) * differential->inAxis[0]->brkTq);
    ndot0 = SimDeltaTime * BrTq / differential->outAxis[0]->I;
    if (((ndot0 * spinVel0) < 0.0) && (fabs(ndot0) > fabs(spinVel0))) {
        ndot0 = -spinVel0;
    }
    if ((spinVel0 == 0.0) && (ndot0 < 0.0)) ndot0 = 0;
    spinVel0 += ndot0;
    
    BrTq = (tdble) (- SIGN(spinVel1) * differential->inAxis[1]->brkTq);
    ndot1 = SimDeltaTime * BrTq / differential->outAxis[1]->I;
    if (((ndot1 * spinVel1) < 0.0) && (fabs(ndot1) > fabs(spinVel1))) {
        ndot1 = -spinVel1;
    }
    if ((spinVel1 == 0.0) && (ndot1 < 0.0)) ndot1 = 0;
    spinVel1 += ndot1;

    if (first) {
        meanv = (tdble) ((spinVel0 + spinVel1) / 2.0);
        engineReaction = SimEngineUpdateRpm(car, meanv);
        if (meanv != 0.0) {
            engineReaction = engineReaction/meanv;
            if ((spinVel1*spinVel0)>0) {
                if (engineReaction != 0.0) {
                    spinVel1 *= engineReaction;
                    spinVel0 *= engineReaction;
                }
            }
        }
    }

    differential->outAxis[0]->spinVel = spinVel0;
    differential->outAxis[1]->spinVel = spinVel1;

    differential->outAxis[0]->Tq = (differential->outAxis[0]->spinVel - differential->inAxis[0]->spinVel) / SimDeltaTime * differential->outAxis[0]->I;
    differential->outAxis[1]->Tq = (differential->outAxis[1]->spinVel - differential->inAxis[1]->spinVel) / SimDeltaTime * differential->outAxis[1]->I;
}


