/***************************************************************************

    file                 : OsgCockpit.cpp
    created              : Thu Dec 2 18:24:02 CEST 2014
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
#include "OsgCockpit.h"
#include <osgDB/WriteFile>

osg::ref_ptr<osg::Node> SDCockpit::initCockpit(tCarElt *car, void *handle)
{
    rcvShadowMask = 0x1;
    castShadowMask = 0x2;
    this->car = car;

    const char *param;
    char path[256];

    GfLogInfo("Cockpit Loaded\n");

    snprintf(path, 256, "%s/%s", SECT_GROBJECTS, SECT_COCKPIT);
    param = GfParmGetStr(handle, path, PRM_MODELCOCKPIT, NULL);


     _cockpit = new osg::Group;
    osg::ref_ptr<osg::Node> cock = new osg::Node;

#if 1
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

        // Load speed-dependant 3D cockpit model if available
        sprintf(buf, "cars/models/%s/cockpit.acc", car->_carName);
        strPath +=buf;
        cock= loader.Load3dFile(strPath, true);
        _cockpit->addChild(cock);

#if 0
    std::string pCockpit_path = GetLocalDir();
    pCockpit_path = pCockpit_path+"cockpit.osg";
    osgDB::writeNodeFile( *cock, pCockpit_path );
#endif
    }
#else
    char buf[4096];

    std::string LocalPath = GetDataDir();

    osg::ref_ptr<osgDB::Options> options = new::osgDB::ReaderWriter::Options();
    options->CACHE_NONE;
    //options = new osgDB::ReaderWriter::Options;

    snprintf(buf, 4096, "drivers/%s/%d/", car->_modName, car->_driverIndex);
    options->getDatabasePathList().push_back(LocalPath+buf);

    snprintf(buf, 4096, "cars/models/%s/", car->_carName);
    options->getDatabasePathList().push_back(LocalPath+buf);

    options->getDatabasePathList().push_back(LocalPath+"data/textures/");
    options->getDatabasePathList().push_back(LocalPath+"data/objects/");

    _cockpit->addChild( osgDB::readNodeFile("cockpit.osg", options));

    options->getDatabasePathList().clear();
    options = NULL;
#endif

    _cockpit->setNodeMask(castShadowMask);

    return _cockpit.get();
}
