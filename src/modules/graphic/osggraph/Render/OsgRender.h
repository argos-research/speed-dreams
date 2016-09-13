/***************************************************************************

    file                 : OsgRender.h
    created              : Mon Aug 21 20:09:40 CEST 2012
    copyright            : (C) 2012 Xavier Bertaux
    email                : bertauxx@yahoo.fr
    version              : $Id: OsgRender.h 1813 2012-11-10 13:45:43Z torcs-ng $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _OSGRENDER_H_
#define _OSGRENDER_H_

#include <osg/Group>
#include <osgShadow/ShadowedScene>

#include <track.h>	 //tTrack

struct Situation;
typedef Situation tSituation;

class SDSky;
class SDScenery;

class SDRender
{
private:
    osg::ref_ptr<osg::Group> m_RealRoot;
    osg::ref_ptr<osg::Group> m_scene;
    osg::ref_ptr<osg::Group> m_CarRoot;
    osg::ref_ptr<osg::Group> skyGroup;

    osg::ref_ptr<osg::StateSet> stateSet;
    osg::ref_ptr<osg::StateSet> skySS;

    osg::ref_ptr<osg::LightSource> lightSource;
    osg::ref_ptr<osg::LightSource> sunLight;

    osg::ref_ptr<osgShadow::ShadowedScene> shadowRoot;

    osg::Vec3f BaseSkyColor;
    osg::Vec3f BaseFogColor;
    osg::Vec3f SkyColor;
    osg::Vec3f FogColor;
    osg::Vec3f CloudsColor;

    osg::Vec4f SceneAmbiant;
    osg::Vec4f SceneDiffuse;
    osg::Vec4f SceneSpecular;
    osg::Vec4f SceneFog;

    osg::Vec4f Scene_ambiant;
    osg::Vec4f Scene_Diffuse;
    osg::Vec4f Scene_Specular;
    osg::Vec4f Scene_Emit;

    unsigned SDSkyDomeDistance;
    unsigned SDNbCloudLayers;
    unsigned SDSkyDomeDistThresh;
    unsigned int rcvShadowMask;
    unsigned int castShadowMask;
    int carsShader;
    int SDDynamicWeather;
    bool SDDynamicSkyDome;

    int cloudsTextureIndex;
    float SDSunDeclination;
    float SDMoonDeclination;
    float SDMax_Visibility;
    double SDVisibility;
    unsigned ShadowIndex;
    unsigned TexSizeIndex;
    unsigned QualityIndex;
    int ShadowTexSize;

    osg::Vec3d *AStarsData;
    osg::Vec3d *APlanetsData;
    int NStars;
    int NPlanets;
    float sol_angle;
    float moon_angle;
    float sky_brightness;

    SDSky *thesky;
    SDScenery *scenery;
    tTrack *SDTrack;

public:
    SDRender(void);
    ~SDRender(void);

    void Init(tTrack *track);
    osg::ref_ptr< osg::StateSet> setFogState();
    osg::Vec4f getSceneColor(void);
    void UpdateTime(tSituation *s);
    void UpdateLight(void);
    void addCars(osg::Node* cars);
    void UpdateFogColor(double angle);
    void UpdateSky(double currentTime, double accelTime);
    void ShadowedScene(void);
    void weather(void);

    inline SDSky * getSky() { return thesky; }
    inline int getShader() { return carsShader; }
	inline int getShaowIndex() { return ShadowIndex; }
    osg::ref_ptr<osg::Group> getRoot() { return m_RealRoot.get(); }
    osg::ref_ptr<osg::Group> getSceneRoot() { return m_scene.get(); }
    osg::ref_ptr<osg::Group> getCarRoot() { return m_CarRoot.get(); }
    osg::Vec3f getFogColor() { return FogColor; }
};

#endif //_OSGRENDER_H_
