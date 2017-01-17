/***************************************************************************

    file                 : OsgException.cpp
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

#include "AccException.h"

using namespace acc3d;

Exception::Exception(std::string error) :
    _error(NULL)
{
    _error = error;
}

Exception::~Exception(){}
