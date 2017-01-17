/***************************************************************************

    file                 : OsgBrake.cpp
    created              : Mon Dec 31 10:24:02 CEST 2012
    copyright            : (C) 2012 by Gaëtan André
    email                : gaetan.andre@gmail.com
    version              : $Id$

***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <osg/Geometry>
#include <osg/CullFace>
#include <osg/Material>
#include <osg/ShadeModel>

#include <portability.h>

#include "OsgBrake.h"
#include "OsgMath.h"

void SDBrakes::setCar(tCarElt * car)
{
    this->car = car;
}

osg::Node *SDBrakes::initBrake(int wheelIndex)
{
    float	alpha;
    osg::Vec3 vtx;
    osg::Vec4 clr;
    osg::Vec3 nrm;
    osg::Vec2 tex;
    float	b_offset = 0;
    tdble	curAngle = 0.0;
    int i;

    osg::Geode *pBrake = new osg::Geode;

    pBrake->setName("Brake Assembly");
	pBrake->setCullingActive(false);

	osg::StateSet* stateSet = pBrake->getOrCreateStateSet();

    osg::ShadeModel* shadeModel = new osg::ShadeModel;
    shadeModel->setMode(osg::ShadeModel::SMOOTH);
    stateSet->setAttributeAndModes(shadeModel);
    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    stateSet->setMode(GL_FOG, osg::StateAttribute::ON);
    stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);
    stateSet->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
    stateSet->setMode(GL_BLEND, osg::StateAttribute::OFF);
    stateSet->setMode(GL_ALPHA_TEST, osg::StateAttribute::OFF);

    osg::Material* material = new osg::Material;
    stateSet->setAttribute(material);

    int brakeBranch = 32;
    float brakeAngle = 2.0 * SD_PI / (tdble)brakeBranch;
    float brakeOffset = 0.1;

    switch(wheelIndex)
    {
    case FRNT_RGT:
        curAngle = -(SD_PI / 2.0 + brakeAngle);
        b_offset = brakeOffset - car->_tireWidth(wheelIndex) / 2.0;
        break;
    case FRNT_LFT:
        curAngle = -(SD_PI / 2.0 + brakeAngle);
        b_offset = car->_tireWidth(wheelIndex) / 2.0 - brakeOffset;
        break;
    case REAR_RGT:
        curAngle = (SD_PI / 2.0 - brakeAngle);
        b_offset = brakeOffset - car->_tireWidth(wheelIndex) / 2.0;
        break;
    case REAR_LFT:
        curAngle = (SD_PI / 2.0 - brakeAngle);
        b_offset = car->_tireWidth(wheelIndex) / 2.0 - brakeOffset;
        break;
    }

    /* hub */
    osg::Vec3Array *hub_vtx = new osg::Vec3Array();
    osg::Vec4Array *hub_clr = new osg::Vec4Array();
    osg::Vec3Array *hub_nrm = new osg::Vec3Array();

    tdble hubRadius;

    /* center */
    vtx[0] = vtx[2] = 0.0;
    vtx[1] = b_offset;
    hub_vtx->push_back(vtx);

    hubRadius = car->_brakeDiskRadius(wheelIndex) * 0.6;
    for (i = 0; i < brakeBranch; i++)
    {
        alpha = (float)i * 2.0 * SD_PI / (float)(brakeBranch - 1);
        vtx[0] = hubRadius * cos(alpha);
        vtx[1] = b_offset;
        vtx[2] = hubRadius * sin(alpha);
        hub_vtx->push_back(vtx);
    }

    clr[0] = clr[1] = clr[2] = 0.0;
    clr[3] = 1.0;
    hub_clr->push_back(clr);
    nrm[0] = nrm[2] = 0.0;

    // Make normal point outside to have proper lighting.
    switch(wheelIndex)
    {
    case FRNT_RGT:
    case REAR_RGT:
        nrm[1] = -1.0;
        break;
    case FRNT_LFT:
    case REAR_LFT:
        nrm[1] = 1.0;
        break;
    }

    hub_nrm->push_back(nrm);

    osg::Geometry *pHub = new osg::Geometry;
    pHub->setName("Hub");
    pHub->addPrimitiveSet( new osg::DrawArrays(osg::PrimitiveSet::TRIANGLE_FAN, 0, brakeBranch+1 ));

    pHub->setVertexArray(hub_vtx);
    pHub->setColorArray(hub_clr);
    pHub->setNormalArray(hub_nrm);

    pHub->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    pHub->setNormalBinding(osg::Geometry::BIND_OFF);

    pBrake->addDrawable(pHub);

    /* Brake disk */
    osg::Vec3Array *brk_vtx = new osg::Vec3Array();
    brk_clr = new osg::Vec4Array();
    osg::Vec3Array *brk_nrm = new osg::Vec3Array();

    for (i = 0; i < (brakeBranch / 2 + 2); i++)
    {
        alpha = curAngle + (float)i * 2.0 * M_PI / (float)(brakeBranch - 1);
        vtx[0] = car->_brakeDiskRadius(wheelIndex) * cos(alpha);
        vtx[1] = b_offset;
        vtx[2] = car->_brakeDiskRadius(wheelIndex) * sin(alpha);
        brk_vtx->push_back(vtx);
        vtx[0] = car->_brakeDiskRadius(wheelIndex) * cos(alpha) * 0.6;
        vtx[1] = b_offset;
        vtx[2] = car->_brakeDiskRadius(wheelIndex) * sin(alpha) * 0.6;
        brk_vtx->push_back(vtx);
    }

    clr[0] = clr[1] = clr[2] = 0.1;
    clr[3] = 1.0;
    brk_clr->push_back(clr);
    brk_nrm->push_back(nrm);

    osg::Geometry *pBDisc = new osg::Geometry;
    pBDisc->setName("Disc Brake");

    pBDisc->addPrimitiveSet( new osg::DrawArrays(osg::PrimitiveSet::TRIANGLE_STRIP, 0, brakeBranch+4 ));

    pBDisc->setVertexArray(brk_vtx);
    pBDisc->setColorArray(brk_clr.get());
    pBDisc->setNormalArray(brk_nrm);

    pBDisc->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    pBDisc->setNormalBinding(osg::Geometry::BIND_OFF);

    pBrake->addDrawable(pBDisc);

    this->brake_disks[wheelIndex] = pBDisc;

    /* Brake caliper */
    osg::Vec3Array *cal_vtx = new osg::Vec3Array();
    osg::Vec4Array *cal_clr = new osg::Vec4Array();
    osg::Vec3Array *cal_nrm = new osg::Vec3Array();

    for (i = 0; i < (brakeBranch / 2 - 2); i++)
    {
        alpha = - curAngle + (float)i * 2.0 * SD_PI / (float)(brakeBranch - 1);
        vtx[0] = (car->_brakeDiskRadius(wheelIndex) + 0.02) * cos(alpha);
        vtx[1] = b_offset;
        vtx[2] = (car->_brakeDiskRadius(wheelIndex) + 0.02) * sin(alpha);
        cal_vtx->push_back(vtx);
        vtx[0] = car->_brakeDiskRadius(wheelIndex) * cos(alpha) * 0.6;
        vtx[1] = b_offset;
        vtx[2] = car->_brakeDiskRadius(wheelIndex) * sin(alpha) * 0.6;
        cal_vtx->push_back(vtx);
    }

    clr[0] = 0.2;
    clr[1] = 0.2;
    clr[2] = 0.2;
    clr[3] = 1.0;
    cal_clr->push_back(clr);
    cal_nrm->push_back(nrm);

    osg::Geometry *pCaliper = new osg::Geometry;
    pCaliper->setName("Caliper");
    pCaliper->addPrimitiveSet( new osg::DrawArrays(osg::PrimitiveSet::TRIANGLE_STRIP, 0, brakeBranch-4 ));

    pCaliper->setVertexArray(cal_vtx);
    pCaliper->setColorArray(cal_clr);
    pCaliper->setNormalArray(cal_nrm);

    pCaliper->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    pCaliper->setNormalBinding(osg::Geometry::BIND_OFF);


    pBrake->addDrawable(pCaliper);

    return pBrake;
}

void SDBrakes::updateBrakes()
{ // TODO clean unused memory ... free pointers if needed
    for(int i=0; i<4; i++)
    {
        osg::Vec4 clr;

        clr[0] = 0.1 + car->_brakeTemp(i) * 1.5;
        clr[1] = 0.1 + car->_brakeTemp(i) * 0.3;
        clr[2] = 0.1 - car->_brakeTemp(i) * 0.3;
        clr[3] = 1.0;
        (*brk_clr)[0] = clr;
        brk_clr->dirty();

        this->brake_disks[i]->setColorArray(brk_clr.get(), osg::Array::BIND_PER_VERTEX);
		//GfLogInfo("brake temp %i = %f\n", i, car->_brakeTemp(i) );
    }
}
