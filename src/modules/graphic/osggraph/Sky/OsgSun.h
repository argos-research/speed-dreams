/***************************************************************************

    file                 : OsgSun.h
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

#ifndef _OSGSUN_H_
#define _OSGSUN_H_

#include <osg/Array>
#include <osg/Node>
#include <osg/MatrixTransform>

class SDSun : public osg::Referenced
{
    osg::ref_ptr<osg::MatrixTransform> sun_transform;

    osg::ref_ptr<osg::Vec4Array> sun_cl;
    osg::ref_ptr<osg::Vec4Array> scene_cl;
    osg::ref_ptr<osg::Vec4Array> ihalo_cl;
    osg::ref_ptr<osg::Vec4Array> ohalo_cl;

    double visibility;
    double prev_sun_angle;
    double sun_angle;
    double sun_rotation;
    double sun_angle_to_scene;

    // used by reposition
    double sun_right_ascension;
    double sun_declination;
    double sun_dist;
    double path_distance;
    double sun_exp2_punch_through;
    osg::Vec3f sun_position;

public:

    // Constructor
    SDSun( void );

    // Destructor
    ~SDSun( void );

    osg::Node* build( std::string path, double dist, double sun_size );

    bool repaint( double sun_angle, double new_visibility );
    bool reposition( osg::Vec3d p, double angle);
    bool update_color_angle(double angle);

    osg::Vec4f get_color();
    osg::Vec4f get_scene_color();

    void setSunAngle(double angle) { sun_angle = angle; }
    double getSunAngle() { return sun_angle; }

    void setSunRotation(double rotation) { sun_rotation = rotation; }
    double getSunRotation() { return sun_rotation; }

    void setSunRightAscension(double ra) { sun_right_ascension = ra; }
    double getSunRightAscension() { return sun_right_ascension; }

    void setSunDeclination( double decl ) { sun_declination = decl; }
    double getSunDeclination() { return sun_declination; }

    void setSunDistance( double dist ) { sun_dist = dist; }
    double getSunDistance() { return sun_dist; }

    osg::Vec3f getSunPosition(){return sun_position;}
};

#endif // _OSGSUN_H_
