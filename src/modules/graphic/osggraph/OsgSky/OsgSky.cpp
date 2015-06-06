/***************************************************************************

    file                 : OsgSky.cpp
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

#include <tgf.h>

#include "OsgSky.h"
//#include "OsgCloudfield.h"
//#include "OsgNewcloud.h"

#include <osg/StateSet>
#include <osg/Depth>

// Used for rise/set effects (flat earth - no rotation of skydome considered here )
void calc_celestial_angles( const osg::Vec3f& body, const osg::Vec3f& view, double& angle, double& rotation )
{
	osg::Vec3f pos = body - view;
	angle = (90*SD_DEGREES_TO_RADIANS) - atan2(pos[2], sqrt(pos[0]*pos[0] + pos[1]*pos[1]));
	rotation = (90*SD_DEGREES_TO_RADIANS) - atan2(pos[0], pos[1]);
}


// Constructor
SDSky::SDSky( void )
{
    effective_visibility = visibility = 10000.0;
    in_puff = false;
    puff_length = 0;
    puff_progression = 0;
    ramp_up = 0.15;
    ramp_down = 0.15;
    in_cloud  = -1;

    dome = 0;
    sun = 0;
    moon = 0;
    planets = 0;
    stars = 0;
    pre_root = 0;

    clouds_3d_enabled = false;
    //clouds_3d_density = 0.8;

    pre_root = new osg::Group;
    pre_root->setNodeMask(BACKGROUND_BIT);
    osg::StateSet* preStateSet = new osg::StateSet;
    preStateSet->setAttribute(new osg::Depth(osg::Depth::LESS, 0.0, 1.0, false));
    pre_root->setStateSet(preStateSet);
    cloud_root = new osg::Group;
    cloud_root->setNodeMask(MODEL_BIT);

    pre_selector = new osg::Switch;
    pre_transform = new osg::Group;

    //_ephTransform = new osg::MatrixTransform;
}

// Destructor
SDSky::~SDSky( void )
{
    delete dome;
    delete sun;
    delete moon;
    delete planets;
    delete stars;
    pre_root->removeChild(0, pre_root->getNumChildren());
    for(unsigned i=0;i<cloud_layers.size();i++)
    {
        delete cloud_layers[i];
    }
    //delete pre_transform;
    //delete post_root;
}

void SDSky::build( std::string tex_path, double h_radius, double v_radius, double sun_size, double sun_dist,
      double moon_size, double moon_dist, int nplanets, osg::Vec3d *planet_data,
      int nstars, osg::Vec3d *star_data )
{
    delete dome;
    delete planets;
    delete stars;
    delete moon;
    delete sun;

    pre_root->removeChild(0, pre_root->getNumChildren());
    for(unsigned i=0;i<cloud_layers.size();i++)
    {
        delete cloud_layers[i];
    }

    dome = new SDSkyDome;
    pre_transform->addChild( dome->build( h_radius, v_radius ));

    //pre_transform->addChild(_ephTransform.get());
    planets = new SDStars;
    pre_transform->addChild( planets->build(nplanets, planet_data, h_radius));
    //_ephTransform->addChild( planets->build(eph.getNumPlanets(), eph.getPlanets(), h_radius));

    stars = new SDStars;
    pre_transform->addChild( stars->build(nstars, star_data, h_radius));
    //_ephTransform->addChild( stars->build(eph.getNumStars(), eph.getStars(), h_radius));

    moon = new SDMoon;
    pre_transform->addChild( moon->build( tex_path, moon_dist, moon_size));
    //_ephTransform->addChild( moon->build(tex_path, moon_size) );

    sun = new SDSun;
    pre_transform->addChild( sun->build( tex_path, sun_dist, sun_size));
    //_ephTransform->addChild( oursun->build(tex_path, sun_size, property_tree_node ) );

    pre_selector->addChild( pre_transform.get());
    pre_root->addChild( pre_selector.get());
}

bool SDSky::repaint( osg::Vec3f& sky_color, osg::Vec3f& fog_color, osg::Vec3f& cloud_color, double sol_angle,
                       double moon_angle, int nplanets, osg::Vec3d *planet_data,
                       int nstars, osg::Vec3d *star_data )
{
    if ( effective_visibility > 100.0 )
    {
        enable();
        dome->repaint( sky_color, fog_color, sol_angle, effective_visibility );

        sun->repaint( sol_angle, effective_visibility );
        moon->repaint( moon_angle );

        for ( unsigned i = 0; i < cloud_layers.size(); ++i )
        {
            if (cloud_layers[i]->getCoverage() != SDCloudLayer::SD_CLOUD_CLEAR)
            {
                cloud_layers[i]->repaint( cloud_color );
                GfOut("Repaint Cloud\n");
            }
        }

        planets->repaint( sol_angle, nplanets, planet_data );
        stars->repaint( sol_angle, nstars, star_data );
    } else
    {
                // turn off sky
                disable();
    }
    /*SDCloudField::updateFog((double)effective_visibility,
                            osg::Vec4f(sc.fog_color, 1.0f);*/
    return true;
}

bool SDSky::reposition( osg::Vec3& view_pos, double spin, /*double gst,*/
                        double dt )
{
  double angle;
  double rotation;

  sun->reposition( view_pos, 0 );
  moon->reposition( view_pos, 0 );

  osg::Vec3f sunpos = sun->getSunPosition ();
  calc_celestial_angles( sunpos, view_pos, angle, rotation );
  sun->setSunAngle( angle );
  sun->setSunRotation( rotation );
  sun->update_color_angle(angle);

  osg::Vec3f moonpos = moon->getMoonPosition();
  calc_celestial_angles( moonpos, view_pos, angle, rotation );
  moon->setMoonAngle( angle );
  moon->setMoonRotation( rotation );

  dome->reposition( view_pos, angle );

  for ( unsigned i = 0; i < cloud_layers.size(); ++i )
  {
      if ( cloud_layers[i]->getCoverage() != SDCloudLayer::SD_CLOUD_CLEAR)
      {
        cloud_layers[i]->reposition( view_pos, dt);
        GfOut("Affichage cloud\n");
      } else
        cloud_layers[i]->getNode()->setAllChildrenOff();
    }

    return true;
}

void SDSky::add_cloud_layer( SDCloudLayer * layer )
{
    cloud_layers.push_back(layer);
    layer->set_enable3dClouds(clouds_3d_enabled);
    cloud_root->addChild(layer->getNode());

}

const SDCloudLayer * SDSky::get_cloud_layer (int i) const
{
    return cloud_layers[i];
}

SDCloudLayer *SDSky::get_cloud_layer (int i)
{
    return cloud_layers[i];
}

int SDSky::get_cloud_layer_count () const
{
    return cloud_layers.size();
}

/*double SDSky::get_3dCloudDensity() const
{
    return SDNewCloud::getDensity();
}

void SDSky::set_3dCloudDensity(double density)
{
    SDNewCloud::setDensity(density);
}

float SDSky::get_3dCloudVisRange() const
{
    return SDCloudField::getVisRange();
}

void SDSky::set_3dCloudVisRange(float vis)
{
    SDCloudField::setVisRange(vis);
    for ( int i = 0; i < (int)cloud_layers.size(); ++i )
    {
        cloud_layers[i]->get_layer3D()->applyVisRange();
    }
}*/

void SDSky::texture_path( const string& path )
{
        const string tex_path =  path;
}

void SDSky::modify_vis( float alt, float time_factor )
{
    float effvis = visibility;

    for ( int i = 0; i < (int)cloud_layers.size(); ++i )
    {
        float asl = cloud_layers[i]->getElevation_m();
        float thickness = cloud_layers[i]->getThickness_m();
        float transition = cloud_layers[i]->getTransition_m();

        double ratio = 1.0;

        if ( cloud_layers[i]->getCoverage() == SDCloudLayer::SD_CLOUD_CLEAR )
        {
                // less than 50% coverage -- assume we're in the clear for now
                ratio = 1.0;
        } else if ( alt < asl - transition )
        {
                // below cloud layer
                ratio = 1.0;
                } else if ( alt < asl )
                {
                // in lower transition
                ratio = (asl - alt) / transition;
                } else if ( alt < asl + thickness )
                {
                // in cloud layer
                ratio = 0.0;
                } else if ( alt < asl + thickness + transition )
                {
                // in upper transition
                ratio = (alt - (asl + thickness)) / transition;
                } else
                {
                // above cloud layer
                ratio = 1.0;
                }

        if ( cloud_layers[i]->getCoverage() == SDCloudLayer::SD_CLOUD_CLEAR )/*||
             cloud_layers[i]->get_layer3D()->defined3D)*/
        {
        } else if ( (cloud_layers[i]->getCoverage() ==
                     SDCloudLayer::SD_CLOUD_FEW)
                    || (cloud_layers[i]->getCoverage() ==
                        SDCloudLayer::SD_CLOUD_SCATTERED) )
        {
            float temp = ratio * 2.0;
            if ( temp > 1.0 ) { temp = 1.0; }
            cloud_layers[i]->setAlpha( temp );
        } else
        {
            cloud_layers[i]->setAlpha( 1.0 );
            effvis *= ratio;
        }

        if ( effvis <= 25.0 )
        {
            effvis = 25.0;
        }

    }

    effective_visibility = effvis;
}
