/***************************************************************************

    file                 : OsgPit.cpp
    created              : Sun Sep 07 20:13:56 CEST 2014
    copyright            : (C)2014 Xavier Bertaux
    email                : bertauxx@yahoo.fr
    version              : $Id: OsgPit.cpp 2436 2014-09-07 14:22:43Z torcs-ng $

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
//#include <osg/Node>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>

#include "OsgScenery.h"

/*SDPit::SDPit(void)
{
}

SDPit::~SDPit(void)
{
    _osgpit->removeChildren(0, _osgpit->getNumChildren());
    _osgpit = NULL;
}

void SDPit::build(const std::string TrackPath)
{
    std::string LocalPath = GetDataDir();

    osgDB::FilePathList pathList = osgDB::Registry::instance()->getDataFilePathList();
    pathList.push_back(TrackPath);
    pathList.push_back(LocalPath+"data/textures");
    osgDB::Registry::instance()->setDataFilePathList(pathList);

    osg::ref_ptr<osg::MatrixTransform> _pit_transform = new osg::MatrixTransform;
    osg::Matrix mat( 1.0f,  0.0f, 0.0f, 0.0f, 0.0f,  0.0f, 1.0f, 0.0f,
                     0.0f, -1.0f, 0.0f, 0.0f, 0.0f,  0.0f, 0.0f, 1.0f);

    osg::ref_ptr<osg::Node> m_pit = osgDB::readNodeFile("pit.ac");
    _pit_transform->setMatrix(mat);
    _pit_transform->addChild( m_pit.get() );

    _osgpit = new osg::Group;
    _osgpit->addChild(_pit_transform.get());
}*/
