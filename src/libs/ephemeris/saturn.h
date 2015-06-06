/****************************************************************************

    file                 : saturn.h
    created              : Sun Aug 02 22:54:56 CET 2012
    copyright            : (C) 1997  Durk Talsma - (C)2012 Xavier Bertaux
    email                : bertauxx@yahoo.fr
    version              : $Id: saturn.h 4811 2012-07-29 17:16:37Z torcs-ng $

 ****************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef _SATURN_H_
#define _SATURN_H_

#include "celestialbody.h"
#include "star.h"

class ePhSaturn : public ePhCelestialBody
{
public:
  ePhSaturn (double mjd);
  ePhSaturn ();
  void updatePosition(double mjd, ePhStar *ourSun);
};

#endif // _SATURN_H_
