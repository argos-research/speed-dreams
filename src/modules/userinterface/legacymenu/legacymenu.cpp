/***************************************************************************

    file                 : legacymenu.cpp
    copyright            : (C) 2011 by Jean-Philippe Meuret                        
    email                : pouillot@users.sourceforge.net   
    version              : $Id: legacymenu.cpp 6097 2015-08-30 23:12:09Z beaglejoe $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <string>
#include <sstream>

#include <iraceengine.h>
#include <igraphicsengine.h>
#include <isoundengine.h>



#include <tgf.hpp>
#include <portability.h>
#include <tgfclient.h>

#include <race.h>
#include <racemanagers.h>

#include "splash.h"
#include "mainmenu.h"
#include "exitmenu.h"
#include "racescreens.h"

#include "legacymenu.h"
#include "displayconfig.h"


// The LegacyMenu singleton.
LegacyMenu* LegacyMenu::_pSelf = 0;

int openGfModule(const char* pszShLibName, void* hShLibHandle)
{
    // Instanciate the (only) module instance.
    LegacyMenu::_pSelf = new LegacyMenu(pszShLibName, hShLibHandle);

    // Register it to the GfModule module manager if OK.
    if (LegacyMenu::_pSelf)
        GfModule::register_(LegacyMenu::_pSelf);

    // Report about success or error.
    return LegacyMenu::_pSelf ? 0 : 1;
}

int closeGfModule()
{
    // Unregister it from the GfModule module manager.
    if (LegacyMenu::_pSelf)
        LegacyMenu::unregister(LegacyMenu::_pSelf);

	DisplayMenuRelease();

    // Delete the (only) module instance.
    delete LegacyMenu::_pSelf;
    LegacyMenu::_pSelf = 0;

    // Report about success or error.
    return 0;
}

LegacyMenu& LegacyMenu::self()
{
    // Pre-condition : 1 successfull openGfModule call.
    return *_pSelf;
}

LegacyMenu::LegacyMenu(const std::string& strShLibName, void* hShLibHandle)
: GfModule(strShLibName, hShLibHandle), _piRaceEngine(0), _piGraphicsEngine(0), _piSoundEngine(0),
_hscrReUpdateStateHook(0), _hscrGame(0), _bfGraphicsState(0)
{
}

bool LegacyMenu::backLoad()
{
    GfLogTrace("Pre-loading menu and game data ...\n");

	bool SupportsHumanDrivers = LmRaceEngine().supportsHumanDrivers();

	// Pre-load the main and race select menus
    // (to be able to get back to them, even when directly starting a given race).
    if (!RmRaceSelectInit(MainMenuInit(SupportsHumanDrivers)))
        return false;

    // Pre-load race managers, drivers, tracks, cars stuff.
    if (!GfRaceManagers::self())
        return false;

    GfLogTrace("Pre-loading menu and game data completed.\n");

    return true;
}

bool LegacyMenu::activateMainMenu()
{
    return MainMenuRun() == 0;
}

bool LegacyMenu::startRace()
{
    // Get the race to start.
    std::string strRaceToStart;
    if (!GfApp().hasOption("startrace", strRaceToStart))
        return false;

    // And run it if there's such a race manager.
    GfRaceManager* pSelRaceMan = GfRaceManagers::self()->getRaceManager(strRaceToStart);
    if (pSelRaceMan) // Should never happen (checked in activate).
    {
        // Initialize the race engine.
        LmRaceEngine().reset();

        // Give the selected race manager to the race engine.
        LmRaceEngine().selectRaceman(pSelRaceMan);

        // Configure the new race (but don't enter the config. menu tree).
        LmRaceEngine().configureRace(/* bInteractive */ false);

        // Start the race engine state automaton
        LmRaceEngine().startNewRace();
    }
	else
	{
        GfLogError("No such race type '%s'\n", strRaceToStart.c_str());

        return false;
    }

    return true;
}

// Implementation of IUserInterface ****************************************

bool LegacyMenu::activate()
{
    bool (*fnSplashBackWork)(void) = LegacyMenu::backLoad;
    bool (*fnOnSplashClosed)(void) = 0;
    bool bInteractive = true;

    // Get the race to start if specified, and check if it's an available one.
    std::string strRaceToStart;
    if (GfApp().hasOption("startrace", strRaceToStart)
		&& !GfRaceManagers::self()->getRaceManager(strRaceToStart))
	{
        GfLogError("No such race type '%s', falling back to interactive choice\n",
				   strRaceToStart.c_str());
		strRaceToStart.clear(); // Cancel wrong choice.
	}

    // If no specified race to start, or if not an available one,
	// simply open the splash screen, load the menus in the background
	// and finally open the main menu.
	if (strRaceToStart.empty())
	{
        // If not specified, simply open the splash screen, load the menus in the background
        // and finally open the main menu.
        fnOnSplashClosed = LegacyMenu::activateMainMenu;
    }

	// Otherwise, run the selected race.
    else
	{
        // Open the splash screen, load some stuff in the background
        // and finally start the specified race.
        fnOnSplashClosed = LegacyMenu::startRace;
        bInteractive = false;
    }

    return SplashScreen(fnSplashBackWork, fnOnSplashClosed, bInteractive);
}

void LegacyMenu::quit()
{
    // Quit the event loop next time.
    GfuiApp().eventLoop().postQuit();
}

void LegacyMenu::shutdown()
{
	// Shutdown graphics in case relevant and not already done.
    if (_piRaceEngine->inData()->_displayMode == RM_DISP_MODE_NORMAL)
	{
        shutdownSound();
        unloadCarsGraphics();
        shutdownGraphicsView();
        unloadTrackGraphics();
        shutdownGraphics(/*bUnloadModule=*/true);
    }

    // Shutdown stuff that needs it.
    ::RmStopRaceMenuShutdown();
    ::RmStartRaceMenuShutdown();
    ::RmShutdownReUpdateStateHook();
}

void LegacyMenu::activateLoadingScreen()
{
    tRmInfo* pReInfo = _piRaceEngine->inData();

    char pszTitle[128];
    if (_piRaceEngine->race()->getManager()->hasSubFiles())
	{
        const char* pszGroup = GfParmGetStr(pReInfo->params, RM_SECT_HEADER, RM_ATTR_NAME, "<no group>");
        snprintf(pszTitle, sizeof (pszTitle), "%s - %s", pReInfo->_reName, pszGroup);
    }
	else
        snprintf(pszTitle, sizeof (pszTitle), "%s", pReInfo->_reName);

	::RmLoadingScreenStart(pszTitle, "data/img/splash-raceload.jpg");
}

void LegacyMenu::addLoadingMessage(const char* pszText)
{
    ::RmLoadingScreenSetText(pszText);
}

void LegacyMenu::shutdownLoadingScreen()
{
    ::RmLoadingScreenShutdown();
}

void LegacyMenu::activateOptimizationScreen()
{
	::RmOptimizationScreenStart("Optimization", "data/img/splash-optimization.jpg");
}

void LegacyMenu::addOptimizationMessage(const char* pszText)
{
    ::RmOptimizationScreenSetText(pszText);
}

void LegacyMenu::addOptimizationParameterMessage(int n, char** Labels, char** Values, char** Ranges)
{
    ::RmOptimizationScreenSetParameterText(n,Labels,Values,Ranges);
}

void LegacyMenu::addOptimizationStatusMessage(
	int LoopsDone, int LoopsRemaining, double VariationScale, double InitialLapTime, double TotalLapTime, double BestLapTime)
{
    ::RmOptimizationScreenSetStatusText(LoopsDone, LoopsRemaining, VariationScale, InitialLapTime, TotalLapTime, BestLapTime);
}

void LegacyMenu::shutdownOptimizationScreen()
{
    ::RmOptimizationScreenShutdown();
}

void LegacyMenu::onRaceConfiguring()
{
    ::RmOptimizationScreenShutdown();
    ::RmRacemanMenu();
}

void LegacyMenu::onRaceEventInitializing()
{
    activateLoadingScreen();
}

bool LegacyMenu::onRaceEventStarting(bool careerNonHumanGroup)
{
    tRmInfo* pReInfo = _piRaceEngine->inData();
    if (GfParmGetEltNb(pReInfo->params, RM_SECT_TRACKS) > 1)
	{
        if (!careerNonHumanGroup)
		{
            ::RmNextEventMenu();

            return false; // Tell the race engine state automaton to stop looping (enter the menu).
        }
		else
		{
            GfLogInfo("Not starting Next Event menu, because there is no human in the race");
            return true;
        }
    }

    GfLogInfo("Not starting Next Event menu, as only one track to race on.\n");

    return true; // Tell the race engine state automaton to go on looping.
}

void LegacyMenu::onRaceInitializing()
{
    // Activate the loading screen at the start of sessions,
    // that is at race session and timed practice or qualifying sessions,
    // and for the first car in non-timed practice or qualifying sessions.
    tRmInfo* pReInfo = _piRaceEngine->inData();
    if ((pReInfo->s->_raceType == RM_TYPE_QUALIF || pReInfo->s->_raceType == RM_TYPE_PRACTICE)
            && pReInfo->s->_totTime < 0.0f)
	{
        if ((int) GfParmGetNum(pReInfo->results, RE_SECT_CURRENT, RE_ATTR_CUR_DRIVER, 0, 1) == 1)
            activateLoadingScreen();
		else
            shutdownLoadingScreen();
    }
	else
	{
        activateLoadingScreen();
    }
}

void LegacyMenu::onOptimizationInitializing()
{
    // Activate the screen at the start of sessions,
    activateOptimizationScreen();
}

bool LegacyMenu::onRaceStarting()
{
    // Switch to Start Race menu only if required (no loading screen in this case).
    tRmInfo* pReInfo = _piRaceEngine->inData();
    const bool bNeedStartMenu =
            strcmp(GfParmGetStr(pReInfo->params, pReInfo->_reRaceName,
            RM_ATTR_SPLASH_MENU, RM_VAL_NO), RM_VAL_YES) == 0;
    if (bNeedStartMenu)
	{
        shutdownLoadingScreen();

        ::RmStartRaceMenu();
    }
	else
        GfLogInfo("Not starting Start Race menu, as specified not to.\n");

    // Tell the race engine state automaton to stop looping
    // if we enter the start menu, or else to go on.
    return bNeedStartMenu ? false : true;
}

void LegacyMenu::onRaceLoadingDrivers()
{
		// Create the game screen according to the actual display mode.
		if (_piRaceEngine->inData()->_displayMode == RM_DISP_MODE_NORMAL)
			_hscrGame = ::RmScreenInit();
		else
        _hscrGame = ::RmResScreenInit();

    // If first driver (of a practice or qualifying) or race session,
    // activate race loading screen.
    if (!(_piRaceEngine->inData()->s->_raceType == RM_TYPE_QUALIF
            || _piRaceEngine->inData()->s->_raceType == RM_TYPE_PRACTICE)
            || (int) GfParmGetNum(_piRaceEngine->inData()->results, RE_SECT_CURRENT, RE_ATTR_CUR_DRIVER, NULL, 1) == 1)
	{
		activateLoadingScreen();
    }
}

void LegacyMenu::onRaceDriversLoaded()
{
    if (_piRaceEngine->inData()->_displayMode == RM_DISP_MODE_NORMAL)
	{
        // It must be done after the cars are loaded and the track is loaded.
        // The track will be unloaded if the event ends.
        // The graphics module is kept open if more than one race is driven.

        // Initialize the graphics and sound engines.
        if (initializeGraphics() && initializeSound())
		{
            char buf[128];
            snprintf(buf, sizeof (buf), "Loading graphics for %s track ...",
                    _piRaceEngine->inData()->track->name);
            addLoadingMessage(buf);

            // Initialize the track graphics.
            loadTrackGraphics(_piRaceEngine->inData()->track);
        }
    }
}

void LegacyMenu::onRaceSimulationReady()
{
    if (_piRaceEngine->inData()->_displayMode == RM_DISP_MODE_NORMAL)
	{
        // Initialize the graphics view.
        setupGraphicsView();

        // Initialize cars graphics.
        addLoadingMessage("Loading graphics for all cars ...");

        loadCarsGraphics(_piRaceEngine->outData()->s);
        addLoadingMessage("Loading sound effects for all cars ...");
        _piSoundEngine->init(_piRaceEngine->outData()->s);
    }
}

bool LegacyMenu::onRaceStartingPaused(){
   GfLogDebug("LegacyMenu::onRaceStartingPaused()\n");

   bool preracePauseEnabled = false;
   char buf[256];
   snprintf(buf, sizeof(buf), "%s%s", GfLocalDir(), RACE_ENG_CFG);

   void* hparmRaceEng = GfParmReadFile(buf, GFPARM_RMODE_STD | GFPARM_RMODE_CREAT);

   // does the config allow Prerace pause?
   const char* preracepause = GfParmGetStr(hparmRaceEng, RM_SECT_RACE_ENGINE, RM_ATTR_STARTPAUSED, RM_VAL_OFF);
   preracePauseEnabled = strcmp(preracepause,RM_VAL_OFF) ? true : false;

   if (preracePauseEnabled){
      ::RmAddPreRacePauseItems();
   }
   
   // Tell the race engine if Prerace Pause is enabled
   return preracePauseEnabled;
}

void LegacyMenu::onRaceStarted()
{
	// Shutdown the loading screen if not already done.
	shutdownLoadingScreen();

	// Activate the game screen.
	GfuiScreenActivate(_hscrGame);
}

void LegacyMenu::onRaceResuming()
{
    // Start the standings menu.
    showStandings();
}

void LegacyMenu::onLapCompleted(int nLapIndex)
{

    if (nLapIndex <= 0)
        return;

    GfLogInfo("Lap #%d completed.\n", nLapIndex);
}

void LegacyMenu::onRaceInterrupted() {
    ::RmStopRaceMenu();
}

bool LegacyMenu::onRaceCooldownStarting(){

   bool cooldownEnabled = false;
   char buf[256];
   snprintf(buf, sizeof(buf), "%s%s", GfLocalDir(), RACE_ENG_CFG);

   void* hparmRaceEng = GfParmReadFile(buf, GFPARM_RMODE_STD | GFPARM_RMODE_CREAT);

   // Does the config allow cooldown driving?
    const char* cooldown = GfParmGetStr(hparmRaceEng, RM_SECT_RACE_ENGINE, RM_ATTR_COOLDOWN, RM_VAL_OFF);
    cooldownEnabled = strcmp(cooldown,RM_VAL_OFF) ? true : false;

   if (cooldownEnabled){
      ::RmAddCooldownItems();
   }
   
   // Tell the race engine if Cooldown is enabled
   return cooldownEnabled;
}

void LegacyMenu::onRaceFinishing()
{
    if (_piRaceEngine->inData()->_displayMode == RM_DISP_MODE_NORMAL)
	{
		shutdownSound();
        unloadCarsGraphics();
        shutdownGraphicsView();
        unloadTrackGraphics();
        RmScreenShutdown();
    }
	else
	{
        RmResScreenShutdown();
    }
}

bool LegacyMenu::onRaceFinished(bool bEndOfSession)
{
    tRmInfo* pReInfo = _piRaceEngine->inData();

    // Display the results of the session for all the competitors
    // only if this is the end of a session (for all competitors),
    // and if specified by the race mode or if the display mode is "normal".
    if (bEndOfSession
		&& (!strcmp(GfParmGetStr(pReInfo->params, pReInfo->_reRaceName, RM_ATTR_DISPRES, RM_VAL_YES), RM_VAL_YES)
            || pReInfo->_displayMode == RM_DISP_MODE_NORMAL))
	{
        // Create the "Race Engine update state" hook if not already done.
        if (!_hscrReUpdateStateHook)
            _hscrReUpdateStateHook = ::RmInitReUpdateStateHook();

        // This is now the "game" screen.
        _hscrGame = _hscrReUpdateStateHook;

        // Display the results menu (will activate the game screen on exit).
        ::RmShowResults(_hscrGame, _piRaceEngine->inData());

        // Tell the race engine state automaton to stop looping (enter the menu).
        return false;
    }

    GfLogInfo("Not starting Results menu (not end of session, or specified not to, or blind mode).\n");

    return true;
}

void LegacyMenu::onRaceEventFinishing()
{
    if (_piRaceEngine->inData()->_displayMode == RM_DISP_MODE_NORMAL)
	{
        unloadTrackGraphics();

        shutdownGraphicsView();
    }
}

void LegacyMenu::showStandings()
{
    // Create the "Race Engine update state" hook if not already done.
    if (!_hscrReUpdateStateHook)
        _hscrReUpdateStateHook = ::RmInitReUpdateStateHook();

    // This is now the "game" screen.
    _hscrGame = _hscrReUpdateStateHook;

    // Display the standings menu (will activate the game screen on exit).
    ::RmShowStandings(_hscrGame, _piRaceEngine->inData(), 0);
}

bool LegacyMenu::onRaceEventFinished(bool bMultiEvent, bool careerNonHumanGroup)
{
    // Only display the standings if both:
    // * there are multiple races for this championship
    // * the results are relevant for the user (do not display in Career mode in non-human groups)
    if (bMultiEvent && !careerNonHumanGroup) {
        // Start the standings menu.
        showStandings();

        // Tell the race engine state automaton to stop looping (enter the standings menu).
        return false;
    }

    GfLogInfo("Not starting Standings menu, as non-multi-event race mode"
            " or no human in this Career group\n");

    // Tell the race engine state automaton to go on looping.
    return true;
}

void LegacyMenu::setResultsTableTitles(const char* pszTitle, const char* pszSubTitle)
{
    ::RmResScreenSetTitles(pszTitle, pszSubTitle);
}

void LegacyMenu::setResultsTableHeader(const char* pszHeader)
{
    ::RmResScreenSetHeader(pszHeader);
}

void LegacyMenu::addResultsTableRow(const char* pszText)
{
    ::RmResScreenAddText(pszText);
}

void LegacyMenu::setResultsTableRow(int nIndex, const char* pszText, bool bHighlight)
{
    ::RmResScreenSetText(pszText, nIndex, bHighlight ? 1 : 0);
}

void LegacyMenu::removeResultsTableRow(int nIndex)
{
    ::RmResScreenRemoveText(nIndex);
}

int LegacyMenu::getResultsTableRowCount() const
{
    return ::RmResGetRows();
}

void LegacyMenu::eraseResultsTable()
{
    ::RmResEraseScreen();
}

void LegacyMenu::activateGameScreen()
{
    GfuiScreenActivate(_hscrGame);
}

// Graphics&Sound engine control =====================================================

bool LegacyMenu::initializeGraphics()
{
    // Check if the module is already loaded, and do nothing more if so.
    if (_piGraphicsEngine)
        return true;

    // Load the graphics module
	const char* pszModName =
		GfParmGetStr(_piRaceEngine->inData()->_reParam, "Modules", "graphic", "ssggraph");
    GfModule* pmodGrEngine = GfModule::load("modules/graphic", pszModName);

    // Check that it implements IGraphicsEngine.
    if (pmodGrEngine)
        _piGraphicsEngine = pmodGrEngine->getInterface<IGraphicsEngine > ();
    if (pmodGrEngine && !_piGraphicsEngine)
	{
        GfModule::unload(pmodGrEngine);
        GfLogError("IGraphicsEngine not implemented by %s\n", pszModName);
    }

    _bfGraphicsState = 0;

    return _piGraphicsEngine  != 0;
}

bool LegacyMenu::initializeSound()
{
	 // Check if the module is already loaded, and do nothing more if so.
    if (_piSoundEngine)
        return true;

    // Load the sound module
	const char* pszModName =
		GfParmGetStr(_piRaceEngine->inData()->_reParam, "Modules", "sound", "snddefault");
    GfModule* pmodGrEngineS = GfModule::load("modules/sound", pszModName);

    // Check that it implements IGraphicsEngine.
    if (pmodGrEngineS)
        _piSoundEngine = pmodGrEngineS->getInterface<ISoundEngine > ();
    if (pmodGrEngineS && !_piSoundEngine)
	{
        GfModule::unload(pmodGrEngineS);
        GfLogError("ISoundEngine not implemented by %s\n", pszModName);
    }

    return _piSoundEngine != 0;
}

bool LegacyMenu::loadTrackGraphics(struct Track* pTrack)
{
    if (!_piGraphicsEngine)
        return false;

    _bfGraphicsState |= eTrackLoaded;

    return _piGraphicsEngine->loadTrack(pTrack);
}

bool LegacyMenu::loadCarsGraphics(struct Situation* pSituation)
{
    if (!_piGraphicsEngine)
        return false;

    _bfGraphicsState |= eCarsLoaded;



    return _piGraphicsEngine ? _piGraphicsEngine->loadCars(pSituation) : false;
}

bool LegacyMenu::setupGraphicsView()
{
    // Initialize the graphics view.
    if (!_piGraphicsEngine)
        return false;

    // Retrieve the screen dimensions.
    int sw, sh, vw, vh;
    GfScrGetSize(&sw, &sh, &vw, &vh);

    _bfGraphicsState |= eViewSetup;

    // Setup the graphics view.
    return _piGraphicsEngine->setupView((sw - vw) / 2, (sh - vh) / 2, vw, vh, _hscrGame);
}

void LegacyMenu::redrawGraphicsView(struct Situation* pSituation)
{
    if (!_piGraphicsEngine)
        return;

    // Update graphics view.
    _piGraphicsEngine->redrawView(pSituation);

    // Update sound "view".
    Camera* pCam = _piGraphicsEngine->getCurCam();
    _piSoundEngine->refresh(pSituation, pCam);
    delete pCam;
}

void LegacyMenu::shutdownSound()
{
    if (!_piSoundEngine)
        return;

    if (_bfGraphicsState & eCarsLoaded)
    {
        _piSoundEngine->shutdown();
    }
}

void LegacyMenu::unloadCarsGraphics()
{
    if (!_piGraphicsEngine)
        return;

    if (_bfGraphicsState & eCarsLoaded)
    {
        _piGraphicsEngine->unloadCars();
        _bfGraphicsState &= ~eCarsLoaded;
    }
}

void LegacyMenu::unloadTrackGraphics()
{
    if (!_piGraphicsEngine)
        return;

    if (_bfGraphicsState & eTrackLoaded)
    {
        _piGraphicsEngine->unloadTrack();
        _bfGraphicsState &= ~eTrackLoaded;
    }
}

void LegacyMenu::shutdownGraphicsView()
{
    if (!_piGraphicsEngine)
        return;

    if (_bfGraphicsState & eViewSetup)
	{
        _piGraphicsEngine->shutdownView();
        _bfGraphicsState &= ~eViewSetup;
    }
}

void LegacyMenu::shutdownGraphics(bool bUnloadModule)
{
    // Do nothing if the module has already been unloaded.
    if (!_piGraphicsEngine)
        return;

    if (bUnloadModule)
	{
        // Unload the graphics module.
        GfModule* pmodGrEngine = dynamic_cast<GfModule*> (_piGraphicsEngine);
#ifndef UNLOAD_SSGGRAPH
        if (pmodGrEngine->getSharedLibName().find("ssggraph") == std::string::npos)
#endif
            GfModule::unload(pmodGrEngine);

        // And remember it was.
        _piGraphicsEngine = 0;
    }

    // A little consistency check.
    if (_bfGraphicsState)
        GfLogWarning("Graphics shutdown procedure not smartly completed (state = 0x%x)\n",
					 _bfGraphicsState);
}

//=========================================================================

void LegacyMenu::setRaceEngine(IRaceEngine& raceEngine)
{
    _piRaceEngine = &raceEngine;
}

// Accessor to the race engine.

IRaceEngine& LegacyMenu::raceEngine()
{
    return *_piRaceEngine;
}

// Accessor to the graphics engine.

IGraphicsEngine* LegacyMenu::graphicsEngine()
{
    return _piGraphicsEngine;
}

// Accessor to the sound engine.

ISoundEngine* LegacyMenu::soundEngine()
{
    return _piSoundEngine;
}
