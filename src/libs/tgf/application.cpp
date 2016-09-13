/***************************************************************************
                      application.cpp -- Application base
                             -------------------
    created              : Mon Apr 14 22:30:04 CEST 2011
    copyright            : (C) 2011 by Jean-Philippe Meuret
    web                  : http://www.speed-dreams.org
    version              : $Id: application.cpp 5858 2014-11-25 19:58:53Z wdbee $
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
    		Application base
    @version	$Id: application.cpp 5858 2014-11-25 19:58:53Z wdbee $
    @ingroup	tgf
*/

#include <cerrno>
#include <ctime>
#include <limits>
#include <iostream>

#ifdef WIN32
#ifndef HAVE_CONFIG_H
#define HAVE_CONFIG_H
#endif
#endif

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <portability.h>

#include "tgf.hpp"


// The singleton.
GfApplication* GfApplication::_pSelf = 0;

GfApplication& GfApplication::self()
{
	if (!_pSelf)
	{
		GfLogError("GfApplication not yet created ; exiting");
		::exit(1);
	}
	
	return *_pSelf;
}

GfApplication::GfApplication(const char* pszName, const char* pszVersion, const char* pszDesc)
: _strName(pszName ? pszName : "GfApplication"), _strDesc(pszDesc ? pszDesc : ""),
  _strVersion(pszVersion ? pszVersion : ""), _pEventLoop(0)
{
	// Check that we are the only instance.
	if (_pSelf)
	{
		fprintf(stderr, "More than one GfApplication instance ; exiting\n");
		::exit(1);
	}

	// Register oneself as the one.
	_pSelf = this;
}

void GfApplication::initialize(bool bLoggingEnabled, int argc, char **argv)
{
	// Store the command line args.
	if (argv)
		for (int i = 0; i < argc; i++)
			_lstArgs.push_back(argv[i]);

	// Initialize the gaming framework (beware: only GfLogDefault booted).
	GfInit(bLoggingEnabled);

    // Trace app. information.
	GfLogInfo("%s %s", _strName.c_str(), _strVersion.c_str());
	if (!_strDesc.empty())
		GfLogInfo(" (%s)", _strDesc.c_str());
	GfLogInfo("\n");

    // Register the command line options (to be parsed).
	registerOption("h",  "help", /* nHasValue = */ false);
	registerOption("v",  "version", /* nHasValue = */ false);
	registerOption("lc", "localdir", /* nHasValue = */ true);
	registerOption("ld", "libdir", /* nHasValue = */ true);
	registerOption("bd", "bindir", /* nHasValue = */ true);
	registerOption("dd", "datadir", /* nHasValue = */ true);
	registerOption("tl", "tracelevel", /* nHasValue = */ true);
	registerOption("ts", "tracestream", /* nHasValue = */ true);
	registerOption("nr", "norandom", /* nHasValue = */ false);
	
	// Help about the command line options.
	addOptionsHelpSyntaxLine("[-v|--version]");
	addOptionsHelpSyntaxLine("[-h|--help]");
	addOptionsHelpSyntaxLine("[-lc|--localdir <dir path>] [-ld|--libdir <dir path>]");
	addOptionsHelpSyntaxLine("[-bd|--bindir <dir path>] [-dd|--datadir <dir path>]");
#ifdef TRACE_OUT
	addOptionsHelpSyntaxLine("[-tl|--tracelevel <integer>]"
							 " [-ts|--tracestream stdout|stderr|<file name>]");
#endif
	addOptionsHelpSyntaxLine("[-nr|--norandom]");
	
	addOptionsHelpExplainLine
		("- locadir : Root dir of the tree where user settings files are stored");
	addOptionsHelpExplainLine
		("            (default=" SD_LOCALDIR ")");
	addOptionsHelpExplainLine
		("- libdir  : Root dir of the tree where loadable modules are installed");
	addOptionsHelpExplainLine
		("            (default=" SD_LIBDIR ")");
	addOptionsHelpExplainLine
		("- bindir  : Dir where the game exe and DLLs are installed");
	addOptionsHelpExplainLine
		("            (default=" SD_BINDIR ")");
	addOptionsHelpExplainLine
		("- datadir : Root dir of the data tree (cars, tracks, ...)");
	addOptionsHelpExplainLine
		("            (default=" SD_DATADIR ")");
#ifdef TRACE_OUT
	addOptionsHelpExplainLine
		("- tracelevel  : Maximum level of displayed traces for the default logger");
	addOptionsHelpExplainLine
		("                (0=Fatal, 1=Error, 2=Warning, 3=Info, 4=Trace, 5=Debug, ... ; default=5)");
	addOptionsHelpExplainLine
		("- tracestream : Target output stream for the default logger (default=stderr)");
	addOptionsHelpExplainLine
		("- norandom : Force reproducible random sequences for every game session (default=off)");
#endif
}

const std::string& GfApplication::name() const
{
	return _strName;
}

const std::string& GfApplication::description() const
{
	return _strDesc;
}

const std::string& GfApplication::version() const
{
	return _strVersion;
}

void GfApplication::updateUserSettings()
{
	GfFileSetup();
	
	// Complete logging system initialisation
	// 1) Reparse the relevant options if present.
	int nDefTraceLevel = std::numeric_limits<int>::min();
	std::string strDefTraceStream;

	std::list<Option>::const_iterator itOpt;
	for (itOpt = _lstOptions.begin(); itOpt != _lstOptions.end(); itOpt++)
	{
		// Not found in the command line => ignore / default value.
		if (!itOpt->bFound)
			continue;
		
		// Trace level threshold (only #ifdef TRACE_OUT)
		if (itOpt->strLongName == "tracelevel")
		{
			if (sscanf(itOpt->strValue.c_str(), "%d", &nDefTraceLevel) < 1)
			{
				printUsage("Error: Could not convert trace level to an integer.");
				nDefTraceLevel = std::numeric_limits<int>::min();
			}
		}
		// Target trace stream (only #ifdef TRACE_OUT)
		else if (itOpt->strLongName == "tracestream")
		{
			strDefTraceStream = itOpt->strValue;
		}
	}

	// 2) Take the options into account.
	// Note: Not achieved earlier because we need one of the user settings files : logging.xml.
	GfLogger::setup();
	if (nDefTraceLevel != std::numeric_limits<int>::min())
		GfLogDefault.setLevelThreshold(nDefTraceLevel);
	if (!strDefTraceStream.empty())
		GfLogDefault.setStream(strDefTraceStream);
}

void GfApplication::setEventLoop(GfEventLoop* pEventLoop)
{
	_pEventLoop = pEventLoop;
}

GfEventLoop& GfApplication::eventLoop()
{
	if (!_pEventLoop)
		GfLogError("GfApplication has no event loop ; crashing !\n");
	
	return *_pEventLoop;
}

void GfApplication::restart()
{
	// Shutdown the gaming framework.
	GfShutdown();

	// Delete the event loop if any.
	delete _pEventLoop;
	_pEventLoop = 0;

	// Reset the Memory Manager
	#ifdef __DEBUG_MEMORYMANAGER__
	(*ReleaseData)();
	#endif

	// Restart the process, using same command line args.
	// 1) The process executable path-name is the 1st arg left untouched.
	GfLogInfo("Restarting :\n");
	GfLogInfo("  Command : %s\n", _lstArgs.front().c_str());

	// 2) Allocate and populate the args array (last arg must be a null pointer).
	// TODO: Add an API for filtering the args (some might not be relevant for a restart).
	GfLogInfo("  Args    : ");
	char** apszArgs = (char**)malloc(sizeof(char*) * (_lstArgs.size() + 1));

	unsigned nArgInd = 0;
	std::list<std::string>::const_iterator itArg;
	for (itArg = _lstArgs.begin(); itArg != _lstArgs.end(); itArg++)
	{
#ifdef WIN32
		// execvp will not automatically surround args with spaces inside with double quotes !
		if (itArg->find(' ') != std::string::npos)
		{
			char pszArg[512];
			snprintf(pszArg, sizeof(pszArg), "\"%s\"", itArg->c_str());
			apszArgs[nArgInd] = strdup(pszArg);
		}
		else
			apszArgs[nArgInd] = strdup(itArg->c_str());

		GfLogInfo("%s ", apszArgs[nArgInd]);
		
#else
		// execvp will automatically surround args with spaces inside with double quotes.
		apszArgs[nArgInd] = strdup(itArg->c_str());

		if (itArg->find(' ') != std::string::npos)
			GfLogInfo("\"%s\" ", itArg->c_str());
		else
			GfLogInfo("%s ", itArg->c_str());
#endif

		// Next arg.
		nArgInd++;
	}
	apszArgs[nArgInd] = 0;
	GfLogInfo("...\n\n");
	
	// 3) Exec the command with its args (replacing current process).
	const int retcode = execvp(_lstArgs.front().c_str(), apszArgs);

	// If the restart was successfull, we never get there ... But if it failed ...
	GfLogError("Failed to restart (exit code %d, %s)\n", retcode, strerror(errno));
	for (nArgInd = 0; apszArgs[nArgInd]; nArgInd++)
		free(apszArgs[nArgInd]);
	free(apszArgs);
	
	exit(1);
}

void GfApplication::printUsage(const char* pszErrMsg) const
{
	if (pszErrMsg)
		std::cout << std::endl << "Error: " << pszErrMsg << std::endl << std::endl;
	
	std::cout << "Usage: " << _lstArgs.front() << " ..." << std::endl;

	std::list<std::string>::const_iterator itSynLine = _optionsHelp.lstSyntaxLines.begin();
	while (itSynLine != _optionsHelp.lstSyntaxLines.end())
	{
		std::cout << "         " << *itSynLine << std::endl;
		itSynLine++;
	}

	std::list<std::string>::const_iterator itExplLine = _optionsHelp.lstExplainLines.begin();
	while (itExplLine != _optionsHelp.lstExplainLines.end())
	{
		std::cout << " " << *itExplLine << std::endl;
		itExplLine++;
	}
}

bool GfApplication::parseOptions()
{
	// Determine and store run-time install root dir.
	GfInitInstallDir(_lstArgs.front().c_str());

	// Parse args, looking for registered options.
	GfLogInfo("Parsing command line args (%d)\n", _lstArgs.size() - 1);
	std::list<std::string>::const_iterator itArg = _lstArgs.begin();
	for (itArg++; itArg != _lstArgs.end(); itArg++)
	{
		bool bArgEaten = false;
		if (itArg->find('-') == 0)
		{
			// We've probably got an option flag : check this a bit more in depth.
			std::list<Option>::iterator itOpt;
			for (itOpt = _lstOptions.begin(); itOpt != _lstOptions.end(); itOpt++)
			{
				if (itOpt->strShortName == itArg->substr(1, std::string::npos)
					|| itOpt->strLongName == itArg->substr(2, std::string::npos))
				{
					// We've got a registered option flag : check if there's a value arg or not.
					if (itOpt->bHasValue)
					{
						const std::string strFlag(*itArg);
						itArg++;
						if (itArg != _lstArgs.end() // Some extra arg available ...
							&& itArg->find('-') != 0) // ... and not an option flag :
						{
							itOpt->strValue = *itArg; // We've got the value.
							GfLogInfo("  %s %s : option '%s'\n", strFlag.c_str(),
									  itArg->c_str(), itOpt->strLongName.c_str());
						}
						else
						{
							// Should have a value arg, but it's not there ... error !
							printUsage();
							return false;
						}
					}
					else
					{
						GfLogInfo("  %s : option '%s'\n",
								  itArg->c_str(), itOpt->strLongName.c_str());
					}

					// Value or not, we've got an option, and we eat the arg(s) : done.
					itOpt->bFound = true;
					bArgEaten = true;
					break;
				}
			}
		}

		// Save any ignored arg in the "remaining" list.
		if (!bArgEaten)
		{
			_vecRemArgs.push_back(*itArg);
			GfLogInfo("  %s : not a registered option\n", itArg->c_str());
		}
	}
	
	// Interpret the detected command line options.
	const char *pszLocalDir = 0;
	const char *pszLibDir = 0;
	const char *pszDataDir = 0;
	const char *pszBinDir = 0;

	bool bTrueRandom = true;

	std::list<Option>::const_iterator itOpt;
	for (itOpt = _lstOptions.begin(); itOpt != _lstOptions.end(); itOpt++)
	{
		// Not found in the command line => ignore / default value.
		if (!itOpt->bFound)
			continue;
		
		// Help about command line
		if (itOpt->strLongName == "help")
		{
			printUsage();
			return false;
		}
		// Version information
		else if (itOpt->strLongName == "version")
		{
			std::cout << _strName << ' ' << _strVersion << std::endl;
			return false;
		}
		// Local dir (root dir of the tree where user settings files are stored)
		else if (itOpt->strLongName == "localdir")
		{
			pszLocalDir = GfSetLocalDir(itOpt->strValue.c_str());
		}
		// Libraries dir (root dir of the tree where loadable modules are installed)
		else if (itOpt->strLongName == "libdir")
		{
			pszLibDir = GfSetLibDir(itOpt->strValue.c_str());
		}
		// Binaries dir (the dir where the game exe and DLLs are installed)
		else if (itOpt->strLongName == "bindir")
		{
			pszBinDir = GfSetBinDir(itOpt->strValue.c_str());
		}
		// Data dir (root dir of the data tree)
		else if (itOpt->strLongName == "datadir")
		{
			pszDataDir = GfSetDataDir(itOpt->strValue.c_str());
		}
		// Initialize random generator or not.
		else if (itOpt->strLongName == "norandom")
		{
			bTrueRandom = false;
		}
		else
		{
			// If we get here, this is normal : the derived classes might have declared
			// specific options that it'll parse itself later.
		}
	}

	// If any of the Speed-Dreams dirs not run-time specified / empty, 
	// use associated compile-time variable SD_XXDIR to get default value
	if (!(pszLocalDir && strlen(pszLocalDir)))
		pszLocalDir = GfSetLocalDir(SD_LOCALDIR);
	if (!(pszLibDir && strlen(pszLibDir)))
		pszLibDir = GfSetLibDir(SD_LIBDIR);
	if (!(pszBinDir && strlen(pszBinDir)))
		pszBinDir = GfSetBinDir(SD_BINDIR);
	if (!(pszDataDir && strlen(pszDataDir)))
		pszDataDir = GfSetDataDir(SD_DATADIR);

	// If the data dir. is not a run-time usable one, may be it's because we are running
	// without installing : try and use the _source_ data dir (it _is_ run-time usable).
	std::string strDataDirProof(pszDataDir);
	// A run-time usable data dir has a "config/logging.xml" file inside.
	strDataDirProof += LOGGING_CFG;
	if (pszDataDir && strlen(pszDataDir) && !GfFileExists(strDataDirProof.c_str()))
	{
		GfLogTrace("Data dir. '%s' not run-time usable, trying source data dir.\n",
				   pszDataDir);
		pszDataDir = GfSetDataDir(SD_DATADIR_SRC);
	}
	
	// Check if ALL the Speed-dreams dirs have a usable value, and exit if not.
	if (!(pszLocalDir && strlen(pszLocalDir)) || !(pszLibDir && strlen(pszLibDir)) 
		|| !(pszBinDir && strlen(pszBinDir)) || !(pszDataDir && strlen(pszDataDir)))
	{
		GfLogTrace("User settings dir. : '%s'\n", GfLocalDir());
		GfLogTrace("Libraries dir.     : '%s'\n", GfLibDir());
		GfLogTrace("Binaries dir.      : '%s'\n", GfBinDir());
		GfLogTrace("Data dir.          : '%s'\n", GfDataDir());
		
		GfLogError("Could not start %s :"
				   " at least 1 of local/data/lib/bin dir is empty\n\n", _strName.c_str());
		
		return false;
	}

	// Initialize random generator with "random" seed, or not (=> always same rand() sequence).
	if (bTrueRandom)
	{
		GfLogInfo("Initializing random number generator for 'true randomness'\n");
		srand((unsigned)time(0));
	}
	// Note: Never calling srand is equivalent to calling it once with seed=1.
	else
	{
		GfLogInfo("Not initializing random number generator, for 'repeatable randomness'\n");
	}
	
	return true;
}

void GfApplication::registerOption(const std::string& strShortName,
								   const std::string& strLongName,
								   bool bHasValue)
{
	// Check if no already registered option has same short or long name.
	std::list<Option>::const_iterator itOpt;
	for (itOpt = _lstOptions.begin(); itOpt != _lstOptions.end(); itOpt++)
	{
		try
		{
		if (itOpt->strShortName == strShortName)
		{
			GfLogError("Can't register option -%s/--%s "
					   "with same short name as -%s/--%s ; ignoring.\n",
					   strShortName.c_str(), strLongName.c_str(),
					   itOpt->strShortName.c_str(), itOpt->strLongName.c_str());
			return;
		}
		else if (itOpt->strLongName == strLongName)
		{
			GfLogError("Can't register option -%s/--%s "
					   "with same long name as -%s/--%s ; ignoring.\n",
					   strShortName.c_str(), strLongName.c_str(),
					   itOpt->strShortName.c_str(), itOpt->strLongName.c_str());
			return;
		}
		}
		catch (std::bad_exception)
		{
			GfLogError("GfApplication::registerOption -%s",strShortName.c_str());
		}

	}

	// All's right : register.
	_lstOptions.push_back(Option(strShortName, strLongName, bHasValue));
}

void GfApplication::addOptionsHelpSyntaxLine(const std::string& strTextLine)
{
	_optionsHelp.lstSyntaxLines.push_back(strTextLine);
}

void GfApplication::addOptionsHelpExplainLine(const std::string& strTextLine)
{
	_optionsHelp.lstExplainLines.push_back(strTextLine);
}

bool GfApplication::hasOption(const std::string& strLongName) const
{
	std::list<Option>::const_iterator itOpt;
	for (itOpt = _lstOptions.begin(); itOpt != _lstOptions.end(); itOpt++)
		if (itOpt->bFound && itOpt->strLongName == strLongName)
			return true;

	return false;
}

bool GfApplication::hasOption(const std::string& strLongName,
							  std::string& strValue) const
{
	std::list<Option>::const_iterator itOpt;
	for (itOpt = _lstOptions.begin(); itOpt != _lstOptions.end(); itOpt++)
		if (itOpt->bFound && itOpt->strLongName == strLongName)
		{
			strValue = itOpt->strValue;
			return true;
		}

	return false;
}

// bool GfApplication::hasUnregisteredOption(const std::string& strShortName,
// 										  const std::string& strLongName) const
// {
// 	std::list<std::string>::const_iterator itArg;
// 	for (itArg = _lstRemArgs.begin(); itArg != _lstRemArgs.end(); itArg++)
// 		if (*itArg == "-" + strShortName || *itArg == "--" + strLongName)
// 			return true;

// 	return false;
// }

// bool GfApplication::hasUnregisteredOption(const std::string& strShortName,
// 										  const std::string& strLongName,
// 										  std::string& strValue) const
// {
// 	std::list<std::string>::const_iterator itArg;
// 	for (itArg = _lstRemArgs.begin(); itArg != _lstRemArgs.end(); itArg++)
// 		if (*itArg == "-" + strShortName || *itArg == "--" + strLongName)
// 		{
// 			itArg++;
// 			if (itArg != _lstRemArgs.end() // Some extra arg available ...
// 				&& itArg->find('-') != 0) // ... but not an option flag.
// 			{
// 				strValue = *itArg;
// 				return true;
// 			}
// 			else
// 				break; // Value not found. TODO: Error handling ?
// 		}

// 	return false;
// }

GfApplication::~GfApplication()
{
	// Shutdown the gaming framework.
	GfShutdown();

	// Delete the event loop if any.
	delete _pEventLoop;
	_pEventLoop = 0;

	// Really shutdown the singleton.
	_pSelf = 0;
}
