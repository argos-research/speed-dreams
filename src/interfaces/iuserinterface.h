/***************************************************************************
                 iuserinterface.h -- Interface for any user interface module

    created              : Mon Mar 7 19:32:14 CEST 2011
    copyright            : (C) 2011 by Jean-Philippe Meuret                         
    web                  : http://www.speed-dreams.org
    version              : $Id: iuserinterface.h 6084 2015-08-21 00:07:15Z beaglejoe $
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
    	Interface for any user interface module
    @version	$Id: iuserinterface.h 6084 2015-08-21 00:07:15Z beaglejoe $
*/

#ifndef __IUSERINTERFACE__H__
#define __IUSERINTERFACE__H__

class IRaceEngine;

struct RmInfo;
struct CarElt;

class IUserInterface
{
public:

	//! Activation of the user interface (splash if any, main menu ...).
	virtual bool activate() = 0;

	//! Request exit of the event loop.
	virtual void quit() = 0;

	//! Termination of the user interface.
	virtual void shutdown() = 0;

	// Race state change notifications.
	virtual void onRaceConfiguring() = 0;
	virtual void onRaceEventInitializing() = 0;
	virtual bool onRaceEventStarting(bool careerNonHuman) = 0;
	virtual void onRaceInitializing() = 0;
	virtual bool onRaceStarting() = 0;
	virtual void onRaceLoadingDrivers() = 0;
	virtual void onRaceDriversLoaded() = 0;
	virtual void onRaceSimulationReady() = 0;
	virtual void onRaceStarted() = 0;
	virtual void onRaceResuming() = 0;
	virtual void onLapCompleted(int nLapIndex) = 0;
	virtual void onRaceInterrupted() = 0;
	virtual void onRaceFinishing() = 0;
	virtual bool onRaceFinished(bool bEndOfSession) = 0;
	virtual void onRaceEventFinishing() = 0;
	virtual bool onRaceEventFinished(bool bMultiEvent, bool careerNonHuman) = 0;
	virtual void onOptimizationInitializing() = 0;

	virtual bool onRaceStartingPaused() = 0;
	virtual bool onRaceCooldownStarting() = 0;

	// Loading messages management.
	virtual void addLoadingMessage(const char* pszText) = 0;
	
	// Optimiaztion messages management.
	virtual void addOptimizationMessage(const char* pszText) = 0;
    virtual void addOptimizationParameterMessage(int n, char** Labels, char** Values, char** Ranges) = 0;
    virtual void addOptimizationStatusMessage(int LoopsDone, int LoopsRemaining, double VariationScale, double InitialLapTime,  double TotalLapTime,  double BestLapTime) = 0;

	// Blind-race results table management.
	virtual void setResultsTableTitles(const char* pszTitle, const char* pszSubTitle) = 0;
	virtual void setResultsTableHeader(const char* pszHeader) = 0;
	virtual void addResultsTableRow(const char* pszText) = 0;
	virtual void setResultsTableRow(int nIndex, const char* pszText,
									bool bHighlight = false) = 0;
	virtual void removeResultsTableRow(int nIndex) = 0;
	virtual void eraseResultsTable() = 0;
	virtual int  getResultsTableRowCount() const = 0;

	//! Race engine setter.
	virtual void setRaceEngine(IRaceEngine& raceEngine) = 0;
};

#include <iraceengine.h>

#endif // __IUSERINTERFACE__H__
