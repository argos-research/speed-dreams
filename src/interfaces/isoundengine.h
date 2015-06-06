/***************************************************************************
                 isoundengine.h -- Interface for sound engines

    created              : Mon Jul 28 19:48:14 CEST 2012
    copyright            : (C) 2012 by Gaëtan André
    web                  : http://www.speed-dreams.org
    version              : $Id: isoundengine.h 5095 2013-01-12 17:57:47Z pouillot $
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

 #ifndef __ISOUNDENGINE__H__
 #define __ISOUNDENGINE__H__


#include "camera.h"

class ISoundEngine 
{
public:
 
    virtual void init(struct Situation* s) = 0;
    virtual void shutdown() = 0;
    virtual void refresh(struct Situation *s, Camera *camera) = 0;
    virtual void mute(bool bOn = true) = 0;
};
 
 #endif // __ISOUNDENGINE__H__
