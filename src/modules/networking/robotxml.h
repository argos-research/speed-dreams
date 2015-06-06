/***************************************************************************

file                 : robotxml.h
created              : July 2009
copyright            : (C) 2009 Brian Gavin
web                  : speed-dreams.sourceforge.net
version              : $Id: robotxml.h 5537 2013-06-24 20:50:53Z torcs-ng $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef ROBOTXML_H
#define ROBOTXML_H

#include <vector>

#include "network.h"

class RobotXml
{
public:
    RobotXml();

    bool CreateRobotFile(const char*pRobotName,std::vector<NetDriver> &vecDriver);
    bool ReadRobotDrivers(const char*pRobotName,std::vector<NetDriver> &vecDrivers);
};

#endif // ROBOTXML_H
