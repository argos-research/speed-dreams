/***************************************************************************

    file        : OsgScreens.cpp
    created     : Sun Jan 13 22:11:03 CEST 2013
    copyright   : (C) 2013 by Xavier Bertaux
    email       : bertauxx@yahoo.fr
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

#include <tgfclient.h>


#include <osgViewer/Viewer>
#include <osgViewer/GraphicsWindow>
#include <osg/GraphicsContext>
#include <osg/ValueObject>

#include "OsgScreens.h"

#include "OsgDebugHUD.h"
#include "OsgFX/OsgReflectionMapping.h"
#include "OsgMain.h"
#include "OsgCar/OsgCar.h"

SDScreens::SDScreens()
   :m_CurrentScreenIndex(0)
{
    debugHUD = new SDDebugHUD();
    viewer = new osgViewer::Viewer();
    viewer->setSceneData(new osg::Group());
}

class CameraDrawnCallback : public osg::Camera::DrawCallback
{
public:
   virtual void operator()(const osg::Camera& cam) const
   {
        SDCars * cars = (SDCars*)getCars();
        osg::Matrixf mat = cam.getViewMatrix();
        cars->updateShadingParameters(mat);
   }
};


void SDScreens::Init(int x,int y, int width, int height, osg::ref_ptr<osg::Node> m_sceneroot)
{

    m_Winx = x;
    m_Winy = y;
    m_Winw = width;
    m_Winh = height;

    //intialising main screen
    osg::ref_ptr<osg::Camera> mirrorCam = new osg::Camera;

    SDView * view = new SDView(viewer->getCamera(),0,0,m_Winw,m_Winh,mirrorCam.get());

    viewer->setThreadingModel(osgViewer::Viewer::CullThreadPerCameraDrawThreadPerContext);
    osg::ref_ptr<osgViewer::GraphicsWindowEmbedded> gw = viewer->setUpViewerAsEmbeddedInWindow(0, 0, m_Winw, m_Winh);
    //viewer->getCamera()->setName("Cam one");
    viewer->getCamera()->setViewport(new osg::Viewport(0, 0, m_Winw, m_Winh));
    viewer->getCamera()->setGraphicsContext(gw);
    viewer->getCamera()->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR); 
    viewer->getCamera()->setPreDrawCallback(new CameraDrawnCallback);

    mirrorCam->setGraphicsContext(gw);
    mirrorCam->setClearMask( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    mirrorCam->setReferenceFrame( osg::Camera::ABSOLUTE_RF );

    Screens.insert(Screens.end(),view);

    root = new osg::Group;
    prerenderRoot = new osg::Group;
    root->addChild(prerenderRoot);
       // root->addChild(reflectionMapping->getCamerasRoot());
    root->addChild(m_sceneroot.get());
    root->addChild(mirrorCam);
    mirrorCam->addChild(m_sceneroot.get());

    //adding all otherer cams
    osg::ref_ptr<osg::Camera> screenCam;
    for(int i=1;i<SD_NB_MAX_SCREEN;i++)
    {
        screenCam = new osg::Camera;
        screenCam->setGraphicsContext(gw);
        screenCam->setClearMask( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
        screenCam->setReferenceFrame( osg::Camera::ABSOLUTE_RF );
        screenCam->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
        screenCam->addChild(m_sceneroot);
        screenCam->setPreDrawCallback(new CameraDrawnCallback);
        screenCam->setNodeMask(0);

        mirrorCam = new osg::Camera;
        mirrorCam->setGraphicsContext(gw);
        mirrorCam->setClearMask( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
        mirrorCam->setReferenceFrame( osg::Camera::ABSOLUTE_RF );
        mirrorCam->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
        mirrorCam->addChild(m_sceneroot);
        mirrorCam->setNodeMask(0);

        view = new SDView(screenCam.get(),0,0,m_Winw,m_Winh,mirrorCam.get());

        Screens.insert(Screens.end(),view);

        root->addChild(screenCam.get());
        root->addChild(mirrorCam.get());
    }



    //debugHUD->setTexture(reflectionMapping->getMap());
    // debugHUD->setTexture(reflectionMapping->getReflectionMap());
    root->addChild(debugHUD->getRootCamera());

    viewer->setSceneData(root.get());
    viewer->realize();
}

void SDScreens::InitCars(tSituation *s)
{
    //char buf[256];
    //char	idx[16];
    //int		index;
    int		i;
    tCarElt 	*elt;
    //void	*hdle;
    const char *pszSpanSplit;
    int grNbSuggestedScreens = 0;

    for (i = 0; i < s->_ncars; i++) 
    {
    	elt = s->cars[i];
        //index = elt->index;
        //hdle = elt->_paramsHandle;

    	// WARNING: This index hack on the human robot for the Career mode
    	//          does no more work with the new "welcome" module system
    	//          (the "normal" index has no more the 10 limit) ... TO BE FIXED !!!!!!!
    	/*if (elt->_driverType == RM_DRV_HUMAN && elt->_driverIndex > 10)
        sprintf(idx, "Robots/index/%d", elt->_driverIndex - 11);
    else
        sprintf(idx, "Robots/index/%d", elt->_driverIndex);

    grCarInfo[index].iconColor[0] = GfParmGetNum(hdle, idx, "red",   (char*)NULL, GfParmGetNum(hdle, ROB_SECT_ARBITRARY, "red",   NULL, 0));
    grCarInfo[index].iconColor[1] = GfParmGetNum(hdle, idx, "green", (char*)NULL, GfParmGetNum(hdle, ROB_SECT_ARBITRARY, "green", NULL, 0));
    grCarInfo[index].iconColor[2] = GfParmGetNum(hdle, idx, "blue",  (char*)NULL, GfParmGetNum(hdle, ROB_SECT_ARBITRARY, "blue",  NULL, 0));
    grCarInfo[index].iconColor[3] = 1.0;
    grInitCar(elt);*/

    	// Pre-assign each human driver (if any) to a different screen
    	// (set him as the "current driver" for this screen).
    	if (grNbSuggestedScreens < SD_NB_MAX_SCREEN
        	&& elt->_driverType == RM_DRV_HUMAN && !elt->_networkPlayer)
    	{
        	Screens[grNbSuggestedScreens]->setCurrentCar(elt);
        	GfLogTrace("Screen #%d : Assigned to %s\n", grNbSuggestedScreens, elt->_name);
        	grNbSuggestedScreens++;
    	}
    }

    /* Check whether view should be spanned across vertical splits */
    pszSpanSplit = GfParmGetStr(grHandle, GR_SCT_GRAPHIC, GR_ATT_SPANSPLIT, GR_VAL_NO);
    m_SpanSplit = strcmp(pszSpanSplit, GR_VAL_YES) ? 0 : 1;

    if (m_SpanSplit == 0 && grNbSuggestedScreens > 1) 
    {
        // Mulitplayer, so ignore the stored number of screens
        m_NbActiveScreens = grNbSuggestedScreens;
        m_NbArrangeScreens = 0;
    } else 
    {
            // Load the real number of active screens and the arrangment.
        m_NbActiveScreens = (int)GfParmGetNum(grHandle, GR_SCT_DISPMODE, GR_ATT_NB_SCREENS, NULL, 1.0);
        m_NbArrangeScreens = (int)GfParmGetNum(grHandle, GR_SCT_DISPMODE, GR_ATT_ARR_SCREENS, NULL, 0.0);
    }

    // Initialize the cameras for all the screens.
    for (unsigned i=0; i<Screens.size();i++)
    {
        Screens[i]->Init(s);
    }

    // Setup the screens (= OpenGL viewports) inside the physical game window.
    this->AdaptScreenSize();
}

void SDScreens::update(tSituation * s,SDFrameInfo* fi)
{
    for (unsigned i=0;i< Screens.size();i++)
    {
        Screens[i]->update(s,fi);
    }


    SDCars * cars = (SDCars *)getCars();
    tCarElt * c = this->getActiveView()->getCurrentCar();
    this->debugHUD->setTexture(cars->getCar(c)->getReflectionMap()->getReflectionMap());

    if (!viewer->done())
        viewer->frame();
}

void SDScreens::splitScreen(long p){
    switch (p) {
        case SD_SPLIT_ADD:
            if (m_NbActiveScreens < SD_NB_MAX_SCREEN)
                m_NbActiveScreens++;
                if (m_SpanSplit)
                    m_NbArrangeScreens=1;
                else
                    m_NbArrangeScreens=0;
            break;
        case SD_SPLIT_REM:
            if (m_NbActiveScreens > 1)
                m_NbActiveScreens--;
                if (m_SpanSplit)
                    m_NbArrangeScreens=1;
                else
                    m_NbArrangeScreens=0;
            break;
        case SD_SPLIT_ARR:
            m_NbArrangeScreens++;
    }

    // Ensure current screen index stays in the righ range.
    if (m_CurrentScreenIndex >= m_NbActiveScreens)
        m_CurrentScreenIndex = m_NbActiveScreens - 1;

    // Save nb of active screens to user settings.
    GfParmSetNum(grHandle, GR_SCT_DISPMODE, GR_ATT_NB_SCREENS, NULL, m_NbActiveScreens);
    GfParmSetNum(grHandle, GR_SCT_DISPMODE, GR_ATT_ARR_SCREENS, NULL, m_NbArrangeScreens);
    GfParmWriteFile(NULL, grHandle, "Graph");
    AdaptScreenSize();
}

void SDScreens::AdaptScreenSize()
{
    int i;

    switch (m_NbActiveScreens)
    {
        default:
        case 1:
            // Hack to allow span-split to function OK
            if (m_NbArrangeScreens > 1) m_NbArrangeScreens = 0;

            // Always Full window.
            Screens[0]->activate(m_Winx, m_Winy, m_Winw, m_Winh, 0.0);
            for (i=1; i < SD_NB_MAX_SCREEN; i++)
                Screens[i]->deactivate();
            break;
        case 2:
            switch (m_NbArrangeScreens) {
            default:
                m_NbArrangeScreens = 0;
            case 0:
                // Top & Bottom half of the window
                Screens[0]->activate(m_Winx, m_Winy + m_Winh / 2, m_Winw, m_Winh / 2, 0.0);
                Screens[1]->activate(m_Winx, m_Winy,              m_Winw, m_Winh / 2, 0.0);
                break;
            case 1:
                // Left & Right half of the window
                Screens[0]->activate(m_Winx,              m_Winy, m_Winw / 2, m_Winh, m_SpanSplit * (-0.5 + 10));
                Screens[1]->activate(m_Winx + m_Winw / 2, m_Winy, m_Winw / 2, m_Winh, m_SpanSplit * (0.5 + 10));
                break;
            case 2:
                // 33/66% Left/Right
                Screens[0]->activate(m_Winx,              m_Winy, m_Winw / 3,   m_Winh, 0.0);
                Screens[1]->activate(m_Winx + m_Winw / 3, m_Winy, m_Winw * 2/3, m_Winh, 0.0);
                break;
            case 3:
                // 66/33% Left/Right
                Screens[0]->activate(m_Winx,                m_Winy, m_Winw * 2/3, m_Winh, 0.0);
                Screens[1]->activate(m_Winx + m_Winw * 2/3, m_Winy, m_Winw / 3,   m_Winh, 0.0);
                break;
            }

            for (i=2; i < SD_NB_MAX_SCREEN; i++)
                Screens[i]->deactivate();
            break;
        case 3:
            switch (m_NbArrangeScreens) 
            {
            default:
                m_NbArrangeScreens = 0;
            case 0:
                // Left/Right below wide
                Screens[0]->activate(m_Winx,              m_Winy + m_Winh / 2, m_Winw,     m_Winh / 2, 0.0);
                Screens[1]->activate(m_Winx,              m_Winy,              m_Winw / 2, m_Winh / 2, 0.0);
                Screens[2]->activate(m_Winx + m_Winw / 2, m_Winy,              m_Winw / 2, m_Winh / 2, 0.0);
                break;
            case 1:
                // All side by side
                Screens[0]->activate(m_Winx,                m_Winy, m_Winw / 3,   m_Winh, m_SpanSplit * (-1 + 10));
                Screens[1]->activate(m_Winx + m_Winw / 3,   m_Winy, m_Winw / 3,   m_Winh, m_SpanSplit * (0.0 + 10));
                Screens[2]->activate(m_Winx + m_Winw * 2/3, m_Winy, m_Winw / 3,   m_Winh, m_SpanSplit * (1 + 10));
                break;
            case 2:
                // Left/Right above wide
                Screens[0]->activate(m_Winx,              m_Winy + m_Winh / 2, m_Winw / 2, m_Winh / 2, 0.0);
                Screens[1]->activate(m_Winx + m_Winw / 2, m_Winy + m_Winh / 2, m_Winw / 2, m_Winh / 2, 0.0);
                Screens[2]->activate(m_Winx,              m_Winy,              m_Winw,     m_Winh / 2, 0.0);
                break;
            case 3:
                // 50/50% Left plus Top/Bottom on Right
                Screens[0]->activate(m_Winx,              m_Winy,              m_Winw / 2, m_Winh, 0.0);
                Screens[1]->activate(m_Winx + m_Winw / 2, m_Winy + m_Winh / 2, m_Winw / 2, m_Winh / 2, 0.0);
                Screens[2]->activate(m_Winx + m_Winw / 2, m_Winy,              m_Winw / 2, m_Winh / 2, 0.0);
                break;
            case 5:
                // 50/50% Top/Bottom on Left plus Right
                Screens[0]->activate(m_Winx,              m_Winy + m_Winh / 2, m_Winw / 2, m_Winh / 2, 0.0);
                Screens[1]->activate(m_Winx + m_Winw / 2, m_Winy,              m_Winw / 2, m_Winh, 0.0);
                Screens[2]->activate(m_Winx,              m_Winy,              m_Winw / 2, m_Winh / 2, 0.0);
                break;
            case 6:
                // 66/33% Left plus Top/Bottom on Right
                Screens[0]->activate(m_Winx,                m_Winy,              m_Winw * 2/3, m_Winh, 0.0);
                Screens[1]->activate(m_Winx + m_Winw * 2/3, m_Winy + m_Winh / 2, m_Winw / 3,   m_Winh / 2, 0.0);
                Screens[2]->activate(m_Winx + m_Winw * 2/3, m_Winy,              m_Winw / 3,   m_Winh / 2, 0.0);
                break;
            case 7:
                // 33/66% Top/Bottom on Left plus Right
                Screens[0]->activate(m_Winx,              m_Winy + m_Winh / 2, m_Winw / 3,   m_Winh / 2, 0.0);
                Screens[1]->activate(m_Winx + m_Winw / 3, m_Winy,              m_Winw * 2/3, m_Winh, 0.0);
                Screens[2]->activate(m_Winx,              m_Winy,              m_Winw / 3,   m_Winh / 2, 0.0);
                break;
            }
            
            for (i=3; i < SD_NB_MAX_SCREEN; i++)
                Screens[i]->deactivate();
            break;
        case 4:
            switch (m_NbArrangeScreens) 
            {
            case 8:
                // 3 side by side + 1 for rear view - only when spanning splits
                if (m_SpanSplit) 
                {
                    Screens[0]->activate(m_Winx,                m_Winy, m_Winw / 4,   m_Winh, m_SpanSplit * (-1 + 10));
                    Screens[1]->activate(m_Winx + m_Winw / 4,   m_Winy, m_Winw / 4,   m_Winh, m_SpanSplit * (0.0 + 10));
                    Screens[2]->activate(m_Winx + m_Winw * 2/4, m_Winy, m_Winw / 4,   m_Winh, m_SpanSplit * (1 + 10));
                    Screens[3]->activate(m_Winx + m_Winw * 3/4, m_Winy, m_Winw / 4,   m_Winh, 0.0);
                    break;
                }
            default:
                m_NbArrangeScreens = 0;
            case 0:
                // Top/Bottom Left/Rigth Quarters
                Screens[0]->activate(m_Winx,              m_Winy + m_Winh / 2, m_Winw / 2, m_Winh / 2, 0.0);
                Screens[1]->activate(m_Winx + m_Winw / 2, m_Winy + m_Winh / 2, m_Winw / 2, m_Winh / 2, 0.0);
                Screens[2]->activate(m_Winx,              m_Winy,              m_Winw / 2, m_Winh / 2, 0.0);
                Screens[3]->activate(m_Winx + m_Winw / 2, m_Winy,              m_Winw / 2, m_Winh / 2, 0.0);
                break;
            case 1:
                // All side by side
                Screens[0]->activate(m_Winx,                m_Winy, m_Winw / 4,   m_Winh, m_SpanSplit * (-1.5 + 10));
                Screens[1]->activate(m_Winx + m_Winw / 4,   m_Winy, m_Winw / 4,   m_Winh, m_SpanSplit * (-0.5 + 10));
                Screens[2]->activate(m_Winx + m_Winw * 2/4, m_Winy, m_Winw / 4,   m_Winh, m_SpanSplit * (0.5 + 10));
                Screens[3]->activate(m_Winx + m_Winw * 3/4, m_Winy, m_Winw / 4,   m_Winh, m_SpanSplit * (1.5 + 10));
                break;
            case 2:
                // Left/Middle/Right above wide
                Screens[0]->activate(m_Winx,                m_Winy + m_Winh / 2, m_Winw / 3, m_Winh / 2, 0.0);
                Screens[1]->activate(m_Winx + m_Winw / 3,   m_Winy + m_Winh / 2, m_Winw / 3, m_Winh / 2, 0.0);
                Screens[2]->activate(m_Winx + m_Winw * 2/3, m_Winy + m_Winh / 2, m_Winw / 3, m_Winh / 2, 0.0);
                Screens[3]->activate(m_Winx,                m_Winy,              m_Winw,     m_Winh / 2, 0.0);
                break;
            case 3:
                // Left/Middle/Right below wide
                Screens[0]->activate(m_Winx,                m_Winy + m_Winh / 2, m_Winw,     m_Winh / 2, 0.0);
                Screens[1]->activate(m_Winx,                m_Winy,              m_Winw / 3, m_Winh / 2, 0.0);
                Screens[2]->activate(m_Winx + m_Winw / 3,   m_Winy,              m_Winw / 3, m_Winh / 2, 0.0);
                Screens[3]->activate(m_Winx + m_Winw * 2/3, m_Winy,              m_Winw / 3, m_Winh / 2, 0.0);
                break;
            case 4:
                // 50/50% Left plus Top/Middle/Bottom on Right
                Screens[0]->activate(m_Winx,              m_Winy,                m_Winw / 2, m_Winh, 0.0);
                Screens[1]->activate(m_Winx + m_Winw / 2, m_Winy + m_Winh * 2/3, m_Winw / 2, m_Winh / 3, 0.0);
                Screens[2]->activate(m_Winx + m_Winw / 2, m_Winy + m_Winh / 3,   m_Winw / 2, m_Winh / 3, 0.0);
                Screens[3]->activate(m_Winx + m_Winw / 2, m_Winy,                m_Winw / 2, m_Winh / 3, 0.0);
                break;
            case 5:
                // 50/50% Top/Middle/Bottom on Left plus Right
                Screens[0]->activate(m_Winx,              m_Winy + m_Winh * 2/3, m_Winw / 2, m_Winh / 3, 0.0);
                Screens[1]->activate(m_Winx + m_Winw / 2, m_Winy,                m_Winw / 2, m_Winh, 0.0);
                Screens[2]->activate(m_Winx,              m_Winy + m_Winh / 3  , m_Winw / 2, m_Winh / 3, 0.0);
                Screens[3]->activate(m_Winx,              m_Winy,                m_Winw / 2, m_Winh / 3, 0.0);
                break;
            case 6:
                // 66/33% Left plus Top/Middle/Bottom on Right
                Screens[0]->activate(m_Winx,                m_Winy,                m_Winw * 2/3, m_Winh, 0.0);
                Screens[1]->activate(m_Winx + m_Winw * 2/3, m_Winy + m_Winh * 2/3, m_Winw / 3,   m_Winh / 3, 0.0);
                Screens[2]->activate(m_Winx + m_Winw * 2/3, m_Winy + m_Winh / 3,   m_Winw / 3,   m_Winh / 3, 0.0);
                Screens[3]->activate(m_Winx + m_Winw * 2/3, m_Winy,                m_Winw / 3,   m_Winh / 3, 0.0);
                break;
            case 7:
                // 33/66% Top/Middle/Bottom on Left plus Right
                Screens[0]->activate(m_Winx,              m_Winy + m_Winh * 2/3, m_Winw / 3,   m_Winh / 3, 0.0);
                Screens[1]->activate(m_Winx + m_Winw / 3, m_Winy,                m_Winw * 2/3, m_Winh, 0.0);
                Screens[2]->activate(m_Winx,              m_Winy + m_Winh / 3  , m_Winw / 3,   m_Winh / 3, 0.0);
                Screens[3]->activate(m_Winx,              m_Winy,                m_Winw / 3,   m_Winh / 3, 0.0);
                break;
            }
            for (i=4; i < SD_NB_MAX_SCREEN; i++)
                Screens[i]->deactivate();
            break;
        case 5:
            switch (m_NbArrangeScreens) 
            {
            default:
                m_NbArrangeScreens = 0;
            case 0:
                // Top/Bottom Left/Middle/Rigth Matrix
                Screens[0]->activate(m_Winx,                m_Winy + m_Winh / 2, m_Winw / 2, m_Winh / 2, 0.0);
                Screens[1]->activate(m_Winx + m_Winw / 2  , m_Winy + m_Winh / 2, m_Winw / 2, m_Winh / 2, 0.0);
                Screens[2]->activate(m_Winx,                m_Winy,              m_Winw / 3, m_Winh / 2, 0.0);
                Screens[3]->activate(m_Winx + m_Winw / 3,   m_Winy,              m_Winw / 3, m_Winh / 2, 0.0);
                Screens[4]->activate(m_Winx + m_Winw * 2/3, m_Winy,              m_Winw / 3, m_Winh / 2, 0.0);
                break;
            case 1:
                // All side by side
                Screens[0]->activate(m_Winx,                m_Winy, m_Winw / 5,   m_Winh, m_SpanSplit * (-2.0 + 10));
                Screens[1]->activate(m_Winx + m_Winw / 5,   m_Winy, m_Winw / 5,   m_Winh, m_SpanSplit * (-1.0 + 10));
                Screens[2]->activate(m_Winx + m_Winw * 2/5, m_Winy, m_Winw / 5,   m_Winh, m_SpanSplit * (0.0 + 10));
                Screens[3]->activate(m_Winx + m_Winw * 3/5, m_Winy, m_Winw / 5,   m_Winh, m_SpanSplit * (1.0 + 10));
                Screens[4]->activate(m_Winx + m_Winw * 4/5, m_Winy, m_Winw / 5,   m_Winh, m_SpanSplit * (2.0 + 10));
                break;
            }
            for (i=5; i < SD_NB_MAX_SCREEN; i++)
                Screens[i]->deactivate();
            break;
        case 6:
            switch (m_NbArrangeScreens) 
            {
            case 2:
                if (m_SpanSplit) 
                {
                    // five side by side + 1 for rear view - only when spanning splits
                    Screens[0]->activate(m_Winx,                m_Winy, m_Winw / 6,   m_Winh, m_SpanSplit * (-2.0 + 10));
                    Screens[1]->activate(m_Winx + m_Winw / 6,   m_Winy, m_Winw / 6,   m_Winh, m_SpanSplit * (-1.0 + 10));
                    Screens[2]->activate(m_Winx + m_Winw * 2/6, m_Winy, m_Winw / 6,   m_Winh, m_SpanSplit * (0.0 + 10));
                    Screens[3]->activate(m_Winx + m_Winw * 3/6, m_Winy, m_Winw / 6,   m_Winh, m_SpanSplit * (1.0 + 10));
                    Screens[4]->activate(m_Winx + m_Winw * 4/6, m_Winy, m_Winw / 6,   m_Winh, m_SpanSplit * (2.0 + 10));
                    Screens[5]->activate(m_Winx + m_Winw * 5/6, m_Winy, m_Winw / 6,   m_Winh, 0.0);
                    break;
                }
            default:
                m_NbArrangeScreens = 0;
            case 0:
                // Top/Bottom Left/Middle/Rigth Matrix
                Screens[0]->activate(m_Winx,                m_Winy + m_Winh / 2, m_Winw / 3, m_Winh / 2, 0.0);
                Screens[1]->activate(m_Winx + m_Winw / 3,   m_Winy + m_Winh / 2, m_Winw / 3, m_Winh / 2, 0.0);
                Screens[2]->activate(m_Winx + m_Winw * 2/3, m_Winy + m_Winh / 2, m_Winw / 3, m_Winh / 2, 0.0);
                Screens[3]->activate(m_Winx,                m_Winy,              m_Winw / 3, m_Winh / 2, 0.0);
                Screens[4]->activate(m_Winx + m_Winw / 3,   m_Winy,              m_Winw / 3, m_Winh / 2, 0.0);
                Screens[5]->activate(m_Winx + m_Winw * 2/3, m_Winy,              m_Winw / 3, m_Winh / 2, 0.0);
                break;
            case 1:
                // All side by side
                Screens[0]->activate(m_Winx,                m_Winy, m_Winw / 6,   m_Winh, m_SpanSplit * (-2.5 + 10));
                Screens[1]->activate(m_Winx + m_Winw / 6,   m_Winy, m_Winw / 6,   m_Winh, m_SpanSplit * (-1.5 + 10));
                Screens[2]->activate(m_Winx + m_Winw * 2/6, m_Winy, m_Winw / 6,   m_Winh, m_SpanSplit * (-0.5 + 10));
                Screens[3]->activate(m_Winx + m_Winw * 3/6, m_Winy, m_Winw / 6,   m_Winh, m_SpanSplit * (0.5 + 10));
                Screens[4]->activate(m_Winx + m_Winw * 4/6, m_Winy, m_Winw / 6,   m_Winh, m_SpanSplit * (1.5 + 10));
                Screens[5]->activate(m_Winx + m_Winw * 5/6, m_Winy, m_Winw / 6,   m_Winh, m_SpanSplit * (2.5 + 10));
                break;
            }
            
            for (i=6; i < SD_NB_MAX_SCREEN; i++)
                Screens[i]->deactivate();
            break;
        }
}

void SDScreens::changeScreen(long p)
{
    switch (p) 
    {
        case SD_NEXT_SCREEN:
            m_CurrentScreenIndex = (m_CurrentScreenIndex + 1) % m_NbActiveScreens;
            break;
        case SD_PREV_SCREEN:
            m_CurrentScreenIndex = (m_CurrentScreenIndex - 1 + m_NbActiveScreens) % m_NbActiveScreens;
            break;
    }
    
    GfLogInfo("Changing current screen to #%d (out of %d)\n", m_CurrentScreenIndex, m_NbActiveScreens);
}

void SDScreens::changeCamera(long p)
{
    this->getActiveView()->getCameras()->nextCamera(p);

    // For SpanSplit ensure screens change together
    if (m_SpanSplit && getActiveView()->getViewOffset() ) 
    {
        int i, camList,camNum;

        getActiveView()->getCameras()->getIntSelectedListAndCamera(&camList,&camNum);

        for (i=0; i < m_NbActiveScreens; i++)
            if (Screens[i]->getViewOffset() )
                Screens[i]->getCameras()->selectCamera(camList,camNum);
    }
}

void SDScreens::toggleDebugHUD(){
    debugHUD->toggleHUD();
}

void SDScreens::registerViewDependantPreRenderNode(osg::ref_ptr<osg::Node> node){
    //TODO : multi-screen support of this feature
    prerenderRoot->addChild(node);
}


SDScreens::~SDScreens()
{
    root->removeChildren(0, root->getNumChildren());
    root = NULL;

    for (unsigned i=0;i< Screens.size();i++)
    {
        delete Screens[i];
    }
    //root.release();
    viewer->setSceneData(new osg::Group());
    //delete viewer->getSceneData();

    delete viewer;
    delete debugHUD;
    viewer = NULL;
}
