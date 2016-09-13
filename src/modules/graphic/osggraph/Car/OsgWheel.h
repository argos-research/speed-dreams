/***************************************************************************

    file                 : OsgWheel.h
    created              : Mon Aug 21 18:24:02 CEST 2012
    copyright            : (C) 2012 by Gaëtan André
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

#ifndef _OSGWHEEL_H_
#define _OSGWHEEL_H_

#include <car.h>
#include <raceman.h>
#include <osg/Group>
#include <osg/Node>
#include <osg/Switch>
#include <osg/MatrixTransform>
#include <vector>

#include "OsgBrake.h"

class SDWheels
{
private :
    tCarElt *car;
    osg::ref_ptr<osg::Switch> wheels_switches[4];
    osg::ref_ptr<osg::MatrixTransform> wheels[4];
    SDBrakes brakes;
    osg::ref_ptr<osg::MatrixTransform> initWheel(int wheelIndec, const char *wheel_mod_name);
    unsigned int rcvShadowMask;
    unsigned int castShadowMask;

public :
    osg::ref_ptr<osg::Node> initWheels(tCarElt *car,void * handle);
    void updateWheels();
};

#endif /* _OSGCAR_H_ */
