/***************************************************************************

    file                 : grbackground.cpp
    created              : Thu Nov 25 21:09:40 CEST 2010
    copyright            : (C) 2010 by Jean-Philippe Meuret
    email                : http://www.speed-dreams.org
    version              : $Id: grbackground.cpp 6164 2015-10-04 23:14:42Z torcs-ng $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <ctime>

#include <robottools.h>	//RtXXX()
#include <portability.h> // snprintf
#include <glfeatures.h>

#include <plib/ssgAux.h>

#include "grscene.h"
#include "grloadac.h"
#include "grmain.h"
#include "grcam.h"	//cGrBackgroundCam
#include "grutil.h"
#include "grSky.h"
#include "grbackground.h"
#include "grMoonpos.h"

// Some exported constants.
const tdble grSkyDomeNeutralFOVDistance = 20000.0f; // Not the smallest, a medium one.

static const double m_log01 = -log( 0.01 );
static const double sqrt_m_log01 = sqrt( m_log01 );

// Some private constants.
static const int NbBackgroundFaces = 36; //Background faces
static const float BackgroundDistance = 1.0f;

static const unsigned SkyDomeDistThresh = 12000; // No dynamic sky below that value.

static const int NMaxStars = 3200;
static const int NMaxPlanets = 0; //No planets displayed for the moment
static const int NMaxCloudLayers = 3;

static const sgVec4 Black            = { 0.0f, 0.0f, 0.0f, 1.0f } ;
static const sgVec4 White            = { 1.0f, 1.0f, 1.0f, 1.0f } ;
static const sgVec4 TranslucentWhite = { 1.0f, 1.0f, 1.0f, 0.8f } ;

static const sgVec4 BaseSkyColor    = { 0.31f, 0.43f, 0.69f, 1.0f };

static int NStars = 0;
static int NPlanets = 0;
static int cloudsTextureIndex = 0;

static const int CloudsTextureIndices[TR_CLOUDS_FULL+1] = { 1, 3, 5, 7, 8 };
static const int NCloudsTextureIndices = sizeof(CloudsTextureIndices) / sizeof(int);

static const char* AEnvShadowKeys[] =
{ "no-cloud", "few-clouds", "scarce-clouds", "many-clouds", "full-cover",
  "full-cover-rain", "night" };
static const int NEnvShadowFullCoverRainIndex = 5; // Index in AEnvShadowKeys
static const int NEnvShadowNightIndex = 6; // Index in AEnvShadowKeys

// Some exported global variables.
ssgStateSelector* grEnvSelector = 0;
cgrMultiTexState* grEnvState = 0;
cgrMultiTexState* grEnvShadowState = 0;
cgrMultiTexState* grEnvShadowStateOnCars = 0;

unsigned grSkyDomeDistance = 0;
static unsigned grNbCloudLayers = 0;

// Some private global variables.
//static int grDynamicWeather = 0;
static bool grDynamicSkyDome = false;
static int grBackgroundType = 0;
static float grSunDeclination = 0.0f;
static float grMoonDeclination = 0.0f;
static float grMax_Visibility = 0.0f;
static double grVisibility = 0.0f;

static ssgBranch *SunAnchor = NULL;

static ssgRoot *TheBackground = NULL;
static ssgTransform *TheSun = NULL;

static cGrSky *TheSky = NULL;

static sgdVec3 *AStarsData = NULL;
static sgdVec3 *APlanetsData = NULL;

static sgVec4 SkyColor;
static sgVec4 BaseFogColor;
static sgVec4 FogColor;
static sgVec4 CloudsColor;

static sgVec4 SceneAmbiant;
static sgVec4 SceneDiffuse;
static sgVec4 SceneSpecular;

/**
 * grInitBackground
 * Initialize the background (mainly the sky).
 */
void
grInitBackground()
{
    char buf[256];
    void *hndl = grTrackHandle;
    ssgLight *light = ssgGetLight(0);

    // If no realistic sky dome requested, or if the track skyversion doesn't support it,
    // we set up a static - texture-based - background
    if (!grSkyDomeDistance )
    {
        GfLogInfo("Setting up static background (mono-texture sky and landscape)\n");

        GLfloat matSpecular[]       = {0.3, 0.3, 0.3, 1.0};
        GLfloat matShininess[]      = {50.0};
        GLfloat lightPosition[]     = {0, 0, 200, 0.0};
        GLfloat lightModelAmbient[] = {0.2, 0.2, 0.2, 1.0};
        GLfloat lightModelDiffuse[] = {0.8, 0.8, 0.8, 1.0};
        GLfloat fogColor[]        = {0.0, 0.0, 0.0, 0.5};

        matSpecular[0] = GfParmGetNum(hndl, TRK_SECT_GRAPH, TRK_ATT_SPEC_R, NULL, matSpecular[0]);
        matSpecular[1] = GfParmGetNum(hndl, TRK_SECT_GRAPH, TRK_ATT_SPEC_G, NULL, matSpecular[1]);
        matSpecular[2] = GfParmGetNum(hndl, TRK_SECT_GRAPH, TRK_ATT_SPEC_B, NULL, matSpecular[2]);

        lightModelAmbient[0] = GfParmGetNum(hndl, TRK_SECT_GRAPH, TRK_ATT_AMBIENT_R, NULL, lightModelAmbient[0]);
        lightModelAmbient[1] = GfParmGetNum(hndl, TRK_SECT_GRAPH, TRK_ATT_AMBIENT_G, NULL, lightModelAmbient[1]);
        lightModelAmbient[2] = GfParmGetNum(hndl, TRK_SECT_GRAPH, TRK_ATT_AMBIENT_B, NULL, lightModelAmbient[2]);

        lightModelDiffuse[0] = GfParmGetNum(hndl, TRK_SECT_GRAPH, TRK_ATT_DIFFUSE_R, NULL, lightModelDiffuse[0]);
        lightModelDiffuse[1] = GfParmGetNum(hndl, TRK_SECT_GRAPH, TRK_ATT_DIFFUSE_G, NULL, lightModelDiffuse[1]);
        lightModelDiffuse[2] = GfParmGetNum(hndl, TRK_SECT_GRAPH, TRK_ATT_DIFFUSE_B, NULL, lightModelDiffuse[2]);

        matShininess[0] = GfParmGetNum(hndl, TRK_SECT_GRAPH, TRK_ATT_SHIN, NULL, matShininess[0]);

        lightPosition[0] = GfParmGetNum(hndl, TRK_SECT_GRAPH, TRK_ATT_LIPOS_X, NULL, lightPosition[0]);
        lightPosition[1] = GfParmGetNum(hndl, TRK_SECT_GRAPH, TRK_ATT_LIPOS_Y, NULL, lightPosition[1]);
        lightPosition[2] = GfParmGetNum(hndl, TRK_SECT_GRAPH, TRK_ATT_LIPOS_Z, NULL, lightPosition[2]);

        glShadeModel(GL_SMOOTH);

        light->setPosition(lightPosition[0],lightPosition[1],lightPosition[2]);
        light->setColour(GL_AMBIENT,lightModelAmbient);
        light->setColour(GL_DIFFUSE,lightModelDiffuse);
        light->setColour(GL_SPECULAR,matSpecular);
        light->setSpotAttenuation(0.0, 0.0, 0.0);

        sgCopyVec3 (fogColor, grTrack->graphic.bgColor);
        sgScaleVec3 (fogColor, 0.8);
        glFogi(GL_FOG_MODE, GL_LINEAR);
        glFogfv(GL_FOG_COLOR, fogColor);
        glFogf(GL_FOG_DENSITY, 0.05);
        glHint(GL_FOG_HINT, GL_DONT_CARE);

        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);
        glEnable(GL_DEPTH_TEST);

        if (!TheSun && grTrack->local.rain == 0)
        {
            ssgaLensFlare *sun_obj = new ssgaLensFlare();
            TheSun = new ssgTransform;
            TheSun->setTransform(lightPosition);
            TheSun->addKid(sun_obj);
            SunAnchor->addKid(TheSun);
        }
    }

    // If realistic sky dome is requested,
    // we create the Sun, the Moon, some stars and the clouds
    else
    {
        GfLogInfo("Setting up realistic %s sky dome :\n", grDynamicSkyDome ? "dynamic" : "static");

        //ssgSetNearFar(1, grSkyDomeDistance);

        // Determine time of day (seconds since 00:00).
        const int timeOfDay = (int)grTrack->local.timeofday;

        // Add random stars (can't optimize from real time of day,
        // in case grDynamicSkyDome - that is dynamic time of day -
        // and also simply because it'd be complicated to take care of actual lattitude).
        NStars = NMaxStars;
        if (AStarsData)
            delete [] AStarsData;
        AStarsData = new sgdVec3[NStars];
        for(int i= 0; i < NStars; i++)
        {
            AStarsData[i][0] = grRandom() * PI;
            AStarsData[i][1] = grRandom() * PI;
            AStarsData[i][2] = grRandom();
        }//for i

        GfLogInfo("  Stars (random) : %d\n", NStars);

        //No planets
        NPlanets = 0;
        APlanetsData = NULL;
        //float visibility = 0;

        const double domeSizeRatio = grSkyDomeDistance / 80000.0;

        GfLogInfo("  Planets : %d\n", NPlanets);

        //Build the sky
        TheSky	= new cGrSky;
        TheSky->build(grSkyDomeDistance, grSkyDomeDistance, 2000 * domeSizeRatio, grSkyDomeDistance, 2000 * domeSizeRatio, grSkyDomeDistance,
            NPlanets, APlanetsData, NStars, AStarsData );

        //Add the Sun itself
        GLfloat sunAscension = grTrack->local.sunascension;
        grSunDeclination = (float)(15 * (double)timeOfDay / 3600 - 90.0);

        TheSky->setSD( DEG2RAD(grSunDeclination));
        TheSky->setSRA( sunAscension );

        GfLogInfo("  Sun : time of day = %02d:%02d:%02d (declination = %.1f deg), "
              "ascension = %.1f deg\n",
                  timeOfDay / 3600, (timeOfDay % 3600) / 60, timeOfDay % 60,
                  grSunDeclination, RAD2DEG(sunAscension));

        /*if ( grSunDeclination > 180 )
            grMoonDeclination = 3.0 + (rand() % 40);
        else
            grMoonDeclination = (rand() % 270);*/

        grMoonDeclination = grUpdateMoonPos(timeOfDay);

        const float moonAscension = grTrack->local.sunascension;

        TheSky->setMD( DEG2RAD(grMoonDeclination) );
        TheSky->setMRA( DEG2RAD(moonAscension) );

        GfLogInfo("  Moon : declination = %.1f deg, ascension = %.1f deg\n",
                  grMoonDeclination, moonAscension);

        // Add the cloud layers
        // TODO :
        //  * Why does thickness and transition get greater as the sky dome distance decreases ?
        //  * More/different cloud layers for each rain strength value (only 2 as for now) ?
        cloudsTextureIndex = CloudsTextureIndices[grTrack->local.clouds];

        cGrCloudLayer *cloudLayers[NMaxCloudLayers];
        snprintf(buf, sizeof(buf), "data/textures/scattered%d.rgba", cloudsTextureIndex);
        if (grTrack->local.rain > 0)
        {
            GfLogInfo("  Cloud cover : Rainy, 1 layer\n");

            cloudLayers[0] = TheSky->addCloud(buf, grSkyDomeDistance, 650,
                                              400 / domeSizeRatio, 400 / domeSizeRatio);
            cloudLayers[0]->setSpeed(300);
            cloudLayers[0]->setDirection(60);
        }
        else if (grNbCloudLayers == 1)
        {
            GfLogInfo("  Cloud cover : 3 layers\n");

            int wind = (rand() % 200) + 100;

            cloudLayers[0] = TheSky->addCloud(buf, grSkyDomeDistance, 2550,
                                              100 / domeSizeRatio, 100 / domeSizeRatio);
            cloudLayers[0]->setSpeed(wind);
            cloudLayers[0]->setDirection(45);

            GfLogInfo("   * layer 1 : speed=60, direction=45, texture=%s\n", buf);

        }
        else if (grNbCloudLayers == 2)
        {
            GfLogInfo("  Cloud cover : 2 layers\n");

            snprintf(buf, sizeof(buf), "data/textures/scattered%d.rgba", 1);
            cloudLayers[0] = TheSky->addCloud(buf, grSkyDomeDistance, 3000,
                                              100 / domeSizeRatio, 100 / domeSizeRatio);
            cloudLayers[0]->setSpeed(30);
            cloudLayers[0]->setDirection(40);

            GfLogInfo("   * layer 1 : speed=30, direction=40, texture=%s\n", buf);

            snprintf(buf, sizeof(buf), "data/textures/scattered%d.rgba", cloudsTextureIndex);
            cloudLayers[1] = TheSky->addCloud(buf, grSkyDomeDistance, 2000,
                                              100 / domeSizeRatio, 100 / domeSizeRatio);
            cloudLayers[1]->setSpeed(60);
            cloudLayers[1]->setDirection(45);

            GfLogInfo("   * layer 2 : speed=60, direction=45, texture=%s\n", buf);

        }
        else if (grNbCloudLayers == 3)
        {
            GfLogInfo("  Cloud cover : 3 layers\n");

            snprintf(buf, sizeof(buf), "data/textures/scattered%d.rgba", 1);
            cloudLayers[0] = TheSky->addCloud(buf, grSkyDomeDistance, 3000,
                                              100 / domeSizeRatio, 100 / domeSizeRatio);
            int wind = (rand() % 40) + 60;
            cloudLayers[0]->setSpeed(wind);
            cloudLayers[0]->setDirection(40);

            GfLogInfo("   * layer 1 : speed=30, direction=40, texture=%s\n", buf);

            snprintf(buf, sizeof(buf), "data/textures/scattered%d.rgba", cloudsTextureIndex);
            cloudLayers[1] = TheSky->addCloud(buf, grSkyDomeDistance, 2000,
                                              100 / domeSizeRatio, 100 / domeSizeRatio);
            cloudLayers[1]->setSpeed(60);
            cloudLayers[1]->setDirection(45);

            GfLogInfo("   * layer 2 : speed=60, direction=45, texture=%s\n", buf);

            snprintf(buf, sizeof(buf), "data/textures/scattered%d.rgba", cloudsTextureIndex);
            cloudLayers[2] = TheSky->addCloud(buf, grSkyDomeDistance, 1000,
                                              100 / domeSizeRatio, 100 / domeSizeRatio);
            cloudLayers[2]->setSpeed(80);
            cloudLayers[2]->setDirection(45);

            GfLogInfo("   * layer 3 : speed=80, direction=45, texture=%s\n", buf);
        }

        // Set up the light source to the Sun position.
        sgCoord sunPosition;
        TheSky->getSunPos(&sunPosition);
        light->setPosition(sunPosition.xyz);

        // Initialize the whole sky dome.
        sgVec3 viewPos;
        sgSetVec3(viewPos, grWrldX/2, grWrldY/2, 0);
        TheSky->repositionFlat( viewPos, 0, 0);

        //Setup visibility according to rain if any
        // TODO: Does visibility really decrease when rain gets heavier ????
        //float visibility = 0.0f;
        switch (grTrack->local.rain)
        {
            case TR_RAIN_NONE:
                //visibility = 0.0f;
                grVisibility = grMax_Visibility;
                break;
            case TR_RAIN_LITTLE:
                //visibility = 400.0f;
                grVisibility = 800.0;
                break;
            case TR_RAIN_MEDIUM:
                //visibility = 500.0f;
                grVisibility = 600.0;
                break;
            case TR_RAIN_HEAVY:
                //visibility = 550.0f;
                grVisibility = 400.0;
                break;
            default:
                GfLogWarning("Unsupported rain strength value %d (assuming none)",
                             grTrack->local.rain);
                grVisibility = 12000.0;
                break;
        }//switch Rain

        //TheSky->modifyVisibility( visibility, 0);
        TheSky->setVisibility( grVisibility ); // Visibility in meters

        //Setup overall light level according to rain if any
        grUpdateLight();

        glLightModelfv( GL_LIGHT_MODEL_AMBIENT, Black);
        ssgGetLight(0) -> setColour( GL_AMBIENT, SceneAmbiant);
        ssgGetLight(0) -> setColour( GL_DIFFUSE, SceneDiffuse);
        ssgGetLight(0) -> setColour( GL_SPECULAR, SceneSpecular);
    }//else grSkyDomeDistance

    /* GUIONS GL_TRUE */
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,GL_FALSE);

#ifdef GL_SEPARATE_SPECULAR_COLOR
    glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL,GL_SEPARATE_SPECULAR_COLOR);
#else
    #ifdef GL_SEPARATE_SPECULAR_COLOR_EXT
    glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL_EXT,GL_SEPARATE_SPECULAR_COLOR_EXT);
    #	endif
#endif
}//grInitBackground


void
grLoadBackgroundGraphicsOptions()
{
    // Sky dome / background.
    grSkyDomeDistance =
        (unsigned)(GfParmGetNum(grHandle, GR_SCT_GRAPHIC, GR_ATT_SKYDOMEDISTANCE, 0, 0) + 0.5);
    if (grSkyDomeDistance > 0 && grSkyDomeDistance < SkyDomeDistThresh)
        grSkyDomeDistance = SkyDomeDistThresh; // If user enabled it (>0), must be at least the threshold.

    grDynamicSkyDome = grSkyDomeDistance > 0 && strcmp(GfParmGetStr(grHandle, GR_SCT_GRAPHIC, GR_ATT_DYNAMICSKYDOME, GR_ATT_DYNAMICSKYDOME_DISABLED), GR_ATT_DYNAMICSKYDOME_ENABLED) == 0;

    GfLogInfo("Graphic options : Sky dome : distance = %u m, dynamic = %s\n",
              grSkyDomeDistance, grDynamicSkyDome ? "true" : "false");

    // Dynamic weather.
    //grDynamicWeather = GfParmGetNum(grHandle, GR_SCT_GRAPHIC, GR_ATT_grDynamicWeather, (char*)NULL, grDynamicWeather);

    // Cloud layers.
    grNbCloudLayers =
        (unsigned)(GfParmGetNum(grHandle, GR_SCT_GRAPHIC, GR_ATT_CLOUDLAYER, 0, 0) + 0.5);

    GfLogInfo("Graphic options : Number of cloud layers : %u\n", grNbCloudLayers);

    grMax_Visibility =
        (unsigned)(GfParmGetNum(grHandle, GR_SCT_GRAPHIC, GR_ATT_VISIBILITY, 0, 0));


}

void
grLoadBackground()
{
    char buf[256];
    int			i;
    float		x, y, z1, z2;
    double		alpha;
    float		texLen;
    ssgSimpleState	*envst;
    sgVec3		vtx;
    sgVec4		clr;
    sgVec3		nrm;
    sgVec2		tex;
    ssgVtxTable 	*bg;
    ssgVertexArray	*bg_vtx;
    ssgTexCoordArray	*bg_tex;
    ssgColourArray	*bg_clr;
    ssgNormalArray	*bg_nrm;
    ssgSimpleState	*bg_st;

    snprintf(buf, sizeof(buf), "tracks/%s/%s;data/img;data/textures;.",
            grTrack->category, grTrack->internalname);
    grFilePath = buf;
    grGammaValue = 1.8;
    grMipMap = 0;

    // Load static background if no sky dome.
    const tTrackGraphicInfo *graphic = &grTrack->graphic;
    TheBackground = 0;
    if (!grSkyDomeDistance )
    {
        GfLogInfo("Loading static background (type %d)\n", graphic->bgtype);

        glClearColor(graphic->bgColor[0], graphic->bgColor[1], graphic->bgColor[2], 1.0);

        TheBackground = new ssgRoot();
        clr[0] = clr[1] = clr[2] = 1.0 / (1.0 + 1.0 * grTrack->local.rain);
        clr[3] = 1.0;
        nrm[0] = nrm[2] = 0.0;
        nrm[1] = 1.0;

        z1 = -0.5;
        z2 = 1.0;

        grBackgroundType = graphic->bgtype;
        switch (grBackgroundType) {
            case TR_BACKGROUND_TYPE_0: //-----------------------------------------------------------
                bg_vtx = new ssgVertexArray(NbBackgroundFaces + 1);
                bg_tex = new ssgTexCoordArray(NbBackgroundFaces + 1);
                bg_clr = new ssgColourArray(1);
                bg_nrm = new ssgNormalArray(1);
                bg_clr->add(clr);
                bg_nrm->add(nrm);

                for (i = 0; i < NbBackgroundFaces + 1; i++) {
                    alpha = (float)i * 2 * PI / (float)NbBackgroundFaces;
                    texLen = (float)i / (float)NbBackgroundFaces;

                    x = BackgroundDistance * cos(alpha);
                    y = BackgroundDistance * sin(alpha);

                    vtx[0] = x;
                    vtx[1] = y;
                    vtx[2] = z1;
                    bg_vtx->add(vtx);
                    tex[0] = texLen*4.0;
                    tex[1] = 0;
                    bg_tex->add(tex);

                    vtx[0] = x;
                    vtx[1] = y;
                    vtx[2] = z2;
                    bg_vtx->add(vtx);
                    tex[0] = texLen*4.0;
                    tex[1] = 1.0;
                    bg_tex->add(tex);
                }//for i

                bg = new ssgVtxTable(GL_TRIANGLE_STRIP, bg_vtx, bg_nrm, bg_tex, bg_clr);
                bg_st = (ssgSimpleState*)grSsgLoadTexState(graphic->background);
                bg_st->disable(GL_LIGHTING);
                bg->setState(bg_st);
                bg->setCullFace(0);
                TheBackground->addKid(bg);
                break;	//case 1

            case TR_BACKGROUND_TYPE_2: //-----------------------------------------------------------
                bg_vtx = new ssgVertexArray(NbBackgroundFaces + 1);
                bg_tex = new ssgTexCoordArray(NbBackgroundFaces + 1);
                bg_clr = new ssgColourArray(1);
                bg_nrm = new ssgNormalArray(1);
                bg_clr->add(clr);
                bg_nrm->add(nrm);

                for (i = 0; i < NbBackgroundFaces / 4 + 1; i++) {
                    alpha = (float)i * 2 * PI / (float)NbBackgroundFaces;
                    texLen = (float)i / (float)NbBackgroundFaces;

                    x = BackgroundDistance * cos(alpha);
                    y = BackgroundDistance * sin(alpha);

                    vtx[0] = x;
                    vtx[1] = y;
                    vtx[2] = z1;
                    bg_vtx->add(vtx);
                    tex[0] = texLen*4.0;
                    tex[1] = 0;
                    bg_tex->add(tex);

                    vtx[0] = x;
                    vtx[1] = y;
                    vtx[2] = z2;
                    bg_vtx->add(vtx);
                    tex[0] = texLen*4.0;
                    tex[1] = 0.5;
                    bg_tex->add(tex);
                }//for i

                bg = new ssgVtxTable(GL_TRIANGLE_STRIP, bg_vtx, bg_nrm, bg_tex, bg_clr);
                bg_st = (ssgSimpleState*)grSsgLoadTexState(graphic->background);
                bg_st->disable(GL_LIGHTING);
                bg->setState(bg_st);
                bg->setCullFace(0);
                TheBackground->addKid(bg);


                bg_vtx = new ssgVertexArray(NbBackgroundFaces + 1);
                bg_tex = new ssgTexCoordArray(NbBackgroundFaces + 1);
                bg_clr = new ssgColourArray(1);
                bg_nrm = new ssgNormalArray(1);
                bg_clr->add(clr);
                bg_nrm->add(nrm);

                for (i = NbBackgroundFaces/4; i < NbBackgroundFaces / 2 + 1; i++) {
                    alpha = (float)i * 2 * PI / (float)NbBackgroundFaces;
                    texLen = (float)i / (float)NbBackgroundFaces;

                    x = BackgroundDistance * cos(alpha);
                    y = BackgroundDistance * sin(alpha);

                    vtx[0] = x;
                    vtx[1] = y;
                    vtx[2] = z1;
                    bg_vtx->add(vtx);
                    tex[0] = texLen*4.0;
                    tex[1] = 0.5;
                    bg_tex->add(tex);

                    vtx[0] = x;
                    vtx[1] = y;
                    vtx[2] = z2;
                    bg_vtx->add(vtx);
                    tex[0] = texLen*4.0;
                    tex[1] = 1.0;
                    bg_tex->add(tex);
                }//for i

                bg = new ssgVtxTable(GL_TRIANGLE_STRIP, bg_vtx, bg_nrm, bg_tex, bg_clr);
                bg_st = (ssgSimpleState*)grSsgLoadTexState(graphic->background);
                bg_st->disable(GL_LIGHTING);
                bg->setState(bg_st);
                bg->setCullFace(0);
                TheBackground->addKid(bg);


                bg_vtx = new ssgVertexArray(NbBackgroundFaces + 1);
                bg_tex = new ssgTexCoordArray(NbBackgroundFaces + 1);
                bg_clr = new ssgColourArray(1);
                bg_nrm = new ssgNormalArray(1);

                bg_clr->add(clr);
                bg_nrm->add(nrm);

                for (i = NbBackgroundFaces / 2; i < 3 * NbBackgroundFaces / 4 + 1; i++) {
                    alpha = (float)i * 2 * PI / (float)NbBackgroundFaces;
                    texLen = (float)i / (float)NbBackgroundFaces;

                    x = BackgroundDistance * cos(alpha);
                    y = BackgroundDistance * sin(alpha);

                    vtx[0] = x;
                    vtx[1] = y;
                    vtx[2] = z1;
                    bg_vtx->add(vtx);
                    tex[0] = texLen*4.0;
                    tex[1] = 0.0;
                    bg_tex->add(tex);

                    vtx[0] = x;
                    vtx[1] = y;
                    vtx[2] = z2;
                    bg_vtx->add(vtx);
                    tex[0] = texLen*4.0;
                    tex[1] = 0.5;
                    bg_tex->add(tex);
                }//for i

                bg = new ssgVtxTable(GL_TRIANGLE_STRIP, bg_vtx, bg_nrm, bg_tex, bg_clr);
                bg_st = (ssgSimpleState*)grSsgLoadTexState(graphic->background);
                bg_st->disable(GL_LIGHTING);
                bg->setState(bg_st);
                bg->setCullFace(0);
                TheBackground->addKid(bg);


                bg_vtx = new ssgVertexArray(NbBackgroundFaces + 1);
                bg_tex = new ssgTexCoordArray(NbBackgroundFaces + 1);
                bg_clr = new ssgColourArray(1);
                bg_nrm = new ssgNormalArray(1);

                bg_clr->add(clr);
                bg_nrm->add(nrm);

                for(i = 3 * NbBackgroundFaces / 4; i < NbBackgroundFaces + 1; i++) {
                    alpha = (float)i * 2 * PI / (float)NbBackgroundFaces;
                    texLen = (float)i / (float)NbBackgroundFaces;

                    x = BackgroundDistance * cos(alpha);
                    y = BackgroundDistance * sin(alpha);

                    vtx[0] = x;
                    vtx[1] = y;
                    vtx[2] = z1;
                    bg_vtx->add(vtx);
                    tex[0] = texLen*4.0;
                    tex[1] = 0.5;
                    bg_tex->add(tex);

                    vtx[0] = x;
                    vtx[1] = y;
                    vtx[2] = z2;
                    bg_vtx->add(vtx);
                    tex[0] = texLen*4.0;
                    tex[1] = 1.0;
                    bg_tex->add(tex);
                }//for i

                bg = new ssgVtxTable(GL_TRIANGLE_STRIP, bg_vtx, bg_nrm, bg_tex, bg_clr);
                bg_st = (ssgSimpleState*)grSsgLoadTexState(graphic->background);
                bg_st->disable(GL_LIGHTING);
                bg->setState(bg_st);
                bg->setCullFace(0);
                TheBackground->addKid(bg);

                break;	//case 2

            case TR_BACKGROUND_TYPE_4: //-----------------------------------------------------------
                z1 = -1.0;
                z2 = 1.0;

                bg_vtx = new ssgVertexArray(NbBackgroundFaces + 1);
                bg_tex = new ssgTexCoordArray(NbBackgroundFaces + 1);
                bg_clr = new ssgColourArray(1);
                bg_nrm = new ssgNormalArray(1);
                bg_clr->add(clr);
                bg_nrm->add(nrm);

                for (i = 0; i < NbBackgroundFaces + 1; i++) {
                    alpha = (double)i * 2 * PI / (double)NbBackgroundFaces;
                    texLen = 1.0 - (float)i / (float)NbBackgroundFaces;

                    x = BackgroundDistance * cos(alpha);
                    y = BackgroundDistance * sin(alpha);

                    vtx[0] = x;
                    vtx[1] = y;
                    vtx[2] = z1;
                    bg_vtx->add(vtx);
                    tex[0] = texLen;
                    tex[1] = 0;
                    bg_tex->add(tex);

                    vtx[0] = x;
                    vtx[1] = y;
                    vtx[2] = z2;
                    bg_vtx->add(vtx);
                    tex[0] = texLen;
                    tex[1] = 1.0;
                    bg_tex->add(tex);
                }//for i

                bg = new ssgVtxTable(GL_TRIANGLE_STRIP, bg_vtx, bg_nrm, bg_tex, bg_clr);
                bg_st = (ssgSimpleState*)grSsgLoadTexState(graphic->background);
                bg_st->disable(GL_LIGHTING);
                bg->setState(bg_st);
                bg->setCullFace(0);
                TheBackground->addKid(bg);
                break;//case 4

            default:
                GfLogError("Unsupported background type %d\n", graphic->bgtype);
                break;
        }//switch grBackgroundType

        if (!SunAnchor && grTrack->local.rain == 0) {
            // Lens Flares when no sky dome (realistic sky dome will use another system when ready).
            SunAnchor = new ssgBranch;
            TheScene->addKid(SunAnchor);
        }

    } //if (!grSkyDomeDistance || grTrack->skyversion < 1)
    else
    {
        // Check / fix the cloud cover index, in any case.
        if (grTrack->local.clouds < 0)
            grTrack->local.clouds = 0;
        else if(grTrack->local.clouds >= NCloudsTextureIndices)
            grTrack->local.clouds = NCloudsTextureIndices - 1;
    }

    // Environment Mapping Settings
    // 1) Horizontal reflexions of the track objects (env.png & co)
    bool bUseEnvPng = false;   // Avoid crash with missing env.rgb files (i.e. Wheel-1)
    bool bDoNotUseEnv = false; // Avoid crash with missing env.png
    grEnvSelector = new ssgStateSelector(graphic->envnb);
    for (i = 0; i < graphic->envnb; i++) {
        GfLogTrace("Loading #%d track-specific env. mapping image :\n", i+1);
        envst = (ssgSimpleState*)grSsgLoadTexState(graphic->env[i]);
    // Avoid crash with missing env.rgb files (i.e. Wheel-1)
        if (!envst) {
            GfLogWarning("Failed : trying fallback env.png\n");
            envst = (ssgSimpleState*)grSsgLoadTexState("env.png");
            if (!envst) {
                GfLogError("No usable Environment Mapping Image for #%d : stop displaying graphics!\n", i);
                bDoNotUseEnv = true;
                break;
            }
            else
                bUseEnvPng = true;
        }
        envst->enable(GL_BLEND);
        grEnvSelector->setStep(i, envst);
        envst->deRef();
    }//for i

        grEnvSelector->selectStep(0); //mandatory !!!

    // Avoid crash with missing env.rgb files (i.e. Wheel-1)
    GfLogTrace("Loading global env. mapping image :\n");
    if (bUseEnvPng)
        grEnvState = grSsgEnvTexState("env.png", cgrMultiTexState::modulate);
    else if (bDoNotUseEnv)
        GfLogError("No env.png found!\n");
    else
        grEnvState = grSsgEnvTexState(graphic->env[0], cgrMultiTexState::modulate);

    // 2) Sky shadows (vertical) (envshadow-xxx.png), according to the weather conditions
    GfLogTrace("Loading sky shadow mapping image :\n");
    grEnvShadowState = 0;
    int nEnvShadowIndex = -1; // Default = not depending on weather conds.
    if (!grSkyDomeDistance )
    {
        // Static / texture-based sky case.
        if (grTrack->local.rain > 0) // Rain => full cloud cover.
            nEnvShadowIndex = NEnvShadowFullCoverRainIndex;
    }
    else
    {
        // Realistic sky dome case.
        // TODO: Find a solution for the "dynamic time" case (not correctly supported here).
        if (grTrack->local.timeofday < 6*60*60 || grTrack->local.timeofday > 18*60*60)
            nEnvShadowIndex = NEnvShadowNightIndex;
        else if (grTrack->local.rain > 0) // Rain => full cloud cover.
            nEnvShadowIndex = NEnvShadowFullCoverRainIndex;
        else
            nEnvShadowIndex = grTrack->local.clouds;
    }
    if (nEnvShadowIndex >= 0)
    {
        char pszEnvFile[64];
        snprintf(pszEnvFile, sizeof(pszEnvFile), "envshadow-%s.png", AEnvShadowKeys[nEnvShadowIndex]);
        grEnvShadowState = grSsgEnvTexState(pszEnvFile, cgrMultiTexState::addColorModulateAlpha);
        if (!grEnvShadowState)
            GfLogWarning("%s not found ; falling back to weather-independant sky shadows"
                         " from envshadow.png\n", pszEnvFile);
    }
    if (!grEnvShadowState)
        grEnvShadowState = grSsgEnvTexState("envshadow.png", cgrMultiTexState::addColorModulateAlpha);
    if (!grEnvShadowState) {
        GfLogError("envshadow.png not found ; exiting !\n");
        GfLogError("(mandatory for top env mapping (should be in <track>.xml or data/textures ;\n");
        GfLogError(" copy the envshadow.png from 'chemisay' to the track you selected ;\n");
        GfLogError(" sorry for exiting, but it would have actually crashed)\n");
        GfScrShutdown();
        exit(-1);
    }//if grEnvShadowState

    // 3) Vertical shadows of track objects on the cars (shadow2.png)
    GfLogTrace("Loading track shadows mapping image :\n");
    grEnvShadowStateOnCars = grSsgEnvTexState("shadow2.png", cgrMultiTexState::modulate);
    if(!grEnvShadowStateOnCars)
        grEnvShadowStateOnCars = grSsgEnvTexState("shadow2.rgb", cgrMultiTexState::modulate);

    if(!grEnvShadowStateOnCars)
        GfLogWarning("shadow2.png/rgb not found ; no shadow mapping on cars for this track\n");
}//grLoadBackground

void grLoadBackgroundSky(void)
{
    char buf2[256];
    const char		*bgsky;
    ssgEntity		*desc2;

    bgsky = "background-sky.ac";
    snprintf(buf2, sizeof(buf2), "tracks/%s/%s;data/textures;.", grTrack->category, grTrack->internalname);
    ssgTexturePath(buf2);
    snprintf(buf2, sizeof(buf2), "tracks/%s/%s;data/objects", grTrack->category, grTrack->internalname);
    ssgModelPath(buf2);

    desc2 = grssgLoadAC3D(bgsky, NULL);
    BackSkyAnchor->addKid(desc2);

    // move backgroundsky in scene center
    sgCoord BackSkypos;
    sgSetCoord(&BackSkypos, grWrldX/2, grWrldY/2, 0, 0, 0, 0);
    BackSkyLoc->setTransform(&BackSkypos);
}

void grLoadBackgroundLand(void)
{
    char buf2[256];
    const char		*bgsky;
    ssgEntity		*desc2;

    bgsky = "land.ac";
    snprintf(buf2, sizeof(buf2), "tracks/%s/%s;data/textures;.", grTrack->category, grTrack->internalname);
    ssgTexturePath(buf2);
    snprintf(buf2, sizeof(buf2), "tracks/%s/%s;data/objects;.", grTrack->category, grTrack->internalname);
    ssgModelPath(buf2);

    desc2 = grssgLoadAC3D(bgsky, NULL);
    BackSkyAnchor->addKid(desc2);
}

void
grPreDrawSky(tSituation* s, float fogStart, float fogEnd)
{
    static const double m_log01 = -log( 0.01 );
    static const double sqrt_m_log01 = sqrt( m_log01 );
	GLbitfield clear_mask;

    if (grSkyDomeDistance )
    {
    const GLfloat fog_exp2_density = sqrt_m_log01 / TheSky->getVisibility();
        glEnable(GL_FOG);
        //glFogf(GL_FOG_START, fogStart);
        //glFogf(GL_FOG_END, fogEnd);
        //glFogi(GL_FOG_MODE, GL_LINEAR);
        glFogi(GL_FOG_MODE, GL_EXP2);
        glFogfv(GL_FOG_COLOR, FogColor);
        glFogf(GL_FOG_DENSITY, fog_exp2_density);
        glHint(GL_FOG_HINT, GL_DONT_CARE);

        ssgGetLight(0)->setColour(GL_DIFFUSE, White);
		clear_mask |= GL_COLOR_BUFFER_BIT;
		glClear( clear_mask );

        TheSky->preDraw();

        glLightModelfv( GL_LIGHT_MODEL_AMBIENT, Black);
        ssgGetLight(0)->setColour(GL_AMBIENT, SceneAmbiant);
        ssgGetLight(0)->setColour(GL_DIFFUSE, SceneDiffuse);
        ssgGetLight(0)->setColour(GL_SPECULAR, SceneSpecular);

    }
}//grPreDrawSky

void
grDrawStaticBackground(cGrCamera *cam, cGrBackgroundCam *bgCam)
{
    if (!TheBackground)
        return;

    TRACE_GL("grDrawStaticBackground: ssgCullAndDraw start");

    bgCam->update(cam);
    bgCam->action();
    ssgCullAndDraw(TheBackground);

    TRACE_GL("grDrawStaticBackground: ssgCullAndDraw");
}//grDrawStaticBackground

void
grPostDrawSky(void)
{
    if (grSkyDomeDistance )
        TheSky->postDraw(grSkyDomeDistance);
}//grPostDrawSky

// Update the sky when time changes
void
grUpdateSky(double currentTime, double accelTime)
{
    // Detect first call (in order to initialize "last times").
    static bool bInitialized = false;
    static double lastTimeHighSpeed = 0;
    static int lastTimeLowSpeed = 0;

    // Nothing to do if static sky dome, or race not started.
    //if (!grDynamicSkyDome)	//TODO(kilo): find some meaning for this variable
    if (!grSkyDomeDistance )
        return;

    if (currentTime < 0) {
        bInitialized = false;
        return;
    }

    if (!bInitialized)
    {
        if (grSkyDomeDistance ) 
		{
            // Ensure the sun and moon positions are reset
            const int timeOfDay = (int)grTrack->local.timeofday;
            GLfloat sunAscension = grTrack->local.sunascension;
            grSunDeclination = (float)(15 * (double)timeOfDay / 3600 - 90.0);

            const float moonAscension = grTrack->local.sunascension;
            grMoonDeclination = grUpdateMoonPos(timeOfDay);

            TheSky->setSD( DEG2RAD(grSunDeclination));
            TheSky->setSRA( sunAscension );

            TheSky->setMD( DEG2RAD(grMoonDeclination) );
            TheSky->setMRA( DEG2RAD(moonAscension) );
        }

        lastTimeHighSpeed = currentTime;
        lastTimeLowSpeed = 60 * (int)floor((accelTime + 60.0) / 60.0);

        bInitialized = true;
        return;
    }

        // At each call, update possibly high speed objects of the sky dome : the clouds.
        sgVec3 viewPos;
        sgSetVec3(viewPos, grWrldX/2, grWrldY/2, 0);
        TheSky->repositionFlat(viewPos, 0, currentTime - lastTimeHighSpeed);

    // Now, we are done for high speed objects.
    lastTimeHighSpeed = currentTime;

    // Check if time to update low speed objects : sun and moon (once every minute).
    int nextTimeLowSpeed = 60 * (int)floor((accelTime + 60.0) / 60.0);
    /*if (nextTimeLowSpeed == lastTimeLowSpeed)
        return;*/
    const float deltaTimeLowSpeed = (float)(nextTimeLowSpeed - lastTimeLowSpeed);
    //lastTimeLowSpeed = nextTimeLowSpeed;

    // Update sun and moon, and thus global lighting / coloring parameters of the scene.
    //GfLogDebug("%f : Updating slow objects of the dynamic sky dome (sun and moon)\n", currentTime);
    if (nextTimeLowSpeed != lastTimeLowSpeed)
    {
        // 1) Update sun position
        const float deltaDecl = deltaTimeLowSpeed * 360.0f / (24.0f * 60.0f * 60.0f);
        grSunDeclination += deltaDecl;
        if (grSunDeclination >= 360.0f)
            grSunDeclination -= 360.0f;

        TheSky->setSD( DEG2RAD(grSunDeclination) );

        // 2) Update moon position
        grMoonDeclination += deltaDecl;
        if (grMoonDeclination >= 360.0f)
            grMoonDeclination -= 360.0f;

        TheSky->setMD( DEG2RAD(grMoonDeclination) );
        lastTimeLowSpeed = nextTimeLowSpeed;
    }

    // 3) Update scene color and light

    grUpdateLight();

}//grUpdateSky

void
grShutdownBackground(void)
{
    if (TheSky)
    {
        delete TheSky;
        TheSky = 0;
    }

//TODO(kilo): why not delete?
    if (TheSun)
        TheSun = 0;

    if (SunAnchor)
        SunAnchor = 0;

    if (BackSkyAnchor)
        BackSkyAnchor = 0;

    if (grEnvState)
    {
        ssgDeRefDelete(grEnvState);
        grEnvState = 0;
    }

    if (grEnvShadowState)
    {
        ssgDeRefDelete(grEnvShadowState);
        grEnvShadowState = 0;
    }

    if (grEnvShadowStateOnCars)
    {
        ssgDeRefDelete(grEnvShadowStateOnCars);
        grEnvShadowStateOnCars = 0;
    }

    if(grEnvSelector)
    {
        delete grEnvSelector;
        grEnvSelector = 0;
    }

}//grShutdownBackground

void grUpdateLight( void )
{
    const float sol_angle = (float)TheSky->getSA();
    const float moon_angle = (float)TheSky->getMA();
    float sky_brightness = (float)(1.0 + cos(sol_angle)) / 2.0f;

    if (grTrack->local.rain > 0) // TODO: Different values for each rain strength value ?
    {
        BaseFogColor[0] = 0.42f;
        BaseFogColor[1] = 0.44f;
        BaseFogColor[2] = 0.50f;

        sky_brightness = (float)pow(sky_brightness, 0.5f);
    }
    else
    {
        BaseFogColor[0] = 0.84f;
        BaseFogColor[1] = 0.84f;
        BaseFogColor[2] = 1.00f;
    }

    SkyColor[0] = BaseSkyColor[0] * sky_brightness;
    SkyColor[1] = BaseSkyColor[1] * sky_brightness;
    SkyColor[2] = BaseSkyColor[2] * sky_brightness;
    SkyColor[3] = BaseSkyColor[3];
    grUpdateFogColor(sol_angle);

    grGammaCorrectRGB( SkyColor );

    // 3a)cloud and fog color
    CloudsColor[0] = FogColor[0] = BaseFogColor[0] * sky_brightness;
    CloudsColor[1] = FogColor[1] = BaseFogColor[1] * sky_brightness;
    CloudsColor[2] = FogColor[2] = BaseFogColor[2] * sky_brightness;
    CloudsColor[3] = FogColor[3] = BaseFogColor[3];

    //grUpdateFogColor(sol_angle);
    grGammaCorrectRGB( CloudsColor );


    float *sun_color = TheSky->get_sun_color();

    if (sol_angle > 1.0)
    {
        if (grVisibility > 1000 && cloudsTextureIndex < 8)
        {
            CloudsColor[0] = CloudsColor[0] * sun_color[0];
            CloudsColor[1] = CloudsColor[1] * sun_color[1];
            CloudsColor[2] = CloudsColor[2] * sun_color[2];
        }
        else
        {
            CloudsColor[0] = CloudsColor[0] * sun_color[0];
            CloudsColor[1] = CloudsColor[1] * sun_color[0];
            CloudsColor[2] = CloudsColor[2] * sun_color[0];
        }
    }

    grGammaCorrectRGB( CloudsColor );

    // 3b) repaint the sky (simply update geometrical, color, ... state, no actual redraw)
    TheSky->repaint(SkyColor, FogColor, CloudsColor, sol_angle, moon_angle,
                    NPlanets, APlanetsData, NStars, AStarsData);

    // 3c) update the main light position (it's at the sun position !)
    sgCoord solpos;
    TheSky->getSunPos(&solpos);
    ssgGetLight(0)-> setPosition(solpos.xyz);

    // 3c) update scene colors.
    if (grVisibility > 1000 && cloudsTextureIndex < 8)
    {
        SceneAmbiant[0] = (sun_color[0]*0.25f + CloudsColor[0]*0.75f) * sky_brightness;
        SceneAmbiant[1] = (sun_color[1]*0.25f + CloudsColor[1]*0.75f) * sky_brightness;
        SceneAmbiant[2] = (sun_color[2]*0.25f + CloudsColor[2]*0.75f) * sky_brightness;
        SceneAmbiant[3] = 1.0;

        SceneDiffuse[0] = (sun_color[0]*0.25f + FogColor[0]*0.75f) * sky_brightness;
        SceneDiffuse[1] = (sun_color[1]*0.25f + FogColor[1]*0.75f) * sky_brightness;
        SceneDiffuse[2] = (sun_color[2]*0.25f + FogColor[2]*0.75f) * sky_brightness;
        SceneDiffuse[3] = 1.0;

        SceneSpecular[0] = sun_color[0] * sky_brightness;
        SceneSpecular[1] = sun_color[1] * sky_brightness;
        SceneSpecular[2] = sun_color[2] * sky_brightness;
        SceneSpecular[3] = 1.0;
    }
    else
    {
        SceneAmbiant[0] = (sun_color[0]*0.25f + CloudsColor[0]*0.75f) * sky_brightness;
        SceneAmbiant[1] = (sun_color[0]*0.25f + CloudsColor[1]*0.75f) * sky_brightness;
        SceneAmbiant[2] = (sun_color[0]*0.25f + CloudsColor[2]*0.75f) * sky_brightness;
        SceneAmbiant[3] = 1.0;

        SceneDiffuse[0] = (sun_color[0]*0.25f + FogColor[0]*0.75f) * sky_brightness;
        SceneDiffuse[1] = (sun_color[0]*0.25f + FogColor[1]*0.75f) * sky_brightness;
        SceneDiffuse[2] = (sun_color[0]*0.25f + FogColor[2]*0.75f) * sky_brightness;
        SceneDiffuse[3] = 1.0;

        SceneSpecular[0] = sun_color[0] * sky_brightness;
        SceneSpecular[1] = sun_color[0] * sky_brightness;
        SceneSpecular[2] = sun_color[0] * sky_brightness;
        SceneSpecular[3] = 1.0;
    }
}//grUpdateLight

void grUpdateFogColor(double sol_angle)
{
    double rotation;

    // first determine the difference between our view angle and local
    // direction to the sun
    rotation = -(TheSky->getSR() + SGD_PI);
    while ( rotation < 0 )
    {
        rotation += SGD_2PI;
    }
    while ( rotation > SGD_2PI )
    {
        rotation -= SGD_2PI;
    }

    // revert to unmodified values before usign them.
    //
    float *sun_color = TheSky->get_sun_color();

    grGammaRestoreRGB( BaseFogColor );

    // Calculate the fog color in the direction of the sun for
    // sunrise/sunset effects.
    //
    float s_red =   (BaseFogColor[0] + 2 * sun_color[0] * sun_color[0]) / 3;
    float s_green = (BaseFogColor[1] + 2 * sun_color[1] * sun_color[1]) / 3;
    float s_blue =  (BaseFogColor[2] + 2 * sun_color[2] * sun_color[2]) / 3;

    // interpolate beween the sunrise/sunset color and the color
    // at the opposite direction of this effect. Take in account
    // the current visibility.
    //
    float av = TheSky->getVisibility();
    if (av > 45000)
       av = 45000;

    float avf = 0.87 - (45000 - av) / 83333.33;
    float sif = 0.5 - cos( sol_angle * 2)/2;

    if (sif < 1e-4)
       sif = 1e-4;

    float rf1 = fabs((rotation - SGD_PI) / SGD_PI);             // 0.0 .. 1.0
    float rf2 = avf * pow(rf1 * rf1, 1 /sif);
    float rf3 = 0.94 - rf2;

    FogColor[0] = rf3 * BaseFogColor[0] + rf2 * s_red;
    FogColor[1] = rf3 * BaseFogColor[1] + rf2 * s_green;
    FogColor[2] = rf3 * BaseFogColor[2] + rf2 * s_blue;
    grGammaCorrectRGB( FogColor );

    // make sure the colors have their original value before they are being
    // used by the rest of the program.
    //
    grGammaCorrectRGB( BaseFogColor );
}
