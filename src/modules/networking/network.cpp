/***************************************************************************

file                 : network.cpp
created              : July 2009
copyright            : (C) 2009 Brian Gavin
web                  : speed-dreams.sourceforge.net
version              : $Id: network.cpp 5854 2014-11-23 17:55:52Z wdbee $

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
   Overview
   The network file is used for networked game play. 
   The server computer handles all of the physics and robot AI.
   The server sends car control (steering, brake, throttle) frequently in unreliable packets.
   The client uses these values to figure out car position
   and does its own physics calculation until the server sends the position information.  

   Every CAR_CONTROL_UPDATE seconds, the server sends out detailed position 
   information in a reliable ENetPacket.  All cars position information is updated
   based on the server values.
 */ 

// Warning this code is VERY rough and unfinished

// TODO: Make a real SD module (dynamically loadable, like simuvx, human, ssggraph ...).


#include <cstdio>
#include <SDL.h>
#include <SDL_thread.h>

#include <graphic.h>

#include "network.h"
#include "pack.h"


bool g_bInit = false;

bool g_bClient = false;
bool g_bServer = false;
NetServer g_server;
NetClient g_client;
SDL_TimerID g_timerId;


NetMutexData::NetMutexData()
{
    m_networkMutex =  SDL_CreateMutex();
}

NetMutexData::~NetMutexData()
{
    SDL_DestroyMutex ( m_networkMutex );
}

void NetMutexData::Lock() 
{
    SDL_mutexP ( m_networkMutex );
}

void NetMutexData::Unlock() 
{
    SDL_mutexV ( m_networkMutex );
}

void NetMutexData::Init()
{
    m_vecCarCtrls.clear();
    m_vecCarStatus.clear();
    m_vecLapStatus.clear();
    m_finishTime = 0.0f;
}

//============================================================

void NetServerMutexData::Init()
{
    m_vecNetworkPlayers.clear();
}

NetServerMutexData::NetServerMutexData()
{
    m_networkMutex=  SDL_CreateMutex();
}

NetServerMutexData::~NetServerMutexData()
{
    SDL_DestroyMutex (m_networkMutex );
}

void NetServerMutexData::Lock() 
{
    SDL_mutexP ( m_networkMutex );
}

void NetServerMutexData::Unlock() 
{
    SDL_mutexV ( m_networkMutex );
}

//============================================================

NetNetwork::NetNetwork()
{
    m_strClass = "network";
    m_bRaceInfoChanged = false;
    m_bRefreshDisplay = false;

    m_sendCtrlTime = 0.0;
    m_sendCarDataTime = 0.0;
    m_pHost = NULL;
    m_currentTime = 0.0;
}

NetNetwork::~NetNetwork()
{
}

void NetNetwork::RaceInit(tSituation *s)
{
    m_sendCtrlTime = 0.0;
    m_sendCarDataTime = 0.0;
    m_timePhysics = 0.0;
    m_currentTime = 0.0;


    m_mapRanks.clear();
    for (int i = 0; i < s->_ncars; i++) 
    {
        tCarElt *pCar = s->cars[i];
        m_mapRanks[i] = pCar->info.startRank;
    }

    m_NetworkData.Init();

}

void NetNetwork::RaceDone()
{
    m_bRaceActive = false;
    m_bBeginRace = false;
    m_bPrepareToRace = false;
    m_bRaceInfoChanged = false;
    m_bTimeSynced = false;
    m_sendCtrlTime = 0.0;
    m_sendCarDataTime = 0.0;
    m_timePhysics = -2.0;

    m_mapRanks.clear();

}


int NetNetwork::GetDriverStartRank(int idx) 
{
    std::map<int,int>::iterator p;
    p = m_mapRanks.find(idx);

    return p->second;
}

//============================================================

NetDriver::NetDriver()
{
    //Initialize values
    idx = -1;
    memset(name,0,sizeof(name));
    memset(car,0,sizeof(car));
    memset(team,0,sizeof(team));
    memset(author,0,sizeof(author));
    racenumber = 1;
    memset(skilllevel,0,sizeof(skilllevel));
    red = 1.0;
    green = 1.0;
    blue = 1.0;
    hostPort = 0;
    client = false;
    memset(module,0,sizeof(module));
    memset(type,0,sizeof(type));
}

bool NetworkInit();

NetNetwork *NetGetNetwork()
{
    if (!g_bInit)
        NetworkInit();

    if (g_bServer)
        return &g_server;

    if (g_bClient)
        return &g_client;

    return NULL;
}


int NetNetwork::GetCarIndex(int startRank,tSituation *s)
{
    for (int i=0;i<s->_ncars;i++)
    {
        if (startRank == s->cars[i]->info.startRank)
            return i;
    }

    assert(false);
    return -1;
}

bool NetNetwork::IsServerMode() 
{
    if (m_strClass == (char*)"server")
        return true;

    return false;
}

void NetNetwork::SetRefreshDisplay(bool bStatus)
{
    m_bRefreshDisplay = bStatus;
    if (!bStatus)
        GfLogDebug("refreshdisplay false\n");
}

void NetNetwork::SetRaceInfoChanged(bool bStatus)
{
    m_bRaceInfoChanged = bStatus;
    if (bStatus)
        m_bRefreshDisplay = true;

    if (!bStatus)
        GfLogDebug("raceinfo false\n");
}

bool NetNetwork::IsClientMode() 
{
    if (m_strClass == "client")
        return true;

    return false;
}

// Get the index of the local network-human driver in the networkhuman interface list
int NetNetwork::GetNetworkHumanIdx()
{
    assert(m_strDriverName!="");
    int idx = 1;

    char buf[255];
    sprintf(buf,"drivers/networkhuman/networkhuman.xml");
    void *params = GfParmReadFileLocal(buf,GFPARM_RMODE_REREAD);
    assert(params);
    char path2[256];

    int i=0;
    const char *pName;
    do
    {
        i++;
        sprintf(path2, "Robots/index/%d",i);
        pName = GfParmGetStr(params, path2, "name",NULL);
        if (pName && strcmp(m_strDriverName.c_str(),pName)==0)
        {
            idx = i;
            break;
        }	
    }
    while(pName);

    GfParmReleaseHandle(params);

    return idx;
}

void NetNetwork::SetRaceXMLFile(char const*pXmlFile)
{
    m_strRaceXMLFile=pXmlFile;
}

int NetNetwork::GetPlayerCarIndex(tSituation *s)
{
    int i=0;
    while (s->cars[i]->info.startRank != (m_driverIdx-1))
        i++;

    return i;
}

void NetNetwork::ClearLocalDrivers()
{
    m_setLocalDrivers.clear();
}

void NetNetwork::SetDriverName(char *pName)
{
    m_strDriverName = pName;
    GfLogInfo("Setting network driver name: %s\n", pName);
}

const char *NetNetwork::GetDriverName()
{
    return m_strDriverName.c_str();
}

void NetNetwork::SetLocalDrivers()
{
}

void NetNetwork::SetCarInfo(const char *pszName)
{
}

bool NetNetwork::FinishRace(double time) 
{
    NetMutexData *pNData = LockNetworkData();
    double finishTime = pNData->m_finishTime;
    UnlockNetworkData();

    if (finishTime<=0.0)
        return false;

    if (time<finishTime)
        return false;

    GfLogInfo("Finishing network race\n");
    return true;	
}

NetMutexData * NetNetwork::LockNetworkData() 
{
    m_NetworkData.Lock();
    return & m_NetworkData;
}

void NetNetwork::UnlockNetworkData()
{
    m_NetworkData.Unlock();
}

void NetNetwork::BroadcastPacket(ENetPacket *pPacket,enet_uint8 channel)
{
}

// Get the index of the local network-human driver in the race driver list 
int	NetNetwork::GetDriverIdx()
{
    int nhidx = GetNetworkHumanIdx();

    assert(m_strRaceXMLFile!="");

    void *params = GfParmReadFileLocal(m_strRaceXMLFile.c_str(),GFPARM_RMODE_STD);
    assert(params);

    int nDriverIdx = -1;

    const int nCars = GfParmGetEltNb(params, RM_SECT_DRIVERS);
    for (int i=1;i<=nCars;i++)
    {
        NetDriver driver;
        ReadDriverData(driver,i,params);
        if (driver.idx == nhidx && strcmp(NETWORKROBOT,driver.module) == 0)
        {
            nDriverIdx = i;
            break;
        }
    }

    GfParmReleaseHandle(params);

    if (nDriverIdx == -1)
        GfLogError("Unable to GetDriverIdx %s\n", m_strDriverName.c_str());

    return nDriverIdx;
}

// Get the Driver instance with given index in the race driver list (= the race XML file)
void NetNetwork::ReadDriverData(NetDriver &driver,int index,void *params)
{
    char path2[256];
    sprintf(path2, "%s/%d", RM_SECT_DRIVERS, index);
    const char *pMod = GfParmGetStr(params, path2, RM_ATTR_MODULE,NULL);
    strncpy(&driver.module[0],pMod,64);
    driver.idx = (int)GfParmGetNum(params, path2, RM_ATTR_IDX, NULL,-1);
}

void NetNetwork::WriteDriverData(NetDriver driver,int index,void *params)
{
    char path2[256];
    sprintf(path2, "%s/%d", RM_SECT_DRIVERS, index);
    GfParmSetStr(params, path2, RM_ATTR_MODULE,driver.module);
    GfParmSetNum(params, path2, RM_ATTR_IDX, NULL,(tdble)driver.idx);
}

std::string NetNetwork::GetNetworkDriverName()
{
    return m_strDriverName;
}

bool NetNetwork::SetCurrentDriver()
{

    void *params = GfParmReadFileLocal("config/graph.xml",GFPARM_RMODE_REREAD);
    assert(params);

    const char *pName =GfParmGetStr(params, RM_SECT_HEADER, RM_ATTR_NAME, "");

    std::string strDriver = GetNetworkDriverName();
    if (strDriver =="")
        return false;

    char path[255];
    sprintf(path, "%s/%d", GR_SCT_DISPMODE, 0);
    GfParmSetStr(params, path, GR_ATT_CUR_DRV, strDriver.c_str());

    //Save our changes
    GfParmWriteFileLocal("config/graph.xml", params, pName);

    GfParmReleaseHandle(params);

    return true;
}

void NetNetwork::SendLapStatusPacket(tCarElt *pCar)
{
    PackedBuffer msg;

    try
    {
        msg.pack_ubyte(LAPSTATUS_PACKET);
        msg.pack_double(pCar->race.bestLapTime);
        msg.pack_double(*pCar->race.bestSplitTime);
        msg.pack_int(pCar->race.laps);
        msg.pack_int(pCar->info.startRank);
    }
//    catch (PackedBufferException &e)
    catch (PackedBufferException)
    {
        GfLogFatal("SendLapStatusPacket: packed buffer error\n");
    }
    GfLogTrace("SendLapStatusPacket: packed data length=%d\n",
            msg.length());

    ENetPacket *pPacket = enet_packet_create (msg.buffer(), 
            msg.length(), 
            ENET_PACKET_FLAG_RELIABLE);

    BroadcastPacket(pPacket,RELIABLECHANNEL);
}


void NetNetwork::SendCarStatusPacket(tSituation *s,bool bForce)
{
    if (s->currentTime<0.0)
        return;

    //Clock error fix it
    if (s->currentTime<m_sendCarDataTime)
    {
        m_sendCarDataTime=s->currentTime-CAR_DATA_UPDATE;
    }

    //Send carinfo packet when enough time has passed(CAR_DATA_UPDATE)
    if (((m_sendCarDataTime+CAR_DATA_UPDATE)>s->currentTime)&&(!bForce))
    {
        return;
    }

    std::vector<tCarElt *> local;
    double time = 0.0;


    //Pack controls values to reduce data size of packet
    for (int i = 0; i < s->_ncars; i++) 
    {
        tCarElt *pCar = s->cars[i];
        //Only transmit local drivers to other clients
        if (m_setLocalDrivers.find(pCar->info.startRank)!=m_setLocalDrivers.end())
        {
            local.push_back(pCar);
        }

    }

    time = s->currentTime;
    m_sendCarDataTime = s->currentTime;


    int iNumCars = local.size();

    PackedBuffer msg;

    try
    {
        msg.pack_ubyte(CARSTATUS_PACKET);
        msg.pack_double(time);
        msg.pack_int(iNumCars);
        for (int i=0;i<iNumCars;i++)
        {
            GfLogTrace("Sending car info: %s,startRank=%i\n",
                    local[i]->info.name, local[i]->info.startRank);
            msg.pack_float(local[i]->race.topSpeed);
            msg.pack_int(local[i]->pub.state);
            msg.pack_int(local[i]->info.startRank);
            msg.pack_int(local[i]->priv.dammage);
            msg.pack_float(local[i]->priv.fuel);
        }
    }
//    catch (PackedBufferException &e)
    catch (PackedBufferException)
    {
        GfLogFatal("SendCarStatusPacket: packed buffer error\n");
    }
    GfLogTrace("SendCarStatusPacket: packed data length=%d\n",
            msg.length());

    ENetPacket * pPacket = enet_packet_create (msg.buffer(), 
            msg.length(), 
            ENET_PACKET_FLAG_RELIABLE);

    BroadcastPacket(pPacket,RELIABLECHANNEL);

}

void NetNetwork::SendCarControlsPacket(tSituation *s)
{
    if (s->currentTime<0.0)
        return;

    //Clock error fix it
    if (s->currentTime<m_sendCtrlTime)
    {
        m_sendCtrlTime=s->currentTime-CAR_CONTROL_UPDATE;
    }

    SendCarStatusPacket(s,false);

    //Send carinfo packet when enough time has passed(CAR_CONTROL_UPDATE)
    if ((m_sendCtrlTime+CAR_CONTROL_UPDATE)>s->currentTime)
    {
        return;
    }

    std::vector<tCarElt *> local;
    double time = 0.0;

    //Pack controls values to reduce data size of packet
    for (int i = 0; i < s->raceInfo.ncars; i++) 
    {
        tCarElt *pCar = s->cars[i];
        //Only transmit local drivers to other clients
        if (m_setLocalDrivers.find(pCar->info.startRank)!=m_setLocalDrivers.end())
        {
            local.push_back(pCar);
        }

    }
    time = s->currentTime;

    m_sendCtrlTime = s->currentTime;

    int iNumCars = local.size();

    PackedBuffer msg;

    try
    {
        msg.pack_ubyte(CARCONTROLS_PACKET);
        msg.pack_double(time);
        msg.pack_int(iNumCars);
        for (int i = 0; i < iNumCars; i++)
        {
            msg.pack_int(local[i]->ctrl.gear);
            msg.pack_float(local[i]->ctrl.brakeCmd);
            msg.pack_float(local[i]->ctrl.steer);
            msg.pack_float(local[i]->ctrl.accelCmd);
            msg.pack_float(local[i]->ctrl.clutchCmd);
            msg.pack_int(local[i]->info.startRank);
            msg.pack_float(local[i]->pub.DynGCg.pos.x);
            msg.pack_float(local[i]->pub.DynGCg.pos.y);
            msg.pack_float(local[i]->pub.DynGCg.pos.z);
            msg.pack_float(local[i]->pub.DynGCg.pos.xy);
            msg.pack_float(local[i]->pub.DynGCg.pos.ax);
            msg.pack_float(local[i]->pub.DynGCg.pos.ay);
            msg.pack_float(local[i]->pub.DynGCg.pos.az);
            msg.pack_float(local[i]->pub.DynGCg.vel.x);
            msg.pack_float(local[i]->pub.DynGCg.vel.y);
            msg.pack_float(local[i]->pub.DynGCg.vel.z);
            msg.pack_float(local[i]->pub.DynGCg.vel.xy);
            msg.pack_float(local[i]->pub.DynGCg.vel.ax);
            msg.pack_float(local[i]->pub.DynGCg.vel.ay);
            msg.pack_float(local[i]->pub.DynGCg.vel.az);
            msg.pack_float(local[i]->pub.DynGCg.acc.x);
            msg.pack_float(local[i]->pub.DynGCg.acc.y);
            msg.pack_float(local[i]->pub.DynGCg.acc.z);
            msg.pack_float(local[i]->pub.DynGCg.acc.xy);
            msg.pack_float(local[i]->pub.DynGCg.acc.ax);
            msg.pack_float(local[i]->pub.DynGCg.acc.ay);
            msg.pack_float(local[i]->pub.DynGCg.acc.az);
        }
    }
//    catch (PackedBufferException &e)
    catch (PackedBufferException)
    {
        GfLogFatal("SendCarControlsPacket: packed buffer error\n");
    }
    GfLogTrace("SendCarControlsPacket: packed data length=%d\n",
            msg.length());

    ENetPacket * pPacket = enet_packet_create (msg.buffer(), 
            msg.length(), 
            ENET_PACKET_FLAG_UNSEQUENCED);

    BroadcastPacket(pPacket,UNRELIABLECHANNEL);
}

void NetNetwork::ReadLapStatusPacket(ENetPacket *pPacket)
{
    PackedBuffer msg(pPacket->data, pPacket->dataLength);
    GfLogTrace("ReadLapStatusPacket: packed data length=%d\n",
            msg.length());

    LapStatus lstatus;
	lstatus.startRank = 0; // Avoid compiler warnings

    try
    {
        msg.unpack_ubyte();
        lstatus.bestLapTime = msg.unpack_double();
        lstatus.bestSplitTime = msg.unpack_double();
        lstatus.laps = msg.unpack_int();
        lstatus.startRank = msg.unpack_int();
    }
//    catch (PackedBufferException &e)
    catch (PackedBufferException)
    {
        GfLogFatal("ReadLapStatusPacket: packed buffer error\n");
    }

    NetMutexData *pNData = LockNetworkData();
    bool bFound = false;
    for (unsigned int i=0;i<pNData->m_vecLapStatus.size();i++)
    {
        if (pNData->m_vecLapStatus[i].startRank == lstatus.startRank)
        {
            bFound = true;
            pNData->m_vecLapStatus[i] = lstatus;
        }
    }

    if (!bFound)
        pNData->m_vecLapStatus.push_back(lstatus);

    UnlockNetworkData();
}


void NetNetwork::ReadCarStatusPacket(ENetPacket *pPacket)
{
    PackedBuffer msg(pPacket->data, pPacket->dataLength);
    GfLogTrace("ReadCarStatusPacket: packed data length=%d\n",
            msg.length());

    double packettime;
    int iNumCars;

    try
    {
        msg.unpack_ubyte();
        packettime = msg.unpack_double();
        iNumCars = msg.unpack_int();

        NetMutexData *pNData = LockNetworkData();

        //Car conrols values (steering,brake,gas,and etc
        for (int i=0;i<iNumCars;i++)
        {
            CarStatus status;

            status.topSpeed = msg.unpack_float();
            status.state = msg.unpack_int();
            status.startRank = msg.unpack_int();
            status.dammage = msg.unpack_int();
            status.fuel = msg.unpack_float();

            status.time = packettime;

            bool bFound = false;
            for (unsigned int i=0;i<pNData->m_vecCarStatus.size();i++)
            {
                if (pNData->m_vecCarStatus[i].startRank == status.startRank)
                {
                    bFound = true;
                    //Only use the data if the time is newer.  Prevent out of order packet
                    if (pNData->m_vecCarStatus[i].time < status.time)
                    {
                        pNData->m_vecCarStatus[i] = status;
                    }
                    else
                    {
                        GfLogTrace("Rejected car status from startRank %i\n",status.startRank);
                    }
                    GfLogTrace("Received car status from startRank %i\n",status.startRank);
                    break;
                }
            }

            if (!bFound)
                pNData->m_vecCarStatus.push_back(status);
        }

        UnlockNetworkData();
    }
//    catch (PackedBufferException &e)
    catch (PackedBufferException)
    {
        GfLogFatal("ReadCarStatusPacket: packed buffer error\n");
    }
}

void NetNetwork::GetHostSettings(std::string &strCarCat,bool &bCollisions)
{
    assert(m_strRaceXMLFile!="");

    void *params = GfParmReadFileLocal(m_strRaceXMLFile.c_str(),GFPARM_RMODE_STD);
    assert(params);

    strCarCat = GfParmGetStr(params, RM_SECT_HEADER,RM_ATTR_CAR_CATEGORY,"All");

    //TODO
    bCollisions = true;
}

void NetNetwork::ReadCarControlsPacket(ENetPacket *pPacket)
{
    PackedBuffer msg(pPacket->data, pPacket->dataLength);
    GfLogTrace("ReadCarControlsPacket: packed data length=%d\n",
            msg.length());

    double packettime;
    int iNumCars;

    try
    {
        msg.unpack_ubyte();
        packettime = msg.unpack_double();
        iNumCars = msg.unpack_int();

        NetMutexData *pNData = LockNetworkData();

        //Car conrols values (steering,brake,gas,and etc
        for (int i=0;i<iNumCars;i++)
        {
            CarControlsData ctrl;	

            ctrl.gear = msg.unpack_int();
            ctrl.brake = msg.unpack_float();
            ctrl.steering = msg.unpack_float();
            ctrl.throttle = msg.unpack_float();
            ctrl.clutch = msg.unpack_float();
            ctrl.startRank = msg.unpack_int();
            ctrl.DynGCg.pos.x = msg.unpack_float();
            ctrl.DynGCg.pos.y = msg.unpack_float();
            ctrl.DynGCg.pos.z = msg.unpack_float();
            ctrl.DynGCg.pos.xy = msg.unpack_float();
            ctrl.DynGCg.pos.ax = msg.unpack_float();
            ctrl.DynGCg.pos.ay = msg.unpack_float();
            ctrl.DynGCg.pos.az = msg.unpack_float();
            ctrl.DynGCg.vel.x = msg.unpack_float();
            ctrl.DynGCg.vel.y = msg.unpack_float();
            ctrl.DynGCg.vel.z = msg.unpack_float();
            ctrl.DynGCg.vel.xy = msg.unpack_float();
            ctrl.DynGCg.vel.ax = msg.unpack_float();
            ctrl.DynGCg.vel.ay = msg.unpack_float();
            ctrl.DynGCg.vel.az = msg.unpack_float();
            ctrl.DynGCg.acc.x = msg.unpack_float();
            ctrl.DynGCg.acc.y = msg.unpack_float();
            ctrl.DynGCg.acc.z = msg.unpack_float();
            ctrl.DynGCg.acc.xy = msg.unpack_float();
            ctrl.DynGCg.acc.ax = msg.unpack_float();
            ctrl.DynGCg.acc.ay = msg.unpack_float();
            ctrl.DynGCg.acc.az = msg.unpack_float();

            ctrl.time = packettime;

            bool bFound = false;
            for (unsigned int i=0;i<pNData->m_vecCarCtrls.size();i++)
            {
                if (pNData->m_vecCarCtrls[i].startRank == ctrl.startRank)
                {
                    bFound = true;
                    //Only use the data if the time is newer.  Prevent out of order packet
                    if (pNData->m_vecCarCtrls[i].time < ctrl.time)
                    {
                        pNData->m_vecCarCtrls[i] = ctrl;
                    }
                    else
                    {
                        GfLogTrace("Rejected car control from startRank %i\n",ctrl.startRank);
                    }
                }
            }

            if (!bFound)
                pNData->m_vecCarCtrls.push_back(ctrl);
        }

        UnlockNetworkData();
    }
//    catch (PackedBufferException &e)
    catch (PackedBufferException)
    {
        GfLogFatal("ReadCarControlsPacket: packed buffer error\n");
    }
}

//==========================================================

Uint32 network_callbackfunc(Uint32 interval, void *param)
{
    if (NetGetNetwork())
        NetGetNetwork()->listen();

    return interval;
}

bool NetworkInit()
{
    g_bInit = true;

    return true;
}

bool RemoveNetworkTimer()
{
    return SDL_RemoveTimer(g_timerId) == SDL_TRUE ? true : false;
}

bool AddNetworkTimer()
{
    //Create a timer callback to listen to the network
    g_timerId = SDL_AddTimer(40, network_callbackfunc,0);

    return true;
}

NetServer *NetGetServer()
{
    if (!g_bServer)
        return NULL;

    return &g_server;
}

NetClient *NetGetClient()
{
    if (!g_bClient)
        return NULL;

    return &g_client;
}

void NetSetServer(bool bStatus)
{
    if (bStatus == g_bServer)
        return;

    g_bServer = bStatus;
    if (g_bServer)
        AddNetworkTimer();
    else
        RemoveNetworkTimer();
}

void NetSetClient(bool bStatus)
{
    if (bStatus == g_bClient)
        return;

    g_bClient = bStatus;
    if (g_bClient)
        AddNetworkTimer();
    else
        RemoveNetworkTimer();
}

bool NetIsServer()
{
    return g_bServer;
}
bool NetIsClient()
{
    return g_bClient;
}


void NetworkListen()
{
    if (NetGetNetwork())
        NetGetNetwork()->listen();
}

bool AddressMatch(ENetAddress &a1,ENetAddress &a2)
{
    if ((a1.host == a2.host)&&(a1.port == a2.port))
        return true;

    return false;
};

// Never used : remove ?
// static int
// networkInit(int /* idx */, void *pt)
// {
//     return 0;
// }


