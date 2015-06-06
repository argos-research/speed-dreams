/***************************************************************************

    file        : OsgScreens.h
    created     : Sat Feb 2013 15:52:19 CEST 2013
    copyright   : (C) 2013 by Gaëtan André
    email       : gaetan.andre@gmail.com
    version     : $Id$
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _OSGSCREENS_H_
#define _OSGSCREENS_H_


#include <raceman.h>        //tSituation
#include <vector>
#include "OsgView.h"

class SDDebugHUD;
class SDReflectionMapping;

#define SD_SPLIT_ADD	0
#define SD_SPLIT_REM	1
#define SD_SPLIT_ARR	2

#define SD_NEXT_SCREEN	0
#define SD_PREV_SCREEN	1

#define SD_NB_MAX_SCREEN 6

class SDScreens
{
    protected:
        osgViewer::Viewer* viewer;
        std::vector<SDView *> Screens;
        osg::ref_ptr<osg::Group> root;
        osg::ref_ptr<osg::Group> prerenderRoot;
        SDDebugHUD * debugHUD;


        int m_Winx;
        int m_Winy;
        int m_Winw;
        int m_Winh;

        int m_NbActiveScreens;
        int m_NbArrangeScreens;
        bool m_SpanSplit;
        int m_CurrentScreenIndex;

        void AdaptScreenSize();
//		int mirrorFlag;
//		void loadParams(tSituation *s);			// Load from parameters files.

	public:
        SDScreens();
        ~SDScreens();

        void Init(int x, int y, int width, int height, osg::ref_ptr<osg::Node> m_sceneroot);
        void InitCars(tSituation *s);
        void update(tSituation *s,SDFrameInfo* fi);
        void splitScreen(long p);
        void changeScreen(long p);
        void changeCamera(long p);
        void registerViewDependantPreRenderNode(osg::ref_ptr<osg::Node> node);
        void toggleDebugHUD();

        inline SDView * getActiveView(){return Screens[m_CurrentScreenIndex];}

	// void activate(int x, int y, int w, int h, float v);
	// inline void deactivate(void) { active = false; }	
	// inline void setCurrentCar(tCarElt *newCurCar) { curCar = newCurCar; }
};

#endif //_OSGSCREENS_H_
