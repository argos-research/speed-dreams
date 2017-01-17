/***************************************************************************

    file                 : OsgSteer.cpp
    created              : Mon Dec 1 18:24:02 CEST 2014
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
//#include <portability.h>
#include <osgDB/WriteFile>

#include "OsgLoader.h"
#include "OsgSteer.h"

SDSteer::SDSteer(void)
{
}

SDSteer::~SDSteer(void)
{
    //delete loader;
}

osg::ref_ptr<osg::Node> SDSteer::initSteer(tCarElt *car, void *handle, bool tracktype)
{
   return NULL;
    /*rcvShadowMask = 0x1;
    castShadowMask = 0x2;
    this->car = car;

    const char *param;
    char path[256];

    osg::ref_ptr<osg::Switch> pWing = new osg::Switch;
    osg::ref_ptr<osg::Node> pWin1 = new osg::Node;
    osg::ref_ptr<osg::Node> pWin2 = new osg::Node;

    snprintf(path, 256, "%s/%s", SECT_GROBJECTS, SECT_WING_MODEL);
    param = GfParmGetStr(handle, path, PRM_WING_1, NULL);

    if (param)
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

        strPath = GetDataDir();

        sprintf(buf, "cars/models/%s/wingR1-src.ac", car->_carName);
        strPath+=buf;

        pWin1 = loader.Load3dFile(strPath, true);
        pWin1->setName("WING1");
        GfLogInfo("Load Wing1 ACC !\n");

        strPath = GetDataDir();
        sprintf(buf, "cars/models/%s/wingR2-src.ac", car->_carName);
        strPath+=buf;
        pWin2 = loader.Load3dFile(strPath, true);

        pWin2->setName("WING2");
        GfLogInfo("Load Wing2 ACC !\n");

        pWing->addChild(pWin1.get(), true);
        pWing->addChild(pWin2.get(), false);

        if (!tracktype)
            pWing->setSingleChildOn(0);
        else
            pWing->setSingleChildOn(1);
#if 1
    std::string pWing_path = GetLocalDir();
    pWing_path = pWing_path+"wing.osg";
    osgDB::writeNodeFile( *pWing, pWing_path );
#endif
    }

    return pWing.get();*/

}
