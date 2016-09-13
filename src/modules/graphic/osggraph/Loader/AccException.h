/***************************************************************************

    file                 : OsgException.h
    created              : Mon Dec 31 10:24:02 CEST 2012
    copyright            : (C)201 by Xavier Bertaux
    email                : bertauxx@yahoo.fr
    version              : $Id$

***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef AC3D_EXCEPTION
#define AC3D_EXCEPTION 1

#include <string>

namespace acc3d
{
class Exception
{
public:
    Exception(std::string error);
    ~Exception();
    std::string getError(){return _error;};
private:
    std::string _error;
};
}

#endif
