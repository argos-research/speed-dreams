/***************************************************************************

file                 : network.cpp
created              : July 2009
copyright            : (C) 2009 Brian Gavin
web                  : speed-dreams.sourceforge.net
version              : $Id: server.cpp 5841 2014-11-16 21:05:57Z wdbee $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ********************************* ******************************************/

#include <cstdio>
#include <SDL.h>

#include "network.h"
#include "robotxml.h" 
#include "pack.h"


NetServer::NetServer()
{
    if (enet_initialize () != 0)
    {
        GfLogError ("An error occurred while initializing ENet.\n");
        assert(false);
    }

    m_strClass = "server";
}

NetServer::~NetServer()
{
    ResetNetwork();
    NetSetServer(false);
}

void NetServer::Disconnect()
{
    ResetNetwork();
    NetSetServer(false);
}

void NetServer::ResetNetwork()
{
    if (m_pServer)
    {
        ENetPeer * pCurrentPeer;

        for (pCurrentPeer = m_pServer-> peers;
                pCurrentPeer < & m_pServer->peers [m_pServer->peerCount];
                ++ pCurrentPeer)
        {
            if (pCurrentPeer->state != ENET_PEER_STATE_CONNECTED)
                continue;

            enet_peer_disconnect (pCurrentPeer, 0);
        }

        ENetEvent event;
        bool bDisconnect = false;

        /* Allow up to 3 seconds for the disconnect to succeed
           and drop any received packets.
         */
        while (enet_host_service (m_pServer, & event, 3000) > 0)
        {
            switch (event.type)
            {
            case ENET_EVENT_TYPE_RECEIVE:
                enet_packet_destroy (event.packet);
                break;

            case ENET_EVENT_TYPE_DISCONNECT:
                GfLogTrace ("Disconnection succeeded.");
                bDisconnect=true;
                break;

            case ENET_EVENT_TYPE_NONE:
            case ENET_EVENT_TYPE_CONNECT:
                // Do nothing.
                break;
            }
        }

        /* We've arrived here, so the disconnect attempt didn't */
        /* succeed yet.  Force the connection down.             */
        if (!bDisconnect)
        {
            ENetPeer * pCurrentPeer1;

            for (pCurrentPeer1 = m_pServer-> peers;
                    pCurrentPeer1 < & m_pServer->peers [m_pServer->peerCount];
                    ++ pCurrentPeer1)
            {
                if (pCurrentPeer1->state != ENET_PEER_STATE_CONNECTED)
                    continue;

                enet_peer_reset (pCurrentPeer1);
            }
        }

        enet_host_destroy(m_pServer);
        m_pServer = NULL;
    }
}

bool NetServer::IsConnected()
{
    if (m_pServer)
        return true;

    return false;
}

bool NetServer::Start(int port)
{
    SetRaceInfoChanged(true);
    m_bPrepareToRace = false;
    m_bBeginRace = false;

    m_timePhysics = -2.0;
    m_sendCarDataTime = 0.0;
    m_sendCtrlTime = 0.0;


    /* Bind the server to the default localhost.     */
    /* A specific host address can be specified by   */
    /* enet_address_set_host (& address, "x.x.x.x"); */

    m_address.host = ENET_HOST_ANY;
    /* Bind the server to port*/
    m_address.port = (enet_uint16)port;

    assert(m_pServer ==NULL);

    GfLogInfo ("Starting network server : Listening on port %d.\n", port);

#if (ENET_VERSION >= 0x010300)
    m_pServer = enet_host_create (& m_address /* the address to bind the server host to */, 
            MAXNETWORKPLAYERS,
            2,     /* assume tha maximum number of channels is allowed*/
            0      /* assume any amount of incoming bandwidth */,
            0      /* assume any amount of outgoing bandwidth */);
#else
    m_pServer = enet_host_create (& m_address /* the address to bind the server host to */, 
            MAXNETWORKPLAYERS,
            0      /* assume any amount of incoming bandwidth */,
            0      /* assume any amount of outgoing bandwidth */);
#endif
    if (m_pServer == NULL)
    {
        GfLogError ("An error occurred while trying to create an ENet server host.\n");
        return false;
    }

    m_pHost = m_pServer;
    return true;
}


bool NetServer::ClientsReadyToRace()
{
    return m_bBeginRace;
}

void NetServer::WaitForClientsStartPacket()
{
    while (!m_bBeginRace)
    {
        SDL_Delay(20);
    }
}

void NetServer::SendStartTimePacket(int &startTime)
{
    //Wait RACESTARTDELEAY seconds to start race
    m_racestarttime = GfTimeClock()+RACESTARTDELEAY;

    PackedBuffer msg;

    try
    {
        msg.pack_ubyte(RACESTARTTIME_PACKET);
        msg.pack_double(m_racestarttime);
    }
//    catch (PackedBufferException &e)
    catch (PackedBufferException)
    {
        GfLogFatal("SendStartTimePacket: packed buffer error\n");
    }
    GfLogTrace("SendStartTimePacket: packed data length=%d\n",
            msg.length());

    ENetPacket * pPacket = enet_packet_create (msg.buffer(), 
            msg.length(), 
            ENET_PACKET_FLAG_RELIABLE);

    BroadcastPacket(pPacket,RELIABLECHANNEL);

    GfLogInfo("Server Start time is %lf\n",m_racestarttime);
}
double NetServer::WaitForRaceStart()
{
    int startTime;
    SendStartTimePacket(startTime);
    GfLogInfo("Server waiting to start the race\n");


    double time = GfTimeClock()-m_racestarttime;

    return time;
}

void NetServer::ClearDrivers()
{
    LockServerData()->m_vecNetworkPlayers.clear();
    LockServerData()->Unlock();
    GenerateDriversForXML();
    Dump("NetServer::ClearDrivers");
}


void NetServer::SetHostSettings(const char *pszCarCat,bool bCollisions)
{
    assert(m_strRaceXMLFile!="");

    void *params = GfParmReadFileLocal(m_strRaceXMLFile.c_str(),GFPARM_RMODE_STD);
    assert(params);
    const char *pName =GfParmGetStr(params, RM_SECT_HEADER, RM_ATTR_NAME, "");


    GfParmSetStr(params, RM_SECT_HEADER,RM_ATTR_CAR_CATEGORY, pszCarCat);
    GfParmWriteFileLocal(m_strRaceXMLFile.c_str(), params, pName);
}


void NetServer::GenerateDriversForXML()
{
    assert(m_strRaceXMLFile!="");

    void *params = GfParmReadFileLocal(m_strRaceXMLFile.c_str(),GFPARM_RMODE_STD);
    assert(params);

    const char *pName =GfParmGetStr(params, RM_SECT_HEADER, RM_ATTR_NAME, "");

    // We only want to add human drivers which don't already exist
    NetServerMutexData *pSData = LockServerData();
    for (int i=0;i<(int)pSData->m_vecNetworkPlayers.size();i++)
    {
        char path2[256];
        int index = 1;
        bool found = false;

        // Skip the ones which aren't active
	if (pSData->m_vecNetworkPlayers[i].active == false) continue;

        while (!found) {
            sprintf(path2, "%s/%d", RM_SECT_DRIVERS, index++);
            if (GfParmExistsSection(params, path2) == 0) {
                // driver not found, so add them at the end
                GfLogInfo("Adding driver %s to XML\n", pSData->m_vecNetworkPlayers[i].name);
                GfParmSetNum(params, path2, RM_ATTR_IDX, (char*)NULL,(tdble) pSData->m_vecNetworkPlayers[i].idx);
                GfParmSetStr(params, path2, RM_ATTR_MODULE, pSData->m_vecNetworkPlayers[i].module);

                // should also write skin information
                break;
            }

            if ((tdble)pSData->m_vecNetworkPlayers[i].idx == GfParmGetNum(params, path2, RM_ATTR_IDX, NULL,1.0) &&
                    strcmp(pSData->m_vecNetworkPlayers[i].module, GfParmGetStr(params, path2, RM_ATTR_MODULE,NULL)) == 0) {
                GfLogInfo("Found driver %s in XML\n", pSData->m_vecNetworkPlayers[i].name);
                found = true;
            }
        }
    }

    UnlockServerData();

    //Save our changes
    GfParmWriteFileLocal(m_strRaceXMLFile.c_str(), params, pName);
}

void NetServer::SetLocalDrivers()
{
    m_setLocalDrivers.clear();

    // add all local drivers
    NetServerMutexData *pSData = LockServerData();
    for (int i=0;i<(int)pSData->m_vecNetworkPlayers.size();i++)
    {
        if(pSData->m_vecNetworkPlayers[i].client == false) {
            m_setLocalDrivers.insert(i);
            GfLogTrace("Adding Human start rank: %i\n",i);
        }
    }
    UnlockServerData();

    assert(m_strRaceXMLFile!="");

    void *params = GfParmReadFileLocal(m_strRaceXMLFile.c_str(),GFPARM_RMODE_STD);
    assert(params);

    //const char *pName =GfParmGetStr(params, RM_SECT_HEADER, RM_ATTR_NAME, "");

    int nCars = GfParmGetEltNb(params, RM_SECT_DRIVERS);	
    //Gather vector of all non human drivers
    std::vector<NetDriver> vecRDrivers;
    for (int i=1;i<=nCars;i++)
    {
        NetDriver driver;
        ReadDriverData(driver,i,params);
        if ((strcmp(driver.module,NETWORKROBOT)!=0)
                &&(strcmp(driver.module,HUMANROBOT)!=0))
        {
            m_setLocalDrivers.insert(i-1);
            GfLogTrace("Adding driver start rank:%i\n",i);
        }
    }
}

void NetServer::OverrideDriverReady(int idx,bool bReady)
{
    NetMutexData *pNData = LockNetworkData();
    pNData->m_vecReadyStatus[idx-1] = bReady;
    UnlockNetworkData();

    Dump("NetServer::OverrideDriverReady");

    SetRaceInfoChanged(true);
}
void NetServer::SetDriverReady(bool bReady)
{
    int idx = GetDriverIdx();

    NetMutexData *pNData = LockNetworkData();
    pNData->m_vecReadyStatus[idx-1] = bReady;
    UnlockNetworkData();

    Dump("NetServer::SetDriverReady");

    SendDriversReadyPacket();
}


void NetServer::UpdateDriver(NetDriver & driver)
{
    assert(m_strRaceXMLFile!="");
    bool bNewDriver = true;

    NetServerMutexData *pSData = LockServerData();

    // Search for the driver in m_vecNetworkPlayers, and update its car name if found.
    for(unsigned int i=0;i<pSData->m_vecNetworkPlayers.size();i++)
    {
        if (strcmp(driver.name,pSData->m_vecNetworkPlayers[i].name)==0)
        {
            bNewDriver = false;
            strncpy(pSData->m_vecNetworkPlayers[i].car,driver.car,64);
            break;
        }
    }

    // If not found, append it to m_vecNetworkPlayers
    if (bNewDriver)
    {
        driver.idx = pSData->m_vecNetworkPlayers.size()+1;

        if (!driver.client)
        {
            driver.address = m_pServer->address;
        }

        pSData->m_vecNetworkPlayers.push_back(driver);

        NetMutexData *pNData = LockNetworkData();
        pNData->m_vecReadyStatus.push_back(false);
        UnlockNetworkData();
    }

    GenerateDriversForXML();

    RobotXml rXml;
    rXml.CreateRobotFile("networkhuman",pSData->m_vecNetworkPlayers);

    UnlockServerData();

    Dump("NetServer::UpdateDriver");

    SetRaceInfoChanged(true);
}

void NetServer::SetCarInfo(const char *pszName)
{
    std::vector<NetDriver> vecDrivers;

    RobotXml robotxml;
    robotxml.ReadRobotDrivers(NETWORKROBOT,vecDrivers);

    for (unsigned int i=0;i<vecDrivers.size();i++)
    {
        if (vecDrivers[i].name == m_strDriverName)
        {
            strncpy(vecDrivers[i].car,pszName,64);
            UpdateDriver(vecDrivers[i]);
            break;
        }
    }
}

void NetServer::CreateNetworkRobotFile()
{
    RobotXml rXml;
    NetServerMutexData *pSData = LockServerData();
    rXml.CreateRobotFile("networkhuman",pSData->m_vecNetworkPlayers);
    UnlockServerData();
}

void NetServer::RemoveDriver(ENetEvent event)
{
    int playerStartIndex;
    ENetAddress address = event.peer->address;

    char hostName[256];
    enet_address_get_host_ip (&address,hostName,256);

    GfLogTrace ("Client Player Info disconnect from %s\n",hostName); 

    std::vector<NetDriver>::iterator p;

    if (m_vecWaitForPlayers.size()>0)
    {
        p = m_vecWaitForPlayers.begin();

        while(p!=m_vecWaitForPlayers.end())
        {

            if ((p->address.host == address.host)&&(p->hostPort == address.port))
            {
                m_vecWaitForPlayers.erase(p);
                break;
            }

            p++;
        }

        if (m_vecWaitForPlayers.size()==0)
            m_bBeginRace = true;
    }

    //look for driver id
    NetServerMutexData *pSData = LockServerData();
    for (p = pSData->m_vecNetworkPlayers.begin();p!=pSData->m_vecNetworkPlayers.end();p++)
    {
        if (p->client)
        {
            if ((p->address.host == address.host)&&(p->hostPort == address.port))
            {
                if(m_bRaceActive)
                {
                    playerStartIndex = p->idx-1;
                    pSData->m_vecNetworkPlayers.erase(p);
                    RemovePlayerFromRace(playerStartIndex);
                    GenerateDriversForXML();
                    RobotXml rXml;
                    rXml.CreateRobotFile("networkhuman",pSData->m_vecNetworkPlayers);
                    SetRaceInfoChanged(true);
                }
                else
                {
                    pSData->m_vecNetworkPlayers.erase(p);
                    GenerateDriversForXML();
                    RobotXml rXml;
                    rXml.CreateRobotFile("networkhuman",pSData->m_vecNetworkPlayers);
                    SetRaceInfoChanged(true);
                }

                UnlockServerData();
                return;
            }
        }
    }

    UnlockServerData();
}

bool NetServer::SendPlayerAcceptedPacket(ENetPeer * pPeer)
{

    //Send to client requesting connection
    PackedBuffer msg;

    try
    {
        msg.pack_ubyte(PLAYERACCEPTED_PACKET);	
    }
//    catch (PackedBufferException &e)
    catch (PackedBufferException)
    {
        GfLogFatal("SendPlayerAcceptedPacket: packed buffer error\n");
    }
    GfLogTrace("SendPlayerAcceptedPacket: packed data length=%d\n",
            msg.length());

    ENetPacket * pPacket = enet_packet_create (msg.buffer(), 
            msg.length(), 
            ENET_PACKET_FLAG_RELIABLE);

    if (enet_peer_send (pPeer, RELIABLECHANNEL, pPacket)==0)
        return true;

    return false;
}

bool NetServer::SendPlayerRejectedPacket(ENetPeer * pPeer,std::string strReason)
{
    //Send to client requesting connection

    PackedBuffer msg;

    try
    {
        msg.pack_ubyte(PLAYERREJECTED_PACKET);	
        msg.pack_stdstring(strReason);
    }
//    catch (PackedBufferException &e)
    catch (PackedBufferException)
    {
        GfLogFatal("SendPlayerRejectedPacket: packed buffer error\n");
    }
    GfLogTrace("SendPlayerRejectedPacket: packed data length=%d\n",
            msg.length());

    ENetPacket * pPacket = enet_packet_create (msg.buffer(), 
            msg.length(), 
            ENET_PACKET_FLAG_RELIABLE);

    if (enet_peer_send (pPeer, RELIABLECHANNEL, pPacket)==0)
        return true;

    return false;
}

void NetServer::SendDriversReadyPacket()
{

    NetMutexData *pNData = LockNetworkData();

    int rsize = pNData->m_vecReadyStatus.size();

    PackedBuffer msg;

    try
    {
        msg.pack_ubyte(ALLDRIVERREADY_PACKET);
        msg.pack_int(rsize);
        for (int i = 0; i < rsize; i++)
        {
            msg.pack_int(pNData->m_vecReadyStatus[i]);
        }
    }
//    catch (PackedBufferException &e)
    catch (PackedBufferException)
    {
        GfLogFatal("SendDriversReadyPacket: packed buffer error\n");
    }
    GfLogTrace("SendDriversReadyPacket: packed data length=%d\n",
            msg.length());

    UnlockNetworkData();

    ENetPacket * pPacket = enet_packet_create (msg.buffer(), 
            msg.length(), 
            ENET_PACKET_FLAG_RELIABLE);

    BroadcastPacket(pPacket,RELIABLECHANNEL);
    m_bRefreshDisplay = true;
}

void NetServer::SendRaceSetupPacket()
{
    PackedBuffer msg;

    try
    {
        msg.pack_ubyte(RACEINFOCHANGE_PACKET);
    }
//    catch (PackedBufferException &e)
    catch (PackedBufferException)
    {
        GfLogFatal("SendRaceSetupPacket: packed buffer error\n");
    }
    GfLogTrace("SendRaceSetupPacket: packed data length=%d\n",
            msg.length());

    ENetPacket * pPacket = enet_packet_create (msg.buffer(), 
            msg.length(), 
            ENET_PACKET_FLAG_RELIABLE);

    BroadcastPacket(pPacket,RELIABLECHANNEL);

    SetRaceInfoChanged(true);
}


void NetServer::ReadDriverReadyPacket(ENetPacket *pPacket)
{
    GfLogTrace ("Read Driver Ready Packet\n"); 

    int idx = 0;
    bool bReady;

    PackedBuffer msg(pPacket->data, pPacket->dataLength);
    GfLogTrace("ReadDriverReadyPacket: packed data length=%d\n",
            msg.length());

    try
    {
        msg.unpack_ubyte();
        idx = msg.unpack_int();
        bReady = msg.unpack_int() ? true : false;
    }
//    catch (PackedBufferException &e)
    catch (PackedBufferException)
    {
        GfLogFatal("SendRaceSetupPacket: packed buffer error\n");
        bReady = false;
    }

    NetMutexData *pNData = LockNetworkData();
    if (idx > 0)
        pNData->m_vecReadyStatus[idx-1] = bReady;
    UnlockNetworkData();

    SendDriversReadyPacket();
}

void NetServer::ReadDriverInfoPacket(ENetPacket *pPacket, ENetPeer * pPeer)
{
    NetDriver driver;

    char hostName[256];
    enet_address_get_host_ip (&driver.address,hostName,256);

    GfLogTrace ("Client Player Info connected from %s\n",hostName); 

    PackedBuffer msg(pPacket->data, pPacket->dataLength);
    GfLogTrace("ReadDriverInfoPacket: packed data length=%d\n",
            msg.length());

    try
    {
        msg.unpack_ubyte();
        driver.idx = msg.unpack_int();
        msg.unpack_string(driver.name, sizeof driver.name);
        msg.unpack_string(driver.sname, sizeof driver.sname);
        msg.unpack_string(driver.cname, sizeof driver.cname);
        msg.unpack_string(driver.car, sizeof driver.car);
        msg.unpack_string(driver.team, sizeof driver.team);
        msg.unpack_string(driver.author, sizeof driver.author);
        driver.racenumber = msg.unpack_int();
        msg.unpack_string(driver.skilllevel, sizeof driver.skilllevel);
        driver.red = msg.unpack_float();
        driver.green = msg.unpack_float();
        driver.blue = msg.unpack_float();
        msg.unpack_string(driver.module, sizeof driver.module);
        msg.unpack_string(driver.type, sizeof driver.type);
        driver.client = msg.unpack_int() ? true : false;
        driver.active = true;
    }
//    catch (PackedBufferException &e)
    catch (PackedBufferException)
    {
        GfLogFatal("ReadDriverInfoPacket: packed buffer error\n");
    }

    GfLogTrace("ReadDriverInfoPacket: driver\n");
    GfLogTrace(".host=%X\n", pPeer->address.host);
    GfLogTrace(".port=%d\n", pPeer->address.port);
    GfLogTrace(".idx=%d\n", driver.idx);
    GfLogTrace(".name=%s\n", driver.name);
    GfLogTrace(".car=%s\n", driver.car);
    GfLogTrace(".team=%s\n", driver.team);
    GfLogTrace(".author=%s\n", driver.author);
    GfLogTrace(".racenumber=%d\n", driver.racenumber);
    GfLogTrace(".skilllevel=%s\n", driver.skilllevel);
    GfLogTrace(".red=%.1f\n", driver.red);
    GfLogTrace(".green=%.1f\n", driver.green);
    GfLogTrace(".blue=%.1f\n", driver.blue);
    GfLogTrace(".module=%s\n", driver.module);
    GfLogTrace(".type=%s\n", driver.type);
    GfLogTrace(".client=%d\n", driver.client);

    //Make sure player name is unique otherwise disconnect player
    NetServerMutexData *pSData = LockServerData();
    for(unsigned int i=0;i<pSData->m_vecNetworkPlayers.size();i++)
    {
        if (strcmp(driver.name,pSData->m_vecNetworkPlayers[i].name)==0)
        {
            // check to see if existing client is just updating details
            if (pPeer->address.host == pSData->m_vecNetworkPlayers[i].address.host) {
                GfLogInfo("Client driver updated details\n");
                break;
            }

            SendPlayerRejectedPacket(pPeer,"Player name already used. Please choose a different name.");
            UnlockServerData();
            return;
        }
    }
    UnlockServerData();

    driver.address.host = pPeer->address.host;
    driver.hostPort = pPeer->address.port;

    SendPlayerAcceptedPacket(pPeer);
    UpdateDriver(driver);

    GfLogTrace("Reading Driver Info Packet:  Driver: %s,Car: %s\n",driver.name,driver.car);
}


//Used to verify that all clients are still connected
void NetServer::PingClients()
{
    ENetPeer * pCurrentPeer;

    for (pCurrentPeer = m_pServer-> peers;
            pCurrentPeer < & m_pServer->peers [m_pServer->peerCount];
            ++ pCurrentPeer)
    {
        if (pCurrentPeer->state != ENET_PEER_STATE_CONNECTED)
            continue;

        enet_peer_ping (pCurrentPeer);
    }
}


//Here you are Xavier a dynamic weather packet
void NetServer::SendWeatherPacket()
{
    GfLogTrace("Sending Weather Packet\n");

    PackedBuffer msg;

    try
    {
        msg.pack_ubyte(WEATHERCHANGE_PACKET);
        //TODO add weather data here
    }
//    catch (PackedBufferException &e)
    catch (PackedBufferException)
    {
        GfLogFatal("SendWeatherPacket: packed buffer error\n");
    }
    GfLogTrace("SendWeatherPacket: packed data length=%d\n",
            msg.length());

    ENetPacket *pWeatherPacket = enet_packet_create(msg.buffer(),
            msg.length(),
            ENET_PACKET_FLAG_RELIABLE);


    BroadcastPacket(pWeatherPacket,RELIABLECHANNEL);
}

void NetServer::SendTimePacket(ENetPacket *pPacketRec, ENetPeer * pPeer)
{
    GfLogTrace("Sending Time Packet\n");

    double time = GfTimeClock();
    GfLogTrace("\nServer time is %lf",time);

    PackedBuffer msg;

    try
    {
        msg.pack_ubyte(SERVER_TIME_SYNC_PACKET);
        msg.pack_double(time);
    }
//    catch (PackedBufferException &e)
    catch (PackedBufferException)
    {
        GfLogFatal("SendTimePacket: packed buffer error\n");
    }
    GfLogTrace("SendTimePacket: packed data length=%d\n",
            msg.length());

    //TODO change to peer send
    ENetPacket * pPacket = enet_packet_create (msg.buffer(), 
            msg.length(), 
            ENET_PACKET_FLAG_UNSEQUENCED);

    enet_peer_send (pPeer, UNRELIABLECHANNEL, pPacket);
}

//Send a file to clients
//Do not use this to send large files
//In future maybe change to TCP based
//64k file size limit
void NetServer::SendFilePacket(const char *pszFile)
{
    char filepath[255];
    sprintf(filepath, "%s%s", GfLocalDir(), pszFile);

    GfLogTrace("Sending file packet: File- %s\n",filepath);

    FILE *pFile = fopen(filepath,"rb");
    if (!pFile)
        return;

    char buf[0xffff];
    size_t size;
    size = fread( buf, 1, 0xffff, pFile );

    //File is to big
    if (!feof(pFile))
    {
        fclose(pFile);
        assert(false);
        return;
    }

    fclose(pFile);
    unsigned int filesize = size;
    GfLogTrace("Server file size %u\n",filesize);

    short namelen = (short)strlen(pszFile);

    /* On 64 bit systems, the following calculates a buffer size that is
     * bigger than necessary, but that is safe. Better too big than too
     * small.
     */
    size_t bufsize = 1 + sizeof namelen + namelen +
        sizeof filesize + filesize;

    PackedBuffer msg(bufsize);

    try
    {
        msg.pack_ubyte(FILE_PACKET);
        msg.pack_short(namelen);
        msg.pack_string(pszFile, namelen);
        msg.pack_uint(filesize);
        msg.pack_string(buf, size);
    }
//    catch (PackedBufferException &e)
    catch (PackedBufferException)
    {
        GfLogFatal("SendFilePacket: packed buffer error\n");
    }
    GfLogTrace("SendFilePacket: packed data length=%d\n",
            msg.length());

    ENetPacket * pPacket = enet_packet_create (msg.buffer(), 
            msg.length(), 
            ENET_PACKET_FLAG_RELIABLE);

    BroadcastPacket(pPacket,RELIABLECHANNEL);
}

bool NetServer::listen()
{
    if (!m_pServer)
        return false;

    bool bHasPacket = false;
    ENetEvent event;
    char hostName[256];    

    while (enet_host_service (m_pServer, & event, 0) > 0)
    {
        switch (event.type)
        {
        case ENET_EVENT_TYPE_CONNECT:

            enet_address_get_host_ip (&event.peer -> address,hostName,256);

            GfLogTrace ("A new client connected from %s\n",hostName); 

            /* Store any relevant client information here. */
            event.peer -> data = (void*)"Client information";

            break;

        case ENET_EVENT_TYPE_RECEIVE:
            ReadPacket(event); 
            bHasPacket = true;
            break;

        case ENET_EVENT_TYPE_DISCONNECT:
            GfLogTrace("\nA client lost the connection.\n");
            enet_address_get_host_ip (&event.peer -> address,hostName,256);
            GfLogTrace ("A new client disconnected from %s\n",hostName); 

            RemoveDriver(event);
            SetRaceInfoChanged(true);

            GfLogTrace ("%s disconected.\n", (char*)event.peer -> data);

            /* Reset the peer's client information. */

            event.peer -> data = NULL;
            break;

        case ENET_EVENT_TYPE_NONE:
            // Do nothing.
            break;
        }
    }

    if (bHasPacket)
        m_activeNetworkTime = GfTimeClock();

    return bHasPacket;
}

//Remove disconnected player from race track
void NetServer::RemovePlayerFromRace(unsigned int idx)
{
    GfLogTrace("Removing disconnected player\n");
    std::vector<CarStatus> vecCarStatus;
    double time = 0.0;

    int startRank = GetDriverStartRank(idx);
    CarStatus cstatus;
    cstatus.topSpeed = -1.0;
    cstatus.fuel = -1.0;
    cstatus.startRank = startRank;
    cstatus.dammage = -1;
    cstatus.state = RM_CAR_STATE_ELIMINATED;
    cstatus.time = m_currentTime;

    NetMutexData *pNData = LockNetworkData();
    pNData->m_vecCarStatus.push_back(cstatus);
    UnlockNetworkData();

    vecCarStatus.push_back(cstatus);

    time = m_currentTime;

    int iNumCars = vecCarStatus.size();

    PackedBuffer msg;

    try
    {
        msg.pack_ubyte(CARSTATUS_PACKET);
        msg.pack_double(time);
        msg.pack_int(iNumCars);
        for (int i=0;i<iNumCars;i++)
        {
            msg.pack_float(vecCarStatus[i].topSpeed);
            msg.pack_int(vecCarStatus[i].state);
            msg.pack_int(vecCarStatus[i].startRank);
            msg.pack_int(vecCarStatus[i].dammage);
            msg.pack_float(vecCarStatus[i].fuel);
        }
    }
//    catch (PackedBufferException &e)
    catch (PackedBufferException)
    {
        GfLogFatal("RemovePlayerFromRace: packed buffer error\n");
    }
    GfLogTrace("RemovePlayerFromRace: packed data length=%d\n",
            msg.length());

    ENetPacket * pPacket = enet_packet_create (msg.buffer(), 
            msg.length(), 
            ENET_PACKET_FLAG_RELIABLE);

    BroadcastPacket(pPacket,RELIABLECHANNEL);
}

void NetServer::ReadPacket(ENetEvent event)
{
    ENetPacket *pPacket = event.packet;
    assert(pPacket->dataLength>=1);
    unsigned char packetId = pPacket->data[0];
    //int datasize = pPacket->dataLength-1;

    switch (packetId)
    {
    case PLAYERINFO_PACKET:
        GfLogTrace("PlayerInfo Packet\n");
        ReadDriverInfoPacket(pPacket,event.peer);
        break;
    case CLIENTREADYTOSTART_PACKET:
        {
            int l;
            char name[256];
            memset(&name[0],0,256);

            PackedBuffer msg(pPacket->data, pPacket->dataLength);
            GfLogTrace("ReadPacket: packed data length=%d\n",
                    msg.length());

            try
            {
                msg.unpack_ubyte();
                l = msg.unpack_int();
                msg.unpack_string(name, l);
            }
//            catch (PackedBufferException &e)
            catch (PackedBufferException)
            {
                GfLogFatal("ReadPacket: packed buffer error\n");
            }

            std::vector<NetDriver>::iterator p;
            p = m_vecWaitForPlayers.begin();
            while(p!=m_vecWaitForPlayers.end())
            {
                if (strcmp(p->name,name)==0)
                {
                    GfLogTrace("%s ready to start\n",&name[0]);
                    m_vecWaitForPlayers.erase(p);
                    break;
                }

                p++;
            }

            if (m_vecWaitForPlayers.size()==0)
                m_bBeginRace = true;

        }
        break;
    case SERVER_TIME_REQUEST_PACKET:
        SendTimePacket(pPacket,event.peer);
        break;
    case CARCONTROLS_PACKET:
        ReadCarControlsPacket(event.packet);
        break;
    case CARSTATUS_PACKET:
        ReadCarStatusPacket(event.packet);
        break;
    case LAPSTATUS_PACKET:
        ReadLapStatusPacket(event.packet);
        break;
    case DRIVERREADY_PACKET:
        ReadDriverReadyPacket(event.packet);
        break;

    default:
        GfLogTrace ("A packet of length %u containing %s was received from %s on channel %u.\n",
                event.packet -> dataLength,
                event.packet -> data,
                (char*)event.peer -> data,
                event.channelID);
    }

    enet_packet_destroy (event.packet);
}

void NetServer::SendFinishTimePacket()
{
    GfLogTrace("Sending finish Time Packet\n");

    NetMutexData *pNData = LockNetworkData();
    double time = pNData->m_finishTime;
    UnlockNetworkData();

    GfLogInfo("Server finish time is %lf\n",time);

    PackedBuffer msg;

    try
    {
        msg.pack_ubyte(FINISHTIME_PACKET);
        msg.pack_double(time);
    }
//    catch (PackedBufferException &e)
    catch (PackedBufferException)
    {
        GfLogFatal("SendFinishTimePacket: packed buffer error\n");
    }
    GfLogTrace("SendFinishTimePacket: packed data length=%d\n",
            msg.length());

    ENetPacket * pPacket = enet_packet_create (msg.buffer(), 
            msg.length(), 
            ENET_PACKET_FLAG_RELIABLE);
    BroadcastPacket(pPacket,RELIABLECHANNEL);
}

void NetServer::SendPrepareToRacePacket()
{
    //Add all players to list except the server player
    NetServerMutexData *pSData = LockServerData();
    for (int i=0;i<(int)pSData->m_vecNetworkPlayers.size();i++)
    {
        if (pSData->m_vecNetworkPlayers[i].client)
        {
            m_vecWaitForPlayers.push_back(pSData->m_vecNetworkPlayers[i]);
        }
    }

    UnlockServerData();

    if (m_vecWaitForPlayers.size()==0)
        m_bBeginRace = true;

    ////TODO send needed xml files to race

    PackedBuffer msg;

    try
    {
        msg.pack_ubyte(PREPARETORACE_PACKET);
    }
//    catch (PackedBufferException &e)
    catch (PackedBufferException)
    {
        GfLogFatal("SendPrepareToRacePacket: packed buffer error\n");
    }
    GfLogTrace("SendPrepareToRacePacket: packed data length=%d\n",
            msg.length());

    ENetPacket * pPacket = enet_packet_create (msg.buffer(), 
            msg.length(), 
            ENET_PACKET_FLAG_RELIABLE);

    BroadcastPacket(pPacket,RELIABLECHANNEL);
}

void NetServer::BroadcastPacket(ENetPacket *pPacket,enet_uint8 channel)
{
    enet_host_broadcast (m_pHost, channel, pPacket);
    m_activeNetworkTime = GfTimeClock();
}

void NetServer::SetFinishTime(double time)
{
    NetMutexData *pNData = LockNetworkData();
    pNData->m_finishTime = time;
    UnlockNetworkData();
    SendFinishTimePacket();
}

int NetServer::NumberofPlayers()
{ 
    int n = LockServerData()->m_vecNetworkPlayers.size();
    UnlockServerData();
    return n;
}

NetServerMutexData * NetServer::LockServerData() 
{
    m_ServerData.Lock();
    return & m_ServerData;
}

void NetServer::UnlockServerData()
{
    m_ServerData.Unlock();
}

void NetServer::Dump(const char* pszCaller)
{
    NetMutexData *pNData = LockNetworkData();
    NetServerMutexData *pSData = LockServerData();

    GfLogDebug("%s : vecReady:%u, vecPlayers:%u\n", 
            pszCaller, pNData->m_vecReadyStatus.size(), pSData->m_vecNetworkPlayers.size());

    UnlockServerData();
    UnlockNetworkData();
}
