/***************************************************************************

    file                 : aero.cpp
    created              : Sun Mar 19 00:04:50 CET 2000
    copyright            : (C) 2000 by Eric Espie
    email                : torcs@free.fr
    version              : $Id: aero.cpp 3948 2011-10-08 07:27:25Z wdbee $

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

tdble rho = 1.290; /* air density, prepare for variable environment */

void 
SimAeroConfig(tCar *car)
{
    void *hdle = car->params;
    tdble Cx, FrntArea;
    
    Cx       = GfParmGetNum(hdle, SECT_AERODYNAMICS, PRM_CX, (char*)NULL, 0.4f);
    FrntArea = GfParmGetNum(hdle, SECT_AERODYNAMICS, PRM_FRNTAREA, (char*)NULL, 2.5f);
    car->aero.Clift[0] = GfParmGetNum(hdle, SECT_AERODYNAMICS, PRM_FCL, (char*)NULL, 0.0f);
    car->aero.Clift[1] = GfParmGetNum(hdle, SECT_AERODYNAMICS, PRM_RCL, (char*)NULL, 0.0f);
    car->aero.CdBody = 0.645f * Cx * FrntArea;
    car->aero.Cd = car->aero.CdBody;
}


void 
SimAeroUpdate(tCar *car, tSituation *s)
{
    tdble	hm;
    int		i;	    
    tCar	*otherCar;
    tdble	x, y;
    tdble	yaw, otherYaw, airSpeed, tmpas, spdang, tmpsdpang, dyaw;
    tdble	dragK = 1.0;

    x = car->DynGCg.pos.x;
    y = car->DynGCg.pos.y;
    yaw = car->DynGCg.pos.az;
    airSpeed = car->DynGC.vel.x;
    spdang = atan2(car->DynGCg.vel.y, car->DynGCg.vel.x);

    if (airSpeed > 10.0) {
		for (i = 0; i < s->_ncars; i++) {
			if (i == car->carElt->index) {
				continue;
			}
			otherCar = &(SimCarTable[i]);
			otherYaw = otherCar->DynGCg.pos.az;
			tmpsdpang = spdang - atan2(y - otherCar->DynGCg.pos.y, x - otherCar->DynGCg.pos.x);
			FLOAT_NORM_PI_PI(tmpsdpang);
			dyaw = yaw - otherYaw;
			FLOAT_NORM_PI_PI(dyaw);
			if ((otherCar->DynGC.vel.x > 10.0) &&
				(fabs(dyaw) < 0.1396)) {
				if (fabs(tmpsdpang) > 2.9671) {	    /* 10 degrees */
					/* behind another car */
					tmpas = (tdble) (1.0 - exp(- 2.0 * DIST(x, y, otherCar->DynGCg.pos.x, otherCar->DynGCg.pos.y) /
									  (otherCar->aero.Cd * otherCar->DynGC.vel.x)));
					if (tmpas < dragK) {
						dragK = tmpas;
					}
				} else if (fabs(tmpsdpang) < 0.1396) {	    /* 8 degrees */
					/* before another car [not sure how much the drag should be reduced in this case. In no case it should be lowered more than 50% I think. - Christos] */
					tmpas = (tdble) (1.0 - 0.5f * exp(- 8.0 * DIST(x, y, otherCar->DynGCg.pos.x, otherCar->DynGCg.pos.y) / (car->aero.Cd * car->DynGC.vel.x)));
					if (tmpas < dragK) {
						dragK = tmpas;
					}
				}
			}
		}
    }
    car->airSpeed2 = airSpeed * airSpeed;
    tdble v2 = car->airSpeed2;

	// simulate ground effect drop off caused by non-frontal airflow (diffusor stops working etc.)

	// Never used : remove ?
	//tdble speed = sqrt(car->DynGC.vel.x*car->DynGC.vel.x + car->DynGC.vel.y*car->DynGC.vel.y);
	//tdble cosa = 1.0f;
	
    car->aero.drag = (tdble) (-SIGN(car->DynGC.vel.x) * car->aero.CdBody * v2 * (1.0f + (tdble)car->dammage / 10000.0f) * dragK * dragK);

    hm = 1.5f * (car->wheel[0].rideHeight + car->wheel[1].rideHeight + car->wheel[2].rideHeight + car->wheel[3].rideHeight);
    hm = hm*hm;
    hm = hm*hm;
    hm = 2 * exp(-3.0f*hm);
    car->aero.lift[0] = - car->aero.Clift[0] * v2 * hm;
    car->aero.lift[1] = - car->aero.Clift[1] * v2 * hm;
}

static const char *WingSect[2] = {SECT_FRNTWING, SECT_REARWING};

tdble F(tWing* wing)
{
	return 1 - exp( pow(-(wing->a / wing->b),wing->c));
}

tdble CliftFromAoA(tWing* wing)
{
	tdble angle = (tdble) (wing->angle * 180/PI);
	//fprintf(stderr,"wing->angle: %g rad = angle: %g deg\n",wing->angle,angle);

	wing->Kz_org = 4.0f * wing->Kx;

	if (angle <= wing->AoAatMax)
	{
		wing->a = wing->f * (angle + wing->AoAOffset);
		//fprintf(stderr,"a: %g\n",wing->a);
		double s = sin(wing->a/180.0*PI);
		//fprintf(stderr,"s: %g\n",s);
		return (tdble)(s * s * (wing->CliftMax + wing->d) - wing->d);
	}
	else
	{
		wing->a = (angle - wing->AoAatMax - 90.0f);
		//fprintf(stderr,"a: %g F(a): %g\n",wing->a,F(wing));
		return (tdble)((wing->CliftMax - F(wing) * (wing->CliftMax - wing->CliftAsymp)) * wing->Kx);
	}
}

void
SimWingConfig(tCar *car, int index)
{
    void   *hdle = car->params;
    tWing  *wing = &(car->wing[index]);
    tdble area;

    area              = GfParmGetNum(hdle, WingSect[index], PRM_WINGAREA, (char*)NULL, 0);
    wing->angle       = GfParmGetNum(hdle, WingSect[index], PRM_WINGANGLE, (char*)NULL, 0);
    wing->staticPos.x = GfParmGetNum(hdle, WingSect[index], PRM_XPOS, (char*)NULL, 0);
    wing->staticPos.z = GfParmGetNum(hdle, WingSect[index], PRM_ZPOS, (char*)NULL, 0);
    wing->staticPos.y = 0.0;

//>>> simuv4
	const char * w = GfParmGetStr(hdle, WingSect[index], PRM_WINGTYPE, "FLAT");

    wing->WingType = 0; // Default if nothing is contained in the wing section

	if (area == 0)
	  wing->WingType = -1;
	else if (strncmp(w,"FLAT",4) == 0)
	  wing->WingType = 0;
	else if (strncmp(w,"PROFILE",7) == 0)
	  wing->WingType = 1;
	else if (strncmp(w,"THIN",4) == 0)
	  wing->WingType = 2;
	// ...

	if (wing->WingType == 1)
	{
		//fprintf(stderr,"index: %d\n",index);
		//fprintf(stderr,"WingType: %d\n",wing->WingType);

		/* [deg] Angle of Attack at the maximum of coefficient of lift */
		wing->AoAatMax = GfParmGetNum(hdle, WingSect[index], PRM_AOAATMAX, (char*) "deg", 90);	
		//fprintf(stderr,"AoAatMax: %g\n",wing->AoAatMax);

		/* [deg] Angle of Attack at coefficient of lift = 0 (-30 < AoAatZero < 0) */
		wing->AoAatZero = GfParmGetNum(hdle, WingSect[index], PRM_AOAATZERO, (char*) "deg", 0);
		//fprintf(stderr,"AoAatZero: %g\n",wing->AoAatZero);
		wing->AoAatZRad = (tdble) (wing->AoAatZero/180*PI);

		/* [deg] Offset for Angle of Attack */
		wing->AoAOffset = GfParmGetNum(hdle, WingSect[index], PRM_AOAOFFSET, (char*) "deg", 0);	
		//fprintf(stderr,"AoAOffset: %g\n",wing->AoAOffset);

		/* Maximum of coefficient of lift (0 < CliftMax < 4) */
		wing->CliftMax = GfParmGetNum(hdle, WingSect[index], PRM_CLMAX, (char*)NULL, 4);
		//fprintf(stderr,"CliftMax: %g\n",wing->CliftMax);

		/* Coefficient of lift at Angle of Attack = 0 */
		wing->CliftZero = GfParmGetNum(hdle, WingSect[index], PRM_CLATZERO, (char*)NULL, 0);
		//fprintf(stderr,"CliftZero: %g\n",wing->CliftZero);

		/* Asymptotic coefficient of lift at large Angle of Attack */
		wing->CliftAsymp = GfParmGetNum(hdle, WingSect[index], PRM_CLASYMP, (char*)NULL, wing->CliftMax);
		//fprintf(stderr,"CliftAsymp: %g\n",wing->CliftAsymp);

		/* Delay of decreasing */
		wing->b = GfParmGetNum(hdle, WingSect[index], PRM_DELAYDECREASE, (char*)NULL, 20);			
		//fprintf(stderr,"b: %g\n",wing->b);

		/* Curvature of start of decreasing */
		wing->c = GfParmGetNum(hdle, WingSect[index], PRM_CURVEDECREASE, (char*)NULL, 2);						
		//fprintf(stderr,"c: %g\n",wing->c);

		/* Scale factor for angle */
		wing->f = (tdble) (90.0 / (wing->AoAatMax + wing->AoAOffset));			
		//fprintf(stderr,"f: %g\n",wing->f);
		double phi = wing->f * (wing->AoAOffset);
		//fprintf(stderr,"phi: %g deg\n",phi);
		phi *= PI / 180;
		//fprintf(stderr,"phi: %g rad\n",phi);
		double sinphi = sin(phi);
		//fprintf(stderr,"sinphi: %g\n",sinphi);
		double sinphi2 = sinphi * sinphi;

		/* Scale at AoA = 0 */
		wing->d = (tdble) (1.8f * (sinphi2 * wing->CliftMax - wing->CliftZero));	
		//fprintf(stderr,"d: %g\n",wing->d);
	}
	else if (wing->WingType == 2)
	{
		wing->AoAatZero = GfParmGetNum(hdle, WingSect[index], PRM_AOAATZERO, (char*)NULL, 0);
			wing->AoAatZero = MAX(MIN(wing->AoAatZero, 0), -PI_6);
		wing->AoStall = GfParmGetNum(hdle, WingSect[index], PRM_ANGLEOFSTALL, (char*)NULL, PI_6*0.5);
			wing->AoStall = MAX(MIN(wing->AoStall, PI_4), 0.017453293f);
		wing->Stallw = GfParmGetNum(hdle, WingSect[index], PRM_STALLWIDTH, (char*)NULL, 0.034906585);
			wing->Stallw = MAX(MIN(wing->Stallw, wing->AoStall), 0.017453293f);
		wing->AR = GfParmGetNum(hdle, WingSect[index], PRM_ASPECTRATIO, (char*)NULL, 0);
	}

	wing->Kx = -1.23f * area;

    if (wing->WingType == 0)
	{
		wing->Kz = 4.0f * wing->Kx;
	    if (index == 1)
		{
			car->aero.Cd = car->aero.CdBody - wing->Kx*sin(wing->angle);
			//fprintf(stderr,"Kz: %g Kx: %g\n",wing->Kz,wing->Kx);
			//fprintf(stderr,"car->aero.Cd: %g angle: %g\n",car->aero.Cd,wing->angle*180/PI);
		}
	}
	else if (wing->WingType == 1) 
	{
        wing->Kz = CliftFromAoA(wing) * wing->Kx;
		//fprintf(stderr,"Kz: %g Kx: %g\n",wing->Kz,wing->Kx);

		if (index == 0)
		{
			car->aero.Cd = (tdble)(car->aero.CdBody - wing->Kx*sin(wing->angle - wing->AoAatZRad));
			//fprintf(stderr,"car->aero.Cd: %g wing->Kx: %g angle: %g wing->AoAatZero: %g\n",car->aero.Cd,wing->Kx,wing->angle*180/PI,wing->AoAatZero);
		}
		else
		{
			car->aero.Cd -= (tdble)(wing->Kx*sin(wing->angle - wing->AoAatZRad));
			//fprintf(stderr,"car->aero.Cd: %g wing->Kx: %g wing->angle: %g wing->AoAatZero: %g\n",car->aero.Cd,wing->Kx,wing->angle*180/PI,wing->AoAatZero);
		}
	}
	else if (wing->WingType == 2)
	{
		if (wing->AR > 0.001) wing->Kz1 =  2 * PI * wing->AR / (wing->AR + 2);
		    else wing->Kz1 = 2 * PI;
		wing->Kx = 0.5 * rho * area;
		wing->Kz2 = 1.05;
		wing->Kz3 = 0.05;
		wing->Kx1 = 0.6;
		wing->Kx2 = 0.006;
		wing->Kx3 = 1.0;
		wing->Kx4 = 0.9;
	}
}


void
SimWingUpdate(tCar *car, int index, tSituation* s)
{
    tWing  *wing = &(car->wing[index]);

	/* return with 0 if no wing present */
	if (wing->WingType == -1) {
	    wing->forces.x = wing->forces.z = 0.0f;
	    return;
	}

	if (index == 1) {
		// Check wing angle controller
		if (car->ctrl->wingControlMode == 2)
			// Update wing angle
			wing->angle = car->ctrl->wingRearCmd;
		car->aero.Cd = car->aero.CdBody - wing->Kx*sin(wing->angle);
    }
	else
		// Check wing angle controller
		if (car->ctrl->wingControlMode == 2)
			// Update wing angle
			wing->angle = car->ctrl->wingFrontCmd;

    tdble vt2 = car->airSpeed2;
	// compute angle of attack
	tdble aoa = atan2(car->DynGC.vel.z, car->DynGC.vel.x) + car->DynGCg.pos.ay;

    aoa += wing->angle;

    if (wing->WingType == 2) //thin wing works for every direction
	{
	    tdble x;
	    while (aoa > PI) aoa -= 2 * PI;
	    while (aoa < -PI) aoa += 2 * PI;
	    /* first calculate coefficients */
	    if (aoa > PI_2)
	    {
		if (aoa > PI - wing->AoStall) wing->forces.x = wing->Kx1 * (PI - aoa) * (PI - aoa) + wing->Kx2;
		else wing->forces.x = wing->Kx3 - wing->Kx4 * cos(2*aoa);
		if (aoa > PI - wing->AoStall + wing->Stallw)
		    {x = (tdble)0.0;}
		else
		{
		    x = aoa - PI + wing->AoStall - wing->Stallw;
		    x = x * x / (x * x + wing->Stallw * wing->Stallw);
		}
		wing->forces.z = -(1-x) * wing->Kz1 * (aoa - PI + wing->AoAatZero) - x * (wing->Kz2 * sin(2*aoa) + wing->Kz3);
	    }
	    else if (aoa > 0)
	    {
		if (aoa < wing->AoStall) wing->forces.x = wing->Kx1 * aoa * aoa + wing->Kx2;
		else wing->forces.x = wing->Kx3 - wing->Kx4 * cos(2*aoa);
		if (aoa < wing->AoStall - wing->Stallw)
		    {x = (tdble)0.0;}
		else
		{
		    x = aoa - wing->AoStall + wing->Stallw;
		    x = x * x / (x * x + wing->Stallw * wing->Stallw);
		}
		wing->forces.z = -(1-x) * wing->Kz1 * (aoa - wing->AoAatZero) - x * (wing->Kz2 * sin(2*aoa) + wing->Kz3);
	    }
	    else if (aoa > -PI_2)
	    {
		if (aoa > -wing->AoStall) wing->forces.x = wing->Kx1 * aoa * aoa + wing->Kx2;
		else wing->forces.x = wing->Kx3 - wing->Kx4 * cos(2*aoa);
		if (aoa > -wing->AoStall + wing->Stallw)
		    {x = (tdble)0.0;}
		else
		{
		    x = aoa + wing->AoStall - wing->Stallw;
		    x = x * x / (x * x + wing->Stallw * wing->Stallw);
		}
		wing->forces.z = -(1-x) * wing->Kz1 * (aoa - wing->AoAatZero) - x * (wing->Kz2 * sin(2*aoa) - wing->Kz3);
	    }
	    else
	    {
		if (aoa < wing->AoStall - PI) wing->forces.x = wing->Kx1 * (PI + aoa) * (PI + aoa) + wing->Kx2;
		else wing->forces.x = wing->Kx3 - wing->Kx4 * cos(2*aoa);
		if (aoa < wing->AoStall - wing->Stallw - PI)
		    {x = (tdble)0.0;}
		else
		{
		    x = aoa - wing->AoStall + wing->Stallw + PI;
		    x = x * x / (x * x + wing->Stallw * wing->Stallw);
		}
		wing->forces.z = -(1-x) * wing->Kz1 * (aoa + wing->AoAatZero + PI) - x * (wing->Kz2 * sin(2*aoa) - wing->Kz3);
	    }
	    
	    /* add induced drag */
	    if (wing->AR > 0.001)
	    {
		if (wing->forces.x > 0.0)
			wing->forces.x += wing->forces.z * wing->forces.z / (wing->AR * 2.8274); //0.9*PI
		else wing->forces.x -= wing->forces.z * wing->forces.z / (wing->AR * 2.8274);
	    }
	    
	    /* then multiply with 0.5*rho*area and the square of velocity */
	    wing->forces.x *= - car->DynGC.vel.x * fabs(car->DynGC.vel.x) * wing->Kx * (1.0f + (tdble)car->dammage / 10000.0);
	    wing->forces.z *= wing->Kx * vt2;
	}
    else if (car->DynGC.vel.x > 0.0f)
	{
		if (wing->WingType == 0)
		{
		    // the sinus of the angle of attack
			tdble sinaoa = sin(aoa);

	        // make drag always negative and have a minimal angle of attack
		    wing->forces.x = (tdble) (wing->Kx * vt2 * (1.0f + (tdble)car->dammage / 10000.0) * MAX(fabs(sinaoa), 0.02));

			// If angle of attack is too large, no downforce, only drag
			if (fabs(aoa) > PI_2)
			{
				wing->forces.z = 0.0;
			}
			else
			{
				// 0 deg -> 30 deg as it was in simuV2.1
				if (fabs(aoa) < PI_6)
				{
				    sinaoa = sin(aoa);
				}
				else // 30 deg -> 90 deg smoothly reduced downforce 
				{
					sinaoa = (tdble) (0.25f * (1.0f - ((aoa-PI_3)/PI_6)*((aoa-PI_3)/PI_6)*((aoa-PI_3)/PI_6)));
				}
				wing->forces.z = (tdble) MIN(0.0,wing->Kz * vt2 * sinaoa);
			}
		}
		else if (wing->WingType == 1)
		{
			wing->forces.x = (tdble) (wing->Kx * vt2 * (1.0f + (tdble)car->dammage / 10000.0) * MAX(fabs(sin(aoa - wing->AoAatZRad)), 0.02));
			wing->forces.z = (tdble) MIN(0.0,wing->Kx* vt2 * CliftFromAoA(wing));
			// fprintf(stderr,"%d fz: %g (%g)\n",index,wing->forces.z,CliftFromAoA(wing));
		}
	} 
	else 
        wing->forces.x = wing->forces.z = 0.0f;
}

