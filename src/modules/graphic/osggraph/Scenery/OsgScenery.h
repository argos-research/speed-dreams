/***************************************************************************

    file                 : OsgScenery.h
    created              : Mon Aug 21 20:09:40 CEST 2012
    copyright            : (C) 2012 Xavier Bertaux
    email                : bertauxx@yahoo.fr
    version              : $Id: OsgScenery.h 1813 2012-11-10 13:45:43Z torcs-ng $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _OSGSCENERY_H_
#define _OSGSCENERY_H_

#include <string>

#include <track.h>	//tTrack
#include <raceman.h> // tSituation

#include <osg/Group>

class	SDBackground;
//class   SDPit;
//class   SDTrackLights;
class	SDScenery;
class   osgLoader;

#define SKYBIN 1
#define TRACKBIN 2

class SDBackground
{
    osg::ref_ptr<osg::Group>	 _background;

    bool	_type;

public:
    // Constructor
    SDBackground(void);

    // Destructor
    ~SDBackground(void);

    void build(bool type, int X, int Y, int Z, const std::string& strTrack);
    void reposition(int X, int Y);

    osg::ref_ptr<osg::Group> getBackground() { return _background.get(); }
};

/*class SDPit
{
    osg::ref_ptr<osg::Group>	 _osgpit;
    bool	_number;

public:
    // Constructor
    SDPit(void);

    // Destructor
    ~SDPit(void);

    void build(const std::string strTrack);

    osg::ref_ptr<osg::Group> getPit() { return _osgpit.get(); }
};

class SDTrackLights
{
private:
    osg::ref_ptr<osg::Group>    _osgtracklight;
    bool    _number;

public:
    // Constructor
    SDTrackLights(void);

    // Destructor
    ~SDTrackLights(void);

    void build(const std::string strTrack);
    osg::ref_ptr<osg::Group> getTrackLight() { return _osgtracklight.get(); }
};*/

static int grWrldX=0;
static int grWrldY=0;
static int grWrldZ = 0;
static int grWrldMaxSize=0;

class SDScenery
{
private:
    SDBackground	*m_background;
    //SDPit           *m_pit;
    //SDTrackLights   *m_tracklights;

    osg::ref_ptr<osg::Group> _scenery;

    tTrack *SDTrack;

    int _max_visibility;
    int _nb_cloudlayer;
    int _DynamicSkyDome;
    int _SkyDomeDistance;
    int _SkyDomeDistThresh;

    bool _bgtype;
    bool _bgsky;
    bool _speedWay;
    bool _speedWayLong;

    std::string _strTexturePath;

    void LoadGraphicsOptions();
    void LoadSkyOptions();
    void CustomizePits(void);
    bool LoadTrack(std::string& strTrack);

public:
    /* Constructor */
    SDScenery(void);

    /* Destructor */
    ~SDScenery(void);

    void LoadScene(tTrack *track);
    void ShutdownScene(void);

    inline static int getWorldX(){return grWrldX;}
    inline static int getWorldY(){return grWrldY;}
    inline static int getWorldZ(){return grWrldZ;}
    inline static int getWorldMaxSize(){return grWrldMaxSize;}
    bool getSpeedWay() { return _speedWay; }
	bool getSpeedWayLong() { return _speedWayLong; }

    osg::ref_ptr<osg::Group> getScene() { return _scenery.get(); }
    osg::ref_ptr<osg::Group> getBackground() { return m_background->getBackground(); }
    //osg::ref_ptr<osg::Group> getPit() { return m_pit->getPit(); }
    //osg::ref_ptr<osg::Group> getTracklight() { return m_tracklights->getTrackLight(); }
};

#endif //_OSGSCENERY_H_
