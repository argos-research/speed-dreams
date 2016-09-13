/***************************************************************************

    file                 : racemanagers.cpp
    created              : December 2010
    copyright            : (C) 2010 Jean-Philippe Meuret
    web                  : speed-dreams.sourceforge.net
    version              : $Id: racemanagers.cpp 5899 2014-12-17 21:00:23Z wdbee $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <map>
#include <sstream>
#include <algorithm>

#include <raceman.h>

#include "cars.h"
#include "tracks.h"
#include "drivers.h"
#include "racemanagers.h"


// An empty string.
static const std::string strEmpty;

class GfRaceManagers::Private
{
public:
	
	// One GfRaceManager structure for each car (no special order).
	std::vector<GfRaceManager*> vecRaceMans;

	// Map for quick access to GfRaceManager by id
	std::map<std::string, GfRaceManager*> mapRaceMansById;

	// Vector of type names, ordered by decreasing priority.
	std::vector<std::string> vecTypes;
};


GfRaceManagers* GfRaceManagers::_pSelf = 0;

GfRaceManagers *GfRaceManagers::self()
{
	if (!_pSelf)
		_pSelf = new GfRaceManagers;
	
	return _pSelf;
}

void GfRaceManagers::shutdown()
{
	delete _pSelf;
	_pSelf = 0;
}

GfRaceManagers::~GfRaceManagers()
{
	std::vector<GfRaceManager*>::const_iterator itRaceMan;
	for (itRaceMan = _pPrivate->vecRaceMans.begin();
		 itRaceMan != _pPrivate->vecRaceMans.end(); itRaceMan++)
		delete *itRaceMan;

	delete _pPrivate;
	_pPrivate = 0;
}

static bool hasHigherPriority(const GfRaceManager* pLeft, const GfRaceManager* pRight)
{
	return pLeft->getPriority() > pRight->getPriority();
}
	
GfRaceManagers::GfRaceManagers()
{
	_pPrivate = new Private;

	// Get the list of racemans from the xml files available in GfDataDir()/config/raceman folder
	tFList* lstFiles = GfDirGetListFiltered("config/raceman", "", PARAMEXT);
	if (!lstFiles)
	{
		GfLogFatal("No race manager available in %sconfig/raceman\n", GfDataDir());
		return;
	}

	// For each found XML file :
	tFList* pFile = lstFiles;
	do 
	{
		// Open the XML descriptor file (look first in user settings, then in the install folder).
		std::ostringstream ossRaceManFileName;
		ossRaceManFileName << GfLocalDir() << "config/raceman/" << pFile->name;
		void* hparmRaceMan = GfParmReadFile(ossRaceManFileName.str().c_str(), GFPARM_RMODE_STD);
		if (!hparmRaceMan)
		{
			ossRaceManFileName.str("");
			ossRaceManFileName << "config/raceman/" << pFile->name;
			hparmRaceMan = GfParmReadFile(ossRaceManFileName.str().c_str(), GFPARM_RMODE_STD);

			// We got if from the data folder : write it to the user settings.
			if (hparmRaceMan)
			{
				ossRaceManFileName.str("");
				ossRaceManFileName << GfLocalDir() << "config/raceman/" << pFile->name;
				GfParmWriteFile(ossRaceManFileName.str().c_str(), hparmRaceMan, NULL);
			}
		}
		
		std::string strRaceManId(pFile->name);
		strRaceManId.erase(strlen(pFile->name) - strlen(PARAMEXT));
		if (!hparmRaceMan)
		{
			GfLogInfo("GfRaceManagers : Ignoring race manager %s (failed to read from config/raceman/%s in %s and %s)\n",
						 strRaceManId.c_str(), pFile->name, GfLocalDir(), GfDataDir());
			continue;
		}

		// Create the race manager and load it from the params file.
		//GfLogDebug("GfRaceManagers::GfRaceManagers: creating '%s'\n", strRaceManId.c_str());
		GfRaceManager* pRaceMan = new GfRaceManager(strRaceManId, hparmRaceMan);

		// Update the GfRaceManagers singleton.
		_pPrivate->vecRaceMans.push_back(pRaceMan);
		_pPrivate->mapRaceMansById[strRaceManId] = pRaceMan;
	} 
	while ((pFile = pFile->next) != lstFiles);
	
	GfDirFreeList(lstFiles, NULL, true, true);

	// Sort the race manager vector by priority.
	std::sort(_pPrivate->vecRaceMans.begin(), _pPrivate->vecRaceMans.end(), hasHigherPriority);

	// Fill the race manager type vector (=> also sorted by priority).
	std::vector<GfRaceManager*>::const_iterator itRaceMan;
	for (itRaceMan = _pPrivate->vecRaceMans.begin();
		 itRaceMan != _pPrivate->vecRaceMans.end(); itRaceMan++)
		if (std::find(_pPrivate->vecTypes.begin(), _pPrivate->vecTypes.end(), (*itRaceMan)->getType())
			== _pPrivate->vecTypes.end())
			_pPrivate->vecTypes.push_back((*itRaceMan)->getType());

	// And log what we've got now.
	print(/* bVerbose */false);
}

const std::vector<std::string>& GfRaceManagers::getTypes() const
{
	return _pPrivate->vecTypes;
}

GfRaceManager* GfRaceManagers::getRaceManager(const std::string& strId) const
{
	std::map<std::string, GfRaceManager*>::const_iterator itRaceMan =
		_pPrivate->mapRaceMansById.find(strId);
	if (itRaceMan != _pPrivate->mapRaceMansById.end())
		return itRaceMan->second;
	
	return 0;
}

GfRaceManager* GfRaceManagers::getRaceManagerWithName(const std::string& strName) const
{
	std::vector<GfRaceManager*>::iterator itRaceMan;
	for (itRaceMan = _pPrivate->vecRaceMans.begin();
		 itRaceMan != _pPrivate->vecRaceMans.end(); itRaceMan++)
		if ((*itRaceMan)->getName() == strName)
			return *itRaceMan;

	return 0;
}

std::vector<GfRaceManager*> GfRaceManagers::getRaceManagersWithType(const std::string& strType) const
{
	std::vector<GfRaceManager*> vecRaceMans;

	std::vector<GfRaceManager*>::iterator itRaceMan;
	for (itRaceMan = _pPrivate->vecRaceMans.begin(); itRaceMan != _pPrivate->vecRaceMans.end(); itRaceMan++)
		if (strType.empty() || (*itRaceMan)->getType() == strType)
			vecRaceMans.push_back(*itRaceMan);

	return vecRaceMans;
}

void GfRaceManagers::print(bool bVerbose) const
{
	GfLogTrace("Race managers : %d types, %d race managers\n",
			   _pPrivate->vecTypes.size(), _pPrivate->vecRaceMans.size());
	std::vector<std::string>::const_iterator itType;
	for (itType = _pPrivate->vecTypes.begin();
		 itType != _pPrivate->vecTypes.end(); itType++)
	{
		GfLogTrace("  %s type :\n", itType->c_str());
		const std::vector<GfRaceManager*> vecRaceMans =
			getRaceManagersWithType(itType->c_str());
		std::vector<GfRaceManager*>::const_iterator itRaceMan;
		for (itRaceMan = vecRaceMans.begin(); itRaceMan != vecRaceMans.end(); itRaceMan++)
		{
			GfLogTrace("    %s : subtype='%s', name='%s', prio=%d, events=%d\n",
					   (*itRaceMan)->getId().c_str(), (*itRaceMan)->getSubType().c_str(),
					   (*itRaceMan)->getName().c_str(), (*itRaceMan)->getPriority(),
					   bVerbose ? (*itRaceMan)->getEventCount() : -1);
		}
	}
}

// GfRaceManager class ---------------------------------------------------------------

GfRaceManager::GfRaceManager(const std::string& strId, void* hparmHandle)
{
	_strId = strId;

	// Load constant properties (never changed afterwards).
	// 1) Name, type, sub-type and priority (ordering the buttons in the race select menu).
	_strName = GfParmGetStr(hparmHandle, RM_SECT_HEADER, RM_ATTR_NAME, "<unknown>");
	_strType = GfParmGetStr(hparmHandle, RM_SECT_HEADER, RM_ATTR_TYPE, "<unknown>");
	_strSubType = GfParmGetStr(hparmHandle, RM_SECT_HEADER, RM_ATTR_SUBTYPE, "");
	_nPriority = (int)GfParmGetNum(hparmHandle, RM_SECT_HEADER, RM_ATTR_PRIO, NULL, 10000);

	// 2) Accepted driver types.
	static const char cFilterSeparator = ';';

	// a) Load accepted driver types from a ';'-separated string (ignore duplicates).
	const char* pszAcceptDrvTypes =
		GfParmGetStr(hparmHandle, RM_SECT_DRIVERS, RM_ATTR_ACCEPT_TYPES, "");
    std::stringstream ssAcceptDrvTypes(pszAcceptDrvTypes);
    std::string strDrvType;
    while(std::getline(ssAcceptDrvTypes, strDrvType, cFilterSeparator))
	{
		std::vector<std::string>::iterator itDrvType =
			std::find(_vecAcceptedDriverTypes.begin(), _vecAcceptedDriverTypes.end(), strDrvType);
		if (itDrvType == _vecAcceptedDriverTypes.end()) // Not already there => store it.
			_vecAcceptedDriverTypes.push_back(strDrvType);
    }

	// b) If the race file finally doesn't specify any accepted driver type,
	//    accpet them all.
	if (_vecAcceptedDriverTypes.empty())
		_vecAcceptedDriverTypes = GfDrivers::self()->getTypes();

	// c) Load rejected driver types from a ';'-separated string (ignore duplicates).
	const char* pszRejectDrvTypes =
		GfParmGetStr(hparmHandle, RM_SECT_DRIVERS, RM_ATTR_REJECT_TYPES, "");
    std::stringstream ssRejectDrvTypes(pszRejectDrvTypes);
    while(std::getline(ssRejectDrvTypes, strDrvType, cFilterSeparator))
	{
		std::vector<std::string>::iterator itDrvType =
			std::find(_vecAcceptedDriverTypes.begin(), _vecAcceptedDriverTypes.end(), strDrvType);
		if (itDrvType != _vecAcceptedDriverTypes.end()) // Accepted til now => now rejected.
			_vecAcceptedDriverTypes.erase(itDrvType);
    }
	
	// 3) Accepted car categories.
	// a) Load accepted car categories from a ';'-separated string (ignore duplicates).
	const char* pszAcceptCarCats =
		GfParmGetStr(hparmHandle, RM_SECT_DRIVERS, RM_ATTR_ACCEPT_CATEGORIES, "");
    std::stringstream ssAcceptCarCats(pszAcceptCarCats);
    std::string strCarCat;
    while(std::getline(ssAcceptCarCats, strCarCat, cFilterSeparator))
	{
		std::vector<std::string>::iterator itCarCat =
			std::find(_vecAcceptedCarCategoryIds.begin(), _vecAcceptedCarCategoryIds.end(), strCarCat);
		if (itCarCat == _vecAcceptedCarCategoryIds.end()) // Not already there => store it.
			_vecAcceptedCarCategoryIds.push_back(strCarCat);
    }

	// b) If the race file finally doesn't specify any accepted car category,
	//    accpet them all.
	if (_vecAcceptedCarCategoryIds.empty())
		_vecAcceptedCarCategoryIds = GfCars::self()->getCategoryIds();

	// c) Load rejected car categories from a ';'-separated string (ignore duplicates).
	const char* pszRejectCarCats =
		GfParmGetStr(hparmHandle, RM_SECT_DRIVERS, RM_ATTR_REJECT_CATEGORIES, "");
    std::stringstream ssRejectCarCats(pszRejectCarCats);
    while(std::getline(ssRejectCarCats, strCarCat, cFilterSeparator))
	{
		std::vector<std::string>::iterator itCarCat =
			std::find(_vecAcceptedCarCategoryIds.begin(), _vecAcceptedCarCategoryIds.end(), strCarCat);
		if (itCarCat != _vecAcceptedCarCategoryIds.end()) // Accepted til now => now rejected.
			_vecAcceptedCarCategoryIds.erase(itCarCat);
    }

	// Load other "mutable" properties.
	reset(hparmHandle, false);
}

void GfRaceManager::reset(void* hparmHandle, bool bClosePrevHdle)
{
	if (bClosePrevHdle && _hparmHandle)
		GfParmReleaseHandle(_hparmHandle);
	_hparmHandle = hparmHandle;

	// Clear the event list
	_vecEventTrackIds.clear();

	// Clear the session name list
	_vecSessionNames.clear();
	
	// No more ready for serialization (in memory, to params) for the moment.
	_bIsDirty = false;
}

// This methos is const because we want it to be called by const methods. No other way.
void GfRaceManager::load() const
{
	// Determine the race manager file from which to load the events info.
	// 1) Simple case : the race manager has no subfiles, use the normal file.
	// 2) Career case : the events are defined in "sub-championships" files.
	//    WARNING: Very partial support for the moment : we only care about the 1st event
	//             ('cause it is quite complicated, with intermixed events between
	//              the other "sub-championships", each defined in a career_<sub-champ>.xmls).
	//             This is not an issue as long as this class is not used in the race engine
	//             or the event management purpose (most of it is in racecareer.cpp for now).
	void* hparmHandle = _hparmHandle;
	const char* pszHasSubFiles =
		GfParmGetStr(_hparmHandle, RM_SECT_SUBFILES, RM_ATTR_HASSUBFILES, RM_VAL_NO);
	_bHasSubFiles = strcmp(pszHasSubFiles, RM_VAL_YES) == 0;
	if (_bHasSubFiles)
	{
		const char* psz1stSubFileName =
			GfParmGetStr(_hparmHandle, RM_SECT_SUBFILES, RM_ATTR_FIRSTSUBFILE, 0);
		if (psz1stSubFileName)
		{
			std::ostringstream ossSubFilePath;
			ossSubFilePath << GfLocalDir() << "config/raceman/" << psz1stSubFileName;
			hparmHandle = GfParmReadFile(ossSubFilePath.str().c_str(), GFPARM_RMODE_STD);
		}
		if (!psz1stSubFileName || !hparmHandle)
		{
			_bHasSubFiles = false;
			hparmHandle = _hparmHandle;
		}
	}

// 	GfLogDebug("GfRaceManager::reset(%s): %s %s\n",
// 			   _strName.c_str(), _hparmHandle == hparmHandle ? "file" : "sub-file",
// 			   GfParmGetFileName(hparmHandle));

	// Clear the event list
	_vecEventTrackIds.clear();

	// And reload it
	// (warning: here, we only check the tracks usability when the specified one was not found).
	std::ostringstream ossSectionPath;
	int nEventNum = 1;
	const char* pszTrackId;
	do
	{
		// Get event track name.
		ossSectionPath.str("");
		ossSectionPath << RM_SECT_TRACKS << '/' << nEventNum;
		pszTrackId = GfParmGetStr(hparmHandle, ossSectionPath.str().c_str(), RM_ATTR_NAME, 0);
//  		GfLogDebug("GfRaceManager::reset(...): event[%d].track = '%s'\n",
//  				   nEventNum-1, pszTrackId);

		// If not end of event list :
		if (pszTrackId)
		{
			// If no such track, try and get the first usable one in the existing categories.
			if (!GfTracks::self()->getTrack(pszTrackId))
			{
				// Get the track category.
				const char* pszCatId =
					GfParmGetStr(hparmHandle, ossSectionPath.str().c_str(), RM_ATTR_CATEGORY, 0);

				// Get the first usable track in the same category,
				// or else in the next categories.
				GfTrack* pTrack = GfTracks::self()->getFirstUsableTrack(pszCatId, pszTrackId, +1, true);
				if (!pTrack)
					pTrack = GfTracks::self()->getFirstUsableTrack(pszCatId, +1, true);

				// If found, select this one for the the event.
				if (pTrack)
				{
					GfLogWarning("Replacing non-existing track '%s' by first usable '%s' "
								 "(event #%d) for %s mode\n", pszTrackId,
								 pTrack->getId().c_str(), nEventNum, _strName.c_str());

					pszTrackId = pTrack->getId().c_str();

					// Now we are no more consistent with the race managers params (in memory).
					_bIsDirty = true;
				}
				else
				{
					// Should never happen : no usable track.
					GfLogError("Skipping non-existing track '%s' (event #%d) for %s mode"
							   " and no other usable track ; let's start praying ...\n",
							   pszTrackId, nEventNum, _strName.c_str());
					break;
				}
			}

			// We got it.
			_vecEventTrackIds.push_back(pszTrackId);

			// Next event.
			nEventNum++;
		}
	}
	while (pszTrackId);
	
	// Clear the session name list
	_vecSessionNames.clear();

	// Session names.
	std::ostringstream ossSecPath;
	int nSessionInd = 1;
    while (nSessionInd <= (int)GfParmGetEltNb(hparmHandle, RM_SECT_RACES))
	{
		ossSecPath.str("");
		ossSecPath << RM_SECT_RACES << '/' << nSessionInd;
		const char* pszSessionName =
			GfParmGetStr(hparmHandle, ossSecPath.str().c_str(), RM_ATTR_NAME, 0);
//  		GfLogDebug("GfRaceManager::reset(...): session '%s'\n", pszSessionName);
		if (pszSessionName && strlen(pszSessionName) > 0)
			_vecSessionNames.push_back(pszSessionName);

		// Next session.
		nSessionInd++;
	}
}

void GfRaceManager::store()
{
	if (!_hparmHandle)
		return;

	// Note: No need to save constant properties (never changed).
	
	// Info about each event in the schedule.
	// WARNING: Not supported for Career mode (TODO ?).
	if (!_bHasSubFiles)
	{
		// a) clear the event list.
		GfParmListClean(_hparmHandle, RM_SECT_TRACKS);

		// b) re-create it from the current event list state (may have changed since loaded).
		std::ostringstream ossSectionPath;
		for (unsigned nEventInd = 0; nEventInd < _vecEventTrackIds.size(); nEventInd++)
		{
// 			GfLogDebug("GfRaceManager::store(%s): event[%u].track = '%s'\n",
// 					   _strName.c_str(), nEventInd, _vecEventTrackIds[nEventInd].c_str());
			ossSectionPath.str("");
			ossSectionPath << RM_SECT_TRACKS << '/' << nEventInd + 1;
			GfParmSetStr(_hparmHandle, ossSectionPath.str().c_str(), RM_ATTR_NAME,
						 _vecEventTrackIds[nEventInd].c_str());
			GfTrack* pTrack = GfTracks::self()->getTrack(_vecEventTrackIds[nEventInd]);
			GfParmSetStr(_hparmHandle, ossSectionPath.str().c_str(), RM_ATTR_CATEGORY,
						 pTrack->getCategoryId().c_str());
		}
	}

	// Now we are consistent with the race managers params (in memory).
	_bIsDirty = false;
}

GfRaceManager::~GfRaceManager()
{
	if (_hparmHandle)
		GfParmReleaseHandle(_hparmHandle);
}
			 
const std::string& GfRaceManager::getId() const
{
	return _strId;
}

void* GfRaceManager::getDescriptorHandle() const
{
	return _hparmHandle;
}

std::string GfRaceManager::getDescriptorFileName() const
{
	return const_cast<const char*>(GfParmGetFileName(_hparmHandle));
}

const std::string& GfRaceManager::getName() const
{
	return _strName;
}

const std::string& GfRaceManager::getType() const
{
	return _strType;
}

const std::string& GfRaceManager::getSubType() const
{
	return _strSubType;
}

int GfRaceManager::getPriority() const
{
	return _nPriority;
}

bool GfRaceManager::isNetwork() const
{
	return _strType == "Online";
}

bool GfRaceManager::acceptsDriverType(const std::string& strType) const
{
	return std::find(_vecAcceptedDriverTypes.begin(), _vecAcceptedDriverTypes.end(), strType)
		   != _vecAcceptedDriverTypes.end();
}

const std::vector<std::string>& GfRaceManager::getAcceptedDriverTypes() const
{
	return _vecAcceptedDriverTypes;
}
	
bool GfRaceManager::acceptsCarCategory(const std::string& strCatId) const
{
	return std::find(_vecAcceptedCarCategoryIds.begin(), _vecAcceptedCarCategoryIds.end(), strCatId)
		   != _vecAcceptedCarCategoryIds.end();
}

const std::vector<std::string>& GfRaceManager::getAcceptedCarCategoryIds() const
{
	return _vecAcceptedCarCategoryIds;
}

unsigned GfRaceManager::getEventCount() const
{
	if (_vecEventTrackIds.empty())
		load(); // Lazy loading.
	
	return _vecEventTrackIds.size();
}

bool GfRaceManager::isMultiEvent() const
{
	if (_vecEventTrackIds.empty())
		load(); // Lazy loading.
	
	return _vecEventTrackIds.size() > 1;
}

GfTrack* GfRaceManager::getEventTrack(unsigned nEventIndex)
{
	if (_vecEventTrackIds.empty())
		load(); // Lazy loading.
	
	GfTrack* pTrack = 0;

	if (!_vecEventTrackIds.empty())
	{
		if (nEventIndex >= _vecEventTrackIds.size())
			nEventIndex = _vecEventTrackIds.size() - 1;
	
		pTrack =
			GfTracks::self()->getTrack(_vecEventTrackIds[nEventIndex]);
	}
	
	return pTrack;
}

void GfRaceManager::setEventTrack(unsigned nEventIndex, GfTrack* pTrack)
{
	if (_vecEventTrackIds.empty())
		load(); // Lazy loading.
	
	if (!pTrack || _vecEventTrackIds.empty())
		return;
	
	if (nEventIndex >= _vecEventTrackIds.size())
		nEventIndex = _vecEventTrackIds.size() - 1;

	_vecEventTrackIds[nEventIndex] = pTrack->getId();
// 	GfLogDebug("GfRaceManager::setEventTrack(evt #%u, track '%s')\n",
// 			   nEventIndex, pTrack->getId().c_str());

	// Now we are no more consistent with the race managers params (in memory).
	_bIsDirty = true;
}

GfTrack* GfRaceManager::getPreviousEventTrack(unsigned nEventIndex)
{
	if (_vecEventTrackIds.empty())
		load(); // Lazy loading.
	
	GfTrack* pTrack = 0;

	if (!_vecEventTrackIds.empty())
	{
		if (nEventIndex >= _vecEventTrackIds.size())
			nEventIndex = _vecEventTrackIds.size() - 1;

		const unsigned nPrevEventIndex = // Beware: Previous of 1st = last.
			(nEventIndex > 0) ? nEventIndex - 1 : _vecEventTrackIds.size() - 1;

		pTrack = GfTracks::self()->getTrack(_vecEventTrackIds[nPrevEventIndex]);
	}
		   
	return pTrack;
}

const std::vector<std::string>& GfRaceManager::getSessionNames() const
{
	if (_vecSessionNames.empty())
		load(); // Lazy loading.
	
	return _vecSessionNames;
}

unsigned GfRaceManager::getSessionCount() const
{
	if (_vecSessionNames.empty())
		load(); // Lazy loading.
	
	return _vecSessionNames.size();
}

const std::string& GfRaceManager::getSessionName(unsigned nIndex) const
{
	if (_vecSessionNames.empty())
		load(); // Lazy loading.
	
	if (_vecSessionNames.empty())
		return strEmpty;
	
	if (nIndex >= _vecSessionNames.size())
		nIndex = _vecSessionNames.size() - 1;
	
	return _vecSessionNames[nIndex];
}

const std::string& GfRaceManager::getSavedConfigsDir() const
{
	if (_strSavedConfigsDir.empty())
	{
		_strSavedConfigsDir = GfLocalDir();
		_strSavedConfigsDir += "config/raceman/";
		_strSavedConfigsDir += _strId;
	}
	
	return _strSavedConfigsDir;
}

bool GfRaceManager::hasSavedConfigsFiles() const
{
	tFList *pFileList = GfDirGetListFiltered(getSavedConfigsDir().c_str(), "", PARAMEXT);

	// Now we know what to answer.
	const bool bAnswer = (pFileList != 0);

	// Free the file list.
	GfDirFreeList(pFileList, 0, true, true);

	// Answer.
	return bAnswer;
}

const std::string& GfRaceManager::getResultsDir() const
{
	if (_strResultsDir.empty())
	{
		_strResultsDir = GfLocalDir();
		_strResultsDir += "results/";
		_strResultsDir += _strId;
	}
	
	return _strResultsDir;
}

bool GfRaceManager::hasResultsFiles() const
{
	tFList *pFileList = GfDirGetListFiltered(getResultsDir().c_str(), "", RESULTEXT);

	// Now we know what to answer.
	const bool bAnswer = (pFileList != 0);

	// Free the file list.
	GfDirFreeList(pFileList, 0, true, true);

	// Answer.
	return bAnswer;
}

bool GfRaceManager::hasSubFiles() const
{
	return _bHasSubFiles;
}

bool GfRaceManager::isDirty() const
{
	return _bIsDirty;
}
