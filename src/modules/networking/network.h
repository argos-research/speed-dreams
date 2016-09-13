/***************************************************************************

file                 : network.h
created              : July 2009
copyright            : (C) 2009 Brian Gavin
web                  : speed-dreams.sourceforge.net
version              : $Id: network.h 6170 2015-10-16 23:19:40Z torcs-ng $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef NETWORK_H
#define NETWORK_H

#ifdef _MSC_VER
#pragma warning (disable: 4251)
#endif

// DLL exported symbols declarator for Windows.
#ifdef WIN32
# ifdef networking_EXPORTS
#  define NETWORKING_API __declspec(dllexport)
# else
#  define NETWORKING_API __declspec(dllimport)
# endif
#else
# define NETWORKING_API
#endif

#include <string>
#include <vector>
#include <set>
#include <map>

#if defined(__APPLE__) && !defined(USE_MACPORTS)
#include <enet.h>
#else
#include <enet/enet.h>
#endif

#define MAXNETWORKPLAYERS 16
//Port used for network play
#define SPEEDDREAMSPORT 28500
#define SPEEDDREAMSPEERPORT 28501

#define UNRELIABLECHANNEL 0
#define RELIABLECHANNEL 1

#define NETWORKROBOT "networkhuman"
#define HUMANROBOT "human"
//Network update rates
#define CAR_CONTROL_UPDATE 0.1
#define CAR_DATA_UPDATE 5.0
#define RACESTARTDELEAY 3.0
#define FINISHDELAY 10.0

//Packet definitions
#define CHAT_PACKET 1
#define PLAYERINFO_PACKET 2
#define RACEINFOCHANGE_PACKET 3
#define PREPARETORACE_PACKET 4
#define CLIENTREADYTOSTART_PACKET 5
#define RACESTARTTIME_PACKET 6
#define CARCONTROLS_PACKET 7
#define FILE_PACKET 8
#define SERVER_TIME_SYNC_PACKET 9
#define SERVER_TIME_REQUEST_PACKET 10
#define WEATHERCHANGE_PACKET 11
#define CARSTATUS_PACKET 12
#define LAPSTATUS_PACKET 13
#define FINISHTIME_PACKET 14
#define DRIVERREADY_PACKET 15
#define ALLDRIVERREADY_PACKET 16
#define PLAYERREJECTED_PACKET 17
#define PLAYERACCEPTED_PACKET 18

#include <track.h>
#include <raceman.h>

enum EnumClientAccepted { PROCESSINGCLIENT,CLIENTREJECTED,CLIENTACCEPTED };

//Use a structure to pass as a network ENetPacket sent 
//Packed / Compressed to reduce internet bandwidth requirements
struct CarControlsPacked
{
    unsigned char	startRank;
    tDynPt	DynGCg;		/* GC global data */

    short steering;//Fixed point between -2,2
    short throttle;//Fixed point between -2,2
    short brake;//Fixed point between -2,2
    short clutch;//Fixed point between -2,2
    unsigned char gear;
};

//Uncompressed car controls pack
struct CarControlsData
{
    int startRank;
    tDynPt DynGCg;		/* GC global data */
    float steering;
    float throttle;
    float brake;
    float clutch;
    int gear;
    double time;
};


struct LapStatus
{
    double bestLapTime;
    double bestSplitTime;
    int laps;
    int startRank;
};

struct CarStatus
{
    float topSpeed;
    int state;
    double time;
    float fuel;
    int dammage;
    int startRank;
};

struct CarStatusPacked
{
    float topSpeed;
    short state;
    float fuel;
    int dammage;
    unsigned char startRank;
};


//Holds driver values 
class NETWORKING_API NetDriver
{
public:
    NetDriver();
    ~NetDriver() {}
    ENetAddress address;
    unsigned short hostPort;

    int idx;
    char name[64];
	char sname[64];
    char cname[4];
    char car[64];
    char team[64];
    char author[64];
    int racenumber;
    char skilllevel[64];
    float red,green,blue;
    char module[64];
    char type[64];
    bool client;
    bool active;
};

//Holds car setup values
struct CarSetup
{
    //TODO
};

struct SDL_mutex;

//Put data here that is read by the network thread and the main thread
class NETWORKING_API NetMutexData
{
public:
    NetMutexData();
    virtual ~NetMutexData();

    void Lock();
    void Unlock();
    void Init();

    SDL_mutex *m_networkMutex;
    std::vector<CarControlsData> m_vecCarCtrls;
    std::vector<CarStatus> m_vecCarStatus;
    std::vector<LapStatus> m_vecLapStatus;
    std::vector<bool> m_vecReadyStatus;
    double m_finishTime;
};

//Put data here that is read by the network thread and the main thread
class NETWORKING_API NetServerMutexData 
{
public:
    void Init();
    NetServerMutexData();
    virtual ~NetServerMutexData();

    void Lock();
    void Unlock();

    SDL_mutex *m_networkMutex;
    std::vector<NetDriver> m_vecNetworkPlayers;
};

class NETWORKING_API NetNetwork
{
public:
    NetNetwork();

    virtual ~NetNetwork();

    void SetCurrentTime(double time) {m_currentTime = time;}
    bool IsServerMode(); 
    bool IsClientMode(); 
    bool SetCurrentDriver();
    int GetNetworkHumanIdx();
    int	 GetDriverIdx();
    int GetCarIndex(int startRank,tSituation *s);
    virtual void ReadLapStatusPacket(ENetPacket *pPacket);
    virtual void SendCarControlsPacket(tSituation *s);
    virtual void SendCarStatusPacket(tSituation *s,bool bForce);
    virtual void SendLapStatusPacket(tCarElt *pCar);
    virtual void SetDriverReady(bool bReady) {};
    virtual bool IsConnected() { return false;}
    virtual bool IsRaceActive() { return m_bRaceActive;}
    virtual void SetRaceActive(bool bStatus) {m_bRaceActive = bStatus;}

    virtual void RaceInit(tSituation *s);
    virtual void RaceDone();
    int GetDriverStartRank(int idx);

    virtual bool listen(){ return false;};

    virtual void Disconnect() {};
    virtual void ResetNetwork() {};

    void ReadCarControlsPacket(ENetPacket *pPacket);
    void ReadCarStatusPacket(ENetPacket *pPacket);
    void PackCarControl(tCarCtrl *pCtrl,int &size,char *&pBuffer);
    void UnPackCarControl(tCarCtrl *&pCtrl,int size,char *pBuffer);
    bool PrepareToRace(){return m_bPrepareToRace;}
    void SetRaceInfoChanged(bool bStatus);
    void SetRefreshDisplay(bool bStatus);
    bool GetRefreshDisplay() {return m_bRefreshDisplay;}
    bool GetRaceInfoChanged(){return m_bRaceInfoChanged;};
    double GetRaceStartTime(){return m_racestarttime;}
    std::string GetNetworkDriverName();
    void SetRaceXMLFile(char const *pXMLFile);
    void ReadDriverData(NetDriver &player,int index,void *param);
    void WriteDriverData(NetDriver player,int index,void *param);
    int GetPlayerCarIndex(tSituation *s);

    void ClearLocalDrivers();

    void SetDriverName(char *pName);
    const char *GetDriverName();
    virtual void SetLocalDrivers();
    void GetHostSettings(std::string &strCarCat,bool &bCollisions);
    virtual void SetCarInfo(const char *pszName);

    virtual bool FinishRace(double time) ;

    NetMutexData * LockNetworkData() ;
    void UnlockNetworkData();

protected:
    std::string m_strDriverName;


    ENetHost * m_pHost;
    virtual void BroadcastPacket(ENetPacket *pPacket,enet_uint8 channel);

    int m_driverIdx;
    bool m_bBeginRace;
    bool m_bRaceInfoChanged;
    bool m_bRefreshDisplay;
    double m_racestarttime;	
    bool m_bPrepareToRace;
    bool m_bTimeSynced;
    bool m_bRaceActive;

    //time when packet was sent or recieved
    double m_activeNetworkTime;
    ENetAddress m_address;

    FILE *m_pFile;

    double m_sendCtrlTime;
    double m_sendCarDataTime;
    double m_currentTime;

    NetMutexData m_NetworkData;


    std::map<int,int>	m_mapRanks;

    std::set<int> m_setLocalDrivers;

    double m_timePhysics;

    std::string m_strClass;
    std::string m_strRaceXMLFile;

};

class NETWORKING_API NetClient: public NetNetwork
{
public:
    NetClient();
    ~NetClient();

    virtual void Disconnect();
    virtual void ResetNetwork();
    virtual bool IsConnected();

    bool ConnectToServer(const char *pAddress,int port, NetDriver *pDriver);
    virtual bool listen();

    //Packets
    bool SendDriverInfoPacket(NetDriver *pDriver);
    virtual void SendDriverReadyPacket(){};
    void SendReadyToStartPacket();
    double WaitForRaceStart();
    void SendServerTimeRequest();
    virtual void SetDriverReady(bool bReady);

    bool TimeSynced(){return m_bTimeSynced;}
    int  LookUpDriverIdx() { return m_driverIdx;}
    bool listenHost(ENetHost * pHost);
    virtual void SetLocalDrivers();

    void ConnectToClients();
    void SetCarInfo(const char *pszName);

protected:
    //Packets
    void ReadRaceSetupPacket(ENetPacket *pPacket);
    void ReadPrepareToRacePacket(ENetPacket *pPacket);
    void ReadStartTimePacket(ENetPacket *pPacket);
    void ReadFilePacket(ENetPacket *pPacket);
    void ReadPacket(ENetEvent event);
    void ReadTimePacket(ENetPacket *pPacket);
    void ReadFinishTimePacket(ENetPacket *pPacket);
    void ReadAllDriverReadyPacket(ENetPacket *pPacket);
    void ReadWeatherPacket(ENetPacket *pPacket);
    void ReadPlayerRejectedPacket(ENetPacket *pPacket);
    void ReadPlayerAcceptedPacket(ENetPacket *pPacket);

    void ConnectToDriver(NetDriver driver);

    virtual void BroadcastPacket(ENetPacket *pPacket,enet_uint8 channel);

    bool m_bConnected;

    double m_lag;
    double m_servertimedifference;
    double m_packetsendtime;
    EnumClientAccepted m_eClientAccepted;


    ENetHost * m_pClient;

    ENetPeer *m_pServer;	

};

class NETWORKING_API NetServer : public NetNetwork
{
public:
    NetServer();
    ~NetServer();

    virtual void Disconnect();
    virtual void ResetNetwork();
    virtual bool IsConnected();

    bool Start(int port);
    virtual bool listen();

    //Network Packets
    void SendRaceSetupPacket();
    void SendPrepareToRacePacket();
    void SendFilePacket(const char *pszFile);
    void WaitForClientsStartPacket();
    void SendStartTimePacket(int &startTime);
    void SendTimePacket(ENetPacket *pPacket, ENetPeer * pPeer);
    void SendFinishTimePacket();
    void SendWeatherPacket();
    bool SendPlayerRejectedPacket(ENetPeer * pPeer,std::string strReason);
    bool SendPlayerAcceptedPacket(ENetPeer * pPeer);
    void SendDriversReadyPacket();
    void PingClients();
    virtual void SetDriverReady(bool bReady);
    void OverrideDriverReady(int idx,bool bReady);

    bool ClientsReadyToRace();
    double WaitForRaceStart();
    void UpdateClientCarInfo(tSituation *s);

    void UpdateDriver(NetDriver & player);

    int  NumberofPlayers();
    NetDriver GetPlayer(int i);
    void ClearDrivers();
    void RemoveDriver(ENetEvent event);
    void CreateNetworkRobotFile();
    //virtual void SendCarControlsPacket(tSituation *s);
    virtual void SetLocalDrivers();
    void SetHostSettings(const char *pszCarCat,bool bCollisions);

    void SetCarInfo(const char *pszName);
    void SetFinishTime(double time);
    void RemovePlayerFromRace(unsigned int idx);

    NetServerMutexData * LockServerData();
    void UnlockServerData();

    void GenerateDriversForXML();
protected:
    //Packets
    void ReadDriverInfoPacket(ENetPacket *ENetPacket, ENetPeer * pPeer);
    void ReadDriverReadyPacket(ENetPacket *pPacket);
    void ReadPacket(ENetEvent event);


    NetServerMutexData m_ServerData;
    virtual void BroadcastPacket(ENetPacket *pPacket,enet_uint8 channel);
    void Dump(const char* pszCaller);


    std::vector<NetDriver> m_vecWaitForPlayers;

    ENetHost * m_pServer;
};


bool AddNetworkTimer();
bool RemoveNetworkTimer();

extern NETWORKING_API void NetSetServer(bool bStatus);
extern NETWORKING_API void NetSetClient(bool bStatus);
extern NETWORKING_API bool NetIsServer();
extern NETWORKING_API bool NetIsClient();

extern NETWORKING_API NetServer *NetGetServer();
extern NETWORKING_API NetClient *NetGetClient();
extern NETWORKING_API NetNetwork *NetGetNetwork();

void NetworkListen();

bool AddressMatch(ENetAddress &a1,ENetAddress &a2);

#endif // NETWORK_H
