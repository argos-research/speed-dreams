/***************************************************************************
                          Tracing / logging system                   
                             -------------------                                         
    created              : Fri Aug 13 22:32:45 CEST 1999
    copyright            : (C) 2010, 2013 by Jean-Philippe Meuret
    web                  : www.speed-dreams.org
    version              : $Id: trace.cpp 5827 2014-11-12 17:05:02Z wdbee $
                                  
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
    Tracing / logging system
    @version	$Id: trace.cpp 5827 2014-11-12 17:05:02Z wdbee $
    @ingroup	trace
*/


#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <cstring>
#include <cctype>
#include <ctime>
#include <sstream>
#include <algorithm>

#include <map>
				  
#ifdef WIN32
#include <windows.h>
#include <windowsx.h>
#else
#include <sys/param.h>
#endif // WIN32

#include "portability.h"
#include "tgf.hpp"


// Level names.
static const char* astrLevelNames[] =
	{ "Fatal", "Error", "Warning", "Info", "Trace", "Debug" };
static const int nLevelNames = (int)(sizeof(astrLevelNames) / sizeof(const char*));


// Logger instances.
std::map<std::string, GfLogger*> gfMapLoggersByName;

// Flag indicating if output is enabled (for all loggers).
bool GfLogger::_bOutputEnabled = false;

// The default logger : created and initialized first of all (see GfLogger::setup() below).
static const char* pszDefLoggerName = "Default";
GfLogger* GfPLogDefault = 0;

void
gfTraceInit(bool bWithLogging)
{
	// Boot the logging system
	// Beware: only GfLogDefault logger initialized at the end,
	//         see GfLogger::setup for other loggers.
	GfLogger::boot(bWithLogging);
}

// Delete all instances of all loggers created while running the program
void
gfTraceShutdown(void)
{
	// Iterator for all the loggers
	std::map<std::string, GfLogger*>::iterator itLog;

	// Delete all instances
	for (itLog = gfMapLoggersByName.begin(); itLog != gfMapLoggersByName.end(); itLog++)
		delete itLog->second;	// a map<x,y> iterator "points" to a pair<x,y> value

	// Empty the map
	gfMapLoggersByName.clear();
}

// GfLogger class implementation ==================================================

// Factory method.
GfLogger* GfLogger::instance(const std::string& strName)
{
	// If the specified logger does not exists yet, create it and put it into the map.
	std::map<std::string, GfLogger*>::iterator itLog = gfMapLoggersByName.find(strName);
	if (itLog == gfMapLoggersByName.end())
	{
		// Default settings (null stream if output disabled).
		GfLogger* pLog =
			(_bOutputEnabled ? new GfLogger(strName) : new GfLogger(strName, 0));
		gfMapLoggersByName[strName] = pLog;

		// Get again from the map : should never fail.
		itLog = gfMapLoggersByName.find(strName);
	}

	return itLog->second;
}

void GfLogger::boot(bool bWithLogging)
{
	// Save global settings.
	_bOutputEnabled = bWithLogging;

	// Create the default logger and pre-initialize it with hard coded default settings
	// (we need it for tracing stuff happening when completing initialization).
	GfPLogDefault = GfLogger::instance(pszDefLoggerName);

	// Trace real date and time (the time column is actually the time elapsed since startup).
	time_t t = time(NULL);
	struct tm *stm = localtime(&t);
	GfLogDefault.info("Date and time : %4d/%02d/%02d %02d:%02d:%02d\n",
					  stm->tm_year+1900, stm->tm_mon+1, stm->tm_mday,
					  stm->tm_hour, stm->tm_min, stm->tm_sec);
	
}

void GfLogger::setup()
{
#if 0
	// Quick unit / non-regression tests for GfLogger : change previous line to "#if 1" to enable.
	GfLogDefault.debug("Testing GfLogger ...\n");

	// Header columns.
	GfLogDefault.setHeaderColumns(GfLogger::eNone);
	GfLogDefault.debug("Trace with no header column at all\n");
	GfLogDefault.setHeaderColumns(GfLogger::eTime);
	GfLogDefault.debug("Trace with time header\n");
	GfLogDefault.setHeaderColumns(GfLogger::eTime|GfLogger::eLogger);
	GfLogDefault.debug("Trace with time+logger header\n");
	GfLogDefault.setHeaderColumns(GfLogger::eTime|GfLogger::eLogger|GfLogger::eLevel);
	GfLogDefault.debug("Trace with time+logger+level header\n");
	GfLogDefault.setHeaderColumns(GfLogger::eAll);
	GfLogDefault.debug("Trace with all-columns header\n");

	// Level threshold.
	GfLogDefault.setLevelThreshold(6);
	GfLogDefault.message(6, "Visible level 6 trace\n");
	GfLogDefault.setLevelThreshold(GfLogger::eInfo);
	GfLogDefault.debug("Invisible debug trace\n");
	GfLogDefault.setLevelThreshold(GfLogger::eDebug);

	// Target stream.
	GfLogDefault.setStream("gflogger-test.log");
	GfLogDefault.debug("Visible test trace\n");
	GfLogDefault.setStream(stderr);
	
	GfLogDefault.debug("Testing GfLogger : done.\n");
#endif	

	// Load logger settings and create and / or configure them as requested.
	// 1) Open and load the logger settings file.
	std::ostringstream ossParm;
	ossParm << GfLocalDir() << LOGGING_CFG;
	void* hparmlogSettings = GfParmReadFile(ossParm.str().c_str(), GFPARM_RMODE_STD);
	if(!hparmlogSettings)
	{
		GfLogDefault.warning("Failed to open or parse logger settings file %s\n",
							 ossParm.str().c_str());
		return;
	}

	// 2) For each logger defined in the settings :
	if (GfParmListSeekFirst(hparmlogSettings, "Loggers"))
	{
		GfLogDefault.warning("Failed to read logger settings from %s\n", ossParm.str().c_str());
		return;
	}
	do
	{
		// Read logger name.
		const char* pszLogName = GfParmListGetCurEltName(hparmlogSettings, "Loggers");
		if (!pszLogName || strlen(pszLogName) == 0)
		{
			GfLogDefault.warning("Not creating logger with invalid name '%s' in settings file\n",
								 pszLogName ? pszLogName : "<null>");
			continue;
		}

		// Read and parse target output stream / file name.
		const char* pszOutStream =
			GfParmGetCurStr(hparmlogSettings, "Loggers", "output stream", "stderr");

		// Read and parse level threshold.
		const char* pszLvlThresh =
			GfParmGetCurStr(hparmlogSettings, "Loggers", "level threshold", "debug");
		int nLvlThresh;
		for (nLvlThresh = 0; nLvlThresh < nLevelNames; nLvlThresh++)
			if (!strcasecmp(pszLvlThresh, astrLevelNames[nLvlThresh]))
				break;
		if (nLvlThresh == nLevelNames) // Not a "named" one : try an integer.
		{
			if (strlen(pszLvlThresh) > 0 && isdigit(pszLvlThresh[0]))
				nLvlThresh = atoi(pszLvlThresh); // We found a positive or nul integer.
			else
				nLvlThresh = TRACE_LEVEL; // Fall back to the default level threshold.
		}

		// Read and parse selected header columns.
		std::string strHdrCols =
			GfParmGetCurStr(hparmlogSettings, "Loggers", "header columns", "all");
		std::transform(strHdrCols.begin(), strHdrCols.end(), strHdrCols.begin(), ::tolower);
		unsigned bfHdrCols = 0;
		if (strHdrCols.find("all") != std::string::npos)
			bfHdrCols = eAll;
		else if (strHdrCols.empty() || strHdrCols.find("none") != std::string::npos)
			bfHdrCols = eNone;
		else
		{
			if (strHdrCols.find("time") != std::string::npos)
				bfHdrCols |= eTime;
			if (strHdrCols.find("logger") != std::string::npos)
				bfHdrCols |= eLogger;
			if (strHdrCols.find("level") != std::string::npos)
				bfHdrCols |= eLevel;
		}
		
		GfLogDefault.trace("Logger '%s' settings : stream=%s, threshold=%s, columns=%s\n",
						   pszLogName, pszOutStream, pszLvlThresh, strHdrCols.c_str());

		// If default logger, apply new settings.
		if (!strcmp(pszLogName, pszDefLoggerName))
		{
			GfLogger* pLog = instance(pszDefLoggerName);
			pLog->setStream(pszOutStream);
			pLog->setHeaderColumns(bfHdrCols);
			pLog->setLevelThreshold(nLvlThresh);
		}

		// If not default logger, create with requested settings.
		else
		{
			GfLogger* pLog = new GfLogger(pszLogName, pszOutStream, nLvlThresh, bfHdrCols);
			gfMapLoggersByName[pszLogName] = pLog;
		}

	}
	while(!GfParmListSeekNext(hparmlogSettings, "Loggers"));
	
	GfParmReleaseHandle(hparmlogSettings);

	// TODO : Update logger settings from command line options if any.
}

GfLogger::GfLogger()
  : _strName(""), _bfHdrCols(0), _pStream(0), _nLvlThresh(0), _bNeedsHeader(true)
{
}

GfLogger::GfLogger(const std::string& strName, FILE* pFile,
				   int nLvlThresh, unsigned bfHdrCols)
  : _strName(strName), _bfHdrCols(bfHdrCols), _pStream(pFile), _nLvlThresh(nLvlThresh),
	_bNeedsHeader(true)
{
	// Trace initial state.
	info("Logger '%s' created : Level threshold ", strName.c_str());
	if (_nLvlThresh >= eFatal && _nLvlThresh <= eDebug)
		info("%s:%d\n", astrLevelNames[_nLvlThresh], _nLvlThresh);
	else
		info("%d\n", _nLvlThresh);
}

GfLogger::GfLogger(const std::string& strName, const std::string& strFilename,
				   int nLvlThresh, unsigned bfHdrCols)
  : _strName(strName), _bfHdrCols(bfHdrCols), _pStream(0), _nLvlThresh(nLvlThresh),
	_bNeedsHeader(true)
{
	// Set file stream.
	setStream(strFilename);
	
	// Trace initial state.
	info("Logger '%s' created : Level threshold ", strName.c_str());
	if (_nLvlThresh >= eFatal && _nLvlThresh <= eDebug)
		info("%s:%d\n", astrLevelNames[_nLvlThresh], _nLvlThresh);
	else
		info("%d\n", _nLvlThresh);
}

GfLogger::~GfLogger()
{
	// Close stream if needed.
	if (_pStream && _pStream != stderr && _pStream != stdout)
		fclose(_pStream);
}

const std::string& GfLogger::name() const
{
	return _strName;
}

int GfLogger::levelThreshold() const
{
	return _nLvlThresh;
}

void GfLogger::setLevelThreshold(int nLevel)
{
	// Trace the change to come.
	if (_pStream && eInfo <= _nLvlThresh && _nLvlThresh != nLevel)
	{
		putLineHeader(eInfo);
		fprintf(_pStream, "Changing trace level threshold to ");
		if (nLevel >= eFatal && nLevel <= eDebug)
			fprintf(_pStream, "%s:%d (was ", astrLevelNames[nLevel], nLevel);
		else
			fprintf(_pStream, "%d (was ", nLevel);
		if (_nLvlThresh >= eFatal && _nLvlThresh <= eDebug)
			fprintf(_pStream, "%s:%d)\n", astrLevelNames[_nLvlThresh], _nLvlThresh);
		else
			fprintf(_pStream, "%d)\n", _nLvlThresh);
		fflush(_pStream);
	}

	// Do the change.
	_nLvlThresh = nLevel;
}

unsigned GfLogger::headerColumns() const
{
	return _bfHdrCols;
}

void GfLogger::setHeaderColumns(unsigned bfHdrCols)
{
	// Trace the change to come.
	if (_pStream && eInfo <= _nLvlThresh && _bfHdrCols != bfHdrCols)
	{
		putLineHeader(eInfo);
		fprintf(_pStream, "Changing header columns to 0x%02X (was 0x%02X)\n",
				bfHdrCols, _bfHdrCols);
		fflush(_pStream);
	}

	// Do the change.
	_bfHdrCols = bfHdrCols;
}

FILE* GfLogger::stream() const
{
	return _pStream;
}

void GfLogger::setStream(FILE* pFile, bool bLogFileChange)
{
	if (pFile == _pStream)
		return;
	
	if (pFile)
	{
		// Trace the stream change on the current stream.
		if (bLogFileChange && _pStream && eInfo <= _nLvlThresh)
		{
			putLineHeader(eInfo);
			fprintf(_pStream, "Changing target stream to ");
			if (pFile == stderr)
				fprintf(_pStream, "stderr\n");
			else if (pFile == stdout)
				fprintf(_pStream, "stdout\n");
			else
				fprintf(_pStream, "(FILE*)%p\n", pFile);
			fflush(_pStream);
		}

		// Close current if needed.
		if (_pStream && _pStream != stderr && _pStream != stdout)
			fclose(_pStream);

		// Do the change.
		_pStream = pFile;
	}
	else
	{
		// Error : can't change to null stream.
		const int nErrNo = errno; // ?? Is there really an C lib error behind ?
		if (_pStream && eError <= _nLvlThresh)
		{
			putLineHeader(eError);
			fprintf(_pStream, 
					"GfLogger::setStream(FILE*) : Null stream (%s)\n", strerror(nErrNo));
			fflush(_pStream);
		}
	}
	
	if (_pStream)
	{
		// Trace date and time on new stream.
		if (eInfo <= _nLvlThresh)
		{
			putLineHeader(eInfo);
			time_t t = time(NULL);
			struct tm *stm = localtime(&t);
			fprintf(_pStream, "Date and time : %4d/%02d/%02d %02d:%02d:%02d\n",
					stm->tm_year+1900, stm->tm_mon+1, stm->tm_mday,
					stm->tm_hour, stm->tm_min, stm->tm_sec);
		}
		
		// Trace current trace level threshold.
		if (eInfo <= _nLvlThresh)
		{
			putLineHeader(eInfo);
			fprintf(_pStream, "Current trace level threshold : ");
			if (_nLvlThresh >= eFatal && _nLvlThresh <= eDebug)
				fprintf(_pStream, "%s\n", astrLevelNames[_nLvlThresh]);
			else
				fprintf(_pStream, "Level%d\n", _nLvlThresh);
		}

		// That's all.
		fflush(_pStream);
	}
}

void GfLogger::setStream(const std::string& strPathname)
{
	// Check special "file names" stderr and stdout.
	if (!strcasecmp(strPathname.c_str(), "stderr"))
	{
		setStream(stderr);
		return;
	}
	else if (!strcasecmp(strPathname.c_str(), "stdout"))
	{
		setStream(stdout);
		return;
	}
	
	// Open the requested target file in GfLocalDir()
	std::string strRealPathname(GfLocalDir());
	strRealPathname += strPathname;
	FILE* pFile = fopen(strRealPathname.c_str(), "w");
	if (pFile)
	{
		// Trace the stream change on the current stream.
		if (_pStream && eInfo <= _nLvlThresh)
		{
			putLineHeader(eInfo);
			fprintf(_pStream, "Changing target stream to %s\n", strRealPathname.c_str());
			fflush(_pStream);
		}
		
		// Do the change (don't trace FILE* value).
		setStream(pFile, false);
	}
	else
	{
		// Error : can't change to unopenable target file.
		const int nErrNo = errno;
		if (_pStream && eError <= _nLvlThresh)
		{
			putLineHeader(eError);
			fprintf(_pStream, "GfLogger::setStream(%s) : Failed to open file for writing (%s)\n",
					strRealPathname.c_str(), strerror(nErrNo));
			fflush(_pStream);
		}
	}
}

void GfLogger::putLineHeader(int nLevel)
{
	if (nLevel > _nLvlThresh)
		return;
	
	if (_bfHdrCols & eTime)
	{
		char* pszClock = GfTime2Str(GfTimeClock(), 0, true, 3);
		fprintf(_pStream, "%s ", pszClock);
		free(pszClock);
	}
	if (_bfHdrCols & eLogger)
	{
		fprintf(_pStream, "%-8.8s ", _strName.c_str());
	}
	if (_bfHdrCols & eLevel)
	{
		if (nLevel >= eFatal && nLevel <= eDebug)
			fprintf(_pStream, "%-7s ", astrLevelNames[nLevel]);
		else
			fprintf(_pStream, "Level%d  ", nLevel);
	}
}

void GfLogger::fatal(const char *pszFmt, ...)
{
#ifdef TRACE_OUT
	if (_pStream && _nLvlThresh >= eFatal)
	{
		if (_bNeedsHeader)
			putLineHeader(eFatal);
		va_list vaArgs;
		va_start(vaArgs, pszFmt);
		vfprintf(_pStream, pszFmt, vaArgs);
		va_end(vaArgs);
		fflush(_pStream);
		_bNeedsHeader = strrchr(pszFmt, '\n') ? true : false;
	}
#endif // TRACE_OUT

#ifdef WIN32
	MessageBox(NULL, "Please contact the maintenance team\n"
			   "and notify them about the error messages in the console",
			   TEXT("Fatal error"), MB_OK|MB_ICONERROR|MB_SETFOREGROUND);
#endif

	::exit(1);
}

#ifdef TRACE_OUT

void GfLogger::error(const char *pszFmt, ...)
{
	if (_pStream && _nLvlThresh >= eError)
	{
		if (_bNeedsHeader)
			putLineHeader(eError);
		va_list vaArgs;
		va_start(vaArgs, pszFmt);
		vfprintf(_pStream, pszFmt, vaArgs);
		va_end(vaArgs);
		fflush(_pStream);
		_bNeedsHeader = strrchr(pszFmt, '\n') ? true : false;
	}
}

void GfLogger::warning(const char *pszFmt, ...)
{
	if (_pStream && _nLvlThresh >= eWarning)
	{
		if (_bNeedsHeader)
			putLineHeader(eWarning);
		va_list vaArgs;
		va_start(vaArgs, pszFmt);
		vfprintf(_pStream, pszFmt, vaArgs);
		va_end(vaArgs);
		fflush(_pStream);
		_bNeedsHeader = strrchr(pszFmt, '\n') ? true : false;
	}
}

void GfLogger::info(const char *pszFmt, ...)
{
	if (_pStream && _nLvlThresh >= eInfo)
	{
		if (_bNeedsHeader)
			putLineHeader(eInfo);
		va_list vaArgs;
		va_start(vaArgs, pszFmt);
		vfprintf(_pStream, pszFmt, vaArgs);
		va_end(vaArgs);
		fflush(_pStream);
		_bNeedsHeader = strrchr(pszFmt, '\n') ? true : false;
	}
}

void GfLogger::trace(const char *pszFmt, ...)
{
	if (_pStream && _nLvlThresh >= eTrace)
	{
		if (_bNeedsHeader)
			putLineHeader(eTrace);
		va_list vaArgs;
		va_start(vaArgs, pszFmt);
		vfprintf(_pStream, pszFmt, vaArgs);
		va_end(vaArgs);
		fflush(_pStream);
		_bNeedsHeader = strrchr(pszFmt, '\n') ? true : false;
	}
}

void GfLogger::debug(const char *pszFmt, ...)
{
	if (_pStream && _nLvlThresh >= eDebug)
	{
		if (_bNeedsHeader)
			putLineHeader(eDebug);
		va_list vaArgs;
		va_start(vaArgs, pszFmt);
		vfprintf(_pStream, pszFmt, vaArgs);
		va_end(vaArgs);
		fflush(_pStream);
		_bNeedsHeader = strrchr(pszFmt, '\n') ? true : false;
	}
}

void GfLogger::message(int nLevel, const char *pszFmt, ...)
{
	if (_pStream && _nLvlThresh >= nLevel)
	{
		if (_bNeedsHeader)
			putLineHeader(nLevel);
		va_list vaArgs;
		va_start(vaArgs, pszFmt);
		vfprintf(_pStream, pszFmt, vaArgs);
		va_end(vaArgs);
		fflush(_pStream);
		_bNeedsHeader = strrchr(pszFmt, '\n') ? true : false;
	}
}

#endif // TRACE_OUT
