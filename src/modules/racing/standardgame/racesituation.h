/***************************************************************************

    file        : racesituation.h
    copyright   : (C) 2010 by Jean-Philippe Meuret
    web         : www.speed-dreams.org 
    version     : $Id: racesituation.h 5803 2014-07-30 03:19:34Z mungewell $

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
    		
    @author	    Jean-Philippe Meuret
    @version	$Id: racesituation.h 5803 2014-07-30 03:19:34Z mungewell $
*/

#ifndef _RACESITUATION_H_
#define _RACESITUATION_H_

#include <string>

#include "raceinit.h" // ReInfo.


// The race situation class ==========================================================
class ReSituation
{
public:
	
	//! Accessor to the singleton (not thread-safe at instanciation time).
	static ReSituation& self();

	//! Delete the singleton.
	static void terminate();
	
	//! Destructor.
	~ReSituation();

	// TODO: Remove when all necessary safe accessors ready.
	struct RmInfo* data();

	//! Activate/Deactivate the thread-safe mode.
	void setThreadSafe(bool bOn = true);

	//! Safe accessors.
	void setDisplayMode(unsigned bfDispMode);
	void accelerateTime(double fMultFactor);
	void setRaceMessage(const std::string& strMsg, double fLifeTime = -1, bool bBig = false);
	
	//! Set pit command for the given car (back from a pit menu or so).
	void setPitCommand(int nCarIndex, const tCarPitCmd *pPitCmd);

private:

	//! Protected constructor to avoid instanciation outside of self().
	ReSituation();

	//! Lock the data when thread-safe mode activated (otherwise do nothing).
	bool lock(const char* pszCallerName = 0);
	
	//! Unlock the data when thread-safe mode activated (otherwise do nothing).
	bool unlock(const char* pszCallerName = 0);

private:

	//! The singleton.
	static ReSituation* _pSelf;

	//! The real data behind.
	struct RmInfo *_pReInfo;
	
	//! The mutex for thread-safe access.
	struct SDL_mutex* _pMutex;

	// ReSituationUpdater is a very good friend 8-D.
	friend class ReSituationUpdater;
};

// The race situation updater class ==================================================
class ReSituationUpdater
{
public:
	
	//! Constructor.
	ReSituationUpdater();

	//! Destructor.
	~ReSituationUpdater();

	//! Set scheduling specs (see below _fSimuTick and _fOutputTick).
	bool setSchedulingSpecs(double fSimuRate, double fOutputRate = 0);

	//! (Re)start (after a stop) the updater if it is not running.
	void start();
	
	//! Stop (pause) the updater if it is running (return its exit status).
	void stop();
	
	//! Terminate the updater (return its exit status ; wait for the thread to return).
	int terminate();
	
	//! Get the situation for the previous step
	struct RmInfo* getPreviousStep();
	
	//! Compute 1 situation update step.
	void runOneStep(double deltaTimeIncrement);

	//! Start computing the situation for the current step
	void computeCurrentStep();

private:

	//! The thread function.
	int threadLoop();
	
	//! The C wrapper on the thread function.
	static int threadLoop(void*);
	
	//! Acknowledge the situation events (simu / graphics synchronization).
	void acknowledgeEvents();

	//! Allocate and initialize a situation (set constants from source).
	struct RmInfo* initSituation(const struct RmInfo* pSource);
	
	//! (Deep) Copy the given situation.
	struct RmInfo* copySituation(struct RmInfo*& pTarget, const struct RmInfo* pSource);

	// Record the situation to a replay database
	void replaySituation(struct RmInfo*& pTarget);

	// Record the situation to a replay database
	void ghostcarSituation(struct RmInfo*& pTarget);

	//! Free the given situation
	void freezSituation(struct RmInfo*& pSituation);
	
private:

	//! Initial number of drivers racing
	int _nInitDrivers;

	//! The previous step of the situation
	struct RmInfo* _pPrevReInfo;

	//! The situation updater thread
	struct SDL_Thread* _pUpdateThread;

	//! True if the updater is actually threaded (may be not the case)
	bool _bThreaded;

	//! True if thread affinity has to be applied (even in case !_bThreaded)
	bool _bThreadAffinity;

	//! Flag to set in order to terminate the updater.
	bool _bTerminate;

	//! Simulation tick (s) (defaults to RCM_MAX_DT_SIMU).
	double _fSimuTick;

	//! Output tick (s) : defines the computeCurrentStep output "rate"
	// - if 0, a nearly real-time rate is enforced, at the expense of variable output rate,
	//   (used in normal real-time gaming mode)
	// - if > 0, the output rate is enforced, at the expense of not keeping the real time.
	//   (used in movie capture gaming mode, to get a stable but slowed-down frame rate).
	double _fOutputTick;

	//! Time of the last output when using the "stable but slowed-down frame rate" mode.
	double _fLastOutputTime;

};

#endif /* _RACESITUATION_H_ */ 



