/***************************************************************************
                 camera.h -- A generic camera

    created              : Sun Jan 6 19:48:14 CEST 2013
    copyright            : (C) 2013 by Gaëtan André
    web                  : http://www.speed-dreams.org
    version              : $Id: camera.h 5095 2013-01-12 17:57:47Z pouillot $
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef __CAMERA__H__
#define __CAMERA__H__

typedef float sndVec3[3];

struct Camera
{
    sndVec3 * Posv;
    sndVec3 * Speedv;
    sndVec3 * Centerv;
    sndVec3 * Upv;
};

 
#endif // __CAMERA__H__
