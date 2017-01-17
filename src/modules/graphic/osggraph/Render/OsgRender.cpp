/***************************************************************************

    file                 : OsgRender.cpp
    created              : Mon Aug 21 20:13:56 CEST 2012
    copyright            : (C) 2012 Xavier Bertaux
    email                : bertauxx@yahoo.fr
    version              : $Id: OsgRender.cpp 2436 2010-05-08 14:22:43Z torcs-ng $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <osgUtil/Optimizer>
#include <osg/MatrixTransform>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgDB/Registry>
#include <osg/Fog>
#include <osg/Light>
#include <osg/LightModel>
#include <osg/LightSource>
#include <osg/Camera>
#include <osgViewer/Viewer>
#include <osgParticle/PrecipitationEffect>
#include <osgShadow/ShadowedScene>
#include <osgShadow/ShadowVolume>
#include <osgShadow/ShadowTexture>
#include <osgShadow/ShadowMap>
#include <osgShadow/SoftShadowMap>
#include <osgShadow/ParallelSplitShadowMap>
#include <osgShadow/LightSpacePerspectiveShadowMap>
#include <osgShadow/StandardShadowMap>
#include <osgShadow/ViewDependentShadowMap>

#include "OsgMain.h"
#include "OsgRender.h"
#include "OsgSky.h"
#include "OsgScenery.h"
#include "OsgMath.h"
#include "OsgColor.h"

#include <glfeatures.h>         //gluXXX
#include <robottools.h>         //RtXXX()

#define MAX_BODIES	2
#define MAX_CLOUDS	3
#define NMaxStars	3000
#define NPLANETS		0       //No planets displayed
#define NB_BG_FACES	36          //Background faces
#define BG_DIST			1.0f
#define SKYDYNAMIC_THR	12000	//Skydynamic setting threshold. No dynamic sky below that.
#define CLEAR_CLOUD 1
#define MORE_CLOUD 6
#define SCARCE_CLOUD 5
#define COVERAGE_CLOUD 8

static const char* ShadowValues[] = { GR_ATT_SHADOW_NONE, GR_ATT_SHADOW_SM, GR_ATT_SHADOW_SSM, GR_ATT_SHADOW_PSSM, GR_ATT_SHADOW_LSPM, GR_ATT_SHADOW_VOLUME, GR_ATT_SHADOW_VDSM};
static const int NbShadowValues = sizeof(ShadowValues) / sizeof(ShadowValues[0]);
static const char* TexSizeValues[] = { GR_ATT_SHADOW_512, GR_ATT_SHADOW_1024, GR_ATT_SHADOW_2048, GR_ATT_SHADOW_4096, GR_ATT_SHADOW_8192 };
static const int NbTexSizeValues = sizeof(TexSizeValues) / sizeof(TexSizeValues[0]);
static const char* QualityValues[] = { GR_ATT_AGR_LITTLE, GR_ATT_AGR_MEDIUM, GR_ATT_AGR_FULL };
static const int NbQualityValues = sizeof(QualityValues) / sizeof(QualityValues[0]);
static const char* ShadersValues[] = { GR_ATT_AGR_NULL, GR_ATT_AGR_LITTLE, GR_ATT_AGR_FULL };
static const int NbShadersValues = sizeof(ShadersValues) / sizeof(ShadersValues[0]);
static const int CloudsTextureIndices[TR_CLOUDS_FULL+1] = {1, 3, 5, 7, 8};
static const int NCloudsTextureIndices = sizeof(CloudsTextureIndices) / sizeof(int);

SDRender::SDRender(void) :
    m_RealRoot(NULL),
    m_scene(NULL),
    m_CarRoot(NULL),
    skyGroup(NULL),
    stateSet(NULL),
    skySS(NULL),
    lightSource(NULL),
    sunLight(NULL),
    shadowRoot(NULL),
    AStarsData(NULL),
    APlanetsData(NULL),
    thesky(NULL),
    scenery(NULL),
    SDTrack(NULL)
{
    BaseSkyColor = osg::Vec3f( 0.31f, 0.43f, 0.69f );
    BaseFogColor = osg::Vec3f( 0.84f, 0.84f, 1.0f );

    Scene_ambiant = osg::Vec4f( 0.8f, 0.8f, 0.8f, 1.0f);
    Scene_Specular = osg::Vec4f( 0.05f, 0.05f, 0.05f, 1.0f);
    Scene_Diffuse = osg::Vec4f( 0.6f, 0.6f, 0.6f, 1.0f);
    Scene_Emit = osg::Vec4f( 0.2f, 0.2f, 0.2f, 1.0f);

    SDSkyDomeDistance = 0;
    SDSkyDomeDistThresh = 12000;
    SDNbCloudLayers = 0;
    SDDynamicWeather = 0;
    SDDynamicSkyDome = false;

    cloudsTextureIndex = 0;
    carsShader = 0;

    rcvShadowMask = 0x1;
    castShadowMask = 0x2;

    SDSunDeclination = 0.0f;
    SDMoonDeclination = 0.0f;
    SDMax_Visibility = 12000.0f;
    SDVisibility = 0.0f;
    NStars = 0;
    NPlanets = 0;
    sol_angle = 0.0;
    moon_angle = 0.0;
    sky_brightness = 0.0;
}

SDRender::~SDRender(void)
{
    m_scene->removeChildren(0, m_scene->getNumChildren());
    m_CarRoot->removeChildren(0, m_CarRoot->getNumChildren());
    skyGroup->removeChildren(0, skyGroup->getNumChildren());
    m_RealRoot->removeChildren(0, m_RealRoot->getNumChildren());
    stateSet->getTextureAttributeList().clear();
    stateSet->getTextureModeList().clear();

    m_scene = NULL;
    m_CarRoot = NULL;
    skyGroup = NULL;
    m_RealRoot = NULL;

    delete thesky;

    thesky = NULL;
    SDTrack = NULL;
}

/**
 * SDRender
 * Initialises a scene (ie a new view).
 *
 * @return 0 if OK, -1 if something failed
 */
void SDRender::Init(tTrack *track)
{
    SDTrack = track;

    std::string datapath = GetDataDir();
    //datapath +="/";
    thesky = new SDSky;
    GfOut("SDSky class\n");

    // Sky dome / background.
    SDSkyDomeDistance = 20000;
    if (SDSkyDomeDistance > 0 && SDSkyDomeDistance < SDSkyDomeDistThresh)
        SDSkyDomeDistance = SDSkyDomeDistThresh; // If user enabled it (>0), must be at least the threshold.

    SDDynamicSkyDome = strcmp(GfParmGetStr(grHandle, GR_SCT_GRAPHIC, GR_ATT_DYNAMICSKYDOME, GR_ATT_DYNAMICSKYDOME_DISABLED), GR_ATT_DYNAMICSKYDOME_ENABLED) == 0;

    GfLogInfo("Graphic options : Sky dome : distance = %u m, dynamic = %s\n",
              SDSkyDomeDistance, SDDynamicSkyDome ? "true" : "false");

    // Dynamic weather.
    //grDynamicWeather = GfParmGetNum(grHandle, GR_SCT_GRAPHIC, GR_ATT_grDynamicWeather, (char*)NULL, grDynamicWeather);

    // Cloud layers.
    SDNbCloudLayers =
            (unsigned)(GfParmGetNum(grHandle, GR_SCT_GRAPHIC, GR_ATT_CLOUDLAYER, 0, 0) + 0.5);

    GfLogInfo("Graphic options : Number of cloud layers : %u\n", SDNbCloudLayers);

    SDMax_Visibility =
            (unsigned)(GfParmGetNum(grHandle, GR_SCT_GRAPHIC, GR_ATT_VISIBILITY, 0, 0));

    ShadowIndex = 0; // Default value index, in case file value not found in list.
    const char* pszShadow =
            GfParmGetStr(grHandle, GR_SCT_GRAPHIC, GR_ATT_SHADOW_TYPE, GR_ATT_SHADOW_NONE);

    for (int i = 0; i < NbShadowValues; i++)
    {
        if (!strcmp(pszShadow, ShadowValues[i]))
        {
            ShadowIndex = i;
            break;
        }
    }

    TexSizeIndex = 0; // Default value index, in case file value not found in list.
    const char* pszTexSize =
            GfParmGetStr(grHandle, GR_SCT_GRAPHIC, GR_ATT_SHADOW_SIZE, GR_ATT_SHADOW_1024);

    for (int i = 0; i < NbTexSizeValues; i++)
    {
        if (!strcmp(pszTexSize, TexSizeValues[i]))
        {
            TexSizeIndex = i;
            break;
        }
    }

    switch (TexSizeIndex)
    {
    case 0:
        ShadowTexSize = 512;
        break;

    case 1:
        ShadowTexSize = 1024;
        break;

    case 2:
        ShadowTexSize = 2048;
        break;

    case 3:
        ShadowTexSize = 4096;
        break;

    case 4:
        ShadowTexSize = 8192;
        break;

    default:
        ShadowTexSize = 1024;
        break;
    }

    QualityIndex = 0; // Default value index, in case file value not found in list.
    const char* pszQuality =
            GfParmGetStr(grHandle, GR_SCT_GRAPHIC, GR_ATT_AGR_QUALITY, GR_ATT_AGR_LITTLE);

    for (int i = 0; i < NbQualityValues; i++)
    {
        if (!strcmp(pszQuality, QualityValues[i]))
        {
            QualityIndex = i;
            break;
        }
    }

    carsShader = 0; // Default value index, in case file value not found in list.
    const char* pszShaders =
            GfParmGetStr(grHandle, GR_SCT_GRAPHIC, GR_ATT_SHADERS, GR_ATT_AGR_NULL);

    for (int i = 0; i < NbShadersValues; i++)
    {
        if (!strcmp(pszShaders, ShadersValues[i]))
        {
            carsShader = i;
            break;
        }
    }

    GfLogInfo("Graphic options : Shadow Type : %u\n", ShadowIndex);
    GfLogInfo("Graphic options : Shadow Texture Size : %u\n", ShadowTexSize);
    GfLogInfo("Graphic options : Shadow Quality : %u\n", QualityIndex);

    NStars = NMaxStars;
    if (AStarsData)
        delete [] AStarsData;

    AStarsData = new osg::Vec3d[NStars];

    for(int i= 0; i < NStars; i++)
    {
        AStarsData[i][0] = SDRandom() * PI;
        AStarsData[i][1] = SDRandom() * PI;
        AStarsData[i][2] = SDRandom() * 7.0;
    }

    GfLogInfo("  Stars (random) : %d\n", NStars);

    NPlanets = 0;
    APlanetsData = NULL;

    GfLogInfo("  Planets : %d\n", NPlanets);

    const int timeOfDay = (int)SDTrack->local.timeofday;
    const double domeSizeRatio = SDSkyDomeDistance / 80000.0;

    GfLogInfo("  domeSizeRation : %d\n", domeSizeRatio);

    thesky->build(datapath, SDSkyDomeDistance, SDSkyDomeDistance, 800,
                  40000, 800, 30000, NPlanets,
                  APlanetsData, NStars, AStarsData );
    GfOut("Build SKY\n");
    GLfloat sunAscension = SDTrack->local.sunascension;
    SDSunDeclination = (float)(15 * (double)timeOfDay / 3600 - 90.0);

    thesky->setSD( DEG2RAD(SDSunDeclination));
    thesky->setSRA( sunAscension );

    GfLogInfo("  Sun : time of day = %02d:%02d:%02d (declination = %.1f deg), "
              "ascension = %.1f deg\n", timeOfDay / 3600, (timeOfDay % 3600) / 60, timeOfDay % 60,
              SDSunDeclination, RAD2DEG(sunAscension));

    if ( SDSunDeclination > 180 )
        SDMoonDeclination = 3.0 + (rand() % 40);
    else
        SDMoonDeclination = (rand() % 270);

    //SDMoonDeclination = grUpdateMoonPos(timeOfDay);
    //SDMoonDeclination = 22.0; /*(rand() % 270);*/

    const float moonAscension = SDTrack->local.sunascension;

    thesky->setMD( DEG2RAD(SDMoonDeclination) );
    thesky->setMRA( DEG2RAD(moonAscension) );

    GfLogInfo("  Moon : declination = %.1f deg, ascension = %.1f deg\n",
              SDMoonDeclination, moonAscension);

    /*

    SDCloudLayer *layer = new SDCloudLayer(datapath);
    layer->setCoverage(layer->SD_CLOUD_CIRRUS);
    layer->setSpeed(30);
    layer->setDirection(20);
    layer->setElevation_m(3000);
    layer->setThickness_m(400  / domeSizeRatio);
    layer->setTransition_m(400  / domeSizeRatio);
    layer->setSpan_m(SDSkyDomeDistance / 2);
    thesky->add_cloud_layer(layer);

    SDCloudLayer *layer2 = new SDCloudLayer(datapath);
    layer2->setCoverage(layer2->SD_CLOUD_CIRRUS2);
    layer2->setSpeed(60);
    layer2->setDirection(20);
    layer2->setElevation_m(1500);
    layer2->setThickness_m(400  / domeSizeRatio);
    layer2->setTransition_m(400  / domeSizeRatio);
    layer2->setSpan_m(SDSkyDomeDistance / 2);
    thesky->add_cloud_layer(layer2);*/

    // Initialize the whole sky dome.
    SDScenery * scenery = (SDScenery *)getScenery();
    double r_WrldX = scenery->getWorldX();
    double r_WrldY = scenery->getWorldY();
    //double r_WrldZ = SDScenery::getWorldZ();
    osg::Vec3 viewPos(r_WrldX / 2, r_WrldY/ 2, 0.0 );

    weather();
    thesky->set_visibility( SDVisibility ); // Visibility in meters

    thesky->reposition( viewPos, 0, 0);
    sol_angle = (float)thesky->getSA();
    moon_angle = (float)thesky->getMA();
    thesky->repaint(SkyColor, FogColor, CloudsColor, sol_angle, moon_angle, NPlanets,
                    APlanetsData, NStars, AStarsData);
    UpdateLight();

    osg::ref_ptr<osg::Group> sceneGroup = new osg::Group;
    osg::ref_ptr<osg::Group> mRoot = new osg::Group;
    osg::ref_ptr<osgShadow::ShadowMap> vdsm = new osgShadow::ShadowMap;
    m_scene = new osg::Group;
    m_CarRoot = new osg::Group;
    m_RealRoot = new osg::Group;
    shadowRoot = new osgShadow::ShadowedScene;

    osg::ref_ptr<osgParticle::PrecipitationEffect> precipitationEffect = new osgParticle::PrecipitationEffect;

    if (SDVisibility < 2000)
    {
        sceneGroup->addChild(precipitationEffect.get());
    }

    osg::ref_ptr<osg::Group> scene = new osg::Group;
    osg::ref_ptr<osg::Group> background = new osg::Group;
    osg::ref_ptr<osg::Group> cargroup = new osg::Group;

    scene->addChild(scenery->getScene());
    cargroup->addChild(m_CarRoot.get());
    background->addChild(scenery->getBackground());

	if(ShadowIndex > 0)
	{
		switch (QualityIndex+1)
		{
		case 0:
			break;
		case 1:
			scene->setNodeMask( rcvShadowMask );
			background->setNodeMask(~(rcvShadowMask | castShadowMask));
			cargroup->setNodeMask(castShadowMask);
			break;
		case 2:
			scene->setNodeMask( rcvShadowMask );
			background->setNodeMask(~(rcvShadowMask | castShadowMask));
			cargroup->setNodeMask(rcvShadowMask | castShadowMask);
			break;
		case 3:
			scene->setNodeMask( rcvShadowMask | castShadowMask);
			background->setNodeMask(~(rcvShadowMask | castShadowMask));
			cargroup->setNodeMask(rcvShadowMask | castShadowMask);
			break;
		default:
			break;
		}
	}

	m_scene->addChild(cargroup.get());
    m_scene->addChild(scene.get());
    m_scene->addChild(background.get());

    sceneGroup->addChild(m_scene.get());

    stateSet = new osg::StateSet;
    stateSet = m_scene->getOrCreateStateSet();
    stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);
    if (SDVisibility < 2000)
        stateSet->setAttributeAndModes(precipitationEffect->getFog());

    float emis = 0.5f * sky_brightness;
    float ambian = 0.8f * sky_brightness;
    osg::ref_ptr<osg::Material> material = new osg::Material;
    material->setColorMode(osg::Material::OFF); // switch glColor usage off
    Scene_ambiant = osg::Vec4f( ambian, ambian, ambian, 1.0f);            ;
    material->setEmission(osg::Material::FRONT_AND_BACK, osg::Vec4(emis, emis, emis, 1.0f));
    material->setAmbient(osg::Material::FRONT_AND_BACK, Scene_ambiant);
    stateSet->setAttributeAndModes(material, osg::StateAttribute::OVERRIDE|osg::StateAttribute::ON);
    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OVERRIDE|osg::StateAttribute::ON);

    lightSource = new osg::LightSource;
    lightSource->getLight()->setDataVariance(osg::Object::DYNAMIC);
    lightSource->getLight()->setLightNum(0);
    // relative because of CameraView being just a clever transform node
    lightSource->setReferenceFrame(osg::LightSource::RELATIVE_RF);
    lightSource->setLocalStateSetModes(osg::StateAttribute::ON);
    lightSource->getLight()->setAmbient(osg::Vec4(0.0f, 0.0f, 0.0f, 0.0f));
    lightSource->getLight()->setDiffuse(osg::Vec4( 0.2f, 0.2f, 0.2f, 1.0f));
    lightSource->getLight()->setSpecular(osg::Vec4(0.0f, 0.0f, 0.0f, 0.0f));
    sceneGroup->addChild(lightSource);

    // we need a white diffuse light for the phase of the moon
    sunLight = new osg::LightSource;
    sunLight->getLight()->setDataVariance(osg::Object::DYNAMIC);
    sunLight->getLight()->setLightNum(1);
    sunLight->setReferenceFrame(osg::LightSource::RELATIVE_RF);
    sunLight->setLocalStateSetModes(osg::StateAttribute::ON);
    sunLight->getLight()->setAmbient(SceneAmbiant);
    sunLight->getLight()->setDiffuse(SceneDiffuse);
    sunLight->getLight()->setSpecular(SceneSpecular);
    sunLight->setStateSetModes(*stateSet,osg::StateAttribute::ON);

    osg::Vec3f sun_position = thesky->sunposition();
    osg::Vec3f sun_direction = -sun_position;
    osg::Vec4f position(sun_position, 1.0f);
    sunLight->getLight()->setPosition(position);
    sunLight->getLight()->setDirection(sun_direction);

    skyGroup = new osg::Group;
    skyGroup->setName("skyCloudsGroup");
    skyGroup->setNodeMask(thesky->BACKGROUND_BIT);
    skyGroup->addChild(thesky->getPreRoot());
    skyGroup->addChild((thesky->getCloudRoot()));

    skySS = new osg::StateSet;
    skySS = skyGroup->getOrCreateStateSet();
    skySS->setMode(GL_LIGHT0, osg::StateAttribute::OFF);
    skySS->setAttributeAndModes( new osg::ColorMask( true, true, true, false ), osg::StateAttribute::ON );

    skyGroup->setNodeMask(~(rcvShadowMask | castShadowMask));
    sunLight->addChild(skyGroup.get());

    mRoot->addChild(sceneGroup.get());
    mRoot->setStateSet(setFogState().get());
    mRoot->addChild(sunLight.get());

    // Clouds are added to the scene graph later
    osg::ref_ptr<osg::StateSet> stateSet2 = new osg::StateSet;
    stateSet2 = mRoot->getOrCreateStateSet();
    stateSet2->setMode(GL_ALPHA_TEST, osg::StateAttribute::ON);
    stateSet2->setMode(GL_LIGHTING, osg::StateAttribute::ON);
    stateSet2->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);

    m_RealRoot->addChild(mRoot.get());

    GfOut("LE POINTEUR %d\n", mRoot.get());
}//SDRender::Init

void SDRender::ShadowedScene()
{
    if (ShadowIndex == 1)
    {
        unsigned int shadowTexUnit = 3;

        osg::ref_ptr<osgShadow::ShadowMap> vdsm = new osgShadow::ShadowMap;
        vdsm->setLight(sunLight.get());
        vdsm->setTextureSize(osg::Vec2s(ShadowTexSize, ShadowTexSize));
        vdsm->setTextureUnit(shadowTexUnit);
        shadowRoot = new osgShadow::ShadowedScene;
        shadowRoot->setReceivesShadowTraversalMask(rcvShadowMask);
        shadowRoot->setCastsShadowTraversalMask(castShadowMask);
        shadowRoot->setShadowTechnique((vdsm.get()));
    }
    else if (ShadowIndex  == 2)
    {
        osg::ref_ptr<osgShadow::SoftShadowMap> ssm = new osgShadow::SoftShadowMap;
        ssm->setLight(sunLight.get());
        ssm->setTextureSize(osg::Vec2s(ShadowTexSize, ShadowTexSize));
        ssm->setSoftnessWidth(1.0f);
        shadowRoot = new osgShadow::ShadowedScene;
        shadowRoot->setReceivesShadowTraversalMask(rcvShadowMask);
        shadowRoot->setCastsShadowTraversalMask(castShadowMask);
        shadowRoot->setShadowTechnique((ssm.get()));
    }
    else if (ShadowIndex == 3)
    {
        osg::ref_ptr<osgShadow::ParallelSplitShadowMap> pssm =
                new osgShadow::ParallelSplitShadowMap(NULL, 3);
        pssm->setTextureResolution(ShadowTexSize);
        pssm->setMinNearDistanceForSplits(0.25f);
        pssm->setMaxFarDistance(512);
        pssm->setPolygonOffset(osg::Vec2(10.0f, 20.0f));
        shadowRoot = new osgShadow::ShadowedScene;
        shadowRoot->setReceivesShadowTraversalMask(rcvShadowMask);
        shadowRoot->setCastsShadowTraversalMask(castShadowMask);
        shadowRoot->setShadowTechnique((pssm.get()));
    }
    else if (ShadowIndex == 4)
    {
        osg::ref_ptr<osgShadow::LightSpacePerspectiveShadowMapCB> lspsm =
                new osgShadow::LightSpacePerspectiveShadowMapCB;

        unsigned int baseTexUnit = 0;
        unsigned int shadowTexUnit = 3;

        lspsm->setMinLightMargin(0.5f);
        lspsm->setMaxFarPlane(2048.0f);
        lspsm->setTextureSize(osg::Vec2s(ShadowTexSize, ShadowTexSize));
        lspsm->setShadowTextureCoordIndex(shadowTexUnit);
        lspsm->setShadowTextureUnit(shadowTexUnit);
        lspsm->setBaseTextureCoordIndex(baseTexUnit);

        lspsm->setBaseTextureUnit(baseTexUnit);
        shadowRoot = new osgShadow::ShadowedScene;
        shadowRoot->setReceivesShadowTraversalMask(rcvShadowMask);
        shadowRoot->setCastsShadowTraversalMask(castShadowMask);
        shadowRoot->setShadowTechnique((lspsm.get()));
    }
    else if (ShadowIndex == 5)
    {
        osg::DisplaySettings::instance()->setMinimumNumStencilBits(8);

        osg::ref_ptr<osgShadow::ShadowVolume> sv = new osgShadow::ShadowVolume;
        sv->setDynamicShadowVolumes(1);
        sv->setDrawMode(osgShadow::ShadowVolumeGeometry::GEOMETRY);

        shadowRoot = new osgShadow::ShadowedScene;
        shadowRoot->setReceivesShadowTraversalMask(rcvShadowMask);
        shadowRoot->setCastsShadowTraversalMask(castShadowMask);
        shadowRoot->setShadowTechnique((sv.get()));
    }
    else if (ShadowIndex == 6)
    {
        osg::ref_ptr<osgShadow::ShadowSettings> shadowSettings = new osgShadow::ShadowSettings;
        shadowSettings->setTextureSize(osg::Vec2s(ShadowTexSize, ShadowTexSize));
        shadowSettings->setBaseShadowTextureUnit(3);
        shadowSettings->setLightNum(1);
        shadowSettings->setShaderHint(osgShadow::ShadowSettings::NO_SHADERS);

        shadowRoot = new osgShadow::ShadowedScene;

        osg::ref_ptr<osgShadow::ViewDependentShadowMap> shadowTechnique = new osgShadow::ViewDependentShadowMap;

        shadowRoot->setShadowSettings(shadowSettings);
        shadowRoot->setShadowTechnique(shadowTechnique);
        shadowRoot->setReceivesShadowTraversalMask(rcvShadowMask);
        shadowRoot->setCastsShadowTraversalMask(castShadowMask);
    }

    shadowRoot->addChild(m_scene.get());
    shadowRoot->addChild(m_CarRoot.get());
    shadowRoot->addChild(sunLight.get());

    m_RealRoot->removeChild(0, m_RealRoot->getNumChildren());

    m_RealRoot->addChild(shadowRoot.get());
    m_RealRoot->addChild(thesky->getCloudRoot());
}

void SDRender::addCars(osg::Node* cars)
{
    m_CarRoot->addChild(cars);

    osgUtil::Optimizer optimizer;
    optimizer.optimize(m_CarRoot.get());
    optimizer.optimize(m_scene.get());

    if ((ShadowIndex > 0) & (SDVisibility > 4000))
        ShadowedScene();
}

void SDRender::UpdateLight( void )
{
    sol_angle = (float)thesky->getSA();
    moon_angle = (float)thesky->getMA();
    sky_brightness = (float)(1.0 + cos(sol_angle)) / 2.0f;

    if (SDTrack->local.rain > 0)
    {
        BaseFogColor = osg::Vec3f(0.42f, 0.44f, 0.50f);
        sky_brightness = (float)pow(sky_brightness, 0.5f);
    }
    else
    {
        BaseFogColor = osg::Vec3f(0.84f, 0.87f, 1.00f);
    }

    SkyColor = BaseSkyColor * sky_brightness;

    UpdateFogColor(sol_angle);

    sd_gamma_correct_rgb( SkyColor._v );

    // 3a)cloud and fog color
    CloudsColor = FogColor = BaseFogColor * sky_brightness;

    //UpdateFogColor(sol_angle);
    sd_gamma_correct_rgb( CloudsColor._v );

    osg::Vec4f suncolor = thesky->get_sun_color();
    osg::Vec3f sun_color = osg::Vec3f(suncolor._v[0], suncolor._v[1], suncolor._v[2]);

    if (sol_angle > 1.0)
    {
        if (SDVisibility > 1000 /*&& cloudsTextureIndex < 8*/)
        {
            CloudsColor = osg::componentMultiply(CloudsColor, sun_color);
        }
        else
        {
            CloudsColor = CloudsColor * sun_color[0];
        }
    }

    sd_gamma_correct_rgb( CloudsColor._v );

    // 3b) repaint the sky (simply update geometrical, color, ... state, no actual redraw)
    thesky->repaint(SkyColor, FogColor, CloudsColor, sol_angle, moon_angle,
                    NPlanets, APlanetsData, NStars, AStarsData);

    // 3c) update scene colors.
    if (SDVisibility > 1000 /*&& cloudsTextureIndex < 8*/)
    {
        SceneAmbiant = osg::Vec4f((sun_color * 0.25f) + (CloudsColor * 0.75f) * sky_brightness, 1.0f);
        SceneDiffuse = osg::Vec4f((sun_color * 0.25f) + (FogColor * 0.75f) * sky_brightness, 1.0f);
        SceneSpecular = osg::Vec4f(sun_color * sky_brightness, 1.0f);
    }
    else
    {
        SceneAmbiant = osg::Vec4f(((CloudsColor._v[0] * 0.75f) + (sun_color._v[0] * 0.25f)) * sky_brightness,
                ((CloudsColor._v[1] * 0.75f) + (sun_color._v[0] * 0.25f)) * sky_brightness,
                ((CloudsColor._v[2] * 0.75f) + (sun_color._v[0] * 0.25f)) * sky_brightness, 1.0f);
        SceneDiffuse = osg::Vec4f(((FogColor._v[0] * 0.75f) + (sun_color._v[0] * 0.25f)) * sky_brightness,
                ((FogColor._v[1] * 0.75f) + (sun_color._v[0] * 0.25f)) * sky_brightness,
                ((FogColor._v[2] * 0.75f) + (sun_color._v[0] * 0.25f)) * sky_brightness, 1.0f);
        SceneSpecular = osg::Vec4f(sun_color._v[0] * sky_brightness, sun_color._v[0] * sky_brightness,
                sun_color._v[0] * sky_brightness, 1.0f);
    }
}//grUpdateLight

osg::ref_ptr< osg::StateSet> SDRender::setFogState()
{
    static const double m_log01 = -log( 0.01 );
    static const double sqrt_m_log01 = sqrt( m_log01 );
    const GLfloat fog_exp2_density = sqrt_m_log01 / thesky->get_visibility();

    SceneFog = osg::Vec4f(FogColor, 1.0f);

    osg::ref_ptr<osg::Fog> fog = new osg::Fog();    //The fog object
    fog->setMode(osg::Fog::EXP2);                   //Fog type
    fog->setDensity(fog_exp2_density);              //Fog density
    fog->setColor(SceneFog);                        //Fog color
    fog->setFogCoordinateSource(osg::Fog::FRAGMENT_DEPTH);
    osg::ref_ptr< osg::StateSet> fogState (new osg::StateSet);
    fogState->setAttributeAndModes(fog.get(), osg::StateAttribute::ON);

    fogState->setMode(GL_FOG, osg::StateAttribute::ON);

    return fogState.get();
}

void SDRender::UpdateFogColor(double sol_angle)
{
    double rotation;

    // first determine the difference between our view angle and local
    // direction to the sun
    rotation = -(thesky->getSR() + SD_PI);
    while ( rotation < 0 )
    {
        rotation += SD_2PI;
    }

    while ( rotation > SD_2PI )
    {
        rotation -= SD_2PI;
    }

    // revert to unmodified values before usign them.
    //
    osg::Vec4f sun_color = thesky->get_sun_color();

    sd_gamma_correct_rgb( BaseFogColor._v );

    // Calculate the fog color in the direction of the sun for
    // sunrise/sunset effects.
    float s_red =   (BaseFogColor._v[0] + 2 * sun_color._v[0] * sun_color._v[0]) / 3;
    float s_green = (BaseFogColor._v[1] + 2 * sun_color._v[1] * sun_color._v[1]) / 3;
    float s_blue =  (BaseFogColor._v[2] + 2 * sun_color._v[2] * sun_color._v[2]) / 3;

    // interpolate beween the sunrise/sunset color and the color
    // at the opposite direction of this effect. Take in account
    // the current visibility.
    float av = thesky->get_visibility();
    if (av > 45000)
        av = 45000;

    float avf = 0.87 - (45000 - av) / 83333.33;
    float sif = 0.5 - cos( sol_angle * 2)/2;

    if (sif < 1e-4)
        sif = 1e-4;

    float rf1 = fabs((rotation - SD_PI) / SD_PI);             // 0.0 .. 1.0
    float rf2 = avf * pow(rf1 * rf1, 1 /sif);
    float rf3 = 0.94 - rf2;

    FogColor._v[0] = rf3 * BaseFogColor._v[0] + rf2 * s_red;
    FogColor._v[1] = rf3 * BaseFogColor._v[1] + rf2 * s_green;
    FogColor._v[2] = rf3 * BaseFogColor._v[2] + rf2 * s_blue;
    sd_gamma_correct_rgb( FogColor._v );

    // make sure the colors have their original value before they are being
    // used by the rest of the program.
    sd_gamma_correct_rgb( BaseFogColor._v );
}

void SDRender::UpdateSky(double currentTime, double accelTime)
{
    // Detect first call (in order to initialize "last times").
    static bool bInitialized = false;
    static double lastTimeHighSpeed = 0;
    static int lastTimeLowSpeed = 0;

    // Nothing to do if static sky dome, or race not started.
    //if (!grDynamicSkyDome)	//TODO(kilo): find some meaning for this variable
    /*if (!SDSkyDomeDistance || SDTrack->skyversion < 1)
        return;*/

    if (currentTime < 0)
    {
        bInitialized = false;
        return;
    }

    if (!bInitialized)
    {
        if ( SDSkyDomeDistance )
        {
            // Ensure the sun and moon positions are reset
            const int timeOfDay = (int)SDTrack->local.timeofday;
            GLfloat sunAscension = SDTrack->local.sunascension;
            SDSunDeclination = (float)(15 * (double)timeOfDay / 3600 - 90.0);

            const float moonAscension = SDTrack->local.sunascension;
            //SDMoonDeclination = grUpdateMoonPos(timeOfDay);

            thesky->setSD( DEG2RAD(SDSunDeclination));
            thesky->setSRA( sunAscension );

            thesky->setMD( DEG2RAD(SDMoonDeclination) );
            thesky->setMRA( DEG2RAD(moonAscension) );
        }

        lastTimeHighSpeed = currentTime;
        lastTimeLowSpeed = 60 * (int)floor(accelTime / 60.0);

        bInitialized = true;
        return;
    }

    // At each call, update possibly high speed objects of the sky dome : the clouds.
    scenery = (SDScenery *)getScenery();
    double r_WrldX = scenery->getWorldX();
    double r_WrldY = scenery->getWorldY();

    osg::Vec3 viewPos(r_WrldX / 2, r_WrldY/ 2, 0.0 );
    thesky->reposition(viewPos, 0, currentTime - lastTimeHighSpeed);

    // Now, we are done for high speed objects.
    lastTimeHighSpeed = currentTime;

    // Check if time to update low speed objects : sun and moon (once every minute).
    int nextTimeLowSpeed = 60 * (int)floor((accelTime + 60.0) / 60.0);

    const float deltaTimeLowSpeed = (float)(nextTimeLowSpeed - lastTimeLowSpeed);

    // Update sun and moon, and thus global lighting / coloring parameters of the scene.
    if (nextTimeLowSpeed != lastTimeLowSpeed)
    {
        // 1) Update sun position
        const float deltaDecl = deltaTimeLowSpeed * 360.0f / (24.0f * 60.0f * 60.0f);
        SDSunDeclination += deltaDecl;
        if (SDSunDeclination >= 360.0f)
            SDSunDeclination -= 360.0f;

        thesky->setSD( DEG2RAD(SDSunDeclination) );

        // 2) Update moon position
        SDMoonDeclination += deltaDecl;
        if (SDMoonDeclination >= 360.0f)
            SDMoonDeclination -= 360.0f;

        thesky->setMD( DEG2RAD(SDMoonDeclination) );
        lastTimeLowSpeed = nextTimeLowSpeed;
    }

    // 3) Update scene color and light
    UpdateLight();

    sunLight->getLight()->setAmbient(SceneAmbiant);
    sunLight->getLight()->setDiffuse(SceneDiffuse);
    sunLight->getLight()->setSpecular(SceneSpecular);
    sunLight->setStateSetModes(*stateSet,osg::StateAttribute::ON);

    float emis = 0.5f * sky_brightness;
    float ambian = 0.8f * sky_brightness;

    Scene_ambiant = osg::Vec4f(ambian, ambian, ambian, 1.0f);
    osg::ref_ptr<osg::Material> material = new osg::Material;

    material->setEmission(osg::Material::FRONT_AND_BACK, osg::Vec4(emis, emis, emis, 1.0f));
    material->setAmbient(osg::Material::FRONT_AND_BACK, Scene_ambiant);
    stateSet->setAttributeAndModes(material, osg::StateAttribute::OVERRIDE|osg::StateAttribute::ON);
    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OVERRIDE|osg::StateAttribute::ON);

    osg::Vec3f sun_position = thesky->sunposition();
    osg::Vec3f sun_direction = -sun_position;
    osg::Vec4f position(sun_position, 1.0f);
    sunLight->getLight()->setPosition(position);
    sunLight->getLight()->setDirection(sun_direction);

}//grUpdateSky

void SDRender::weather(void)
{
    std::string datapath = GetDataDir();
    double domeSizeRatio = SDSkyDomeDistance / 80000.0;

    // Cloud layers.
    SDNbCloudLayers =
            (unsigned)(GfParmGetNum(grHandle, GR_SCT_GRAPHIC, GR_ATT_CLOUDLAYER, 0, 0) + 0.5);

    GfLogInfo("Graphic options : Number of cloud layers : %u\n", SDNbCloudLayers);

    cloudsTextureIndex = CloudsTextureIndices[SDTrack->local.clouds];
    unsigned int SDRain = 0;

    switch (SDTrack->local.rain)
    {
    case TR_RAIN_NONE:
        SDVisibility = SDMax_Visibility;
        SDRain = 0;
        break;
    case TR_RAIN_LITTLE:
        SDVisibility = 800.0;
        SDRain = 1;
        break;
    case TR_RAIN_MEDIUM:
        SDVisibility = 400.0;
        SDRain = 2;
        break;
    case TR_RAIN_HEAVY:
        SDVisibility = 200.0;
        SDRain = 3;
        break;
    default:
        GfLogWarning("Unsupported rain strength value %d (assuming none)",
                     SDTrack->local.rain);
        SDVisibility = 12000.0;
        break;
    }//switch Rain

    if (SDRain > 0)
    {
        SDCloudLayer *layer = new SDCloudLayer(datapath);
        layer->setCoverage(layer->SD_CLOUD_OVERCAST);
        layer->setSpeed(60);
        layer->setDirection(20);
        layer->setElevation_m(1000);
        layer->setThickness_m(400  / domeSizeRatio);
        layer->setTransition_m(400  / domeSizeRatio);
        layer->setSpan_m(SDSkyDomeDistance / 2);
        thesky->add_cloud_layer(layer);
    }
    else if (SDNbCloudLayers == 1)
    {
        SDCloudLayer *layer = new SDCloudLayer(datapath);
        layer->setCoverage(layer->SD_CLOUD_CIRRUS);
        layer->setSpeed(30);
        layer->setDirection(20);
        layer->setElevation_m(8000);
        layer->setThickness_m(400  / domeSizeRatio);
        layer->setTransition_m(400  / domeSizeRatio);
        layer->setSpan_m(SDSkyDomeDistance / 2);
        thesky->add_cloud_layer(layer);
    }
    else if (SDNbCloudLayers == 2)
    {
        SDCloudLayer *layer = new SDCloudLayer(datapath);
        layer->setCoverage(layer->SD_CLOUD_CIRRUS);
        layer->setSpeed(30);
        layer->setDirection(50);
        layer->setElevation_m(8000);
        layer->setThickness_m(400  / domeSizeRatio);
        layer->setTransition_m(400  / domeSizeRatio);
        layer->setSpan_m(SDSkyDomeDistance / 2);
        thesky->add_cloud_layer(layer);

        SDCloudLayer *layer2 = new SDCloudLayer(datapath);
        layer2->setCoverage(layer2->SD_CLOUD_FEW);
        layer2->setSpeed(80);
        layer2->setDirection(50);
        layer2->setElevation_m(3500);
        layer2->setThickness_m(400  / domeSizeRatio);
        layer2->setTransition_m(400  / domeSizeRatio);
        layer2->setSpan_m(SDSkyDomeDistance / 2);
        thesky->add_cloud_layer(layer2);
    }
    else if (SDNbCloudLayers == 3)
    {
        SDCloudLayer *layer = new SDCloudLayer(datapath);
        layer->setCoverage(layer->SD_CLOUD_CIRRUS);
        layer->setSpeed(30);
        layer->setDirection(20);
        layer->setElevation_m(8000);
        layer->setThickness_m(400  / domeSizeRatio);
        layer->setTransition_m(400  / domeSizeRatio);
        layer->setSpan_m(SDSkyDomeDistance / 2);
        thesky->add_cloud_layer(layer);

        SDCloudLayer *layer2 = new SDCloudLayer(datapath);
        layer2->setCoverage(layer2->SD_CLOUD_SCATTERED);
        layer2->setSpeed(60);
        layer2->setDirection(20);
        layer2->setElevation_m(3500);
        layer2->setThickness_m(400  / domeSizeRatio);
        layer2->setTransition_m(400  / domeSizeRatio);
        layer2->setSpan_m(SDSkyDomeDistance / 2);
        thesky->add_cloud_layer(layer2);

        SDCloudLayer *layer3 = new SDCloudLayer(datapath);
        layer3->setCoverage(layer2->SD_CLOUD_FEW);
        layer3->setSpeed(90);
        layer3->setDirection(20);
        layer3->setElevation_m(2500);
        layer3->setThickness_m(400  / domeSizeRatio);
        layer3->setTransition_m(400  / domeSizeRatio);
        layer3->setSpan_m(SDSkyDomeDistance / 2);
        thesky->add_cloud_layer(layer3);
    }
}

osg::Vec4f SDRender::getSceneColor(void)
{
    return Scene_ambiant;
}
