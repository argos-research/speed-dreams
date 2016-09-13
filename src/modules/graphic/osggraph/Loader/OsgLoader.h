/***************************************************************************

    file                 : OsgLoader.h
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

#include <osgDB/ReadFile>

#include "ReaderWriterACC.h"
//#include "ReaderWriterACC2.h"

class osgLoader
{
public:
    osgLoader(void);
    ~osgLoader(void);

    osg::Node *Load3dFile(std::string strFile, bool car, std::string& name);
    osg::ref_ptr<osg::Image> LoadImageFile(std::string strFile);
    void AddSearchPath(std::string strPath);

protected:
    osg::ref_ptr<osgDB::ReaderWriter::Options> m_pOpt;
    ReaderWriterACC m_ACCReader;
	//ReaderWriterACC2 m_ACCReader2;
};
