/***************************************************************************

    file        : legacymenu.h
    copyright   : (C) 2011 by Jean-Philippe Meuret
    email       : pouillot@users.sourceforge.net
    version     : $Id: legacymenu.h 6084 2015-08-21 00:07:15Z beaglejoe $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
 
/** @file    
    		The "legacy menu" user interface module
    @version    $Id: legacymenu.h 6084 2015-08-21 00:07:15Z beaglejoe $
*/

#ifndef _LEGACYMENU_H_
#define _LEGACYMENU_H_

#include <iuserinterface.h>
#include <igraphicsengine.h>
#include <isoundengine.h>

#include <tgf.hpp>

class IGraphicsEngine;
class ISoundEngine;
struct Situation;


// DLL exported symbols declarator for Windows.
#ifdef WIN32
# ifdef LEGACYMENU_DLL
#  define LEGACYMENU_API __declspec(dllexport)
# else
#  define LEGACYMENU_API __declspec(dllimport)
# endif
#else
# define LEGACYMENU_API
#endif


// The C interface of the module.
extern "C" int LEGACYMENU_API openGfModule(const char* pszShLibName, void* hShLibHandle);
extern "C" int LEGACYMENU_API closeGfModule();

// The module main class (inherits GfModule, and implements IUserInterface).
class LEGACYMENU_API LegacyMenu : public GfModule, public IUserInterface
{
// Implementation of IUserInterface.
public:

	//! Activation of the user interface (splash if any, main menu ...).
	virtual bool activate();
	
	//! Request exit of the event loop.
	virtual void quit();
	
	//! Termination of the user interface.
	virtual void shutdown();

	// Race state change notifications.
	virtual void onRaceConfiguring();
	virtual void onRaceEventInitializing();
	virtual bool onRaceEventStarting(bool careerNonHumanGroup);
	virtual void onRaceInitializing();
	virtual bool onRaceStarting();
	virtual void onRaceLoadingDrivers();
	virtual void onRaceDriversLoaded();
	virtual void onRaceSimulationReady();
	virtual void onRaceStarted();
	virtual void onRaceResuming();
	virtual void onLapCompleted(int nLapIndex);
	virtual void onRaceInterrupted();
	virtual void onRaceFinishing();
	virtual bool onRaceFinished(bool bEndOfSession);
	virtual void onRaceEventFinishing();
	virtual bool onRaceEventFinished(bool bMultiEvent, bool careerNonHumanGroup);
	virtual void onOptimizationInitializing();

	virtual bool onRaceStartingPaused();
	virtual bool onRaceCooldownStarting();
	
	// Loading messages management.
	virtual void addLoadingMessage(const char* pszText);

	// Optimization messages management.
	virtual void addOptimizationMessage(const char* pszText);
    virtual void addOptimizationParameterMessage(int n, char** Labels, char** Values, char** Ranges);
    virtual void addOptimizationStatusMessage(int LoopsDone, int LoopsRemaining, double VariationScale, double InitialLapTime,  double TotalLapTime,  double BestLapTime);

	// Blind-race results table management.
	virtual void setResultsTableTitles(const char* pszTitle, const char* pszSubTitle);
	virtual void setResultsTableHeader(const char* pszHeader);
	virtual void addResultsTableRow(const char* pszText);
	virtual void setResultsTableRow(int nIndex, const char* pszText, bool bHighlight = false);
	virtual void removeResultsTableRow(int nIndex);
	virtual void eraseResultsTable();
	virtual int  getResultsTableRowCount() const;

	// Setter for the race engine.
	virtual void setRaceEngine(IRaceEngine& raceEngine);

// Other services.
public:

	// Accessor to the singleton.
	static LegacyMenu& self();

	//! Accessor to the race engine.
	IRaceEngine& raceEngine();

	// Graphics engine control.
	void redrawGraphicsView(struct Situation* pSituation);
	void shutdownGraphics(bool bUnloadModule = true);

	// Loading screen management.
	void activateLoadingScreen();
	void shutdownLoadingScreen();

	// Optimization screen management.
	void activateOptimizationScreen();
	void shutdownOptimizationScreen();

	//! Game screen management.
	void activateGameScreen();
	
	//! Accessor to the graphics engine.
	IGraphicsEngine* graphicsEngine();

	//! Accessor to the sound engine.
	ISoundEngine* soundEngine();

 protected:

	//! Protected constructor to avoid instanciation outside (but friends).
	LegacyMenu(const std::string& strShLibName, void* hShLibHandle);
	
	//! Make the C interface functions nearly member functions.
	friend int openGfModule(const char* pszShLibName, void* hShLibHandle);
	friend int closeGfModule();

	//! Graphics engine control.
	bool initializeGraphics();
	bool initializeSound();
	bool loadTrackGraphics(struct Track* pTrack);
	bool loadCarsGraphics(struct Situation* pSituation);
	bool setupGraphicsView();
	void shutdownGraphicsView();
	void unloadCarsGraphics();
	void unloadTrackGraphics();
	void shutdownSound(); 

	//! Load stuff in the background of the splash screen (menus, XML data, ...).
	static bool backLoad();

	//! Activate the main menu.
	static bool activateMainMenu();
	
	//! Direct race startup (race specified in command line args).
	static bool startRace();

	//! Start the standings menu.
	void showStandings();

 protected:

	//! The singleton.
	static LegacyMenu* _pSelf;

	//! The race engine.
	IRaceEngine* _piRaceEngine;

	//! The graphics engine.
	IGraphicsEngine* _piGraphicsEngine;
        
        //! The sound engine.
	ISoundEngine* _piSoundEngine;

	//! The "Race Engine update state" hook (a GfuiScreenActivate'able object).
	void* _hscrReUpdateStateHook;
	
	//! The game screen.
	void* _hscrGame;

	//! The graphics state.
	enum { eTrackLoaded = 0x1, eCarsLoaded = 0x2, eViewSetup = 0x4 };
	unsigned _bfGraphicsState;
};

// Shortcut to the race engine.
inline extern IRaceEngine& LmRaceEngine()
{
	return LegacyMenu::self().raceEngine();
}
				  
#endif /* _LEGACYMENU_H_ */ 
