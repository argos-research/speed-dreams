/***************************************************************************

    file                 : OsgCockpit.h
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

#ifndef _OSGCOCKPIT_H_
#define _OSGCOCKPIT_H_

#include <car.h>
#include <raceman.h>
#include <osg/Group>
#include <osg/Node>
#include <osg/MatrixTransform>
#include <vector>


class SDCockpit
{
    private :
        tCarElt *car;
        osg::ref_ptr<osg::Group> _cockpit;

        unsigned int rcvShadowMask;
        unsigned int castShadowMask;

    public :
        osg::ref_ptr<osg::Node> initCockpit(tCarElt *car,void * handle);
};

#endif /* _OSGCOCKPIT_H_ */
