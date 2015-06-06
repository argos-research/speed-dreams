/***************************************************************************

    file                 : OsgBrake.h
    created              : Mon Dec 31 10:24:02 CEST 2012
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

#ifndef _OSGBRAKE_H_
#define _OSGBRAKE_H_

#include <osg/Geode>
#include <car.h>

class SDBrakes
{
    private :
        tCarElt *car;
        osg::ref_ptr<osg::Geometry> brake_disks[4];

    public :
        void setCar(tCarElt * car);
        osg::ref_ptr<osg::Geode> initBrake(int wheelIndex);
        void updateBrakes();
};

#endif /* _OSGBRAKE_H_ */
