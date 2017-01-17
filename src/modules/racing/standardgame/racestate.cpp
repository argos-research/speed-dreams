/***************************************************************************

    file        : racestate.cpp
    created     : Sat Nov 16 12:00:42 CET 2002
    copyright   : (C) 2002 by Eric Espie
    email       : eric.espie@torcs.org   
    version     : $Id: racestate.cpp 6153 2015-09-28 03:11:19Z beaglejoe $
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
    		The Race Engine State Automaton
    @author	<a href=mailto:eric.espie@torcs.org>Eric Espie</a>
    @version	$Id: racestate.cpp 6153 2015-09-28 03:11:19Z beaglejoe $
*/

#include <raceman.h>

#include "standardgame.h"

#include "racesituation.h"
#include "racemain.h"
#include "raceinit.h"
#include "racenetwork.h"
#include "raceupdate.h"
#include "raceresults.h"

#include "racestate.h"


// State Automaton Init
void
ReStateInit(void *prevMenu)
{
}


// State Automaton Management
void
ReStateManage(void)
{
	int mode = RM_SYNC | RM_NEXT_STEP;

	do {
		switch (ReInfo->_reState) {
			case RE_STATE_CONFIG:
				GfLogInfo("%s now in CONFIG state\n", ReInfo->_reName);
				// Race configuration
				mode = ReConfigure();
				if (mode & RM_NEXT_STEP) {
					ReInfo->_reState = RE_STATE_EVENT_INIT;
				}
				break;

			case RE_STATE_EVENT_INIT:
				GfLogInfo("%s now in EVENT_INIT state\n", ReInfo->_reName);
				// Load the event description (track and drivers list)
				mode = ReRaceEventInit();
				if (mode & RM_NEXT_STEP) {
					ReInfo->_reState = RE_STATE_PRE_RACE;
				}
				break;

			case RE_STATE_PRE_RACE:
				GfLogInfo("%s now in PRE_RACE state\n", ReInfo->_reName);
				mode = RePreRace();
				if (mode & RM_NEXT_RACE) {
					if (mode & RM_NEXT_STEP) {
						ReInfo->_reState = RE_STATE_EVENT_SHUTDOWN;
					}
				} else if (mode & RM_NEXT_STEP) {
					ReInfo->_reState = RE_STATE_RACE_START;
				}
				break;

			case RE_STATE_RACE_START:
				GfLogInfo("%s now in RACE_START state\n", ReInfo->_reName);
				mode = ReRaceStart();
				if (mode & RM_NEXT_STEP) {
					ReInfo->_reState = RE_STATE_NETWORK_WAIT;
					GfLogInfo("%s now in NETWORK_WAIT state\n", ReInfo->_reName);
				}
				break;

			case RE_STATE_NETWORK_WAIT:
				mode = ReNetworkWaitReady();
				if (mode & RM_NEXT_STEP) {
					// Not an online race, or else all online players ready
					ReInfo->_reState = RE_STATE_PRE_RACE_PAUSE;
					GfLogInfo("%s now in PRE RACE PAUSE state\n", ReInfo->_reName);
				}
				break;

			case RE_STATE_PRE_RACE_PAUSE:
				mode = RePreRacePause();
				if (mode & RM_NEXT_STEP) {
					// player is ready
					ReInfo->_reState = RE_STATE_RACE;
					ReInfo->s->currentTime = -2.0;
					GfLogInfo("%s now in RACE state\n", ReInfo->_reName);
				}
				break;

			case RE_STATE_RACE:
				mode = ReUpdate();
				if (ReInfo->s->_raceState == RM_RACE_ENDED) {
					// Race is finished
					mode = ReRaceCooldown();
					if (mode & RM_NEXT_STEP) {
						ReInfo->_reState = RE_STATE_RACE_END;
					}
					else {
						ReInfo->_reState = RE_STATE_RACE_COOLDOWN;
						GfLogInfo("%s now in COOLDOWN state\n", ReInfo->_reName);
					}
				} else if (mode & RM_END_RACE) {
					// Race was interrupted (paused) by the player
					ReInfo->_reState = RE_STATE_RACE_STOP;
				}
				break;

			case RE_STATE_RACE_COOLDOWN:
				{
					// Player is on victory lap or joy riding
					// TODO rethink this transition
					// this state will transition to RE_STATE_RACE_END when ReStopCooldown() is called by UI
					mode = ReUpdate();
				}
				break;

			case RE_STATE_RACE_STOP:
				GfLogInfo("%s now in RACE_STOP state\n", ReInfo->_reName);
				// Race was interrupted (paused) by the player
				mode = ReRaceStop();
				if (mode & RM_NEXT_STEP) {
					ReInfo->_reState = RE_STATE_RACE_END;
				}
				break;

			case RE_STATE_RACE_END:
				GfLogInfo("%s now in RACE_END state\n", ReInfo->_reName);
				mode = ReRaceEnd();
				if (mode & RM_NEXT_STEP) {
					ReInfo->_reState = RE_STATE_POST_RACE;
				} else if (mode & RM_NEXT_RACE) {
					ReInfo->_reState = RE_STATE_RACE_START;
				}
				break;

			case RE_STATE_POST_RACE:
				GfLogInfo("%s now in POST_RACE state\n", ReInfo->_reName);
				mode = RePostRace();
				if (mode & RM_NEXT_STEP) {
					ReInfo->_reState = RE_STATE_EVENT_SHUTDOWN;
				} else if (mode & RM_NEXT_RACE) {
					ReInfo->_reState = RE_STATE_PRE_RACE;
				}
				break;

			case RE_STATE_EVENT_SHUTDOWN:
				GfLogInfo("%s now in EVENT_SHUTDOWN state\n", ReInfo->_reName);
				mode = ReRaceEventShutdown();
				if (mode & RM_NEXT_STEP) {
					ReInfo->_reState = RE_STATE_SHUTDOWN;
				} else if (mode & RM_NEXT_RACE) {
					ReInfo->_reState = RE_STATE_EVENT_INIT;
				}
				break;

			case RE_STATE_SHUTDOWN:
				GfLogInfo("%s now in SHUTDOWN state\n", ReInfo->_reName);
				// Back to the race manager menu
				ReInfo->_reState = RE_STATE_CONFIG;
				mode = RM_SYNC;
				break;

			case RE_STATE_ERROR:
				// If this state is set, there was a serious error:
				// i.e. no driver in the race (no one selected OR parameters out of range)
				// Error messages are normally dumped in the game trace stream !
				// TODO: Define another screen showing the error messages instead of
				// only having it in the console window!
				GfLogInfo("%s now in ERROR state\n", ReInfo->_reName);
				// Back to race manager menu
				ReInfo->_reState = RE_STATE_CONFIG;
				mode = RM_SYNC;
				break;

			case RE_STATE_EXIT:
				// Exit the race engine.
				mode = ReExit();
				break;
		}

		if (mode & RM_ERROR) {
			GfLogError("Race engine error (see above messages)\n");
			ReInfo->_reState = RE_STATE_ERROR;
			mode = RM_SYNC;
		}

		//GfLogDebug("ReStateManage : New state 0x%X, %sing.\n",
		//		   ReInfo->_reState, (mode & RM_SYNC) ? "loop" : "return");
		
	} while (mode & RM_SYNC);
}

// Change and Execute a New State
void
ReStateApply(void *pvState)
{
	ReInfo->_reState = (int)(long)pvState;

	ReStateManage();
}
