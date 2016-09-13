/***************************************************************************

    file                 : OsgMoon.cpp
    created              : Mon Aug 21 18:24:02 CEST 2012
    copyright            : (C)2012 by Xavier Bertaux
    email                : bertauxx@yahoo.fr
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

#include <stdio.h>
#include <iostream>

#include <osg/Array>
#include <osg/AlphaFunc>
#include <osg/BlendFunc>
#include <osg/CullFace>
#include <osg/Geometry>
#include <osg/Geode>
#include <osg/Node>
#include <osg/ShadeModel>
#include <osg/TexEnv>
#include <osg/Texture2D>
#include <osgDB/ReadFile>

#include "OsgMath.h"
#include "OsgColor.h"
#include "OsgSphere.h"
#include "OsgMoon.h"

// Constructor
SDMoon::SDMoon( void ) :
    prev_moon_angle(-1)
{
}

// Destructor
SDMoon::~SDMoon( void )
{
}

// build the moon object
osg::Node* SDMoon::build( std::string path, double dist, double size )
{
    std::string TmpPath = path;
    osg::Node* orb = SDMakeSphere(size, 15, 15);
    osg::StateSet* stateSet = orb->getOrCreateStateSet();
    stateSet->setRenderBinDetails(-5, "RenderBin");

    moon_size = size;
    moon_dist = dist;

    path = TmpPath+"data/sky/moon.png";
    osg::ref_ptr<osg::Image> image = osgDB::readImageFile(path);
    osg::ref_ptr<osg::Texture2D> texture = new osg::Texture2D(image.get());
    stateSet->setTextureAttributeAndModes(0, texture, osg::StateAttribute::ON);
    osg::TexEnv* texEnv = new osg::TexEnv;
    texEnv->setMode(osg::TexEnv::MODULATE);
    stateSet->setTextureAttribute(0, texEnv, osg::StateAttribute::ON);

    orb_material = new osg::Material;
    orb_material->setColorMode(osg::Material::DIFFUSE);
    orb_material->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4(1, 1, 1, 1));
    orb_material->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4(0, 0, 0, 1));
    orb_material->setEmission(osg::Material::FRONT_AND_BACK,osg::Vec4(0, 0, 0, 1));
    orb_material->setSpecular(osg::Material::FRONT_AND_BACK,osg::Vec4(0, 0, 0, 1));
    orb_material->setShininess(osg::Material::FRONT_AND_BACK, 0);
    stateSet->setAttribute(orb_material.get());
    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::ON);
    stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
    stateSet->setMode(GL_FOG, osg::StateAttribute::OFF);
    osg::ShadeModel* shadeModel = new osg::ShadeModel;
    shadeModel->setMode(osg::ShadeModel::SMOOTH);
    stateSet->setAttributeAndModes(shadeModel);
    osg::CullFace* cullFace = new osg::CullFace;
    cullFace->setMode(osg::CullFace::BACK);
    stateSet->setAttributeAndModes(cullFace);

    osg::BlendFunc* blendFunc = new osg::BlendFunc;
    blendFunc->setFunction(osg::BlendFunc::SRC_ALPHA, osg::BlendFunc::ONE);
    stateSet->setAttributeAndModes(blendFunc);

    osg::AlphaFunc* alphaFunc = new osg::AlphaFunc;
    alphaFunc->setFunction(osg::AlphaFunc::GREATER);
    alphaFunc->setReferenceValue(0.01);
    stateSet->setAttribute(alphaFunc);
    stateSet->setMode(GL_ALPHA_TEST, osg::StateAttribute::ON);

    repaint( 0.0 );

    moon_transform = new osg::MatrixTransform;
    moon_transform->addChild( orb );

    return moon_transform.get();
}

bool SDMoon::repaint( double moon_angle )
{
    if (prev_moon_angle == moon_angle)
        return true;

    prev_moon_angle = moon_angle;

    float moon_factor = 4*cos(moon_angle);

    if (moon_factor > 1) moon_factor = 1.0;
    if (moon_factor < -1) moon_factor = -1.0;
    moon_factor = (moon_factor/2) + 0.5f;

    osg::Vec4 color;
    color[1] = sqrt(moon_factor);
    color[0] = sqrt(color[1]);
    color[2] = moon_factor * moon_factor;
    color[2] *= color[2];
    color[3] = 1.0;

    sd_gamma_correct_rgb( color._v );

    orb_material->setDiffuse(osg::Material::FRONT_AND_BACK, color);

    return true;
}

bool SDMoon::reposition(osg::Vec3d p, double angle )
{
    osg::Matrix T1, T2, GST, RA, DEC;

    T1.makeTranslate(p);
    GST.makeRotate((float)(angle), osg::Vec3(0.0, 0.0, -1.0));
    RA.makeRotate(moonAscension - 90.0 * SD_DEGREES_TO_RADIANS, osg::Vec3(0, 0, 1));
    DEC.makeRotate(moondeclination, osg::Vec3(1, 0, 0));
    T2.makeTranslate(osg::Vec3(0, moon_dist, 0));

    osg::Matrix R = T2*T1*GST*DEC*RA;
    moon_transform->setMatrix(R);

    osg::Vec4f pos = osg::Vec4f(0.0,0.0,0.0,1.0)*R;
    moon_position = osg::Vec3f(pos._v[0],pos._v[1],pos._v[2]);

    return true;
}
