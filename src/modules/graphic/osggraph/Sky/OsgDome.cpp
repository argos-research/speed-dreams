/***************************************************************************

    file                 : OsgDome.cpp
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

#include <cmath>
#include <iterator>

#include <osg/Array>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/CullFace>
#include <osg/Node>
#include <osg/Math>
#include <osg/MatrixTransform>
#include <osg/Material>
#include <osg/ShadeModel>
#include <osg/PrimitiveSet>

#include<tgf.h>

#include "OsgMath.h"
#include "OsgVectorArrayAdapter.h"
#include "OsgDome.h"

using namespace osg;
using namespace osggraph;

static const float center_elev = 1.0;


    const int numRings = 64; //sizeof(domeParams) / sizeof(domeParams[0]);
    const int numBands = 64; // 12
    const int halfBands = numBands / 2;

    // Make dome a bit over half sphere
    const float domeAngle = 120.0;

    const float bandDelta = 360.0 / numBands;
    const float ringDelta = domeAngle / (numRings+1);

    // Which band is at horizon
    const int halfRings = numRings * (90.0 / domeAngle);
    const int upperRings = numRings * (60.0 / domeAngle); // top half
    const int middleRings = numRings * (15.0 / domeAngle);

static const float upper_radius = 0.9701; // (.6, 0.15)
static const float upper_elev = 0.2425;

static const float middle_radius = 0.9960; // (.9, .08)
static const float middle_elev = 0.0885;

static const float lower_radius = 1.0;
static const float lower_elev = 0.0;

static const float bottom_radius = 0.9922; // (.8, -.1)
static const float bottom_elev = -0.1240;

// Constructor
SDSkyDome::SDSkyDome( void )
{
    asl = 0;
}

// Destructor
SDSkyDome::~SDSkyDome( void )
{
}

struct GridIndex
{
    SDVectorArrayAdapter<Vec3Array> gridAdapter;
    Vec3Array& grid;
    GridIndex(Vec3Array& array, int rowStride, int baseOffset) :
        gridAdapter(array, rowStride, baseOffset), grid(array)
    {
    }
    unsigned short operator() (int ring, int band)
    {
        return (unsigned short)(&gridAdapter(ring, band) - &grid[0]);
    }
};

void SDSkyDome::makeDome(int rings, int bands, DrawElementsUShort& elements)
{

    std::back_insert_iterator<DrawElementsUShort> pusher
        = std::back_inserter(elements);
    GridIndex grid(*dome_vl, numBands, 1);

    for (int i = 0; i < bands; i++)
    {
        *pusher = 0;  *pusher = grid(0, i+1);  *pusher = grid(0, i);
        // down a band
        for (int j = 0; j < rings - 1; ++j)
        {
            *pusher = grid(j, i);  *pusher = grid(j, (i + 1)%bands);
            *pusher = grid(j + 1, (i + 1)%bands);
            *pusher = grid(j, i);  *pusher =  grid(j + 1, (i + 1)%bands);
            *pusher =  grid(j + 1, i);
        }
    }
}

osg::Node* SDSkyDome::build( double hscale, double vscale )
{
    osg::Geode* geode = new osg::Geode;
    geode->setName("Skydome");
    geode->setCullingActive(false);

    osg::StateSet* stateSet = geode->getOrCreateStateSet();
    stateSet->setRenderBinDetails(-10, "RenderBin");

    osg::ShadeModel* shadeModel = new osg::ShadeModel;
    shadeModel->setMode(osg::ShadeModel::SMOOTH);
    stateSet->setAttributeAndModes(shadeModel);
    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    stateSet->setMode(GL_FOG, osg::StateAttribute::OFF);
    stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
    stateSet->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
    stateSet->setMode(GL_BLEND, osg::StateAttribute::OFF);
    stateSet->setMode(GL_ALPHA_TEST, osg::StateAttribute::OFF);
    stateSet->setAttribute(new osg::CullFace(osg::CullFace::BACK));

    osg::Material* material = new osg::Material;
    stateSet->setAttribute(material);

    dome_vl = new osg::Vec3Array(1 + numRings * numBands);
    dome_cl = new osg::Vec3Array(1 + numRings * numBands);
    // generate the raw vertex data

    (*dome_vl)[0].set(0.0, 0.0, center_elev * vscale);
    osggraph::SDVectorArrayAdapter<Vec3Array> vertices(*dome_vl, numBands, 1);

    for ( int i = 0; i < numBands; ++i )
    {
        double theta = (i * bandDelta) * SD_DEGREES_TO_RADIANS;
        double sTheta = hscale*sin(theta);
        double cTheta = hscale*cos(theta);
        for (int j = 0; j < numRings; ++j)
        {
            vertices(j, i).set(cTheta * sin((j+1)*ringDelta*SD_DEGREES_TO_RADIANS),
                               sTheta * sin((j+1)*ringDelta*SD_DEGREES_TO_RADIANS),
                               vscale * cos((j+1)*ringDelta*SD_DEGREES_TO_RADIANS));
        }
    }

    DrawElementsUShort* domeElements
        = new osg::DrawElementsUShort(GL_TRIANGLES);
    makeDome(numRings, numBands, *domeElements);
    osg::Geometry* geom = new Geometry;
    geom->setName("Dome Elements");
    geom->setUseDisplayList(false);
    geom->setVertexArray(dome_vl.get());
    geom->setColorArray(dome_cl.get());
    geom->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    geom->setNormalBinding(osg::Geometry::BIND_OFF);
    geom->addPrimitiveSet(domeElements);
    geode->addDrawable(geom);
    // force a repaint of the sky colors with ugly defaults
    repaint(osg::Vec3f(1.0, 1.0, 1.0), osg::Vec3f(1.0, 1.0, 1.0), 0.0, 10000.0 );
    dome_transform = new osg::MatrixTransform;
    dome_transform->addChild(geode);

    return dome_transform.get();
}

static void sd_fade_to_black(osg::Vec3f sky_color[], float asl, int count)
{
    const float ref_asl = 10000.0f;
    const float d = exp( - asl / ref_asl );
    for(int i = 0; i < count ; i++)
        sky_color[i] *= d;
}

inline void sd_clampColor(osg::Vec3& color)
{
    color.x() = osg::clampTo(color.x(), 0.0f, 1.0f);
    color.y() = osg::clampTo(color.y(), 0.0f, 1.0f);
    color.z() = osg::clampTo(color.z(), 0.0f, 1.0f);
}

bool SDSkyDome::repaint( const Vec3f& sky_color,
                    const Vec3f& fog_color, double sun_angle, double vis )
{
   osg::Vec3f outer_param, outer_diff;
   osg::Vec3f middle_param, middle_diff;

   // Check for sunrise/sunset condition
   //sun_angle *= SD_RADIANS_TO_DEGREES - 90.0;
   if (sun_angle > 80.0)
   {
       // 0.0 - 0.4
       double sunAngleFactor = 10.0 - fabs(90.0 - sun_angle);
       static const osg::Vec3f outerConstant(1.0 / 20.0, 1.0 / 40.0, -1.0 / 30.0);
       static const osg::Vec3f middleConstant(1.0 / 40.0, 1.0 / 80.0, 0.0);
       outer_param = outerConstant * sunAngleFactor;
       middle_param = middleConstant * sunAngleFactor;
       outer_diff = outer_param * (1.0 / numRings);
       middle_diff = middle_param * (1.0 / numRings);
   } else
   {
       outer_param = osg::Vec3f(0, 0, 0);
       middle_param = osg::Vec3f(0, 0, 0);
       outer_diff = osg::Vec3f(0, 0, 0);
       middle_diff = osg::Vec3f(0, 0, 0);
   }

   osg::Vec3f outer_amt = outer_param;
   osg::Vec3f middle_amt = middle_param;

   // Magic factors for coloring the sky according visibility and
   // zenith angle.
   const double cvf = osg::clampBelow(vis, 45000.0);
   const double vis_factor = osg::clampTo((vis - 1000.0) / 2000.0, 0.0, 1.0);
   const float upperVisFactor = 1.0 - vis_factor * (0.7 + 0.3 * cvf/45000);
   const float middleVisFactor = 1.0 - vis_factor * (0.1 + 0.85 * cvf/45000);

   (*dome_cl)[0] = sky_color;
   osggraph::SDVectorArrayAdapter<Vec3Array> colors(*dome_cl, numBands, 1);
   const double saif = sun_angle / SD_PI;
   static const osg::Vec3f blueShift(0.8, 1.0, 1.2);
   const osg::Vec3f skyFogDelta = sky_color - fog_color;

   for (int i = 0; i < halfBands+1; i++)
   {
       osg::Vec3f diff = componentMultiply(skyFogDelta, blueShift);
       diff *= (0.8 + saif - ((halfBands-i)/(float)(numBands-2)));

       colors(upperRings, i) = (sky_color - (diff * upperVisFactor));

       // Color top half by linear interpolation (90...60 degrees)
       int j=0;
       for (; j < upperRings; j++)
            colors(j, i) = (sky_color * (1 - j / (float)upperRings) + (colors(upperRings, i))* (j / (float)upperRings));

        j++; // Skip the 60 deg ring

        // From 60 to ~85 degrees
        for (int l = 0; j < upperRings + middleRings + 1; j++, l++)
            colors(j, i) = ((colors(upperRings, i) * (1- l / (float)middleRings)) + ((sky_color - diff * middleVisFactor  + middle_amt) * (l / (float)middleRings)));

        // 85 to 90 degrees
        for (int l = 0; j < halfRings; j++, l++)
            colors(j, i) = (colors(upperRings + middleRings, i) * (1 - l / (float)(halfRings - upperRings - middleRings))) + ((fog_color + outer_amt) * l / (float)(halfRings - upperRings - middleRings));

        for (int j = 0; j < numRings - 1; ++j)
            sd_clampColor(colors(j, i));

        outer_amt -= outer_diff;
        middle_amt -= middle_diff;
    }

    // Other side of dome is mirror of the other
    for (int i = halfBands+1; i < numBands; ++i)
        for (int j = 0; j < numRings-1; ++j)
            colors(j, i) = colors(j, numBands - i);

    // Fade colors to black when going to space
    // Center of dome is blackest and then fade decreases towards horizon
    sd_fade_to_black(&(*dome_cl)[0], asl * center_elev, 1);
    for (int i = 0; i < numRings - 1; ++i)
    {
        float fadeValue = (asl+0.05f) * cos(i*ringDelta*SD_DEGREES_TO_RADIANS);
        if(fadeValue < 0.0) fadeValue = 0.0;
        sd_fade_to_black(&colors(i, 0), fadeValue, numBands);
    }

    // All rings below horizon are fog color
    for ( int i = halfRings; i < numRings; i++)
        for ( int j = 0; j < numBands; j++ )
            colors(i, j) = fog_color;

    dome_cl->dirty();
    return true;
}

bool SDSkyDome::reposition( const osg::Vec3f &p, double spin )
{
    osg::Matrix T, SPIN;

    T.makeTranslate( p );
    SPIN.makeRotate(spin, osg::Vec3(0, 0, 1));

    dome_transform->setMatrix( T * SPIN );

    return true;
}