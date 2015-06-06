/***************************************************************************

    file                 : OsgSun.cpp
    created              : Mon Aug 21 18:24:02 CEST 2012
    copyright            : (C) 2012 by Xavier Bertaux
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

#include <string>
#include <osg/AlphaFunc>
#include <osg/BlendFunc>
#include <osg/Fog>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Material>
#include <osg/ShadeModel>
#include <osg/TexEnv>
#include <osg/Texture2D>
#include <osgDB/ReadFile>

#include "OsgUtil/OsgColor.h"
#include "OsgSun.h"
#include "OsgUtil/OsgMath.h"

// Constructor
SDSun::SDSun( void ) :
    visibility(-9999.0), prev_sun_angle(-9999.0), path_distance(60000.0),
    sun_exp2_punch_through(7.0e-06)
{
}

// Destructor
SDSun::~SDSun( void )
{
}

osg::Node* SDSun::build( std::string path, double dist, double sun_size )
{
    std::string TmpPath = path;
    sun_transform = new osg::MatrixTransform;
    osg::StateSet* stateSet = sun_transform->getOrCreateStateSet();

    osg::TexEnv* texEnv = new osg::TexEnv;
    texEnv->setMode(osg::TexEnv::MODULATE);
    stateSet->setTextureAttribute(0, texEnv, osg::StateAttribute::ON);

    osg::Material* material = new osg::Material;
    material->setColorMode(osg::Material::AMBIENT_AND_DIFFUSE);
    material->setEmission(osg::Material::FRONT_AND_BACK, osg::Vec4(0,0,0,1));
    material->setSpecular(osg::Material::FRONT_AND_BACK, osg::Vec4(0,0,0,1));
    stateSet->setAttribute(material);

    osg::ShadeModel* shadeModel = new osg::ShadeModel;
    shadeModel->setMode(osg::ShadeModel::SMOOTH);
    stateSet->setAttributeAndModes(shadeModel);

    osg::AlphaFunc* alphaFunc = new osg::AlphaFunc;
    alphaFunc->setFunction(osg::AlphaFunc::ALWAYS);
    stateSet->setAttributeAndModes(alphaFunc);

    osg::BlendFunc* blendFunc = new osg::BlendFunc;
    blendFunc->setSource(osg::BlendFunc::SRC_ALPHA);
    blendFunc->setDestination(osg::BlendFunc::ONE_MINUS_SRC_ALPHA);
    stateSet->setAttributeAndModes(blendFunc);

    stateSet->setMode(GL_FOG, osg::StateAttribute::OFF);
    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    stateSet->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
    stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);

    sun_dist = dist;

    osg::Geode* geode = new osg::Geode;
    stateSet = geode->getOrCreateStateSet();

    stateSet->setRenderBinDetails(-6, "RenderBin");

    path = TmpPath+"data/sky/inner_halo.png";
    osg::ref_ptr<osg::Image> image = osgDB::readImageFile(path);
    osg::ref_ptr<osg::Texture2D> texture = new osg::Texture2D(image.get());
    stateSet->setTextureAttributeAndModes(0, texture);

    sun_cl = new osg::Vec4Array;
    sun_cl->push_back(osg::Vec4(1, 1, 1, 1));

    scene_cl = new osg::Vec4Array;
    scene_cl->push_back(osg::Vec4(1, 1, 1, 1));

    osg::Vec3Array* sun_vl = new osg::Vec3Array;
    sun_vl->push_back(osg::Vec3(-sun_size, 0, -sun_size));
    sun_vl->push_back(osg::Vec3(sun_size, 0, -sun_size));
    sun_vl->push_back(osg::Vec3(-sun_size, 0, sun_size));
    sun_vl->push_back(osg::Vec3(sun_size, 0, sun_size));

    osg::Vec2Array* sun_tl = new osg::Vec2Array;
    sun_tl->push_back(osg::Vec2(0, 0));
    sun_tl->push_back(osg::Vec2(1, 0));
    sun_tl->push_back(osg::Vec2(0, 1));
    sun_tl->push_back(osg::Vec2(1, 1));

    osg::Geometry* geometry = new osg::Geometry;
    geometry->setUseDisplayList(false);
    geometry->setVertexArray(sun_vl);
    geometry->setColorArray(sun_cl.get());
    geometry->setColorBinding(osg::Geometry::BIND_OVERALL);
    geometry->setNormalBinding(osg::Geometry::BIND_OFF);
    geometry->setTexCoordArray(0, sun_tl);
    geometry->addPrimitiveSet(new osg::DrawArrays(GL_TRIANGLE_STRIP, 0, 4));
    geode->addDrawable(geometry);

    sun_transform->addChild( geode );

    osg::Geode* geode1 = new osg::Geode;
    stateSet = geode1->getOrCreateStateSet();
    stateSet->setRenderBinDetails(-7, "RenderBin");

    path = TmpPath+"data/sky/inner_halo.png";
    osg::ref_ptr<osg::Image> image2 = osgDB::readImageFile(path);
    osg::ref_ptr<osg::Texture2D> texture2 = new osg::Texture2D(image2.get());
    stateSet->setTextureAttributeAndModes(0, texture2);

    ihalo_cl = new osg::Vec4Array;
    ihalo_cl->push_back(osg::Vec4(1, 1, 1, 1));

    float ihalo_size = sun_size * 2.0;
    osg::Vec3Array* ihalo_vl = new osg::Vec3Array;
    ihalo_vl->push_back(osg::Vec3(-ihalo_size, 0, -ihalo_size));
    ihalo_vl->push_back(osg::Vec3(ihalo_size, 0, -ihalo_size));
    ihalo_vl->push_back(osg::Vec3(-ihalo_size, 0, ihalo_size));
    ihalo_vl->push_back(osg::Vec3(ihalo_size, 0, ihalo_size));

    osg::Vec2Array* ihalo_tl = new osg::Vec2Array;
    ihalo_tl->push_back(osg::Vec2(0, 0));
    ihalo_tl->push_back(osg::Vec2(1, 0));
    ihalo_tl->push_back(osg::Vec2(0, 1));
    ihalo_tl->push_back(osg::Vec2(1, 1));

    geometry = new osg::Geometry;
    geometry->setUseDisplayList(false);
    geometry->setVertexArray(ihalo_vl);
    geometry->setColorArray(ihalo_cl.get());
    geometry->setColorBinding(osg::Geometry::BIND_OVERALL);
    geometry->setNormalBinding(osg::Geometry::BIND_OFF);
    geometry->setTexCoordArray(0, ihalo_tl);
    geometry->addPrimitiveSet(new osg::DrawArrays(GL_TRIANGLE_STRIP, 0, 4));
    geode->addDrawable(geometry);

    sun_transform->addChild( geode1 );

    osg::Geode* geode2 = new osg::Geode;
    stateSet = geode2->getOrCreateStateSet();
    stateSet->setRenderBinDetails(-8, "RenderBin");

    path = TmpPath+"data/sky/halo.png";
    osg::ref_ptr<osg::Image> image3 = osgDB::readImageFile(path);
    osg::ref_ptr<osg::Texture2D> texture3 = new osg::Texture2D(image3.get());
    stateSet->setTextureAttributeAndModes(0, texture3);

    ohalo_cl = new osg::Vec4Array;
    ohalo_cl->push_back(osg::Vec4(1, 1, 1, 1));

    float ohalo_size = sun_size * 10.0;
    osg::Vec3Array* ohalo_vl = new osg::Vec3Array;
    ohalo_vl->push_back(osg::Vec3(-ohalo_size, 0, -ohalo_size));
    ohalo_vl->push_back(osg::Vec3(ohalo_size, 0, -ohalo_size));
    ohalo_vl->push_back(osg::Vec3(-ohalo_size, 0, ohalo_size));
    ohalo_vl->push_back(osg::Vec3(ohalo_size, 0, ohalo_size));

    osg::Vec2Array* ohalo_tl = new osg::Vec2Array;
    ohalo_tl->push_back(osg::Vec2(0, 0));
    ohalo_tl->push_back(osg::Vec2(1, 0));
    ohalo_tl->push_back(osg::Vec2(0, 1));
    ohalo_tl->push_back(osg::Vec2(1, 1));

    geometry = new osg::Geometry;
    geometry->setUseDisplayList(false);
    geometry->setVertexArray(ohalo_vl);
    geometry->setColorArray(ohalo_cl.get());
    geometry->setColorBinding(osg::Geometry::BIND_OVERALL);
    geometry->setNormalBinding(osg::Geometry::BIND_OFF);
    geometry->setTexCoordArray(0, ohalo_tl);
    geometry->addPrimitiveSet(new osg::DrawArrays(GL_TRIANGLE_STRIP, 0, 4));
    geode2->addDrawable(geometry);

    sun_transform->addChild( geode2 );

    repaint( 0.0, 1.0 );

    return sun_transform.get();
}

bool SDSun::repaint( double sun_angle, double new_visibility )
{
    if ( visibility != new_visibility )
    {
        if (new_visibility < 100.0) new_visibility = 100.0;
        else if (new_visibility > 45000.0) new_visibility = 45000.0;
        visibility = new_visibility;

        static const float sqrt_m_log01 = sqrt( -log( 0.01 ) );
        sun_exp2_punch_through = sqrt_m_log01 / ( visibility * 15 );
    }

    if ( prev_sun_angle != sun_angle )
    {
        prev_sun_angle = sun_angle;

        double aerosol_factor;
        if ( visibility < 100 )
        {
            aerosol_factor = 8000;
        }
        else
        {
            aerosol_factor = 80.5 / log( visibility / 100 /*99.9*/ );
        }

        double rel_humidity, density_avg;
        rel_humidity = 0.5;
        density_avg = 0.7;

        osg::Vec4 i_halo_color, o_halo_color, scene_color, sun_color;

        double red_scat_f, /*red_scat_corr_f,*/ green_scat_f, blue_scat_f;

        red_scat_f = (aerosol_factor * path_distance * density_avg) / 5E+07;
        //red_scat_corr_f = sun_exp2_punch_through / (1 - red_scat_f);
        sun_color[0] = 1 -red_scat_f;
        //sun_color[0] = 1 - (1.1 * red_scat_f);
        i_halo_color[0] = 1 - (1.1 * red_scat_f);
        o_halo_color[0] = 1 - (1.4 * red_scat_f);
        //scene_color[0] = 1 - red_scat_f;

        // Green - 546.1 nm
        if (sun_declination > 5.0 || sun_declination < 2.0)
        {
            green_scat_f = ( aerosol_factor * path_distance * density_avg ) / 5E+07;
        }
        else
            green_scat_f = ( aerosol_factor * path_distance * density_avg ) / 8.8938E+06;

        sun_color[1] = 1 - green_scat_f /* red_scat_corr_f*/;
        //sun_color[1] = 1 - (1.1 * (green_scat_f /* red_scat_corr_f*/));
        i_halo_color[1] = 1 - (1.1 * (green_scat_f /* red_scat_corr_f*/));
        o_halo_color[1] = 1 - (1.4 * (green_scat_f /* red_scat_corr_f*/));
        //scene_color[1] = 1 - green_scat_f;

        // Blue - 435.8 nm
        blue_scat_f = (aerosol_factor * path_distance * density_avg) / 3.607E+06;
        sun_color[2] = 1 - blue_scat_f /* red_scat_corr_f*/;
        //sun_color[2] = 1 - (1.1 * (blue_scat_f /* red_scat_corr_f*/));
        i_halo_color[2] = 1 - (1.1 * (blue_scat_f /* red_scat_corr_f*/));
        o_halo_color[2] = 1 - (1.4 * (blue_scat_f /* red_scat_corr_f*/));
        //scene_color[2] = 1 - blue_scat_f;

        // Alpha
        sun_color[3] = 1;
        i_halo_color[3] = 1;
        //scene_color[3] = 1;

        o_halo_color[3] = blue_scat_f;
        if ( ( new_visibility < 10000 ) &&  ( blue_scat_f > 1 ))
        {
            o_halo_color[3] = 2 - blue_scat_f;
        }

        double saturation = 1 - ( rel_humidity / 200 );
        sun_color[1] += (( 1 - saturation ) * ( 1 - sun_color[1] ));
        sun_color[2] += (( 1 - saturation ) * ( 1 - sun_color[2] ));

        i_halo_color[1] += (( 1 - saturation ) * ( 1 - i_halo_color[1] ));
        i_halo_color[2] += (( 1 - saturation ) * ( 1 - i_halo_color[2] ));

        o_halo_color[1] += (( 1 - saturation ) * ( 1 - o_halo_color[1] ));
        o_halo_color[2] += (( 1 - saturation ) * ( 1 - o_halo_color[2] ));

        // just to make sure we're in the limits
        if ( sun_color[0] < 0 ) sun_color[0] = 0;
        else if ( sun_color[0] > 1) sun_color[0] = 1;
        if ( i_halo_color[0] < 0 ) i_halo_color[0] = 0;
        else if ( i_halo_color[0] > 1) i_halo_color[0] = 1;
        if ( o_halo_color[0] < 0 ) o_halo_color[0] = 0;
        else if ( o_halo_color[0] > 1) o_halo_color[0] = 1;

        if ( sun_color[1] < 0 ) sun_color[1] = 0;
        else if ( sun_color[1] > 1) sun_color[1] = 1;
        if ( i_halo_color[1] < 0 ) i_halo_color[1] = 0;
        else if ( i_halo_color[1] > 1) i_halo_color[1] = 1;
        if ( o_halo_color[1] < 0 ) o_halo_color[1] = 0;
        else if ( o_halo_color[0] > 1) o_halo_color[1] = 1;

        if ( sun_color[2] < 0 ) sun_color[2] = 0;
        else if ( sun_color[2] > 1) sun_color[2] = 1;
        if ( i_halo_color[2] < 0 ) i_halo_color[2] = 0;
        else if ( i_halo_color[2] > 1) i_halo_color[2] = 1;
        if ( o_halo_color[2] < 0 ) o_halo_color[2] = 0;
        else if ( o_halo_color[2] > 1) o_halo_color[2] = 1;
        if ( o_halo_color[3] < 0 ) o_halo_color[3] = 0;
        else if ( o_halo_color[3] > 1) o_halo_color[3] = 1;

        sd_gamma_correct_rgb( i_halo_color._v );
        sd_gamma_correct_rgb( o_halo_color._v );
        sd_gamma_correct_rgb( sun_color._v );

        (*sun_cl)[0] = sun_color;
        sun_cl->dirty();
        (*ihalo_cl)[0] = i_halo_color;
        ihalo_cl->dirty();
        (*ohalo_cl)[0] = o_halo_color;
        ohalo_cl->dirty();
    }

    return true;
}

bool SDSun::reposition( osg::Vec3d p, double angle)
{
    osg::Matrix T1, RA, DEC, T2, GST;

    T1.makeTranslate(p);
    GST.makeRotate((float)(angle), osg::Vec3(0.0, 0.0, -1.0));
    RA.makeRotate((float)(sun_right_ascension - 90 *SD_DEGREES_TO_RADIANS) , osg::Vec3(0, 0, 1));
    DEC.makeRotate((float)(sun_declination), osg::Vec3(1, 0, 0));
    T2.makeTranslate(0, sun_dist, 0);

    osg::Matrix R = T2*T1*GST*DEC*RA;
    sun_transform->setMatrix(R);

    osg::Vec4f pos = osg::Vec4f(0.0,0.0,0.0,1.0)*R;
    sun_position = osg::Vec3f(pos._v[0],pos._v[1],pos._v[2]);

    return true;
}

bool SDSun::update_color_angle( double angle )
{
    // Suncolor related things:
    if ( prev_sun_angle != sun_angle )
    {
        if ( sun_angle == 0 ) sun_angle = 0.1;
        const double r_earth_pole = 6356752.314;
        const double r_tropo_pole = 6356752.314 + 8000;
        const double epsilon_earth2 = 6.694380066E-3;
        const double epsilon_tropo2 = 9.170014946E-3;

        double r_tropo = r_tropo_pole / sqrt ( 1 - ( epsilon_tropo2 * pow ( cos( 0.0 ), 2 )));
        double r_earth = r_earth_pole / sqrt ( 1 - ( epsilon_earth2 * pow ( cos( 0.0 ), 2 )));

        double position_radius = r_earth;

        double gamma =  SD_PI - sun_angle;
        double sin_beta =  ( position_radius * sin ( gamma )  ) / r_tropo;

        if (sin_beta > 1.0) sin_beta = 1.0;

        double alpha =  SD_PI - gamma - asin( sin_beta );

        path_distance = sqrt( pow( position_radius, 2 ) + pow( r_tropo, 2 )
                        - ( 2 * position_radius * r_tropo * cos( alpha ) ));

        double alt_half = sqrt( pow ( r_tropo, 2 ) + pow( path_distance / 2, 2 ) - r_tropo * path_distance * cos( asin( sin_beta )) ) - r_earth;

        if ( alt_half < 0.0 ) alt_half = 0.0;
    }

    return true;
}

osg::Vec4f SDSun::get_color()
{
    return osg::Vec4f((*ohalo_cl)[0][0], (*ohalo_cl)[0][1], (*ohalo_cl)[0][2], (*ohalo_cl)[0][3]);
}

osg::Vec4f SDSun::get_scene_color()
{
    return osg::Vec4f((*sun_cl)[0][0], (*sun_cl)[0][1], (*sun_cl)[0][2], (*sun_cl)[0][3]);
}
