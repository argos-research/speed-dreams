/***************************************************************************

    file                 : OsgDriver.h
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

#ifndef _OSGDRIVER_H_
#define _OSGDRIVER_H_

#include <car.h>
#include <raceman.h>
#include <osg/Group>
#include <osg/Node>
#include <osg/Switch>
#include <osg/MatrixTransform>
#include <vector>


class SDDriver
{
    private :
        tCarElt *car;
        osg::ref_ptr<osg::Switch> _driverSwitch;

        //osg::ref_ptr<osg::MatrixTransform> initWheel(int wheelIndec, const char *wheel_mod_name);
        unsigned int rcvShadowMask;
        unsigned int castShadowMask;

    public :
        SDDriver(void);
        ~SDDriver(void);
        osg::ref_ptr<osg::Switch> initDriver(tCarElt *car, void * handle);
        void updateDriver();
};

#endif /* _OSGDRIVER_H_ */
