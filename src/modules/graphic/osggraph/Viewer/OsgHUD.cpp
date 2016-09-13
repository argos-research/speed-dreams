/***************************************************************************

    file        : OsgHUD.cpp
    created     : Sun Nov 23 20:12:19 CEST 2014
    copyright   : (C) 2014 by Xavier Bertaux
    email       : Xavier Bertaux
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

#include <osg/Material>
#include <osg/Geode>
#include <osg/BlendFunc>
#include <osg/Depth>
#include <osg/PolygonOffset>
#include <osg/MatrixTransform>
#include <osg/Camera>
#include <osg/RenderInfo>

#include <osgDB/WriteFile>

#include <osgText/Text>

#include "OsgHUD.h"
#include "OsgMain.h"

SDHUD::SDHUD()
{
    _cameraHUD = new osg::Camera;
}

void SDHUD::CreateHUD(int scrH, int scrW)
{
    // set the projection matrix
    _cameraHUD->setProjectionMatrix(osg::Matrix::ortho2D(0, scrW, 0, scrH));

    // set the view matrix
    _cameraHUD->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    _cameraHUD->setViewMatrix(osg::Matrix::identity());

    // only clear the depth buffer
    _cameraHUD->setClearMask(GL_DEPTH_BUFFER_BIT);

    // draw subgraph after main camera view.
    _cameraHUD->setRenderOrder(osg::Camera::POST_RENDER);

    // we don't want the camera to grab event focus from the viewers main camera(s).
    _cameraHUD->setAllowEventFocus(false);

    osg::ref_ptr<osg::Geode> geode = new osg::Geode();

    std::string FontPath = GetDataDir();
    FontPath = FontPath+"data/fonts/vera.ttf";

    // turn lighting off for the text and disable depth test to ensure it's always ontop.
    osg::ref_ptr<osg::StateSet> stateset = geode->getOrCreateStateSet();
    stateset->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

    //osg::Vec3 position(150.0f,800.0f,0.0f);
    // osg::Vec3 delta(0.0f,-120.0f,0.0f);
}

void
SDHUD::DispDebug(const tSituation *s, const SDFrameInfo* frame)
{
    /*char buf[256];
    snprintf(buf, sizeof(buf), "FPS: %.1f(%.1f)  ",
             frame->fInstFps, frame->fAvgFps);
    int dx = GfuiFontWidth(GFUI_FONT_SMALL_C, buf);

    int x2 = rightAnchor - dx;    // 2nd column
    int y = TOP_ANCHOR - 15;
    int dy = GfuiFontHeight(GFUI_FONT_SMALL_C);
    int x = (_debugFlag > 1) ? x2 - dx : x2;   // 1st column

    // Display frame rates (instant and average)
    snprintf(buf, sizeof(buf), "FPS: %.1f(%.1f)",
             frame->fInstFps, frame->fAvgFps);
    GfuiDrawString(buf, normal_color_, GFUI_FONT_SMALL_C, x, y);

    if (_debugFlag == 2)
    {
        //Only display detailed information in Debug Mode > 1
        // Display frame counter in 2nd column
        snprintf(buf, sizeof(buf),  "Frm: %u", frame->nTotalFrames);
        GfuiDrawString(buf, normal_color_, GFUI_FONT_SMALL_C, x2, y);

        // Display current fovy in 2nd row, 1st column
        y -= dy;
        snprintf(buf, sizeof(buf), "FovY: %2.1f", curCam->getFovY());
        GfuiDrawString(buf, normal_color_, GFUI_FONT_SMALL_C, x, y);

        // Display simulation time in 2nd row, 2nd column
        snprintf(buf, sizeof(buf),  "Time: %.f", s->currentTime);
        GfuiDrawString(buf, normal_color_, GFUI_FONT_SMALL_C, x2, y);
    } else if (_debugFlag == 3)
    {
        // Only display detailed information in Debug Mode > 1
        // Display segment name in 2nd column
        snprintf(buf, sizeof(buf),  "Seg: %s", car_->_trkPos.seg->name);
        GfuiDrawString(buf, normal_color_, GFUI_FONT_SMALL_C, x2, y);

        // Display distance from start in 2nd row, 1st column
        y -= dy;
        snprintf(buf, sizeof(buf), "DfS: %5.0f", car_->_distFromStartLine);
        GfuiDrawString(buf, normal_color_, GFUI_FONT_SMALL_C, x, y);

        // Display current camera in 2nd row, 2nd column
        tRoadCam *curCam = car_->_trkPos.seg->cam;
        if (curCam)
        {
            snprintf(buf, sizeof(buf), "Cam: %s", curCam->name);
            GfuiDrawString(buf, normal_color_, GFUI_FONT_SMALL_C, x2, y);
        }
    }*/
}  // grDispDebug

void SDHUD::RefreshBoard(tSituation *s, const SDFrameInfo* frameInfo,
                            const tCarElt *currCar)
{
  /* _car = currCar;
  //DispMisc(isCurrScreen);

  if (_arcadeFlag)
  {
    //DispArcade(s);
  } else
  {
    if (_debugFlag)
      DispDebug(s, frameInfo);
    if (_GFlag)
      //DispGGraph();
    if (_boardFlag)
      //DispCarBoard(s);
    if (_leaderFlag)
      //DispLeaderBoard(s);
    if (_counterFlag)
      //DispCounterBoard2();
  }*/

  //trackMap->display(currCar, s, 0, 0, rightAnchor, TOP_ANCHOR);
}

void SDHUD::ToggleHUD1()
{
    _cameraHUD->setNodeMask(1-_cameraHUD->getNodeMask());
}

SDHUD::~SDHUD()
{
}
