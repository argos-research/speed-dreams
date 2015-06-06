/***************************************************************************

    file                 : textonly.cpp
    copyright            : (C) 2011 by Jean-Philippe Meuret                        
    email                : pouillot@users.sourceforge.net   
    version              : $Id$

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
#include <vector>
#include <sstream>

#include <raceman.h>
#include <iraceengine.h>

#include <portability.h>

#include <racemanagers.h>
#include <race.h>

#include "textonly.h"


// The TextOnlyUI singleton.
TextOnlyUI* TextOnlyUI::_pSelf = 0;

// The results table class.
class TextOnlyUI::ResultsTable
{
 public:
	std::string strTitle;
	std::string strSubTitle;
	std::string strHeader;
	std::vector<std::string> vecLines;
};


int openGfModule(const char* pszShLibName, void* hShLibHandle)
{
	// Instanciate the (only) module instance.
	TextOnlyUI::_pSelf = new TextOnlyUI(pszShLibName, hShLibHandle);

	// Register it to the GfModule module manager if OK.
	if (TextOnlyUI::_pSelf)
		GfModule::register_(TextOnlyUI::_pSelf);

	// Report about success or error.
	return TextOnlyUI::_pSelf ? 0 : 1;
}

int closeGfModule()
{
	// Unregister it from the GfModule module manager.
	if (TextOnlyUI::_pSelf)
		GfModule::unregister(TextOnlyUI::_pSelf);

	// Delete the (only) module instance.
	delete TextOnlyUI::_pSelf;
	TextOnlyUI::_pSelf = 0;

	// Report about success or error.
	return 0;
}

TextOnlyUI& TextOnlyUI::self()
{
	// Pre-condition : 1 successfull openGfModule call.
	return *_pSelf;
}

TextOnlyUI::TextOnlyUI(const std::string& strShLibName, void* hShLibHandle)
: GfModule(strShLibName, hShLibHandle), _piRaceEngine(0), _pResTable(new ResultsTable)
{
}

// Implementation of IUserInterface ****************************************
bool TextOnlyUI::activate()
{
    // Get the race to start.
    std::string strRaceToStart;
	(void)GfApp().hasOption("startrace", strRaceToStart); // Should always be true (see main).
		
    // Check if it's an available one, and refuse activation if not.
	GfRaceManager* pSelRaceMan =  GfRaceManagers::self()->getRaceManager(strRaceToStart);
    if (!pSelRaceMan)
	{
        GfLogError("No such race type '%s'\n", strRaceToStart.c_str());
		
		return false;
	}

	// Otherwise, run the selected race.
	// * Initialize the race engine.
	raceEngine().reset();

	// * Give the selected race manager to the race engine.
	raceEngine().selectRaceman(pSelRaceMan, /*bKeepHumans=*/false);
		
	// * Configure the new race (no user interaction needed).
	raceEngine().configureRace(/* bInteractive */ false);

	// * Force "result-only" mode for all the sessions of the race with initial "normal" mode
	//   (don't change the ones with "simu simu" mode).
	raceEngine().race()->forceResultsOnly();

	// * Start the race engine state automaton
	raceEngine().startNewRace();

	return true;
}

void TextOnlyUI::quit()
{
	// Quit the event loop next time.
	GfApp().eventLoop().postQuit();
}

void TextOnlyUI::shutdown()
{
    raceEngine().cleanup();
}

void TextOnlyUI::onRaceConfiguring()
{
	//GfLogDebug("TextOnlyUI::onRaceConfiguring()\n");

	// When all the race events are over, the race engine state is set to CONFIG.
	quit();
}

void TextOnlyUI::onRaceEventInitializing()
{
	// Actually nothing to do.
	GfLogDebug("TextOnlyUI::onRaceEventInitializing()\n");
}

bool TextOnlyUI::onRaceEventStarting(bool careerNonHumanGroup)
{
	GfLogDebug("TextOnlyUI::onRaceEventStarting()\n");

	// Tell the race engine state automaton to go on looping.
	return true;
}

void TextOnlyUI::onRaceInitializing()
{
	// Actually nothing to do.
	GfLogDebug("TextOnlyUI::onRaceInitializing()\n");
}

bool TextOnlyUI::onRaceStarting()
{
	GfLogDebug("TextOnlyUI::onRaceStarting()\n");

	// Tell the race engine state automaton to go on looping.
	return true;
}

void TextOnlyUI::onRaceLoadingDrivers()
{
	// Actually nothing to do.
	GfLogDebug("TextOnlyUI::onRaceLoadingDrivers()\n");
}

void TextOnlyUI::onRaceDriversLoaded()
{
	// Actually nothing to do.
	GfLogDebug("TextOnlyUI::onRaceDriversLoaded()\n");
}

void TextOnlyUI::onRaceSimulationReady()
{
	// Actually nothing to do.
	GfLogDebug("TextOnlyUI::onRaceSimulationReady()\n");
}

void TextOnlyUI::updateRaceEngine()
{
    ToRaceEngine().updateState();
}

void TextOnlyUI::onRaceStarted()
{
	GfLogDebug("TextOnlyUI::onRaceStarted()\n");

	// Configure the event loop : compute = update the race engine.
	GfApp().eventLoop().setRecomputeCB(TextOnlyUI::updateRaceEngine);
}

void TextOnlyUI::onRaceResuming()
{
	GfLogWarning("TextOnlyUI::onRaceResuming : Should never be called\n");
}

void TextOnlyUI::onLapCompleted(int nLapIndex)
{
	if (nLapIndex <= 0)
		return;

	GfLogInfo("Lap #%d completed.\n", nLapIndex);
	
    // Dump the results table.
	GfLogInfo("%s - %s\n", _pResTable->strTitle.c_str(), _pResTable->strSubTitle.c_str());
	GfLogInfo("%s\n", _pResTable->strHeader.c_str());
	
	std::vector<std::string>::const_iterator itLine;
	for (itLine = _pResTable->vecLines.begin(); itLine != _pResTable->vecLines.end(); itLine++)
		GfLogInfo("%s\n", itLine->c_str());
}

void TextOnlyUI::onRaceInterrupted()
{
	GfLogWarning("TextOnlyUI::onRaceInterrupted : Should never be called\n");
}

void TextOnlyUI::onRaceFinishing()
{
	GfLogDebug("TextOnlyUI::onRaceFinishing()\n");
	
	// Configure the event loop : compute = nothing.
	GfApp().eventLoop().setRecomputeCB(0);
}

bool TextOnlyUI::onRaceFinished(bool bEndOfSession)
{
	GfLogDebug("TextOnlyUI::onRaceFinished(%send of session)\n", bEndOfSession ? "" : "not ");

	if (bEndOfSession)
	{
		// TODO: Dump results table in the console (something like RmShowResults) ?
	}
	
	// Tell the race engine state automaton to go on looping.
	return true;
}

void TextOnlyUI::onRaceEventFinishing()
{
	// Actually nothing to do.
	GfLogDebug("TextOnlyUI::onRaceEventFinishing()\n");
}

bool TextOnlyUI::onRaceEventFinished(bool bMultiEvent, bool careerNonHumanGroup)
{
	GfLogDebug("TextOnlyUI::onRaceEventFinished(%smulti-event)\n", bMultiEvent ? "" : "not ");

	if (bMultiEvent)
	{
		// TODO: Dump results table in the console (something like RmShowStandings) ?
	}
	
	// Tell the race engine state automaton to go on looping.
	return true;
}

void TextOnlyUI::addLoadingMessage(const char* pszText)
{
    GfLogTrace("%s\n", pszText);
}

void TextOnlyUI::setResultsTableTitles(const char* pszTitle, const char* pszSubTitle)
{
	_pResTable->strTitle = pszTitle;
	_pResTable->strSubTitle = pszSubTitle;

	if (_pResTable->vecLines.size() < (unsigned)getResultsTableRowCount())
		_pResTable->vecLines.resize(getResultsTableRowCount());
    GfLogDebug("TextOnlyUI::setResultsTableTitles : nMaxRows=%d\n", getResultsTableRowCount());
}

void TextOnlyUI::setResultsTableHeader(const char* pszHeader)
{
	_pResTable->strHeader = pszHeader;
}

void TextOnlyUI::addResultsTableRow(const char* pszText)
{
	_pResTable->vecLines.push_back(pszText);
}

void TextOnlyUI::setResultsTableRow(int nIndex, const char* pszText, bool bHighlight)
{
	if (nIndex < 0 || nIndex >= (int)_pResTable->vecLines.size())
	{
		GfLogWarning("TextOnlyUI::setResultsTableRow : No such row %d in [0, %d] ; resizing.\n",
					 nIndex, (int)_pResTable->vecLines.size() - 1);
		_pResTable->vecLines.resize(nIndex + 1);
	}
		
	_pResTable->vecLines[nIndex] = pszText;
	if (bHighlight)
		_pResTable->vecLines[nIndex] += " *";
}

void TextOnlyUI::removeResultsTableRow(int nIndex)
{
	if (nIndex < 0 || nIndex >= (int)_pResTable->vecLines.size())
	{
		GfLogWarning("TextOnlyUI::removeResultsTableRow : No such row %d in [0, %d] ; ignoring.\n",
					 nIndex, (int)_pResTable->vecLines.size() - 1);
		return;
	}

	_pResTable->vecLines.erase(_pResTable->vecLines.begin() + nIndex);
}

int TextOnlyUI::getResultsTableRowCount() const
{
	// Unlike the menu GUI, we are not limited in any way here
	// (but of course the number of competitors).
	int nMaxRows = (int)raceEngine().race()->getCompetitorsCount();

	// But with the Career mode, which has not been moved yet to using tgfdata (TODO),
	// the number of competitors is not correctly initialized in tgfdata ...
	if (nMaxRows <= 0)
		nMaxRows = 50; // Whatever : if not enough, the result table will silently grow.

	return nMaxRows;
}

void TextOnlyUI::eraseResultsTable()
{
	_pResTable->vecLines.clear();
}

//=========================================================================
void TextOnlyUI::setRaceEngine(IRaceEngine& raceEngine)
{
	_piRaceEngine = &raceEngine;
}

// Accessors to the race engine.
IRaceEngine& TextOnlyUI::raceEngine()
{
	return *_piRaceEngine;
}

const IRaceEngine& TextOnlyUI::raceEngine() const
{
	return *_piRaceEngine;
}
