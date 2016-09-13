/***************************************************************************

    file                     : OsgTrackLight.h
    created                  : Sun Apr 26 00:00:41 CEST 2015
    copyright                : (C)2015 by Xavier Bertaux
    email                    : bertauxx@yahoo.fr
    version                  : $Id: OsgTrackLight.h 4693 2015-04-26 03:12:09Z torcs-ng $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _OSGTRACKLIGHT_H
#define _OSGTRACKLIGHT_H

#include <string>
using std::string;

#include <osg/ref_ptr>
#include <osg/Array>
#include <osg/Geode>
#include <osg/Group>
#include <osg/MatrixTransform>
#include <osg/Switch>

typedef struct Situation tSituation;

void grTrackLightInit();
void grTrackLightUpdate( tSituation *s );
void grTrackLightShutdown();

class SDTrackLight : public osg::Referenced
{
public:

    enum Coverage
    {
        SD_LIGHT_OFF = 0,
        SD_LIGHT_RED,
        SD_LIGHT_GREEN,
        SD_LIGHT_YELLOW,
        SD_LIGHT_BLUE,
        SD_LIGHT_WHITE,
        SD_MAX_LIGHT
    };

    SDTrackLight( void );
    ~SDTrackLight( void );

    init(tSituation *s)

    void rebuild();

    bool repaint( const osg::Vec3f& fog_color );

    osg::ref_ptr<osg::Switch> getNode() { return lights_root.get(); }

protected:
    void setTextureOffset(const osg::Vec2& offset);
private:

    osg::ref_ptr<osg::Switch> light_root;
    osg::ref_ptr<osg::MatrixTransform> layer_transform;

    osg::ref_ptr<osg::Vec4Array> cl[4];
    osg::ref_ptr<osg::Vec3Array> vl[4];
    osg::ref_ptr<osg::Vec2Array> tl[4];
    osg::ref_ptr<osg::Vec3Array> tl2[4];

    tSituation *situation;

    // height above sea level (meters)
    std::string texture_path;

    osg::Vec2 base;
};

#endif // _OSGTRACKLIGHT_H