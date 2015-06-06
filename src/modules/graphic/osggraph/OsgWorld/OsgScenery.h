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
//class	SDSpectators;
//class	SDTrees;
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

    void build(bool type, int X, int Y, int Z, const std::string strTrack);
	void reposition(int X, int Y);

    osg::Group* getBackground() { return _background.get(); }
};

/*class SDSpectators
{
private:

	osg::ref_prt<osg::Node>	_spectators;

	int	_number;

public:

	void SDSpectators(void);
	void ~SDSpectators(void);
	void build(int number, tTrack *track);
};

class SDTrees
{
private:

	osg::ref_ptr<osg::Node> _trees;

public:

	void SDTrees(void);
	void ~SDTrees(void);
	void build(tTrack *track);
};*/

static int grWrldX=0;
static int grWrldY=0;
static int grWrldZ = 0;
static int grWrldMaxSize=0;

class SDScenery
{
private:
	SDBackground	*m_background;
	//SDSpectators	*m_spectators;
	//SDTrees	*m_trees;
	osg::ref_ptr<osg::Group> _scenery;

    tTrack *SDTrack;

    int _grWrldX;
    int _grWrldY;
    int _grWrldZ;
    int _grWrldMaxSize;
    int _max_visibility;
    int _nb_cloudlayer;
    int _DynamicSkyDome;
    int _SkyDomeDistance;
    int _SkyDomeDistThresh;

	bool _bgtype;
	bool _bgsky;

	//_spectators = 0;
	//_trees = 0;
	//_pits = 0;
	std::string _strTexturePath;

	void LoadGraphicsOptions();
	void LoadSkyOptions();
	void CustomizePits(void);
	bool LoadTrack(std::string strTrack);


public:

	/* Constructor */
	SDScenery(void);

	/* Destructor */
	~SDScenery(void);

    void LoadScene(tTrack *track);
    void CreatePit(tTrack *track);
	//void	addSpectators(SDSpectators->build(number, tTrack *track));
	//void	addTrees(SDTrees->build(tTrack *track));
    void ShutdownScene(void);
	//void

    inline static int getWorldX(){return grWrldX;}
    inline static int getWorldY(){return grWrldY;}
    inline static int getWorldZ(){return grWrldZ;}
    inline static int getWorldMaxSize(){return grWrldMaxSize;}

    osg::Group* getScene() { return _scenery.get(); }
    osg::Group* getBackground() { return m_background->getBackground(); }

	//osg::ref_ptr<osg::Group>	getSDScenery { return _scenery };
	//osg::Group	getSDBackground { return SDBackground->getbackground; }
};

#endif //_OSGSCENERY_H_
