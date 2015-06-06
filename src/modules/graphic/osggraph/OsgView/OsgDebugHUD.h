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

#ifndef _OSGDEBUGHUD_H_
#define _OSGDEBUGHUD_H_


class SDDebugHUD
{
    private:
        osg::ref_ptr<osg::Camera> HUD_camera;

    public:
        void toggleHUD();
        void setTexture(osg::ref_ptr<osg::Texture> map);
        SDDebugHUD();
        ~SDDebugHUD();

        inline osg::ref_ptr<osg::Camera> getRootCamera(){
            return HUD_camera;
        }


};

#endif //_OSGDEBUGHUD_H_
