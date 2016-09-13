/***************************************************************************

    file                 : OsgStars.h
    created              : Mon Aug 21 18:24:02 CEST 2012
    copyright            : (C)2012 by Xavier Bertaux
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

#ifndef _OSGSTARS_H_
#define _OSGSTARS_H_

#include <osg/Array>

class SDStars : public osg::Referenced
{
    osg::ref_ptr<osg::Vec4Array> stars_cl;
    int old_phase;

public:

    // Constructor
    SDStars( void );

    // Destructor
    ~SDStars( void );

    // initialize the stars structure
    osg::Node* build( int num, const osg::Vec3d star_data[], double star_dist );

    bool repaint( double sun_angle, int num, const osg::Vec3d star_data[] );
};

#endif // _OSGSTARS_H_
