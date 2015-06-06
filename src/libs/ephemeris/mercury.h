/****************************************************************************

    file                 : mercury.h
    created              : Sun Aug 02 22:54:56 CET 2012
    copyright            : (C) 1997  Durk Talsma - (C)2012 Xavier Bertaux
    email                : bertauxx@yahoo.fr
    version              : $Id: mercury.h 4811 2012-07-29 17:16:37Z torcs-ng $

 ****************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef _MERCURY_H_
#define _MERCURY_H_

#include "celestialbody.h"
#include "star.h"

class ePhMercury : public ePhCelestialBody
{
public:
  ePhMercury (double mjd);
  ePhMercury ();
  void updatePosition(double mjd, ePhStar* ourSun);
};

#endif // _MERURY_H_
