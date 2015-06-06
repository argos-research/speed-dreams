/***************************************************************************

file                 : network.cpp
created              : July 2009
copyright            : (C) 2009 Brian Gavin
web                  : speed-dreams.sourceforge.net
version              : $Id: robotxml.cpp 5746 2013-12-05 07:19:29Z mungewell $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

/*
   This file will generate the xml file needed for a robot file.
   Currently only used for networkhuman.xml
 */

#include "robotxml.h"

RobotXml::RobotXml()
{
}

bool RobotXml::CreateRobotFile(const char*pRobotName,std::vector<NetDriver> &vecDrivers)
{
    char buf[255];
    sprintf(buf,"drivers/%s/%s.xml",pRobotName,pRobotName);
    void *params = GfParmReadFileLocal(buf,GFPARM_RMODE_CREAT);
    //Erase existing robots
    GfParmListClean(params, "Robots");
    char path2[256];

    for (int i=0;i<(int)vecDrivers.size();i++)
    {
        sprintf(path2, "Robots/index/%d",i+1);
        GfParmSetStr(params, path2, "name",vecDrivers[i].name);
        GfParmSetStr(params, path2, "short name",vecDrivers[i].sname);
        GfParmSetStr(params, path2, "code name", vecDrivers[i].cname);
        GfParmSetStr(params, path2, "car name",vecDrivers[i].car);
        GfParmSetNum(params, path2, "race number", (char*)NULL,(tdble) vecDrivers[i].racenumber);
        GfParmSetNum(params, path2, "red", (char*)NULL, vecDrivers[i].red);
        GfParmSetNum(params, path2, "green", (char*)NULL, vecDrivers[i].green);
        GfParmSetNum(params, path2, "blue", (char*)NULL, vecDrivers[i].blue);
        GfParmSetStr(params, path2, "type",vecDrivers[i].type);
        GfParmSetStr(params, path2, "skill level",vecDrivers[i].skilllevel);
        GfParmSetStr(params, path2, "networkrace","yes");
        if (vecDrivers[i].client)
            GfParmSetStr(params, path2, "client","yes");
        else
            GfParmSetStr(params, path2, "client","no");


        char hostName[256];
        enet_address_get_host_ip (&vecDrivers[i].address,hostName,256);
        GfParmSetStr(params, path2, "host",hostName);
        GfParmSetNum(params, path2, "port",(char*)NULL, vecDrivers[i].address.port);
    }

    //Save our changes
    GfParmWriteFileLocal(buf, params, pRobotName);

    GfParmReleaseHandle(params);

    return true;
}

bool RobotXml::ReadRobotDrivers(const char*pRobotName,std::vector<NetDriver> &vecDrivers)
{
    char buf[255];
    sprintf(buf,"drivers/%s/%s.xml",pRobotName,pRobotName);
    void *params = GfParmReadFileLocal(buf,GFPARM_RMODE_REREAD);

    char path2[256];

    //bool bFound = true;
    //int i = 1;
    int nPlayers = GfParmGetEltNb(params, "Robots/index");

    for (int i= 1;i<=nPlayers;i++)
    {
        sprintf(path2, "Robots/index/%i",i);
        NetDriver driver;
        strncpy(driver.name,GfParmGetStr(params, path2, "name",NULL),64);
        std::string strClient = GfParmGetStr(params, path2, "client",NULL);
        if (strClient == "yes")
            driver.client = true;
        else 
            driver.client = false;

        strncpy(driver.car, GfParmGetStr(params, path2, "short name", NULL), 64);
        strncpy(driver.car, GfParmGetStr(params, path2, "code name",NULL), 3);
        strncpy(driver.car,GfParmGetStr(params, path2, "car name",NULL),64);
        strncpy(driver.type,GfParmGetStr(params, path2, "type",NULL),64);
        strncpy(driver.skilllevel,GfParmGetStr(params, path2, "skill level",NULL),64);

        driver.racenumber = (int)GfParmGetNum(params, path2, "race number",NULL,1.0);
        driver.red = GfParmGetNum(params, path2, "red",NULL,1.0);
        driver.green = GfParmGetNum(params, path2, "green",NULL,1.0);
        driver.blue = GfParmGetNum(params, path2, "blue",NULL,1.0);
        std::string strHost = GfParmGetStr(params, path2, "host",NULL);

        ENetAddress address;
        enet_address_set_host (& address, strHost.c_str());
        driver.address.host = address.host;
        driver.address.port = (enet_uint16)GfParmGetNum(params, path2, "port",NULL,0);
        strncpy(driver.module,NETWORKROBOT,64);
        vecDrivers.push_back(driver);
    }

    GfParmReleaseHandle(params);
    return true;
}

