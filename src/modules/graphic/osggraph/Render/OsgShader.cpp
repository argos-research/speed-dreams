/***************************************************************************

    file        : OsgShader.cpp
    created     : Sat Feb 2013 15:52:19 CEST 2013
    copyright   : (C) 2013 by Gaëtan André
    email       : gaetan.andre@gmail.com
    version     : $Id$
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "OsgRender.h"

#include <osg/Program>
#include <osg/Node>
#include <osg/Uniform>
#include <tgf.h>

#include "OsgMain.h"
#include "OsgShader.h"
#include "OsgCar.h"
#include "OsgSky.h"

SDCarShader::SDCarShader(osg::Node *car, SDCar *c)
{
    std::string TmpPath = GetDataDir();
    osg::ref_ptr<osg::Shader> vertShader = new osg::Shader( osg::Shader::VERTEX);
    osg::ref_ptr<osg::Shader> fragShader = new osg::Shader( osg::Shader::FRAGMENT);
    vertShader->loadShaderSourceFromFile(TmpPath+"data/shaders/car.vert");
    fragShader->loadShaderSourceFromFile(TmpPath+"data/shaders/car.frag");
    program = new osg::Program;
    program->addShader( vertShader.get() );
    program->addShader( fragShader.get() );

    //pCar= dynamic_cast<osg::Group *> (car);
    pCar = car;
    this->pSdCar = c;
    stateset = pCar->getOrCreateStateSet();
    stateset->setAttributeAndModes(program);

    diffuseMap = new osg::Uniform("diffusemap", 0 );
    stateset->addUniform(diffuseMap);

    specularColor = new osg::Uniform("specularColor", osg::Vec4(0.8f, 0.8f, 0.8f, 1.0f));
    stateset->addUniform(specularColor);

    lightVector = stateset->getOrCreateUniform("lightvector",osg::Uniform::FLOAT_VEC3);
    lightPower = stateset->getOrCreateUniform("lightpower",osg::Uniform::FLOAT_VEC4);
    ambientColor = stateset->getOrCreateUniform("ambientColor",osg::Uniform::FLOAT_VEC4);
    shininess = new osg::Uniform("smoothness", 300.0f);
    stateset->addUniform(shininess);

    reflectionMappingMethod = new osg::Uniform("reflectionMappingMethod", this->pSdCar->getReflectionMappingMethod());
    reflectionMapCube = new osg::Uniform("reflectionMapCube", 2 );
    reflectionMap2DSampler = new osg::Uniform("reflectionMap2DSampler", 2 );
    reflectionMapStaticOffsetCoords = stateset->getOrCreateUniform("reflectionMapStaticOffsetCoords", osg::Uniform::FLOAT_VEC3);

    stateset->addUniform(reflectionMappingMethod);
    stateset->addUniform(reflectionMap2DSampler);
    stateset->addUniform(reflectionMapCube);
}

void SDCarShader::update(osg::Matrixf view)
{
    SDRender * ren = (SDRender *)getRender();
    osg::Vec3f sun_pos= ren->getSky()->getSun()->getSunPosition();
    osg::Vec4f sun_color = ren->getSky()->get_sun_color();
    osg::Vec4f scene_color = ren->getSceneColor();

    osg::Vec4f lv = osg::Vec4f(sun_pos.x(),sun_pos.y(),sun_pos.z(),0.0f);
    lv = lv*view;

    lightVector->set(osg::Vec3f(lv.x(),lv.y(),lv.z()));
    lightPower->set(sun_color);
    ambientColor->set(scene_color);
}
