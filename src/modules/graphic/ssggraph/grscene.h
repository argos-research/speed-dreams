/***************************************************************************

    file                 : grscene.h
    created              : Mon Aug 21 20:09:40 CEST 2000
    copyright            : (C) 2000 by Eric Espie
    email                : torcs@free.fr
    version              : $Id: grscene.h 6113 2015-09-10 22:48:36Z torcs-ng $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _GRSCENE_H_
#define _GRSCENE_H_

//TODO: What is this??? kilo
#ifdef GUIONS
#include <glib.h>
#endif //GUIONS

#include <track.h>	//tTrack
#include <raceman.h> // tSituation
#include "grmultitexstate.h"


extern int grWrldX;
extern int grWrldY;
extern int grWrldZ;
extern int grWrldMaxSize;
extern bool grSpeedway;
extern bool grSpeedwayshort;
extern tTrack *grTrack;

extern ssgRoot *TheScene;
extern ssgBranch *LandAnchor;
extern ssgBranch *CarsAnchor;
extern ssgBranch *ShadowAnchor;
extern ssgBranch *PitsAnchor;
extern ssgBranch *SmokeAnchor;
extern ssgBranch *SkidAnchor;
extern ssgBranch *CarlightAnchor;
extern ssgBranch *TrackLightAnchor;
extern ssgBranch *ThePits;
extern ssgBranch *BackSkyAnchor;
extern ssgTransform *BackSkyLoc;

extern ssgStateSelector	*grEnvSelector;
extern cgrMultiTexState	*grEnvState;
extern cgrMultiTexState	*grEnvShadowState;
extern cgrMultiTexState	*grEnvShadowStateOnCars;

class cGrCamera;
class cGrBackgroundCam;

//!Public interface
extern int grInitScene(void);
extern int grLoadScene(tTrack *track);
extern void grDrawScene();
extern void grShutdownScene(void);
extern void grCustomizePits(void);
extern void grLoadPitsIndicator(tdble x,tdble y, tdble z, char *buf, int Pitind);
extern void grDrawBackground(class cGrCamera *, class cGrBackgroundCam *bgCam);

//TODO: Question: What is this??? kilo
//      Possible answer: Some try to lower GPU load by filtering the scene ? JP
#ifdef GUIONS
class cDoV
{
public:
  tdble FrontLevelGroupGlobal; /* the distance for the end of the front scene */
  tdble FrontLevelGroup1;      /* the distance for the end of the front scene for group type 1*/
  tdble FrontLevelGroup2;      /* the distance for the end of the front scene for group type 2*/
  tdble FrontLevelGroup3;      /* the distance for the end of the front scene for group type 3*/

  tdble RearLevelGroupGlobal; /* the distance for the end of the front scene */
  tdble RearLevelGroup1;
  tdble RearLevelGroup2;
  tdble RearLevelGroup3;

  tdble FrontLevelMap1;      /* the distance for the end of the front scene with only one mapping*/
  tdble FrontLevelMap2;      /* the distance for the end of the front scene with two mapping*/
  tdble FrontLevelMap3;      /* the distance for the end of the front scene with three mapping*/
  tdble RearLevelMap1;
  tdble RearLevelMap2;
  tdble RearLevelMap3;
};

class cHashMapElement
{
  char	*name;
  int		numberOfMapToApply;
};

class cDistanceOfViewHashing
{
public:
  char				*name;				//segment name
  GHashTable	*ViewGroup;		//all object to display group1+group2+group3 for this segment */
  int ViewGroup_num;				//number of object */
  int ViewGroupMap1_num;
  int ViewGroupMap2_num;
  int ViewGroupMap3_num;
};

extern cDistanceOfViewHashing_t *SceneHashing;
extern cDoV	*currentDistanceOfView;
extern cDoV	PlayableDistanceOfView;
extern cDoV	UnPlayableDistanceOfView;
#endif //GUIONS

#endif //_GRSCENE_H_
