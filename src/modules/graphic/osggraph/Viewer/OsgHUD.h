/***************************************************************************

    file        : OsgHUD.h
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

#ifndef _OSGHUD_H_
#define _OSGHUD_H_

#include <car.h>        // tCarElt
#include <raceman.h>    // tSituation

#include <osg/Camera>

class SDFrameInfo;

class SDHUD
{
    private:
        osg::ref_ptr<osg::Camera> _cameraHUD;
        tCarElt *_car;

        bool    _debugFlag;
        bool    _arcadeFlag;
        bool    _boardFlag;
        bool    _leaderFlag;
        bool    _counterFlag;
        bool    _GFlag;

        //Dash colour handling
        float *_normal_color;
        float *_danger_color;
        float *_ok_color;
        float *_error_color;
        float *_inactive_color_;
        float *_emphasized_color;
        float *_ahead_color;
        float *_behind_color;
        float *_arcade_color;
        float *_background_color;

    public:
        SDHUD();
        ~SDHUD();

        void ToggleHUD1();
        void ToggleHUD2();
        void ToggleHUD3();
        void ToggleHUD4();
        void ToogleFPS();
        void ToogleHudBoard();

        void CreateHUD( int scrH, int scrW);
        void DispDebug(const tSituation *s, const SDFrameInfo* frame);
        void RefreshBoard(tSituation *s, const SDFrameInfo* frameInfo, const tCarElt *currCar);

        inline osg::ref_ptr<osg::Camera> getRootCamera()
        {
            return _cameraHUD;
        }
};

#endif //_OSGHUD_H_
