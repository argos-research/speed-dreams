/***************************************************************************

    file                 : grcar.cpp
    created              : Mon Aug 21 18:24:02 CEST 2000
    copyright            : (C) 2000 by Eric Espie
    email                : torcs@free.fr
    version              : $Id: grcar.cpp 5943 2015-04-02 20:30:41Z torcs-ng $

***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <portability.h> // snprintf
#include <glfeatures.h>
#include <robottools.h> //RELAXATION

#include "grcar.h"
#include "grmain.h"
#include "grshadow.h"	//ssgVtxTableShadow
#include "grskidmarks.h"	//grDrawSkidmarks
#include "grsmoke.h"	//grAddSmoke
#include "grscene.h"	//CarsAnchor, ShadowAnchor
#include "grboard.h"	//grInitBoardCar
#include "grloadac.h"
#include "grutil.h"
#include "grcarlight.h"	//grUpdateCarlight


ssgBranch *CarsAnchorTmp = 0;

static int grCarIndex;

static ssgSimpleState *brakeState = NULL;
static ssgSimpleState *commonState = NULL;

void
grInitCommonState(void)
{
	/* brake */
	if (brakeState == NULL) {
		brakeState = new ssgSimpleState;
		brakeState->ref();
		brakeState->disable(GL_LIGHTING);
		brakeState->disable(GL_TEXTURE_2D);
	}

	if (commonState == NULL) {
		commonState = new ssgSimpleState;
		commonState->ref();
		commonState->disable(GL_LIGHTING);
		commonState->disable(GL_TEXTURE_2D);
		commonState->setColourMaterial(GL_AMBIENT_AND_DIFFUSE);
	}
}


static ssgTransform *
initWheel(tCarElt *car, int wheel_index, const char *wheel_mod_name)
{
	int	i, j, k;
	float	alpha;
	sgVec3	vtx;
	sgVec4	clr;
	sgVec3	nrm;
	sgVec2	tex;
	float	b_offset = 0;
	tdble	curAngle = 0.0;

	static const int brakeBranch = 16;
	static const tdble brakeAngle = 2.0 * M_PI / (tdble)brakeBranch;
	static const tdble brakeOffset = 0.1;

	switch(wheel_index) {
		case FRNT_RGT:
			curAngle = -(M_PI / 2.0 + brakeAngle);
			b_offset = brakeOffset - car->_tireWidth(wheel_index) / 2.0;
			break;
		case FRNT_LFT:
			curAngle = -(M_PI / 2.0 + brakeAngle);
			b_offset = car->_tireWidth(wheel_index) / 2.0 - brakeOffset;
			break;
		case REAR_RGT:
			curAngle = (M_PI / 2.0 - brakeAngle);
			b_offset = brakeOffset - car->_tireWidth(wheel_index) / 2.0;
			break;
		case REAR_LFT:
			curAngle = (M_PI / 2.0 - brakeAngle);
			b_offset = car->_tireWidth(wheel_index) / 2.0 - brakeOffset;
			break;
	}

	/* hub */
	ssgVertexArray	*brk_vtx = new ssgVertexArray(brakeBranch + 1);
	ssgColourArray	*brk_clr = new ssgColourArray(1);
	ssgNormalArray	*brk_nrm = new ssgNormalArray(1);
	tdble hubRadius;
	
	/* center */
	vtx[0] = vtx[2] = 0.0;
	vtx[1] = b_offset;
	brk_vtx->add(vtx);
	
	hubRadius = car->_brakeDiskRadius(wheel_index) * 0.6;
	for (i = 0; i < brakeBranch; i++) {
		alpha = (float)i * 2.0 * M_PI / (float)(brakeBranch - 1);
		vtx[0] = hubRadius * cos(alpha);
		vtx[1] = b_offset;
		vtx[2] = hubRadius * sin(alpha);
		brk_vtx->add(vtx);
	}
	

	clr[0] = clr[1] = clr[2] = 0.0;
	clr[3] = 1.0;
	brk_clr->add(clr);
	nrm[0] = nrm[2] = 0.0;

	// Make normal point outside to have proper lighting.
	switch(wheel_index) {
		case FRNT_RGT:
		case REAR_RGT:
			nrm[1] = -1.0;
			break;
		case FRNT_LFT:
		case REAR_LFT:
			nrm[1] = 1.0;
			break;
	}

	brk_nrm->add(nrm);

	ssgVtxTable *brk = new ssgVtxTable(GL_TRIANGLE_FAN, brk_vtx, brk_nrm, NULL, brk_clr);
	brk->setCullFace(0);
	brk->setState(commonState);

	ssgTransform *wheel = new ssgTransform;
	wheel->addKid(brk);

	/* Brake disk */
	brk_vtx = new ssgVertexArray(brakeBranch + 4);
	brk_clr = new ssgColourArray(1);
	brk_nrm = new ssgNormalArray(1);

	for (i = 0; i < (brakeBranch / 2 + 2); i++) {
		alpha = curAngle + (float)i * 2.0 * M_PI / (float)(brakeBranch - 1);
		vtx[0] = car->_brakeDiskRadius(wheel_index) * cos(alpha);
		vtx[1] = b_offset;
		vtx[2] = car->_brakeDiskRadius(wheel_index) * sin(alpha);
		brk_vtx->add(vtx);
		vtx[0] = car->_brakeDiskRadius(wheel_index) * cos(alpha) * 0.6;
		vtx[1] = b_offset;
		vtx[2] = car->_brakeDiskRadius(wheel_index) * sin(alpha) * 0.6;
		brk_vtx->add(vtx);
	}
	

	clr[0] = clr[1] = clr[2] = 0.1;
	clr[3] = 1.0;
	brk_clr->add(clr);
	//nrm[0] = nrm[2] = 0.0;
	//nrm[1] = 1.0;
	brk_nrm->add(nrm);
	
	brk = new ssgVtxTable(GL_TRIANGLE_STRIP, brk_vtx, brk_nrm, NULL, brk_clr);
	brk->setCullFace(0);
	brk->setState(brakeState);
	grCarInfo[grCarIndex].brkColor[wheel_index] = brk_clr;

	wheel->addKid(brk);

	/* Brake caliper */
	brk_vtx = new ssgVertexArray(brakeBranch - 4);
	brk_clr = new ssgColourArray(1);
	brk_nrm = new ssgNormalArray(1);

	for (i = 0; i < (brakeBranch / 2 - 2); i++) {
		alpha = - curAngle + (float)i * 2.0 * M_PI / (float)(brakeBranch - 1);
		vtx[0] = (car->_brakeDiskRadius(wheel_index) + 0.02) * cos(alpha);
		vtx[1] = b_offset;
		vtx[2] = (car->_brakeDiskRadius(wheel_index) + 0.02) * sin(alpha);
		brk_vtx->add(vtx);
		vtx[0] = car->_brakeDiskRadius(wheel_index) * cos(alpha) * 0.6;
		vtx[1] = b_offset;
		vtx[2] = car->_brakeDiskRadius(wheel_index) * sin(alpha) * 0.6;
		brk_vtx->add(vtx);
	}
	

	clr[0] = 0.2;
	clr[1] = 0.2;
	clr[2] = 0.2;
	clr[3] = 1.0;
	brk_clr->add(clr);
	//nrm[0] = nrm[2] = 0.0;
	//nrm[1] = 1.0;
	brk_nrm->add(nrm);

	brk = new ssgVtxTable(GL_TRIANGLE_STRIP, brk_vtx, brk_nrm, NULL, brk_clr);
	brk->setCullFace(0);
	brk->setState(commonState);

	wheel->addKid(brk);

	DBG_SET_NAME(wheel, "Wheel", grCarIndex, wheel_index);

	grCarInfo[grCarIndex].wheelPos[wheel_index] = wheel;

	/* wheels */
	ssgTransform *whrotation = new ssgTransform;
	grCarInfo[grCarIndex].wheelRot[wheel_index] = whrotation;
	wheel->addKid(whrotation);
	ssgSelector *whselector = new ssgSelector;
	whrotation->addKid(whselector);
	grCarInfo[grCarIndex].wheelselector[wheel_index] = whselector;

	float wheelRadius = car->_rimRadius(wheel_index) + car->_tireHeight(wheel_index);


	// Create wheels for 4 speeds (stillstanding - fast --> motion blur, look at the texture).
	// Note: These hard-coded values should really be read from the car's XML file
	char wheel_file_name[32];
	for (j = 0; j < 4; j++) {

	    ssgBranch *whl_branch = new ssgBranch;
	    
	    // Load speed-dependant 3D wheel model if available
	    ssgEntity *whl3d = 0;
	    if (wheel_mod_name && strlen(wheel_mod_name)) {
			snprintf(wheel_file_name, 32, "%s%d.acc", wheel_mod_name, j);
			whl3d = grssgCarWheelLoadAC3D(wheel_file_name, NULL, car->index);
	    }

	    // If we have a 3D wheel, use it, otherwise use auto- generated wheel...
	    if (whl3d) {
		// Adapt size of the wheel
		ssgTransform *whl_size = new ssgTransform;
		sgMat4 wheelsz;
		
		sgSetVec4(wheelsz[0], wheelRadius * 2, SG_ZERO, SG_ZERO, SG_ZERO) ;
		sgSetVec4(wheelsz[1], SG_ZERO, car->_tireWidth(wheel_index), SG_ZERO, SG_ZERO) ;
		sgSetVec4(wheelsz[2], SG_ZERO, SG_ZERO, wheelRadius * 2, SG_ZERO) ;
		sgSetVec4(wheelsz[3], SG_ZERO, SG_ZERO, SG_ZERO, SG_ONE) ;
		
		whl_size->setTransform(wheelsz);
		
		whl_size->addKid(whl3d);
		whl3d = whl_size;
		
		if (wheel_index == FRNT_RGT || wheel_index == REAR_RGT) {
		    // flip wheel around so it faces the right way
		    ssgTransform *whl_mesh_transform = new ssgTransform;
		    sgCoord wheelpos;
		    sgSetCoord(&wheelpos, 0, 0, 0, 180, 0, 0);
		    whl_mesh_transform->setTransform( &wheelpos);
		    whl_mesh_transform->addKid(whl3d);
		    whl_branch->addKid(whl_mesh_transform);
		} else {
		    whl_branch->addKid(whl3d);
		}

	    // If we don't have a 3D wheel, use auto-generated wheel...
	    } else {

	        static const int wheelBranch = 16;
		static const sgVec2 toffset[4] = 
		    { { 0.0, 0.5 }, { 0.5, 0.5 }, { 0.0, 0.0 }, { 0.5, 0.0 } };

		/* Try and load rim texture if not already done */
		if (!grCarInfo[grCarIndex].wheelTexture) {
		    const char *wheelTexFName = 
		      GfParmGetStr(car->_carHandle, SECT_GROBJECTS, PRM_WHEEL_TEXTURE, "tex-wheel.png");
		    grCarInfo[grCarIndex].wheelTexture = grSsgLoadTexState(wheelTexFName);
		    /*if (grCarInfo[grCarIndex].wheelTexture->getRef() > 0)
		      grCarInfo[grCarIndex].wheelTexture->deRef();*/
		}

		/* Tread */
		ssgVertexArray	*whl_vtx = new ssgVertexArray(2 * wheelBranch);
		ssgColourArray	*whl_clr = new ssgColourArray(2 * wheelBranch);
		ssgNormalArray	*whl_nrm = new ssgNormalArray(1);
		
		whl_nrm->add(nrm);
		clr[3] = 1.0;
		for (i = 0; i < wheelBranch; i++) {
		    alpha = (float)i * 2.0 * M_PI / (float)(wheelBranch - 1);
		    vtx[0] = wheelRadius * cos(alpha);
		    vtx[2] = wheelRadius * sin(alpha);
		    vtx[1] = - car->_tireWidth(wheel_index) / 2.0;
		    whl_vtx->add(vtx);
		    vtx[1] = car->_tireWidth(wheel_index) / 2.0;
		    whl_vtx->add(vtx);
		    if (i % 2)
			clr[0] = clr[1] = clr[2] = 0.15;
		    else
			clr[0] = clr[1] = clr[2] = 0.0;
		    whl_clr->add(clr);
		    whl_clr->add(clr);
		}

		ssgVtxTable *whl = new ssgVtxTable(GL_TRIANGLE_STRIP, whl_vtx, whl_nrm, NULL, whl_clr);
		whl->setState(commonState);
		whl->setCullFace(0);
		whl_branch->addKid(whl);

		/* Rim */
        switch(wheel_index)
        {
		    case FRNT_RGT:
		    case REAR_RGT:
			b_offset = -0.05;
			break;
		    case FRNT_LFT:
		    case REAR_LFT:
			b_offset = 0.05;
			break;
		}

		// Make inside rim very dark and take care of normals.
		float colorfactor[2];
		float norm_orig = nrm[1];

        if (nrm[1] > 0.0f)
        {
		    colorfactor[0] = 0.3f;
		    colorfactor[1] = 1.0f;
		    nrm[1] *= -1.0f;
		} else {
		    colorfactor[0] = 1.0f;
		    colorfactor[1] = 0.3f;
		}

		for (k = 0; k < 2; k++) {
		    ssgVertexArray	*whl_vtx = new ssgVertexArray(wheelBranch + 1);
		    ssgTexCoordArray	*whl_tex = new ssgTexCoordArray(wheelBranch + 1);
		    ssgColourArray	*whl_clr = new ssgColourArray(1);
		    ssgNormalArray	*whl_nrm = new ssgNormalArray(1);

		    clr[0] = 0.8f*colorfactor[k];
		    clr[1] = 0.8f*colorfactor[k];
		    clr[2] = 0.8f*colorfactor[k];
		    clr[3] = 1.0f;
		    
		    whl_clr->add(clr);
		    whl_nrm->add(nrm);
		    vtx[0] = vtx[2] = 0.0;
		    vtx[1] = (float)(2 * k - 1) * car->_tireWidth(wheel_index) / 2.0 - b_offset;
		    whl_vtx->add(vtx);
		    tex[0] = 0.25 + toffset[j][0];
		    tex[1] = 0.25 + toffset[j][1];
		    whl_tex->add(tex);
		    vtx[1] = (float)(2 * k - 1) * car->_tireWidth(wheel_index) / 2.0;
		    for (i = 0; i < wheelBranch; i++) {
			alpha = (float)i * 2.0 * M_PI / (float)(wheelBranch - 1);
			vtx[0] = wheelRadius * cos(alpha);
			vtx[2] = wheelRadius * sin(alpha);
			whl_vtx->add(vtx);
			tex[0] = 0.25 + 0.25 * cos(alpha) + toffset[j][0];
			tex[1] = 0.25 + 0.25 * sin(alpha) + toffset[j][1];
			whl_tex->add(tex);
		    }
		    ssgVtxTable *whl = new ssgVtxTable(GL_TRIANGLE_FAN, whl_vtx, whl_nrm, whl_tex, whl_clr);
		    whl->setState(grCarInfo[grCarIndex].wheelTexture);
		    whl->setCullFace(0);
		    whl_branch->addKid(whl);
		    
		    // Swap normal for "inside" rim face.
		    nrm[1] *= -1.0;
		}

		nrm[1] = norm_orig;
	    }

	    whselector->addKid(whl_branch);
	}
	
	return wheel;
}


#define GR_SHADOW_POINTS	6

void
grInitShadow(tCarElt *car)
{
	char		buf[512];
	const char	*shdTexName;
	int			i;
	float		x;
	sgVec3		vtx;
	sgVec4		clr;
	sgVec3		nrm;
	sgVec2		tex;
	ssgVertexArray	*shd_vtx = new ssgVertexArray(GR_SHADOW_POINTS+1);
	ssgColourArray	*shd_clr = new ssgColourArray(1);
	ssgNormalArray	*shd_nrm = new ssgNormalArray(1);
	ssgTexCoordArray	*shd_tex = new ssgTexCoordArray(GR_SHADOW_POINTS+1);

	snprintf(buf, sizeof(buf), "cars/models/%s;", car->_carName);
	if (strlen(car->_masterModel) > 0) // Add the master model path if we are using a template.
		snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "cars/models/%s;", car->_masterModel);
		
	grFilePath = buf;

	shdTexName = GfParmGetStr(car->_carHandle, SECT_GROBJECTS, PRM_SHADOW_TEXTURE, "");

	grCarInfo[car->index].shadowAnchor = new ssgBranch();

	clr[0] = clr[1] = clr[2] = 1.0;
	clr[3] = 1.0;
	shd_clr->add(clr);
	nrm[0] = nrm[1] = 0.0;
	nrm[2] = 1.0;
	shd_nrm->add(nrm);

	/* vertices */
#define MULT	1.1
	vtx[2] = 0.0;
	for (i = 0, x = car->_dimension_x * MULT / 2.0; i < GR_SHADOW_POINTS / 2;
		 i++, x -= car->_dimension_x * MULT / (float)(GR_SHADOW_POINTS - 2) * 2.0) {
		/*vtx[0] = x;
		vtx[1] = car->_dimension_y * MULT / 2.0;
		shd_vtx->add(vtx);
		tex[0] = 1.0 - (float)i / (float)((GR_SHADOW_POINTS - 2) / 2.0);
		tex[1] = 1.0;
		shd_tex->add(tex);

		vtx[1] = -car->_dimension_y * MULT / 2.0;
		shd_vtx->add(vtx);
		tex[1] = 0.0;
		shd_tex->add(tex);*/
		vtx[0] = x;
		tex[0] = 1.0 - (float)i / (float)((GR_SHADOW_POINTS - 2) / 2.0);

		vtx[1] = -car->_dimension_y * MULT / 2.0;
		shd_vtx->add(vtx);
		tex[1] = 0.0;
		shd_tex->add(tex);

		vtx[1] = car->_dimension_y * MULT / 2.0;
		shd_vtx->add(vtx);
		tex[1] = 1.0;
		shd_tex->add(tex);
	}

	grCarInfo[car->index].shadowBase = new ssgVtxTableShadow(GL_TRIANGLE_STRIP, shd_vtx, shd_nrm, shd_tex, shd_clr);
	grMipMap = 0;
	grCarInfo[car->index].shadowBase->setState(grSsgLoadTexState((char *)shdTexName));
	grCarInfo[car->index].shadowCurr = (ssgVtxTableShadow *)grCarInfo[car->index].shadowBase->clone(SSG_CLONE_GEOMETRY);
	grCarInfo[car->index].shadowAnchor->addKid(grCarInfo[car->index].shadowCurr);
	ShadowAnchor->addKid(grCarInfo[car->index].shadowAnchor);
	grCarInfo[car->index].shadowBase->ref();

}

void
grPropagateDamage (ssgEntity* l, sgVec3 poc, sgVec3 force, int cnt)
{
	//showEntityType (l);
	if (l->isAKindOf (ssgTypeBranch())) {
		
		ssgBranch* br = (ssgBranch*) l;

		for (int i = 0 ; i < br -> getNumKids () ; i++ ) {
			ssgEntity* ln = br->getKid (i);
			grPropagateDamage(ln, poc, force, cnt+1);
		}
	}
	
	if (l->isAKindOf (ssgTypeVtxTable())) {
		sgVec3* v;
		int Nv;
		ssgVtxTable* vt = (ssgVtxTable*) l;
		Nv = vt->getNumVertices();
		vt->getVertexList ((void**) &v);
		tdble sigma = sgLengthVec3 (force);
		tdble invSigma = 5.0;
		//		if (sigma < 0.1) 
		//			invSigma = 10.0;
		//		else
		//			invSigma = 1.0/sigma;
		for (int i=0; i<Nv; i++) {
			tdble r =  sgDistanceSquaredVec3 (poc, v[i]);
			tdble f = exp(-r*invSigma)*5.0;
			v[i][0] += force[0]*f;
			v[i][1] += force[1]*f;
			// use sigma as a random number generator (!)
			v[i][2] += (force[2]+0.02*sin(2.0*r + 10.0*sigma))*f;
			//printf ("(%f %f %f)\n", v[i][0], v[i][1], v[i][2]);
		}
	}
}

void
grPropagateDamage (tSituation *s)
{
	for (int i = 0; i < s->_ncars; i++) {
		tCarElt* car = s->cars[i];
		if (car->priv.collision_state.collision_count > 0) {
			tCollisionState* collision_state = &car->priv.collision_state;
			grPropagateDamage(grCarInfo[car->index].carEntity,
							  collision_state->pos, collision_state->force, 0);
		}
	}
}


void 
grPreInitCar(tCarElt *car)
{
	strncpy(car->_masterModel,
			GfParmGetStr(car->_carHandle, SECT_GROBJECTS, PRM_TEMPLATE, ""), MAX_NAME_LEN - 1);
	car->_masterModel[MAX_NAME_LEN - 1] = 0;
}

void 
grInitCar(tCarElt *car)
{
	static const char* pszTexFileExt = ".png";
	static const int nMaxTexPathSize = 4096;
	char buf[nMaxTexPathSize];
	int index;
	int selIndex;
	ssgEntity *carEntity;
	ssgSelector *LODSel;

	/* ssgBranchCb		*branchCb; */
	ssgTransform *wheel[4];
	int nranges;
	int i, j;
	void *handle;
	const char *param;
	int lg;
	char path[256];
	grssgLoaderOptions options;
	sgVec3 lightPos;
	int lightNum;
	const char *lightType;
	int lightTypeNum;

	TRACE_GL("loadcar: start");

	if (!CarsAnchorTmp) {
		CarsAnchorTmp = new ssgBranch();
	}

	grCarIndex = index = car->index;	/* current car's index */
	handle = car->_carHandle;

	/* Initialize board */
	grInitBoardCar(car);

	/* Schedule texture mapping if we are using a custom skin and/or a master 3D model */
	const bool bMasterModel = strlen(car->_masterModel) != 0;
	const bool bCustomSkin = strlen(car->_skinName) != 0;

	GfLogInfo("Loading graphics for %s (driver:%s, skin:%s.%x, master model:%s)\n",
			  car->_carName, car->_name,
			  bCustomSkin ? car->_skinName : "standard", car->_skinTargets,
			  bMasterModel ? car->_masterModel : "self");
	
	/* 1) Whole livery : <car name>.png => <car name>-<skin name>.png */
	std::string strSrcTexName(bMasterModel ? car->_masterModel : car->_carName);
	std::string strTgtTexName(car->_carName);
	if (bCustomSkin && car->_skinTargets & RM_CAR_SKIN_TARGET_WHOLE_LIVERY)
	{
		strTgtTexName += '-';
		strTgtTexName += car->_skinName;
	}

	if (strSrcTexName != strTgtTexName)
	{
		strSrcTexName += pszTexFileExt;
		strTgtTexName += pszTexFileExt;
		options.addTextureMapping(strSrcTexName.c_str(), strTgtTexName.c_str());
		GfLogTrace("Using skinned livery %s\n", strTgtTexName.c_str());
	}

	/* 2) Interior : <car name>-int.png => <car name>-int-<skin name>.png */
	if (bCustomSkin && car->_skinTargets & RM_CAR_SKIN_TARGET_INTERIOR)
	{
		strSrcTexName = (bMasterModel ? car->_masterModel : car->_carName);
		strTgtTexName = car->_carName;
		strTgtTexName += "-int-";
		strTgtTexName += car->_skinName;
		strSrcTexName += pszTexFileExt;
		strTgtTexName += pszTexFileExt;
		options.addTextureMapping(strSrcTexName.c_str(), strTgtTexName.c_str());
		GfLogTrace("Using skinned interior %s\n", strTgtTexName.c_str());
	}
	
	/* 3) driver if present */
	if (bCustomSkin && car->_skinTargets & RM_CAR_SKIN_TARGET_DRIVER)
	{
		strSrcTexName = "driver"; // Warning: Must be consistent with wheel<i>.ac/.acc contents
		strTgtTexName = strSrcTexName + '-' + car->_skinName + pszTexFileExt;
		strSrcTexName += pszTexFileExt;
		options.addTextureMapping(strSrcTexName.c_str(), strTgtTexName.c_str());
		GfLogTrace("Using skinned driver %s\n", strTgtTexName.c_str());
	}

	/* 4) 3D wheels if present */
	if (bCustomSkin && car->_skinTargets & RM_CAR_SKIN_TARGET_3D_WHEELS)
	{
		strSrcTexName = "wheel3d"; // Warning: Must be consistent with <car>.ac/.acc contents
		strTgtTexName = strSrcTexName + '-' + car->_skinName + pszTexFileExt;
		strSrcTexName += pszTexFileExt;
		options.addTextureMapping(strSrcTexName.c_str(), strTgtTexName.c_str());
		GfLogTrace("Using skinned 3D wheels %s\n", strTgtTexName.c_str());
	}

	grssgSetCurrentOptions(&options);

	/* Load visual attributes */
	car->_exhaustNb = GfParmGetEltNb(handle, SECT_EXHAUST);
	car->_exhaustNb = MIN(car->_exhaustNb, 2);
	car->_exhaustPower = GfParmGetNum(handle, SECT_EXHAUST, PRM_POWER, NULL, 1.0);
	for (i = 0; i < car->_exhaustNb; i++) {
		snprintf(path, 256, "%s/%d", SECT_EXHAUST, i + 1);
		car->_exhaustPos[i].x = GfParmGetNum(handle, path, PRM_XPOS, NULL, -car->_dimension_x / 2.0);
		car->_exhaustPos[i].y = -GfParmGetNum(handle, path, PRM_YPOS, NULL, car->_dimension_y / 2.0);
		car->_exhaustPos[i].z = GfParmGetNum(handle, path, PRM_ZPOS, NULL, 0.1);
	}

	snprintf(path, 256, "%s/%s", SECT_GROBJECTS, SECT_LIGHT);
	lightNum = GfParmGetEltNb(handle, path);
	for (i = 0; i < lightNum; i++) {
		snprintf(path, 256, "%s/%s/%d", SECT_GROBJECTS, SECT_LIGHT, i + 1);
		lightPos[0] = GfParmGetNum(handle, path, PRM_XPOS, NULL, 0);
		lightPos[1] = GfParmGetNum(handle, path, PRM_YPOS, NULL, 0);
		lightPos[2] = GfParmGetNum(handle, path, PRM_ZPOS, NULL, 0);
		lightType = GfParmGetStr(handle, path, PRM_TYPE, "");
		lightTypeNum = LIGHT_NO_TYPE;
		if (!strcmp(lightType, VAL_LIGHT_HEAD1)) {
			lightTypeNum = LIGHT_TYPE_FRONT;
		} else if (!strcmp(lightType, VAL_LIGHT_HEAD2)) {
			lightTypeNum = LIGHT_TYPE_FRONT2;
		} else if (!strcmp(lightType, VAL_LIGHT_BRAKE)) {
			lightTypeNum = LIGHT_TYPE_BRAKE;
		} else if (!strcmp(lightType, VAL_LIGHT_BRAKE2)) {
			lightTypeNum = LIGHT_TYPE_BRAKE2;
		} else if (!strcmp(lightType, VAL_LIGHT_REAR)) {
			lightTypeNum = LIGHT_TYPE_REAR;
		}
		grAddCarlight(car, lightTypeNum, lightPos, GfParmGetNum(handle, path, PRM_SIZE, NULL, 0.2));
	}

	grLinkCarlights(car);


	GfOut("[gr] Init(%d) car %s for driver %s index %d\n", index, car->_carName, car->_modName, car->_driverIndex);

	/* Set textures search path for (?) rear/front/brake lights, exhaust fires and wheels :
	   0) driver level specified, in the user settings
	   1) driver level specified, 2) car level specified, 3) common textures */
	grFilePath = (char*)malloc(nMaxTexPathSize);
	lg = 0;
	lg += snprintf(grFilePath + lg, nMaxTexPathSize - lg, "%sdrivers/%s/%d/%s;",
				   GfLocalDir(), car->_modName, car->_driverIndex, car->_carName);
	if (bMasterModel)
		lg += snprintf(grFilePath + lg, nMaxTexPathSize - lg, "%sdrivers/%s/%d/%s;",
					   GfLocalDir(), car->_modName, car->_driverIndex, car->_masterModel);
	
	lg += snprintf(grFilePath + lg, nMaxTexPathSize - lg, "%sdrivers/%s/%d;",
				   GfLocalDir(), car->_modName, car->_driverIndex);
	
	lg += snprintf(grFilePath + lg, nMaxTexPathSize - lg, "%sdrivers/%s/%s;",
				   GfLocalDir(), car->_modName, car->_carName);
	if (bMasterModel)
		lg += snprintf(grFilePath + lg, nMaxTexPathSize - lg, "%sdrivers/%s/%s;",
					   GfLocalDir(), car->_modName, car->_masterModel);
	
	lg += snprintf(grFilePath + lg, nMaxTexPathSize - lg, "%sdrivers/%s;",
				   GfLocalDir(), car->_modName);
	
	lg += snprintf(grFilePath + lg, nMaxTexPathSize - lg, "drivers/%s/%d/%s;",
				   car->_modName, car->_driverIndex, car->_carName);
	if (bMasterModel)
		lg += snprintf(grFilePath + lg, nMaxTexPathSize - lg, "drivers/%s/%d/%s;",
					   car->_modName, car->_driverIndex, car->_masterModel);
	
	lg += snprintf(grFilePath + lg, nMaxTexPathSize - lg, "drivers/%s/%d;",
				   car->_modName, car->_driverIndex);
	
	lg += snprintf(grFilePath + lg, nMaxTexPathSize - lg, "drivers/%s/%s;",
				   car->_modName, car->_carName);
	if (bMasterModel)
		lg += snprintf(grFilePath + lg, nMaxTexPathSize - lg, "drivers/%s/%s;",
					   car->_modName, car->_masterModel);

	lg += snprintf(grFilePath + lg, nMaxTexPathSize - lg, "drivers/%s;", car->_modName);

	lg += snprintf(grFilePath + lg, nMaxTexPathSize - lg, "cars/models/%s;", car->_carName);
	if (bMasterModel)
		lg += snprintf(grFilePath + lg, nMaxTexPathSize - lg, "cars/models/%s;", car->_masterModel);

	lg += snprintf(grFilePath + lg, nMaxTexPathSize - lg, "data/textures");

	grCarInfo[index].envSelector = (ssgStateSelector*)grEnvSelector->clone();
	grCarInfo[index].envSelector->ref();

	/* the base transformation of the car (rotation + translation) */
	grCarInfo[index].carTransform = new ssgTransform;
	DBG_SET_NAME(grCarInfo[index].carTransform, car->_modName, index, -1);

	/* Level of details */
	grCarInfo[index].LODSelector = LODSel = new ssgSelector;
	grCarInfo[index].carTransform->addKid(LODSel);
	snprintf(path, 256, "%s/%s", SECT_GROBJECTS, LST_RANGES);
	nranges = GfParmGetEltNb(handle, path) + 1;
	if (nranges < 2) {
		GfOut("Error not enough levels of detail\n");
		FREEZ(grFilePath);
		return;
	}

	/* First LOD */
	ssgBranch *carBody = new ssgBranch;
	DBG_SET_NAME(carBody, "LOD", index, 0);
	LODSel->addKid(carBody);

	/* Set 3D model search path and textures search path for the ones applied on the model :
	   0) driver level specified, in the user settings
	   1) driver level specified, 2) car level specified, 3) common models / textures */
	lg = 0;
	lg += snprintf(buf + lg, nMaxTexPathSize - lg, "%sdrivers/%s/%d/%s;",
				   GfLocalDir(), car->_modName, car->_driverIndex, car->_carName);
	if (bMasterModel)
		lg += snprintf(buf + lg, nMaxTexPathSize - lg, "%sdrivers/%s/%d/%s;",
					   GfLocalDir(), car->_modName, car->_driverIndex, car->_masterModel);
	
	lg += snprintf(buf + lg, nMaxTexPathSize - lg, "%sdrivers/%s/%d;",
				   GfLocalDir(), car->_modName, car->_driverIndex);
	
	lg += snprintf(buf + lg, nMaxTexPathSize - lg, "%sdrivers/%s/%s;",
				   GfLocalDir(), car->_modName, car->_carName);
	if (bMasterModel)
		lg += snprintf(buf + lg, nMaxTexPathSize - lg, "%sdrivers/%s/%s;",
					   GfLocalDir(), car->_modName, car->_masterModel);
	
	lg += snprintf(buf + lg, nMaxTexPathSize - lg, "%sdrivers/%s;",
				   GfLocalDir(), car->_modName);
	
	lg += snprintf(buf + lg, nMaxTexPathSize - lg, "drivers/%s/%d/%s;",
				   car->_modName, car->_driverIndex, car->_carName);
	if (bMasterModel)
		lg += snprintf(buf + lg, nMaxTexPathSize - lg, "drivers/%s/%d/%s;",
					   car->_modName, car->_driverIndex, car->_masterModel);
	
	lg += snprintf(buf + lg, nMaxTexPathSize - lg, "drivers/%s/%d;",
				   car->_modName, car->_driverIndex);
	
	lg += snprintf(buf + lg, nMaxTexPathSize - lg, "drivers/%s/%s;",
				   car->_modName, car->_carName);
	if (bMasterModel)
		lg += snprintf(buf + lg, nMaxTexPathSize - lg, "drivers/%s/%s;",
					   car->_modName, car->_masterModel);

	lg += snprintf(buf + lg, nMaxTexPathSize - lg, "drivers/%s;", car->_modName);

	lg += snprintf(buf + lg, nMaxTexPathSize - lg, "cars/models/%s;", car->_carName);
	if (bMasterModel)
		lg += snprintf(buf + lg, nMaxTexPathSize - lg, "cars/models/%s;", car->_masterModel);
	
	lg += snprintf(buf + lg, nMaxTexPathSize - lg, "data/objects;");

	lg += snprintf(buf + lg, nMaxTexPathSize - lg, "data/textures");

	ssgModelPath(buf);
	ssgTexturePath(buf);

	/* loading raw car level 0*/
	selIndex = 0; 	/* current selector index */
	snprintf(buf, nMaxTexPathSize, "%s.ac",
			 bMasterModel ? car->_masterModel : car->_carName); /* default car 3D model file */
	snprintf(path, 256, "%s/%s/1", SECT_GROBJECTS, LST_RANGES);
	param = GfParmGetStr(handle, path, PRM_CAR, buf);
	grCarInfo[index].LODThreshold[selIndex] = GfParmGetNum(handle, path, PRM_THRESHOLD, NULL, 0.0);
	carEntity = grssgCarLoadAC3D(param, NULL, index);
	grCarInfo[index].carEntity = carEntity;

	/* Set a selector on the driver */
	ssgBranch *b = (ssgBranch *)carEntity->getByName((char*)"DRIVER");
	grCarInfo[index].driverSelector = new ssgSelector;
	if (b) {
		ssgBranch *bp = b->getParent(0);
		bp->addKid(grCarInfo[index].driverSelector);
		grCarInfo[index].driverSelector->addKid(b);
		bp->removeKid(b);
		grCarInfo[index].driverSelector->select(1);
		grCarInfo[index].driverSelectorinsg = true;
	} else {
		grCarInfo[index].driverSelectorinsg = false;
	}

	/* Set a selector on the rearwing */
	ssgBranch *rw = (ssgBranch *)carEntity->getByName((char*)"REARWING");
	grCarInfo[index].rearwingSelector = new ssgSelector;
	if (rw) {
		ssgBranch *bp = b->getParent(0);
		bp->addKid(grCarInfo[index].rearwingSelector);
		grCarInfo[index].rearwingSelector->addKid(b);
		bp->removeKid(b);
		grCarInfo[index].rearwingSelector->select(1);
		grCarInfo[index].rearwingSelectorinsg = true;
	} else {
		grCarInfo[index].rearwingSelectorinsg = false;
	}

	/* Set a selector on the wing type*/
	snprintf(path, 256, "%s/%s", SECT_GROBJECTS, SECT_WING_MODEL);
	param = GfParmGetStr(handle, path, PRM_WING_1, NULL);
	if (param)
	{
		if (grSpeedway)
		{
            if (grSpeedwayshort)
                param = GfParmGetStr(handle, path, PRM_WING_1, NULL);

            else
                param = GfParmGetStr(handle, path, PRM_WING_2, NULL);
		}
        else
            param = GfParmGetStr(handle, path, PRM_WING_3, NULL);

		ssgEntity *wingEntity = grssgCarLoadAC3D(param, NULL, index);
		carBody->addKid(wingEntity);
	}

	DBG_SET_NAME(carEntity, "Body", index, -1);
	carBody->addKid(carEntity);

	/* add wheels : 3D wheel model is now shipped as wheel<speed-i>.acc files : 
	   auto-generated one is kept in code , but not used anymore */
	/*char *wheelTexFName = 
	    GfParmGetStrNC(handle, SECT_GROBJECTS, PRM_WHEEL_TEXTURE, "tex-wheel.png");
	    grCarInfo[index].wheelTexture = grSsgLoadTexState(wheelTexFName);*/
	/*if (grCarInfo[grCarIndex].wheelTexture->getRef() > 0)
	  grCarInfo[grCarIndex].wheelTexture->deRef();*/
	grCarInfo[index].wheelTexture = 0;
	const char *wheelFront3DModFileNamePrfx = 
	    GfParmGetStr(handle, SECT_GROBJECTS, PRM_FRONT_WHEEL_3D, "");
	const char *wheelRear3DModFileNamePrfx = 
	    GfParmGetStr(handle, SECT_GROBJECTS, PRM_REAR_WHEEL_3D, "");
	const char *wheel3DModFileNamePrfx = 
	    GfParmGetStr(handle, SECT_GROBJECTS, PRM_WHEEL_3D, "wheel");

	grGammaValue = 1.8;
	grMipMap = 0;

	if (*wheelFront3DModFileNamePrfx)
	{
	    wheel[FRNT_RGT] = initWheel(car, FRNT_RGT, wheelFront3DModFileNamePrfx);
	    wheel[FRNT_LFT] = initWheel(car, FRNT_LFT, wheelFront3DModFileNamePrfx);
	}
	else
	{
	    wheel[FRNT_RGT] = initWheel(car, FRNT_RGT, wheel3DModFileNamePrfx);
	    wheel[FRNT_LFT] = initWheel(car, FRNT_LFT, wheel3DModFileNamePrfx);
	}

	if (*wheelRear3DModFileNamePrfx)
	{
	    wheel[REAR_RGT] = initWheel(car, REAR_RGT, wheelRear3DModFileNamePrfx);
	    wheel[REAR_LFT] = initWheel(car, REAR_LFT, wheelRear3DModFileNamePrfx);
	}
	else
	{
	    wheel[REAR_RGT] = initWheel(car, REAR_RGT, wheel3DModFileNamePrfx);
	    wheel[REAR_LFT] = initWheel(car, REAR_LFT, wheel3DModFileNamePrfx);
	}

	for (i = 0; i < 4; i++)
	    carBody->addKid(wheel[i]);

	grCarInfo[index].LODSelectMask[0] = 1 << selIndex; /* car mask */
	selIndex++;
	grCarInfo[index].sx=carTrackRatioX;
	grCarInfo[index].sy=carTrackRatioY;

	/* Other LODs */
	for (i = 2; i < nranges; i++) {
		carBody = new ssgBranch;
		snprintf(buf, nMaxTexPathSize, "%s/%s/%d", SECT_GROBJECTS, LST_RANGES, i);
		param = GfParmGetStr(handle, buf, PRM_CAR, "");
		grCarInfo[index].LODThreshold[selIndex] = GfParmGetNum(handle, buf, PRM_THRESHOLD, NULL, 0.0);
		/* carEntity = ssgLoad(param); */
		carEntity = grssgCarLoadAC3D(param, NULL, index);;
		DBG_SET_NAME(carEntity, "LOD", index, i-1);
		carBody->addKid(carEntity);
		if (!strcmp(GfParmGetStr(handle, buf, PRM_WHEELSON, "no"), "yes")) {
			/* add wheels */
			for (j = 0; j < 4; j++){
				carBody->addKid(wheel[j]);
			}
		}
		LODSel->addKid(carBody);
		grCarInfo[index].LODSelectMask[i-1] = 1 << selIndex; /* car mask */
		selIndex++;
	}
	/* default range selection */
	LODSel->select(grCarInfo[index].LODSelectMask[0]);

	/* add Steering Wheel 0 (if one exists) */
	snprintf(path, 256, "%s/%s", SECT_GROBJECTS, SECT_STEERWHEEL);
	param = GfParmGetStr(handle, path, PRM_SW_MODEL, NULL);
	if (param)
	{
		ssgEntity *steerEntityLo = grssgCarLoadAC3D(param, NULL, index);
		ssgEntity *steerEntityHi = NULL;
		
		if (steerEntityLo)
		{
			grCarInfo[index].nSteer = 1;
			grCarInfo[index].steerMovt = GfParmGetNum(handle, path, PRM_SW_MOVT, NULL, 1.0);

			grCarInfo[index].steerSelector = new ssgSelector;
			ssgBranch *steerBranch = new ssgBranch;
			ssgTransform *steerLoc = new ssgTransform;

			sgCoord steerpos;
			tdble xpos = GfParmGetNum(handle, path, PRM_XPOS, NULL, 0.0);
			tdble ypos = GfParmGetNum(handle, path, PRM_YPOS, NULL, 0.0);
			tdble zpos = GfParmGetNum(handle, path, PRM_ZPOS, NULL, 0.0);
			tdble angl = GfParmGetNum(handle, path, PRM_SW_ANGLE, NULL, 0.0);

			grCarInfo[index].steerRot[0] = new ssgTransform;
			sgSetCoord(&steerpos, 0, 0, 0, 0, 0, 0);
			grCarInfo[index].steerRot[0]->setTransform( &steerpos );
			grCarInfo[index].steerRot[0]->addKid( steerEntityLo );

			sgSetCoord(&steerpos, xpos, ypos, zpos, 0, 0, angl);
			steerLoc->setTransform( &steerpos);
			steerLoc->addKid( grCarInfo[index].steerRot[0] );
			steerBranch->addKid( steerLoc );

			grCarInfo[index].steerSelector->addKid( steerBranch );
			grCarInfo[index].steerSelector->select(1);

			param = GfParmGetStr(handle, path, PRM_SW_MODELHR, NULL);
			if (param)
			{
				steerEntityHi = grssgCarLoadAC3D(param, NULL, index);
				if (steerEntityHi)
				{
					grCarInfo[index].nSteer = 2;

					grCarInfo[index].steerRot[1] = new ssgTransform;
					sgSetCoord(&steerpos, 0, 0, 0, 0, 0, 0);
					grCarInfo[index].steerRot[1]->setTransform( &steerpos );
					grCarInfo[index].steerRot[1]->addKid( steerEntityHi );

					steerBranch = new ssgBranch;
					steerLoc = new ssgTransform;

					sgSetCoord(&steerpos, xpos, ypos, zpos, 0, 0, angl);
					steerLoc->setTransform( &steerpos);
					steerLoc->addKid( grCarInfo[index].steerRot[1] );
					steerBranch->addKid( steerLoc );
					grCarInfo[index].steerSelector->addKid( steerBranch );
				}
			}

			grCarInfo[index].carTransform->addKid( grCarInfo[index].steerSelector );
		}
	}
	else
	{
		grCarInfo[index].nSteer = 0;
	}

	// separate driver models for animation according to steering wheel angle ...
	snprintf(path, 256, "%s/%s", SECT_GROBJECTS, LST_DRIVER);
	nranges = GfParmGetEltNb(handle, path) + 1;
	grCarInfo[index].nDRM = nranges - 1;
	grCarInfo[index].DRMSelector = NULL;

	if (nranges > 1)
	{
		// We have at least one separate driver model to add...
		ssgEntity *driverEntity;
		ssgSelector *DRMSel;
		grCarInfo[index].DRMSelector = DRMSel = new ssgSelector;
		grCarInfo[index].carTransform->addKid(DRMSel);
	
		selIndex = 0;

		// add the drivers
		for (i = 1; i < nranges; i++)
		{
			ssgBranch *driverBody = new ssgBranch;
			ssgTransform *driverLoc = new ssgTransform;
			sgCoord driverpos;

			snprintf(buf, nMaxTexPathSize, "%s/%s/%d", SECT_GROBJECTS, LST_DRIVER, i);
			param = GfParmGetStr(handle, buf, PRM_DRIVERMODEL, "");
			grCarInfo[index].DRMThreshold[selIndex] = GfParmGetNum(handle, buf, PRM_DRIVERSTEER, NULL, 0.0);

			tdble xpos = GfParmGetNum(handle, buf, PRM_XPOS, NULL, 0.0);
			tdble ypos = GfParmGetNum(handle, buf, PRM_YPOS, NULL, 0.0);
			tdble zpos = GfParmGetNum(handle, buf, PRM_ZPOS, NULL, 0.0);
			sgSetCoord(&driverpos, xpos, ypos, zpos, 0, 0, 0.0);
			driverLoc->setTransform( &driverpos);

			driverEntity = grssgCarLoadAC3D(param, NULL, index);
			DBG_SET_NAME(driverEntity, "DRM", index, i-1);

			driverLoc->addKid(driverEntity);

			driverBody->addKid(driverLoc);
			DRMSel->addKid(driverBody);
			grCarInfo[index].DRMSelectMask[i-1] = 1 << selIndex; 
			selIndex++;
		}
		
		// select a default driver - steer value of 0.0 is desired...
		for (i = 1; i < nranges; i++)
		{
			if (grCarInfo[index].DRMThreshold[i-1] == 0.0f)
			{
				DRMSel->select( grCarInfo[index].DRMSelectMask[i-1] );
				break;
			}
		}
		if (i == nranges)
			DRMSel->select( grCarInfo[index].DRMSelectMask[0] );
	}

	CarsAnchor->addKid(grCarInfo[index].carTransform);
    

	// Separate rear wing models for animation according to rear wing angle ...
	snprintf(path, 256, "%s/%s", SECT_GROBJECTS, LST_REARWING);
	nranges = GfParmGetEltNb(handle, path) + 1;
	grCarInfo[index].nDRM2 = nranges - 1;
	grCarInfo[index].DRMSelector2 = NULL;

	if (nranges > 1)
	{
		// We have at least one separate rearwing model to add...
		ssgEntity *rearwingEntity;
		ssgSelector *DRMSel2;
		grCarInfo[index].DRMSelector2 = DRMSel2 = new ssgSelector;
		grCarInfo[index].carTransform->addKid(DRMSel2);
	
		selIndex = 0;

		// Add the rearwings
		for (i = 1; i < nranges; i++)
		{
			ssgBranch *rearwingBody = new ssgBranch;
			ssgTransform *rearwingLoc = new ssgTransform;
			sgCoord rearwingpos;

			snprintf(buf, nMaxTexPathSize, "%s/%s/%d", SECT_GROBJECTS, LST_REARWING, i);
			param = GfParmGetStr(handle, buf, PRM_REARWINGMODEL, "");
			grCarInfo[index].DRMThreshold2[selIndex] = GfParmGetNum(handle, buf, PRM_REARWINGANGLE, NULL, 0.0);

			tdble xpos = GfParmGetNum(handle, buf, PRM_XPOS, NULL, 0.0);
			tdble ypos = GfParmGetNum(handle, buf, PRM_YPOS, NULL, 0.0);
			tdble zpos = GfParmGetNum(handle, buf, PRM_ZPOS, NULL, 0.0);
			sgSetCoord(&rearwingpos, xpos, ypos, zpos, 0, 0, 0.0);
			rearwingLoc->setTransform( &rearwingpos);

			rearwingEntity = grssgCarLoadAC3D(param, NULL, index);
			DBG_SET_NAME(rearwingEntity, "DRM2", index, i-1);

			rearwingLoc->addKid(rearwingEntity);

			rearwingBody->addKid(rearwingLoc);
			DRMSel2->addKid(rearwingBody);
			grCarInfo[index].DRMSelectMask2[i-1] = 1 << selIndex; 
			selIndex++;
		}
		
		// select a default rearwing - angle value of 0.0 is desired...
		for (i = 1; i < nranges; i++)
		{
			if (grCarInfo[index].DRMThreshold2[i-1] == 0.0f)
			{
				DRMSel2->select( grCarInfo[index].DRMSelectMask2[i-1] );
				break;
			}
		}
		if (i == nranges)
			DRMSel2->select( grCarInfo[index].DRMSelectMask2[0] );
	}

	CarsAnchor->addKid(grCarInfo[index].carTransform);

	//grCarInfo[index].carTransform->print(stdout, "-", 1);

	FREEZ(grFilePath);

	TRACE_GL("loadcar: end");
}

static void
grDrawShadow(tCarElt *car, int visible)
{
	int		i;
	ssgVtxTableShadow	*shadow;
	sgVec3	*vtx;

	if (grCarInfo[car->index].shadowAnchor->getNumKids() != 0) {
		grCarInfo[car->index].shadowAnchor->removeKid(grCarInfo[car->index].shadowCurr);
	}

	if (visible) {
		shadow = (ssgVtxTableShadow *)grCarInfo[car->index].shadowBase->clone(SSG_CLONE_GEOMETRY);
		/* shadow->setState(shadowState); */
		shadow->setCullFace(TRUE);
		shadow->getVertexList((void**)&vtx);

		shadow->transform(grCarInfo[car->index].carPos);

		for (i = 0; i < GR_SHADOW_POINTS; i++) {
			vtx[i][2] = RtTrackHeightG(car->_trkPos.seg, vtx[i][0], vtx[i][1]) + 0.00;
		}

		grCarInfo[car->index].shadowCurr = shadow;
		grCarInfo[car->index].shadowAnchor->addKid(shadow);
	}
}


tdble grGetDistToStart(tCarElt *car)
{
	tTrackSeg *seg;
	tdble lg;

	seg = car->_trkPos.seg;
	lg = seg->lgfromstart;

	switch (seg->type) {
		case TR_STR:
			lg += car->_trkPos.toStart;
			break;
		default:
			lg += car->_trkPos.toStart * seg->radius;
			break;
	}
	return lg;
}

void
grDrawCar(tSituation *s, tCarElt *car, tCarElt *curCar, int dispCarFlag, int dispDrvFlag, double curTime, class cGrPerspCamera *curCam)
{
	sgCoord wheelpos;
	int index, i, j;
	static float maxVel[3] = { 20.0, 40.0, 70.0 };
	float lod;

	TRACE_GL("cggrDrawCar: start");

	index = car->index;

	grCarInfo[index].distFromStart=grGetDistToStart(car);
	grCarInfo[index].envAngle=RAD2DEG(car->_yaw);

	if (grCarInfo[index].nSteer > 0)
		grCarInfo[index].steerSelector->select(1);

	if ((car == curCar) && (dispCarFlag != 1)) {
		grCarInfo[index].LODSelector->select(0);
		if (grCarInfo[index].nDRM > 0)
			grCarInfo[index].DRMSelector->select(0);
	} else {
		lod = curCam->getLODFactor(car->_pos_X, car->_pos_Y, car->_pos_Z);
		i = 0;
		while (lod < grCarInfo[index].LODThreshold[i] * grLodFactorValue) {
			i++;
		}
		if ((car->_state & RM_CAR_STATE_DNF) && (grCarInfo[index].LODThreshold[i] > 0.0)) {
			i++;
		}
		grCarInfo[index].LODSelector->select(grCarInfo[index].LODSelectMask[i]);
		if (dispDrvFlag || car != curCar) 
		{
			grCarInfo[index].driverSelector->select(1);

	 		// Animated driver model selection according to steering wheel angle
			if (grCarInfo[index].nDRM > 0 && s->currentTime - grCarInfo[index].lastDRMswitch > 0.1)
			{
				// choose a driver model to display
				int curDRM = 0;
				float curSteer = 0.0f;
				int lastDRM = grCarInfo[index].DRMSelector->getSelect();
	
				for (i=0; i<grCarInfo[index].nDRM; i++)
				{
					if ((car->_steerCmd > 0.0 && 
					    grCarInfo[index].DRMThreshold[i] >= 0.0 &&
					    grCarInfo[index].DRMThreshold[i] <= car->_steerCmd &&
					    grCarInfo[index].DRMThreshold[i] >= curSteer) ||
					    (car->_steerCmd < 0.0 && 
					    grCarInfo[index].DRMThreshold[i] <= 0.0 &&
					    grCarInfo[index].DRMThreshold[i] >= car->_steerCmd &&
					    grCarInfo[index].DRMThreshold[i] <= curSteer))
					{
						curDRM = i;
						curSteer = grCarInfo[index].DRMThreshold[i];
					}
				}
		
				grCarInfo[index].DRMSelector->select( grCarInfo[index].DRMSelectMask[curDRM] );
				if (lastDRM != curDRM)
				{
					grCarInfo[index].lastDRMswitch = s->currentTime;
				}
			}
		} 
		else 
		{
			grCarInfo[index].driverSelector->select(0);
			//grCarInfo[index].DRMSelector->select(0);
			if (grCarInfo[index].nDRM > 0)
				grCarInfo[index].DRMSelector->select(0);
			if (grCarInfo[index].nSteer > 1)
				grCarInfo[index].steerSelector->select(2);
		}

		if (dispDrvFlag || car != curCar) 
		{
			grCarInfo[index].rearwingSelector->select(1);

	 		// Animated rearwing model selection according to wing angle
			if (grCarInfo[index].nDRM2 > 0)
			{
				// choose a rearwing model to display
				int curDRM = 0;
				//float curAngle = 0.0f;
				//int lastDRM = grCarInfo[index].DRMSelector->getSelect();
	
				for (i=0; i<grCarInfo[index].nDRM2; i++)
				{
					float wingangle = car->_wingRCmd * 180 / PI;
					if ((wingangle > 0.0) 
						&& (wingangle < 10.0)
					    && (grCarInfo[index].DRMThreshold2[i] >= 0.0)
					    && (grCarInfo[index].DRMThreshold2[i] <= 10.0))
					{
						curDRM = i;
						//curAngle = grCarInfo[index].DRMThreshold[i];
					}
					else if ((wingangle > 10.0) 
						&& (wingangle < 35.0)
					    && (grCarInfo[index].DRMThreshold2[i] >= 10.0)
					    && (grCarInfo[index].DRMThreshold2[i] <= 35.0))
					{
						curDRM = i;
						//curAngle = grCarInfo[index].DRMThreshold[i];
					}
					else if ((wingangle > 35.0) 
					    && (grCarInfo[index].DRMThreshold2[i] > 35.0))
					{
						curDRM = i;
						//curAngle = grCarInfo[index].DRMThreshold[i];
					}
				}
		
				grCarInfo[index].DRMSelector2->select( grCarInfo[index].DRMSelectMask2[curDRM] );
			}
		} 
		else 
		{
			grCarInfo[index].rearwingSelector->select(0);
			if (grCarInfo[index].nDRM2 > 0)
				grCarInfo[index].DRMSelector2->select(0);
		}
	}

	if (grCarInfo[index].nSteer)
	{
		sgSetCoord( &wheelpos, 0, 0, 0, 0, RAD2DEG(-car->_steerCmd * grCarInfo[index].steerMovt), 0 );
		grCarInfo[index].steerRot[0]->setTransform( &wheelpos );
		if (grCarInfo[index].nSteer > 1)
			grCarInfo[index].steerRot[1]->setTransform( &wheelpos );
	}

	sgCopyMat4(grCarInfo[index].carPos, car->_posMat);
	grCarInfo[index].px=car->_pos_X;
	grCarInfo[index].py=car->_pos_Y;

	grCarInfo[index].carTransform->setTransform(grCarInfo[index].carPos);

	if ((car == curCar) && (dispCarFlag != 1)) {
		grDrawShadow(car, 0);
	} else {
		grDrawShadow(car, 1);
	}
	
	grUpdateSkidmarks(car, curTime); 
	grDrawSkidmarks(car);
	grAddSmoke(car, curTime);
	
	if ((car == curCar) && (dispCarFlag != 1)) {
		grUpdateCarlight(car, curCam, 0);
	} else {
		grUpdateCarlight(car, curCam, 1);
	}

	/* Env mapping selection by the position on the track */
	grCarInfo[index].envSelector->selectStep(car->_trkPos.seg->envIndex);

	/* wheels */
	for (i = 0; i < 4; i++) {
		float	*clr;

		sgSetCoord(&wheelpos, car->priv.wheel[i].relPos.x, car->priv.wheel[i].relPos.y, car->priv.wheel[i].relPos.z,
					RAD2DEG(car->priv.wheel[i].relPos.az), RAD2DEG(car->priv.wheel[i].relPos.ax), 0);
		grCarInfo[index].wheelPos[i]->setTransform(&wheelpos);
		sgSetCoord(&wheelpos, 0, 0, 0, 0, 0, RAD2DEG(car->priv.wheel[i].relPos.ay));
		grCarInfo[index].wheelRot[i]->setTransform(&wheelpos);
		for (j = 0; j < 3; j++) {
			if (fabs(car->_wheelSpinVel(i)) < maxVel[j]) 
				break;
		}
		grCarInfo[index].wheelselector[i]->select(1<<j);
		clr = grCarInfo[index].brkColor[i]->get(0);
		clr[0] = 0.1 + car->_brakeTemp(i) * 1.5;
		clr[1] = 0.1 + car->_brakeTemp(i) * 0.3;
		clr[2] = 0.1 - car->_brakeTemp(i) * 0.3;
	}

	/* push the car at the end of the display order */
	CarsAnchorTmp->addKid(grCarInfo[index].carTransform);
	CarsAnchor->removeKid(grCarInfo[index].carTransform);
	CarsAnchor->addKid(grCarInfo[index].carTransform);
	CarsAnchorTmp->removeKid(grCarInfo[index].carTransform);

	TRACE_GL("cggrDrawCar: end");
}

