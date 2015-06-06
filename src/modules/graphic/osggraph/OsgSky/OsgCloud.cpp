/***************************************************************************

    file                     : OsgCloud.cpp
    created                  : Wen Mar 27 00:00:41 CEST 2013
    copyright                : (C) 2013 by Xavier Bertaux
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
#include <osg/TextureCubeMap>
#include <osg/TexMat>
#include <osg/Fog>
#include <osgDB/ReadFile>
#include <osgFX/BumpMapping>

#include <tgf.h>

#include "OsgUtil/OsgMath.h"
//#include "OsgNewcloud.h"
//#include "OsgCloudfield.h"
//#include "OsgStateAttributeFactory.h"
#include "OsgCloud.h"

using namespace osg;

static osg::ref_ptr<osg::StateSet> layer_states[SDCloudLayer::SD_MAX_CLOUD_COVERAGES];
static osg::ref_ptr<osg::StateSet> layer_states2[SDCloudLayer::SD_MAX_CLOUD_COVERAGES];
static osg::ref_ptr<osg::TextureCubeMap> cubeMap;
static bool state_initialized = false;

/*const std::string SGCloudLayer::SG_CLOUD_OVERCAST_STRING = "overcast";
const std::string SGCloudLayer::SG_CLOUD_BROKEN_STRING = "broken";
const std::string SGCloudLayer::SG_CLOUD_SCATTERED_STRING = "scattered";
const std::string SGCloudLayer::SG_CLOUD_FEW_STRING = "few";
const std::string SGCloudLayer::SG_CLOUD_CIRRUS_STRING = "cirrus";
const std::string SGCloudLayer::SG_CLOUD_CLEAR_STRING = "clear";*/

// make an StateSet for a cloud layer given the named texture
static osg::StateSet*
SDMakeState(const std::string &path, const char* colorTexture, const char* normalTexture)
{
    osg::StateSet *stateSet = new osg::StateSet;

    std::string TmpPath;
    TmpPath = path+"data/sky/"+colorTexture;
    GfOut("Path Sky cloud color texture = %s\n", TmpPath.c_str());
    osg::ref_ptr<osg::Image> image = osgDB::readImageFile(TmpPath);
    osg::ref_ptr<osg::Texture2D> texture = new osg::Texture2D(image.get());
    stateSet->setTextureAttributeAndModes(0, texture);
    stateSet->setTextureMode(0, GL_TEXTURE_2D, osg::StateAttribute::ON);

    osg::ref_ptr<osg::ShadeModel> Smooth = new osg::ShadeModel;
    Smooth->setMode(ShadeModel::SMOOTH);
    Smooth->setDataVariance(Object::STATIC);
    stateSet->setAttributeAndModes(Smooth);
    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

    osg::ref_ptr<osg::AlphaFunc> StandardAlphaFunc = new AlphaFunc;
    StandardAlphaFunc->setFunction(osg::AlphaFunc::GREATER);
    StandardAlphaFunc->setReferenceValue(0.01);
    StandardAlphaFunc->setDataVariance(Object::STATIC);
    stateSet->setAttributeAndModes(StandardAlphaFunc);

    osg::ref_ptr<BlendFunc> StandardBlendFunc = new BlendFunc;
    StandardBlendFunc->setSource(BlendFunc::SRC_ALPHA);
    StandardBlendFunc->setDestination(BlendFunc::ONE_MINUS_SRC_ALPHA);
    StandardBlendFunc->setDataVariance(Object::STATIC);
    stateSet->setAttributeAndModes(StandardBlendFunc);

    osg::Material* material = new osg::Material;
    material->setColorMode(osg::Material::AMBIENT_AND_DIFFUSE);
    material->setEmission(osg::Material::FRONT_AND_BACK,
                          osg::Vec4(0.05, 0.05, 0.05, 0));
    material->setSpecular(osg::Material::FRONT_AND_BACK,
                          osg::Vec4(0, 0, 0, 1));
    stateSet->setAttribute(material);
    stateSet->setMode(GL_FOG, osg::StateAttribute::OFF);

    stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::ON);

    stateSet->setMode(GL_LIGHT0, osg::StateAttribute::OFF);

    //If the normal texture is given prepare a bumpmapping enabled state
    if (normalTexture)
    {
       TmpPath = path+"data/sky/"+normalTexture;
       GfOut("Path Cloud normal texture = %s\n", TmpPath.c_str());
       osg::ref_ptr<osg::Image> image = osgDB::readImageFile(TmpPath);
       osg::ref_ptr<osg::Texture2D> texture = new osg::Texture2D(image.get());
       stateSet->setTextureAttributeAndModes(2, texture);
       stateSet->setTextureMode(2, GL_TEXTURE_2D, osg::StateAttribute::ON);
    }

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
    layer_visibility(25.0),
    layer_coverage(SD_CLOUD_CLEAR),
    scale(4000.0),
    speed(0.0),
    direction(0.0),
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

    osg::Image *dummyImage = new osg::Image;
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
    SDRandom();
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
            setSpan_m( 40000 );
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
    osg::StateAttribute* attr = layer_root->getStateSet()
        ->getTextureAttribute(0, osg::StateAttribute::TEXMAT);
    osg::TexMat* texMat = dynamic_cast<osg::TexMat*>(attr);
    if (!texMat)
        return;
    texMat->setMatrix(osg::Matrix::translate(offset[0], offset[1], 0.0));
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

        cubeMap = new osg::TextureCubeMap;
        cubeMap->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
        cubeMap->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
        cubeMap->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
        cubeMap->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
        cubeMap->setWrap(osg::Texture::WRAP_R, osg::Texture::CLAMP_TO_EDGE);

        const int size = 32;
        const float half_size = 16.0f;
        const float offset = 0.5f;
        osg::Vec3 zero_normal(0.5, 0.5, 0.5);

        osg::Image* image = new osg::Image;
        image->allocateImage(size, size, 1, GL_RGB, GL_UNSIGNED_BYTE);
        unsigned char *ptr = image->data(0, 0);

        for (int j = 0; j < size; j++ )
        {
            for (int i = 0; i < size; i++ )
            {
                osg::Vec3 tmp(half_size, -( j + offset - half_size ),
                          -( i + offset - half_size ) );
                tmp.normalize();
                tmp = tmp*0.5 - zero_normal;
            
                *ptr++ = (unsigned char)( tmp[ 0 ] * 255 );
                *ptr++ = (unsigned char)( tmp[ 1 ] * 255 );
                *ptr++ = (unsigned char)( tmp[ 2 ] * 255 );
            }
        }

        cubeMap->setImage(osg::TextureCubeMap::POSITIVE_X, image);

        image = new osg::Image;
        image->allocateImage(size, size, 1, GL_RGB, GL_UNSIGNED_BYTE);
        ptr = image->data(0, 0);
        for (int j = 0; j < size; j++ )
        {
            for (int i = 0; i < size; i++ )
            {
                osg::Vec3 tmp(-half_size, -( j + offset - half_size ),
                          ( i + offset - half_size ) );
                tmp.normalize();
                tmp = tmp*0.5 - zero_normal;
            
                *ptr++ = (unsigned char)( tmp[ 0 ] * 255 );
                *ptr++ = (unsigned char)( tmp[ 1 ] * 255 );
                *ptr++ = (unsigned char)( tmp[ 2 ] * 255 );
            }
        }

        cubeMap->setImage(osg::TextureCubeMap::NEGATIVE_X, image);

        image = new osg::Image;
        image->allocateImage(size, size, 1, GL_RGB, GL_UNSIGNED_BYTE);
        ptr = image->data(0, 0);

        for (int j = 0; j < size; j++ )
        {
            for (int i = 0; i < size; i++ )
            {
                osg::Vec3 tmp(( i + offset - half_size ), half_size,
                          ( j + offset - half_size ) );
                tmp.normalize();
                tmp = tmp*0.5 - zero_normal;
            
                *ptr++ = (unsigned char)( tmp[ 0 ] * 255 );
                *ptr++ = (unsigned char)( tmp[ 1 ] * 255 );
                *ptr++ = (unsigned char)( tmp[ 2 ] * 255 );
            }
        }

        cubeMap->setImage(osg::TextureCubeMap::POSITIVE_Y, image);

        image = new osg::Image;
        image->allocateImage(size, size, 1, GL_RGB, GL_UNSIGNED_BYTE);
        ptr = image->data(0, 0);

        for (int j = 0; j < size; j++ )
        {
            for (int i = 0; i < size; i++ )
            {
                osg::Vec3 tmp(( i + offset - half_size ), -half_size,
                          -( j + offset - half_size ) );
                tmp.normalize();
                tmp = tmp*0.5 - zero_normal;

                *ptr++ = (unsigned char)( tmp[ 0 ] * 255 );
                *ptr++ = (unsigned char)( tmp[ 1 ] * 255 );
                *ptr++ = (unsigned char)( tmp[ 2 ] * 255 );
            }
        }

        cubeMap->setImage(osg::TextureCubeMap::NEGATIVE_Y, image);

        image = new osg::Image;
        image->allocateImage(size, size, 1, GL_RGB, GL_UNSIGNED_BYTE);
        ptr = image->data(0, 0);

        for (int j = 0; j < size; j++ )
        {
            for (int i = 0; i < size; i++ )
            {
                osg::Vec3 tmp(( i + offset - half_size ),
                          -( j + offset - half_size ), half_size );
                tmp.normalize();
                tmp = tmp*0.5 - zero_normal;
            
                *ptr++ = (unsigned char)( tmp[ 0 ] * 255 );
                *ptr++ = (unsigned char)( tmp[ 1 ] * 255 );
                *ptr++ = (unsigned char)( tmp[ 2 ] * 255 );
            }
        }

        cubeMap->setImage(osg::TextureCubeMap::POSITIVE_Z, image);

        image = new osg::Image;
        image->allocateImage(size, size, 1, GL_RGB, GL_UNSIGNED_BYTE);
        ptr = image->data(0, 0);

        for (int j = 0; j < size; j++ )
        {
            for (int i = 0; i < size; i++ )
            {
                osg::Vec3 tmp(-( i + offset - half_size ),
                          -( j + offset - half_size ), -half_size );
                tmp.normalize();
                tmp = tmp*0.5 - zero_normal;
                *ptr++ = (unsigned char)( tmp[ 0 ] * 255 );
                *ptr++ = (unsigned char)( tmp[ 1 ] * 255 );
                *ptr++ = (unsigned char)( tmp[ 2 ] * 255 );
            }
        }

        cubeMap->setImage(osg::TextureCubeMap::NEGATIVE_Z, image);

        osg::StateSet* state;
        state = SDMakeState(texture_path, "overcast.png", "overcast_n.png");
        layer_states[SD_CLOUD_OVERCAST] = state;
        state = SDMakeState(texture_path, "overcast_top.png", "overcast_top_n.png");
        layer_states2[SD_CLOUD_OVERCAST] = state;

        state = SDMakeState(texture_path, "broken.png", "broken_n.png");
        layer_states[SD_CLOUD_BROKEN] = state;
        layer_states2[SD_CLOUD_BROKEN] = state;

        state = SDMakeState(texture_path, "scattered.png", "scattered_n.png");
        layer_states[SD_CLOUD_SCATTERED] = state;
        layer_states2[SD_CLOUD_SCATTERED] = state;

        state = SDMakeState(texture_path, "few.png", "few_n.png");
        layer_states[SD_CLOUD_FEW] = state;
        layer_states2[SD_CLOUD_FEW] = state;

        state = SDMakeState(texture_path, "cirrus.png", "cirrus_n.png");
        layer_states[SD_CLOUD_CIRRUS] = state;
        layer_states2[SD_CLOUD_CIRRUS] = state;

        layer_states[SD_CLOUD_CLEAR] = 0;
        layer_states2[SD_CLOUD_CLEAR] = 0;

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
    }

    scale = 4000.0;

    setTextureOffset(base);

    const float layer_scale = layer_span / scale;
    const float mpi = SD_PI/4;
    const float layer_to_core = (SD_EARTH_RAD * 1000 + layer_asl);
    const float layer_angle = 0.5 * layer_span / layer_to_core;
    const float border_to_core = layer_to_core * cos(layer_angle);
    const float alt_diff = /*layer_asl * 1.5f;*/layer_to_core - border_to_core;
    
    for (int i = 0; i < 4; i++)
    {
        if ( layer[i] != NULL )
        {
            layer_transform->removeChild(layer[i].get());
        }
      
        vl[i] = new osg::Vec3Array;
        cl[i] = new osg::Vec4Array;
        tl[i] = new osg::Vec2Array;
            
        osg::Vec3 vertex(layer_span*(i-2)/2, -layer_span,
                         alt_diff * (sin(i*mpi) - 2));
        osg::Vec2 tc(layer_scale * i/4, 0.0f);
        osg::Vec4 color(cloudColors[0], (i == 0) ? 0.0f : 0.15f);
      
        cl[i]->push_back(color);
        vl[i]->push_back(vertex);
        tl[i]->push_back(tc);
      
        for (int j = 0; j < 4; j++)
        {
            vertex = osg::Vec3(layer_span*(i-1)/2, layer_span*(j-2)/2,
                               alt_diff * (sin((i+1)*mpi) + sin(j*mpi) - 2));
            tc = osg::Vec2(layer_scale * (i+1)/4, layer_scale * j/4);
            color = osg::Vec4(cloudColors[0],
                             ( (j == 0) || (i == 3)) ?
                             ( (j == 0) && (i == 3)) ? 0.0f : 0.15f : 1.0f );
        
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
      
        vertex = osg::Vec3(layer_span*(i-1)/2, layer_span, alt_diff * (sin((i+1)*mpi) - 2));
        tc = osg::Vec2(layer_scale * (i+1)/4, layer_scale);      
        color = osg::Vec4(cloudColors[0], (i == 3) ? 0.0f : 0.15f );
      
        cl[i]->push_back( color );
        vl[i]->push_back( vertex );
        tl[i]->push_back( tc );
      
        osg::Geometry* geometry = new osg::Geometry;
        geometry->setUseDisplayList(false);
        geometry->setVertexArray(vl[i].get());
        geometry->setNormalBinding(osg::Geometry::BIND_OFF);
        geometry->setColorArray(cl[i].get());
        geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
        geometry->setTexCoordArray(0, tl[i].get());
        geometry->addPrimitiveSet(new osg::DrawArrays(GL_TRIANGLE_STRIP, 0, vl[i]->size()));
        layer[i] = new osg::Geode;
      
        std::stringstream sstr;
        sstr << "Cloud Layer (" << i << ")";
        geometry->setName(sstr.str());
        layer[i]->setName(sstr.str());
        layer[i]->addDrawable(geometry);
        layer_transform->addChild(layer[i].get());
    }

    if ( layer_states[layer_coverage].valid() )
    {
        osg::CopyOp copyOp;
        osg::StateSet* stateSet = static_cast<osg::StateSet*>(layer_states2[layer_coverage]->clone(copyOp));
        stateSet->setDataVariance(osg::Object::DYNAMIC);
        group_top->setStateSet(stateSet);
        stateSet = static_cast<osg::StateSet*>(layer_states[layer_coverage]->clone(copyOp));
        stateSet->setDataVariance(osg::Object::DYNAMIC);
        group_bottom->setStateSet(stateSet);

        GfOut("layer_states[layer_coverage]\n");
    }
}

bool SDCloudLayer::repaint( const osg::Vec3f& fog_color )
{
    osg::Vec4f combineColor(fog_color, cloud_alpha);
    osg::TexEnvCombine* combiner
        = dynamic_cast<osg::TexEnvCombine*>(layer_root->getStateSet()
                                            ->getTextureAttribute(1, osg::StateAttribute::TEXENV));
    combiner->setConstantColor(combineColor);

    return true;
}

bool SDCloudLayer::reposition( const osg::Vec3f& p, double dt )
{
    if (getCoverage() != SDCloudLayer::SD_CLOUD_CLEAR)
    {
        osg::Vec3 asl_offset( p );
        asl_offset.normalize();
        if ( p.y() <= layer_asl )
        {
            asl_offset *= layer_asl;
        } else
        {
            asl_offset *= layer_asl + layer_thickness;
        }

        asl_offset += p;

        osg::Matrix T, LON, LAT;
        T.makeTranslate( asl_offset );
        //LON.makeRotate(lon, osg::Vec3(0, 0, 1));
        //LAT.makeRotate(90.0 * SD_DEGREES_TO_RADIANS - lat, osg::Vec3(0, 1, 0));

        layer_transform->setMatrix( T );

        group_bottom->getStateSet()->setRenderBinDetails(-(int)layer_asl, "RenderBin");
        group_top->getStateSet()->setRenderBinDetails((int)layer_asl, "RenderBin");
        if ( alt <= layer_asl )
        {
            layer_root->setSingleChildOn(0);
        } else if ( alt >= layer_asl + layer_thickness )
        {
            layer_root->setSingleChildOn(1);
        } else
        {
            layer_root->setAllChildrenOff();
        }

        double sp_dist = speed * dt;
        
        if ( p.x() != last_x || p.y() != last_y || sp_dist != 0 )
        {
            double ax = 0.0, ay = 0.0, bx = 0.0, by = 0.0;

            if (sp_dist > 0)
            {
                bx = cos((180.0-direction) * SD_DEGREES_TO_RADIANS) * sp_dist;
                by = sin((180.0-direction) * SD_DEGREES_TO_RADIANS) * sp_dist;
            }

            double xoff = (ax + bx) / (2 * scale);
            double yoff = (ay + by) / (2 * scale);

            const float layer_scale = layer_span / scale;

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
            scale = layer_scale;
        }
    }

    //layer3D->reposition( p, up, lon, lat, dt, layer_asl, speed, direction);

    return true;
}

void SDCloudLayer::set_enable3dClouds(bool enable)
{
    if (/*layer3D->isDefined3D() &&*/ enable)
    {
        //cloud_root->setChildValue(layer3D->getNode(), true);
        cloud_root->setChildValue(layer_root.get(),   false);
    } else
    {
        //cloud_root->setChildValue(layer3D->getNode(), false);
        cloud_root->setChildValue(layer_root.get(),   true);
    }
}
