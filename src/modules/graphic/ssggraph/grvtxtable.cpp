/***************************************************************************

    file                 : grvtxtable.cpp
    created              : Fri Mar 22 23:16:44 CET 2002
    copyright            : (C) 2001 by Christophe Guionneau
    version              : $Id: grvtxtable.cpp 4130 2011-11-10 18:29:06Z pouillot $

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

#include <glfeatures.h>

#include "grvtxtable.h"
#include "grmain.h"
#include "grscene.h"	//grEnvState, grEnvShadowState, grEnvShadowStateOnCars
#include "grcar.h"		//grCarInfo
#include "grutil.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

/* from grloadac.cpp (beuark!) */
extern double shad_xmax;
extern double shad_ymax;
extern double shad_xmin;
extern double shad_ymin;


cgrVtxTable::cgrVtxTable ()
: ssgVtxTable()
{
	//GfLogDebug("cgrVtxTable@%p()\n", this);

	_nTexMaps = 1; // The one of the ssgVtxTable.
	
	for (int nStInd = 0; nStInd < NMaxTexMaps-1; nStInd++)
	{
		_mTexCoords[nStInd] = 0;
		_mTexStates[nStInd] = 0;
	}
	
	_indices = 0;
	_stripes = 0;
	_numStripes = 0;
}

cgrVtxTable::cgrVtxTable (int nTexMaps,
						  GLenum ty, ssgVertexArray   *vl,
						  ssgNormalArray   *nl,
						  ssgTexCoordArray *tl,
						  ssgTexCoordArray *tl1, ssgTexCoordArray *tl2, ssgTexCoordArray *tl3,
						  ssgColourArray *cl,
						  ssgIndexArray * stripeIndex, int numstripes, ssgIndexArray *il)
: ssgVtxTable(ty, vl, nl, tl, cl)
{
	// GfLogDebug("cgrVtxTable@%p(tm=%d, tl=%p, tl1=%p, tl2=%p, tl3=%p"
	// 		   ", si=%p, sn=%d, sl=%p, cl=%p)\n",
	// 		   this, nTexMaps, tl, tl1, tl2, tl3, stripeIndex, numstripes, il, cl);

	_nTexMaps = nTexMaps < 1 ? 1 : nTexMaps; // At least the one of the ssgVtxTable.

	_mTexCoords[0] = tl1 ? tl1 : new ssgTexCoordArray();
	_mTexCoords[0]->ref();
	_mTexCoords[1] = tl2 ? tl2 : new ssgTexCoordArray();
	_mTexCoords[1]->ref();
	_mTexCoords[2] = tl3 ? tl3 : new ssgTexCoordArray();
	_mTexCoords[2]->ref();

	for (int nStInd = 0; nStInd < NMaxTexMaps-1; nStInd++)
	{
		_mTexStates[nStInd] = 0;
	}

	// Optional stripes.
	_indices = il; // ? il : new ssgIndexArray();
	if (_indices)
		_indices->ref();
	_stripes = stripeIndex; // ? stripeIndex : new ssgIndexArray();
	if (_stripes)
		_stripes->ref();
	_numStripes = numstripes;
}


cgrVtxTable::~cgrVtxTable ()
{
	for (int nStInd = 0; nStInd < NMaxTexMaps-1; nStInd++)
	{
		ssgDeRefDelete(_mTexCoords[nStInd]);
		ssgDeRefDelete(_mTexStates[nStInd]);
	}

	if (_stripes) {
		ssgDeRefDelete(_indices);
		ssgDeRefDelete(_stripes);
	}
}

void cgrVtxTable::copy_from (cgrVtxTable *src, int clone_flags)
{
	ssgVtxTable::copy_from (src, clone_flags);

	_nTexMaps = src->_nTexMaps;

	for (int nStInd = 0; nStInd < NMaxTexMaps-1; nStInd++)
	{
		if (src->_mTexCoords[nStInd] && (clone_flags & SSG_CLONE_GEOMETRY)) {
			_mTexCoords[nStInd] =
				(ssgTexCoordArray *)(src->_mTexCoords[nStInd]->clone(clone_flags));
		} else {
			_mTexCoords[nStInd] = src->_mTexCoords[nStInd];
		}
	}

	// TODO : Clone states ?
	
	if (src->_stripes) {
		_numStripes = src->_numStripes;
		ssgDeRefDelete(_indices);
		if (src->_indices && (clone_flags & SSG_CLONE_GEOMETRY)) {
			_indices = (ssgIndexArray *)(src->_indices->clone(clone_flags));
		} else {
			_indices = src->_indices;
		}

		if (_indices) {
			_indices->ref();
		}

		ssgDeRefDelete(_stripes);
		if (src->_stripes && (clone_flags & SSG_CLONE_GEOMETRY)) {
			_stripes = (ssgIndexArray *)(src->_stripes->clone(clone_flags));
		} else {
			_stripes = src->_stripes;
		}

		if (_stripes) {
			_stripes->ref();
		}
	}
}

ssgBase *cgrVtxTable::clone (int clone_flags)
{
	cgrVtxTable *b = new cgrVtxTable;
	b->copy_from(this, clone_flags);
	return b;
}

float *cgrVtxTable::getMultiTexCoord (int nStateInd, int nCoordInd)
{
	if (nCoordInd >= getNumTexCoords())
		nCoordInd = getNumTexCoords()-1;
	return (getNumTexCoords() <= 0) ? _ssgTexCoord00 : _mTexCoords[nStateInd]->get(nCoordInd);
}

cgrMultiTexState *cgrVtxTable::getMultiTexState (int nStateInd)
{
	return _mTexStates[nStateInd] ;
}

void cgrVtxTable::setMultiTexState (int nStateInd, cgrMultiTexState *st)
{
	ssgDeRefDelete (_mTexStates[nStateInd]);

	_mTexStates[nStateInd] = st;

	if (_mTexStates[nStateInd]) {
		_mTexStates[nStateInd]->ref();
	}
}

void cgrVtxTable::draw_geometry_array ()
{
	TRACE_GL("cgrVtxTable::draw_geometry_array: start");

	int num_colours = getNumColours();
	int num_normals = getNumNormals();
	int numTexCoords = getNumTexCoords();

	sgVec3 *nm = (sgVec3 *) getNormals()->get(0);
	sgVec4 *cl = (sgVec4 *) getColours()->get(0);

	if (_mTexStates[0]) {
		_mTexStates[0]->apply(GL_TEXTURE1_ARB);
	}

	if (_mTexStates[1]) {
		_mTexStates[1]->apply(GL_TEXTURE2_ARB);
	}

	if (grMaxTextureUnits > 1) {
		glActiveTextureARB(GL_TEXTURE0_ARB);
	}

	glEnable (GL_TEXTURE_2D);

	if (num_colours == 0) {
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	}

	if (num_colours == 1) {
		glColor4fv(cl[0]);
	}

	if (num_normals == 1) {
		glNormal3fv(nm[0]);
	}

	glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);

	if (num_normals > 1) {
		glEnableClientState(GL_NORMAL_ARRAY);
		glNormalPointer(GL_FLOAT, 0, getNormals()->get(0));
	}

	if (numTexCoords > 1) {
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, 0, getTexCoords()->get(0));

		if (_mTexStates[0]) {
			glClientActiveTextureARB(GL_TEXTURE1_ARB);
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			glTexCoordPointer(2, GL_FLOAT, 0, _mTexCoords[0]->get(0));
		}

		if (_mTexStates[1]) {
			glClientActiveTextureARB(GL_TEXTURE2_ARB);
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			glTexCoordPointer(2, GL_FLOAT, 0, _mTexCoords[1]->get(0));
		}

	}

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, getVertices()->get(0));

	if (grMaxTextureUnits > 1) {
		glClientActiveTextureARB(GL_TEXTURE0_ARB);
	}
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);


	int i = 0;
	short *ii = 0;
	int j = 0;
	int p = 0;

	for (j = 0; j < _numStripes; j++) {
		i = (short)*(_stripes->get(j));
		ii = _indices->get(p);
		glDrawElements(gltype, i, GL_UNSIGNED_SHORT, ii);
		p += i;
	}

	glPopClientAttrib ();
	if (_mTexStates[0]) {
		glActiveTextureARB(GL_TEXTURE1_ARB);
		glDisable(GL_TEXTURE_2D);
	}

	if (_mTexStates[1]) {
		glActiveTextureARB(GL_TEXTURE2_ARB);
		glDisable(GL_TEXTURE_2D);
	}

	if (grMaxTextureUnits > 1) {
		glActiveTextureARB(GL_TEXTURE0_ARB);
	}
	
	TRACE_GL("cgrVtxTable::draw_geometry_array: end");
}

// cgrVtxTableTrackPart class ======================================================

cgrVtxTableTrackPart::cgrVtxTableTrackPart(int nTexMaps,
										   GLenum ty, ssgVertexArray   *vl,
										   ssgNormalArray   *nl,
										   ssgTexCoordArray *tl,
										   ssgTexCoordArray *tl1, ssgTexCoordArray *tl2, ssgTexCoordArray *tl3,
										   ssgColourArray *cl,
										   ssgIndexArray* stripeIndex, int numstripes, ssgIndexArray *il)
: cgrVtxTable(nTexMaps, ty, vl, nl, tl, tl1, tl2, tl3, cl, stripeIndex, numstripes, il)
{
	//GfLogDebug("cgrVtxTableTrackPart@%p(...)\n", this);
}

void cgrVtxTableTrackPart::draw ()
{
	if (!preDraw())
		return;

	if (hasState())
		getState()->apply();

#ifdef _SSG_USE_DLIST
	if (dlist)
		glCallList(dlist);
	else
#endif

	if(!_stripes)
	{
		if (_nTexMaps == 1)
			ssgVtxTable::draw_geometry ();
		else
			draw_geometry();
	}
	else
	{
		draw_geometry_array();
	}

	if (postDrawCB)
		(*postDrawCB)(this);
}

void cgrVtxTableTrackPart::draw_geometry ()
{
	TRACE_GL("cgrVtxTableTrackPart::draw_geometry: start");

	if (_mTexStates[0]) {
		_mTexStates[0]->apply(GL_TEXTURE1_ARB);
	}

	if (_mTexStates[1]) {
		_mTexStates[1]->apply(GL_TEXTURE2_ARB);
	}

	int num_colours = getNumColours ();
	int num_normals = getNumNormals ();
	int num_vertices = getNumVertices ();
	int numTexCoords = getNumTexCoords ();

	sgVec3 *vx = (sgVec3 *) getVertices()->get(0);
	sgVec3 *nm = (sgVec3 *) getNormals()->get(0);
	sgVec2 *tx = (sgVec2 *) getTexCoords()->get(0);
	sgVec2 *tx1 = (sgVec2 *)(_mTexStates[0] ? _mTexCoords[0]->get(0) : 0);
	sgVec2 *tx2 = (sgVec2 *)(_mTexStates[1] ? _mTexCoords[1]->get(0) : 0);
	sgVec4 *cl = (sgVec4 *) getColours()->get(0);

	glBegin (gltype);

	if (num_colours == 0) {
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	}

	if (num_colours == 1){
		glColor4fv(cl[0]);
	}

	if (num_normals == 1) {
		glNormal3fv(nm[0]);
	}

	for (int i = 0; i < num_vertices; i++) {
		if (num_normals > 1) {
			glNormal3fv(nm[i]);
		}

		if (numTexCoords > 1){
			glTexCoord2fv (tx [ i ]);
			glMultiTexCoord2fvARB(GL_TEXTURE0_ARB, tx[i]);
			if (_mTexStates[0]) {
				glMultiTexCoord2fvARB(GL_TEXTURE1_ARB, tx1[i]);
			}
			if (_mTexStates[1]) {
				glMultiTexCoord2fvARB(GL_TEXTURE2_ARB, tx2[i]);
			}
		}
		glVertex3fv(vx[i]);
	}
	
	glEnd ();

	if (_mTexStates[0]) {
		glActiveTextureARB(GL_TEXTURE1_ARB);
		glDisable(GL_TEXTURE_2D);
	}

	if (_mTexStates[1]) {
		glActiveTextureARB(GL_TEXTURE2_ARB);
		glDisable(GL_TEXTURE_2D);
	}

	if (grMaxTextureUnits > 1) {
		glActiveTextureARB(GL_TEXTURE0_ARB);
	}
	TRACE_GL("cgrVtxTableTrackPart::draw_geometry: end");
}

// cgrVtxTableCarPart class ================================================

// TODO: For car parts, _mStates[*] are not used, replaced by grEnvXXStateYY.
//       => this should be unified, through grStateFactory, when it will really manage
//          states, and be enhanced up to be able to replace grutils state functions.
//       That way, here we could use _mStates[*], and grloadac.cpp could normally
//       setup these states through grStateFactory, the only difference with the track case
//       being that the texture file names would not be read from the .acc, but "hard-coded"
//       (see grbackground.cpp, when it loads grEnvState, grEnvShadowState
//        and grEnvShadowStateOnCars).

cgrVtxTableCarPart::cgrVtxTableCarPart ()
: cgrVtxTable(), _carIndex(0)
{
	//GfLogDebug("cgrVtxTableCarPart@%p()\n", this);
}

cgrVtxTableCarPart::cgrVtxTableCarPart(int nTexMaps, int carIndex,
									   GLenum ty, ssgVertexArray   *vl,
									   ssgNormalArray   *nl,
									   ssgTexCoordArray *tl,
									   ssgTexCoordArray *tl1, ssgTexCoordArray *tl2, ssgTexCoordArray *tl3,
									   ssgColourArray *cl,
									   ssgIndexArray* stripeIndex, int numstripes, ssgIndexArray *il)
: cgrVtxTable(nTexMaps, ty, vl, nl, tl, tl1, tl2, tl3, cl, stripeIndex, numstripes, il),
  _carIndex(carIndex)

{
	//GfLogDebug("cgrVtxTableCarPart@%p(car=%d ...)\n", this, carIndex);
}

void cgrVtxTableCarPart::draw ()
{
	if (!preDraw())
		return;

	if (hasState())
		getState()->apply();

#ifdef _SSG_USE_DLIST
	if (dlist)
		glCallList(dlist);
	else
#endif

	if(!_stripes)
	{
		if (_nTexMaps == 1)
			ssgVtxTable::draw_geometry ();
		else
			draw_geometry();
	}
	else
	{
		if (_nTexMaps == 1)
			cgrVtxTable::draw_geometry_array();
		else
			draw_geometry_array();
	}

	if (postDrawCB)
		(*postDrawCB)(this);
}

void cgrVtxTableCarPart::draw_geometry ()
{
	tdble ttx = 0;
	tdble tty = 0;
	tdble ttz = 0;
	sgMat4 mat;
	sgVec3 axis;

	TRACE_GL("cgrVtxTableCarPart::draw_geometry");

	if (_nTexMaps > 2 && grEnvShadowState) {
		/* UP Vector for OpenGl */
		axis[0] = 0;
		axis[1] = 0;
		axis[2] = 1;

		glActiveTextureARB(GL_TEXTURE2_ARB);
		sgMakeRotMat4(mat, grCarInfo[_carIndex].envAngle, axis);
		glMatrixMode(GL_TEXTURE);
		glLoadIdentity();
		glMultMatrixf((float *)mat);
		glMatrixMode(GL_MODELVIEW);
		grEnvShadowState->apply(GL_TEXTURE2_ARB);
	}

	if (_nTexMaps > 1 && grEnvState) {
		grEnvState->apply(GL_TEXTURE1_ARB);
	
		glActiveTextureARB(GL_TEXTURE1_ARB);
		glEnable(GL_TEXTURE_2D);
		glMatrixMode(GL_TEXTURE);
		glLoadIdentity();
		ttx = grCarInfo[_carIndex].distFromStart/100;
		sgMakeTransMat4(mat, ttx, tty, ttz);
		glMultMatrixf((float *)mat);
		glMatrixMode(GL_MODELVIEW);
	}

	int num_colours = getNumColours();
	int num_normals = getNumNormals();
	int num_vertices = getNumVertices();
	int numTexCoords = getNumTexCoords();

	sgVec3 *vx = (sgVec3 *) getVertices()->get(0);
	sgVec3 *nm = (sgVec3 *) getNormals()->get(0);
	sgVec2 *tx = (sgVec2 *) getTexCoords()->get(0);
	sgVec2 *tx1 = (sgVec2 *)(grEnvState ? _mTexCoords[0]->get(0) : 0);
	sgVec2 *tx2 = (sgVec2 *)(grEnvShadowState ? _mTexCoords[1]->get(0) : 0);
	sgVec4 *cl = (sgVec4 *) getColours()->get(0);

	glBegin(gltype);

	if (num_colours == 0) {
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	}

	if (num_colours == 1) {
		glColor4fv(cl[0]);
	}

	if (num_normals == 1) {
		glNormal3fv(nm[0]);
	}

	for (int i = 0; i < num_vertices; i++)
	{
		if (num_normals > 1) {
			glNormal3fv(nm[i]);
		}

		if (numTexCoords > 1) {
			glMultiTexCoord2fvARB(GL_TEXTURE0_ARB, tx[i]);
			// #439 : Seel below : test tx2 in any case ...
			if (tx2 && _nTexMaps > 2) {
				glMultiTexCoord2fvARB(GL_TEXTURE2_ARB, tx2[i]);
			}
			// #439 : Don't know why, but sometimes (agl-kart), tx1=0
			if (tx1 && _nTexMaps > 1)
				glMultiTexCoord2fvARB(GL_TEXTURE1_ARB, tx1[i]);
		}
		glVertex3fv(vx[i]);
	}

	glEnd ();

	if (_nTexMaps > 1 && grEnvState) {
		glActiveTextureARB(GL_TEXTURE1_ARB);
		glDisable(GL_TEXTURE_2D);
		glMatrixMode(GL_TEXTURE);
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);
	}
	
	if (_nTexMaps > 2 && grEnvShadowState) {
		glActiveTextureARB(GL_TEXTURE2_ARB);
		glDisable(GL_TEXTURE_2D);
	}
	
	glActiveTextureARB(GL_TEXTURE0_ARB);

	TRACE_GL("cgrVtxTableCarPart::draw_geometry: end");
}

void cgrVtxTableCarPart::draw_geometry_array ()
{
	TRACE_GL("cgrVtxTableCarPart::draw_geometry_array: start");

	int num_colours = getNumColours();
	int num_normals = getNumNormals();
	int numTexCoords = getNumTexCoords();
	tdble ttx = 0;
	tdble tty = 0;
	tdble ttz = 0;
	sgMat4 mat;
	sgMat4 mat2;
	sgMat4 mat4;
	sgVec3 axis;

	sgVec3 *nm = (sgVec3 *) getNormals()->get(0);
	sgVec4 *cl = (sgVec4 *) getColours()->get(0);

	if (_nTexMaps > 2 && grEnvShadowState) {
		/* UP Vector for OpenGl */
		axis[0] = 0;
		axis[1] = 0;
		axis[2] = 1;

		glActiveTextureARB(GL_TEXTURE2_ARB);
		sgMakeRotMat4(mat, grCarInfo[_carIndex].envAngle, axis);
		glMatrixMode(GL_TEXTURE);
		glLoadIdentity();
		glMultMatrixf((float *)mat);
		glMatrixMode(GL_MODELVIEW);
		grEnvShadowState->apply(GL_TEXTURE2_ARB);
	}

	if (_nTexMaps > 3 && grEnvShadowStateOnCars) {
		tdble xxx = (grCarInfo[_carIndex].px-shad_xmin)/(shad_xmax-shad_xmin);
		tdble yyy = (grCarInfo[_carIndex].py-shad_ymin)/(shad_ymax-shad_ymin);

		/* UP Vector for OpenGl */
		axis[0]=0;
		axis[1]=0;
		axis[2]=1;

		mat2[0][0] = grCarInfo[_carIndex].sx;
		mat2[0][1] = 0;
		mat2[0][2] = 0;
		mat2[0][3] = 0 ;

		mat2[1][0] = 0;
		mat2[1][1] = grCarInfo[_carIndex].sy;
		mat2[1][2] = 0;
		mat2[1][3] = 0 ;

		mat2[2][0] = 0;
		mat2[2][1] = 0;
		mat2[2][2] = 1;
		mat2[2][3] = 0 ;


		mat2[3][0] = 0;
		mat2[3][1] = 0;
		mat2[3][2] = 0;
		mat2[3][3] = 1;


		glActiveTextureARB(GL_TEXTURE3_ARB);
		sgMakeRotMat4(mat, grCarInfo[_carIndex].envAngle, axis);
		glMatrixMode(GL_TEXTURE);
		glLoadIdentity();

		sgMakeTransMat4(mat4, xxx, yyy, 0);
		glMultMatrixf((float *)mat4);

		glMultMatrixf((float *)mat);
		glMultMatrixf((float *)mat2);

		glMatrixMode(GL_MODELVIEW);
		grEnvShadowStateOnCars->apply(GL_TEXTURE3_ARB);
	}

	if (_nTexMaps > 1 && grEnvState) {
		grEnvState->apply(GL_TEXTURE1_ARB);
		
		glActiveTextureARB(GL_TEXTURE1_ARB);
		glEnable(GL_TEXTURE_2D);
		glMatrixMode(GL_TEXTURE);
		glLoadIdentity();

		ttx = grCarInfo[_carIndex].distFromStart/100;
		sgMakeTransMat4(mat, ttx, tty, ttz);
		glMultMatrixf((float *)mat);
		glMatrixMode(GL_MODELVIEW);
	}

	glActiveTextureARB(GL_TEXTURE0_ARB);
	glEnable(GL_TEXTURE_2D);

	if (num_colours == 0) {
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	}

	if (num_colours == 1) {
		glColor4fv(cl[0]);
	}

	if (num_normals == 1) {
		glNormal3fv(nm[0]);
	}

	glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);

	if (num_normals > 1) {
		glEnableClientState(GL_NORMAL_ARRAY);
		glNormalPointer(GL_FLOAT, 0, getNormals()->get(0));
	}

	if (numTexCoords > 1) {
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, 0, getTexCoords()->get(0));

		if (_nTexMaps > 1 && grEnvState) {
			glClientActiveTextureARB(GL_TEXTURE1_ARB);
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			glTexCoordPointer(2, GL_FLOAT, 0, _mTexCoords[0]->get(0));
		}

		if (_nTexMaps > 2 && grEnvShadowState) {
			glClientActiveTextureARB(GL_TEXTURE2_ARB);
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			glTexCoordPointer(2, GL_FLOAT, 0, _mTexCoords[1]->get(0));
		}

		if (_nTexMaps > 3 && grEnvShadowStateOnCars) {
			glClientActiveTextureARB (GL_TEXTURE3_ARB);
			glEnableClientState (GL_TEXTURE_COORD_ARRAY);
			glTexCoordPointer (2, GL_FLOAT, 0, _mTexCoords[2]->get(0));
		}
	}

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, getVertices()->get(0));

	glClientActiveTextureARB (GL_TEXTURE0_ARB);
	glEnableClientState (GL_TEXTURE_COORD_ARRAY);

	int i = 0;
	short *ii = NULL;
	int j = 0;
	int p = 0;

	for (j = 0; j < _numStripes; j++) {
		i = (short)*(_stripes->get(j));
		ii = _indices->get(p);
		glDrawElements(gltype, i, GL_UNSIGNED_SHORT, ii);
		p += i;
	}

	glPopClientAttrib();
	
	glActiveTextureARB(GL_TEXTURE1_ARB);
	glDisable (GL_TEXTURE_2D);
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);

	if (_nTexMaps > 2 && grEnvShadowState) {
		glActiveTextureARB(GL_TEXTURE2_ARB);
		glMatrixMode(GL_TEXTURE);
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);
		glDisable(GL_TEXTURE_2D);
	}

	if (_nTexMaps > 3 && grEnvShadowStateOnCars) {
		glActiveTextureARB(GL_TEXTURE3_ARB);
		glMatrixMode(GL_TEXTURE);
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);
		glDisable(GL_TEXTURE_2D);
	}

	glActiveTextureARB (GL_TEXTURE0_ARB);
	
	TRACE_GL("cgrVtxTableCarPart::draw_geometry_array");
}

void cgrVtxTableCarPart::copy_from (cgrVtxTableCarPart *src, int clone_flags)
{
	cgrVtxTable::copy_from (src, clone_flags);

	_carIndex = src->_carIndex;
}

ssgBase *cgrVtxTableCarPart::clone (int clone_flags)
{
	cgrVtxTableCarPart *b = new cgrVtxTableCarPart;
	b->copy_from(this, clone_flags);
	return b;
}
