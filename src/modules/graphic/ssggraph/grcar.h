
/***************************************************************************

    file                 : grcar.h
    created              : Mon Aug 21 18:21:15 CEST 2000
    copyright            : (C) 2000 by Eric Espie
    email                : torcs@free.fr
    version              : $Id: grcar.h 5602 2013-07-16 16:44:21Z torcs-ng $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

 
#ifndef _GRCAR_H_
#define _GRCAR_H_

#include <plib/ssg.h>
#include <car.h>			//tCarElt
#include <raceman.h>	//tSituation
#include "grcam.h"		//cGrPerspCamera

class cGrSkidmarks;
class ssgVtxTableShadow;

class tgrCarInstrument
{
public:
	ssgSimpleState	*texture;
	GLuint	CounterList;
	GLuint	needleList;
	tdble		needleXCenter, needleYCenter;
	tdble		digitXCenter, digitYCenter;
	tdble		minValue, maxValue;
	tdble		minAngle, maxAngle;
	tdble		*monitored;
	tdble		prevVal;
	tdble		rawPrev;
	int			digital;
	float		needleColor[4];
};


class tgrCarInfo
{
public:
	float				iconColor[4];
	ssgTransform		*carTransform;
	ssgSelector			*LODSelector;
	ssgSelector			*DRMSelector;
	ssgSelector			*DRMSelector2;
	ssgEntity			*carEntity;
	int                 nSteer;
	int					LODSelectMask[32];
	float				LODThreshold[32];
	int					DRMSelectMask[32];
	int					DRMSelectMask2[32];
	int                 nDRM;
	int					nDRM2;
	float				DRMThreshold[32];
	float				DRMThreshold2[32];
	float				lastDRMswitch;
	ssgSelector			*driverSelector;
	ssgSelector			*steerSelector;
	ssgSelector			*rearwingSelector;
	bool				driverSelectorinsg;
	bool				rearwingSelectorinsg;
	ssgStateSelector	*envSelector;
	ssgTransform		*wheelPos[4];
	ssgTransform		*wheelRot[4];
	ssgTransform		*steerRot[2];
	ssgColourArray		*brkColor[4];
	ssgSelector			*wheelselector[4];
	ssgState			*wheelTexture;
	ssgVtxTableShadow	*shadowBase;
	ssgVtxTableShadow	*shadowCurr;
	ssgBranch			*shadowAnchor;
	cGrSkidmarks		*skidmarks;
	sgMat4				carPos;
	tgrCarInstrument	instrument[2];
	tdble				distFromStart;
	tdble				envAngle;
	int					fireCount;
	tdble				steerMovt;
	tdble				px;
	tdble				py;
	tdble				sx;
	tdble				sy;
};

extern tgrCarInfo	*grCarInfo;

// grPreInitCar must be called once for each car before the first call to any other car function
extern void grPreInitCar(tCarElt *car);

extern void grInitShadow(tCarElt *car);
extern void grInitCar(tCarElt *car);
extern void grDrawCar(tSituation *, tCarElt*, tCarElt *, int, int, double curTime, class cGrPerspCamera *curCam);
extern void grInitCommonState(void);
//extern void grPropagateDamage (ssgEntity* l, sgVec3 poc, sgVec3 force, int cnt);
extern void grPropagateDamage (tSituation *);
#endif /* _GRCAR_H_ */ 
