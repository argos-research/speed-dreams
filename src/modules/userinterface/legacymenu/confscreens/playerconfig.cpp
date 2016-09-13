/***************************************************************************

    file                 : playerconfig.cpp
    created              : Wed Apr 26 20:05:12 CEST 2000
    copyright            : (C) 2000 by Eric Espie
    email                : torcs@free.fr
    version              : $Id: playerconfig.cpp 6270 2015-11-23 19:44:40Z madbad $

***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <string>

#include <portability.h>
#include <tgfclient.h>
#include <robot.h>
#include <playerpref.h>
#include <graphic.h>
#include <gui.h>
#ifdef WEBSERVER
#include <webserver.h>
#endif //WEBSERVER

#include <drivers.h>

#include "controlconfig.h"
#include "playerconfig.h"


static const char *PlayerNamePrompt	= "-- Enter name --";
static const char *NoPlayer = "-- No one --";
static const char *HumanDriverModuleName  = "human";
static const char *DefaultCarName  = "sc-lynx-220";
#ifdef WEBSERVER
static const char *DefaultWebserverusername  = "username";
static const char *DefaultWebserverpassword  = "password";
#endif //WEBSERVER

static const char *SkillLevelString[] = { ROB_VAL_ARCADE, ROB_VAL_SEMI_ROOKIE, ROB_VAL_ROOKIE, ROB_VAL_AMATEUR, ROB_VAL_SEMI_PRO, ROB_VAL_PRO };
static const int NbSkillLevels = sizeof(SkillLevelString) / sizeof(SkillLevelString[0]);

static char buf[1024];

static const float LabelColor[] = {1.0, 0.0, 1.0, 1.0};

static int	ScrollList;
static void	*ScrHandle = NULL;
static void	*PrevScrHandle = NULL;

static void	*PrefHdle = NULL;
static void	*PlayerHdle = NULL;
static void	*GraphHdle = NULL;

static int NameEditId;
static int RaceNumEditId;
static int GearChangeEditId;
static int PitsEditId;
static int SkillEditId;
static int AutoReverseEditId;
static int AutoReverseLeftId;
static int AutoReverseRightId;
static int AutoReverseLabelId;
#ifdef WEBSERVER
static int WebUsernameEditId;
static int WebPasswordEditId;
#endif //WEBSERVER

/* Struct to define a generic ("internal name / id", "displayable name") pair */
typedef struct tInfo
{
    char	*name;
    char	*dispname;
    #ifdef WEBSERVER
    char	*webserverusername;
    char	*webserverpassword;
	#endif //WEBSERVER
    
} tInfo;

/* Player info struct */
struct tPlayerInfo
{
public:

	tPlayerInfo(const char *name = HumanDriverModuleName, const char *dispname = 0,
				const char *defcarname = 0, int racenumber = 0, int skilllevel = 0,
				float *color = 0, 
				tGearChangeMode gearchangemode = GEAR_MODE_AUTO, int autoreverse = 0, 
				int nbpitstops = 0
				#ifdef WEBSERVER
				,
				const char *webserverusername = 0,
				const char *webserverpassword = 0
				#endif //WEBSERVER
				) 
	{
		_info.name = 0;
		setName(name);
		_info.dispname = 0;
		setDispName(dispname);
		_defcarname = 0;
		setDefaultCarName(defcarname);
		_racenumber = racenumber; 
		_gearchangemode = gearchangemode; 
		_nbpitstops = nbpitstops; 
		_skilllevel = skilllevel; 
		_autoreverse = autoreverse;
		#ifdef WEBSERVER
		_webserverusername = 0;
		setWebserverusername(webserverusername);
		_webserverpassword = 0;
		setWebserverpassword(webserverpassword);
		#endif //WEBSERVER	
		_color[0] = color ? color[0] : 1.0; 
		_color[1] = color ? color[1] : 1.0; 
		_color[2] = color ? color[2] : 0.5; 
		_color[3] = color ? color[3] : 1.0;
	}

	tPlayerInfo(const tPlayerInfo &src)
	{
		_info.name = 0;
		setName(src._info.name);
		_info.dispname = 0;
		setDispName(src._info.dispname);
		_defcarname = 0;
		setDefaultCarName(src._defcarname);
		_racenumber = src._racenumber; 
		_gearchangemode = src._gearchangemode; 
		_nbpitstops = src._nbpitstops; 
		_skilllevel = src._skilllevel; 
		_autoreverse = src._autoreverse;
		#ifdef WEBSERVER
		_info.webserverusername = 0;
		setWebserverusername(src._info.webserverusername);
		_info.webserverpassword = 0;
		setWebserverpassword(src._info.webserverpassword);
		#endif //WEBSERVER
		_color[0] = src._color[0]; 
		_color[1] = src._color[1]; 
		_color[2] = src._color[2]; 
		_color[3] = src._color[3];
	}

	const char *name()  const { return _info.name; };
	const char *dispName()  const { return _info.dispname; }
	const char *defaultCarName()  const { return _defcarname; }
	int raceNumber() const { return _racenumber; }
	tGearChangeMode gearChangeMode() const { return _gearchangemode; }
	int nbPitStops() const { return _nbpitstops; }
	float color(int idx) const { return (idx >= 0 && idx < 4) ? _color[idx] : 0.0; }
	int skillLevel() const { return _skilllevel; }
	int autoReverse() const { return _autoreverse; }
	#ifdef WEBSERVER
	const char *webserverusername()  const { return _webserverusername; }
	const char *webserverpassword()  const { return _webserverpassword; }
	#endif //WEBSERVER

	void setName(const char *name)
	{
		if (_info.name)
			delete[] _info.name;
		if (!name || strlen(name) == 0)
			name = HumanDriverModuleName;
		_info.name = new char[strlen(name)+1];
		strcpy(_info.name, name); // Can't use strdup : free crashes in destructor !?
	}
	void setDispName(const char *dispname)
	{
		if (_info.dispname)
			delete[] _info.dispname;
		if (!dispname)
			dispname = NoPlayer;
		_info.dispname = new char[strlen(dispname)+1];
		strcpy(_info.dispname, dispname); // Can't use strdup : free crashes in destructor !?
	}
	void setDefaultCarName(const char *defcarname)
	{
		if (_defcarname)
			delete[] _defcarname;
		if (!defcarname || strlen(defcarname) == 0)
			defcarname = DefaultCarName;
		_defcarname = new char[strlen(defcarname)+1];
		strcpy(_defcarname, defcarname); // Can't use strdup : free crashes in destructor !?
	}
	
	#ifdef WEBSERVER
	void setWebserverusername(const char *webserverusername)
	{
		if (_webserverusername)
			delete[] _webserverusername;
		if (!webserverusername || strlen(webserverusername) == 0)
			webserverusername = DefaultWebserverusername;
		_webserverusername = new char[strlen(webserverusername)+1];
		strcpy(_webserverusername, webserverusername); // Can't use strdup : free crashes in destructor !?
	}
	void setWebserverpassword(const char *webserverpassword)
	{
		if (_webserverpassword)
			delete[] _webserverpassword;
		if (!webserverpassword || strlen(webserverpassword) == 0)
			webserverpassword = DefaultWebserverpassword;
		_webserverpassword = new char[strlen(webserverpassword)+1];
		strcpy(_webserverpassword, webserverpassword); // Can't use strdup : free crashes in destructor !?
	}
	#endif //WEBSERVER

	void setRaceNumber(int raceNumber) { _racenumber = raceNumber; }
	void setGearChangeMode(tGearChangeMode gearChangeMode) { _gearchangemode = gearChangeMode; }
	void setNbPitStops(int nbPitStops) { _nbpitstops = nbPitStops; }
	void setSkillLevel(int skillLevel) { _skilllevel = skillLevel; }
	void setAutoReverse(int autoReverse) { _autoreverse = autoReverse; }

	~tPlayerInfo() 
	{
		if (_info.dispname)
			delete[] _info.dispname;
		if (_info.name)
			delete[] _info.name;
		if (_defcarname)
			delete[] _defcarname;

		#ifdef WEBSERVER
		if (_webserverusername)
			delete[] _webserverusername;
		if (_webserverpassword)
			delete[] _webserverpassword;
		#endif //WEBSERVER			

	}

	// Gear change mode enum to string conversion
	const char *gearChangeModeString() const
	{ 
		const char *gearChangeStr;
	
		if (_gearchangemode == GEAR_MODE_AUTO) {
			gearChangeStr = HM_VAL_AUTO;
		} else if (_gearchangemode == GEAR_MODE_GRID) {
			gearChangeStr = HM_VAL_GRID;
		} else if (_gearchangemode == GEAR_MODE_HBOX) {
			gearChangeStr = HM_VAL_HBOX;
		} else {
			gearChangeStr = HM_VAL_SEQ;
		}
 
		return gearChangeStr;
	}

private:

	tInfo			_info;
	char*			_defcarname;
	int				_racenumber;
	tGearChangeMode	_gearchangemode;
	int				_nbpitstops;
	float			_color[4];
	int				_skilllevel;
	int				_autoreverse;
	#ifdef WEBSERVER
	char*			_webserverusername;
	char*			_webserverpassword;
	#endif //WEBSERVER	
};


/* The human driver (= player) info list */
typedef std::deque<tPlayerInfo*> tPlayerInfoList;
static tPlayerInfoList PlayersInfo;

/* The currently selected player (PlayersInfo.end() if none) */
static tPlayerInfoList::iterator CurrPlayer;

/* A bool to ("yes", "no") conversion table */
static const char *Yn[] = {HM_VAL_YES, HM_VAL_NO};

static int ReloadValues = 1;

/* Load screen editable fields with relevant values :
   - all empty if no player selected,
   - from selected player current settings otherwise 
*/
static void
refreshEditVal(void)
{
    int autoRevVisible = GFUI_INVISIBLE;

    if (CurrPlayer == PlayersInfo.end()) {

		GfuiEditboxSetString(ScrHandle, NameEditId, "");
		GfuiEnable(ScrHandle, NameEditId, GFUI_DISABLE);

		GfuiEditboxSetString(ScrHandle, RaceNumEditId, "");
		GfuiEnable(ScrHandle, RaceNumEditId, GFUI_DISABLE);

		GfuiLabelSetText(ScrHandle, GearChangeEditId, "");
		GfuiEnable(ScrHandle, GearChangeEditId, GFUI_DISABLE);

		GfuiEditboxSetString(ScrHandle, PitsEditId, "");
		GfuiEnable(ScrHandle, PitsEditId, GFUI_DISABLE);

		GfuiLabelSetText(ScrHandle, SkillEditId, "");
		GfuiEnable(ScrHandle, SkillEditId, GFUI_DISABLE);

		GfuiLabelSetText(ScrHandle, AutoReverseEditId, "");
		GfuiEnable(ScrHandle, AutoReverseEditId, GFUI_DISABLE);
		
		#ifdef WEBSERVER
		GfuiEditboxSetString(ScrHandle, WebUsernameEditId, "");
		GfuiEnable(ScrHandle, WebUsernameEditId, GFUI_DISABLE);

		GfuiEditboxSetString(ScrHandle, WebPasswordEditId, "");
		GfuiEnable(ScrHandle, WebPasswordEditId, GFUI_DISABLE);
		#endif //WEBSERVER

    } else {

        if (strcmp((*CurrPlayer)->dispName(), NoPlayer)) {
			GfuiEditboxSetString(ScrHandle, NameEditId, (*CurrPlayer)->dispName());
		} else {
			GfuiEditboxSetString(ScrHandle, NameEditId, PlayerNamePrompt);
		}
		GfuiEnable(ScrHandle, NameEditId, GFUI_ENABLE);

		snprintf(buf, sizeof(buf), "%d", (*CurrPlayer)->raceNumber());
		GfuiEditboxSetString(ScrHandle, RaceNumEditId, buf);
		GfuiEnable(ScrHandle, RaceNumEditId, GFUI_ENABLE);

		GfuiLabelSetText(ScrHandle, GearChangeEditId, (*CurrPlayer)->gearChangeModeString());
		GfuiEnable(ScrHandle, GearChangeEditId, GFUI_ENABLE);

		snprintf(buf, sizeof(buf), "%d", (*CurrPlayer)->nbPitStops());
		GfuiEditboxSetString(ScrHandle, PitsEditId, buf);
		GfuiEnable(ScrHandle, PitsEditId, GFUI_ENABLE);

		GfuiLabelSetText(ScrHandle, SkillEditId, SkillLevelString[(*CurrPlayer)->skillLevel()]);
		GfuiEnable(ScrHandle, SkillEditId, GFUI_ENABLE);

		GfuiLabelSetText(ScrHandle, AutoReverseEditId, Yn[(*CurrPlayer)->autoReverse()]);
		GfuiEnable(ScrHandle, AutoReverseEditId, GFUI_ENABLE);

		#ifdef WEBSERVER
		snprintf(buf, sizeof(buf), "%s", (*CurrPlayer)->webserverusername());		
		GfuiEditboxSetString(ScrHandle, WebUsernameEditId, buf);
		GfuiEnable(ScrHandle, WebUsernameEditId, GFUI_ENABLE);

		snprintf(buf, sizeof(buf), "%s", (*CurrPlayer)->webserverpassword());		
		GfuiEditboxSetString(ScrHandle, WebPasswordEditId, buf);
		GfuiEnable(ScrHandle, WebPasswordEditId, GFUI_ENABLE);
		#endif //WEBSERVER
		

		if ((*CurrPlayer)->gearChangeMode() == GEAR_MODE_AUTO)
			autoRevVisible = GFUI_VISIBLE;
    }

    GfuiVisibilitySet(ScrHandle, AutoReverseLabelId, autoRevVisible);
    GfuiVisibilitySet(ScrHandle, AutoReverseLeftId, autoRevVisible);
    GfuiVisibilitySet(ScrHandle, AutoReverseEditId, autoRevVisible);
    GfuiVisibilitySet(ScrHandle, AutoReverseRightId, autoRevVisible);
    
}

static void
onSelect(void * /* Dummy */)
{
    tPlayerInfoList::difference_type elemIdx;
    GfuiScrollListGetSelectedElement(ScrHandle, ScrollList, (void**)&elemIdx);

    CurrPlayer = PlayersInfo.begin() + elemIdx;

    refreshEditVal();
}

/* Update players scroll-list from PlayersInfo array */
static void
UpdtScrollList(void)
{
    const char	*str;
    int		i;
    void	*tmp;

    /* free the previous scrollist elements */
    while((str = GfuiScrollListExtractElement(ScrHandle, ScrollList, 0, (void**)&tmp)) != NULL) {
    }
    for (i = 0; i < (int)PlayersInfo.size(); i++) {
		GfuiScrollListInsertElement(ScrHandle, ScrollList, PlayersInfo[i]->dispName(), i, (void*)(long)i);
    }

    if (CurrPlayer != PlayersInfo.end()) {
		GfuiScrollListShowElement(ScrHandle, ScrollList, (int)(CurrPlayer - PlayersInfo.begin()));
    }
}

/* Put given player settings (from PlayersInfo array) to the human drivers and preferences params (index is the identification number in params, beginning at 1)*/
static void
PutPlayerSettings(unsigned index)
{
    if (!PlayerHdle || !PrefHdle) {
		return;
    }

    tPlayerInfo *player = PlayersInfo[index-1];

	// Graphics params (take driver name changes into account for camera settings).
    char drvSectionPath[128];
    snprintf(drvSectionPath, sizeof(drvSectionPath), "%s/%s/%u", ROB_SECT_ROBOTS, ROB_LIST_INDEX, index);
	const char* pszOldDispName = GfParmGetStr(PlayerHdle, drvSectionPath, ROB_ATTR_NAME, "");
	if (strcmp(pszOldDispName, player->dispName())) { // Only if the display name changed.
		char drvDispSecPath[128];
		snprintf(drvDispSecPath, sizeof(drvDispSecPath), "%s/%s", GR_SCT_DISPMODE, pszOldDispName);
		if (!GraphHdle) // Load graphic params file if not already done.
		{
			char pszGraphFileName[256];
			snprintf(pszGraphFileName, sizeof(pszGraphFileName), "%s%s", GfLocalDir(), GR_PARAM_FILE);
			GraphHdle = GfParmReadFile(pszGraphFileName, GFPARM_RMODE_REREAD);
		}

		if (GfParmExistsSection(GraphHdle, drvDispSecPath)) { // Change section name.
			GfParmListRenameElt(GraphHdle, GR_SCT_DISPMODE, pszOldDispName, player->dispName());
		}

		if (!GfParmListSeekFirst(GraphHdle, GR_SCT_DISPMODE)) {
			do {
				const char* pszSecName = GfParmListGetCurEltName(GraphHdle, GR_SCT_DISPMODE);
				if (!pszSecName || !isdigit(*pszSecName))
					continue; // Ignore sections whose name is not (likely) a screen id.
				snprintf(drvDispSecPath, sizeof(drvDispSecPath), "%s/%s", GR_SCT_DISPMODE, pszSecName);
				const char* pszCurDrvName =
					GfParmGetStr(GraphHdle, drvDispSecPath, GR_ATT_CUR_DRV, "");
				if (!strcmp(pszOldDispName, pszCurDrvName)) // Change current driver name.
					GfParmSetStr(GraphHdle, drvDispSecPath, GR_ATT_CUR_DRV, player->dispName());
			} while (!GfParmListSeekNext(GraphHdle, GR_SCT_DISPMODE));
		}
	}
	
    // Human driver params
    GfParmSetStr(PlayerHdle, drvSectionPath, ROB_ATTR_NAME, player->dispName());
    GfParmSetStr(PlayerHdle, drvSectionPath, ROB_ATTR_SNAME, player->dispName());
    GfParmSetStr(PlayerHdle, drvSectionPath, ROB_ATTR_CODE, "PLA");
    GfParmSetStr(PlayerHdle, drvSectionPath, ROB_ATTR_CAR, player->defaultCarName());
    GfParmSetNum(PlayerHdle, drvSectionPath, ROB_ATTR_RACENUM, (char*)NULL, player->raceNumber());
    GfParmSetNum(PlayerHdle, drvSectionPath, ROB_ATTR_RED, (char*)NULL, player->color(0));
    GfParmSetNum(PlayerHdle, drvSectionPath, ROB_ATTR_GREEN, (char*)NULL, player->color(1));
    GfParmSetNum(PlayerHdle, drvSectionPath, ROB_ATTR_BLUE, (char*)NULL, player->color(2));
    GfParmSetStr(PlayerHdle, drvSectionPath, ROB_ATTR_TYPE, ROB_VAL_HUMAN);
    GfParmSetStr(PlayerHdle, drvSectionPath, ROB_ATTR_LEVEL, SkillLevelString[player->skillLevel()]);

    // Driver preferences params
    snprintf(drvSectionPath, sizeof(drvSectionPath), "%s/%s/%u", HM_SECT_PREF, HM_LIST_DRV, index);
    GfParmSetStr(PrefHdle, drvSectionPath, HM_ATT_TRANS, player->gearChangeModeString());
    GfParmSetNum(PrefHdle, drvSectionPath, HM_ATT_NBPITS, (char*)NULL, (tdble)player->nbPitStops());
    GfParmSetStr(PrefHdle, drvSectionPath, HM_ATT_AUTOREVERSE, Yn[player->autoReverse()]);
	#ifdef WEBSERVER
    GfParmSetStr(PrefHdle, drvSectionPath, "WebServerUsername", player->webserverusername());
    GfParmSetStr(PrefHdle, drvSectionPath, "WebServerPassword", player->webserverpassword());
	#endif //WEBSERVER    
    
    
    /* Allow neutral gear in sequential mode if neutral gear command not defined */
    if (player->gearChangeMode() == GEAR_MODE_SEQ
	    && !strcmp(GfParmGetStr(PrefHdle, drvSectionPath, HM_ATT_GEAR_N, "-"), "-"))
        GfParmSetStr(PrefHdle, drvSectionPath, HM_ATT_SEQSHFT_ALLOW_NEUTRAL, HM_VAL_YES);
    else
        GfParmSetStr(PrefHdle, drvSectionPath, HM_ATT_SEQSHFT_ALLOW_NEUTRAL, HM_VAL_NO);

    /* Allow reverse gear in sequential mode if reverse gear command not defined */
    if (player->gearChangeMode() == GEAR_MODE_SEQ
	    && !strcmp(GfParmGetStr(PrefHdle, drvSectionPath, HM_ATT_GEAR_R, "-"), "-"))
        GfParmSetStr(PrefHdle, drvSectionPath, HM_ATT_SEQSHFT_ALLOW_REVERSE, HM_VAL_YES);
    else
        GfParmSetStr(PrefHdle, drvSectionPath, HM_ATT_SEQSHFT_ALLOW_REVERSE, HM_VAL_NO);
    
    /* Release gear lever goes neutral in grid mode if no neutral gear command defined */
    if (player->gearChangeMode() == GEAR_MODE_GRID
		&& !strcmp(GfParmGetStr(PrefHdle, drvSectionPath, HM_ATT_GEAR_N, "-"), "-"))
        GfParmSetStr(PrefHdle, drvSectionPath, HM_ATT_REL_BUT_NEUTRAL, HM_VAL_YES);
    else
        GfParmSetStr(PrefHdle, drvSectionPath, HM_ATT_REL_BUT_NEUTRAL, HM_VAL_NO);
}

/* Create a brand new player into the list, and update players scroll-list and all editable fields*/
static void
onNewPlayer(void * /* dummy */)
{
    unsigned newPlayerIdx, playerIdx;
    char sectionPath[128];
    char driverId[8];
    char newDriverId[8];

    // Insert new player after current (or after last if no current)
    CurrPlayer = PlayersInfo.insert(CurrPlayer + (CurrPlayer == PlayersInfo.end() ? 0 : 1),
									new tPlayerInfo(HumanDriverModuleName));

    // Get new current player index (= identification number in params).
    newPlayerIdx = (unsigned)(CurrPlayer - PlayersInfo.begin()) + 1;

    // Update preferences and drivers params (rename those after, add new).
    snprintf(sectionPath, sizeof(sectionPath), "%s/%s", HM_SECT_PREF, HM_LIST_DRV);
    for (playerIdx = PlayersInfo.size() - 1; playerIdx >= newPlayerIdx; playerIdx--) {
        snprintf(driverId, sizeof(driverId), "%u", playerIdx);
		snprintf(newDriverId, sizeof(newDriverId), "%u", playerIdx+1);
		GfParmListRenameElt(PrefHdle, sectionPath, driverId, newDriverId);
    }

    snprintf(sectionPath, sizeof(sectionPath), "%s/%s", ROB_SECT_ROBOTS, ROB_LIST_INDEX);
    for (playerIdx = PlayersInfo.size() - 1; playerIdx >= newPlayerIdx; playerIdx--) {
        snprintf(driverId, sizeof(driverId), "%u", playerIdx);
		snprintf(newDriverId, sizeof(newDriverId), "%u", playerIdx+1);
		GfParmListRenameElt(PlayerHdle, sectionPath, driverId, newDriverId);
    }

    PutPlayerSettings(newPlayerIdx);

    // Update GUI.
    refreshEditVal();
    UpdtScrollList();
}

/* Create a new player into the list by copying the currently selected one, and update players scroll-list and all editable fields (copy also the control settings) */
static void
onCopyPlayer(void * /* dummy */)
{
    unsigned curPlayerIdx, newPlayerIdx;
    unsigned playerIdx;
    char sectionPath[128];
    char driverId[8];
    char newDriverId[8];

    tGearChangeMode gearChange;
    
    if (CurrPlayer != PlayersInfo.end()) {

        // Get current (source) player index (= identification number in params).
        curPlayerIdx = (unsigned)(CurrPlayer - PlayersInfo.begin()) + 1;

		// Save current player gear change mode
		gearChange = (*CurrPlayer)->gearChangeMode();

		// Get current player control settings.
		ControlGetSettings(PrefHdle, curPlayerIdx);

		// Insert new player after current
		CurrPlayer = PlayersInfo.insert(CurrPlayer + 1, new tPlayerInfo(**CurrPlayer));

        // Get new (copy) player index (= identification number in params).
		newPlayerIdx = (unsigned)(CurrPlayer - PlayersInfo.begin()) + 1;

		// Update preferences and drivers params (rename those after, add new).
		snprintf(sectionPath, sizeof(sectionPath), "%s/%s", HM_SECT_PREF, HM_LIST_DRV);
		for (playerIdx = PlayersInfo.size() - 1; playerIdx >= newPlayerIdx; playerIdx--) {
			snprintf(driverId, sizeof(driverId), "%u", playerIdx);
			snprintf(newDriverId, sizeof(newDriverId), "%u", playerIdx+1);
			GfParmListRenameElt(PrefHdle, sectionPath, driverId, newDriverId);
		}
    
		snprintf(sectionPath, sizeof(sectionPath), "%s/%s", ROB_SECT_ROBOTS, ROB_LIST_INDEX);
		for (playerIdx = PlayersInfo.size() - 1; playerIdx >= newPlayerIdx; playerIdx--) {
			snprintf(driverId, sizeof(driverId), "%u", playerIdx);
			snprintf(newDriverId, sizeof(newDriverId), "%u", playerIdx+1);
			GfParmListRenameElt(PlayerHdle, sectionPath, driverId, newDriverId);
		}

		PutPlayerSettings(newPlayerIdx);

		// Set new player control settings (copy of previous current one's).
		ControlPutSettings(PrefHdle, newPlayerIdx, gearChange);

		// Update GUI.
        refreshEditVal();
        UpdtScrollList();
    }
}

/* Remove the selected player from the list, and update players scroll-list and all editable fields*/
static void
onDeletePlayer(void * /* dummy */)
{
    int delPlayerIdx;
    unsigned int playerIdx;
    char sectionPath[128];
    char driverId[8];
    char newDriverId[8];
    
    if (CurrPlayer != PlayersInfo.end()) {

        // Get current player index (= identification number in params).
        delPlayerIdx = CurrPlayer - PlayersInfo.begin() + 1;

		// Remove current player from list. Select next if any.
		delete *CurrPlayer;
        CurrPlayer = PlayersInfo.erase(CurrPlayer);

		// Update preferences and drivers params.
		snprintf(sectionPath, sizeof(sectionPath), "%s/%s", HM_SECT_PREF, HM_LIST_DRV);
		snprintf(driverId, sizeof(driverId), "%d", delPlayerIdx);
		if (!GfParmListRemoveElt(PrefHdle, sectionPath, driverId)) {
			for (playerIdx = delPlayerIdx; playerIdx <= PlayersInfo.size(); playerIdx++) {
				snprintf(driverId, sizeof(driverId), "%u", playerIdx+1);
				snprintf(newDriverId, sizeof(newDriverId), "%u", playerIdx);
				GfParmListRenameElt(PrefHdle, sectionPath, driverId, newDriverId);
			}
		}

		snprintf(sectionPath, sizeof(sectionPath), "%s/%s", ROB_SECT_ROBOTS, ROB_LIST_INDEX);
		snprintf(driverId, sizeof(driverId), "%d", delPlayerIdx);
		if (!GfParmListRemoveElt(PlayerHdle, sectionPath, driverId)) {
			for (playerIdx = delPlayerIdx; playerIdx <= PlayersInfo.size(); playerIdx++) {
				snprintf(driverId, sizeof(driverId), "%u", playerIdx+1);
				snprintf(newDriverId, sizeof(newDriverId), "%u", playerIdx);
				GfParmListRenameElt(PlayerHdle, sectionPath, driverId, newDriverId);
			}
		}

		// Update GUI.
		refreshEditVal();
		UpdtScrollList();
    }
}

/* Activate the control config screen if a player is selected */
static void
onConfControls(void * /* dummy */ )
{
    unsigned curPlayerIdx;
    
    if (CurrPlayer != PlayersInfo.end()) {

        ReloadValues = 0;

        curPlayerIdx = (unsigned)(CurrPlayer - PlayersInfo.begin()) + 1;
		GfuiScreenActivate(ControlMenuInit(ScrHandle, PrefHdle, curPlayerIdx, (*CurrPlayer)->gearChangeMode(), 0));
    }
}

/* Load human driver (= player) info list (PlayersInfo) from preferences and human drivers files ;
   load associated scroll list */
static int
GenPlayerList(void)
{
    char sstring[128];
    int i;
    int j;
    const char *driver;
    const char *defaultCar;
    int skilllevel;
    const char *str;
    int racenumber;
    float color[4];
    
    /* Reset players list */
    tPlayerInfoList::iterator playerIter;
    for (playerIter = PlayersInfo.begin(); playerIter != PlayersInfo.end(); playerIter++)
        delete *playerIter;
    PlayersInfo.clear();

    /* Load players settings from human.xml file *//*was meant: preferences.xml file?*/
    snprintf(buf, sizeof(buf), "%s%s", GfLocalDir(), HM_DRV_FILE);
    PlayerHdle = GfParmReadFile(buf, GFPARM_RMODE_REREAD);
    if (PlayerHdle == NULL) {
        return -1;
    }

    for (i = 0; ; i++) {
        snprintf(sstring, sizeof(sstring), "%s/%s/%d", ROB_SECT_ROBOTS, ROB_LIST_INDEX, i+1);
		driver = GfParmGetStr(PlayerHdle, sstring, ROB_ATTR_NAME, "");
		if (strlen(driver) == 0) {
			break; // Exit at end of driver list.
		} else {
			str = GfParmGetStr(PlayerHdle, sstring, ROB_ATTR_LEVEL, SkillLevelString[0]);
			skilllevel = 0;
			for(j = 0; j < NbSkillLevels; j++) {
				if (strcmp(SkillLevelString[j], str) == 0) {
					skilllevel = j;
					break;
				}
			}
			defaultCar  = GfParmGetStr(PlayerHdle, sstring, ROB_ATTR_CAR, 0);
			racenumber  = (int)GfParmGetNum(PlayerHdle, sstring, ROB_ATTR_RACENUM, (char*)NULL, 0);
			color[0]    = (float)GfParmGetNum(PlayerHdle, sstring, ROB_ATTR_RED, (char*)NULL, 1.0);
			color[1]    = (float)GfParmGetNum(PlayerHdle, sstring, ROB_ATTR_GREEN, (char*)NULL, 1.0);;
			color[2]    = (float)GfParmGetNum(PlayerHdle, sstring, ROB_ATTR_BLUE, (char*)NULL, 0.5);;
			color[3]    = 1.0;
			PlayersInfo.push_back(new tPlayerInfo(HumanDriverModuleName, // Driver module name
												  driver,  // Player (display) name
												  defaultCar, // Default car name.
												  racenumber, // Race number
												  skilllevel, // skill level
												  color));  // Colors
 
		}
    }

	/* No currently selected player */
    CurrPlayer = PlayersInfo.end();

	/* Update scroll-list from PlayersInfo */
    UpdtScrollList();

    /* Load players settings from human/preferences.xml file*/
    snprintf(buf, sizeof(buf), "%s%s", GfLocalDir(), HM_PREF_FILE);
    PrefHdle = GfParmReadFile(buf, GFPARM_RMODE_REREAD);
    if (!PrefHdle) {
        return -1;
    }

    for (i = 0; i < (int)PlayersInfo.size(); i++) {
        snprintf(sstring, sizeof(sstring), "%s/%s/%d", HM_SECT_PREF, HM_LIST_DRV, i+1);
		str = GfParmGetStr(PrefHdle, sstring, HM_ATT_TRANS, HM_VAL_AUTO);
		if (!strcmp(str, HM_VAL_AUTO)) {
			PlayersInfo[i]->setGearChangeMode(GEAR_MODE_AUTO);
		} else if (!strcmp(str, HM_VAL_GRID)) {
			PlayersInfo[i]->setGearChangeMode(GEAR_MODE_GRID);
		} else if (!strcmp(str, HM_VAL_HBOX)) {
			PlayersInfo[i]->setGearChangeMode(GEAR_MODE_HBOX);
		} else {
			PlayersInfo[i]->setGearChangeMode(GEAR_MODE_SEQ);
		} /* Note: Deprecated "manual" value (after R1.3.0) smoothly converted to "sequential" (backward compatibility) */
		PlayersInfo[i]->setNbPitStops(GfParmGetNum(PrefHdle, sstring, HM_ATT_NBPITS, (char*)NULL, 0));
		if (!strcmp(GfParmGetStr(PrefHdle, sstring, HM_ATT_AUTOREVERSE, Yn[0]), Yn[0])) {
			PlayersInfo[i]->setAutoReverse(0);
		} else {
			PlayersInfo[i]->setAutoReverse(1);
		}

		#ifdef WEBSERVER
		str = GfParmGetStr(PrefHdle, sstring, "WebServerUsername", 0);
		PlayersInfo[i]->setWebserverusername(str);

		str = GfParmGetStr(PrefHdle, sstring, "WebServerPassword", 0);
		PlayersInfo[i]->setWebserverpassword(str);
		#endif //WEBSERVER


    }

    return 0;
}

/* Quit driver config menu (changes must have been saved before if needed) */
static void
onQuitPlayerConfig(void * /* dummy */)
{
    // Next time, we'll have to reload the player settings from the files.
    ReloadValues = 1;

    // Reset player list
    tPlayerInfoList::iterator playerIter;
    for (playerIter = PlayersInfo.begin(); playerIter != PlayersInfo.end(); playerIter++)
        delete *playerIter;
    PlayersInfo.clear();

    // Close driver and preference params files.
    GfParmReleaseHandle(PlayerHdle);
    PlayerHdle = 0;

    GfParmReleaseHandle(PrefHdle);
    PrefHdle = 0;

    // Activate caller screen/menu
    GfuiScreenActivate(PrevScrHandle);
}

/* Save players info (from PlayersInfo array) to the human drivers and preferences XML files,
   as well as graphics settings for players if they changed */
static void
onSavePlayerList(void * /* dummy */)
{
    int		index;

    if (!PlayerHdle || !PrefHdle) {
		return;
    }

    // Force current edit to loose focus (if one has it) and update associated variable.
    GfuiUnSelectCurrent();

    for (index = 1; index <= (int)PlayersInfo.size(); index++) {
        PutPlayerSettings(index);
    }

    GfParmWriteFile(NULL, PlayerHdle, HumanDriverModuleName);
    GfParmWriteFile(NULL, PrefHdle, "preferences");
	if (GraphHdle) // Write graphic params file if needed.
		GfParmWriteFile(NULL, GraphHdle, "Graph");

	// Temporary, as long as this menu has not been ported to using the tgfdata layer
	// in order to access drivers data (rather that directly read/writing the XML files).
	// Reload the drivers from disk.
	GfDrivers::self()->reload();

    onQuitPlayerConfig(0 /* dummy */);
}

static void
onChangeName(void * /* dummy */)
{
    char	*val;

    if (CurrPlayer != PlayersInfo.end()) {
        val = GfuiEditboxGetString(ScrHandle, NameEditId);

        // Remove leading spaces (#587)
        std::string strIn(val);
        size_t startpos = strIn.find_first_not_of(" \t"); // Find the first character position after excluding leading blank spaces
        size_t endpos = strIn.find_last_not_of(" \t");  // Find last non-whitespace char position
        if (startpos != std::string::npos && endpos != std::string::npos) {
            strIn = strIn.substr(startpos, endpos - startpos + 1);
        } else {
            strIn.assign(NoPlayer); // If it was all whitespace, assign default
        }

        (*CurrPlayer)->setDispName((strIn == PlayerNamePrompt || strIn == NoPlayer) ? NoPlayer : strIn.c_str());
    }

    UpdtScrollList();
}

#ifdef WEBSERVER
static void
onChangeWebserverusername(void * /* dummy */)
{
    char	*val;

    if (CurrPlayer != PlayersInfo.end()) {
        val = GfuiEditboxGetString(ScrHandle, WebUsernameEditId);

        // Remove leading spaces (#587)
        std::string strIn(val);
        size_t startpos = strIn.find_first_not_of(" \t"); // Find the first character position after excluding leading blank spaces
        size_t endpos = strIn.find_last_not_of(" \t");  // Find last non-whitespace char position
        if (startpos != std::string::npos && endpos != std::string::npos) {
            strIn = strIn.substr(startpos, endpos - startpos + 1);
        } else {
            strIn.assign(DefaultWebserverusername); // If it was all whitespace, assign default
        }
        (*CurrPlayer)->setWebserverusername(strIn.c_str());

    }

    UpdtScrollList();
}

static void
onChangeWebserverpassword(void * /* dummy */)
{
    char	*val;

    if (CurrPlayer != PlayersInfo.end()) {
        val = GfuiEditboxGetString(ScrHandle, WebPasswordEditId);

        // Remove leading spaces (#587)
        std::string strIn(val);
        size_t startpos = strIn.find_first_not_of(" \t"); // Find the first character position after excluding leading blank spaces
        size_t endpos = strIn.find_last_not_of(" \t");  // Find last non-whitespace char position
        if (startpos != std::string::npos && endpos != std::string::npos) {
            strIn = strIn.substr(startpos, endpos - startpos + 1);
        } else {
            strIn.assign(DefaultWebserverpassword); // If it was all whitespace, assign default
        }
        (*CurrPlayer)->setWebserverpassword(strIn.c_str());
    }

    UpdtScrollList();
}

static void onWebserverLoginTest(void * /* dummy */)
{
	if (CurrPlayer != PlayersInfo.end()) {

		std::string username = (*CurrPlayer)->webserverusername();
		std::string password = (*CurrPlayer)->webserverpassword();
		
		extern TGFCLIENT_API WebServer webServer;
		webServer.sendLogin(username.c_str(),password.c_str());

		//request a redisplay
	    GfuiApp().eventLoop().postRedisplay();
	}
}
#endif //WEBSERVER

//#641 New player name should get empty when clicking on it
// In the Player Configuration menu, when you create
// a new player, his name is set to "-- Enter name --",
// as a prompt for changing it.
// But when you want to change it, you have to first erase
// this prompt string all.
// Would be great if the prompt string disappear when clicked.
// Note(kilo): this way if one clicks the player name editbox
// and leaves without entering anything, the new player will be called
// "--No Player--" and stored with that name.
static void
onActivateName(void * /* dummy */)
{
    std::string strIn = GfuiEditboxGetString(ScrHandle, NameEditId);
    if (strIn == PlayerNamePrompt) {
        (*CurrPlayer)->setDispName("");
        GfuiEditboxSetString(ScrHandle, NameEditId, (*CurrPlayer)->dispName());
    }

    UpdtScrollList();
}

static void
onChangeNum(void * /* dummy */)
{
    char	*val;
    
    if (CurrPlayer != PlayersInfo.end()) {
		val = GfuiEditboxGetString(ScrHandle, RaceNumEditId);
		(*CurrPlayer)->setRaceNumber((int)strtol(val, (char **)NULL, 0));
		snprintf(buf, sizeof(buf), "%d", (*CurrPlayer)->raceNumber());
		GfuiEditboxSetString(ScrHandle, RaceNumEditId, buf);
    }
}

static void
onChangePits(void * /* dummy */)
{
    char	*val;
    
    if (CurrPlayer != PlayersInfo.end()) {    
        val = GfuiEditboxGetString(ScrHandle, PitsEditId);
        (*CurrPlayer)->setNbPitStops((int)strtol(val, (char **)NULL, 0));
		snprintf(buf, sizeof(buf), "%d", (*CurrPlayer)->nbPitStops());
		GfuiEditboxSetString(ScrHandle, PitsEditId, buf);
    }
}

static void
onChangeLevel(void *vp)
{
    if (CurrPlayer == PlayersInfo.end()) {
		return;
    }

    int skillLevel = (*CurrPlayer)->skillLevel();
    if (vp == 0) {
		skillLevel--;
		if (skillLevel < 0) {
			skillLevel = NbSkillLevels - 1;
		}
    } else {
		skillLevel++;
		if (skillLevel == NbSkillLevels) {
			skillLevel = 0;
		}
    }
    (*CurrPlayer)->setSkillLevel(skillLevel);

    refreshEditVal();
}

static void
onChangeReverse(void *vdelta)
{
    const long delta = (long)vdelta;
    
    if (CurrPlayer == PlayersInfo.end()) {
		return;
    }

    int autoReverse = (*CurrPlayer)->autoReverse();
    autoReverse += (int)delta;
    if (autoReverse < 0) {
		autoReverse = 1;
    } else if (autoReverse > 1) {
		autoReverse = 0;
    }
    (*CurrPlayer)->setAutoReverse(autoReverse);

    refreshEditVal();
}


/* Gear change mode change callback */
static void
onChangeGearChange(void *vp)
{
    if (CurrPlayer == PlayersInfo.end()) {
		return;
    }
    tGearChangeMode gearChangeMode = (*CurrPlayer)->gearChangeMode();
    if (vp == 0) {
		if (gearChangeMode == GEAR_MODE_AUTO) {
			gearChangeMode = GEAR_MODE_HBOX;
		} else if (gearChangeMode == GEAR_MODE_SEQ) {
			gearChangeMode = GEAR_MODE_AUTO;
		} else if (gearChangeMode == GEAR_MODE_HBOX) {
			gearChangeMode = GEAR_MODE_GRID;
		}
		else {
			gearChangeMode = GEAR_MODE_SEQ;
		}
    } else {
		if (gearChangeMode == GEAR_MODE_AUTO) {
			gearChangeMode = GEAR_MODE_SEQ;
		} else if (gearChangeMode == GEAR_MODE_SEQ) {
			gearChangeMode = GEAR_MODE_GRID;
		} else if (gearChangeMode == GEAR_MODE_GRID) {
			gearChangeMode = GEAR_MODE_HBOX;
		}
		else {
			gearChangeMode = GEAR_MODE_AUTO;
		}
    }
    (*CurrPlayer)->setGearChangeMode(gearChangeMode);

    refreshEditVal();
}

static void
onActivate(void * /* dummy */)
{
    if (ReloadValues) {
	  
		/* Load players settings */
		GenPlayerList();

		/* Initialize current player and select it */
		CurrPlayer = PlayersInfo.begin();
		GfuiScrollListSetSelectedElement(ScrHandle, ScrollList, 0);
    }
	
    /* Display editable fields values */
    refreshEditVal();
}

void *
PlayerConfigMenuInit(void *prevMenu)
{
    /* Save previous screen handle for exit time */ 
    PrevScrHandle = prevMenu;

    /* Screen already created : nothing more to do */
    if (ScrHandle) {
		return ScrHandle;
    }

    /* Create the screen, load menu XML descriptor and create static controls */
    ScrHandle = GfuiScreenCreate((float*)NULL, NULL, onActivate, NULL, (tfuiCallback)NULL, 1);
    void *param = GfuiMenuLoad("playerconfigmenu.xml");
    GfuiMenuCreateStaticControls(ScrHandle, param);

    /* Player scroll list */
    ScrollList = GfuiMenuCreateScrollListControl(ScrHandle, param, "playerscrolllist", NULL, onSelect);

    /* New, copy and delete player buttons */
    GfuiMenuCreateButtonControl(ScrHandle, param, "new", NULL, onNewPlayer);
    GfuiMenuCreateButtonControl(ScrHandle, param, "copy", NULL, onCopyPlayer);
    GfuiMenuCreateButtonControl(ScrHandle, param, "delete", NULL, onDeletePlayer);

    /* Access to control screen button */
    GfuiMenuCreateButtonControl(ScrHandle, param, "controls", NULL, onConfControls);

    /* Player name editbox */
    NameEditId = GfuiMenuCreateEditControl(ScrHandle, param, "nameedit", NULL, onActivateName, onChangeName);

    /* Player skill level "combobox" (left arrow, label, right arrow) */
    GfuiMenuCreateButtonControl(ScrHandle, param, "skillleftarrow", (void*)0, onChangeLevel);
    GfuiMenuCreateButtonControl(ScrHandle, param, "skillrightarrow", (void*)1, onChangeLevel);
    SkillEditId = GfuiMenuCreateLabelControl(ScrHandle, param, "skilltext");

    /* Races and pits numbers editboxes (Must they really stay here ?) */
    /* TODO: definitely should be moved from here to pre-race menus. kilo */
    RaceNumEditId = GfuiMenuCreateEditControl(ScrHandle, param, "racenumedit", NULL, NULL, onChangeNum);
    PitsEditId = GfuiMenuCreateEditControl(ScrHandle, param, "pitstopedit", NULL, NULL, onChangePits);
    
    /* Gear changing mode and associated "combobox" (left arrow, label, right arrow) */
    GfuiMenuCreateButtonControl(ScrHandle, param, "gearleftarrow", (void*)0, onChangeGearChange);
    GfuiMenuCreateButtonControl(ScrHandle, param, "gearrightarrow", (void*)1, onChangeGearChange);
    GearChangeEditId = GfuiMenuCreateLabelControl(ScrHandle, param, "geartext");

    /* Gear changing auto-reverse flag and associated "combobox" (left arrow, label, right arrow) */
    AutoReverseLabelId = GfuiMenuCreateLabelControl(ScrHandle, param, "autoreversetext");
    AutoReverseLeftId = GfuiMenuCreateButtonControl(ScrHandle, param, "autoleftarrow", (void*)-1, onChangeReverse);
    AutoReverseRightId = GfuiMenuCreateButtonControl(ScrHandle, param, "autorightarrow", (void*)1, onChangeReverse);
    AutoReverseEditId = GfuiMenuCreateLabelControl(ScrHandle, param, "autotext");

    // Accept and Cancel buttons.
    GfuiMenuCreateButtonControl(ScrHandle, param, "ApplyButton", NULL, onSavePlayerList);
    GfuiMenuCreateButtonControl(ScrHandle, param, "CancelButton", NULL, onQuitPlayerConfig);

	#ifdef WEBSERVER
    /* Web username and password editbox */
    WebUsernameEditId = GfuiMenuCreateEditControl(ScrHandle, param, "webusernameedit", NULL, NULL, onChangeWebserverusername);
    WebPasswordEditId = GfuiMenuCreateEditControl(ScrHandle, param, "webpasswordedit", NULL, NULL, onChangeWebserverpassword);

    /* Web test button */
    GfuiMenuCreateButtonControl(ScrHandle, param, "weblogintest", NULL, onWebserverLoginTest);
	#endif //WEBSERVER


    // Close menu XML descriptor.
    GfParmReleaseHandle(param);
    
    // Register keyboard shortcuts.
    GfuiAddKey(ScrHandle, GFUIK_RETURN, "Accept and save changes", NULL, onSavePlayerList, NULL);
    GfuiAddKey(ScrHandle, GFUIK_ESCAPE, "Cancel changes", NULL, onQuitPlayerConfig, NULL);
    GfuiAddKey(ScrHandle, GFUIK_F1, "Help", ScrHandle, GfuiHelpScreen, NULL);
    GfuiAddKey(ScrHandle, GFUIK_F12, "Screen-Shot", NULL, GfuiScreenShot, NULL);
	//     GfuiAddKey(ScrHandle, GFUIK_UP, "Previous Car", (void*)0, ChangeCar, NULL);
	//     GfuiAddKey(ScrHandle, GFUIK_DOWN, "Next Car", (void*)1, ChangeCar, NULL);
	//     GfuiAddKey(ScrHandle, GFUIK_PAGEUP, "Previous Car Category", (void*)0, ChangeCat, NULL);
	//     GfuiAddKey(ScrHandle, GFUIK_PAGEDOWN, "Next Car Category", (void*)1, ChangeCat, NULL);

    return ScrHandle;
}

