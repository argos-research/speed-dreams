/***************************************************************************
                      guiapplication.cpp -- GUI Application base
                             -------------------
    created              : Sat Apr 16 19:30:04 CEST 2011
    copyright            : (C) 2011 by Jean-Philippe Meuret
    web                  : http://www.speed-dreams.org
    version              : $Id: guiapplication.cpp 5163 2013-02-19 18:28:35Z pouillot $
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "tgfclient.h"


GfuiApplication::GfuiApplication(const char* pszName, const char* pszVersion, const char* pszDesc)
: GfApplication((pszName ? pszName : "GfuiApplication"), pszVersion, pszDesc),
  _bWindowUp(false)
{
}

void GfuiApplication::initialize(bool bLoggingEnabled, int argc, char **argv)
{
	// Base initialization first.
	GfApplication::initialize(bLoggingEnabled, argc, argv);
	
	// Register command line options.
	registerOption("m", "hardmouse", /* nHasValue = */ false);
	
	// Help about these options.
	addOptionsHelpSyntaxLine("[-m|--hardmouse]");
	addOptionsHelpExplainLine("- hardmouse : Use hardware mouse cursor");
}

bool GfuiApplication::parseOptions()
{
	// Parse command line for registered options, and interpret standard ones.
	if (!GfApplication::parseOptions())
		return false;

	// Then interpret the specific ones.
	std::list<Option>::const_iterator itOpt;
	for (itOpt = _lstOptions.begin(); itOpt != _lstOptions.end(); itOpt++)
	{
		// Not found in the command line => ignore / default value.
		if (!itOpt->bFound)
			continue;
		
        // Allow the hardware mouse cursor
		if (itOpt->strLongName == "hardmouse")
        {
			GfuiMouseSetHWPresent();
        }
	}

	return true;
}

bool GfuiApplication::setupWindow(bool bNoMenu, int nWinWidth, int nWinHeight, int nFullScreen)
{
	// Initialize the window/screen.
	_bWindowUp = true; // In case, GfScrInit() would call restart() ...
	_bWindowUp = GfScrInit(nWinWidth, nWinHeight, nFullScreen);

	// Initialize the UI menu infrastructure.
	if (_bWindowUp && !bNoMenu)
		GfuiInit();

	return _bWindowUp;
}

GfuiEventLoop& GfuiApplication::eventLoop()
{
	if (!_pEventLoop)
	{
		GfLogError("GfuiApplication has no event loop ; exiting\n");
		exit(1);
	}
	
    return *dynamic_cast<GfuiEventLoop*>(_pEventLoop);
}

void GfuiApplication::restart()
{
	// Shutdown the window/screen.
	if (_bWindowUp)
	{
		GfuiShutdown();
		_bWindowUp = false;
	}

	// Up to the base class to complete the job.
	GfApplication::restart();
}

GfuiApplication::~GfuiApplication()
{
	// Shutdown the window/screen.
	if (_bWindowUp)
	{
		GfuiShutdown();
		_bWindowUp = false;
	}

	// Note: GfApplication (base class) destructor called now.
}
