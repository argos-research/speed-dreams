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
 ***************************************************************************/

#ifndef _OSGREFLECTIONMAPPING_H_
#define _OSGREFLECTIONMAPPING_H_

#include <osg/TextureCubeMap>
#include <osg/Texture2D>

#include <car.h>

#define REFLECTIONMAPPING_OFF 0
#define REFLECTIONMAPPING_STATIC 1
#define REFLECTIONMAPPING_HYBRID 2
#define REFLECTIONMAPPING_DYNAMIC 3

class SDCar;

class SDReflectionMapping
{
    private:
        osg::ref_ptr<osg::Group> camerasRoot;
        std::vector< osg::ref_ptr<osg::Camera> > cameras;
        osg::ref_ptr<osg::Texture> reflectionMap;
        SDCar *car;
        inline osg::ref_ptr<osg::Group> getCamerasRoot(){
            return camerasRoot;
        }

    public:
        SDReflectionMapping(SDCar *c);
        ~SDReflectionMapping();

        inline osg::ref_ptr<osg::Texture> getReflectionMap(){
            return reflectionMap;
        }

        void update();
};

#endif //_OSGREFLECTIONMAPPING_H_
