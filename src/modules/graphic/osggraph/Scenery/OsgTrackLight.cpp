/***************************************************************************

    file                 : OsgTrackLight.cpp
    created              : Sun Oct 05 20:13:56 CEST 2014
    copyright            : (C) 2014 Xavier Bertaux
    email                : bertauxx@yahoo.fr
    version              : $Id: OsgTrackLight.cpp 2436 2014-10-05 20:22:43Z torcs-ng $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <osg/MatrixTransform>
//#include <osg/Node>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgDB/Registry>

#include <raceman.h>
#include <track.h>

#include "OsgScenery.h"

#ifndef TRUE
#define TRUE 1
#endif //TRUE
#ifndef FALSE
#define FALSE 0
#endif //FALSE


static osg::ref_ptr<osg::StateSet> light_states[SDTrackLight::SD_MAX_LIGHT];
static osg::ref_ptr<osg::StateSet> light_states2[SDTrackLight::SD_MAX_LIGHT];
static bool state_initialized = false;

typedef struct LightInfo
{
    int index;
    ssgVtxTable *light;
    //ssgSimpleState* onState;
    //ssgSimpleState* offState;
    ssgStateSelector *states;
    struct LightInfo *next;
} tLightInfo;

typedef struct TrackLights
{
    tLightInfo *st_red;
    tLightInfo *st_green;
    tLightInfo *st_yellow;
    tLightInfo *st_green_st;
} tTrackLights;

static tTrackLights trackLights;

//static void setOnOff( tLightInfo *light, char onoff );

static void calcNorm( osg::Vec3 topleft, osg::Vec3 bottomright, osg::Vec3 *result )
{
    (*result)[ 0 ] = bottomright[ 1 ] - topleft[ 1 ];
    (*result)[ 1 ] = topleft[ 0 ] - bottomright[ 0 ];
    (*result)[ 2 ] = 0.0f;
}

// make an StateSet for a lights given the named texture
static osg::StateSet *SDLightState(const std::string &path, const char* colorTexture)
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
    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    stateSet->setMode(GL_LIGHT0, osg::StateAttribute::OFF);

    return stateSet;
}

static void deleteStates()
{
    tStateList *current = statelist;
    tStateList *next;

    while( current )
    {
        next = current->next;

        if( current->state )
        {
            current->state->deRef();
            delete current->state;
        }
        free( current );
        current = next;
    }
}

static void addLight( tGraphicLightInfo *info, tTrackLights *lights, ssgBranch *parent )
{
    tLightInfo *trackLight;
    int states = 2;

    osg::ref_ptr<osg::Vec3Array> vertexArray = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec3Array> normalArray = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec4Array> colourArray = new osg::Vec4Array();
    osg::ref_ptr<osg::Vec2Array> texArray = new osg::Vec2Array();

    osg::Vec3 vertex;
    osg::Vec3 normal;
    osg::Vec4 colour;
    osg::Vec2 texcoord;

    colour[ 0 ] = info->red;
    colour[ 1 ] = info->green;
    colour[ 2 ] = info->blue;
    colour[ 3 ] = 1.0f;
    colourArray->push_back( colour );

    vertex[ 0 ] = info->topleft.x;
    vertex[ 1 ] = info->topleft.y;
    vertex[ 2 ] = info->topleft.z;
    vertexArray->push_back( vertex );
    vertex[ 2 ] = info->bottomright.z;
    vertexArray->push_back( vertex );
    vertex[ 0 ] = info->bottomright.x;
    vertex[ 1 ] = info->bottomright.y;
    vertex[ 2 ] = info->topleft.z; //?
    vertexArray->push_back( vertex );
    vertex[ 2 ] = info->topleft.z;
    vertex[ 2 ] = info->bottomright.z; //?
    vertexArray->push_back( vertex );

    calcNorm( vertexArray->get( 0 ), vertexArray->get( 2 ), &normal );
    normalArray->add( normal );
    normalArray->add( normal );
    normalArray->add( normal );
    normalArray->add( normal );

    texcoord[ 0 ] = 0.0f;
    texcoord[ 1 ] = 0.0f;
    texArray->push_back( texcoord );
    texcoord[ 0 ] = 0.0f;
    texcoord[ 1 ] = 1.0f;
    texArray->push_back( texcoord );
    texcoord[ 0 ] = 1.0f;
    texcoord[ 1 ] = 0.0f;
    texArray->push_back( texcoord );
    texcoord[ 0 ] = 1.0f;
    texcoord[ 1 ] = 1.0f;
    texArray->push_back( texcoord );

    if( info->role == GR_TRACKLIGHT_START_YELLOW || info->role == GR_TRACKLIGHT_POST_YELLOW ||
        info->role == GR_TRACKLIGHT_POST_GREEN || info->role == GR_TRACKLIGHT_POST_RED ||
        info->role == GR_TRACKLIGHT_POST_BLUE || info->role == GR_TRACKLIGHT_POST_WHITE ||
        info->role == GR_TRACKLIGHT_PIT_BLUE )
    {
        states = 3;
    }

    trackLight = (tLightInfo*)malloc( sizeof( tLightInfo ) );
    trackLight->index = info->index;
    trackLight->light = new ssgVtxTable( GL_TRIANGLE_STRIP, vertexArray, normalArray, texArray, colourArray );
    trackLight->states = new ssgStateSelector( states );
    trackLight->states->setStep( 0, createState( info->offTexture ) );
    trackLight->states->setStep( 1 + MAX( states - 2, info->index % 2 ), createState( info->onTexture ) );
    if( states == 3 )
        trackLight->states->setStep( 1 + ( info->index + 1 ) % 2, createState( info->offTexture ) );
    trackLight->states->selectStep( 0 );
    trackLight->light->setState( trackLight->states );
    //trackLight->onState = createState( info->onTexture );
    //trackLight->offState = createState( info->offTexture );

    switch( info->role )
    {
    case GR_TRACKLIGHT_START_RED:
        trackLight->next = lights->st_red;
        lights->st_red = trackLight;
        break;
    case GR_TRACKLIGHT_START_GREEN:
        trackLight->next = lights->st_green;
        lights->st_green = trackLight;
        break;
    case GR_TRACKLIGHT_START_GREENSTART:
        trackLight->next = lights->st_green_st;
        lights->st_green_st = trackLight;
        break;
    case GR_TRACKLIGHT_START_YELLOW:
        trackLight->next = lights->st_yellow;
        lights->st_yellow = trackLight;
        break;
    case GR_TRACKLIGHT_POST_YELLOW:
    case GR_TRACKLIGHT_POST_GREEN:
    case GR_TRACKLIGHT_POST_RED:
    case GR_TRACKLIGHT_POST_BLUE:
    case GR_TRACKLIGHT_POST_WHITE:
    case GR_TRACKLIGHT_PIT_RED:
    case GR_TRACKLIGHT_PIT_GREEN:
    case GR_TRACKLIGHT_PIT_BLUE:
    default:
        delete trackLight->light;
        free( trackLight );
        return;
    }

    parent->addKid( trackLight->light );
}

// Constructor
SDTrackLight::SDTrackLight( const string &tex_path, tSituation *s ) :
    lights_root(new osg::Switch),
    texture_path(tex_path),
    light_st(SD_LIGHT_OFF),
{
    lights_root->addChild(light_root.get(), true);

    osg::ref_ptr<osg::StateSet> rootSet = new osg::StateSet;
    rootSet = layer_root->getOrCreateStateSet();
    rootSet->setTextureAttribute(0, new osg::TexMat);
    rootSet->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
    // Combiner for fog color and cloud alpha

    rootSet->setTextureMode(1, GL_TEXTURE_2D, osg::StateAttribute::ON);

    osg::ref_ptr<osg::Image> dummyImage = new osg::Image;
    dummyImage->allocateImage(1, 1, 1, GL_LUMINANCE_ALPHA,
                              GL_UNSIGNED_BYTE);
    unsigned char* imageBytes = dummyImage->data(0, 0);
    imageBytes[0] = 255;
    imageBytes[1] = 255;
    osg::ref_ptr<osg::Texture2D> DefaultTexture = new osg::Texture2D;
    DefaultTexture->setImage(dummyImage);
    DefaultTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
    DefaultTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
    DefaultTexture->setDataVariance(osg::Object::STATIC);

    rootSet->setTextureAttributeAndModes(1, WhiteTexture, osg::StateAttribute::ON);

    rebuild();
}

// Destructor
SDTrackLight::~SDTrackLight()
{
}

// build the cloud object
void SDCloudLayer::rebuild()
{
    if ( !state_initialized )
    {
        state_initialized = true;

        GfOut("initializing cloud layers\n");

        osg::ref_ptr<osg::StateSet> state;
        state = SDLightState(texture_path, "start-light-red.png");
        layer_states[SD_LIGHT_RED] = state;

        state = SDLightState(texture_path, "start-light-green.png");
        layer_states[SD_LIGHT_GREEN] = state;

        state = SDLightState(texture_path, "start-light-yellow.png");
        layer_states[SD_LIGHT_YELLOW] = state;

        state = SDLightState(texture_path, "start-light-blue.png");
        layer_states[SD_LIGHT_BLUE] = state;

        state = SDLightState(texture_path, "start-light-white.png");
        layer_states[SD_LIGHT_WHITE] = state;

        state = SDLightState(texture_path, "start-light-off.png");
        layer_states[SD_LIGHT_OFF] = state;

    for (int i = 0; i < 4; i++)
    {
          if ( layer[i] != NULL )
          {
            layer_transform->removeChild(layer[i].get()); // automatic delete
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

    //OSGFIXME: true
    if ( layer_states[layer_coverage].valid() )
    {
        osg::CopyOp copyOp;    // shallow copy
        // render bin will be set in reposition
        osg::ref_ptr<osg::StateSet> stateSet = static_cast<osg::StateSet*>(layer_states2[layer_coverage]->clone(copyOp));
        stateSet->setDataVariance(osg::Object::DYNAMIC);
        group_top->setStateSet(stateSet.get());
        stateSet = static_cast<osg::StateSet*>(layer_states[layer_coverage]->clone(copyOp));
        stateSet->setDataVariance(osg::Object::DYNAMIC);
        group_bottom->setStateSet(stateSet.get());
    }
#if 0
    osgDB::writeNodeFile(*layer_transform,"/home/xavier/cloud.ac");
#endif
}

static void addLights( tTrackLights *lights, ssgBranch *parent )
{
    int xx;

    for( xx = 0; xx < grTrack->graphic.nb_lights; ++xx )
        addLight( &(grTrack->graphic.lights[ xx ]), lights, parent );
}

static void manageStartLights( tTrackLights *startlights, tSituation *s, char phase )
{
    static int  onoff_red_index = -1;
    static char onoff_red = FALSE;
    static char onoff_green = FALSE;
    static char onoff_green_st = FALSE;
    static char onoff_yellow = FALSE;
    static char onoff_phase = 1;
    char onoff;
    int current_index;
    char active = s->currentTime >= 0.0f && ( s->_totTime < 0.0f || s->currentTime < s->_totTime );
    tLightInfo *current;

    if( s->currentTime < 0.0f )
        current_index = (int)floor( s->currentTime * -10.0f );
    else
        current_index = -1;

    current = startlights->st_red;
    onoff = !active && s->_raceType != RM_TYPE_RACE;
    if( current_index != onoff_red_index || onoff != onoff_red )
    {
        onoff_red_index = current_index;
        onoff_red = onoff;
        while( current )
        {
            //setOnOff( current, onoff || ( current_index >= 0 && current_index < current->index ) );
            current->states->selectStep( ( onoff || ( current_index >= 0 && current_index < current->index ) ) ? 1 : 0 );
            current = current->next;
        }
    }

    current = startlights->st_green;
    onoff = active && s->_raceType != RM_TYPE_RACE;
    if( onoff_green != onoff )
    {
        onoff_green = onoff;
        while( current )
        {
            //setOnOff( current, onoff );
            current->states->selectStep( onoff ? 1 : 0 );
            current = current->next;
        }
    }

    current = startlights->st_green_st;
    onoff = active && ( s->_raceType != RM_TYPE_RACE || s->currentTime < 30.0f );
    if( onoff_green_st != onoff )
    {
        onoff_green_st = onoff;
        while( current )
        {
            //setOnOff( current, onoff );
            current->states->selectStep( onoff ? 1 : 0 );
            current = current->next;
        }
    }

    current = startlights->st_yellow;
    onoff = FALSE;
    if( onoff_yellow != onoff || ( onoff && phase != onoff_phase ) )
    {
        onoff_yellow = onoff;
        while( current )
        {
            //setOnOff( current, onoff ? ( phase + current->index ) % 2 : 0 );
            current->states->selectStep( onoff ? phase : 0 );
            current = current->next;
        }
    }

    onoff_phase = phase;
}

void grTrackLightInit()
{
    statelist = NULL;
    lightBranch = new ssgBranch();
    TrackLightAnchor->addKid( lightBranch );
    memset( &trackLights, 0, sizeof( tTrackLights ) );
    addLights( &trackLights, lightBranch );
}

void grTrackLightUpdate( tSituation *s )
{
    char phase = (char)( ( (int)floor( fmod( s->currentTime + 120.0f, (double)0.3f ) / 0.3f ) % 2 ) + 1 );
    manageStartLights( &trackLights, s, phase );
}

void grTrackLightShutdown()
{
    TrackLightAnchor->removeAllKids();
    //lightBranch->removeAllKids();
    /*delete lightBranch;*/ lightBranch = NULL;
    deleteStates();
}



/*SDTrackLights::SDTrackLights(void)
{
}

SDTrackLights::~SDTrackLights(void)
{
    _osgtracklight->removeChildren(0, _osgtracklight->getNumChildren());
    _osgtracklight = NULL;
}

void SDTrackLights::build(const std::string TrackPath)
{
        osg::ref_ptr<osg::StateSet> state;
}*/
