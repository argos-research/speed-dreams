/***************************************************************************

    file                 : main.cpp
    created              : Sat Sep  2 10:40:47 CEST 2000
    copyright            : (C) 2000 by Patrice & Eric Espie
    email                : torcs@free.fr
    version              : $Id: main.cpp 5854 2014-11-23 17:55:52Z wdbee $

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
#include <iostream>
#include <sstream>

#include <portability.h>
#include <tgfclient.h>
#include <tgfdata.h>
#include <tgf.h>

#ifdef WIN32
#ifndef HAVE_CONFIG_H
#define HAVE_CONFIG_H
#endif
#endif

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <raceman.h> // RACE_ENG_CFG 
#include <iraceengine.h>
#include <iuserinterface.h>

// If defined in tgf.h:
// Use new Memory Manager ...
#ifdef __DEBUG_MEMORYMANAGER__
#include "memmanager.h"

// Use global variables for debugging ...
IUserInterface* piUserItf = 0;
GfModule* pmodUserItf = NULL;
IRaceEngine* piRaceEngine = 0;
GfModule* pmodRaceEngine = NULL;
// ... Use global variables for debugging

// Garbage Collection in case of GfuiApp().restart();
void ReleaseData(void)
{
	// Shortcut: Use Memory Manager as garbage collector
	GfMemoryManagerRelease(false); // Release the memory manager without dump

	/*
	// For debugging only ...
	if (piUserItf && piRaceEngine)
	{
		// Shutdown and unload the user interface and race engine modules.
		// piUserItf->shutdown();
		// piRaceEngine->shutdown();
		
		// GfModule::unload(pmodUserItf);
		// GfModule::unload(pmodRaceEngine);
		
		// Shutdown the data layer.
		// GfData::shutdown();  << causes crashes if called from here

		// Shortcut: Use Memory Manager as garbage collector
		GfMemoryManagerRelease(false); // Release the memory manager without dump
	}
	// ... For debugging only
	*/
}
#endif 
// ... Use new Memory Manager

/*
 * Function
 *    main
 *
 * Description
 *    Main function of the game
 *
 * Parameters
 *    argc Number of command line args, + 1 for the executable name
 *    argv Array of zero-terminated strings, 1 for each arg
 *
 * Return
 *    0 status code if OK, non-0 otherwise.
 */
int
main(int argc, char *argv[])
{

// If defined in tgf.h:
// Use new Memory Manager ...
#ifdef __DEBUG_MEMORYMANAGER__

	#if defined(_DEBUG)
	fprintf(stderr,"__DEBUG_MEMORYMANAGER__ enabled\n\n");
	fprintf(stderr,"If debugging -> Attach to the process ... \n");
	fprintf(stderr,"\nand than press [Enter] to start the program\n");
	getchar();
	#endif

	// THIS HAS TO BE THE FIRST LINE OF CODE (except the console output)!!!
	GfMemoryManagerInitialize();

	// For hunting of corrupted memory blocks comment the following line
	//GfMemoryManagerSetup(4); // Add 4 bytes per block
#else
	// Use local variables ...
	IUserInterface* piUserItf = 0;
	GfModule* pmodUserItf = NULL;
	IRaceEngine* piRaceEngine = 0;
	GfModule* pmodRaceEngine = NULL;
	// ... Use local variables
#endif
// ... Use new Memeory Manager

	// Look for the "text-only" option flag in the command-line args.
	bool bTextOnly = false;
	for (int i = 1; i < argc; i++)
		if (!strcmp(argv[i], "-x") || !strcmp(argv[i], "--textonly"))
		{
			bTextOnly = true;
			break;
		}

	// Create and initialize the application (graphical or text-only UI).
	GfApplication* pApp;
	if (bTextOnly)
		pApp = new GfApplication("Speed Dreams", VERSION_LONG, "an Open Motorsport Sim");
	else
		pApp = new GfuiApplication("Speed Dreams", VERSION_LONG, "an Open Motorsport Sim");
	pApp->initialize(/*bLoggingEnabled=*/true, argc, argv);
	
	// Register app. specific options and help text.
	pApp->registerOption("s", "startrace", /* nHasValue = */ true);
	pApp->registerOption("x", "textonly", /* nHasValue = */ false);
	
	pApp->addOptionsHelpSyntaxLine("[-s|--startrace <race name> [-x|--textonly] ]");
	pApp->addOptionsHelpExplainLine
	 	("- race name : Name without extension of the selected raceman file,");
	pApp->addOptionsHelpExplainLine
	 	("              among the .xml files in <user settings>/config/raceman (no default)");
	pApp->addOptionsHelpExplainLine
	 	("- text-only : Run the specified race without any GUI (suitable for a headless computer)");

	// Parse the command line for registered options.
    if (!pApp->parseOptions())
		return 1;

	// Some more checks about command line options.
	std::string strRaceToStart;
	if (bTextOnly && (!pApp->hasOption("startrace", strRaceToStart) || strRaceToStart.empty()))
	{
		std::cerr << "Exiting from " << pApp->name()
				  << " because no race specified in text-only mode." << std::endl;
		return 1;
	}
	
	// If "data dir" specified in any way, cd to it.
	if(chdir(GfDataDir()))
	{
		GfLogError("Could not start %s : failed to cd to the datadir '%s' (%s)\n",
				   pApp->name().c_str(), GfDataDir(), strerror(errno));
		return 1;
	}

	// Update user settings files from installed ones.
    pApp->updateUserSettings();

   // Initialize the event loop management layer (graphical or text-only UI).
	GfEventLoop* pEventLoop;
	if (bTextOnly)
		pEventLoop = new GfEventLoop;
	else
		pEventLoop = new GfuiEventLoop;
	pApp->setEventLoop(pEventLoop);

	// When there's a GUI, setup the window / screen and menu infrastructure.
    if (!bTextOnly && !dynamic_cast<GfuiApplication*>(pApp)->setupWindow())
	{
		std::cerr << "Exiting from " << pApp->name()
				  << " after some error occurred (see above)." << std::endl;
		return 1;
	}

	// Load the user interface module (graphical or text-only UI).
	pmodUserItf =
		GfModule::load("modules/userinterface", (bTextOnly ?  "textonly" : "legacymenu"));

	// Check that it implements IUserInterface.
	if (pmodUserItf)
	{
		piUserItf = pmodUserItf->getInterface<IUserInterface>();
	}

	// Initialize the data layer (needed by any race engine module).
	GfData::initialize();

	// Load the race engine module (specified in the user settings : raceengine.xml file).
	std::ostringstream ossParm;
	ossParm << GfLocalDir() << RACE_ENG_CFG;
	void* hREParams =
		GfParmReadFile(ossParm.str().c_str(), GFPARM_RMODE_REREAD | GFPARM_RMODE_CREAT);
	const char* pszModName = GfParmGetStr(hREParams, "Modules", "racing", "standardgame");

	pmodRaceEngine = GfModule::load("modules/racing", pszModName);

	// Check that it implements IRaceEngine.
	if (pmodRaceEngine)
	{
		piRaceEngine = pmodRaceEngine->getInterface<IRaceEngine>();
	}

	// Connect the race engine and the user interface modules.
	if (piUserItf && piRaceEngine)
	{
		piRaceEngine->setUserInterface(*piUserItf);
		piUserItf->setRaceEngine(*piRaceEngine);
	}
	
	if (piUserItf && piRaceEngine)
	{
#ifdef __DEBUG_MEMORYMANAGER__
		// Allow to use Garbage Collection in case of GfuiApp().restart();
		pApp->ReleaseData = &ReleaseData;
#endif

		// Enter the user interface.
		if (piUserItf->activate())
		{
			// Game event loop (when it returns, it's simply because we are exiting).
			pApp->eventLoop()();
		}
		
		// Shutdown and unload the user interface and race engine modules.
		piUserItf->shutdown();
		piRaceEngine->shutdown();
		
		GfModule::unload(pmodUserItf);
		GfModule::unload(pmodRaceEngine);
		
		// Shutdown the data layer.
		GfData::shutdown();
	}

	// Done with the app instance.
	const std::string strAppName(pApp->name());
	delete pApp;

 	// That's all (but trace what we are doing).
	if (piUserItf && piRaceEngine)
		GfLogInfo("Exiting normally from %s.\n", strAppName.c_str());
	else
		std::cerr << "Exiting from " << strAppName
				  << " after some error occurred (see above)." << std::endl;

	// If defined in tgf.h:
	// Use new Memory Manager ...
	#ifdef __DEBUG_MEMORYMANAGER__

	GfMemoryManagerSaveToFile();

	// THIS HAS TO BE THE LAST LINE OF CODE BEFORE RETURN!!!
	GfMemoryManagerRelease();

	#endif
	// ... Use new Memory Manager

	return (piUserItf && piRaceEngine) ? 0 : 1;
}

