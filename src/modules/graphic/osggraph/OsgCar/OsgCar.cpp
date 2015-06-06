/***************************************************************************

    file                 : OsgCar.cpp
    created              : Mon Aug 21 18:24:02 CEST 2012
    copyright            : (C) 2012 by Gaetan André
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

#include <osg/MatrixTransform>
#include <osg/Switch>
#include <osg/Group>
#include <osgViewer/Viewer>
#include <osg/Program>
#include <osg/Geode>
#include <osg/Geometry>
#include <portability.h>
#include <osg/Texture2D>
#include <osg/BlendFunc>
#include <osg/Depth>
#include <osgDB/ReadFile>

#include <robottools.h>

#include "OsgLoader/OsgLoader.h"
#include "OsgCar.h"
#include "OsgUtil/OsgMath.h"
#include "OsgView/OsgScreens.h"
#include "OsgFX/OsgRender.h"
#include "OsgMain.h"
#include "OsgSky/OsgSky.h"


#include "OsgFX/OsgShader.h"
#include "OsgFX/OsgReflectionMapping.h"





osg::Node *SDCar::loadCar(tCarElt *car)
{
    this->car = car;
    static const int nMaxTexPathSize = 4096;
    char buf[nMaxTexPathSize];
    int index;
    void *handle;
    //const char *param;,car_branch->getMatrix()
    int lg =0;
    char path[256];
    
    osgLoader loader;
    std::string TmpPath = GetDataDir();
    std::string strTPath;
    
    strncpy(car->_masterModel, GfParmGetStr(car->_carHandle, SECT_GROBJECTS, PRM_TEMPLATE, ""), MAX_NAME_LEN - 1);
    car->_masterModel[MAX_NAME_LEN - 1] = 0;

    index = car->index;	/* current car's index */
    handle = car->_carHandle;

    /* Initialize board */
    /* Schedule texture mapping if we are using a custom skin and/or a master 3D model */
    const bool bMasterModel = strlen(car->_masterModel) != 0;

    GfOut("[gr] Init(%d) car %s for driver %s index %d\n", index, car->_carName, car->_modName, car->_driverIndex);
    GfOut("[gr] Init(%d) car %s MasterModel name\n", index, car->_masterModel);

    snprintf(buf, nMaxTexPathSize, "%sdrivers/%s/%d/",
             GfLocalDir(), car->_modName, car->_driverIndex);
    strTPath = TmpPath+buf;
    loader.AddSearchPath(strTPath);

    snprintf(buf, nMaxTexPathSize, "%sdrivers/%s/%s/",
             GfLocalDir(), car->_modName, car->_carName);
    strTPath = TmpPath+buf;
    loader.AddSearchPath(strTPath);
    
    if (bMasterModel)
    {
        snprintf(buf, nMaxTexPathSize, "%sdrivers/%s/%s/",
                 GfLocalDir(), car->_modName, car->_masterModel);
        strTPath = TmpPath+buf;
        loader.AddSearchPath(strTPath);
    }

    snprintf(buf, nMaxTexPathSize, "%sdrivers/%s/",
             GfLocalDir(), car->_modName);
    strTPath = TmpPath+buf;
    loader.AddSearchPath(strTPath);

    snprintf(buf, nMaxTexPathSize, "drivers/%s/%d/%s/",
             car->_modName, car->_driverIndex, car->_carName);
    strTPath = TmpPath+buf;
    loader.AddSearchPath(strTPath);
    
    if (bMasterModel)
    {
        lg += snprintf(buf, nMaxTexPathSize, "drivers/%s/%d/%s/",
                       car->_modName, car->_driverIndex, car->_masterModel);
        strTPath = TmpPath+buf;
        loader.AddSearchPath(strTPath);
    }
    
    snprintf(buf, nMaxTexPathSize, "drivers/%s/%d/",
             car->_modName, car->_driverIndex);
    strTPath = TmpPath+buf;
    loader.AddSearchPath(strTPath);

    snprintf(buf, nMaxTexPathSize, "drivers/%s/%s/",
             car->_modName, car->_carName);
    strTPath = TmpPath+buf;
    loader.AddSearchPath(strTPath);
    
    if (bMasterModel)
    {
        snprintf(buf, nMaxTexPathSize, "drivers/%s/%s/",
                 car->_modName, car->_masterModel);
        strTPath = TmpPath+buf;
        loader.AddSearchPath(strTPath);
    }

    snprintf(buf, nMaxTexPathSize, "drivers/%s/", car->_modName);
    strTPath = TmpPath+buf;
    loader.AddSearchPath(strTPath);

    snprintf(buf, nMaxTexPathSize, "cars/models/%s/", car->_carName);
    strTPath = TmpPath+buf;
    loader.AddSearchPath(strTPath);
    
    if (bMasterModel)
    {
        snprintf(buf, nMaxTexPathSize, "cars/models/%s/", car->_masterModel);
        strTPath = TmpPath+buf;
        loader.AddSearchPath(strTPath);
    }

    snprintf(buf, nMaxTexPathSize, "data/objects/");
    strTPath = TmpPath+buf;
    loader.AddSearchPath(strTPath);

    snprintf(buf, nMaxTexPathSize, "data/textures/");
    strTPath = TmpPath+buf;
    loader.AddSearchPath(strTPath);

    /* loading raw car level 0*/
    //  selIndex = 0; 	/* current selector index */
    snprintf(buf, nMaxTexPathSize, "%s.ac",
             bMasterModel ? car->_masterModel : car->_carName); /* default car 3D model file */
    snprintf(path, 256, "%s/%s/1", SECT_GROBJECTS, LST_RANGES);
    // param = GfParmGetStr(handle, path, PRM_CAR, buf);

    std::string strPath = GetDataDir();

    if (bMasterModel)
        sprintf(buf, "cars/models/%s/%s.acc", car->_masterModel, car->_masterModel);
    else
        sprintf(buf, "cars/models/%s/%s.acc", car->_carName, car->_carName);

    strPath+=buf;

    std::string name = car->_carName;

    osg::ref_ptr<osg::Node> pCar = new osg::Node;

    if (name != "ref-sphere-osg")
    {
        strPath+=buf;
        GfOut("Chemin Textures : %s\n", strTPath.c_str());
        pCar = loader.Load3dFile(strPath, true);
        GfOut("Load Car ACC !\n");
    }
    else
    {
        //strPath+=buf;
        osg::ref_ptr<osgDB::Options> options = new osgDB::Options();
        options->getDatabasePathList().push_back(strPath);
        std::string strTPath = GetDataDir();
        snprintf(buf, 4096, "data/textures/");
        strTPath += buf;
        options->getDatabasePathList().push_back(strTPath);
        snprintf(buf, 4096, "cars/models/ref-sphere-osg/");
        strTPath = GetDataDir();
        strTPath +=buf;
        options->getDatabasePathList().push_back(strTPath);
       // GfOut("Load Car OSG !\n");
        pCar = osgDB::readNodeFile("ref-sphere-osg.osg", options);
    }


    /*strPath+=buf;
    GfOut("Chemin Textures : %s\n", strTPath.c_str());
    osg::ref_ptr<osg::Node> pCar = loader.Load3dFile(strPath, true);*/

    osg::ref_ptr<osg::MatrixTransform> transform1 = new osg::MatrixTransform;
    transform1->addChild(pCar);


   // GfOut("loaded car %d",pCar.get());
    this->car_branch = transform1.get();

    this->car_branch->addChild(wheels.initWheels(car,handle));
    /*this->car_branch->getOrCreateStateSet()->setMode
            ( GL_DEPTH_TEST, osg::StateAttribute::PROTECTED|osg::StateAttribute::ON );*/


    this->car_root = new osg::Group;
    car_root->addChild(car_branch);
    if (SHADOW_TECHNIQUE == 0)
        this->car_root->addChild(this->initOcclusionQuad(car));

   // car_root->setNodeMask(1);


    this->shader = new SDCarShader(pCar.get(),this);


    this->reflectionMappingMethod = REFLECTIONMAPPING_OFF;


    this->reflectionMapping = new SDReflectionMapping(this);
  //  this->setReflectionMap(reflectionMapping->getReflectionMap());

    return this->car_root;
}

bool SDCar::isCar(tCarElt*c){
    return c==car;
}
SDReflectionMapping * SDCar::getReflectionMap(){
    return reflectionMapping;
}

int SDCar::getReflectionMappingMethod(){
    return reflectionMappingMethod;
}

tCarElt * SDCar::getCar(){
    return car;
}

#define GR_SHADOW_POINTS 6
#define MULT 1.1
osg::ref_ptr<osg::Node> SDCar::initOcclusionQuad(tCarElt *car){

    osg::Vec3f vtx;
    osg::Vec2f tex;
    float x;
    int i;

    char buf[512];
    std::string TmpPath = GetDataDir();

    //  GfOut("\n################## LOADING SHADOW ###############################\n");

    std::string shadowTextureName = GfParmGetStr(car->_carHandle, SECT_GROBJECTS, PRM_SHADOW_TEXTURE, "");

    snprintf(buf, sizeof(buf), "cars/models/%s/", car->_carName);
    if (strlen(car->_masterModel) > 0) // Add the master model path if we are using a template.
        snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "cars/models/%s/", car->_masterModel);

    std::string dir = buf;
    shadowTextureName = TmpPath +dir+shadowTextureName;

   // GfOut("\n lepath = %s\n",shadowTextureName.c_str());

    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec2Array> texcoords = new osg::Vec2Array;
    vtx._v[2] = 0.0;
    for (i = 0, x = car->_dimension_x * MULT / 2.0; i < GR_SHADOW_POINTS / 2;
         i++, x -= car->_dimension_x * MULT / (float)(GR_SHADOW_POINTS - 2) * 2.0) {

        vtx._v[0] = x;
        tex._v[0] = 1.0 - (float)i / (float)((GR_SHADOW_POINTS - 2) / 2.0);

        vtx._v[1] = -car->_dimension_y * MULT / 2.0;
        vertices->push_back(vtx);
        tex._v[1] = 0.0;
        texcoords->push_back(tex);

        vtx._v[1] = car->_dimension_y * MULT / 2.0;
        vertices->push_back(vtx);
        tex._v[1] = 1.0;
        texcoords->push_back(tex);
    }

    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array;
    normals->push_back( osg::Vec3(0.0f,0.0f, 1.0f) );

    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
    colors->push_back( osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f) );

    quad = new osg::Geometry;
    quad->setVertexArray( vertices.get() );
    quad->setNormalArray( normals.get() );
    quad->setNormalBinding( osg::Geometry::BIND_OVERALL );
    quad->setColorArray( colors.get() );
    quad->setColorBinding( osg::Geometry::BIND_OVERALL );
    quad->setTexCoordArray( 0, texcoords.get() );
    quad->addPrimitiveSet( new osg::DrawArrays(GL_TRIANGLE_STRIP, 0, vertices->size()) );

    quad->setDataVariance(osg::Object::DYNAMIC);

    osg::ref_ptr<osg::Texture2D> texture = new osg::Texture2D;
    osg::ref_ptr<osg::Image> image = osgDB::readImageFile(shadowTextureName);
    texture->setImage( image.get() );

    osg::ref_ptr<osg::BlendFunc> blendFunc = new osg::BlendFunc;
    blendFunc->setFunction( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    osg::ref_ptr<osg::Geode> root = new osg::Geode;
    root->addDrawable( quad.get() );

    osg::StateSet* stateset = root->getOrCreateStateSet();
    //stateset->setMode( GL_DEPTH_TEST, osg::StateAttribute::OFF );
    stateset->setTextureAttributeAndModes(0, texture.get() );
    stateset->setAttributeAndModes( blendFunc );
    stateset->setRenderingHint(osg::StateSet::TRANSPARENT_BIN );

    shadowVertices = vertices;

    //  GfOut("\n################## LOADED SHADOW ###############################\n");




    return root.get();
}

void SDCar::deactivateCar(tCarElt*car){
    if(this->car == car){
        this->car_root->setNodeMask(0);
    }
}

void SDCar::activateCar(tCarElt*car){
    if(this->car == car){
        this->car_root->setNodeMask(1);
    }
}

void SDCar::updateCar()
{
    osg::Vec3 p;

    p[0] = car->_pos_X;//+ car->_drvPos_x;
    p[1] = car->_pos_Y;//+car->_drvPos_y;
    p[2] = car->_pos_Z;//+car->_drvPos_z;

    osg::Matrix mat(car->_posMat[0][0],car->_posMat[0][1],car->_posMat[0][2],car->_posMat[0][3],
            car->_posMat[1][0],car->_posMat[1][1],car->_posMat[1][2],car->_posMat[1][3],
            car->_posMat[2][0],car->_posMat[2][1],car->_posMat[2][2],car->_posMat[2][3],
            car->_posMat[3][0],car->_posMat[3][1],car->_posMat[3][2],car->_posMat[3][3]);

    wheels.updateWheels();

    this->car_branch->setMatrix(mat);

    reflectionMapping->update();
    this->setReflectionMap(reflectionMapping->getReflectionMap());

    //ugly computation,
    if (SHADOW_TECHNIQUE == 0)
    {
        osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
        for(unsigned int i=0;i<shadowVertices->size();i++)
        {
            osg::Vec3 vtx = (*shadowVertices.get())[i];
            osg::Vec4 vtx_world = osg::Vec4(vtx,1.0f)*mat;
            vtx_world._v[2] = RtTrackHeightG(car->_trkPos.seg, vtx_world.x(), vtx_world.y()); //0.01 needed, we have to sort out why
            vertices->push_back(osg::Vec3(vtx_world.x(), vtx_world.y(), vtx_world.z()));
        }
        quad->setVertexArray(vertices);
    }
}

void SDCar::updateShadingParameters(osg::Matrixf modelview){

    shader->update(modelview);
}

void SDCar::setReflectionMap(osg::ref_ptr<osg::Texture> map){
    car_branch->getOrCreateStateSet()->setTextureAttributeAndModes(2,map,osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
}

SDCars::SDCars(void)
{
    cars_branch = new osg::Group;
}

SDCars::~SDCars(void)
{
    for(unsigned i=0;i<the_cars.size();i++){
        delete the_cars[i];
    }
}

void SDCars::addSDCar(SDCar * car)
{
    the_cars.insert(the_cars.end(),car);
}

void SDCars::loadCars(tSituation * pSituation)
{
    tSituation *s = pSituation;
    this->situation = pSituation;
    for (int i = 0; i < s->_ncars; i++)
    {
        tCarElt* elt = s->cars[i];
        SDCar * car = new SDCar;
        this->addSDCar(car);
        this->cars_branch->addChild(car->loadCar(elt));
    }
    
    return;;
}

void SDCars::deactivateCar(tCarElt*car){
    std::vector<SDCar *>::iterator it;
    for(it = the_cars.begin(); it!= the_cars.end(); it++)
    {
        (*it)->deactivateCar(car);
    }
}

void SDCars::activateCar(tCarElt*car){
    std::vector<SDCar *>::iterator it;
    for(it = the_cars.begin(); it!= the_cars.end(); it++)
    {
        (*it)->activateCar(car);
    }
}

SDCar * SDCars::getCar(tCarElt*car){
    std::vector<SDCar *>::iterator it;
    SDCar * res;
    for(it = the_cars.begin(); it!= the_cars.end(); it++)
    {
        if((*it)->isCar(car)){
            res = *it;
        }
    }
    return res;
}




void SDCars::updateCars()
{
    std::vector<SDCar *>::iterator it;
    for(it = the_cars.begin(); it!= the_cars.end(); it++)
    {
        (*it)->updateCar();
    }
}

void SDCars::updateShadingParameters(osg::Matrixf modelview){
    std::vector<SDCar *>::iterator it;
    for(it = the_cars.begin(); it!= the_cars.end(); it++)
    {
        (*it)->updateShadingParameters(modelview);
    }
}

void SDCars::unLoad()
{
    cars_branch->removeChildren(0, cars_branch->getNumChildren());
    cars_branch = NULL;
}
