/***************************************************************************

    file                 : OsgDome.h
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
#ifndef _OSGSKYDOME_H
#define _OSGSKYDOME_H

#include <osg/ref_ptr>
#include <osg/Array>
#include <osg/MatrixTransform>

#include "OsgReferenced.h"
#include "OsgMath.h"

namespace osg
{
class DrawElementsUShort;
}

class SDSkyDome : public osg::Referenced
{
    osg::ref_ptr<osg::MatrixTransform> dome_transform;
    double asl;

    osg::ref_ptr<osg::Vec3Array> dome_vl;
    osg::ref_ptr<osg::Vec3Array> dome_cl;
public:

    // Constructor
    SDSkyDome( void );

    // Destructor
    ~SDSkyDome( void );

    osg::Node *build( double hscale = 80000.0, double vscale = 80000.0 );
    bool repaint( const osg::Vec3f &sky_color, const osg::Vec3f &fog_color,
                  double sun_angle, double vis );

    bool reposition( const osg::Vec3f& p, double spin );
private:
    void makeDome(int rings, int bands, osg::DrawElementsUShort& elements);
};

#endif // OSGSKYDOME_H
