/***************************************************************************

    file        : textonly.h
    copyright   : (C) 2011 by Jean-Philippe Meuret
    email       : pouillot@users.sourceforge.net
    version     : $Id$

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
    @version    $Id$
*/

#ifndef _TEXTONLY_H_
#define _TEXTONLY_H_

#include <iuserinterface.h>

#include <tgf.hpp>

// DLL exported symbols declarator for Windows.
#ifdef WIN32
# ifdef TEXTONLY_DLL
#  define TEXTONLY_API __declspec(dllexport)
# else
#  define TEXTONLY_API __declspec(dllimport)
# endif
#else
# define TEXTONLY_API
#endif


// The C interface of the module.
extern "C" int TEXTONLY_API openGfModule(const char* pszShLibName, void* hShLibHandle);
extern "C" int TEXTONLY_API closeGfModule();

// The module main class (inherits GfModule, and implements IUserInterface).
class TEXTONLY_API TextOnlyUI : public GfModule, public IUserInterface
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
	virtual bool onRaceEventStarting(bool careerNonHuman);
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
	virtual bool onRaceEventFinished(bool bMultiEvent, bool careerNonHumanGroupo);
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
	static TextOnlyUI& self();

	//! Accessors to the race engine.
	IRaceEngine& raceEngine();
	const IRaceEngine& raceEngine() const;

 protected:

	//! Protected constructor to avoid instanciation outside (but friends).
	TextOnlyUI(const std::string& strShLibName, void* hShLibHandle);
	
	//! Make the C interface functions nearly member functions.
	friend int openGfModule(const char* pszShLibName, void* hShLibHandle);
	friend int closeGfModule();

	//! Recompute callback for the event loop.
	static void updateRaceEngine();

 protected:

	//! The singleton.
	static TextOnlyUI* _pSelf;

	//! The race engine.
	IRaceEngine* _piRaceEngine;

	//! The results table.
	class ResultsTable;
	ResultsTable* _pResTable;
};

//! Shortcut to the race engine.
inline extern IRaceEngine& ToRaceEngine()
{
	return TextOnlyUI::self().raceEngine();
}
				  
#endif /* _TEXTONLY_H_ */ 
