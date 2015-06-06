/***************************************************************************

    file                     : OsgCloud.h
    created                  : Wen Mar 27 00:00:41 CEST 2013
    copyright                : (C) 2013 by Xavier Bertaux
    email                    : bertauxx@yahoo.fr
    version                  : $Id: OsgCloud.h 4693 2013-03-27 03:12:09Z torcs-ng $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#ifndef _OSGCLOUD_H
#define _OSGCLOUD_H

#include <string>
using std::string;

#include <osg/ref_ptr>
#include <osg/Array>
#include <osg/Geode>
#include <osg/Group>
#include <osg/MatrixTransform>
#include <osg/Switch>

//class SDCloudField;

class SDCloudLayer
{
public:

    enum Coverage
    {
        SD_CLOUD_OVERCAST = 0,
        SD_CLOUD_BROKEN,
        SD_CLOUD_SCATTERED,
        SD_CLOUD_FEW,
        SD_CLOUD_CIRRUS,
        SD_CLOUD_CLEAR,
        SD_MAX_CLOUD_COVERAGES
    };

    SDCloudLayer( const string &tex_path );

    ~SDCloudLayer( void );

    /** get the cloud span (in meters) */
    float getSpan_m () const;
    void setSpan_m (float span_m);

    float getElevation_m () const;
    void setElevation_m (float elevation_m, bool set_span = true);

    float getThickness_m () const;
    void setThickness_m (float thickness_m);

    float getVisibility_m() const;
    void setVisibility_m(float visibility_m);

    float getTransition_m () const;
    void setTransition_m (float transition_m);

    Coverage getCoverage () const;
    void setCoverage (Coverage coverage);

    /*const string & getCoverageString() const;
    static const string & getCoverageString( Coverage coverage );

    static Coverage getCoverageType( const std::string & coverage );
    void setCoverageString( const string & coverage );*/

    inline void setDirection(float dir)
    {
        // cout << "cloud dir = " << dir << endl;
        direction = dir;
    }

    inline float getDirection() { return direction; }
    inline void setSpeed(float sp)
    {
        //Out("cloud speed = %f\n", sp);
        speed = sp;
    }

    inline float getSpeed() { return speed; }
    inline void setAlpha( float alpha )
    {
        if ( alpha < 0.0 ) { alpha = 0.0; }
        if ( alpha > max_alpha ) { alpha = max_alpha; }
        cloud_alpha = alpha;
    }

    inline void setMaxAlpha( float alpha )
    {
        if ( alpha < 0.0 ) { alpha = 0.0; }
        if ( alpha > 1.0 ) { alpha = 1.0; }
        max_alpha = alpha;
    }

    inline float getMaxAlpha() const
    {
        return max_alpha;
    }

    void rebuild();
    void set_enable3dClouds(bool enable);

    bool repaint( const osg::Vec3f& fog_color );
    bool reposition(const osg::Vec3f& p, double dt );

    osg::Switch* getNode() { return cloud_root.get(); }

    /** return the 3D layer cloud associated with this 2D layer */
    //SDCloudField *get_layer3D(void) { return layer3D; }

protected:
    void setTextureOffset(const osg::Vec2& offset);
private:

    osg::ref_ptr<osg::Switch> cloud_root;
    osg::ref_ptr<osg::Switch> layer_root;
    osg::ref_ptr<osg::Group> group_top, group_bottom;
    osg::ref_ptr<osg::MatrixTransform> layer_transform;
    osg::ref_ptr<osg::Geode> layer[4];

    float cloud_alpha;          // 1.0 = drawn fully, 0.0 faded out completely

    osg::ref_ptr<osg::Vec4Array> cl[4];
    osg::ref_ptr<osg::Vec3Array> vl[4];
    osg::ref_ptr<osg::Vec2Array> tl[4];
    osg::ref_ptr<osg::Vec3Array> tl2[4];

    // height above sea level (meters)
    std::string texture_path;
    float layer_span;
    float layer_asl;
    float layer_thickness;
    float layer_transition;
    float layer_visibility;
    Coverage layer_coverage;
    float scale;
    float speed;
    float direction;
    float alt;

    //SGGeod last_pos;
    osg::Vec3f last_pos;
    double max_alpha;
    double last_x, last_y;

    osg::Vec2 base;

    //SDCloudField *layer3D;
};

#endif // _OSGCLOUD_H
