/****************************************************************************

    file                 : star.cpp
    created              : Sun Aug 02 22:54:56 CET 2012
    copyright            : (C) 1997  Durk Talsma - (C)2012 Xavier Bertaux
    email                : bertauxx@yahoo.fr
    version              : $Id: star.cpp 4811 2012-07-29 17:16:37Z torcs-ng $

 ****************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef _STAR_H_
#define _STAR_H_

#include "celestialbody.h"

class ePhStar : public ePhCelestialBody
{

private:

    double xs, ys;     // the sun's rectangular geocentric coordinates
    double ye, ze;     // the sun's rectangularequatorial rectangular geocentric coordinates
    double distance;   // the sun's distance to the earth

public:

    ePhStar (double mjd);
    ePhStar ();
    ~ePhStar();
    void updatePosition(double mjd);
    double getM() const;
    double getw() const;
    double getxs() const;
    double getys() const;
    double getye() const;
    double getze() const;
    double getDistance() const;
};


inline double ePhStar::getM() const
{
  	return M;
}

inline double ePhStar::getw() const
{
  	return w;
}

inline double ePhStar::getxs() const
{
  	return xs;
}

inline double ePhStar::getys() const
{
  	return ys;
}

inline double ePhStar::getye() const
{
   	return ye;
}

inline double ePhStar::getze() const
{
   	return ze;
}

inline double ePhStar::getDistance() const
{
  	return distance;
}

#endif // _STAR_H_
