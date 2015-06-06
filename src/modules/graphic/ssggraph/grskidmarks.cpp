/***************************************************************************

    file			: grskidmarks.cpp
    created		: Fri Mar 22 23:17:24 CET 2002
    copyright	: (C) 2001-2005 by Christophe Guionneau
                                 Christos Dimitrakakis
                                 Bernhard Wymann
    version		: $Id: grskidmarks.cpp 4236 2011-12-03 10:42:22Z pouillot $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <plib/ssg.h>

#include "grskidmarks.h"
#include "grmain.h"		//grHandle, grNbCars
#include "grshadow.h"	//ssgVtxTableShadow
#include "grcar.h"		//grCarInfo
#include "grscene.h"	//SkidAnchor

#ifdef DMALLOC
#include "dmalloc.h"
#endif

static ssgSimpleState *skidState = NULL;

static int grSkidMaxStripByWheel;
static int grSkidMaxPointByStrip;
static double grSkidDeltaT;
static sgVec3 nrm;
static ssgNormalArray *shd_nrm;


/**
 * grInitSkidmarks
 * Initializes the skidmark structure for a car
 *
 * @param car - the car to setup the skidmarks for
 */
void
grInitSkidmarks(const tCarElt *car)
{
	grSkidMaxStripByWheel = (int)GfParmGetNum(grHandle, GR_SCT_GRAPHIC, GR_ATT_MAXSTRIPBYWHEEL,
							(char*)NULL, MAXSTRIP_BYWHEEL);
	grSkidMaxPointByStrip = (int)GfParmGetNum(grHandle, GR_SCT_GRAPHIC, GR_ATT_MAXPOINTBYSTRIP,
							(char*)NULL, MAXPOINT_BY_STRIP);
	grSkidDeltaT = (double)GfParmGetNum(grHandle, GR_SCT_GRAPHIC, GR_ATT_SKIDDELTAT,
					(char*)NULL, DELTATSTRIP);

	if(grSkidMaxStripByWheel) {
		shd_nrm = new ssgNormalArray(1);
		nrm[0] = nrm[1] = 0.0;
		nrm[2] = 1.0;
		shd_nrm->add(nrm);

		if (skidState == NULL) {
			skidState = new ssgSimpleState();
			if (skidState) {
				skidState->disable(GL_LIGHTING);
				skidState->enable(GL_BLEND);
				skidState->enable(GL_CULL_FACE);
				// add texture
				skidState->enable(GL_TEXTURE_2D);
				skidState->setColourMaterial(GL_AMBIENT_AND_DIFFUSE);
				skidState->setTexture ("data/textures/grey-tracks.png", TRUE, TRUE, TRUE);
			}//if skidState
		}//if skidState NULL

		grCarInfo[car->index].skidmarks = new cGrSkidmarks;
	}//if grSkidMaxStripByWheel
}//grInitSkidmarks


/**
 * grUpdateSkidmarks
 * Updates the skidmarks for a car, if neccessary.
 *
 * @param car	the car with skidmarks
 * @param t time limits
 */
void
grUpdateSkidmarks(const tCarElt *car, const double t)
{
	if(grSkidMaxStripByWheel) {
		grCarInfo[car->index].skidmarks->Update(car, t);
	}//if grSkidMaxStripByWheel
}//grUpdateSkidmarks


/**
 * grShutdownSkidmarks
 * Removes all the skidmark informations of a car
 */
void
grShutdownSkidmarks(void)
{
	GfOut("-- grShutdownSkidmarks\n");

	if(grSkidMaxStripByWheel) {
		SkidAnchor->removeAllKids();
		for(int i = 0; i < grNbCars; i++) {
			delete grCarInfo[i].skidmarks;
			grCarInfo[i].skidmarks = NULL;
		}//for i
		skidState = NULL;
	}//if grSkidMaxStripByWheel
}//grShutdownSkidmarks


/**
 * grDrawSkidmarks - does nothing now.
 *
 * @param car The car to draw skidmarks for
 */
void
grDrawSkidmarks(const tCarElt *car)
{
    /* nothing to do */
}


/**
 * @class cGrSkidStrip
 * cGrSkidStrip class, has info about a skidmark strip
 */

/**
 * Constructor
 * Builds up all the arrays that hold the skids.
 */
cGrSkidStrip::cGrSkidStrip()
{
	vtx = new ssgVertexArray *[grSkidMaxStripByWheel];
	tex = new ssgTexCoordArray *[grSkidMaxStripByWheel];
	vta = new ssgVtxTableShadow *[grSkidMaxStripByWheel];
	clr = new ssgColourArray *[grSkidMaxStripByWheel];

	smooth_colour[0]=0.0;
	smooth_colour[1]=0.0;
	smooth_colour[2]=0.0;
	smooth_colour[3]=0.0;

	state = new int[grSkidMaxStripByWheel];
	size  = new int[grSkidMaxStripByWheel];

	for(int k = 0; k < grSkidMaxStripByWheel; k++) {
		state[k] = SKID_UNUSED;
		vtx[k] = new ssgVertexArray(grSkidMaxPointByStrip + 1);
		tex[k] = new ssgTexCoordArray(grSkidMaxPointByStrip + 1);
		clr[k] = new ssgColourArray(grSkidMaxPointByStrip + 1);
		vta[k] = new ssgVtxTableShadow(GL_TRIANGLE_STRIP, vtx[k], shd_nrm, tex[k], clr[k]);
		vta[k]->setCullFace(0);
		vta[k]->setState(skidState);
		timeStrip = 0;
		SkidAnchor->addKid(vta[k]);
	}//for k
	
	running_skid = 0;	// no skid is in use
	next_skid = 0;	// the next skid to used is the first one
	last_state_of_skid = false;	// there was no skid for this wheel during the next shot
	tex_state = 0.0;
}//cGrSkidStrip::cGrSkidStrip


/**
 * Destructor
 */
cGrSkidStrip::~cGrSkidStrip()
{
	delete [] vtx;
	delete [] vta;
	delete [] tex;
	delete [] state;
	delete [] size;
	delete [] clr;
}//~cGrSkidStrip


/**
 * Begin
 * Begins a new skid or continues the last one.
 * 
 * @param tvtx
 * @param tclist
 * @param sgVec4
 * @param t			Time constraints
 */
void
cGrSkidStrip::Begin(sgVec3 *tvtx, sgVec2 *tclist, sgVec4 *cur_clr, const double t)
{
	if(last_state_of_skid == false) {
		/* begin case */
		last_state_of_skid = true;
		/* start a new one */
		running_skid = next_skid;
		if(state[running_skid] != SKID_UNUSED) {
			vtx[running_skid]->removeAll();
			tex[running_skid]->removeAll();
			clr[running_skid]->removeAll();
			tex_state = 0.0;
		}//if state != SKID_UNUSED

		tex_state = 0.0;
		state[running_skid] = SKID_BEGIN;

		vtx[running_skid]->add(tvtx[0]);
		vtx[running_skid]->add(tvtx[1]);
		tex[running_skid]->add(tclist[0]);
		tex[running_skid]->add(tclist[1]);
		clr[running_skid]->add(*cur_clr);
		clr[running_skid]->add(*cur_clr);

		vta[running_skid]->recalcBSphere();
		size[running_skid] = 2;
		timeStrip = t;
		vta[running_skid]->setCullFace(TRUE);
	} else {
		/* continue case */
		vtx[running_skid]->add(tvtx[0]);
		vtx[running_skid]->add(tvtx[1]);
		tex[running_skid]->add(tclist[0]);
		tex[running_skid]->add(tclist[1]);
		clr[running_skid]->add(*cur_clr);
		clr[running_skid]->add(*cur_clr);

		size[running_skid] += 2;
		if(state[running_skid] == SKID_BEGIN)	{
			state[running_skid] = SKID_RUNNING;
			/*SkidAnchor->addKid(vta[running_skid]);*/
		}//if state == SKID_BEGIN
		vta[running_skid]->recalcBSphere();
		timeStrip=t;
	}//begin or continue skid
}//cGrSkidStrip::BeginOrContinue


/**
 * End
 * This is the end of a skid or there is nothing to do.
 * 
 */
void
cGrSkidStrip::End()
{
	if(last_state_of_skid == true) {
		/* end the running skid */
		state[running_skid] = SKID_STOPPED;
		vta[running_skid]->recalcBSphere();
		last_state_of_skid = false;

		next_skid += 1;
		if (next_skid >= grSkidMaxStripByWheel) {
			next_skid = 0;
			/* reset the next skid vertexArray */
			vtx[next_skid]->removeAll();
			tex[running_skid]->removeAll();
			clr[next_skid]->removeAll();
		}//if next_skid >= grSkidMaxStripByWheel
	}//if last_state_of_skid == true
}//cGrSkidStrip::End


/**
 * Update
 *
 * @param car The car to update the skidmarks for
 * @param t Time constraints
 */
void
cGrSkidmarks::Update(const tCarElt* car, const double t)
{
	sgVec4 cur_clr;
	cur_clr[0] = cur_clr[1] = cur_clr[2] = 1.0f;

	for(int i = 0; i < 4; i++) {
		tdble sling_mud = 1.0f;
		tdble skid_sensitivity = 0.75f;
		
		if(car->priv.wheel[i].seg) { // sanity check
			const char* s = car->priv.wheel[i].seg->surface->material;
			if(strstr(s, "sand")) {
				cur_clr[0] = 0.8f;
				cur_clr[1] = 0.6f;
				cur_clr[2] = 0.35f;
				skid_sensitivity = 0.9f;
			} else if (strstr(s, "dirt")) {
				cur_clr[0] = 0.7f;
				cur_clr[1] = 0.55f;
				cur_clr[2] = 0.45f;
				skid_sensitivity = 0.9f;
			} else if (strstr(s,"mud")) {
				cur_clr[0] = 0.5f;
				cur_clr[1] = 0.35f;
				cur_clr[2] = 0.15f;
				skid_sensitivity = 1.0f;
			} else if (strstr(s,"grass")) {
				cur_clr[0] = 0.75f;
				cur_clr[1] = 0.5f;
				cur_clr[2] = 0.3f;
				skid_sensitivity = 0.8f;
			} else if (strstr(s,"gravel")) {
				cur_clr[0] = 0.6f;
				cur_clr[1] = 0.6f;
				cur_clr[2] = 0.6f;
				skid_sensitivity = 0.7f;
			} else {
				sling_mud=0.0;
				cur_clr[0] = 0.0f;
				cur_clr[1] = 0.0f;
				cur_clr[2] = 0.0f;
				skid_sensitivity = 0.5f;
			}//if strstr(s, some_material)
		}//if car->priv.wheel

		if(car->_skid[i] > 0.1f) {
			cur_clr[3] = tanh(skid_sensitivity*car->_skid[i]);
		} else {
			cur_clr[3] = 0.0f;
		}//if car->skid

		for(int c = 0; c < 3; c++) {
			tdble tmp = strips[i].smooth_colour[c];
			strips[i].smooth_colour[c] = 0.9f*tmp + 0.1f*cur_clr[c];
			cur_clr[c] = tmp;
		}//for c

		if((t - strips[i].timeStrip) < grSkidDeltaT) {
			continue;
		}//if t -...

		if((car->_speed_x * car->_speed_x + car->_speed_y * car->_speed_y) > 1.0f) {
			if (cur_clr[3] > 0.1f) {
				ssgVertexArray *basevtx = new ssgVertexArray(4 * 2 + 1);
				tdble sling_left = -sling_mud;
				tdble sling_right = sling_mud; 

				// TO-DO: Temporary fix, trying to make sure that
				// skids are above the road surface. This is needed
				// because the wheel position depends on the current
				// physical model road height, which is not exactly
				// the same as the gfx road height, even if it is so
				// on average, because the physical model adds some
				// sinewave to the road height to simulate uneveness
				// of the track.  A better fix would be to add a
				// routine grTrackHeightL(tTrkLocPos *p), similar to 
				// TrTrackHeightL(), but which aim to give the height
				// of the graphical track.
				tdble z_adjust = 0.95f;
				tdble contact_z = car->priv.wheel[i].relPos.z - car->_wheelRadius(i)*z_adjust; 

				//One side
				sgVec3 vtx;
				vtx[0] = car->priv.wheel[i].relPos.x - car->_tireHeight(i);
				// Because of backface culling the winding of the triangles matters.
				if (car->_speed_x > 0.0f) {
					vtx[1] = car->priv.wheel[i].relPos.y + (sling_right + 1.0f)*car->_tireWidth(i) / 2.0f;
				} else {
					vtx[1] = car->priv.wheel[i].relPos.y + (sling_left - 1.0f)* car->_tireWidth(i) / 2.0f;
				}//if car->_speed
				vtx[2] = contact_z;
				basevtx->add(vtx);

				//Other side
				//vtx[0] = car->priv.wheel[i].relPos.x - car->_tireHeight(i);
				// Because of backface culling the winding of the triangles matters.
				if (car->_speed_x > 0.0f) {
					vtx[1] = car->priv.wheel[i].relPos.y + (sling_left - 1.0f)* car->_tireWidth(i) / 2.0f;
				} else {
					vtx[1] = car->priv.wheel[i].relPos.y + (sling_right + 1.0f)*car->_tireWidth(i) / 2.0f;
				}
				//vtx[2] = contact_z;
				basevtx->add(vtx);
				
				ssgTexCoordArray *texcoords = new ssgTexCoordArray();
				sgVec2 TxVtx;
				TxVtx[0] = strips[i].tex_state;
				TxVtx[1] = 0.75f + sling_right * 0.25f;
				texcoords->add(TxVtx);

				TxVtx[1] = 0.25f + sling_left * 0.25f;
				texcoords->add(TxVtx);

				//should get it from somewhere else really.
				//I could also account for radius, but that's a bit of an overkill.
				float dt = 0.01f;
				strips[i].tex_state += dt*car->_wheelSpinVel(i);

				sgVec3 *tvtx;
				sgVec2 *tclist;
				base = new ssgVtxTable(GL_TRIANGLE_STRIP, basevtx, NULL, texcoords, NULL);
				base->transform(grCarInfo[car->index].carPos);
				base->getVertexList((void**)&tvtx);
				base->getTexCoordList((void**)&tclist);

				strips[i].Begin(tvtx, tclist, &cur_clr, t);
				
				basevtx->removeAll();
				delete base;
			//end if (cur_clr[3] > 0.1f)
			} else {
				strips[i].End();
			}//else cur_clr[3] > 0.1f
		}//if car->_speed
	}//for i
}//cGrSkidmarks::Update


