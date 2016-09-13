/***************************************************************************

file                 : network.cpp
created              : July 2009
copyright            : (C) 2009 Brian Gavin
web                  : speed-dreams.sourceforge.net
version              : $Id: client.cpp 5841 2014-11-16 21:05:57Z wdbee $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <portability.h>

#include <SDL.h>

#include "network.h"
#include "robotxml.h"
#include "pack.h"

NetClient::NetClient()
{
    if (enet_initialize () != 0)
        GfLogError ("An error occurred while initializing ENet.\n");

    m_strClass = "client";
    m_pServer = NULL;
    m_pClient = NULL;
    m_pHost = NULL;
    m_eClientAccepted = PROCESSINGCLIENT;
}


NetClient::~NetClient()
{
    ResetNetwork();
    NetSetClient(false);
}

void NetClient::Disconnect()
{
    m_bConnected = false;

    ResetNetwork();
    NetSetClient(false);
}

void NetClient::ResetNetwork()
{
    if (m_pClient == NULL)
        return;

    if (m_pServer == NULL)
        return;

    ENetEvent event;

    enet_peer_disconnect (m_pServer, 0);

    bool bDisconnect = false;

    /* Allow up to 3 seconds for the disconnect to succeed
       and drop any packets received packets.
     */
    while (enet_host_service (m_pClient, & event, 3000) > 0)
    {
        switch (event.type)
        {
        case ENET_EVENT_TYPE_RECEIVE:
            enet_packet_destroy (event.packet);
            break;

        case ENET_EVENT_TYPE_DISCONNECT:
            GfLogTrace ("Network disconnection succeeded.");
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
        enet_peer_reset (m_pServer);

    NetSetClient(false);

    ENetPeer * pCurrentPeer1;

    if (m_pHost ==NULL)
        return;

    for (pCurrentPeer1 = m_pHost-> peers;
            pCurrentPeer1 < & m_pHost->peers [m_pHost->peerCount];
            ++ pCurrentPeer1)
    {
        if (pCurrentPeer1->state != ENET_PEER_STATE_CONNECTED)
            continue;

        enet_peer_reset (pCurrentPeer1);
    }

    enet_host_destroy(m_pHost);
    m_pHost = NULL;

}

bool NetClient::ConnectToServer(const char *pAddress,int port, NetDriver *pDriver)
{
    m_bTimeSynced = false;
    m_bPrepareToRace = false;
    m_bBeginRace = false;
    m_timePhysics = -2.0;
    m_servertimedifference = 0.0;
    m_sendCarDataTime = 0.0;
    m_sendCtrlTime = 0.0;
    m_bPrepareToRace = false;
    m_bBeginRace = false;
    m_bConnected = false;
    m_pClient = NULL;
    m_pHost = NULL;

#if (ENET_VERSION >= 0x010300)
    m_pClient = enet_host_create (NULL /* create a client host */,
            MAXNETWORKPLAYERS, 
            2, /*channel limit*/
            0/* downstream bandwidth */,
            0/* upstream bandwidth */);
#else
    m_pClient = enet_host_create (NULL /* create a client host */,
            MAXNETWORKPLAYERS, 
            0/* downstream bandwidth */,
            0/* upstream bandwidth */);
#endif

    if (m_pClient == NULL)
    {
        GfLogError ("An error occurred while trying to create an ENet client host.\n");
        ResetNetwork();
        return false;
    }

    ENetAddress caddress;
    caddress.host = ENET_HOST_ANY;

    /* Bind the server to port*/
    caddress.port = SPEEDDREAMSPEERPORT;

#if (ENET_VERSION >= 0x010300)
    m_pHost = enet_host_create (&caddress /* create a peer host */,
            MAXNETWORKPLAYERS, 
            2, /*channel limit*/
            0/* downstream bandwidth */,
            0/* upstream bandwidth */);
#else
    m_pHost = enet_host_create (&caddress /* create a peer host */,
            MAXNETWORKPLAYERS, 
            0/* downstream bandwidth */,
            0/* upstream bandwidth */);
#endif
    if(m_pHost==NULL)
    {
        //try the other ports
        for (int i=1;i<5;i++)
        {
            caddress.port++;
#if (ENET_VERSION >= 0x010300)
            m_pHost = enet_host_create (&caddress,MAXNETWORKPLAYERS,2,0,0);
#else
            m_pHost = enet_host_create (&caddress,MAXNETWORKPLAYERS,0,0);
#endif
            if(m_pHost)
                break;

        }

        if (m_pHost == NULL)
        {
            GfLogError("Unable to setup client listener\n");
            return false;
        }
    }

    ENetAddress address;
    ENetEvent event;

    enet_address_set_host (& address, pAddress);
    address.port = (enet_uint16)port;

    /* Initiate the connection, allocating the two channels 0 and 1. */
    GfLogError ("Initiating network connection to host '%s:%d' ...\n", pAddress, port);
#if (ENET_VERSION >= 0x010300)
    m_pServer = enet_host_connect (m_pClient, & address, 2, 0);
#else
    m_pServer = enet_host_connect (m_pClient, & address, 2);
#endif

    if (m_pServer == NULL)
    {
        GfLogInfo ("No available peers for initiating an ENet connection.\n");
        ResetNetwork();
        return false;
    }

    /* Wait up to 5 seconds for the connection attempt to succeed. */
    if (enet_host_service (m_pClient, & event, 5000) > 0 &&
            event.type == ENET_EVENT_TYPE_CONNECT)
    {
        m_address.host = m_pClient->address.host;
        m_address.port = m_pClient->address.port;
        m_bConnected = true;
        GfLogInfo ("Network connection accepted.\n");
    }
    else
    {
        m_bConnected = false;
        ResetNetwork();
        GfLogError ("Network connection refused.\n");
        return false;
    }

    m_eClientAccepted = PROCESSINGCLIENT;
    SendDriverInfoPacket(pDriver);

    //Wait for server to accept or reject 
    GfLogInfo ("Sent local driver info to the network server : waiting ...\n");
    while(m_eClientAccepted == PROCESSINGCLIENT)
        SDL_Delay(50);

    if (m_eClientAccepted == CLIENTREJECTED)
    {
        m_bConnected = false;
        ResetNetwork();
        return false;
    }
    else
        GfLogInfo ("Driver info accepted by the network server.\n");

    return m_bConnected;
}

bool NetClient::IsConnected()
{
    return m_bConnected;
}

void NetClient::SetDriverReady(bool bReady)
{
    // Get local driver index in the race driver list
    int idx = GetDriverIdx();

    NetMutexData *pNData = LockNetworkData();
    pNData->m_vecReadyStatus[idx-1] = bReady;
    UnlockNetworkData();

    PackedBuffer msg;

    try
    {
        msg.pack_ubyte(DRIVERREADY_PACKET);
        msg.pack_int(idx);
        msg.pack_int(bReady);
    }
//    catch (PackedBufferException &e)
    catch (PackedBufferException)
    {
        GfLogFatal("SetDriverReady: packed buffer error\n");
    }
    GfLogTrace("SetDriverReady: packed data length=%d\n", msg.length());

    ENetPacket *pPacket = enet_packet_create (msg.buffer(), 
            msg.length(), 
            ENET_PACKET_FLAG_RELIABLE);

    if (enet_peer_send (m_pServer, RELIABLECHANNEL, pPacket)==0)
        return;
}

bool NetClient::SendDriverInfoPacket(NetDriver *pDriver)
{
    SetDriverName(pDriver->name);
    pDriver->address.port = m_pHost->address.port;

    GfLogTrace("SendDriverInfoPacket: pDriver\n");
    GfLogTrace("->host=%d\n", pDriver->address.host);
    GfLogTrace("->port=%d\n", pDriver->address.port);
    GfLogTrace("->idx=%d\n", pDriver->idx);
    GfLogTrace("->name=%s\n", pDriver->name);
    GfLogTrace("->sname=%s\n", pDriver->sname);
    GfLogTrace("->cname=%s\n", pDriver->cname);
    GfLogTrace("->car=%s\n", pDriver->car);
    GfLogTrace("->team=%s\n", pDriver->team);
    GfLogTrace("->author=%s\n", pDriver->author);
    GfLogTrace("->racenumber=%d\n", pDriver->racenumber);
    GfLogTrace("->skilllevel=%s\n", pDriver->skilllevel);
    GfLogTrace("->red=%.1f\n", pDriver->red);
    GfLogTrace("->green=%.1f\n", pDriver->green);
    GfLogTrace("->blue=%.1f\n", pDriver->blue);
    GfLogTrace("->module=%s\n", pDriver->module);
    GfLogTrace("->type=%s\n", pDriver->type);
    GfLogTrace("->client=%d\n", pDriver->client);

    PackedBuffer msg;

    try
    {
        msg.pack_ubyte(PLAYERINFO_PACKET);
        msg.pack_int(pDriver->idx);
        msg.pack_string(pDriver->name, sizeof pDriver->name);
        msg.pack_string(pDriver->sname, sizeof pDriver->sname);
        msg.pack_string(pDriver->cname, sizeof pDriver->cname);
        msg.pack_string(pDriver->car, sizeof pDriver->car);
        msg.pack_string(pDriver->team, sizeof pDriver->team);
        msg.pack_string(pDriver->author, sizeof pDriver->author);
        msg.pack_int(pDriver->racenumber);
        msg.pack_string(pDriver->skilllevel,
                sizeof pDriver->skilllevel);
        msg.pack_float(pDriver->red);
        msg.pack_float(pDriver->green);
        msg.pack_float(pDriver->blue);
        msg.pack_string(pDriver->module, sizeof pDriver->module);
        msg.pack_string(pDriver->type, sizeof pDriver->type);
        msg.pack_int(pDriver->client);
    }
//    catch (PackedBufferException &e)
    catch (PackedBufferException)
    {
        GfLogFatal("SendDriverInfoPacket: packed buffer error\n");
    }
    GfLogTrace("SendDriverInfoPacket: packed data length=%d\n",
            msg.length());

    ENetPacket * pPacket = enet_packet_create (msg.buffer(), 
            msg.length(), 
            ENET_PACKET_FLAG_RELIABLE);

    if (enet_peer_send (m_pServer, RELIABLECHANNEL, pPacket)==0)
        return true;

    return false;
}

void NetClient::SendReadyToStartPacket()
{

    std::string strDName = GetDriverName();
    GfLogTrace("Sending ready to start packet\n");

    PackedBuffer msg;

    try
    {
        msg.pack_ubyte(CLIENTREADYTOSTART_PACKET);
        msg.pack_stdstring(strDName);
    }
//    catch (PackedBufferException &e)
    catch (PackedBufferException)
    {
        GfLogFatal("SendReadyToStartPacket: packed buffer error\n");
    }
    GfLogTrace("SendReadyToStartPacket: packed data length=%d\n",
            msg.length());

    ENetPacket *pPacket = enet_packet_create (msg.buffer(), 
            msg.length(), 
            ENET_PACKET_FLAG_RELIABLE);

    if (enet_peer_send (m_pServer, RELIABLECHANNEL, pPacket))
        GfLogError("SendReadyToStartPacket : enet_peer_send failed\n");
}


void NetClient::SendServerTimeRequest()
{
    m_packetsendtime = GfTimeClock(); 

    PackedBuffer msg;

    try
    {
        msg.pack_ubyte(SERVER_TIME_REQUEST_PACKET);
    }
//    catch (PackedBufferException &e)
    catch (PackedBufferException)
    {
        GfLogFatal("SendServerTimeRequest: packed buffer error\n");
    }
    GfLogTrace("SendServerTimeRequest: packed data length=%d\n",
            msg.length());


    ENetPacket *pPacket = enet_packet_create (msg.buffer(), 
            msg.length(), 
            ENET_PACKET_FLAG_UNSEQUENCED);

    if (enet_peer_send (m_pServer, UNRELIABLECHANNEL, pPacket))
        GfLogError("SendServerTimeRequest : enet_peer_send failed\n");
}

double NetClient::WaitForRaceStart()
{
    while(!m_bBeginRace)
    {
        SDL_Delay(20);
    }

    return GfTimeClock()-m_racestarttime;
}


void NetClient::ReadStartTimePacket(ENetPacket *pPacket)
{
    GfLogTrace("Received the start race Packet\n");
    //double time = GfTimeClock();

    PackedBuffer msg(pPacket->data, pPacket->dataLength);
    GfLogTrace("ReadStartTimePacket: packed data length=%d\n",
            msg.length());

    try
    {
        msg.unpack_ubyte();
        m_racestarttime = msg.unpack_double();
    }
//    catch (PackedBufferException &e)
    catch (PackedBufferException)
    {
        GfLogFatal("ReadStartTimePacket: packed buffer error\n");
    }

    //Adjust start time based on client clock
    m_racestarttime= m_racestarttime+m_servertimedifference;
    m_bBeginRace = true;

}

void NetClient::ReadPlayerRejectedPacket(ENetPacket *pPacket)
{
    m_eClientAccepted = CLIENTREJECTED;
    GfLogWarning ("Server rejected connection.\n");
}

void NetClient::ReadPlayerAcceptedPacket(ENetPacket *pPacket)
{
    m_eClientAccepted = CLIENTACCEPTED;
    GfLogTrace ("Server accepted connection.\n");
}

bool NetClient::listenHost(ENetHost * pHost)
{
    if (pHost == NULL)
        return false;

    bool bHasPacket = false;

    ENetEvent event;

    while (enet_host_service(pHost, & event, 0) > 0)
    {
        switch (event.type)
        {
        case ENET_EVENT_TYPE_CONNECT:
            char hostName[256];
            enet_address_get_host_ip (&event.peer->address,hostName,256);

            GfLogTrace ("A new client connected from %s\n",hostName); 

            /* Store any relevant client information here. */
            event.peer -> data = (void*)"Client information";

            break;

        case ENET_EVENT_TYPE_RECEIVE:
            //printf ("A packet of length %u containing %s was received from %s on channel %u.\n",
            //        event.packet -> dataLength,
            //        event.packet -> data,
            //        event.peer -> data,
            //        event.channelID);
            ReadPacket(event);        
            bHasPacket = true;
            break;

        case ENET_EVENT_TYPE_DISCONNECT:
            if(event.peer == m_pServer)
            {
                m_bConnected = false;
                /* Reset the peer's client information. */
                GfLogTrace("Server disconnected\n");
            }

            event.peer -> data = NULL;
            break;

        case ENET_EVENT_TYPE_NONE:
            // Do nothing.
            break;
        }
    }

    return bHasPacket;

}

bool NetClient::listen()
{
    if (!m_bConnected)
        return false;

    listenHost(m_pClient);
    listenHost(m_pHost);

    return true;
}


void NetClient::ReadPacket(ENetEvent event)
{
    ENetPacket *pPacket = event.packet;
    assert(pPacket->dataLength>=1);
    unsigned char packetId = pPacket->data[0];
    //unsigned char *pData = &pPacket->data[1];
    //int datasize = pPacket->dataLength-1;

    switch (packetId)
    {
    case RACEINFOCHANGE_PACKET:
        ReadRaceSetupPacket(event.packet);
        break;
    case PREPARETORACE_PACKET:
        ReadPrepareToRacePacket(event.packet);
        break;
    case RACESTARTTIME_PACKET:
        ReadStartTimePacket(event.packet);
        break;
    case CARCONTROLS_PACKET:
        ReadCarControlsPacket(event.packet);
        break;
    case FILE_PACKET:
        ReadFilePacket(event.packet);
        break;
    case SERVER_TIME_SYNC_PACKET:
        ReadTimePacket(event.packet);
        break;
    case WEATHERCHANGE_PACKET:
        ReadWeatherPacket(event.packet);
        break;
    case CARSTATUS_PACKET:
        ReadCarStatusPacket(event.packet);
        break;
    case LAPSTATUS_PACKET:
        ReadLapStatusPacket(event.packet);
        break;
    case FINISHTIME_PACKET:
        ReadFinishTimePacket(event.packet);
        break;
    case ALLDRIVERREADY_PACKET:
        ReadAllDriverReadyPacket(event.packet);
        break;
    case PLAYERREJECTED_PACKET:
        ReadPlayerRejectedPacket(event.packet);
        break;
    case PLAYERACCEPTED_PACKET:
        ReadPlayerAcceptedPacket(event.packet);
        break;
    default:
        assert(false);
        GfLogDebug ("A packet of length %u containing %s was received from %s on channel %u.\n",
                event.packet -> dataLength,
                event.packet -> data,
                (char*)event.peer -> data,
                event.channelID);

    }

    enet_packet_destroy (event.packet);
}

void NetClient::ReadPrepareToRacePacket(ENetPacket *pPacket)
{
    GfLogTrace("Received the start race Packet\n");

    //unsigned char packetId = pPacket->data[0];



    m_bPrepareToRace = true;


}
void NetClient::ReadRaceSetupPacket(ENetPacket *pPacket)
{
    GfLogTrace("\nRecieving race setup\n");

    SetRaceInfoChanged(true);
}

void NetClient::ConnectToDriver(NetDriver driver)
{
    char hostName[256];
    enet_address_get_host_ip (&driver.address,hostName,256);

    if (!driver.client)
    {
        GfLogTrace("Skipping server: %s Address: %s\n",driver.name,hostName);
        return;
    }

    if (strcmp(driver.name,GetDriverName())==0)
    {
        GfLogTrace("Skipping ourself: %s Address:  %s\n",driver.name,hostName);
        return;
    }

    ENetPeer * pCurrentPeer;

    for (pCurrentPeer = m_pClient-> peers;
            pCurrentPeer < & m_pClient->peers [m_pClient->peerCount];
            ++ pCurrentPeer)
    {
        if (pCurrentPeer->state == ENET_PEER_STATE_CONNECTED)
        {
            if ((pCurrentPeer->address.host == driver.address.host)&&
                    (pCurrentPeer->address.port == driver.address.port))
            {
                GfLogTrace("Already connected to driver: %s Address: %s\n",driver.name,hostName);
                return;
            }
        }

    }

    GfLogTrace("connecting to driver: %s Address: %s\n",driver.name,hostName);

    //Connect to peer player
    //ENetPeer *pPeer = enet_host_connect (m_pClient, &driver.address, 2);



    ENetEvent event;

    if (enet_host_service (m_pClient, & event, 5000) > 0 &&
            event.type == ENET_EVENT_TYPE_CONNECT)
    {
        GfLogTrace("Successfully connected to peer\n");
        return;
    }
    else
    {
        //char hostName[256];
        //enet_address_get_host_ip (&event.peer->address,hostName,256);
        GfLogWarning("Failed to connect to peer! (%X)\n", &event.peer->address);
        return;
    }

}

void NetClient::ReadWeatherPacket(ENetPacket *pPacket)
{
    //TODO Xavier read in weather data
}
void NetClient::ReadAllDriverReadyPacket(ENetPacket *pPacket)
{
    int rsize;

    PackedBuffer msg(pPacket->data, pPacket->dataLength);
    GfLogTrace("ReadAllDriverReadyPacket: packed data length=%d\n",
            msg.length());

    try
    {
        msg.unpack_ubyte();
        rsize = msg.unpack_int();

        NetMutexData *pNData = LockNetworkData();
        pNData->m_vecReadyStatus.clear();
        pNData->m_vecReadyStatus.resize(rsize);
        for (int i=0;i<rsize;i++)
            pNData->m_vecReadyStatus[i] = msg.unpack_int() ? true : false;

        UnlockNetworkData();
        SetRaceInfoChanged(true);
    }
//    catch (PackedBufferException &e)
    catch (PackedBufferException)
    {
        GfLogFatal("ReadAllDriverReadyPacket: packed buffer error\n");
    }

    GfLogTrace("Received All Driver Ready Packet\n");
}

void NetClient::ReadFinishTimePacket(ENetPacket *pPacket)
{
    PackedBuffer msg(pPacket->data, pPacket->dataLength);
    GfLogTrace("ReadFinishTimePacket: packed data length=%d\n",
            msg.length());

    try
    {
        msg.unpack_ubyte();

        NetMutexData *pNData = LockNetworkData();
        pNData->m_finishTime = msg.unpack_double();
        UnlockNetworkData();
    }
//    catch (PackedBufferException &e)
    catch (PackedBufferException)
    {
        GfLogFatal("ReadFinishTimePacket: packed buffer error\n");
    }

    GfOut("Received finish time packet\n");
}

void NetClient::ReadTimePacket(ENetPacket *pPacket)
{
    double curTime = GfTimeClock();
    m_lag = (curTime-m_packetsendtime)/2.0;
    GfLogTrace ("Connection lag is %lf seconds\n",m_lag);

    double time = 0;

    PackedBuffer msg(pPacket->data, pPacket->dataLength);
    GfLogTrace("ReadTimePacket: packed data length=%d\n",
            msg.length());

    try
    {
        msg.unpack_ubyte();
        time = msg.unpack_double();
    }
//    catch (PackedBufferException &e)
    catch (PackedBufferException)
    {
        GfLogFatal("ReadTimePacket: packed buffer error\n");
    }

    m_servertimedifference = curTime-time;
    m_bTimeSynced = true;
}
void NetClient::ReadFilePacket(ENetPacket *pPacket)
{
    short len;
    size_t writeSize;
    char file[255];
    unsigned int filesize = 0;
    char *filedata = 0;

    memset(file, 0, sizeof file);

    PackedBuffer msg(pPacket->data, pPacket->dataLength);
    GfLogTrace("ReadFilePacket: packed data length=%d\n",
            msg.length());

    try
    {
        msg.unpack_ubyte();
        len = msg.unpack_short();
        msg.unpack_string(file, len);
        filesize = msg.unpack_int();

        GfLogTrace("Client file size %u\n",filesize);
        filedata = new char[filesize];

        msg.unpack_string(filedata, filesize);
    }
//    catch (PackedBufferException &e)
    catch (PackedBufferException)
    {
        GfLogFatal("ReadFilePacket: packed buffer error\n");
    }

    char filepath[255];
    snprintf(filepath, sizeof filepath, "%s%s", GfLocalDir(), file);

    FILE *pFile = fopen(filepath,"w+b");
    GfLogTrace("Reading file packet: File- %s\n",filepath);

    if (filedata && filesize > 0)
    {
        writeSize = fwrite(filedata, filesize, 1, pFile);
        if( writeSize <= 0 )
            GfLogTrace("Not all bytes are send to file");
    }
    fclose(pFile);

    delete [] filedata;
}

void NetClient::BroadcastPacket(ENetPacket *pPacket,enet_uint8 channel)
{
    ENetPacket * pHostPacket = enet_packet_create (pPacket->data, 
            pPacket->dataLength, 
            pPacket->flags);

    //Send to connected clients
    enet_host_broadcast (m_pHost, channel, pPacket);

    //Send to server
    enet_peer_send (m_pServer, channel, pHostPacket);

    m_activeNetworkTime = GfTimeClock();
}

void NetClient::SetCarInfo(const char *pszName)
{
    std::vector<NetDriver> vecDrivers;

    RobotXml robotxml;
    robotxml.ReadRobotDrivers(NETWORKROBOT,vecDrivers);

    for (unsigned int i=0;i<vecDrivers.size();i++)
    {
        if (vecDrivers[i].name == m_strDriverName)
        {
            strncpy(vecDrivers[i].car,pszName,64);
            SendDriverInfoPacket(&vecDrivers[i]);
        }
    }
}

void NetClient::ConnectToClients()
{
    std::vector<NetDriver> vecDrivers;

    RobotXml robotxml;
    robotxml.ReadRobotDrivers(NETWORKROBOT,vecDrivers);

    for(unsigned int i=0;i<vecDrivers.size();i++)
    {
        ConnectToDriver(vecDrivers[i]);
    }

}

void NetClient::SetLocalDrivers()
{
    m_setLocalDrivers.clear();
    m_driverIdx = GetDriverIdx();
    m_setLocalDrivers.insert(m_driverIdx-1);
    GfLogTrace("Adding Human start rank: %i\n",m_driverIdx-1);
}

