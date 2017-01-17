/***************************************************************************

    file        : racestate.cpp
    created     : Sat Nov 16 12:00:42 CET 2002
    copyright   : (C) 2002 by Eric Espie
    email       : eric.espie@torcs.org   
    version     : $Id: racestate.cpp 5856 2014-11-25 17:05:47Z wdbee $
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
    @version	$Id: racestate.cpp 5856 2014-11-25 17:05:47Z wdbee $
*/

#include <raceman.h>

#include "genparoptv1.h"

#include "racesituation.h"
#include "racemain.h"
#include "raceinit.h"
#include "raceupdate.h"
#include "raceresults.h"
#include "portability.h"

#include "racestate.h"

// Use new Memory Manager ...
#ifdef __DEBUG_MEMORYMANAGER__
#include "memmanager.h"
#endif
// ... Use new Memory Manager

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
				// Use new Memory Manager ...
				#ifdef __DEBUG_MEMORYMANAGER__
				//fprintf(stderr,"Initialise memory manager tracking ...\n");
				GfMemoryManagerSetGroup(1);
				#endif
				// ... Use new Memory Manager

			case RE_STATE_EVENT_LOOP:
				GfLogInfo("%s now in EVENT_INIT_LOOP state\n", ReInfo->_reName);
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
					ReInfo->_reState = RE_STATE_RACE;
					GfLogInfo("%s now in RACE state\n", ReInfo->_reName);
				}
				break;

			case RE_STATE_RACE:
				mode = ReUpdate();
				if (ReInfo->s->_raceState == RM_RACE_ENDED) {
					// Race is finished
					ReInfo->_reState = RE_STATE_RACE_END;
				} else if (mode & RM_END_RACE) {
					// Race was interrupted (paused) by the player
					ReInfo->_reState = RE_STATE_RACE_STOP;
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

			case RE_STATE_EVOLUTION:
				GfLogInfo("RaceEngine: state = RE_STATE_EVOLUTION\n");
	  			mode = ReEvolution();
				// Setup short cut
				if (mode & RM_NEXT_STEP) {
				  /* Back to optimization */
				  ReInfo->_reState = RE_STATE_EVENT_LOOP;
				} else {
				  /* Next step */
				  // Use new Memory Manager ...
				  #ifdef __DEBUG_MEMORYMANAGER__
				  fprintf(stderr,"... Reset memory manager tracking\n");
				  GfMemoryManagerSetGroup(0);
				  #endif
				  // ... Use new Memory Manager

				  ReInfo->_reState = RE_STATE_SHUTDOWN;
				}
				break;

			case RE_STATE_EVENT_SHUTDOWN:
				GfLogInfo("%s now in EVENT_SHUTDOWN state\n", ReInfo->_reName);
				mode = ReRaceEventShutdown();
				if (mode & RM_NEXT_STEP) {
					ReInfo->_reState = RE_STATE_EVOLUTION;
				} else if (mode & RM_NEXT_RACE) {
					ReInfo->_reState = RE_STATE_EVENT_INIT;
				}
				break;

			case RE_STATE_SHUTDOWN:
				GfLogInfo("%s now in SHUTDOWN state\n", ReInfo->_reName);
				ReCleanupGeneticOptimisation();
				ReInfo->_reState = RE_STATE_RESULTS;
				mode = RM_SYNC;
				break;

			case RE_STATE_RESULTS:
				GfLogInfo("%s now in RESULTS state\n", ReInfo->_reName);
				ReDisplayResults();
				ReInfo->_reState = RE_STATE_CLEANUP;
				mode = RM_SYNC;
				break;

			case RE_STATE_CLEANUP:
				GfLogInfo("%s now in CLEANUP state\n", ReInfo->_reName);
				ReCleanupReInfo();
				// Back to the race manager menu
				ReInfo->_reState = RE_STATE_WAITFORKEYPRESS;
				mode = RM_SYNC;
				break;

			case RE_STATE_WAITFORKEYPRESS:
				GfLogInfo("%s now in WAITFORKEYPRESS state\n", ReInfo->_reName);
				mode = ReWaitForKeyPress();
				if (mode & RM_NEXT_STEP){
					ReInfo->_reState = RE_STATE_CONFIG;
				} else {
					ReInfo->_reState = RE_STATE_WAITFORKEYPRESS;
				}
				mode = RM_SYNC;
				break;

			case RE_STATE_ERROR:
				// If this state is set, there was a serious error:
				// i.e. no driver in the race (no one selected OR parameters out of range)
				// Error messages are normally dumped in the game trace stream !
				// TODO: Define another screen showing the error messages instead of
				// only having it in the console window!
				GfLogInfo("%s now in ERROR state\n", ReInfo->_reName);
				ReCleanupGeneticOptimisation();
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
