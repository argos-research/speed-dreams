/***************************************************************************

    file                 : OsgLoader.cpp
    created              : Mon Aug 21 18:24:02 CEST 2012
    copyright            : (C)2013 by Xavier Bertaux
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

#include "OsgLoader.h"

#include <tgf.hpp>

#include <osg/MatrixTransform>
#include <osg/Node>

osgLoader::osgLoader(void) :
    m_pOpt(new osgDB::ReaderWriter::Options())
{
}

osgLoader::~osgLoader(void)
{
}

void osgLoader::AddSearchPath(std::string strPath)
{
    m_pOpt->getDatabasePathList().push_back(strPath);
}

osg::ref_ptr<osg::Image> osgLoader::LoadImageFile(std::string strFile)
{
    osg::ref_ptr<osg::Image> Image;
    std::string absFileName = osgDB::findDataFile(strFile, m_pOpt);
    if (absFileName.empty())
        return Image;

    Image = osgDB::readRefImageFile(absFileName, m_pOpt);
    GfLogDebug("Loaded %s \n", absFileName.c_str());

    return Image;
}

osg::Node *osgLoader::Load3dFile(std::string strFile, bool car, std::string& name)
{
    osg::Node *pNode = NULL;
    std::string ext = osgDB::getFileExtension(strFile);

	m_ACCReader.SetCar(car);
	m_ACCReader.SetSkin(name);

    if (ext == "acc")
    {
        //Use custom ACC file loader
        osgDB::ReaderWriter::ReadResult rr = m_ACCReader.readNode(strFile, m_pOpt);
        GfOut("Object ACC load = %s -  %d \n", strFile.c_str(), rr.validNode());
        if (rr.validNode())
        {
            osg::Node *nod = rr.takeNode();
            osg::MatrixTransform *rot = new osg::MatrixTransform;
            osg::Matrix mat( 1.0f,  0.0f, 0.0f, 0.0f,
                             0.0f,  0.0f, 1.0f, 0.0f,
                             0.0f, -1.0f, 0.0f, 0.0f,
                             0.0f,  0.0f, 0.0f, 1.0f);
            rot->setMatrix(mat);
            rot->addChild(nod);

            return rot;
        }
        else
        {
            return NULL;
        }
    }
    else
    {
        pNode = osgDB::readNodeFile(strFile, m_pOpt);
		GfOut("Object AC load = %s\n", strFile.c_str());
    }

    osg::MatrixTransform *rot = new osg::MatrixTransform;
    osg::Matrix mat( 1.0f,  0.0f, 0.0f, 0.0f,
                     0.0f,  0.0f, 1.0f, 0.0f,
                     0.0f, -1.0f, 0.0f, 0.0f,
                     0.0f,  0.0f, 0.0f, 1.0f);
    rot->setMatrix(mat);
    rot->addChild(pNode);

    return rot;
}
