/***************************************************************************

    file        : racenetwork.cpp
    copyright   : (C) 2009 by Brian Gavin 
    web         : www.speed-dreams.org 
    version     : $Id: racenetwork.cpp 5081 2012-12-30 18:24:16Z pouillot $

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
    		
    @author	    Brian Gavin
    @version	$Id: racenetwork.cpp 5081 2012-12-30 18:24:16Z pouillot $
*/

#include <network.h>

#include "standardgame.h"

#include "racesituation.h"
#include "racenetwork.h"


static void
reNetworkSetCarPhysics(double timeDelta,CarControlsData *pCt)
{
	tDynPt *pDynCG = RePhysicsEngine().getCar(pCt->startRank);

	// double errX = pDynCG->pos.x-pCt->DynGCg.pos.x;
	// double errY = pDynCG->pos.y-pCt->DynGCg.pos.y;
	// double errZ = pDynCG->pos.z-pCt->DynGCg.pos.z;

	int idx = NetGetNetwork()->GetCarIndex(pCt->startRank,ReInfo->s);
	
	//Car controls (steering,gas,brake, gear
	tCarElt *pCar = ReInfo->s->cars[idx];
	pCar->ctrl.accelCmd = pCt->throttle;
	pCar->ctrl.brakeCmd = pCt->brake;
	pCar->ctrl.clutchCmd = pCt->clutch;
	pCar->ctrl.gear = pCt->gear;
	pCar->ctrl.steer = pCt->steering;

	pDynCG->pos = pCt->DynGCg.pos;
	pDynCG->acc = pCt->DynGCg.acc;
	pDynCG->vel = pCt->DynGCg.vel;

	double step = 0.0;
	if (timeDelta>0.0)
	{
		//predict car position
		while(timeDelta>0.0)
		{
			if (timeDelta>RCM_MAX_DT_SIMU)
			{
				step = RCM_MAX_DT_SIMU;
			}
			else
				step = timeDelta;

			timeDelta-=step;
			RePhysicsEngine().updateCar(ReInfo->s, step, pCt->startRank);
		}
	}

	//GfLogTrace("Network position error is %lf %lf %lf and delta is %lf\n",errX,errY,errZ,timeDelta);

	//Car physics
//	RePhysicsEngine().setCar(pCt->DynGCg, pCt->startRank);
}

static void
reNetworkSetCarStatus(CarStatus *pStatus)
{
	int idx = NetGetNetwork()->GetCarIndex(pStatus->startRank,ReInfo->s);

	tCarElt *pCar = ReInfo->s->cars[idx];

	if (pStatus->dammage > 0.0)
		pCar->priv.dammage = pStatus->dammage;
	if (pStatus->fuel >0.0)
		pCar->priv.fuel = pStatus->fuel;
	if (pStatus->topSpeed >0.0)
		pCar->race.topSpeed = pStatus->topSpeed;

	pCar->pub.state = pStatus->state;
	

}

static void
reNetworkSetLapStatus(LapStatus *pStatus)
{
	int idx = NetGetNetwork()->GetCarIndex(pStatus->startRank,ReInfo->s);

	tCarElt *pCar = ReInfo->s->cars[idx];
	pCar->race.bestLapTime = pStatus->bestLapTime;
	*pCar->race.bestSplitTime = (double)pStatus->bestSplitTime;
	pCar->race.laps = pStatus->laps;
	GfLogTrace("Setting network lap status\n");
}

void
ReNetworkOneStep()
{
	tSituation *s = ReInfo->s;

	//Do network updates if needed
	//CarControlsData *pControls = NULL;
	int numCars = 0;
	
	NetMutexData *pNData = NetGetNetwork()->LockNetworkData();

	numCars = pNData->m_vecCarCtrls.size();
	if (numCars>0)
	{
		for (int i=0;i<numCars;i++)
		{
			double timeDelta = s->currentTime-pNData->m_vecCarCtrls[i].time;
			if (timeDelta >= 0)
			{
				reNetworkSetCarPhysics(timeDelta,&pNData->m_vecCarCtrls[i]);
			}
			else if (timeDelta <= -1.0)
			{
				GfLogTrace("Ignoring physics packet (delta is %lf)\n", timeDelta);
			}
		}
	}

	NetGetNetwork()->SetCurrentTime(s->currentTime);
	pNData->m_vecCarCtrls.clear();

	//do car status updates if needed
	numCars = pNData->m_vecCarStatus.size();

	if (numCars>0)
	{
		for (int i=0;i<numCars;i++)
		{
			double delta = s->currentTime-pNData->m_vecCarStatus[i].time;
			if (delta>=0)
				reNetworkSetCarStatus(&pNData->m_vecCarStatus[i]);
		}
	}

	std::vector<CarControlsData>::iterator p = pNData->m_vecCarCtrls.begin();
	while(p!=pNData->m_vecCarCtrls.end())
	{
		if(p->time<s->currentTime)
			p = pNData->m_vecCarCtrls.erase(p);
		else 
			p++;
	}

	//do lap status updates if needed
	numCars = 0;
	numCars = pNData->m_vecLapStatus.size();
	if (numCars>0)
	{
		for (int i=0;i<numCars;i++)
		{
			reNetworkSetLapStatus(&pNData->m_vecLapStatus[i]);
		}
	}

	pNData->m_vecLapStatus.clear();

	NetGetNetwork()->UnlockNetworkData();
}

int
ReNetworkWaitReady()
{
	// No wait if not an online race.
	if (!NetGetNetwork())
		return RM_SYNC | RM_NEXT_STEP;

	// If network race, wait for other players and start when the server tells to
	bool bWaitFinished = false;
	if (NetGetClient())
	{
		NetGetClient()->SendReadyToStartPacket();
		ReInfo->s->currentTime = NetGetClient()->WaitForRaceStart();
		GfLogInfo("Client beginning race in %lf seconds!\n", - ReInfo->s->currentTime);
		bWaitFinished = true;
	}
	
	else if (NetGetServer())
	{
		if (NetGetServer()->ClientsReadyToRace())
		{
			ReInfo->s->currentTime = NetGetServer()->WaitForRaceStart();
			GfLogInfo("Server beginning race in %lf seconds!\n", - ReInfo->s->currentTime);
			bWaitFinished = true;
		}
	}

	if (bWaitFinished)
	{
		ReSituation::self().setRaceMessage("", -1/*always*/, /*big=*/true);
		return RM_SYNC | RM_NEXT_STEP;
	}
	else
	{
		ReSituation::self().setRaceMessage("Waiting for online players",
										   -1/*always*/, /*big=*/true);
		return RM_ASYNC;
	}
}

void
ReNetworkCheckEndOfRace()
{
	// Check for end of online race.
	if (NetGetNetwork() && NetGetNetwork()->FinishRace(ReInfo->s->currentTime))
		ReInfo->s->_raceState = RM_RACE_ENDED;
}
