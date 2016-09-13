/***************************************************************************

    file        : racerunningmenus.cpp
    created     : Sat Nov 16 18:22:00 CET 2002
    copyright   : (C) 2002 by Eric Espie
    email       : eric.espie@torcs.org   
    version     : $Id: racerunningmenus.cpp 6084 2015-08-21 00:07:15Z beaglejoe $                                  

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
    		The menus for when the race is running
    @author	<a href=mailto:eric.espie@torcs.org>Eric Espie</a>
    @version	$Id: racerunningmenus.cpp 6084 2015-08-21 00:07:15Z beaglejoe $
*/

#include <cstdlib>
#include <cstdio>
#include <cctype>
#include <sstream>

#include <portability.h>
#include <tgf.hpp>
#include <tgfclient.h>

#include <isoundengine.h>
#include <raceman.h>
#include <robot.h>

#include "legacymenu.h"
#include "racescreens.h"


static void	*rmScreenHandle = 0;
static int	rmPauseId;
static int	rmMsgId;
static int	rmBigMsgId;

static float black[4] = { 0.0, 0.0, 0.0, 0.0 };


// Uncomment to activate FPS limiter at compile-time.
//#define UseFPSLimiter true

#ifdef UseFPSLimiter

// Forced max. graphics update rate (Hz) (0 means no maximum).
static double FPSLimMaxRate = 0;

// Last time a real graphics update was done (only used if FPSLimMaxRate).
static double FPSLimLastTime = 0;

#endif

/**************************************************************************
 * "Slow resume race" manager (from Race Stop menu)
 */
const double RmProgressiveTimeModifier::_sfTimeMultiplier = 4;
const double RmProgressiveTimeModifier::_sfDelay = 1;
const double RmProgressiveTimeModifier::_sfTimeLapse = 2;

RmProgressiveTimeModifier rmProgressiveTimeModifier;

RmProgressiveTimeModifier::RmProgressiveTimeModifier()
{
	reset();
}

void RmProgressiveTimeModifier::reset()
{
	_bExecRunning = false;
	_fExecStartTime = 0;
	_fWholeTimeLapse = 0;
	_fOldTimeMultiplier = _sfTimeMultiplier;
	_fResetterTimeMultiplier = 1;
}

void RmProgressiveTimeModifier::start()
{
	// First, reset to the initial time multiplier if already running.
	if (_bExecRunning)
		LmRaceEngine().accelerateTime(1 / _fResetterTimeMultiplier);
	
	// Initially apply the whole simulation-time change.
	LmRaceEngine().accelerateTime(_sfTimeMultiplier);
	
	// Log the activation time.
	_fExecStartTime = GfTimeClock();

	// Initialize manager state.
	_fWholeTimeLapse = _sfDelay + _sfTimeLapse;
	_fOldTimeMultiplier = _fResetterTimeMultiplier = _sfTimeMultiplier;
	
	// Enable the manager.
	_bExecRunning = true;
}

void RmProgressiveTimeModifier::execute()
{
	if (_bExecRunning)
	{
		// Get current time.
		const double fExecCurrentTime = GfTimeClock();
		
		// Calculate the difference from start time to current time :
		// how long have run the manger until now ?
		double fExecTimeDifference = fExecCurrentTime - _fExecStartTime;
		
		// We wait until we reached the delay time.
		if (fExecTimeDifference > _sfDelay)
		{
			// We should be sure that we dont set a speed higher than the 1.0 one ;
			// this can happen if the execTimeDifference is higther that the timeLapse.
			if (fExecTimeDifference > _fWholeTimeLapse)
				fExecTimeDifference = _fWholeTimeLapse;

			// Factor to restore normal game speed.
			const double fResetter = 1 / _fOldTimeMultiplier;

			// Factor to apply the new acceleration.
			const double fNewMult =
				1 + _sfTimeMultiplier * ((_fWholeTimeLapse - fExecTimeDifference) / _fWholeTimeLapse);

			// Apply the simulation-time changes.
			LmRaceEngine().accelerateTime(fResetter * fNewMult);

			// Remember the integrated-since-start applied acceleration.
			_fResetterTimeMultiplier *= fResetter * fNewMult;

			// Remember the new applied acceleration.
			_fOldTimeMultiplier = fNewMult;
		}
		
		// if the timeLapse is reached we should not run next time.
		if (fExecTimeDifference >= _fWholeTimeLapse)
			terminate();
	}
}

void RmProgressiveTimeModifier::terminate()
{
	// Seems we have done our work. Lets keep quiet until next time start() is called.
	reset();
}

/***************************************************************************/
static void
rmUpdateRaceEngine()
{
    LmRaceEngine().updateState();
}

/**************************************************************************
 * Normal race screen (3D animated scene mode = non "blind" mode)
 */

// Current values for the menu messages.
static std::string rmStrCurMsg;
static std::string rmStrCurBigMsg;

// Race pause flag
// There are 2 concepts of "Pause" here, and they can overlap each other :
// 1) the race engine : LmRaceEngine().outData()->s->_raceState & RM_RACE_PAUSED
//      which matches with the in-game 'P' key shortcut to freeze the race screen,
// 2) the menu one : you can also pause the game through the 'Esc' key (=> Stop Race menu)
//      and through the 'F1' key (=> help menu).
// This flag is for the 2nd one :
//  when you unpause in the 2nd one meaning, you may stay paused in the 1st one meaning.
static bool rmRacePaused = false;

// Flag to know if the menu state has been changed (and thus needs a redraw+redisplay).
static bool rmbMenuChanged = false;

bool rmPreRacePause = false;

struct RmMovieCapture
{
    int		enabled;
    int		active;
    double	simuRate; // Hz
    double	frameRate; // Hz
    char*	outputBase;
    int		currentCapture;
    int		currentFrame;
};


static RmMovieCapture rmMovieCapture =
{
	false, // enabled
	false, // active
	0.0,   // simuRate
	0.0,   // frameRate
	0,     // outputBase
	0,     // currentCapture
	0      // currentFrame
};
				  
static void
rmInitMovieCapture()
{
	// Don't do it twice.
	if (rmMovieCapture.outputBase)
		return;

	// But do it the first time.
	char buf[256];
	snprintf(buf, sizeof(buf), "%s%s", GfLocalDir(), RACE_ENG_CFG);

	void* hparmRaceEng = GfParmReadFile(buf, GFPARM_RMODE_REREAD | GFPARM_RMODE_CREAT);

	rmMovieCapture.enabled =
		strcmp(GfParmGetStr(hparmRaceEng, RM_SECT_MOVIE_CAPTURE, RM_ATT_CAPTURE_ENABLE,
							RM_VAL_NO),
			   RM_VAL_NO) ? true : false;
	rmMovieCapture.active = false;
	if (!rmMovieCapture.enabled)
	{
		rmMovieCapture.outputBase = 0;
		GfLogInfo("Movie capture disabled (see raceengine.xml)\n");
	}
	else
	{
		rmMovieCapture.frameRate =
			GfParmGetNum(hparmRaceEng, RM_SECT_MOVIE_CAPTURE, RM_ATT_CAPTURE_FPS, NULL, 25.0);
		rmMovieCapture.simuRate = 1.0 / RCM_MAX_DT_SIMU;
		char pszDefOutputBase[256];
		snprintf(pszDefOutputBase, sizeof(pszDefOutputBase), "%s%s",
				 GfLocalDir(), GfParmGetStr(hparmRaceEng, RM_SECT_MOVIE_CAPTURE,
											RM_ATT_CAPTURE_OUT_DIR, "captures"));
		rmMovieCapture.outputBase = strdup(pszDefOutputBase);
		GfDirCreate(pszDefOutputBase); // In case not already done.
		GfLogInfo("Movie capture enabled (%.0f FPS, PNG frames in %s)\n", 
				  rmMovieCapture.frameRate, rmMovieCapture.outputBase);
	}
}

static void
rmCaptureScreen()
{
	char filename[256];
    
    snprintf(filename, sizeof(filename), "%s/sd-%4.4d-%8.8d.png", rmMovieCapture.outputBase,
			 rmMovieCapture.currentCapture, rmMovieCapture.currentFrame++);
	
    GfScrCaptureAsPNG(filename);
}

static void
rmUpdateRaceMessages()
{
	if (!rmScreenHandle)
		return;

	// Set the new text for the "message" label if it changed.
	const char *pszMsg = LmRaceEngine().outData()->_reMessage;
	if ((pszMsg && rmStrCurMsg != pszMsg) || (!pszMsg && !rmStrCurMsg.empty()))
	{
		rmStrCurMsg = pszMsg ? pszMsg : "";
		GfuiLabelSetText(rmScreenHandle, rmMsgId, rmStrCurMsg.c_str());
		
		// The menu changed.
		rmbMenuChanged = true;
	}
	
	// Set the new text for the "big message" label if it changed.
	const char *pszBigMsg = LmRaceEngine().outData()->_reBigMessage;
	if ((pszBigMsg && rmStrCurBigMsg != pszBigMsg) || (!pszBigMsg && !rmStrCurBigMsg.empty()))
	{
		rmStrCurBigMsg = pszBigMsg ? pszBigMsg : "";
		GfuiLabelSetText(rmScreenHandle, rmBigMsgId, rmStrCurBigMsg.c_str());
		
		// The menu changed.
		rmbMenuChanged = true;
	}
}

static void
rmRedisplay()
{
	// Process any pending (human) pit request.
	const bool bPitRequested = RmCheckPitRequest();
	
#ifdef UseFPSLimiter

	// Auto FPS limitation if specified and if not capturing frames.
	if (FPSLimMaxRate > 0 && !rmMovieCapture.active)
	{
		// If too early to refresh graphics, do nothing more than wait a little.
		const double dCurrentTime = GfTimeClock();
		if (dCurrentTime < FPSLimLastTime + 1.0 / FPSLimMaxRate)
		{
			// Wait a little, to let the CPU take breath.
			// Note : Theorical resolution is 1ms, but actual one is from far more
			//        (10-15ms under Windows, even worse under Linux ?)
			//        which explains a lower than expected actual FPS mean.
			GfSleep(0.001);

			// Only giving back control to the scheduler gives good results
			// as for the actual mean FPS, but keeps the CPU 100 % (not very cool).
			//GfSleep(0.0);
			
			// Request an update in the next event loop though.
			GfuiApp().eventLoop().postRedisplay();
			
			return;
		}

		// Otherwise, last update time is now : go on with graphics update.
		FPSLimLastTime = dCurrentTime;
	}
	
#endif
	
	// Exec the "slow resume race" manager, if needed.
	if (!rmPreRacePause)
	{
		rmProgressiveTimeModifier.execute();
	}

	// Redraw the graphics part of the GUI if requested.
	const bool bUpdateGraphics =
		LmRaceEngine().outData()->_displayMode == RM_DISP_MODE_NORMAL
		&& !bPitRequested && LegacyMenu::self().graphicsEngine();
	
	if (bUpdateGraphics)
	{
		//GfSchedBeginEvent("raceupdate", "graphics");
		LegacyMenu::self().redrawGraphicsView(LmRaceEngine().outData()->s);
		//GfSchedEndEvent("raceupdate", "graphics");
	}

	// Synchronize the menu with the race messages if any changed.
	rmUpdateRaceMessages();

	// Redraw the menu part of the GUI
	// (always necessary if the graphics were redrawn first).
	if (bUpdateGraphics || rmbMenuChanged)
		GfuiRedraw();

	// Really do the display work.
	if (bUpdateGraphics || rmbMenuChanged)
		GfuiSwapBuffers();

	// The menu changes has now been taken into account.
	rmbMenuChanged = false;

	// Capture the newly displayed frame if movie capture mode.
	if (rmMovieCapture.active)
		rmCaptureScreen();

	// Request an redisplay in the next event loop.
	GfuiApp().eventLoop().postRedisplay();
}

// Warning : This function is called when the race is actually starting in "non-blind" mode,
//           but also when coming back from the Stop Race menu (the user chose "Resume")
//           or from the Help menu. It must also consider that the race may be currently paused !

static void
rmScreenActivate(void * /* dummy */)
{
	// Configure the FPS limiter if active.
#ifdef UseFPSLimiter
	
	// Get the max. refresh rate from the screen config params file.
	std::ostringstream ossConfFile;
	ossConfFile << GfLocalDir() << GFSCR_CONF_FILE;
	void* hparmScrConf = GfParmReadFile(ossConfFile.str().c_str(), GFPARM_RMODE_STD);
	FPSLimLastTime = 0.0;
	FPSLimMaxRate =
		GfParmGetNum(hparmScrConf, GFSCR_SECT_VALIDPROPS, GFSCR_ATT_MAXREFRESH, NULL, 0.0);
	if (FPSLimMaxRate)
		GfLogInfo("FPS limiter is on (%.1f Hz).\n", FPSLimMaxRate);
	else
		GfLogInfo("FPS limiter is off.\n");
	
	GfParmReleaseHandle(hparmScrConf);
	
#endif

	// Configure the event loop.
	GfuiApp().eventLoop().setRecomputeCB(rmUpdateRaceEngine);
    GfuiApp().eventLoop().setRedisplayCB(rmRedisplay);

	// If not paused ...
	if ((!rmRacePaused)&&(!rmPreRacePause))
	{
		// Reset normal sound volume.
		if (LegacyMenu::self().soundEngine())
			LegacyMenu::self().soundEngine()->mute(false);
	
		// Resynchronize the race engine.
		LmRaceEngine().start();
	}
	
	// Request a redisplay for the next event loop.
    GfuiApp().eventLoop().postRedisplay();
	
	// The menu changed.
	rmbMenuChanged = true;
}

static void
rmRacePause(void * /* vboard */)
{
   // Pause is disabled during Pre Race Pause
   // as the simulation is already Paused
   if (!rmPreRacePause)
   {
      if (rmRacePaused)
      {
         if (LegacyMenu::self().soundEngine())
            LegacyMenu::self().soundEngine()->mute(false);

		LmRaceEngine().start();

		// Hide the "Pause" label.
		GfuiVisibilitySet(rmScreenHandle, rmPauseId, GFUI_INVISIBLE);
		
		// Show again the hidden message label.
		GfuiVisibilitySet(rmScreenHandle, rmMsgId, GFUI_VISIBLE);

		// Launch the "slow resume race" manager if non-blind mode.
		if (LmRaceEngine().outData()->_displayMode == RM_DISP_MODE_NORMAL)
			rmProgressiveTimeModifier.start();
    }
	else
	{
		if (LegacyMenu::self().soundEngine())
			LegacyMenu::self().soundEngine()->mute(true);

		LmRaceEngine().stop();

		// Show the "Pause" label.
		GfuiVisibilitySet(rmScreenHandle, rmPauseId, GFUI_VISIBLE);

		// Hide the message label (no need to bother the user with the time mult. factor
		// when it is changing, whihc occurs when the user hits P when a slow start
		// is in-process).
		GfuiVisibilitySet(rmScreenHandle, rmMsgId, GFUI_INVISIBLE);
    }
	
	// Toggle the race-paused flag.
	rmRacePaused = !rmRacePaused;
	
	// The menu changed.
	rmbMenuChanged = true;
	}
}

static void
rmSkipPreStart(void * /* dummy */)
{
	// TODO: move this to a new LmRaceEngine().skipRacePreStart() ...
	tRmInfo* reInfo = LmRaceEngine().inData();
    if (reInfo->s->currentTime < -1.0) {
		reInfo->s->currentTime = -1.0;
		reInfo->_reLastRobTime = -1.0;
    }
}

static void
rmTimeMod (void *pvCmd)
{
	double fMultFactor = 0.0; // The mult. factor for resetting "real time" simulation step.
	if ((long)pvCmd > 0)
		fMultFactor = 0.5; // Accelerate time means reduce the simulation time step.
	else if ((long)pvCmd < 0)
		fMultFactor = 2.0; // Slow-down time means increase the simulation time step.
	LmRaceEngine().accelerateTime(fMultFactor);
}

static void
rmToggleMovieCapture(void * /* dummy */)
{
    if (!rmMovieCapture.enabled) 
    {
		GfLogWarning("Movie capture is not enabled : command ignored\n");
		return;
    }
    
    if (!(LmRaceEngine().outData()->_displayMode & RM_DISP_MODE_NORMAL))
    {
		GfLogWarning("Movie capture is available only in normal display mode : command ignored\n");
		return;
    }
    
    rmMovieCapture.active = !rmMovieCapture.active;
    if (rmMovieCapture.active)
	{
		// Try and change the race engine scheduling scheme for movie capture.
		if (LmRaceEngine().setSchedulingSpecs(rmMovieCapture.simuRate, rmMovieCapture.frameRate))
		{
			rmMovieCapture.currentFrame = 0;
			rmMovieCapture.currentCapture++;
			GfLogInfo("Starting movie capture\n");
		}
		else
		{
			// Not supported (multi-threaded mode).
			rmMovieCapture.active = false;
			GfLogWarning("Movie capture not supported in multi-threaded mode : command ignored\n");
		}
    }
	else
	{
		GfLogInfo("Stopping movie capture\n");
		LmRaceEngine().setSchedulingSpecs(1.0 / RCM_MAX_DT_SIMU);
		LmRaceEngine().start(); // Resynchronize the race engine.
    }
}

static void
rmHideShowMouseCursor(void * /* dummy */)
{
    GfuiMouseToggleVisibility();
}

static void
rmApplyState(void *pvState)
{
    LmRaceEngine().applyState((int)(long)pvState);
}

// Not used : see below the commented-out call.
// static void
// rmOneStep(void *pvState)
// {
//     LmRaceEngine().step((int)(long)pvState);
// }

static void
rmOpenHelpScreen(void * /* dummy */)
{
	LmRaceEngine().stop();

	if (LegacyMenu::self().soundEngine())
		LegacyMenu::self().soundEngine()->mute(true);
	
	GfuiHelpScreen(rmScreenHandle, RmBackToRaceHookInit());
}

static void
rmAddKeys()
{
    GfuiAddKey(rmScreenHandle, GFUIK_F1,  "Help", NULL, rmOpenHelpScreen, NULL);
    GfuiAddKey(rmScreenHandle, GFUIK_F12, "Screen shot", NULL, GfuiScreenShot, NULL);

    GfuiAddKey(rmScreenHandle, '-', "Slow down time",    (void*)-1, rmTimeMod, NULL);
    GfuiAddKey(rmScreenHandle, '+', "Accelerate time",   (void*)+1, rmTimeMod, NULL);
    GfuiAddKey(rmScreenHandle, '.', "Restore real time", (void*)0, rmTimeMod, NULL);
	
    GfuiAddKey(rmScreenHandle, 'p', "Pause Race",        (void*)0, rmRacePause, NULL);
    GfuiAddKey(rmScreenHandle, GFUIK_ESCAPE,  "Stop current race", (void*)RE_STATE_RACE_STOP, rmApplyState, NULL);
    GfuiAddKey(rmScreenHandle, 'q', GFUIM_ALT, "Quit game now, save nothing",    (void*)RE_STATE_EXIT, rmApplyState, NULL);
    GfuiAddKey(rmScreenHandle, ' ', "Skip pre-start",    (void*)0, rmSkipPreStart, NULL);
	
	// WARNING: Sure this won't work with multi-threading On/Auto ...
    //GfuiAddKey(rmScreenHandle, '0', "One step simulation",    (void*)1, rmOneStep, NULL);
	
    GfuiAddKey(rmScreenHandle, 'c', "Movie capture (if enabled)", (void*)0, rmToggleMovieCapture, NULL);
    GfuiAddKey(rmScreenHandle, 'o', "Hide / Show mouse cursor",   (void*)0, rmHideShowMouseCursor, NULL);
}

void *
RmScreenInit()
{
    // Release screen if was initialized.
    RmScreenShutdown();

    // Create screen, load menu XML descriptor and create static controls.
    rmScreenHandle = GfuiScreenCreate(black, 0, rmScreenActivate, 0, 0, 0);
    void *menuXMLDescHdle = GfuiMenuLoad("raceglscreen.xml");
    GfuiMenuCreateStaticControls(rmScreenHandle, menuXMLDescHdle);

    // Create Message, BigMessage and Pause labels.
    rmMsgId = GfuiMenuCreateLabelControl(rmScreenHandle, menuXMLDescHdle, "message");
    rmBigMsgId = GfuiMenuCreateLabelControl(rmScreenHandle, menuXMLDescHdle, "bigmessage");
    rmPauseId = GfuiMenuCreateLabelControl(rmScreenHandle, menuXMLDescHdle, "pause");

    // Close menu XML descriptor.
    GfParmReleaseHandle(menuXMLDescHdle);
    
    // Register keyboard shortcuts.
    rmAddKeys();

    // We are starting "unpaused".
    GfuiVisibilitySet(rmScreenHandle, rmPauseId, GFUI_INVISIBLE);
	rmRacePaused = false;
	rmPreRacePause = false;

	// Re-initialize the progressive time modifier,
	// in case the race was exited while it was running.
	rmProgressiveTimeModifier.reset();
	
	// Initialize the movie capture system.
	rmInitMovieCapture();
	
    return rmScreenHandle;
}

void
RmScreenShutdown()
{
    if (rmScreenHandle) {
		GfuiScreenRelease(rmScreenHandle);
		rmScreenHandle = 0;
    }
}


static void
rmActivateReUpdateStateHook(void * /* dummy */)
{
    rmUpdateRaceEngine();
}

static void	*pvUpdateStateHookHandle = 0;
	
void *
RmInitReUpdateStateHook()
{
    if (!pvUpdateStateHookHandle)
		pvUpdateStateHookHandle = GfuiHookCreate(0, rmActivateReUpdateStateHook);

    return pvUpdateStateHookHandle;
}

void
RmShutdownReUpdateStateHook()
{
	GfuiHookRelease(pvUpdateStateHookHandle);
	pvUpdateStateHookHandle = 0;
}

static void 
RmReadyToRace(void * /* dummy */)
{
   if (GfuiRemoveKey(rmScreenHandle,GFUIK_RETURN,"Ready"))
   {
   		GfLogInfo("<Enter> key for Ready' removed \n");
   }
   else
   {
      GfLogInfo("FAILED to remove <Enter> to Start key \n");
   }

   // The menu changed.
   rmbMenuChanged = true;

   rmPreRacePause = false;

   // Enable the sound
   if (LegacyMenu::self().soundEngine())
   {
      LegacyMenu::self().soundEngine()->mute(false);
   }

   LmRaceEngine().stopPreracePause();
}

void 
RmAddPreRacePauseItems()
{
   if(rmScreenHandle)
   {
      rmPreRacePause = true;
      GfuiAddKey(rmScreenHandle, GFUIK_RETURN,  "Ready", 0, RmReadyToRace, NULL);
      
      // The menu changed.
      rmbMenuChanged = true;

      // Disable the sound
      if (LegacyMenu::self().soundEngine())
      {
         LegacyMenu::self().soundEngine()->mute(true);
      }
   }
}

static void 
RmResultShow(void * /* dummy */)
{
   LmRaceEngine().stopCooldown();
}


void 
RmAddCooldownItems()
{
   if(rmScreenHandle)
   {
      GfuiAddKey(rmScreenHandle, GFUIK_RETURN,  "Results", 0, RmResultShow, NULL);
      
      // The menu changed.
      rmbMenuChanged = true;
   }
}

/**************************************************************************
 * Result only screen (blind mode)
 */
static float rmColors[2][4]; // Initialized at menu-load time (2 RGBA float color arrays).

static void	*rmResScreenHdle = 0;

static int	rmResTitleId;
static int	rmResSubTitleId;
static int	rmResHeaderId;
static int*	rmResRowLabelId = 0; // Initialized at menu-load time.

static float** rmResRowColor = 0; // Initialized at menu-load time.
static char** rmResRowText = 0; // Initialized at menu-load time.

static int	rmCurRowIndex;

static int rmNMaxResRows = 0; // Initialized at menu-load time.

// Flag to know if the menu state has been changed (and thus needs a redraw+redisplay).
static bool rmbResMenuChanged = false;

static void
rmResRedisplay()
{
	// Redraw the menu part of the GUI if necessary.
	if (rmbResMenuChanged)
		GfuiRedraw();

	// Really do the display work.
	if (rmbResMenuChanged)
		GfuiSwapBuffers();

	// The menu changes has now been taken into account.
	rmbResMenuChanged = false;
	
	// Request an redisplay in the next event loop.
	GfuiApp().eventLoop().postRedisplay();
}

static void
rmResScreenActivate(void * /* dummy */)
{
	GfLogInfo("Entering Result menu ...\n");
	
	// Configure the event loop.
	GfuiApp().eventLoop().setRecomputeCB(rmUpdateRaceEngine);
	GfuiApp().eventLoop().setRedisplayCB(rmResRedisplay);

	// Resynchronize the race engine.
	LmRaceEngine().start();

	// Request a redisplay for the next event loop.
	GfuiApp().eventLoop().postRedisplay();
	
	// The menu changed.
	rmbResMenuChanged = true;
}

// Used only in commented-out RmResShowCont (see below) : remove ?
// static void
// rmContDisplay()
// {
//     GfuiDisplay();
	
//     GfuiApp().eventLoop().postRedisplay();
// }

// static void
// rmResCont(void * /* dummy */)
// {
//     rmUpdateRaceEngine();
// }

static void
rmResScreenDeactivate(void * /* dummy */)
{
}

void
RmResScreenShutdown()
{
	freez(rmResRowLabelId);
	if (rmResRowText)
	{
		for (int i = 0; i < rmNMaxResRows; i++)
			free(rmResRowText[i]);
		freez(rmResRowText);
	}
	freez(rmResRowColor);
}

void*
RmResScreenInit()
{
	if (rmResScreenHdle)
		GfuiScreenRelease(rmResScreenHdle);

	tRmInfo* reInfo = LmRaceEngine().inData();

    // Create screen, load menu XML descriptor and create static controls.
    rmResScreenHdle = GfuiScreenCreate(black, 0, rmResScreenActivate, 0, rmResScreenDeactivate, 0);
    void *hmenu = GfuiMenuLoad("raceblindscreen.xml");
    GfuiMenuCreateStaticControls(rmResScreenHdle, hmenu);

    // Create variable main title (race session) label.
    rmResTitleId = GfuiMenuCreateLabelControl(rmResScreenHdle, hmenu, "Title");

    // Create background image if any specified.
    const char* img = GfParmGetStr(reInfo->params, RM_SECT_HEADER, RM_ATTR_RUNIMG, 0);
    if (img)
		GfuiScreenAddBgImg(rmResScreenHdle, img);
    
    // Create variable subtitle (driver and race name, lap number) label.
    rmResSubTitleId = GfuiMenuCreateLabelControl(rmResScreenHdle, hmenu, "SubTitle");

    // Create table header label.
    rmResHeaderId = GfuiMenuCreateLabelControl(rmResScreenHdle, hmenu, "Header");

	// Get layout properties, except for nMaxResultRows (see below).
    const int yTopRow = (int)GfuiMenuGetNumProperty(hmenu, "yTopRow", 400);
    const int yRowShift = (int)GfuiMenuGetNumProperty(hmenu, "yRowShift", 20);

	// Allocate row info arrays, if not already done.
	if (!rmResRowLabelId)
	{
		// Load nMaxResultRows/colors only the first time (ignore any later change,
		// otherwize, we'd have to realloc the row info arrays).
		rmNMaxResRows = (int)GfuiMenuGetNumProperty(hmenu, "nMaxResultRows", 20);
		const GfuiColor cNormal =
			GfuiColor::build(GfuiMenuGetStrProperty(hmenu, "rowColorNormal", "0x0000FF"));
		const GfuiColor cHighlighted =
			GfuiColor::build(GfuiMenuGetStrProperty(hmenu, "rowColorHighlighted", "0x00FF00"));
		memcpy(rmColors[0], cNormal.toFloatRGBA(), sizeof(rmColors[0]));
		memcpy(rmColors[1], cHighlighted.toFloatRGBA(), sizeof(rmColors[1]));
	
		rmResRowLabelId = (int*)calloc(rmNMaxResRows, sizeof(int));
		rmResRowText = (char**)calloc(rmNMaxResRows, sizeof(char*));
		rmResRowColor = (float**)calloc(rmNMaxResRows, sizeof(float*));
	}

    // Create result rows (1 label for each).
    int	y = yTopRow;
    for (int i = 0; i < rmNMaxResRows; i++)
	{
		freez(rmResRowText[i]);
		rmResRowColor[i] = rmColors[0];
		rmResRowLabelId[i] =
			GfuiMenuCreateLabelControl(rmResScreenHdle, hmenu, "Row", true, // from template
									   "", GFUI_TPL_X, y, GFUI_TPL_FONTID, GFUI_TPL_WIDTH,
									   GFUI_TPL_ALIGN, GFUI_TPL_MAXLEN, rmResRowColor[i]);
		y -= yRowShift;
    }

    // Close menu XML descriptor.
    GfParmReleaseHandle(hmenu);
    
    // Register keyboard shortcuts.
    GfuiAddKey(rmResScreenHdle, GFUIK_F1, "Help", rmResScreenHdle, GfuiHelpScreen, NULL);
    GfuiAddKey(rmResScreenHdle, GFUIK_F12, "Screen shot", NULL, GfuiScreenShot, NULL);

    GfuiAddKey(rmResScreenHdle, GFUIK_ESCAPE, "Stop current race",
			   (void*)RE_STATE_RACE_STOP, rmApplyState, NULL);
    GfuiAddKey(rmResScreenHdle, 'q', GFUIM_ALT, "Quit game now, save nothing",
			   (void*)RE_STATE_EXIT, rmApplyState, NULL);

    // Initialize current result row.
    rmCurRowIndex = 0;

    return rmResScreenHdle;
}

void
RmResScreenSetTitles(const char *pszTitle, const char *pszSubTitle)
{
    if (!rmResScreenHdle)
		return;
	
	GfuiLabelSetText(rmResScreenHdle, rmResTitleId, pszTitle);
	GfuiLabelSetText(rmResScreenHdle, rmResSubTitleId, pszSubTitle);
	
	// The menu changed.
	rmbResMenuChanged = true;
}

void
RmResScreenSetHeader(const char *pszHeader)
{
    if (!rmResScreenHdle)
		return;
	
	GfuiLabelSetText(rmResScreenHdle, rmResHeaderId, pszHeader);
	
	// The menu changed.
	rmbResMenuChanged = true;
}

// Cleanup the given row text :
// * replace leading zeros in non-time columns.
static char*
rmCleanRowText(const char* pszText)
{
	char* pszTargetText = strdup(pszText);
	char* pszSearchText = strdup(pszText);
	char* pszToken = strtok(pszSearchText, " ");
	while (pszToken)
	{
		if (!strchr(pszToken, ':')) // Leave time fields unchanged.
		{
			unsigned nLead0s = 0;
			int minus = 0;

			if (pszToken[0] == '-' && isdigit(pszToken[nLead0s+1])) {
				minus = 1;
				nLead0s++;
			}

			while (nLead0s + 1 < strlen(pszToken)
				   && pszToken[nLead0s] == '0' && isdigit(pszToken[nLead0s+1]))
				nLead0s++;

			while (nLead0s != 0)
			{
				if (minus == 1) {
                                	pszTargetText[pszToken - pszSearchText + nLead0s - 1] = '-';
					minus = 0;
				} else
                                	pszTargetText[pszToken - pszSearchText + nLead0s - 1] = ' ';
				nLead0s--;
			}
		}
		pszToken = strtok(0, " ");
	}
	free(pszSearchText);
	
	return pszTargetText;
}

void
RmResScreenAddText(const char *text)
{
    if (!rmResScreenHdle)
		return;
	
    if (rmCurRowIndex == rmNMaxResRows)
	{
		free(rmResRowText[0]);
		for (int i = 1; i < rmNMaxResRows; i++)
		{
			rmResRowText[i - 1] = rmResRowText[i];
			GfuiLabelSetText(rmResScreenHdle, rmResRowLabelId[i - 1], rmResRowText[i]);
		}
		rmCurRowIndex--;
    }
	else
	{
		free(rmResRowText[rmCurRowIndex]);
	}
    rmResRowText[rmCurRowIndex] = rmCleanRowText(text);
    GfuiLabelSetText(rmResScreenHdle, rmResRowLabelId[rmCurRowIndex], rmResRowText[rmCurRowIndex]);
    rmCurRowIndex++;
	
	// The menu changed.
	rmbResMenuChanged = true;
}

void
RmResScreenSetText(const char *text, int row, int clr)
{
    if (!rmResScreenHdle)
		return;
	
    if (row >= 0 && row < rmNMaxResRows)
	{
		free(rmResRowText[row]);
		rmResRowText[row] = rmCleanRowText(text);
		rmResRowColor[row] = (clr >= 0 && clr < 2) ? rmColors[clr] : rmColors[0];
		GfuiLabelSetText(rmResScreenHdle, rmResRowLabelId[row], rmResRowText[row]);
		GfuiLabelSetColor(rmResScreenHdle, rmResRowLabelId[row], rmResRowColor[row]);

		// The menu changed.
		rmbResMenuChanged = true;
    }
}

int
RmResGetRows()
{
    return rmNMaxResRows;
}

void
RmResEraseScreen()
{
	if (!rmResScreenHdle)
		return;
	
	for (int i = 0; i < rmNMaxResRows; i++)
		RmResScreenSetText("", i, 0);
	
	// The menu changed.
	rmbResMenuChanged = true;
}


void
RmResScreenRemoveText(int row)
{
	if (!rmResScreenHdle)
		return;
	
	if (row < rmNMaxResRows)
	{
		freez(rmResRowText[row]);
		GfuiLabelSetText(rmResScreenHdle, rmResRowLabelId[row], "");
		
		// The menu changed.
		rmbResMenuChanged = true;
    }
}

// Never used : remove ?
//void
//RmResShowCont()
//{
//    GfuiButtonCreate(rmResScreenHdle, "Continue", GFUI_FONT_LARGE_C,
//					 320, 15, GFUI_BTNSZ, GFUI_ALIGN_HC, 0, 0, rmResCont,
//					 NULL, (tfuiCallback)NULL, (tfuiCallback)NULL);
//    GfuiAddKey(rmResScreenHdle, GFUIK_RETURN,  "Continue", 0, rmResCont, NULL);
//    GfuiAddKey(rmResScreenHdle, GFUIK_ESCAPE,  "Continue", 0, rmResCont, NULL);
//
//    GfuiApp().eventLoop().setRedisplayCB(rmContDisplay);
//	
//    GfuiApp().eventLoop().postRedisplay();
//}
