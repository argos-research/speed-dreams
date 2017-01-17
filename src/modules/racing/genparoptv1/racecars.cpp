/***************************************************************************
 
    file        : racecars.cpp
    created     : Sat Nov 23 09:05:23 CET 2002
    copyright   : (C) 2002 by Eric Espie 
    email       : eric.espie@torcs.org 
    version     : $Id: racecars.cpp 5854 2014-11-23 17:55:52Z wdbee $

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
    		
    @author	<a href=mailto:eric.espie@torcs.org>Eric Espie</a>
    @version	$Id: racecars.cpp 5854 2014-11-23 17:55:52Z wdbee $
*/

#include <cstdlib>

#include <portability.h>
#include <raceman.h>
#include <robot.h>
#include <robottools.h>
#include <teammanager.h>

#include "genparoptv1.h"

#include "racesituation.h"
#include "raceupdate.h"
#include "raceresults.h"
#include "racecars.h"


/* Compute Pit stop time */
void
ReCarsUpdateCarPitTime(tCarElt *car)
{
	tSituation *s = ReInfo->s;
	tReCarInfo *info = &(ReInfo->_reCarInfo[car->index]);
	tCarPenalty *penalty;
	int i;

	// GfLogDebug("ReCarsUpdateCarPitTime(%s) : typ=%d, fuel=%f, rep=%d\n",
	// 		   car->_name, car->_pitStopType, car->_pitFuel, car->_pitRepair);

	switch (car->_pitStopType) {
		case RM_PIT_REPAIR:
			info->totalPitTime = 2.0f + fabs((double)(car->_pitFuel)) / 8.0f + (tdble)(fabs((double)(car->_pitRepair))) * 0.007f;
			car->_scheduledEventTime = s->currentTime + info->totalPitTime;
			RePhysicsEngine().reconfigureCar(car);

			for (i=0; i<4; i++) {
				car->_tyreCondition(i) = 1.01f;
				car->_tyreT_in(i) = 50.0f;
				car->_tyreT_mid(i) = 50.0f;
				car->_tyreT_out(i) = 50.0f;
			}
			GfLogInfo("%s in repair pit stop for %.1f s (refueling by %.1f l, repairing by %d).\n",
					  car->_name, info->totalPitTime, car->_pitFuel, car->_pitRepair);
			break;
		case RM_PIT_STOPANDGO:
			penalty = GF_TAILQ_FIRST(&(car->_penaltyList));
			if (penalty && penalty->penalty == RM_PENALTY_10SEC_STOPANDGO)
				info->totalPitTime = 10.0;
			else
				info->totalPitTime = 0.0;
			car->_scheduledEventTime = s->currentTime + info->totalPitTime;

			// Prevent car->_state & RM_CAR_STATE_PIT from being true for a too short delay,
			// in order for the penalty management to detect it.
			if (car->_scheduledEventTime < s->currentTime + RCM_MAX_DT_SIMU)
				car->_scheduledEventTime += RCM_MAX_DT_SIMU;
														 
			GfLogInfo("%s in Stop-and-Go pit stop for %.1f s.\n", car->_name, info->totalPitTime);
			break;
	}
}


/* Prepare to open the pit menu when back in the main updater (thread) */
static void
reCarsSchedulePitMenu(tCarElt *car)
{
	// Do nothing if one car is already scheduled for the pit menu
	// (this one will have to wait for the current one exiting from the menu)
	if (ReInfo->_rePitRequester)
	{
		GfLogInfo("%s would like to pit, but the pit menu is already in use.\n", car->_name);
		return;
	}

	// Otherwise, "post" a pit menu request for this car.
	ReInfo->_rePitRequester = car;
}


static void
reCarsAddPenalty(tCarElt *car, int penalty)
{
	char msg[64];
	tCarPenalty *newPenalty;

	if (penalty == RM_PENALTY_DRIVETHROUGH)
		snprintf(msg, sizeof(msg), "%s Drive-Through penalty", car->_name);
	else if (penalty == RM_PENALTY_STOPANDGO)
		snprintf(msg, sizeof(msg), "%s Stop-and-Go penalty", car->_name);
	else if (penalty == RM_PENALTY_10SEC_STOPANDGO)
		snprintf(msg, sizeof(msg), "%s 10s Stop-and-Go penalty", car->_name);
	else if (penalty == RM_PENALTY_DISQUALIFIED)
		snprintf(msg, sizeof(msg), "%s disqualified", car->_name);
	msg[sizeof(msg)-1] = 0; // Some snprintf implementations fail to do so.

	ReSituation::self().setRaceMessage(msg, 5);

	/* If disqualified, remove the car from the track */
	if (penalty == RM_PENALTY_DISQUALIFIED)
	{
		car->_state |= RM_CAR_STATE_ELIMINATED;
		return;
	}

	// Add the penalty in the list.
	newPenalty = (tCarPenalty*)calloc(1, sizeof(tCarPenalty));
	newPenalty->penalty = penalty;
	newPenalty->lapToClear = car->_laps + 5;
	GF_TAILQ_INSERT_TAIL(&(car->_penaltyList), newPenalty, link);
	//GfLogDebug("reCarsAddPenalty(car #%d) : Added penalty %p\n", car->index, newPenalty);
}

static void
reCarsRemovePenalty(tCarElt *car, tCarPenalty *penalty)
{
	GF_TAILQ_REMOVE(&(car->_penaltyList), penalty, link);
	//GfLogDebug("reCarsRemovePenalty(car #%d) : Removed penalty %p\n",
	//		   car->index, penalty);
	FREEZ(penalty);
}

/* Compute the race rules and penalties */
static void
reCarsApplyRaceRules(tCarElt *car)
{
	char msg[64];
    tCarPenalty		*penalty;
    tTrack		*track = ReInfo->track;
    tRmCarRules		*rules = &(ReInfo->rules[car->index]);
    tTrackSeg		*seg = RtTrackGetSeg(&(car->_trkPos));
    tReCarInfo		*info = &(ReInfo->_reCarInfo[car->index]);
    tTrackSeg		*prevSeg = RtTrackGetSeg(&(info->prevTrkPos));
    static const float	ctrlMsgColor[] = {0.0, 0.0, 1.0, 1.0};

	// DNF cars which need too much time for the current lap, this is mainly to avoid
	// that a "hanging" driver can stop the quali from finishing.
	// Allowed time is longest pitstop possible + time for tracklength with speed???
	// (currently fixed 10 [m/s]).
	// For simplicity. Human driver is an exception to this rule, to allow explorers
	// to enjoy the landscape.
	// Also - don't remove cars that are currently being repaired in pits
	// TODO: Make it configurable.
	if ((car->_curLapTime > 84.5 + ReInfo->track->length/10.0) &&
	    !(car->_state & RM_CAR_STATE_PIT) &&
	    (car->_driverType != RM_DRV_HUMAN))
	{
		if (!(car->_state & RM_CAR_STATE_ELIMINATED))
			GfLogInfo("%s eliminated (too long to finish the lap).\n", car->_name);
		car->_state |= RM_CAR_STATE_ELIMINATED;
	    return;
	}

	// Stop here (no more rules) if not in "Pro" skill level.
	if (car->_skillLevel < 3)
		return;

	// Stop here (no more rules) if "penalties" feature not enables for this race.
	if (! (ReInfo->s->_features & RM_FEATURE_PENALTIES) )
		return;
	
	// Otherwise, update control board and do the referee job.
	// 1) Update control board message about the current penalty if any.
	//    TODO: Optimization : Add penalty->timeStamp and car->ctrl.timeStamp fields
	//          to avoid doing this again and again as long as the penalty is not cleared
	//          whereas it's only needed when the penalty reaches the _penaltyList head.
	penalty = GF_TAILQ_FIRST(&(car->_penaltyList));
    if (penalty) {
		switch (penalty->penalty) {
			case RM_PENALTY_DRIVETHROUGH:
				snprintf(car->ctrl.msg[3], RM_CMD_MAX_MSG_SIZE, "Drive-Through Penalty");
				break;
			case RM_PENALTY_STOPANDGO:
				snprintf(car->ctrl.msg[3], RM_CMD_MAX_MSG_SIZE, "Stop-and-Go Penalty");
				break;
			case RM_PENALTY_10SEC_STOPANDGO:
				snprintf(car->ctrl.msg[3], RM_CMD_MAX_MSG_SIZE, "10s Stop-and-Go Penalty");
				break;
			default:
				*(car->ctrl.msg[3]) = 0;
				break;
		}
		car->ctrl.msg[3][RM_CMD_MAX_MSG_SIZE - 1] = 0; // Some snprintf implementations fail to do so.
		memcpy(car->ctrl.msgColor, ctrlMsgColor, sizeof(car->ctrl.msgColor));
    } else {
		// No penalty => no message.
		*(car->ctrl.msg[3]) = 0;
	}
    
	// 2) Check if not too late for the 1st penalty if any.
    if (penalty) {
		if (car->_laps > penalty->lapToClear) {
			// The penalty was not "executed" : too late to clear => disqualified (out of race)
			reCarsAddPenalty(car, RM_PENALTY_DISQUALIFIED);
			GfLogInfo("%s disqualified (penalty not executed after 5 laps).\n", car->_name);
			return;
		}
	}

	// 3) Check if we can hopefuly clear the penalty because just entered the pit lane.
	//    (means that we enter the clearing process, but that it may fail ; nothing sure)
    if (prevSeg->raceInfo & TR_PITSTART) {

		//if (seg->raceInfo & TR_PIT)
		//	GfLogDebug("%s crossed pit lane entry.\n", car->_name);
		if (penalty) {
			// just entered the pit lane
			if (seg->raceInfo & TR_PIT) {
				switch (penalty->penalty) {
					case RM_PENALTY_DRIVETHROUGH:
						snprintf(msg, sizeof(msg), "%s Drive-Through penalty clearing", car->_name);
						msg[sizeof(msg)-1] = 0; // Some snprintf implementations fail to do so.
						ReSituation::self().setRaceMessage(msg, 5);
						rules->ruleState |= RM_PNST_DRIVETHROUGH;
						GfLogInfo("%s might get its Drive-Through penalty cleared.\n", car->_name);
						break;
					case RM_PENALTY_STOPANDGO:
					case RM_PENALTY_10SEC_STOPANDGO:
						snprintf(msg, sizeof(msg), "%s Stop-and-Go penalty clearing", car->_name);
						msg[sizeof(msg)-1] = 0; // Some snprintf implementations fail to do so.
						ReSituation::self().setRaceMessage(msg, 5);
						rules->ruleState |= RM_PNST_STOPANDGO;
						GfLogInfo("%s might get his Stop-and-Go Penalty cleared.\n", car->_name);
						break;
				}
			}
		}
		
	// 4) If in pit lane for more than 1 segment :
    } else if (prevSeg->raceInfo & TR_PIT) {
		
		if (seg->raceInfo & TR_PIT) {
			// 4a) Check if we can go on with clearing the penalty because stopped in pit.
			if (car->_state & RM_CAR_STATE_PIT) {
				//GfLogDebug("%s is pitting.\n", car->_name);
				if (rules->ruleState & RM_PNST_STOPANDGO && car->_pitStopType == RM_PIT_STOPANDGO) {
					GfLogInfo("%s Stop-and-Go accepted.\n", car->_name);
					rules->ruleState |= RM_PNST_STOPANDGO_OK; // Stop-and-Go really done.
				}
			}
		} else if (seg->raceInfo & TR_PITEND) {
			//GfLogDebug("%s crossing pit lane exit.\n", car->_name);
			// 4b) Check if the penalty can really and finally be removed because exiting pit lane
			//     and everything went well in the clearing process til then.
			if (rules->ruleState & (RM_PNST_DRIVETHROUGH | RM_PNST_STOPANDGO_OK)) {
				snprintf(msg, sizeof(msg), "%s penalty cleared", car->_name);
				msg[sizeof(msg)-1] = 0; // Some snprintf implementations fail to do so.
				ReSituation::self().setRaceMessage(msg, 5);
				penalty = GF_TAILQ_FIRST(&(car->_penaltyList));
				reCarsRemovePenalty(car, penalty);
				GfLogInfo("%s %s penalty cleared.\n", car->_name,
						  (rules->ruleState & RM_PNST_DRIVETHROUGH) ? "Drive-Through" : "Stop-and-Go");
			}
			rules->ruleState = 0;
		} else {
			// 4c) Exiting pit lane the wrong way : add new stop and go penalty if possible.
			//GfLogDebug("%s exiting pit lane by a side (bad).\n", car->_name);
			if (!(rules->ruleState & RM_PNST_STOPANDGO)) {
				reCarsAddPenalty(car, RM_PENALTY_STOPANDGO);
				rules->ruleState = RM_PNST_STOPANDGO;
				GfLogInfo("%s got a Stop-and-Go penalty (went out the pits at a wrong place).\n",
						  car->_name);
			}
		}
		
	// 5) Still crossing pit lane exit (probably a long PITEND segment) : Nothing bad.
    } else if (seg->raceInfo & TR_PITEND) {

		rules->ruleState = 0;
		
	// 6) Entering the pits at a wrong place, add new stop and go penalty if possible.
    } else if (seg->raceInfo & TR_PIT) {
		//GfLogDebug("%s entering pit lane by a side (bad).\n", car->_name);
		if (!(rules->ruleState & RM_PNST_STOPANDGO)) {
			reCarsAddPenalty(car, RM_PENALTY_STOPANDGO);
			rules->ruleState = RM_PNST_STOPANDGO;
			GfLogInfo("%s got a Stop-and-Go penalty (went in the pits at a wrong place).\n",
					  car->_name);
		}
    }

	// 7) If too fast in a speed limited section, add new drive-through penalty if possible.
    if (seg->raceInfo & TR_SPEEDLIMIT) {
		if (!(rules->ruleState & (RM_PNST_OVERSPEED | RM_PNST_STOPANDGO))
			&& car->_speed_x > track->pits.speedLimit) {
			rules->ruleState |= RM_PNST_OVERSPEED;
			reCarsAddPenalty(car, RM_PENALTY_DRIVETHROUGH);
			GfLogInfo("%s got a Drive-Through penalty (too fast in the pits).\n", car->_name);
		}
    }

    // Check for jumping starting lights
    if (ReInfo->s->_raceState & RM_RACE_PRESTART  && car->_speed_x > 1) {
		if (!(rules->ruleState & (RM_PNST_STOPANDGO))) {
			reCarsAddPenalty(car, RM_PENALTY_STOPANDGO);
			rules->ruleState = RM_PNST_STOPANDGO;
			GfLogInfo("%s got a Stop-and-Go penalty (jumped starting lights).\n",
				car->_name);
		}
    }
}

void
ReCarsManageCar(tCarElt *car, bool& bestLapChanged)
{
	char msg[64];
	int i;
	int xx;
	tTrackSeg *sseg;
	tdble wseg;
	static const float ctrlMsgColor[] = {0.0, 0.0, 1.0, 1.0};
	tSituation *s = ReInfo->s;
	
	tReCarInfo *info = &(ReInfo->_reCarInfo[car->index]);

	// Update top speeds.
	if (car->_speed_x > car->_topSpeed)
		car->_topSpeed = car->_speed_x;

	// (practice and qualification only).
	if (car->_speed_x > info->topSpd)
		info->topSpd = car->_speed_x;
	if (car->_speed_x < info->botSpd)
		info->botSpd = car->_speed_x;
	
	// Pitstop management.
	if (car->_pit) {

		// If the driver can ask for a pit, update control messages whether slot occupied or not.
		if (car->ctrl.raceCmd & RM_CMD_PIT_ASKED) {
			// Pit already occupied?
			if (car->_pit->pitCarIndex == TR_PIT_STATE_FREE)
				snprintf(car->ctrl.msg[2], RM_CMD_MAX_MSG_SIZE, "Can Pit");
			else
				snprintf(car->ctrl.msg[2], RM_CMD_MAX_MSG_SIZE, "Pit Occupied");
			car->ctrl.msg[2][RM_CMD_MAX_MSG_SIZE-1] = 0; // Some snprintf implementations fail to do so.
			memcpy(car->ctrl.msgColor, ctrlMsgColor, sizeof(car->ctrl.msgColor));
		}

		// If pitting, check if pitting delay over, and end up with pitting process if so.
		if (car->_state & RM_CAR_STATE_PIT) {
			car->ctrl.raceCmd &= ~RM_CMD_PIT_ASKED; // clear the flag.
			// Note: Due to asynchronous behaviour of the main updater and the situation updater,
			//       we have to wait for car->_scheduledEventTime being set to smthg > 0.
			if (car->_scheduledEventTime > 0.0) {
				if (car->_scheduledEventTime < s->currentTime) {
					car->_state &= ~RM_CAR_STATE_PIT;
					car->_pit->pitCarIndex = TR_PIT_STATE_FREE;
					snprintf(msg, sizeof(msg), "%s pit stop %.1f s", car->_name, info->totalPitTime);
					msg[sizeof(msg)-1] = 0; // Some snprintf implementations fail to do so.
					ReSituation::self().setRaceMessage(msg, 5);
					GfLogInfo("%s exiting pit (%.1f s elapsed).\n", car->_name, info->totalPitTime);
				} else {
					snprintf(car->ctrl.msg[2], RM_CMD_MAX_MSG_SIZE, "In pits %.1f s",
							s->currentTime - info->startPitTime);
					car->ctrl.msg[2][RM_CMD_MAX_MSG_SIZE-1] = 0; // Some snprintf implementations fail to do so.
				}
			}
			
		// If the driver asks for a pit, check if the car is in the right conditions
		// (position, speed, ...) and start up pitting process if so.
		} else if ((car->ctrl.raceCmd & RM_CMD_PIT_ASKED) &&
				   car->_pit->pitCarIndex == TR_PIT_STATE_FREE &&	
				   (s->_maxDammage == 0 || car->_dammage <= s->_maxDammage)) {
			snprintf(car->ctrl.msg[2], RM_CMD_MAX_MSG_SIZE, "Pit request");
			car->ctrl.msg[2][RM_CMD_MAX_MSG_SIZE-1] = 0; // Some snprintf implementations fail to do so.
 
			tdble lgFromStart = car->_trkPos.seg->lgfromstart;
			
			switch (car->_trkPos.seg->type) {
				case TR_STR:
					lgFromStart += car->_trkPos.toStart;
					break;
				default:
					lgFromStart += car->_trkPos.toStart * car->_trkPos.seg->radius;
					break;
			}
		
			if ((lgFromStart > car->_pit->lmin) && (lgFromStart < car->_pit->lmax)) {
				int side;
				tdble toBorder;
				if (ReInfo->track->pits.side == TR_RGT) {
					side = TR_SIDE_RGT;
					toBorder = car->_trkPos.toRight;
				} else {
					side = TR_SIDE_LFT;
					toBorder = car->_trkPos.toLeft;
				}
				
				sseg = car->_trkPos.seg->side[side];
				wseg = RtTrackGetWidth(sseg, car->_trkPos.toStart);
				if (sseg->side[side]) {
					sseg = sseg->side[side];
					wseg += RtTrackGetWidth(sseg, car->_trkPos.toStart);
				}
				if (((toBorder + wseg) < (ReInfo->track->pits.width - car->_dimension_y / 2.0)) &&
					(fabs(car->_speed_x) < 1.0) && (fabs(car->_speed_y) < 1.0))
				{
					// All conditions fullfilled => enter pitting process
					car->_state |= RM_CAR_STATE_PIT;
					car->_scheduledEventTime = 0.0; // Pit will really start when set to smthg > 0.
					car->_nbPitStops++;
					for (i = 0; i < car->_pit->freeCarIndex; i++) {
						if (car->_pit->car[i] == car) {
							car->_pit->pitCarIndex = i;
							break;
						}
					}
					info->startPitTime = s->currentTime;
					snprintf(msg, sizeof(msg), "%s in pits", car->_name);
					msg[sizeof(msg)-1] = 0; // Some snprintf implementations fail to do so.
					ReSituation::self().setRaceMessage(msg, 5);
					GfLogInfo("%s entering in pit slot.\n", car->_name);
					if (car->robot->rbPitCmd(car->robot->index, car, s) == ROB_PIT_MENU) {
						// the pit cmd is modified by menu.
						reCarsSchedulePitMenu(car);
					} else {
						ReCarsUpdateCarPitTime(car);
					}
				}
				else
				{   // The cars speed or offset is out of accepted range
					// Show the user/developer/robot the reason of the issue
  				    tTeamDriver* TeamDriver = RtTeamDriverByCar(car);
					if (TeamDriver)
					{
					  TeamDriver->StillToGo  = 0.0;
					  TeamDriver->MoreOffset = 0.0;
					  TeamDriver->TooFastBy  = 0.0;
					}

					float Offset = (float) ((toBorder + wseg) - (ReInfo->track->pits.width - car->_dimension_y / 2.0));
  				    if (Offset >= 0.0)
					{
						// The car's position across the track is out of accepted range 
						snprintf(car->ctrl.msg[2], RM_CMD_MAX_MSG_SIZE, "Offset: %.02f",Offset);
						car->ctrl.msg[2][RM_CMD_MAX_MSG_SIZE-1] = 0; // Some snprintf implementations fail to do so.
						if (TeamDriver)
						  TeamDriver->MoreOffset = Offset;
					}

					float TooFastBy = MAX(fabs(car->_speed_x),fabs(car->_speed_y));
  				    if (TooFastBy >= 1.0)
					{
						// The car's speed is out of accepted range 
						snprintf(car->ctrl.msg[2], RM_CMD_MAX_MSG_SIZE, "Speed: %.02f",TooFastBy);
						car->ctrl.msg[2][RM_CMD_MAX_MSG_SIZE-1] = 0; // Some snprintf implementations fail to do so.
						if (TeamDriver)
						  TeamDriver->TooFastBy = TooFastBy;
					}
				}
			}
			else
			{	// The car's position along the track is out of accepted range
				// Show the user/developer/robot the reason of the issue
				tTeamDriver* TeamDriver = RtTeamDriverByCar(car);
				if (TeamDriver)
				{
				  TeamDriver->StillToGo  = 0.0;
				  TeamDriver->MoreOffset = 0.0;
				  TeamDriver->TooFastBy  = 0.0;
				}

				if (car->_pit->lmin > lgFromStart)
				{
				  float StillToGo = car->_pit->lmin - lgFromStart;
				  snprintf(car->ctrl.msg[2], RM_CMD_MAX_MSG_SIZE, "Still to go: %0.2f m" ,StillToGo);
				  car->ctrl.msg[2][RM_CMD_MAX_MSG_SIZE-1] = 0; // Some snprintf implementations fail to do so.
				  if (TeamDriver)
				    TeamDriver->StillToGo = StillToGo;
				}
				else if (car->_pit->lmax < lgFromStart)
				{
  				  float StillToGo = lgFromStart - car->_pit->lmax;
				  snprintf(car->ctrl.msg[2], RM_CMD_MAX_MSG_SIZE, "Overrun: %0.2f m" ,StillToGo);
				  car->ctrl.msg[2][RM_CMD_MAX_MSG_SIZE-1] = 0; // Some snprintf implementations fail to do so.
				  if (TeamDriver)
				    TeamDriver->StillToGo = -StillToGo;
				}
			}
		}
	}

	/* Check if it is in a new sector */
	while (true)
	{
		if (car->_currentSector < ReInfo->track->numberOfSectors - 1 && car->_laps > 0 && info->lapFlag == 0)
		{
			/* Must pass at least one sector before the finish */
			if (RtGetDistFromStart(car) > ReInfo->track->sectors[car->_currentSector])
			{
				/* It is in a new sector : update split time */
				car->_curSplitTime[car->_currentSector] = car->_curLapTime;
				++car->_currentSector;
				continue;
			}
		}
		break;
	}
	
	/* Start Line Crossing */
	if (info->prevTrkPos.seg != car->_trkPos.seg) {
		
		if ((info->prevTrkPos.seg->raceInfo & TR_LAST)
			&& (car->_trkPos.seg->raceInfo & TR_START)) {
			
			if (info->lapFlag == 0) {

				// If the car has not yet finished the race :
				if (!(car->_state & RM_CAR_STATE_FINISH)) {

					// 1 more lap completed
					// (Note: lap with index 0 finishes when the car crosses the start line the 1st time,
					//        and is thus considered a real lap, whereas it is not).
					car->_laps++;

					car->_remainingLaps--;
					if (car->_pos == 1 && s->currentTime < s->_totTime
						&& s->_raceType == RM_TYPE_RACE)
					{
						/* First car passed finish time before the time ends: increase the number of laps for everyone */
						for (xx = 0; xx < s->_ncars; ++xx)
							++ReInfo->s->cars[xx]->_remainingLaps;
						++s->_totLaps;
					}
					
					car->_currentSector = 0;
					if (car->_laps > 1) {
						car->_lastLapTime = s->currentTime - info->sTime;
						if (car->_bestLapTime != 0) {
							car->_deltaBestLapTime = car->_lastLapTime - car->_bestLapTime;
						}
						if ((car->_lastLapTime < car->_bestLapTime) || (car->_bestLapTime == 0)) {
							car->_bestLapTime = car->_lastLapTime;
							memcpy(car->_bestSplitTime, car->_curSplitTime, sizeof(double)*(ReInfo->track->numberOfSectors - 1) );
							if (s->_raceType != RM_TYPE_RACE && s->_ncars > 1)
							{
								/* Best lap time is made better : update times behind leader */
								bestLapChanged = true;
								car->_timeBehindLeader = car->_bestLapTime - s->cars[0]->_bestLapTime;
								if (car->_pos > 1)
								{
									car->_timeBehindPrev = car->_bestLapTime - s->cars[car->_pos - 1]->_bestLapTime;
								}
								else
								{
									/* New best time for the leader : update the differences */
									for (xx = 1; xx < s->_ncars; ++xx)
									{
										if (s->cars[xx]->_bestLapTime > 0.0f)
											s->cars[xx]->_timeBehindLeader = s->cars[xx]->_bestLapTime - car->_bestLapTime;
									}
								}
								if (car->_pos + 1 < s->_ncars && s->cars[car->_pos+1]->_bestLapTime > 0.0f)
									car->_timeBeforeNext = s->cars[car->_pos + 1]->_bestLapTime - car->_bestLapTime;
								else
									car->_timeBeforeNext = 0;
							}
						}
					}
					if (car->_laps > 0) {
						car->_curTime += s->currentTime - info->sTime;
						
						if (car->_pos != 1 && s->_raceType == RM_TYPE_RACE) {
							car->_timeBehindLeader = car->_curTime - s->cars[0]->_curTime;
							car->_lapsBehindLeader = s->cars[0]->_laps - car->_laps;
							car->_timeBehindPrev = car->_curTime - s->cars[car->_pos - 2]->_curTime;
							s->cars[car->_pos - 2]->_timeBeforeNext = car->_timeBehindPrev;
						} else if (s->_raceType == RM_TYPE_RACE) {
							car->_timeBehindLeader = 0;
							car->_lapsBehindLeader = 0;
							car->_timeBehindPrev = 0;
						}
						
						info->sTime = (tdble)s->currentTime;
/*
						if (ReInfo->s->_raceType == RM_TYPE_PRACTICE && 
								(car->_laps > 1 || s->_totLaps == 0))
							ReSavePracticeLap(car);
*/
					}

					if (ReInfo->_displayMode == RM_DISP_MODE_NONE)
					{
						switch(s->_raceType)
						{
							case RM_TYPE_PRACTICE:
								//ReUpdatePracticeCurRes(car);
								break;
							case RM_TYPE_QUALIF:
								//ReUpdateQualifCurRes(car);
								break;
							case RM_TYPE_RACE:
								//ReUpdateRaceCurRes();
								break;
							default:
								break;
						}
					}
	
					info->topSpd = car->_speed_x;
					info->botSpd = car->_speed_x;
					if ((car->_remainingLaps < 0 && s->currentTime > s->_totTime) || (s->_raceState == RM_RACE_FINISHING)) {
						car->_state |= RM_CAR_STATE_FINISH;
						s->_raceState = RM_RACE_FINISHING;
						if (ReInfo->s->_raceType == RM_TYPE_RACE) {
							if (car->_pos == 1) {
								snprintf(msg, sizeof(msg), "Winner %s", car->_name);
								msg[sizeof(msg)-1] = 0; // Some snprintf implementations fail to do so.
								//ReSituation::self().setRaceMessage(msg, 10, /*big=*/true);
							} else {
								const char *numSuffix = "th";
								if (abs(12 - car->_pos) > 1) { /* leave suffix as 'th' for 11 to 13 */
									switch (car->_pos % 10) {
										case 1:
											numSuffix = "st";
											break;
										case 2:
											numSuffix = "nd";
											break;
										case 3:
											numSuffix = "rd";
											break;
										default:
											break;
									}
								}
								snprintf(msg, sizeof(msg), "%s finished %d%s", car->_name, car->_pos, numSuffix);
								msg[sizeof(msg)-1] = 0; // Some snprintf implementations fail to do so.
								ReSituation::self().setRaceMessage(msg, 5);
							}
						}
					}
					
					// Notify the UI when a lap is completed (by the leader)
					// and race results have been updated.
					if (car->_pos == 1)
						ReUI().onLapCompleted(car->_laps - 1);

				} else {
					// Prevent infinite looping of cars around track,
					// allowing one lap after finish for the first car, but no more
					for (i = 0; i < s->_ncars; i++) {
						s->cars[i]->_state |= RM_CAR_STATE_FINISH;
					}
					return;
				}

			} else {
				info->lapFlag--;
			}
		}
		if ((info->prevTrkPos.seg->raceInfo & TR_START)
			&& (car->_trkPos.seg->raceInfo & TR_LAST)) {
			/* going backward through the start line */
			info->lapFlag++;
		}
	} // Start Line Crossing


	// Apply race rules (penalties if enabled).
	reCarsApplyRaceRules(car);

	// Update misc car info.
	info->prevTrkPos = car->_trkPos;
	car->_curLapTime = s->currentTime - info->sTime;
	car->_distFromStartLine = car->_trkPos.seg->lgfromstart +
		(car->_trkPos.seg->type == TR_STR ? car->_trkPos.toStart : car->_trkPos.toStart * car->_trkPos.seg->radius);
	car->_distRaced = (car->_laps - 1) * ReInfo->track->length + car->_distFromStartLine;
}

void 
ReCarsSortCars(void)
{
    int		i,j;
    int		xx;
    tCarElt	*car;
    tSituation	*s = ReInfo->s;
    char msg[64];
	
    // Check cars are driving the right way around track
    for (i = 0; i < s->_ncars; i++) {
	if (s->cars[i]->_prevFromStartLine < s->cars[i]->_distFromStartLine) {
	    s->cars[i]->_wrongWayTime = s->currentTime + 5.0;
	}
	
	s->cars[i]->_prevFromStartLine = s->cars[i]->_distFromStartLine;

	if (s->cars[i]->_wrongWayTime < s->currentTime 
		&& s->cars[i]->_speed_xy > 10 
		&& s->cars[i]->_driverType == RM_DRV_HUMAN
		&& s->cars[i]->_state != RM_CAR_STATE_ELIMINATED) {
	    snprintf(msg, sizeof(msg), "%s Wrong Way", s->cars[i]->_name);
		msg[sizeof(msg)-1] = 0; // Some snprintf implementations fail to do so.
	    ReSituation::self().setRaceMessage(msg, 2);
	    // prevent flickering occuring by 'short timing', assuming > 10fps
	    s->cars[i]->_wrongWayTime = s->currentTime + 1.9;
	}
    }

	int allfinish = (s->cars[0]->_state & RM_CAR_STATE_FINISH) ? 1 : 0;
    for (i = 1; i < s->_ncars; i++) {
	j = i;
	while (j > 0) {
	    if (!(s->cars[j]->_state & RM_CAR_STATE_FINISH)) {
		allfinish = 0;
		if ((ReInfo->s->_raceType == RM_TYPE_RACE && s->cars[j]->_distRaced > s->cars[j-1]->_distRaced) ||
		    (ReInfo->s->_raceType != RM_TYPE_RACE && s->cars[j]->_bestLapTime > 0.0f && ( s->cars[j]->_bestLapTime < s->cars[j-1]->_bestLapTime ||
		                                                                                  s->cars[j-1]->_bestLapTime <= 0.0f))) {
		    car = s->cars[j];
		    s->cars[j] = s->cars[j-1];
		    s->cars[j-1] = car;
		    s->cars[j]->_pos = j+1;
		    s->cars[j-1]->_pos = j;
		    if (s->_raceType != RM_TYPE_RACE)
		    {
		    	if (j-1 > 0)
			{
			    s->cars[j-1]->_timeBehindPrev = s->cars[j-1]->_bestLapTime - s->cars[j-2]->_bestLapTime;
			}
			else
			{
			    s->cars[j-1]->_timeBehindPrev = 0;
			    for (xx = 1; xx < s->_ncars; ++xx)
			    {
			    	/* New leader */
				if (s->cars[xx]->_bestLapTime > 0.0f)
			    	    s->cars[xx]->_timeBehindLeader = s->cars[xx]->_bestLapTime - s->cars[0]->_bestLapTime;
			    }
			}
			if (s->cars[j]->_bestLapTime)
			    s->cars[j-1]->_timeBeforeNext = s->cars[j-1]->_bestLapTime - s->cars[j]->_bestLapTime;
			else
			    s->cars[j-1]->_timeBeforeNext = 0;
			s->cars[j]->_timeBehindPrev = s->cars[j]->_bestLapTime - s->cars[j-1]->_bestLapTime;
			if (j+1 < s->_ncars && s->cars[j+1]->_bestLapTime > 0.0f)
			    s->cars[j]->_timeBeforeNext = s->cars[j]->_bestLapTime - s->cars[j+1]->_bestLapTime;
			else
			    s->cars[j]->_timeBeforeNext = 0;
		    }
		    j--;
		    continue;
		}
	    }
	    j = 0;
	}
    }
    if (allfinish) {
	ReInfo->s->_raceState = RM_RACE_ENDED;
	//GfLogDebug("ReCarsSortCars: Race completed.\n");
    }
}
