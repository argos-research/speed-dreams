/***************************************************************************

    file                     : OsgCloud.cpp
    created                  : Wen Mar 27 00:00:41 CEST 2013
    copyright                : (C)2013 by Xavier Bertaux
    email                    : bertauxx@yahoo.fr
    version                  : $Id: OsgCloud.cpp 4693 2013-03-27 03:12:09Z torcs-ng $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <sstream>
#include <math.h>

#include <osg/AlphaFunc>
#include <osg/BlendFunc>
#include <osg/CullFace>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Material>
#include <osg/ShadeModel>
#include <osg/TexEnv>
#include <osg/TexEnvCombine>
#include <osg/Texture2D>
#include <osg/TexMat>
#include <osg/Fog>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>

#include <tgf.h>

#include "OsgMath.h"
//#include "OsgNewcloud.h"
//#include "OsgCloudfield.h"
//#include "OsgStateAttributeFactory.h"
#include "OsgCloud.h"

using namespace osg;

static osg::ref_ptr<osg::StateSet> layer_states[SDCloudLayer::SD_MAX_CLOUD_COVERAGES];
static osg::ref_ptr<osg::StateSet> layer_states2[SDCloudLayer::SD_MAX_CLOUD_COVERAGES];
static bool state_initialized = false;

// make an StateSet for a cloud layer given the named texture
static osg::StateSet*
SDMakeState(const std::string &path, const char* colorTexture, const char* normalTexture )
{
    osg::StateSet *stateSet = new osg::StateSet;

    std::string TmpPath;
    TmpPath = path+"data/sky/"+colorTexture;
    GfLogInfo("Path Sky cloud color texture = %s\n", TmpPath.c_str());
    osg::ref_ptr<osg::Image> image = osgDB::readImageFile(TmpPath);
    osg::ref_ptr<osg::Texture2D> texture = new osg::Texture2D(image.get());
    texture->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
    texture->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
    stateSet->setTextureAttributeAndModes(0, texture.get());
    stateSet->setTextureMode(0, GL_TEXTURE_2D, osg::StateAttribute::ON);

	TmpPath = path+"data/sky/"+normalTexture; 
	GfLogInfo("Path Sky cloud normal texture = %s\n", TmpPath.c_str());
    osg::ref_ptr<osg::Image> image2 = osgDB::readImageFile(TmpPath);
    osg::ref_ptr<osg::Texture2D> texture2 = new osg::Texture2D(image2.get());
    texture2->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
    texture2->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
    stateSet->setTextureAttributeAndModes(1, texture2.get());
    stateSet->setTextureMode(1, GL_TEXTURE_2D, osg::StateAttribute::ON);

    osg::ref_ptr<osg::ShadeModel> Smooth = new osg::ShadeModel;
    Smooth->setMode(ShadeModel::SMOOTH);
    Smooth->setDataVariance(Object::STATIC);
    stateSet->setAttributeAndModes(Smooth.get());
    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

    osg::ref_ptr<osg::AlphaFunc> StandardAlphaFunc = new AlphaFunc;
    StandardAlphaFunc->setFunction(osg::AlphaFunc::GREATER);
    StandardAlphaFunc->setReferenceValue(0.01);
    StandardAlphaFunc->setDataVariance(Object::STATIC);
    stateSet->setAttributeAndModes(StandardAlphaFunc.get());

    osg::ref_ptr<BlendFunc> StandardBlendFunc = new BlendFunc;
    StandardBlendFunc->setSource(BlendFunc::SRC_ALPHA);
    StandardBlendFunc->setDestination(BlendFunc::ONE_MINUS_SRC_ALPHA);
    StandardBlendFunc->setDataVariance(Object::STATIC);
    
	stateSet->setAttributeAndModes(StandardBlendFunc.get());
	stateSet->setMode(GL_FOG, osg::StateAttribute::OFF);
    stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);
    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::ON);
    stateSet->setMode(GL_LIGHT0, osg::StateAttribute::OFF);    

    return stateSet;
}

// Constructor
SDCloudLayer::SDCloudLayer( const string &tex_path ) :
    cloud_root(new osg::Switch),
    layer_root(new osg::Switch),
    group_top(new osg::Group),
    group_bottom(new osg::Group),
    layer_transform(new osg::MatrixTransform),
    cloud_alpha(1.0),
    texture_path(tex_path),
    layer_span(0.0),
    layer_asl(0.0),
    layer_thickness(0.0),
    layer_transition(0.0),
    layer_visibility(6000.0),
    layer_coverage(SD_CLOUD_CLEAR),
    scale(4000.0),
    speed(0.0),
    direction(0.0),
    alt(1.0),
    /*last_course(0.0),*/
    max_alpha(1.0)
{
    cloud_root->addChild(layer_root.get(), true);
    layer_root->addChild(group_bottom.get());
    layer_root->addChild(group_top.get());

    osg::ref_ptr<osg::StateSet> rootSet = new osg::StateSet;
    rootSet = layer_root->getOrCreateStateSet();
    rootSet->setRenderBinDetails( 9, "DepthSortedBin");
    rootSet->setTextureAttribute(0, new osg::TexMat);
    rootSet->setMode(GL_CULL_FACE, osg::StateAttribute::ON);
    // Combiner for fog color and cloud alpha
    osg::TexEnvCombine* combine0 = new osg::TexEnvCombine;
    osg::TexEnvCombine* combine1 = new osg::TexEnvCombine;
    combine0->setCombine_RGB(osg::TexEnvCombine::MODULATE);
    combine0->setSource0_RGB(osg::TexEnvCombine::PREVIOUS);
    combine0->setOperand0_RGB(osg::TexEnvCombine::SRC_COLOR);
    combine0->setSource1_RGB(osg::TexEnvCombine::TEXTURE0);
    combine0->setOperand1_RGB(osg::TexEnvCombine::SRC_COLOR);
    combine0->setCombine_Alpha(osg::TexEnvCombine::MODULATE);
    combine0->setSource0_Alpha(osg::TexEnvCombine::PREVIOUS);
    combine0->setOperand0_Alpha(osg::TexEnvCombine::SRC_ALPHA);
    combine0->setSource1_Alpha(osg::TexEnvCombine::TEXTURE0);
    combine0->setOperand1_Alpha(osg::TexEnvCombine::SRC_ALPHA);

    combine1->setCombine_RGB(osg::TexEnvCombine::MODULATE);
    combine1->setSource0_RGB(osg::TexEnvCombine::PREVIOUS);
    combine1->setOperand0_RGB(osg::TexEnvCombine::SRC_COLOR);
    combine1->setSource1_RGB(osg::TexEnvCombine::CONSTANT);
    combine1->setOperand1_RGB(osg::TexEnvCombine::SRC_COLOR);
    combine1->setCombine_Alpha(osg::TexEnvCombine::MODULATE);
    combine1->setSource0_Alpha(osg::TexEnvCombine::PREVIOUS);
    combine1->setOperand0_Alpha(osg::TexEnvCombine::SRC_ALPHA);
    combine1->setSource1_Alpha(osg::TexEnvCombine::CONSTANT);
    combine1->setOperand1_Alpha(osg::TexEnvCombine::SRC_ALPHA);
    combine1->setDataVariance(osg::Object::DYNAMIC);
    rootSet->setTextureAttributeAndModes(0, combine0);
    rootSet->setTextureAttributeAndModes(1, combine1);
    rootSet->setTextureMode(1, GL_TEXTURE_2D, osg::StateAttribute::ON);

    osg::ref_ptr<osg::Image> dummyImage = new osg::Image;
    dummyImage->allocateImage(1, 1, 1, GL_LUMINANCE_ALPHA,
                              GL_UNSIGNED_BYTE);
    unsigned char* imageBytes = dummyImage->data(0, 0);
    imageBytes[0] = 255;
    imageBytes[1] = 255;
    osg::ref_ptr<osg::Texture2D> WhiteTexture = new osg::Texture2D;
    WhiteTexture->setImage(dummyImage);
    WhiteTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
    WhiteTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
    WhiteTexture->setDataVariance(osg::Object::STATIC);

    rootSet->setTextureAttributeAndModes(1, WhiteTexture, osg::StateAttribute::ON);
    rootSet->setDataVariance(osg::Object::DYNAMIC);

    // Ensure repeatability of the random seed within 10 minutes,
    // to keep multi-computer systems in sync.
    //SDRandom();
    base = osg::Vec2(SDRandom(), SDRandom());
    group_top->addChild(layer_transform.get());
    group_bottom->addChild(layer_transform.get());

    //layer3D = new SGCloudField();
    //cloud_root->addChild(layer3D->getNode(), false);

    rebuild();
}

// Destructor
SDCloudLayer::~SDCloudLayer()
{
  //delete layer3D;
}

float SDCloudLayer::getSpan_m () const
{
    return layer_span;
}

void SDCloudLayer::setSpan_m (float span_m)
{
    if (span_m != layer_span)
    {
        layer_span = span_m;
        rebuild();
    }
}

float SDCloudLayer::getElevation_m () const
{
    return layer_asl;
}

void SDCloudLayer::setElevation_m (float elevation_m, bool set_span)
{
    layer_asl = elevation_m;

    if (set_span)
    {
        if (elevation_m > 4000)
            setSpan_m(  elevation_m * 10 );
        else
            setSpan_m( 6000 );
    }
}

float SDCloudLayer::getThickness_m () const
{
    return layer_thickness;
}

void SDCloudLayer::setThickness_m (float thickness_m)
{
    layer_thickness = thickness_m;
}

float SDCloudLayer::getVisibility_m() const
{
    return layer_visibility;
}

void SDCloudLayer::setVisibility_m (float visibility_m)
{
    layer_visibility = visibility_m;
}

float SDCloudLayer::getTransition_m () const
{
    return layer_transition;
}

void SDCloudLayer::setTransition_m (float transition_m)
{
    layer_transition = transition_m;
}

SDCloudLayer::Coverage
SDCloudLayer::getCoverage () const
{
    return layer_coverage;
}

void SDCloudLayer::setCoverage (Coverage coverage)
{
    if (coverage != layer_coverage)
    {
        layer_coverage = coverage;
        rebuild();
    }
}

void SDCloudLayer::setTextureOffset(const osg::Vec2& offset)
{
    osg::StateAttribute *attr = layer_root->getStateSet()
        ->getTextureAttribute(0, osg::StateAttribute::TEXMAT);
    osg::TexMat *texMat = dynamic_cast<osg::TexMat*>(attr);
    if (!texMat)
        return;

    texMat->setMatrix(osg::Matrix::translate(offset[0],offset[1], 0.0));
}

// colors for debugging the cloud layers
Vec3 cloudColors[] = {Vec3(1.0f, 1.0f, 1.0f), Vec3(1.0f, 1.0f, 1.0f),
                      Vec3(1.0f, 1.0f, 1.0f), Vec3(1.0f, 1.0f, 1.0f)};

// build the cloud object
void SDCloudLayer::rebuild()
{
    if ( !state_initialized )
    {
        state_initialized = true;

        GfOut("initializing cloud layers\n");

        osg::ref_ptr<osg::StateSet> state;
        state = SDMakeState(texture_path, "overcast.png", "overcast_n.png");
        layer_states[SD_CLOUD_OVERCAST] = state;
        state = SDMakeState(texture_path, "overcast_top.png", "overcast_top_n.png");
        layer_states2[SD_CLOUD_OVERCAST] = state;

        state = SDMakeState(texture_path, "overcast2.png", "overcast2_n.png");
        layer_states[SD_CLOUD_OVERCAST2] = state;
        state = SDMakeState(texture_path, "overcast2_top.png", "overcast2_top_n.png");
        layer_states2[SD_CLOUD_OVERCAST2] = state;

        state = SDMakeState(texture_path, "broken.png", "broken_n.png");
        layer_states[SD_CLOUD_BROKEN] = state;
        layer_states2[SD_CLOUD_BROKEN] = state;

        state = SDMakeState(texture_path, "broken2.png", "broken2_n.png");
        layer_states[SD_CLOUD_BROKEN2] = state;
        layer_states2[SD_CLOUD_BROKEN2] = state;

        state = SDMakeState(texture_path, "scattered.png", "scattered_n.png" );
        layer_states[SD_CLOUD_SCATTERED] = state;
        layer_states2[SD_CLOUD_SCATTERED] = state;

        state = SDMakeState(texture_path, "scattered2.png", "scattered2_n.png" );
        layer_states[SD_CLOUD_SCATTERED2] = state;
        layer_states2[SD_CLOUD_SCATTERED2] = state;

        state = SDMakeState(texture_path, "many.png", "many_n.png");
        layer_states[SD_CLOUD_MANY] = state;
        layer_states2[SD_CLOUD_MANY] = state;

        state = SDMakeState(texture_path, "many2.png", "many2_n.png" );
        layer_states[SD_CLOUD_MANY2] = state;
        layer_states2[SD_CLOUD_MANY2] = state;

        state = SDMakeState(texture_path, "few.png", "few_n.png");
        layer_states[SD_CLOUD_FEW] = state;
        layer_states2[SD_CLOUD_FEW] = state;

        state = SDMakeState(texture_path, "few2.png", "few2_n.png");
        layer_states[SD_CLOUD_FEW2] = state;
        layer_states2[SD_CLOUD_FEW2] = state;

        state = SDMakeState(texture_path, "cirrus.png", "cirrus_n.png");
        layer_states[SD_CLOUD_CIRRUS] = state;
        layer_states2[SD_CLOUD_CIRRUS] = state;

        state = SDMakeState(texture_path, "cirrus2.png", "cirrus2_n.png");
        layer_states[SD_CLOUD_CIRRUS2] = state;
        layer_states2[SD_CLOUD_CIRRUS2] = state;

        layer_states[SD_CLOUD_CLEAR] = 0;
        layer_states2[SD_CLOUD_CLEAR] = 0;

#if 1
        // experimental optimization that may not make any difference
        // at all :/
        osg::CopyOp copyOp;
        for (int i = 0; i < SD_MAX_CLOUD_COVERAGES; ++i)
        {
            //SDStateAttributeFactory *saf = SDStateAttributeFactory::instance();
            if (layer_states[i].valid())
            {
                if (layer_states[i] == layer_states2[i])
                    layer_states2[i] = static_cast<osg::StateSet*>(layer_states[i]->clone(copyOp));

                osg::ref_ptr<CullFace> CullFaceFront = new CullFace(CullFace::FRONT);
                CullFaceFront->setDataVariance(Object::STATIC);
                layer_states[i]->setAttribute(CullFaceFront);

                osg::ref_ptr<osg::CullFace> CullFaceBack = new CullFace(CullFace::BACK);
                CullFaceBack->setDataVariance(Object::STATIC);
                layer_states2[i]->setAttribute(CullFaceBack);
            }
        }
#endif
    }

    //scale = 4000.0;

    setTextureOffset(base);
    // build the cloud layer
    const float layer_scale = layer_span / scale;
    const float mpi = SD_PI/4;

    // caclculate the difference between a flat-earth model and
    // a round earth model given the span and altutude ASL of
    // the cloud layer. This is the difference in altitude between
    // the top of the inverted bowl and the edge of the bowl.
    const float alt_diff = layer_asl * 1.5f;

    for (int i = 0; i < 4; i++)
    {
          if ( layer[i] != NULL )
          {
            layer_transform->removeChild(layer[i].get()); // automatic delete
          }

          vl[i] = new osg::Vec3Array;
          cl[i] = new osg::Vec4Array;
          tl[i] = new osg::Vec2Array;

          osg::Vec3 vertex(layer_span*(i-2)/2, -layer_span, alt_diff * (sin(i*mpi) - 2));
          osg::Vec2 tc(layer_scale * i/4, 0.0f);
          osg::Vec4 color(cloudColors[0], (i == 0) ? 0.0f : 0.15f);

          cl[i]->push_back(color);
          vl[i]->push_back(vertex);
          tl[i]->push_back(tc);

          for (int j = 0; j < 4; j++)
          {
            vertex = osg::Vec3(layer_span*(i-1)/2, layer_span*(j-2)/2, alt_diff * (sin((i+1)*mpi) + sin(j*mpi) - 2));
            tc = osg::Vec2(layer_scale * (i+1)/4, layer_scale * j/4);
            color = osg::Vec4(cloudColors[0], ( (j == 0) || (i == 3)) ? ( (j == 0) && (i == 3)) ? 0.0f : 0.15f : 1.0f );

            cl[i]->push_back(color);
            vl[i]->push_back(vertex);
            tl[i]->push_back(tc);

            vertex = osg::Vec3(layer_span*(i-2)/2, layer_span*(j-1)/2,
                               alt_diff * (sin(i*mpi) + sin((j+1)*mpi) - 2) );
            tc = osg::Vec2(layer_scale * i/4, layer_scale * (j+1)/4 );
            color = osg::Vec4(cloudColors[0],
                              ((j == 3) || (i == 0)) ?
                              ((j == 3) && (i == 0)) ? 0.0f : 0.15f : 1.0f );
            cl[i]->push_back(color);
            vl[i]->push_back(vertex);
            tl[i]->push_back(tc);
          }

          vertex = osg::Vec3(layer_span*(i-1)/2, layer_span,
                             alt_diff * (sin((i+1)*mpi) - 2));
          tc = osg::Vec2(layer_scale * (i+1)/4, layer_scale);
          color = osg::Vec4(cloudColors[0], (i == 3) ? 0.0f : 0.15f );

          cl[i]->push_back( color );
          vl[i]->push_back( vertex );
          tl[i]->push_back( tc );

          osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry;
          geometry->setUseDisplayList(false);
          geometry->setVertexArray(vl[i].get());
          geometry->setNormalBinding(osg::Geometry::BIND_OFF);
          geometry->setColorArray(cl[i].get(), osg::Array::BIND_PER_VERTEX);
          geometry->setTexCoordArray(0, tl[i].get(), osg::Array::BIND_PER_VERTEX);
          geometry->addPrimitiveSet(new osg::DrawArrays(GL_TRIANGLE_STRIP, 0, vl[i]->size()));
          layer[i] = new osg::Geode;

        std::stringstream sstr;
        sstr << "Cloud Layer (" << i << ")";
        geometry->setName(sstr.str());
        layer[i]->setName(sstr.str());
        layer[i]->addDrawable(geometry.get());
        layer_transform->addChild(layer[i].get());
    }

    if ( layer_states[layer_coverage].valid() )
    {
        osg::CopyOp copyOp;    // shallow copy

        osg::ref_ptr<osg::StateSet> stateSet = static_cast<osg::StateSet*>(layer_states2[layer_coverage]->clone(copyOp));
        stateSet->setDataVariance(osg::Object::DYNAMIC);
        group_top->setStateSet(stateSet.get());
        stateSet = static_cast<osg::StateSet*>(layer_states[layer_coverage]->clone(copyOp));
        stateSet->setDataVariance(osg::Object::DYNAMIC);
        group_bottom->setStateSet(stateSet.get());
    }
}

bool SDCloudLayer::repaint( const osg::Vec3f &fog_color )
{
    osg::Vec4f combineColor(fog_color, cloud_alpha);
    osg::ref_ptr<osg::TexEnvCombine> combiner
        = dynamic_cast<osg::TexEnvCombine*>(layer_root->getStateSet()
                                            ->getTextureAttribute(1, osg::StateAttribute::TEXENV));
    combiner->setConstantColor(combineColor);
	
    return true;
}

bool SDCloudLayer::reposition(const osg::Vec3f &p, double dt )
{
    if (getCoverage() != SDCloudLayer::SD_CLOUD_CLEAR)
    {
        osg::Vec3f asl_offset;

        if (alt <= layer_asl )
        {
            asl_offset = osg::Vec3f(p._v[0], p._v[1], layer_asl);
        } else
        {
            asl_offset = osg::Vec3f(p._v[0], p._v[1],layer_asl + layer_thickness);
        }

        //asl_offset.normalize();
        //asl_offset += p;

        osg::Matrix T;
        T.makeTranslate( asl_offset );

        layer_transform->setMatrix( T );

        group_bottom->getStateSet()->setRenderBinDetails(-(int)layer_asl, "RenderBin");
        group_top->getStateSet()->setRenderBinDetails((int)layer_asl, "RenderBin");
        if ( alt <= layer_asl )
        {
            layer_root->setSingleChildOn(0);
            GfLogDebug("Cloud dessous\n");
        } else if ( alt >= layer_asl + layer_thickness )
        {
            layer_root->setSingleChildOn(1);
            GfLogDebug("Cloud dessus\n");
        } else
        {
            layer_root->setAllChildrenOff();
            GfLogDebug("Cut children layer root\n");
        }

        double sp_dist = speed * dt;
        
        if ( p._v[0] != last_x || p._v[1] != last_y || sp_dist != 0 )
        {
            double ax = 0.0, ay = 0.0, bx = 0.0, by = 0.0;

            ax = p._v[0] - last_x;
            ay = p._v[1] - last_y;

            if (sp_dist > 0)
            {
                bx = cos(-direction * SD_DEGREES_TO_RADIANS) * sp_dist;
                by = sin(-direction * SD_DEGREES_TO_RADIANS) * sp_dist;
                GfLogDebug("sp_dist > 0\n");
            }

            double xoff = (ax + bx) / (2 * scale);
            double yoff = (ay + by) / (2 * scale);

            //const float layer_scale = layer_span / scale;
            base[0] += xoff;

            if ( base[0] > -10.0 && base[0] < 10.0 )
            {
                base[0] -= (int)base[0];
            } else
            {
                base[0] = 0.0;
            }

            base[1] += yoff;

            if ( base[1] > -10.0 && base[1] < 10.0 )
            {
                base[1] -= (int)base[1];
            } else
            {
                base[1] = 0.0;
            }

            setTextureOffset(base);
            last_pos = p;
            last_x = p._v[0];
            last_y = p._v[1];
        }
    }

    GfLogDebug("CloudLayer Alt = %.f\n", layer_asl);

    return true;
}

void SDCloudLayer::set_enable3dClouds(bool enable)
{
    if (/*layer3D->isDefined3D() &&*/ enable)
    {
        //cloud_root->setChildValue(layer3D->getNode(), true);
        cloud_root->setChildValue(layer_root.get(), false);
    } else
    {
        //cloud_root->setChildValue(layer3D->getNode(), false);
        cloud_root->setChildValue(layer_root.get(), true);
    }
}