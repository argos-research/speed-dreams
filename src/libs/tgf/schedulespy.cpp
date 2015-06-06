/***************************************************************************

    file        : schedulespy.cpp
    author      : Jean-Philippe Meuret (jpmeuret@free.fr)

    A tool to study the way some special code sections (named "events" here)
    in the program are actually scheduled at a fine grain level.
    Absolute time and duration are logged in memory each time declared sections
    are executed.
    A detailed schedule of these events can be printed to a text file at the end.

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifdef SCHEDULE_SPY

#include <vector>
#include <map>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <locale>

#include <portability.h>

#include "tgf.h"


class GfScheduleEventLog
{
 public:
	GfScheduleEventLog(unsigned nMaxEvents, double dIgnoreDelay);
	void configure(unsigned nMaxEvents, double dIgnoreDelay);
	void reinit(double dZeroTime);
	void beginEvent();
	void endEvent();
	unsigned nbEvents() const { return _vecDurations.size(); };
	float startTime(unsigned nEventInd) const { return _vecStartTimes[nEventInd]; };
	float duration(unsigned nEventInd) const { return _vecDurations[nEventInd]; };
 private:
	double _dZeroTime;
	double _dIgnoreDelay;
	unsigned _nMaxEvents;
	std::vector<float> _vecStartTimes;
	std::vector<float> _vecDurations;
};

class GfScheduleSpy
{
 protected:
	GfScheduleSpy(const char* pszName, std::map<std::string, GfScheduleSpy*>& mapSpies);
 public:
	~GfScheduleSpy();
	static GfScheduleSpy* self(const char* pszName);
	void configureEventLog(const char* pszLogName, unsigned nMaxEvents, double dIgnoreDelay);
	void beginSession();
	void beginEvent(const char* pszLogName);
	void endEvent(const char* pszLogName);
	void endSession();
	void printReport(const char* pszFileName, double fTimeResolution,
					 double fDurationUnit, double fDurationResolution);
 private:
	const char* _pszName;
	std::map<std::string, GfScheduleSpy*>& _rmapSpies;
	double _dZeroTime;
	typedef std::map<std::string, GfScheduleEventLog*> MapEventLogs;
	MapEventLogs _mapEventLogs;
};

// GfScheduleEventLog class implementation //-------------------------------------------------

// 
GfScheduleEventLog::GfScheduleEventLog(unsigned nMaxEvents, double dIgnoreDelay)
{
	configure(nMaxEvents, dIgnoreDelay);
}

// 
void GfScheduleEventLog::configure(unsigned nMaxEvents, double dIgnoreDelay)
{
	_nMaxEvents = nMaxEvents <= _vecStartTimes.max_size() ? nMaxEvents : _vecStartTimes.max_size();
	_dIgnoreDelay = dIgnoreDelay;
}

// Precondition : configure()
void GfScheduleEventLog::reinit(double dZeroTime)
{
	_dZeroTime = dZeroTime;
	_vecStartTimes.reserve(_nMaxEvents);
	_vecDurations.reserve(_nMaxEvents);
}

// Precondition : reinit()
void GfScheduleEventLog::beginEvent()
{
	// Ignore events after the _nMaxEvents'th (to avoid vector resize, which is slow).
	if (_vecStartTimes.size() < _vecStartTimes.capacity())
	{
		const double dNowTime = GfTimeClock();
		if (dNowTime >= _dZeroTime + _dIgnoreDelay)
			_vecStartTimes.push_back((float)(dNowTime - _dZeroTime));
	}
}

// Precondition : beginEvent()
void GfScheduleEventLog::endEvent()
{
	// Ignore events after the _nMaxEvents'th (to avoid vector resize, which is slow).
	if (_vecStartTimes.size() < _vecStartTimes.capacity())
	{
		const double dNowTime = GfTimeClock();
		if (dNowTime >= _dZeroTime + _dIgnoreDelay)
			_vecDurations.push_back((float)(dNowTime - _dZeroTime)
									- _vecStartTimes[_vecStartTimes.size() - 1]);
	}
}

// GfScheduleSpy class implementation //-------------------------------------------------------

GfScheduleSpy::GfScheduleSpy(const char* pszName,
							 std::map<std::string, GfScheduleSpy*>& mapSpies)
: _pszName(pszName), _rmapSpies(mapSpies)
{
}

GfScheduleSpy::~GfScheduleSpy()
{
	_rmapSpies.erase(_pszName);
}

GfScheduleSpy* GfScheduleSpy::self(const char* pszName)
{
	static std::map<std::string, GfScheduleSpy*> mapSpies;
	if (mapSpies.find(pszName) == mapSpies.end())
		mapSpies[pszName] = new GfScheduleSpy(pszName, mapSpies);

	return mapSpies[pszName];
}

void GfScheduleSpy::configureEventLog(const char* pszLogName,
									  unsigned nMaxEvents, double dIgnoreDelay)
{
	if (_mapEventLogs.find(pszLogName) == _mapEventLogs.end())
		_mapEventLogs[pszLogName] = new GfScheduleEventLog(nMaxEvents, dIgnoreDelay);
	else
		_mapEventLogs[pszLogName]->configure(nMaxEvents, dIgnoreDelay);
}

// Precondition : for all needed event logs, configureEventLog(...)
void GfScheduleSpy::beginSession()
{
	GfOut("Beginning schedule spy session\n");
	
	_dZeroTime = GfTimeClock();
	MapEventLogs::iterator iterLogs;
	for (iterLogs = _mapEventLogs.begin(); iterLogs != _mapEventLogs.end(); iterLogs++)
		(*iterLogs).second->reinit(_dZeroTime);
}

// Precondition : beginSession()
void GfScheduleSpy::beginEvent(const char* pszLogName)
{
	if (_mapEventLogs.find(pszLogName) != _mapEventLogs.end())
		_mapEventLogs[pszLogName]->beginEvent();
	else
		GfError("GfScheduleSpy : Can't beginEvent in undeclared event log '%s'\n", pszLogName);
}

// Precondition : beginEvent(pszLogName)
void GfScheduleSpy::endEvent(const char* pszLogName)
{
	if (_mapEventLogs.find(pszLogName) != _mapEventLogs.end())
		_mapEventLogs[pszLogName]->endEvent();
	else
		GfError("GfScheduleSpy : Can't endEvent in undeclared event log '%s'\n", pszLogName);
}

// Precondition : beginSession()
void GfScheduleSpy::endSession()
{
	GfOut("Ending schedule spy session\n");
}

// Precondition : endSession()
void GfScheduleSpy::printReport(const char* pszFileName, double fTimeResolution,
								double fDurationUnit, double fDurationResolution)
{
	// Output file :
	// a) Build absolute path (mandatory storage under GfLocalDir()/debug folder.
	std::ostringstream ossFilePathName;
	ossFilePathName << GfLocalDir() << "debug/" << pszFileName;
	GfOut("Writing schedule spy report to %s\n", ossFilePathName.str().c_str());

	// b) Create parent dir(s) if needed.
	const std::size_t nLastSlashPos = ossFilePathName.str().rfind('/');
	const std::string strDirPathName(ossFilePathName.str().substr(0, nLastSlashPos));
	GfDirCreate(strDirPathName.c_str());

	// c) Finally open in write mode.
	std::ofstream outFStream(ossFilePathName.str().c_str());
	if (!outFStream)
	{
		GfError("Could not open %s for writing report\n", ossFilePathName.str().c_str());
		return;
	}
		
	// Initialize the next event index for each log (a kind of cursor inside each log).
	std::map<std::string, unsigned> mapNextEventInd;
	MapEventLogs::const_iterator itLog;
	for (itLog = _mapEventLogs.begin(); itLog != _mapEventLogs.end(); itLog++)
	{
		const std::string& strLogName = (*itLog).first;
		mapNextEventInd[strLogName] = 0;
	}

	// Print columns header (1st line : name of each event log).
	std::ostringstream ossStepLine;
	for (itLog = _mapEventLogs.begin(); itLog != _mapEventLogs.end(); itLog++)
	{
		const std::string& strLogName = (*itLog).first;
		ossStepLine << '\t' << strLogName << '\t';
	}
	outFStream << ossStepLine.str() << std::endl;

	// Print columns header (2nd line : Contents description for each column).
	ossStepLine.str("");
	ossStepLine.imbue(std::locale("")); // Set stream locale to the OS-level-defined one.
	ossStepLine << "Time";
	for (itLog = _mapEventLogs.begin(); itLog != _mapEventLogs.end(); itLog++)
	{
		const std::string& strLogName = (*itLog).first;
		ossStepLine << "\tStart\tDuration";
	}
	outFStream << ossStepLine.str() << std::endl;
	
	// For each time step (fTimeResolution), print events info.
	const int nTimePrecision = (int)ceil(-log10(fTimeResolution));
	const int nDurationPrecision = (int)ceil(-log10(fDurationResolution / fDurationUnit));
	double dRelTime = 0.0;
	bool bEventAreLeft = true;
	ossStepLine << std::fixed;
	while (bEventAreLeft)
	{
		// As many lines per step as necessary to print all avents inside.
		bool bEventAreLeftInStep = true;
		while (bEventAreLeftInStep)
		{
			unsigned nbProcessedEvents = 0;
			ossStepLine.str("");

			// 1st column = step start time.
			ossStepLine << std::setprecision(nTimePrecision) << dRelTime;
			ossStepLine << std::setprecision(nDurationPrecision);
			for (itLog = _mapEventLogs.begin(); itLog != _mapEventLogs.end(); itLog++)
			{
				// Then 2 columns per event : relative start time and duration of the event.
				const std::string& strLogName = (*itLog).first;
				const GfScheduleEventLog* pEventLog = (*itLog).second;
				const unsigned& nEventInd = mapNextEventInd[strLogName];
				if (nEventInd < pEventLog->nbEvents()
					&& pEventLog->startTime(nEventInd) >= dRelTime
					&& pEventLog->startTime(nEventInd) < dRelTime + fTimeResolution)
				{
					ossStepLine << '\t'
								<< (pEventLog->startTime(nEventInd) - dRelTime) / fDurationUnit
								<< '\t'
								<< pEventLog->duration(nEventInd) / fDurationUnit;
					mapNextEventInd[strLogName]++; // Event processed.
					nbProcessedEvents++;
				}
				else
				{
					ossStepLine << "\t\t";
				}
			}
			
			// Print report line if any was produced,
			// and check if any event left to process in this time step.
			if (nbProcessedEvents > 0)
			{
				outFStream << ossStepLine.str() << std::endl;
			}
			else
			{
				bEventAreLeftInStep = false;
			}
		}

		// Check if any event left to process.
		bEventAreLeft = false;
		for (itLog = _mapEventLogs.begin(); itLog != _mapEventLogs.end(); itLog++)
		{
			const std::string& strLogName = (*itLog).first;
			const GfScheduleEventLog* pEventLog = (*itLog).second;
			if (mapNextEventInd[strLogName] < pEventLog->nbEvents())
			{
				bEventAreLeft = true;
				break;
			}
		}
				
		// Next time step.
		dRelTime += fTimeResolution;
	}

	outFStream.close();
}

// C interface functions //-----------------------------------------------------------------

/* Configure an event log before using it (can be called more than once to change settings)
   @ingroup schedulespy
   \param pszLogName   Name/id
   \param nMaxEvents   Maximum number of events to be logged (other ignored)
   \param dIgnoreDelay Delay (s) before taking Begin/EndEvent into account (from BeginSession time) */
void GfSchedConfigureEventLog(const char* pszSpyName, const char* pszLogName,
							  unsigned nMaxEvents, double dIgnoreDelay)
{
	GfScheduleSpy::self(pszSpyName)->configureEventLog(pszLogName, nMaxEvents, dIgnoreDelay);
}

/* Start a new spying session
   @ingroup schedulespy
   \precondition All event logs must be configured at least once before) */
void GfSchedBeginSession(const char* pszSpyName)
{
	GfScheduleSpy::self(pszSpyName)->beginSession();
}

/* Log the beginning of an event (enter the associated code section)
   @ingroup schedulespy
   \precondition BeginSession
   \param pszLogName   Name/id                                        */
void GfSchedBeginEvent(const char* pszSpyName, const char* pszLogName)
{
	GfScheduleSpy::self(pszSpyName)->beginEvent(pszLogName);
}

/* Log the end of an event (exit from the associated code section)
   @ingroup schedulespy
   \precondition BeginEvent(pszLogName)
   \param pszLogName   Name/id                                        */
void GfSchedEndEvent(const char* pszSpyName, const char* pszLogName)
{
	GfScheduleSpy::self(pszSpyName)->endEvent(pszLogName);
}

/* Terminate the current spying session
   @ingroup schedulespy
   \precondition BeginSession             */
void GfSchedEndSession(const char* pszSpyName)
{
	GfScheduleSpy::self(pszSpyName)->endSession();
}

/* Print a table log :
   * each time step (duration = fTimeResolution) is displayed on as many lines
     as necessary to log all the events that occured during the step
   * 1st column = step start time, in seconds since the call to BeginSession
   * 2 columns for each event log : start time (relative to the step start) and duration
   @ingroup schedulespy
   \param pszFileName          Target text file for the log
   \param fTimeResolution      Resolution to use for time = duration of 1 step
   \param fDurationUnit        Unit to use for durations (1 = 1s, 1.0e-3 = 1ms ...)
   \param fDurationResolution  Resolution to use for durations (must divide fDurationUnit)
   \precondition EndSession                                                     */
void GfSchedPrintReport(const char* pszSpyName, const char* pszFileName,
						double fTimeResolution,	double fDurationUnit, double fDurationResolution)
{
	std::ostringstream ossFileName;
	ossFileName << pszSpyName << '-' << pszFileName;
	GfScheduleSpy::self(pszSpyName)->printReport(ossFileName.str().c_str(), fTimeResolution,
												 fDurationUnit, fDurationResolution);
}

#endif // SCHEDULE_SPY
