/***************************************************************************

    file                 : networkingmenu.cpp
    created              : July 2009
    copyright            : (C) 2009 Brian Gavin
    web                  : speed-dreams.sourceforge.net
    version              : $Id: networkingmenu.cpp 5870 2014-11-29 12:14:30Z wdbee $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


/* This file deals with the networking menus.  
   The server sees a list of players and the client sees a list of other players.
   Should also allow choosing IP address, track, etc ...
*/

#include <cstdlib>
#include <cstdio>
#include <string>

#include <portability.h>
#include <tgf.hpp>
#include <tgfclient.h>

#include <raceman.h>
#include <robot.h>
#include <race.h>
#include <racemanagers.h>
#include <tracks.h>
#include <cars.h>
#include <drivers.h>
#include <network.h>

#include <playerconfig.h>
#include <playerpref.h>
#include <hostsettingsmenu.h>
#include <carsettingsmenu.h>

#include "legacymenu.h"
#include "racescreens.h"
#include "garagemenu.h"


int g_readystatus[MAXNETWORKPLAYERS];
int g_playerNames[MAXNETWORKPLAYERS];
int g_carNames[MAXNETWORKPLAYERS];

int g_trackHd;
int g_lapsHd;
int g_catHd;
int g_OutlineId;

int g_CarSetupButtonId;
int g_HostSettingsButtonId;
int g_DisconnectButtonId;
int g_CancelButtonId;
int g_ReadyCheckboxId;
int g_RaceSetupId;

static int g_IPEditId;
static int g_NameId;

static char	buf[1024];
static void	*racemanMenuHdle = NULL;
static bool	bRobotsReady = false;
static bool	bGarage = false;

//static char *g_pDriver = NULL;
//static char *g_pCar = NULL;
static std::string g_strDriver;
static std::string g_strCar;

//static char ip[1024];

static float green[] = {0.0, 1.0, 0.0, 1.0};
static float white[] = {1.0, 1.0, 1.0, 1.0};

static std::string g_strHostIP = "127.0.0.1";

HostSettingsMenu g_HostMenu;
CarSettingsMenu g_CarMenu;

// The car selection menu.
static RmGarageMenu GarageMenu;

static int GetHumanDriver(NetDriver &driver,int index);
static void ServerPrepareStartNetworkRace(void *pVoid);
static void NetworkClientConnectMenu(void * /* dummy */);


static void SetReadyStatus(int index,bool bStatus)
{
	GfuiStaticImageSetActive(racemanMenuHdle,g_readystatus[index],bStatus ? 1 : 0);
}

static void EnableMenuHostButtons(bool bChecked)
{
	// Disable/enable menu selections
	const int isEnabled = bChecked ? GFUI_DISABLE : GFUI_ENABLE;
	GfuiEnable(racemanMenuHdle, g_CarSetupButtonId, isEnabled);
	//GfuiEnable(racemanMenuHdle, g_HostSettingsButtonId, isEnabled);		
	GfuiEnable(racemanMenuHdle, g_CancelButtonId, isEnabled);
	GfuiEnable(racemanMenuHdle, g_RaceSetupId, isEnabled);
}

static void onHostPlayerReady(tCheckBoxInfo* pInfo)
{
	tRmInfo* reInfo = LmRaceEngine().inData();
	char dname[256];

	int nCars = GfParmGetEltNb(reInfo->params, RM_SECT_DRIVERS);

	NetServerMutexData *pSData = NetGetServer()->LockServerData();
	for (int i=1; i <= nCars; i++) {
		sprintf(dname, "%s/%d", RM_SECT_DRIVERS, i);

		GfLogInfo("Setting driver %d to %d\n", i, pInfo->bChecked);
		if(strcmp(NETWORKROBOT, GfParmGetStr(reInfo->params, dname, RM_ATTR_MODULE, "")) == 0) {
			// Human drive, check if local
			int index = GfParmGetNum(reInfo->params, dname, RM_ATTR_IDX, NULL, 1.0) - 1;

			GfLogInfo("Index %d\n", index);
			if (pSData->m_vecNetworkPlayers[index].client == false)
				NetGetServer()->OverrideDriverReady(i, pInfo->bChecked);
		} else {
			// Robot driver, all are local
			NetGetServer()->OverrideDriverReady(i, pInfo->bChecked);
		}

		bRobotsReady = pInfo->bChecked;
	}
	NetGetServer()->UnlockServerData();

	EnableMenuHostButtons(pInfo->bChecked);
	GfLogInfo("menu ready\n");
}

static void EnableMenuClientButtons(bool bChecked)
{
	// Disable/enable menu selections
	const int isEnabled = bChecked ? GFUI_DISABLE : GFUI_ENABLE;
	GfuiEnable(racemanMenuHdle, g_CarSetupButtonId, isEnabled);
	GfuiEnable(racemanMenuHdle, g_DisconnectButtonId, isEnabled);
}

static void onClientPlayerReady(tCheckBoxInfo* pInfo)
{
	int nDriverIdx = NetGetNetwork()->GetDriverIdx();

	// Local Human Driver
	if (nDriverIdx > -1) {
		SetReadyStatus(nDriverIdx-1, pInfo->bChecked);
		NetGetNetwork()->SetDriverReady(pInfo->bChecked);
	}

	EnableMenuClientButtons(pInfo->bChecked);
}

static std::string 
GetTrackName(const char *category, const char *trackName)
{
    void *trackHandle;
	std::string name;

    sprintf(buf, "tracks/%s/%s/%s.%s", category, trackName, trackName, TRKEXT);
    trackHandle = GfParmReadFile(buf, GFPARM_RMODE_STD);

    if (trackHandle) {
        name = GfParmGetStr(trackHandle, TRK_SECT_HDR, TRK_ATT_NAME, trackName);
    } else {
        GfTrace("Could not read file %s\n", buf);
        return 0;
    }

    GfParmReleaseHandle(trackHandle);
    return name;
}

// Not used for the moment.
// static void
// onChangeCarCategory(void * pData)
// {
	
// }

static std::string
GetTrackPreviewFileName(const char *pszCategory, const char *pszTrack)
{
	char buf[256];

	// Try JPEG first.
    snprintf(buf, sizeof(buf), "tracks/%s/%s/%s.jpg", pszCategory, pszTrack, pszTrack);
    buf[255] = 0; /* snprinf manual is not clear about that ... */

	// Then PNG if not found.
	if (!GfFileExists(buf))
	{
		snprintf(buf, sizeof(buf), "tracks/%s/%s/%s.png", pszCategory, pszTrack, pszTrack);
		buf[255] = 0; /* snprinf manual is not clear about that ... */
	}

	// Then fallback.
	if (!GfFileExists(buf))
		strncpy(buf, "data/img/splash-networkrace.jpg", sizeof(buf));
	
	return std::string(buf);
}

static std::string
GetTrackOutlineFileName(const char *pszCategory,const char *pszTrack)
{
	char buf[256];

    snprintf(buf, sizeof(buf), "tracks/%s/%s/outline.png", pszCategory, pszTrack);
	
	if (!GfFileExists(buf))
		strncpy(buf, "data/img/transparent.png", sizeof(buf));
	
	return std::string(buf);
}

static void 
UpdateNetworkPlayers()
{
	GfDriver* newDriver;
	NetNetwork *pNetwork = NetGetNetwork();

	if (pNetwork->GetRefreshDisplay() == false)
		return;

	tRmInfo* reInfo = LmRaceEngine().inData();

	//Set current driver that camera will look at
	pNetwork->SetCurrentDriver();

	//reload xml file
	NetGetNetwork()->SetRaceXMLFile("config/raceman/networkrace.xml");
	reInfo->params = GfParmReadFileLocal("config/raceman/networkrace.xml",GFPARM_RMODE_REREAD);
	assert(reInfo->params);

	reInfo->_reName = GfParmGetStr(reInfo->params, RM_SECT_HEADER, RM_ATTR_NAME, "");
	assert(reInfo->_reName);

	// Scan each of the human drivers to see if they're active in this race
	if (NetIsServer()) {
		NetServerMutexData *pSData = NetGetServer()->LockServerData();
		assert(pSData);

		// Ensure that garage menu knows about driver
		for (unsigned int i=0; i < pSData->m_vecNetworkPlayers.size(); i++) {
			newDriver = GfDrivers::self()->getDriver(NETWORKROBOT, pSData->m_vecNetworkPlayers[i].idx);

			if (!newDriver) { 
				GfLogInfo("Driver %s not found, reloading drivers\n", pSData->m_vecNetworkPlayers[i].name);
				GfDrivers::self()->reload();
				LmRaceEngine().race()->load(LmRaceEngine().race()->getManager(), true);
				break;
			}
		}

		for (unsigned int i=0; i < pSData->m_vecNetworkPlayers.size(); i++) {
			int k = 1;
			char path2[256];

			pSData->m_vecNetworkPlayers[i].active = false;
			newDriver = GfDrivers::self()->getDriver(NETWORKROBOT, pSData->m_vecNetworkPlayers[i].idx);

			// Scan through drivers listed in 'networkrace.xml'
			while (pSData->m_vecNetworkPlayers[i].active == false) {
				sprintf(path2, "%s/%d", RM_SECT_DRIVERS, k++);
				if (GfParmExistsSection(reInfo->params, path2) == 0) {
					GfLogInfo("UpdateNetworkPlayers: Removing driver %s\n", pSData->m_vecNetworkPlayers[i].name);

					if (pSData->m_vecNetworkPlayers[i].client) {
						//need to tell/force client to disconnect
					}
					break;
				}

				if ((tdble)pSData->m_vecNetworkPlayers[i].idx == GfParmGetNum(reInfo->params, path2, RM_ATTR_IDX, NULL, 1.0) &&
						strcmp(NETWORKROBOT, GfParmGetStr(reInfo->params, path2, RM_ATTR_MODULE, "")) == 0) {
					pSData->m_vecNetworkPlayers[i].active = true;
				}
			}

			// add or remove from competitor list (for garage menu)
			GfDriver* activeDriver = LmRaceEngine().race()->getCompetitor(NETWORKROBOT, pSData->m_vecNetworkPlayers[i].idx);
			if (pSData->m_vecNetworkPlayers[i].active) {
				if (!activeDriver) LmRaceEngine().race()->appendCompetitor(newDriver);
			} else {
				if (activeDriver) LmRaceEngine().race()->removeCompetitor(newDriver);
			}
		}
		NetGetServer()->UnlockServerData();
	} else {
#if 1
		// Client XML files already written to disk - this works but is not the best solution....
		GfDrivers::self()->reload();
		LmRaceEngine().race()->load(LmRaceEngine().race()->getManager(), true);
#endif
	}

	//Update track info
	std::string strTrackPath = GfParmGetStr(reInfo->params, "Tracks/1", RM_ATTR_NAME, "");
	std::string strCategory = GfParmGetStr(reInfo->params, "Tracks/1", RM_ATTR_CATEGORY, "");

	std::string strTrackName = GetTrackName(strCategory.c_str(),strTrackPath.c_str());

	sprintf(buf, "%s", strTrackName.c_str());
	GfuiLabelSetText(racemanMenuHdle,g_trackHd,buf);

	//Store current track - client needs this
	GfTrack* PCurTrack = GfTracks::self()->getTrackWithName(buf);
	LmRaceEngine().race()->getManager()->setEventTrack(0, PCurTrack);
	
	int laps = (int)GfParmGetNum(reInfo->params, reInfo->_reName,"laps", "", 1);
	sprintf(buf, "%i", laps);
	GfuiLabelSetText(racemanMenuHdle,g_lapsHd,buf);

	GfuiScreenAddBgImg(racemanMenuHdle,
					   GetTrackPreviewFileName(strCategory.c_str(),strTrackPath.c_str()).c_str());
	GfuiStaticImageSet(racemanMenuHdle, g_OutlineId,
					   GetTrackOutlineFileName(strCategory.c_str(),strTrackPath.c_str()).c_str());

	// Update category info
	std::string strCarCat;
	bool bCollisions;
	NetGetNetwork()->GetHostSettings(strCarCat,bCollisions);
	GfuiLabelSetText(racemanMenuHdle,g_catHd,strCarCat.c_str());

	//fill in player data
	int nCars = GfParmGetEltNb(reInfo->params, RM_SECT_DRIVERS);

	char	dname[256];
	char    robpath[256];

	float *pColor = &green[0];

	bool bEveryoneReadyToRace = true;
	
	for (int i = 1; i < nCars+1; i++) 
	{
		sprintf(dname, "%s/%d", RM_SECT_DRIVERS, i);

		const char* robot = GfParmGetStr(reInfo->params, dname, RM_ATTR_MODULE, "");

		//lookup playerName and car name
		sprintf(robpath,"drivers/%s/%s.xml",robot,robot);
		void *pMod = GfParmReadFileLocal(robpath,GFPARM_RMODE_REREAD);

		if (pMod == NULL)
		{
			//try again in other path
			sprintf(robpath,"drivers/%s/%s.xml",robot,robot);
			pMod = GfParmReadFile(robpath,GFPARM_RMODE_REREAD);
			if (pMod == NULL)
				continue;
		}

		assert(pMod);

		char ppname[256];
		int idx = GfParmGetNum(reInfo->params, dname, RM_ATTR_IDX, "",0);
		sprintf(ppname,"Robots/index/%d",idx);
		const char* name = GfParmGetStr(pMod, ppname, RM_ATTR_NAME, "");

		const char* car = GfParmGetStr(pMod, ppname, "car name", "");
		std::string strRealCar = GfCars::self()->getCar(car)->getName();

		// WAIT : pNData->m_vecReadyStatus[i-1] ?!
		//        This can only work when _only_ networkhuman drivers in the race
		//        (that is _no_robot_driver_) ; because m_vecReadyStatus is indexed
		//        by the networkhuman drivers list.
		// TO fix this, 2 solutions:
		// 1) make the networking module take care of the robot drivers too
		//    (in m_vecReadyStatus, m_vecNetworkPlayers, ...)
		// 2) make the networking _menu_ only take care of the networkhuman drivers.
		bool bReady = bRobotsReady;
		if(strcmp(NETWORKROBOT, GfParmGetStr(reInfo->params, dname, RM_ATTR_MODULE, "")) == 0) {
			// Write car model, as it may have changed via garage menu
			if (NetIsServer()) {
				NetServerMutexData *pSData = NetGetServer()->LockServerData();
				strncpy(pSData->m_vecNetworkPlayers[idx-1].car, car, 64);
				GfLogInfo("idx %d car set to %s\n", idx, car);

				// also need to write back for garage menu
				const GfCar* newCar = GfCars::self()->getCar(car);
				newDriver = GfDrivers::self()->getDriver(NETWORKROBOT, pSData->m_vecNetworkPlayers[idx-1].idx);
				newDriver->setCar(newCar);
				NetGetServer()->UnlockServerData();
			}

			//GfLogInfo("idx %d, m_vecReadyStatus.size() %d\n", idx, pNData->m_vecReadyStatus.size());
			NetMutexData *pNData = NetGetNetwork()->LockNetworkData();
			bReady = pNData->m_vecReadyStatus[idx-1];
			NetGetNetwork()->UnlockNetworkData();
		}

		int readyindex = 0;
		if (bReady)
			readyindex = 1;	
		else
			bEveryoneReadyToRace = false;

		if (strcmp(NetGetNetwork()->GetDriverName(),name)==0)
		{
			pColor = &green[0];
			g_strCar = strRealCar;
			//Make sure checkbox matches ready state
			GfuiCheckboxSetChecked(racemanMenuHdle, g_ReadyCheckboxId, bReady);
			if (NetGetClient())
				EnableMenuClientButtons(bReady);
			else
				EnableMenuHostButtons(bReady);
		}
		else
			pColor = &white[0];

		GfuiVisibilitySet(racemanMenuHdle,g_readystatus[i-1],true);
		GfuiStaticImageSetActive(racemanMenuHdle,g_readystatus[i-1],readyindex);
		GfuiLabelSetColor(racemanMenuHdle, g_playerNames[i-1], pColor);
		GfuiLabelSetText(racemanMenuHdle,g_playerNames[i-1],name);

		GfuiLabelSetColor(racemanMenuHdle, g_carNames[i-1], pColor);
		GfuiLabelSetText(racemanMenuHdle,g_carNames[i-1],strRealCar.c_str());
		GfParmReleaseHandle(pMod);
	}

	//Clear out rest of table
	for (int i=nCars;i<MAXNETWORKPLAYERS;i++)
	{
		GfuiVisibilitySet(racemanMenuHdle,g_readystatus[i],false);
		GfuiLabelSetText(racemanMenuHdle,g_playerNames[i],"");
		GfuiLabelSetText(racemanMenuHdle,g_carNames[i],"");
	}

	pNetwork->SetRefreshDisplay(false);
	GfuiApp().eventLoop().postRedisplay();

	if (NetIsClient())
	{	
		NetGetClient()->ConnectToClients();

		if (!NetGetClient()->TimeSynced())
		{
			NetGetClient()->SendServerTimeRequest();
		}
	}

	if (NetIsServer())
	{
		if (bEveryoneReadyToRace && nCars > 1)
			ServerPrepareStartNetworkRace(NULL);
	}
}


static void
rmNetworkClientDisconnect(void * /* dummy */)
{
	GfOut("Disconnecting from server\n");
	if (NetGetClient())
		NetGetClient()->Disconnect();

	GfuiScreenActivate(RmRaceSelectMenuHandle);
}


static void
CheckDriversCategory()
{
	bool bDriversChange = false;
	std::string strCarCat;
	bool bCollisions;
	NetGetNetwork()->GetHostSettings(strCarCat,bCollisions);
	if (strCarCat =="All")
		return;

	const std::vector<std::string> vecCars =
		GfCars::self()->getCarIdsInCategory(strCarCat);

	//Make sure all cars are in the correct category or force change of car
	unsigned int count = 0;
	NetServerMutexData *pSData = NetGetServer()->LockServerData();

	count = pSData->m_vecNetworkPlayers.size();
	
	for (unsigned int i=0;i<count;i++)
	{
		const GfCar* pCar = GfCars::self()->getCar(pSData->m_vecNetworkPlayers[i].car);
		if (pCar->getCategoryId() != strCarCat)
		{
			//Pick first car in categroy
			//strncpy(pSData->m_vecNetworkPlayers[i].car,vecCars[0].c_str(),64);
			bDriversChange = true;
			NetGetServer()->OverrideDriverReady(pSData->m_vecNetworkPlayers[i].idx,false);
		}
	}

	if(bDriversChange)
	{
		NetGetServer()->CreateNetworkRobotFile();
	}
	
	//NetGetServer()->UnlockDrivers();
	NetGetServer()->UnlockServerData();
}

static void
HostServerIdle(void)
{
	GfuiIdle();
	if (NetIsServer())
	{
		if (NetGetServer()->GetRaceInfoChanged())
		{
			CheckDriversCategory();
			//Send to clients all of the xml files we modified and client needs to reload
			NetGetServer()->SendFilePacket("drivers/networkhuman/networkhuman.xml");
			NetGetServer()->SendFilePacket("config/raceman/networkrace.xml");
			NetGetServer()->SendRaceSetupPacket();
			NetGetServer()->SendDriversReadyPacket();
			NetGetServer()->SetRaceInfoChanged(false);
		}
		else
		{
			if (NetGetServer()->GetRefreshDisplay())
			{
				UpdateNetworkPlayers();
			}

		}

		GfuiApp().eventLoop().postRedisplay();
	}
	
    /* Let CPU take breath (and fans stay at low and quiet speed) */
    GfSleep(0.001);
}


static void
ClientIdle(void)
{
	GfuiIdle();
	if (NetIsClient())
	{
		if (!NetGetClient()->TimeSynced())
		{
			NetGetClient()->SendServerTimeRequest();
		}

		if (NetGetClient()->GetRefreshDisplay())
		{
			//Update the screen
			UpdateNetworkPlayers();
			GfuiApp().eventLoop().postRedisplay();
		}

		if (NetGetClient()->PrepareToRace())
		{
			NetGetClient()->SetLocalDrivers();
			LmRaceEngine().startNewRace();
		}

		if (!NetGetClient()->IsConnected())
		{
			rmNetworkClientDisconnect(NULL);
		}

		GfuiApp().eventLoop().postRedisplay();
	}
	
    /* Let CPU take breath (and fans stay at low and quiet speed) */
    GfSleep(0.001);
}

static void
NetworkRaceInfo()
{
	NetDriver driver;
	int i = 1;

	NetGetServer()->SetRaceXMLFile("config/raceman/networkrace.xml");

	//Look up race info
	tRmInfo* reInfo = LmRaceEngine().inData();
	reInfo->params = GfParmReadFileLocal("config/raceman/networkrace.xml",GFPARM_RMODE_STD);

	int nCars = GfParmGetEltNb(reInfo->params, RM_SECT_DRIVERS);

	if (nCars == 0)
	{
		// Add all local humans if there are no drivers already specified
		while (GetHumanDriver(driver,i++)) {
			driver.client = false;
			driver.active = true;
			NetGetServer()->UpdateDriver(driver);
			NetGetServer()->SetDriverName(driver.name);
			GfLogInfo("NetworkRaceInfo: Adding default driver %s\n",driver.name);

		}

		// ensure changes writen to 'networkrace.xml'
		NetGetServer()->GenerateDriversForXML();

		// add drivers so they show up in race config dialogue
		GfDrivers::self()->reload();
		LmRaceEngine().race()->load(LmRaceEngine().race()->getManager(), true);
	} else {
		// Add the humans which are already in the race
		char	dname[256];

		for (i = 1; i < nCars+1; i++) {
			sprintf(dname, "%s/%d", RM_SECT_DRIVERS, i);

			if(strcmp(NETWORKROBOT, GfParmGetStr(reInfo->params, dname, RM_ATTR_MODULE, "")) == 0) {
				if (GetHumanDriver(driver,i) > -1) {
					driver.client = false;
					driver.active = true;
					NetGetServer()->UpdateDriver(driver);
					NetGetServer()->SetDriverName(driver.name);
					GfLogInfo("NetworkRaceInfo: Adding default driver %s\n",driver.name);
				}
			}
		}
	}

	// make sure nobody is 'ready to race'
	NetMutexData *pNData = NetGetNetwork()->LockNetworkData();
	for (unsigned int i=0; i < pNData->m_vecReadyStatus.size(); i++)
		pNData->m_vecReadyStatus[i] = false;
	NetGetNetwork()->UnlockNetworkData();
	bRobotsReady = false;

	// ensure the system knows about 'new' network drivers
	reInfo->params = GfParmReadFileLocal("config/raceman/networkrace.xml",GFPARM_RMODE_REREAD);
	reInfo->_reName = GfParmGetStr(reInfo->params, RM_SECT_HEADER, RM_ATTR_NAME, "");
}

// Not used for the moment.
// static void
// NetworkDisplay(void)
// {
// }

static void OnActivateNetworkClient(void *)
{
	int nDriverIdx = NetGetNetwork()->GetDriverIdx();

	if(NetGetNetwork()->IsConnected() && nDriverIdx > -1) {
		// Menu reactivated after garage menu is done
		NetDriver driver;
		char newName[64];
		char dname[256];

		// check for car change
		if (bGarage == true ) {
			bGarage = false;

			tRmInfo* reInfo = LmRaceEngine().inData();
			reInfo->params = GfParmReadFileLocal("config/raceman/networkrace.xml",GFPARM_RMODE_REREAD);
			reInfo->_reName = GfParmGetStr(reInfo->params, RM_SECT_HEADER, RM_ATTR_NAME, "");

			sprintf(dname, "%s/%d", RM_SECT_DRIVERS, nDriverIdx);
			int idx = GfParmGetNum(reInfo->params, dname, RM_ATTR_IDX, "",0);

			GfDriver* PCurrentDriver = GfDrivers::self()->getDriver(NETWORKROBOT, idx);
			strncpy(newName, PCurrentDriver->getCar()->getId().c_str(), sizeof(newName));

			GfLogInfo("Client: Index %d changed to %s\n", idx, newName);
			NetGetNetwork()->SetCarInfo(newName);
		} else {
			// Ensure menu system knows about all cars
			GfDrivers::self()->reload();
			// tRmInfo* reInfo = LmRaceEngine().inData(); // Never used
			LmRaceEngine().race()->load(LmRaceEngine().race()->getManager(), true);
		}
	}

	GfuiApp().eventLoop().setRecomputeCB(ClientIdle);
	bGarage = false;
}


static void
OnActivateNetworkHost(void *)
{
	tRmInfo* reInfo = LmRaceEngine().inData();

	// Set everyone to the 'not-ready' state
	bRobotsReady = 0;
	NetMutexData *pNData = NetGetNetwork()->LockNetworkData();
	for (unsigned int i=0;i<pNData->m_vecReadyStatus.size();i++)
		pNData->m_vecReadyStatus[i] = false;
	NetGetNetwork()->UnlockNetworkData();

	NetGetServer()->SetRaceInfoChanged(true);

	reInfo->params = GfParmReadFileLocal("config/raceman/networkrace.xml",GFPARM_RMODE_REREAD);
	assert(reInfo->params);
	reInfo->_reName = GfParmGetStr(reInfo->params, RM_SECT_HEADER, RM_ATTR_NAME, "");
	GfuiApp().eventLoop().setRecomputeCB(HostServerIdle);	

	NetGetServer()->SetRefreshDisplay(true);
}

static void
rmNetworkServerDisconnect(void * /* dummy */)
{
	GfLogInfo("Disconnecting all clients\n");
	if (NetGetServer())
		NetGetServer()->Disconnect();

	GfuiScreenActivate(RmRaceSelectMenuHandle);
}

static void
rmCarSettingsMenu(void *pMenu)
{
	int nDriverIdx = NetGetNetwork()->GetDriverIdx();

	if (nDriverIdx > -1) {
		NetDriver driver;
		// char newName[64]; Never used
		char dname[256];

		// check for car change
		GfLogInfo("Car %d changed \n", nDriverIdx);

		tRmInfo* reInfo = LmRaceEngine().inData();
		reInfo->params = GfParmReadFileLocal("config/raceman/networkrace.xml",GFPARM_RMODE_REREAD);
		reInfo->_reName = GfParmGetStr(reInfo->params, RM_SECT_HEADER, RM_ATTR_NAME, "");

		sprintf(dname, "%s/%d", RM_SECT_DRIVERS, nDriverIdx);
		int idx = GfParmGetNum(reInfo->params, dname, RM_ATTR_IDX, "",0);

		// Garage menu to change clients car
		GfDriver* PCurrentDriver = GfDrivers::self()->getDriver(NETWORKROBOT, idx);

		GarageMenu.setPreviousMenuHandle(racemanMenuHdle);
		GarageMenu.runMenu(LmRaceEngine().race(), PCurrentDriver);
		bGarage = true;
	}
}

static void
rmNetworkHostSettingsMenu(void *pMenu)
{
	g_HostMenu.initialize(pMenu);
	
	RmSetRacemanMenuHandle(g_HostMenu.getMenuHandle());

	g_HostMenu.runMenu();
}


// Host on-line race menu.
void
RmNetworkHostMenu(void * /* dummy */)
{
	GfLogTrace("Entering Network Host menu.\n");
	
	if (!NetGetNetwork())
	{
		NetSetServer(true);
		NetSetClient(false);
		if (!NetGetServer()->Start(SPEEDDREAMSPORT))
		{
			NetSetServer(false);
			return;
		}
	}

	if (racemanMenuHdle)
		GfuiScreenRelease(racemanMenuHdle);

	racemanMenuHdle = GfuiScreenCreate(NULL,  NULL, (tfuiCallback)OnActivateNetworkHost, 
								   NULL, (tfuiCallback)NULL, 1);

	void *mparam = GfuiMenuLoad("networkhostmenu.xml");

	GfuiMenuCreateStaticControls(racemanMenuHdle, mparam);

	RmSetRacemanMenuHandle(racemanMenuHdle);

	NetworkRaceInfo();

	g_trackHd = GfuiMenuCreateLabelControl(racemanMenuHdle,mparam,"trackname");

	g_lapsHd = GfuiMenuCreateLabelControl(racemanMenuHdle,mparam,"lapcountname");
	g_catHd = GfuiMenuCreateLabelControl(racemanMenuHdle,mparam,"carcatname");
    
	g_OutlineId = GfuiMenuCreateStaticImageControl(racemanMenuHdle,mparam,"outlineimage");
	
	//Show players
	for (int i = 0; i < MAXNETWORKPLAYERS; i++) {
		char buf[1024];
		sprintf(buf,"ready%i",i);
		g_readystatus[i] = GfuiMenuCreateStaticImageControl(racemanMenuHdle,mparam,buf);
		GfuiVisibilitySet(racemanMenuHdle,g_readystatus[i],false);

		sprintf(buf,"driver%i",i);
		g_playerNames[i] = GfuiMenuCreateLabelControl(racemanMenuHdle,mparam,buf);
		GfuiLabelSetText(racemanMenuHdle,g_playerNames[i],"");

		sprintf(buf,"car%i",i);
		g_carNames[i] =  GfuiMenuCreateLabelControl(racemanMenuHdle,mparam,buf);
		GfuiLabelSetText(racemanMenuHdle,g_carNames[i],"");
	}

	g_ReadyCheckboxId =
		GfuiMenuCreateCheckboxControl(racemanMenuHdle, mparam, "playerreadycheckbox",
								NULL, onHostPlayerReady);

	g_HostSettingsButtonId =
		GfuiMenuCreateButtonControl(racemanMenuHdle, mparam, "networkhostsettings",
								racemanMenuHdle, rmNetworkHostSettingsMenu);
	GfuiEnable(racemanMenuHdle, g_HostSettingsButtonId, GFUI_DISABLE);

	g_RaceSetupId =
		GfuiMenuCreateButtonControl(racemanMenuHdle, mparam, "racesetup",
								racemanMenuHdle, RmConfigureRace);

	GfuiMenuCreateButtonControl(racemanMenuHdle, mparam, "start race",
								NULL, ServerPrepareStartNetworkRace);
	g_CancelButtonId = GfuiMenuCreateButtonControl(racemanMenuHdle, mparam, "cancel",
								NULL, rmNetworkServerDisconnect);

	GfParmReleaseHandle(mparam);
	
	GfuiMenuDefaultKeysAdd(racemanMenuHdle);
	GfuiAddKey(racemanMenuHdle, GFUIK_ESCAPE, "Back to previous menu",
								0, 0, rmNetworkServerDisconnect);

	UpdateNetworkPlayers();

	GfuiScreenActivate(racemanMenuHdle);
}

static void
ShowWaitingToConnectScreen()
{
	GfLogTrace("Entering Network Wait Connection menu.\n");
	
	if (racemanMenuHdle) 
		GfuiScreenRelease(racemanMenuHdle);

	racemanMenuHdle = GfuiScreenCreate(NULL,  NULL, (tfuiCallback) NULL, 
									   NULL, (tfuiCallback)NULL,  1);

	void *mparam = GfuiMenuLoad("networkwaitconnectmenu.xml");
	GfuiMenuCreateStaticControls(racemanMenuHdle, mparam);
	    
	GfuiMenuDefaultKeysAdd(racemanMenuHdle);

	GfuiScreenActivate(racemanMenuHdle);

	GfuiApp().eventLoop().postRedisplay();
}

void
RmNetworkClientMenu(void * /* dummy */)
{
	GfLogTrace("Entering Network Client menu.\n");
	
	ShowWaitingToConnectScreen();
	
	if (!NetGetClient())
	{
		NetSetServer(false);
		NetSetClient(true);

		NetDriver driver;
		GetHumanDriver(driver,1);
		driver.client = true;
		driver.active = true;
		strcpy(driver.name,g_strDriver.c_str());
		if (!NetGetClient()->ConnectToServer((char*)g_strHostIP.c_str(),SPEEDDREAMSPORT,&driver))
		{
			//failed so back to connect menu
			NetworkClientConnectMenu(NULL);
			return;
		}

		//NetGetClient()->SendDriverInfoPacket(&driver);
	}

	if (racemanMenuHdle)
		GfuiScreenRelease(racemanMenuHdle);

	racemanMenuHdle = GfuiScreenCreate(NULL,  NULL, (tfuiCallback)OnActivateNetworkClient, 
									   NULL, (tfuiCallback)NULL, 1);

	void *mparam = GfuiMenuLoad("networkclientmenu.xml");
	GfuiMenuCreateStaticControls(racemanMenuHdle, mparam);

	GfuiMenuDefaultKeysAdd(racemanMenuHdle);

	RmSetRacemanMenuHandle(racemanMenuHdle);

	g_trackHd = GfuiMenuCreateLabelControl(racemanMenuHdle,mparam,"trackname");

	g_lapsHd = GfuiMenuCreateLabelControl(racemanMenuHdle,mparam,"lapcountname");
	g_catHd = GfuiMenuCreateLabelControl(racemanMenuHdle,mparam,"carcatname");

	g_OutlineId = GfuiMenuCreateStaticImageControl(racemanMenuHdle,mparam,"outlineimage");
	
	//Show players
	for (int i = 0; i < MAXNETWORKPLAYERS; i++) 
	{
		char buf[1024];
		sprintf(buf,"ready%i",i);
		g_readystatus[i] = GfuiMenuCreateStaticImageControl(racemanMenuHdle,mparam,buf);
		GfuiVisibilitySet(racemanMenuHdle,g_readystatus[i],false);

		sprintf(buf,"driver%i",i);
		g_playerNames[i] = GfuiMenuCreateLabelControl(racemanMenuHdle,mparam,buf);
		GfuiLabelSetText(racemanMenuHdle,g_playerNames[i],"");

		sprintf(buf,"car%i",i);
		g_carNames[i] =  GfuiMenuCreateLabelControl(racemanMenuHdle,mparam,buf);
		GfuiLabelSetText(racemanMenuHdle,g_carNames[i],"");
	}

	g_ReadyCheckboxId =
		GfuiMenuCreateCheckboxControl(racemanMenuHdle, mparam, "playerreadycheckbox",
									  NULL, onClientPlayerReady);
	g_CarSetupButtonId =
		GfuiMenuCreateButtonControl(racemanMenuHdle, mparam, "garage",
									racemanMenuHdle, rmCarSettingsMenu);

	g_DisconnectButtonId =
		GfuiMenuCreateButtonControl(racemanMenuHdle, mparam, "disconnect",
									NULL,rmNetworkClientDisconnect);

	GfParmReleaseHandle(mparam);

	UpdateNetworkPlayers();
	
	GfuiScreenActivate(racemanMenuHdle);
	
	GfuiApp().eventLoop().setRecomputeCB(ClientIdle);
}

static void 
ChangeName(void * /*dummy*/)
{
   	char	*val;

   	val = GfuiEditboxGetString(racemanMenuHdle, g_NameId);
	if (val!=NULL)
		g_strDriver = val;
}

static void
ChangeIP(void * /* dummy */)
{
   	char	*val;

   	val = GfuiEditboxGetString(racemanMenuHdle, g_IPEditId);
	if (val!=NULL)
		g_strHostIP = val;
}


static void
LookupPlayerSetup(std::string & strDriver,std::string & strCar)
{
    	void	*drvinfo;
	
	char buf[255];
	sprintf(buf, "%s", HM_DRV_FILE);

	drvinfo = GfParmReadFileLocal(buf, GFPARM_RMODE_REREAD);
		assert(drvinfo);
	if (drvinfo == NULL) {
		return;
	}

	char sstring[256];

	sprintf(sstring, "%s/%s/%d", ROB_SECT_ROBOTS, ROB_LIST_INDEX, 1);
	strDriver = GfParmGetStr(drvinfo, sstring, ROB_ATTR_NAME, "");
	strCar = GfParmGetStr(drvinfo, sstring, ROB_ATTR_CAR, "");
	GfParmReleaseHandle(drvinfo);
}

static void
NetworkClientConnectMenu(void * /* dummy */)
{
	GfLogTrace("Entering Network Client Connect menu.\n");
	
	LookupPlayerSetup(g_strDriver,g_strCar);

	if (racemanMenuHdle)
		GfuiScreenRelease(racemanMenuHdle);

	racemanMenuHdle = GfuiScreenCreate(NULL, NULL, (tfuiCallback)NULL, NULL, (tfuiCallback)NULL, 1);

	void *mparam = GfuiMenuLoad("networkclientconnectmenu.xml");
	
	GfuiMenuCreateStaticControls(racemanMenuHdle, mparam);

	g_IPEditId = GfuiMenuCreateEditControl(racemanMenuHdle, mparam, "IPAddrEdit", 0, 0, ChangeIP);

	char namebuf[255];
	snprintf(namebuf, sizeof(namebuf), "%s", g_strDriver.c_str());
	g_NameId = GfuiMenuCreateEditControl(racemanMenuHdle, mparam, "PlayerNameEdit", 0, 0, ChangeName);
	GfuiEditboxSetString(racemanMenuHdle, g_NameId, namebuf);
	
	GfuiMenuCreateButtonControl(racemanMenuHdle, mparam, "ConnectButton",
								0, RmNetworkClientMenu);
	GfuiMenuCreateButtonControl(racemanMenuHdle, mparam, "BackButton",
								RmRaceSelectMenuHandle, GfuiScreenActivate);
	
	GfuiMenuDefaultKeysAdd(racemanMenuHdle);
	GfuiAddKey(racemanMenuHdle, GFUIK_ESCAPE, "Back to previous menu",
								RmRaceSelectMenuHandle, 0, GfuiScreenActivate);

	GfParmReleaseHandle(mparam);
	
	GfuiScreenActivate(racemanMenuHdle);
}

void
RmNetworkMenu(void *)
{
	GfLogTrace("Entering Network menu.\n");
	
 	tRmInfo* reInfo = LmRaceEngine().inData();
	void *params = reInfo->params;

	if (NetGetNetwork())
	{
		NetGetNetwork()->ResetNetwork();
	}

	racemanMenuHdle = GfuiScreenCreate(NULL,  NULL, (tfuiCallback)NULL, 
								NULL, (tfuiCallback)NULL, 1);

	void *mparam = GfuiMenuLoad("networkmenu.xml");

	GfuiMenuCreateStaticControls(racemanMenuHdle, mparam);

	const int nTitleLabelId = GfuiMenuCreateLabelControl(racemanMenuHdle, mparam, "TitleLabel");
	const char* pszTitle = GfParmGetStr(params, RM_SECT_HEADER, RM_ATTR_NAME, 0);
	if (pszTitle)
		GfuiLabelSetText(racemanMenuHdle, nTitleLabelId, pszTitle);
	
	GfuiMenuCreateButtonControl(racemanMenuHdle, mparam, "HostButton",
								0, RmNetworkHostMenu);

	GfuiMenuCreateButtonControl(racemanMenuHdle, mparam, "JoinButton",
								0, NetworkClientConnectMenu);

	GfuiMenuCreateButtonControl(racemanMenuHdle, mparam, "BackButton",
								RmRaceSelectMenuHandle, GfuiScreenActivate);

	GfuiMenuDefaultKeysAdd(racemanMenuHdle);
	GfuiAddKey(racemanMenuHdle, GFUIK_ESCAPE, "Back to previous menu",
								RmRaceSelectMenuHandle, 0, GfuiScreenActivate);

	GfParmReleaseHandle(mparam);
	
	GfuiScreenActivate(racemanMenuHdle);
}

static void
ServerPrepareStartNetworkRace(void * /* dummy */)
{
	NetGetServer()->SetLocalDrivers();
	
	//Tell all clients to prepare to race and wait for response from all clients
	NetGetServer()->SendPrepareToRacePacket();

	//restore the idle function
	GfuiApp().eventLoop().setRecomputeCB(GfuiIdle);
	LmRaceEngine().startNewRace();
}

// Retrieve the NetDriver instance with given index in the human module interface list
static int
GetHumanDriver(NetDriver &driver,int index)
{
	void *params = GfParmReadFileLocal("drivers/human/human.xml", GFPARM_RMODE_STD);
	assert(params);
	char path2[256];
	sprintf(path2, "Robots/index/%d",index);

	if (GfParmExistsSection(params, path2) == 0) return 0;

	strncpy(driver.name,GfParmGetStr(params, path2, "name",NULL),64);
	strncpy(driver.sname,GfParmGetStr(params, path2, ROB_ATTR_SNAME, NULL), 64);
	strncpy(driver.cname,GfParmGetStr(params, path2, ROB_ATTR_CODE, NULL), 4);
	strncpy(driver.car,GfParmGetStr(params, path2, "car name",NULL),64);
	strncpy(driver.type,GfParmGetStr(params, path2, "type",NULL),64);
	strncpy(driver.skilllevel,GfParmGetStr(params, path2, "skill level",NULL),64);

	driver.racenumber = GfParmGetNum(params, path2, "race number",NULL,1.0);
	driver.red = GfParmGetNum(params, path2, "red",NULL,1.0);
	driver.green = GfParmGetNum(params, path2, "green",NULL,1.0);
	driver.blue = GfParmGetNum(params, path2, "blue",NULL,1.0);

	strncpy(driver.module,NETWORKROBOT,64);
	GfParmReleaseHandle(params);

	return 1;
}
