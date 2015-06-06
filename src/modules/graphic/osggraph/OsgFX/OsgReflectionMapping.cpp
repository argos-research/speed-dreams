/***************************************************************************

    file        : OsgScreens.h
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
 ****************************************************************************/

#include <osg/Group>
#include <osg/Texture2D>
#include <osg/Camera>
#include <osgViewer/Viewer>

#include "OsgView/OsgScreens.h"



#include "OsgCar/OsgCar.h"
#include "OsgRender.h"
#include "OsgMain.h"


#include "OsgReflectionMapping.h"

#include <car.h>



class CameraPreCallback : public osg::Camera::DrawCallback
{
private:
    tCarElt * car;
public:

    void setCar(tCarElt *c){
        car = c;
    }

   virtual void operator()(const osg::Camera& cam) const
   {

        SDCars * cars = (SDCars *)getCars();

        cars->deactivateCar(car);
   }
};

class CameraPostCallback : public osg::Camera::DrawCallback
{
private:
    tCarElt * car;
public:

    void setCar(tCarElt *c){
        car = c;
    }



   virtual void operator()(const osg::Camera& cam) const
   {

        SDCars * cars = (SDCars *)getCars();

        cars->activateCar(car);
   }
};

CameraPreCallback * pre_cam = new CameraPreCallback;
CameraPostCallback * post_cam = new CameraPostCallback;


SDReflectionMapping::SDReflectionMapping(SDCar *c):car(c){

    SDRender * render = (SDRender *)getRender();
    osg::ref_ptr<osg::Node> m_sceneroot =  render->getRoot();


    osg::ref_ptr<osg::TextureCubeMap> reflectionMap;

    reflectionMap = new osg::TextureCubeMap;
    this->reflectionMap =reflectionMap;
    reflectionMap->setTextureSize( 256, 256 );
    reflectionMap->setInternalFormat( GL_RGB);
    reflectionMap->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
    reflectionMap->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
    reflectionMap->setWrap(osg::Texture::WRAP_R, osg::Texture::CLAMP_TO_EDGE);
    reflectionMap->setFilter(osg::TextureCubeMap::MIN_FILTER,osg::TextureCubeMap::LINEAR);
    reflectionMap->setFilter(osg::TextureCubeMap::MAG_FILTER,osg::TextureCubeMap::LINEAR);

    camerasRoot = new osg::Group;

    for(int i=0;i<6;i++){


        osg::ref_ptr<osg::Camera> camera = new osg::Camera;
        camera->setViewport( 0, 0, 256, 256 );
        camera->setClearMask( GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT );
        //camera->setClearColor(osg::Vec4(0.0,0.0,0.0,1.0));

        camera->setRenderOrder( osg::Camera::PRE_RENDER );
        camera->setRenderTargetImplementation(
        osg::Camera::FRAME_BUFFER_OBJECT );
        camera->attach( osg::Camera::COLOR_BUFFER, reflectionMap,0,i );

        camera->setReferenceFrame( osg::Camera::ABSOLUTE_RF );
        camera->addChild( m_sceneroot );

      //  camera->setPreDrawCallback(pre_cam);
       //  camera->setPostDrawCallback(post_cam);


        //camera->setProjectionMatrixAsOrtho(,1,-1,1,0,1000000);

        camera->setProjectionMatrixAsPerspective(90.0,1.0,1.0,1000000.0);
        //camera->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
        camerasRoot->addChild(camera);
        cameras.push_back(camera);
        //camera->setNodeMask(0);


  }



     //ACTIVATE to enable Environment MApping <= temporary hack.
    //SDScreens * screens = (SDScreens*)getScreens();
    //screens->registerViewDependantPreRenderNode(this->getCamerasRoot());

    //cameras[4]->setNodeMask(1);


}

void SDReflectionMapping::update(){

    //TODO support for multi screen

    SDScreens * screens = (SDScreens*)getScreens();
    osg::Camera * viewCam = screens->getActiveView()->getOsgCam();

    tCarElt * car = this->car->getCar();

    pre_cam->setCar(car);
    post_cam->setCar(car);

    sgVec3 p;
    osg::Vec3 eye,center,up;

    p[0] = car->_drvPos_x;
    p[1] = car->_bonnetPos_y;
    p[2] = car->_drvPos_z;
    sgXformPnt3(p, car->_posMat);

    eye[0] = p[0];
    eye[1] = p[1];
    eye[2] = p[2];


  /*  P[0] = car->_drvPos_x + 30.0 * cos(2*PI/3 * car->_glance + offset);
    P[1] = car->_bonnetPos_y - 30.0 * sin(2*PI/3 * car->_glance + offset);
    P[2] = car->_drvPos_z;
    sgXformPnt3(P, car->_posMat);

    center[0] =  P[0];
    center[1] = P[1];
    center[2] = P[2];

    up[0] = car->_posMat[2][0];
    up[1] = car->_posMat[2][1];
    up[2] = car->_posMat[2][2];*/


    osg::Matrix n = osg::Matrix(-1.0,0.0,0.0,0.0,
                               0.0,1.0,0.0,0.0,
                               0.0,0.0,1.0,0.0,
                               0.0,0.0,0.0,1.0);

    cameras[osg::TextureCubeMap::POSITIVE_Z]->setViewMatrix(osg::Matrix::translate(-eye)*osg::Matrix::rotate(viewCam->getViewMatrix().getRotate())*n);
    osg::Matrix mat = cameras[osg::TextureCubeMap::POSITIVE_Z]->getViewMatrix();

    osg::Matrix negX = osg::Matrix::rotate(osg::inDegrees(-90.0),osg::Y_AXIS);
    osg::Matrix negZ = osg::Matrix::rotate(osg::inDegrees(-180.0),osg::Y_AXIS);
    osg::Matrix posX = osg::Matrix::rotate(osg::inDegrees(90.0),osg::Y_AXIS);

    osg::Matrix negY= osg::Matrix::rotate(osg::inDegrees(-90.0),osg::X_AXIS);
    osg::Matrix posY= osg::Matrix::rotate(osg::inDegrees(90.0),osg::X_AXIS);

    cameras[osg::TextureCubeMap::NEGATIVE_X]->setViewMatrix(mat*negX);
    cameras[osg::TextureCubeMap::NEGATIVE_Z]->setViewMatrix(mat*negZ);
    cameras[osg::TextureCubeMap::POSITIVE_X]->setViewMatrix(mat*posX);

    cameras[osg::TextureCubeMap::NEGATIVE_Y]->setViewMatrix(mat*negY);
    cameras[osg::TextureCubeMap::POSITIVE_Y]->setViewMatrix(mat*posY);

    /*for(unsigned int i=0;i<cameras.size();i++){
        cameras[i]->setViewMatrixAsLookAt(eye,center,up);
    }*/
}

SDReflectionMapping::~SDReflectionMapping(){
}
