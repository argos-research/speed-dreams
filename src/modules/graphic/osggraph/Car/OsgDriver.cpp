/***************************************************************************

    file                 : OsgDriver.cpp
    created              : Wen Dec 3 18:24:02 CEST 2014
    copyright            : (C) 2014 by Xavier Bertaux
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

/* Portability don't build with MingW 4.8 */
#ifdef _MSC_VER
#include <portability.h>
#endif // _MSC_VER

#include "OsgLoader.h"
#include "OsgDriver.h"
#include <osgDB/WriteFile>

SDDriver::SDDriver(void)
{
}

SDDriver::~SDDriver(void)
{
    //delete loader;
}

osg::ref_ptr<osg::Switch> SDDriver::initDriver(tCarElt *car,void *handle)
{
    rcvShadowMask = 0x1;
    castShadowMask = 0x2;
    this->car = car;

    const char *param;
    char path[256];

    // separate driver models for animation according to steering wheel angle ...
    snprintf(path, 256, "%s/%s", SECT_GROBJECTS, LST_DRIVER);
    int nranges = GfParmGetEltNb(handle, path) + 1;
    _driverSwitch = new osg::Switch;

    if (nranges > 1)
    {
        osgLoader loader;
        char buf[4096];

        std::string TmpPath = GetDataDir();
        std::string strTPath;
        std::string strPath;

        snprintf(buf, 4096, "drivers/%s/%d/", car->_modName, car->_driverIndex);
        strTPath = TmpPath+buf;
        loader.AddSearchPath(strTPath);

        snprintf(buf, 4096, "cars/models/%s/", car->_carName);
        strTPath = TmpPath+buf;
        loader.AddSearchPath(strTPath);
        GfOut("Chemin Textures : %s\n", strTPath.c_str());

        int selIndex = 0;
        std::string tmp = GetLocalDir();
        std::string driver_path;

        // add the drivers
        for (int i = 1; i < nranges; i++)
        {
            osg::ref_ptr<osg::Node> driver_branch = new osg::Node;
            osg::ref_ptr<osg::MatrixTransform> position = new osg::MatrixTransform;

            snprintf(buf, 4096, "%s/%s/%d", SECT_GROBJECTS, LST_DRIVER, i);
            param = GfParmGetStr(handle, buf, PRM_DRIVERMODEL, "");
            //grCarInfo[index].DRMThreshold[selIndex] = GfParmGetNum(handle, buf, PRM_DRIVERSTEER, NULL, 0.0);

            tdble xpos = GfParmGetNum(handle, buf, PRM_XPOS, NULL, 0.0);
            tdble ypos = GfParmGetNum(handle, buf, PRM_YPOS, NULL, 0.0);
            tdble zpos = GfParmGetNum(handle, buf, PRM_ZPOS, NULL, 0.0);
            osg::Matrix pos = osg::Matrix::translate(xpos, ypos, zpos);

            driver_path = tmp+param;

            driver_branch = loader.Load3dFile(driver_path, true);

            position->addChild(driver_branch);
            _driverSwitch->addChild(position);
            driver_path ="";

            selIndex++;
        }

        _driverSwitch->setSingleChildOn(0);

#if 1
        std::string pDriver_path = GetLocalDir();
        pDriver_path = pDriver_path+"driver.osg";
        osgDB::writeNodeFile( *_driverSwitch, pDriver_path );
#endif
    }

    GfLogTrace("Driver Loading \n");

    return _driverSwitch.get();
}

/*void SDWheels::updateWheels()
{
    int j;
    static float maxVel[3] = { 20.0, 40.0, 70.0 };

    brakes.updateBrakes();

    for(int i=0; i<4; i++)
    {
        osg::Matrix spinMatrix = osg::Matrix::rotate(car->priv.wheel[i].relPos.ay, osg::Y_AXIS);

        osg::Matrix posMatrix = osg::Matrix::translate(car->priv.wheel[i].relPos.x, car->priv.wheel[i].relPos.y, car->priv.wheel[i].relPos.z);

        osg::Matrix camberDirMatrix = osg::Matrix::rotate(car->priv.wheel[i].relPos.ax, osg::X_AXIS,//camber
                                         0.0, osg::Y_AXIS,
                                         car->priv.wheel[i].relPos.az, osg::Z_AXIS );//direction

        posMatrix = camberDirMatrix * posMatrix;
        osg::MatrixTransform * trans = dynamic_cast<osg::MatrixTransform *>(wheels[i]->getChild(0));
        trans->setMatrix(spinMatrix);
        wheels[i]->setMatrix(posMatrix);

        for (j = 0; j < 3; j++)
        {
          if (fabs(car->_wheelSpinVel(i)) < maxVel[j])
            break;
        }

        this->wheels_switches[i]->setSingleChildOn(j);
    }
}*/
