/****************************************************************************

    file                 : stardata.h
    created              : Sun Aug 02 22:54:56 CET 2012
    copyright            : (C) 2000  Curtis L. Olson - (C)2012 Xavier Bertaux
    email                : bertauxx@yahoo.fr
    version              : $Id: stardata.h 4811 2012-07-29 17:16:37Z torcs-ng $

 ****************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#ifndef _STARDATA_H
#define _STARDATA_H

#include <vector>
#include <string>
#include <SGMath.h>

class ePhStarData 
{
public:
    // Constructor
    ePhStarData( const char *path );

    // Destructor
    ~ePhStarData();

    // load the stars database
    bool load( const char *path );

    // stars
    inline int getNumStars() const { return _stars.size(); }
    inline SGVec3d *getStars() { return &(_stars[0]); }

private:
    std::vector<SGVec3d> _stars;
};

#endif // _STARDATA_H
