/****************************************************************************

    file                 : moonpos.h
    created              : Sun Aug 02 22:54:56 CET 2012
    copyright            : (C) 1997  Durk Talsma - (C)2012 Xavier Bertaux
    email                : bertauxx@yahoo.fr
    version              : $Id: moonpos.h 4811 2012-07-29 17:16:37Z torcs-ng $

 ****************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef _MOONPOS_H_
#define _MOONPOS_H_


#include <constants.h>

#include "celestialbody.h"
#include "star.h"


class ePhMoonPos : public ePhCelestialBody
{

private:

public:
    ePhMoonPos(double mjd);
    ePhMoonPos();
    ~ePhMoonPos();
    void updatePosition(double mjd, double lst, double lat, ePhStar *ourSun);
    // void newImage();
};

#endif // _MOONPOS_H_
