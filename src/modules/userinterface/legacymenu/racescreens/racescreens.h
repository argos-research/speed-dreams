/***************************************************************************

    file                 : racescreens.h
    created              : Sat Mar 18 23:33:01 CET 2000
    copyright            : (C) 2000 by Eric Espie
    email                : torcs@free.fr
    version              : $Id: racescreens.h 6084 2015-08-21 00:07:15Z beaglejoe $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
 
/**
    @defgroup	racescreens	Race menus.
    The race manager menus.
*/

#ifndef __RACESCREENS_H__
#define __RACESCREENS_H__

#include <raceman.h>

#include <itrackloader.h>

#include <tgfclient.h> // tfuiCallback

class GfRace;

typedef struct RmTrackSelect
{
	GfRace      *pRace; /* The race to update */
    void        *prevScreen;	/* Race manager screen to go back */
    void        *nextScreen;	/* Race manager screen to go after select */
    ITrackLoader	*piTrackLoader;	/* Track loader */
} tRmTrackSelect;

typedef struct RmDriverSelect
{
	GfRace      *pRace; /* The race to update */
    void        *prevScreen;	/* Race manager screen to go back */
    void        *nextScreen;	/* Race manager screen to go after select */
} tRmDriverSelect;

typedef struct RmRaceParam
{
	GfRace          *pRace; /* The race to update */
    std::string		session; /* The race session to configure (RM_VAL_ANYRACE for all of them) */
    void        	*prevScreen;	/* Race manager screen to go back */
    void        	*nextScreen;	/* Race manager screen to go after select */
} tRmRaceParam;

typedef void (*tfSelectFile) (const char *);

enum RmFileSelectMode { RmFSModeLoad, RmFSModeSave };

typedef struct RmFileSelect
{
    std::string	title;
    std::string	dirPath;
	std::string namePrefix;
	std::string nameSuffix;
    void* prevScreen;
    tfSelectFile select;
	RmFileSelectMode mode;
} tRmFileSelect;

extern void RmTrackSelect(void * /* vs */);

extern void RmDriversSelect(void * /* vs */);

extern void RmPitMenuStart(tCarElt * /* car */, tSituation * /* situation */, tfuiCallback /* callback */);

extern void RmLoadingScreenStart(const char * /* text */, const char * /* bgimg */);
extern void RmLoadingScreenSetText(const char * /* text */);
extern void RmLoadingScreenShutdown();

extern void RmOptimizationScreenStart(const char * /* text */, const char * /* bgimg */);
extern void RmOptimizationScreenSetText(const char * /* text */);
extern void RmOptimizationScreenSetParameterText(int /* n */, char** /* label */, char** /* value */, char** /* range */);
extern void RmOptimizationScreenSetStatusText(int, int, double, double, double, double);
extern void RmOptimizationScreenShutdown();

extern void RmGameScreen();

extern void RmShowResults(void * /* prevHdle */, tRmInfo * /* info */);

extern void* RmBackToRaceHookInit();
extern void RmStopRaceMenu();
extern void RmStopRaceMenuShutdown();

extern void RmStartRaceMenu();
extern void RmStartRaceMenuShutdown();

extern void RmRaceParamsMenu(void* vrp);

extern void RmShowStandings(void* prevHdle, tRmInfo *info, int start = 0);

extern void* RmFileSelect(void* vs);

// From racemanmenus.
extern void RmRacemanMenu();
extern void RmNextEventMenu();
extern void RmConfigureRace(void*  /* dummy */);
extern void RmSetRacemanMenuHandle(void*  handle);

extern void* RmGetRacemanMenuHandle();

extern void RmConfigRunState(bool bStart = false);

// From raceselectmenu.
extern void* RmRaceSelectInit(void* precMenu);

// From racerunningmenus.
extern void* RmScreenInit();
extern void RmScreenShutdown();
extern void* RmInitReUpdateStateHook();
extern void RmShutdownReUpdateStateHook();
extern bool RmCheckPitRequest();

extern void* RmResScreenInit();
extern void RmResScreenShutdown();
extern void RmResScreenSetTitles(const char *pszTitle, const char *pszSubTitle);
extern void RmResScreenSetHeader(const char *pszHeader);
extern void RmResScreenAddText(const char *pszText);
extern void RmResScreenSetText(const char *pszText, int nRowIndex, int nColorIndex);
extern void RmResScreenRemoveText(int nRowIndex);
//extern void RmResShowCont(); // Never used : remove ?
extern int  RmResGetRows();
extern void RmResEraseScreen();

extern void RmAddPreRacePauseItems();
extern void RmAddCooldownItems();

// From networkingmenu.
extern void RmNetworkClientMenu(void* pVoid);
extern void RmNetworkMenu(void* /* dummy */);
extern void RmNetworkHostMenu(void* /* dummy */);

// The Race Select menu.
extern void *RmRaceSelectMenuHandle;

// Progressive simulation time modifier, for more user-friendly resuming
// a race from the Stop Race menu (progressively accelerates time from a low factor).
class RmProgressiveTimeModifier
{
 public:

	//! Constructor.
	RmProgressiveTimeModifier();

	//! Reset state as if just born.
	void reset();

	//! Start the ramp up.
	void start();

	//! Simulation step : aimed at being called at each display loop.
	void execute();

 private:

	//! For when the ramp up is over.
	void terminate();

 private:
	
	// Should we run the manager at next simu step ?
	bool _bExecRunning;
	
	// Log the manager activation time (real value will be stored on start())
	double _fExecStartTime;

	// Total duration of the "progressive acceleration of time" process.
	double _fWholeTimeLapse;

	// Log the last time acceleration change (needed to calculate the time restore factor)
	double _fOldTimeMultiplier;

	// Log the integrated time acceleration change (needed when multiple start without terminate)
	double _fResetterTimeMultiplier;
	
 private:
	
	// Config: Set how much time will take to restore to normal speed (after the delay)
	static const double _sfTimeMultiplier;

	// Config: Set how much we wait before starting to apply the time acceleration
	static const double _sfDelay;
	
	// Config: Set how much the time will be initially changed (as a fraction of the current time)
	static const double _sfTimeLapse;

};

#endif /* __RACESCREENS_H__ */

