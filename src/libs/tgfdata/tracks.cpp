/***************************************************************************

    file                 : tracks.cpp
    created              : Sun Nov 21 19:00:00 CET 2010
    copyright            : (C) 2010 by Jean-Philippe MEURET
    web                  : speed-dreams.sourceforge.net
    version              : $Id: tracks.cpp 5899 2014-12-17 21:00:23Z wdbee $
                     
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

#include <tgf.h>
#include <itrackloader.h>

#include "tracks.h"

				  
// Private data for GfTracks				  
class GfTracks::Private
{
public:

	Private() : piTrackLoader(0) {}
	
public:
	
	// One GfTrack structure for each track (order = sorted directory one).
	std::vector<GfTrack*> vecTracks;

	// Map for quick access to GfTrack by id
	std::map<std::string, GfTrack*> mapTracksById;

	// Vector of category Ids.
	std::vector<std::string> vecCatIds;

	// Vector of category names.
	std::vector<std::string> vecCatNames;

	// Track module interface (needed to get some tracks properties).
	ITrackLoader* piTrackLoader;
};


GfTracks* GfTracks::_pSelf = 0;

GfTracks *GfTracks::self()
{
	if (!_pSelf)
		_pSelf = new GfTracks;
	
	return _pSelf;
}

void GfTracks::shutdown()
{
	delete _pSelf;
	_pSelf = 0;
}

GfTracks::~GfTracks()
{
	std::vector<GfTrack*>::const_iterator itTrack;
	for (itTrack = _pPrivate->vecTracks.begin();
		 itTrack != _pPrivate->vecTracks.end(); itTrack++)
		delete *itTrack;

	delete _pPrivate;
	_pPrivate = 0;
}

GfTracks::GfTracks()
{
	_pPrivate = new GfTracks::Private;

	// Get the list of sub-dirs in the "tracks" folder (the track categories).
	tFList* lstCatFolders = GfDirGetList("tracks");
	if (!lstCatFolders)
	{
		GfLogFatal("No track category available in the 'tracks' folder\n");
		return;
	}
	
	tFList* pCatFolder = lstCatFolders;
	do 
	{
		//GfLogDebug("GfTracks::GfTracks() : Examining category %s\n", pCatFolder->name);
		
		// Ignore "." and ".." folders.
		const char* pszCatId = pCatFolder->name;
		if (pszCatId[0] == '.') 
			continue;
			
		// Get the list of sub-dirs in the "tracks" folder (the track categories).
		std::string strDirName("tracks/");
		strDirName += pszCatId;
		tFList* lstTrackFolders = GfDirGetList(strDirName.c_str());
		if (!lstTrackFolders)
		{
			GfLogWarning("No track available in the '%s' folder\n", strDirName.c_str());
			continue;
		}

		// Add new category.
		_pPrivate->vecCatIds.push_back(pszCatId);

		// Look at the tracks in this category.
		tFList* pTrackFolder = lstTrackFolders;
		do 
		{
			//GfLogDebug("GfTracks::GfTracks() : Examining track %s\n", pTrackFolder->name);
		
			// Determine and check the XML file of the track.
			const char* pszTrackId = pTrackFolder->name;
			
			std::ostringstream ossFileName;
			ossFileName << "tracks/" << pszCatId << '/' << pszTrackId
						<< '/' << pszTrackId << '.' << TRKEXT;
			const std::string strTrackFileName(ossFileName.str());
			if (!GfFileExists(strTrackFileName.c_str()))
			{
				GfLogInfo("Ignoring track %s : %s not found\n",
							 pszTrackId, strTrackFileName.c_str());
				continue;
			}

			// Get 1st level track info (those which don't need to open any file).
			ossFileName.str("");
			ossFileName << "tracks/" << pszCatId << '/' << pszTrackId
						<< '/' << pszTrackId << ".jpg";
			std::string strPreviewFileName(ossFileName.str());
			if (!GfFileExists(strPreviewFileName.c_str()))
			{
				ossFileName.str("");
				ossFileName << "tracks/" << pszCatId << '/' << pszTrackId
							<< '/' << pszTrackId << ".png";
				strPreviewFileName = ossFileName.str();
			}
			if (!GfFileExists(strPreviewFileName.c_str()))
				strPreviewFileName = "data/img/splash-trackselect.jpg";
			
			ossFileName.str("");
			ossFileName << "tracks/" << pszCatId << '/' << pszTrackId << '/' << "outline.png";
			std::string strOutlineFileName(ossFileName.str());
			if (!GfFileExists(strOutlineFileName.c_str()))
				strOutlineFileName = "data/img/notrackoutline.png";
			
			// Store track info in the GfTrack structure.
			GfTrack* pTrack = new GfTrack;
			pTrack->setId(pszTrackId);
			pTrack->setCategoryId(pszCatId);
			pTrack->setDescriptorFile(strTrackFileName);
			pTrack->setPreviewFile(strPreviewFileName);
			pTrack->setOutlineFile(strOutlineFileName);

			// Update the GfTracks singleton.
			_pPrivate->vecTracks.push_back(pTrack);
			_pPrivate->mapTracksById[pszTrackId] = pTrack;
		}
		while ((pTrackFolder = pTrackFolder->next) != lstTrackFolders);
		
		GfDirFreeList(lstTrackFolders, NULL, true, true);
	} 
	while ((pCatFolder = pCatFolder->next) != lstCatFolders);
	
	GfDirFreeList(lstCatFolders, NULL, true, true);
	
	// Sort the car category ids and driver types vectors.
	std::sort(_pPrivate->vecCatIds.begin(), _pPrivate->vecCatIds.end());

	// Trace what we got.
	print(false); // No verbose here, otherwise infinite recursion.
}

ITrackLoader* GfTracks::getTrackLoader() const
{
	return _pPrivate->piTrackLoader;
}

void GfTracks::setTrackLoader(ITrackLoader* piTrackLoader)
{
	_pPrivate->piTrackLoader = piTrackLoader;
}

const std::vector<std::string>& GfTracks::getCategoryIds() const
{
	return _pPrivate->vecCatIds;
}

const std::vector<std::string>& GfTracks::getCategoryNames() const
{
	// Get category names only when asked for (need to open files for this).
	if (_pPrivate->vecCatNames.empty())
	{
		std::vector<std::string>::const_iterator itCatId;
		for (itCatId = _pPrivate->vecCatIds.begin(); itCatId != _pPrivate->vecCatIds.end(); itCatId++)
		{
			std::ostringstream ossFileName;
			ossFileName << "data/tracks/" << *itCatId << '.' << TRKEXT;
			void* hparmCat = GfParmReadFile(ossFileName.str().c_str(), GFPARM_RMODE_STD);
			const char* pszCatName;
			if (!hparmCat)
			{
				GfLogError("Could not read track category file %s\n", ossFileName.str().c_str());
				pszCatName = (*itCatId).c_str();
			}
			else
				pszCatName =
					GfParmGetStr(hparmCat, TRK_SECT_HDR, TRK_ATT_NAME, (*itCatId).c_str());
			_pPrivate->vecCatNames.push_back(pszCatName);

			GfParmReleaseHandle(hparmCat);
		}

		// Update tracks, as they don't have their category name set yet.
		for (unsigned nCatInd = 0; nCatInd < _pPrivate->vecCatIds.size(); nCatInd++)
		{
			std::vector<GfTrack*> vecTracksInCat =
				getTracksInCategory(_pPrivate->vecCatIds[nCatInd]);
			std::vector<GfTrack*>::iterator itTrack;
			for (itTrack = vecTracksInCat.begin(); itTrack != vecTracksInCat.end(); itTrack++)
				(*itTrack)->setCategoryName(_pPrivate->vecCatNames[nCatInd]);
		}
	}
	
	return _pPrivate->vecCatNames;
}

GfTrack* GfTracks::getTrack(const std::string& strId) const
{
	std::map<std::string, GfTrack*>::iterator itTrack =
		_pPrivate->mapTracksById.find(strId);
	if (itTrack != _pPrivate->mapTracksById.end())
		return itTrack->second;
	
	return 0;
}

GfTrack* GfTracks::getTrackWithName(const std::string& strName) const
{
	std::vector<GfTrack*>::iterator itTrack;
	for (itTrack = _pPrivate->vecTracks.begin(); itTrack != _pPrivate->vecTracks.end(); itTrack++)
		if ((*itTrack)->getName() == strName)
			return *itTrack;

	return 0;
}

std::vector<GfTrack*> GfTracks::getTracksInCategory(const std::string& strCatId) const
{
	std::vector<GfTrack*> vecTracksInCat;

	std::vector<GfTrack*>::iterator itTrack;
	for (itTrack = _pPrivate->vecTracks.begin(); itTrack != _pPrivate->vecTracks.end(); itTrack++)
		if (strCatId.empty() || (*itTrack)->getCategoryId() == strCatId)
			vecTracksInCat.push_back(*itTrack);

	return vecTracksInCat;
}

std::vector<std::string> GfTracks::getTrackIdsInCategory(const std::string& strCatId) const
{
	std::vector<std::string> vecTrackIds;

	std::vector<GfTrack*>::const_iterator itTrack;
	for (itTrack = _pPrivate->vecTracks.begin(); itTrack != _pPrivate->vecTracks.end(); itTrack++)
		if (strCatId.empty() || (*itTrack)->getCategoryId() == strCatId)
			vecTrackIds.push_back((*itTrack)->getId());

	return vecTrackIds;
}

std::vector<std::string> GfTracks::getTrackNamesInCategory(const std::string& strCatId) const
{
	std::vector<std::string> vecTrackNames;

	std::vector<GfTrack*>::const_iterator itTrack;
	for (itTrack = _pPrivate->vecTracks.begin(); itTrack != _pPrivate->vecTracks.end(); itTrack++)
		if (strCatId.empty() || (*itTrack)->getCategoryId() == strCatId)
			vecTrackNames.push_back((*itTrack)->getName());

	return vecTrackNames;
}

/** 
 * GfTracks::getFirstUsableTrack
 * 
 * Retrieve the first usable track in the given category, searching in the given direction
 * and skipping the first found if specified
 * 
 * @param   strCatId       Id of the category to search inside of.
 * @param   strFromTrackId Id of the track from which to start the search.
 * @param   nSearchDir     <0 = previous, >0 = next.
 * @param   bSkipFrom      If true, skip the first found track.
 */
GfTrack* GfTracks::getFirstUsableTrack(const std::string& strCatId,
									   const std::string& strFromTrackId,
									   int nSearchDir, bool bSkipFrom) const
{
	// Check and fix nSearchDir.
	nSearchDir = nSearchDir > 0 ? +1 : -1;
	
	// Check category.
	if (!strCatId.empty()
		&& std::find(_pPrivate->vecCatIds.begin(), _pPrivate->vecCatIds.end(), strCatId)
		   == _pPrivate->vecCatIds.end())
	{
		GfLogError("GfTracks::getFirstUsableTrack(1) : No such category %s\n", strCatId.c_str());
		return 0;
	}

	// Retrieve tracks in this category.
	const std::vector<GfTrack*> vecTracksInCat = getTracksInCategory(strCatId);
	if (vecTracksInCat.size() == 0)
	{
		// Should never happen, empty categories are not even created ...
		GfLogError("GfTracks::getFirstUsableTrack : Empty category %s\n", strCatId.c_str());
		return 0;
	}
	
	// Retrieve the index of the specified track to start from, if any.
	int nCurTrackInd = 0;
	if (!strFromTrackId.empty())
	{
		std::vector<GfTrack*>::const_iterator itTrack = vecTracksInCat.begin();
		while (itTrack != vecTracksInCat.end())
		{
			if ((*itTrack)->getId() == strFromTrackId)
			{
				nCurTrackInd = itTrack - vecTracksInCat.begin();
				break;
			}
			itTrack++;
		}
	}
	
	int nTrackInd = nCurTrackInd;
	if (bSkipFrom || !vecTracksInCat[nTrackInd]->isUsable())
	{
		const int nPrevTrackInd = nCurTrackInd;
		do
		{
			nTrackInd =
				(nTrackInd + nSearchDir + vecTracksInCat.size()) % vecTracksInCat.size();
		}
		while (nTrackInd != nPrevTrackInd && !vecTracksInCat[nTrackInd]->isUsable());
	}

	GfTrack* pTrack = 0;
	if (vecTracksInCat[nTrackInd]->isUsable())
		pTrack = vecTracksInCat[nTrackInd];

	return pTrack;
}
				  
/** 
 * GfTracks::getFirstUsableTrack
 * 
 * Retrieve the first usable track among all categories, searching in the given direction
 * from the given category, but skipping it if specified
 * 
 * @param   strFromCatId   Id of the category to search inside of.
 * @param   nSearchDir     <0 = previous, >0 = next.
 * @param   bSkipFrom      If true, skip the first found track.
 */
GfTrack* GfTracks::getFirstUsableTrack(const std::string& strFromCatId,
									   int nSearchDir, bool bSkipFrom) const
{
	// Check and fix nSearchDir.
	nSearchDir = nSearchDir > 0 ? +1 : -1;
	
	// Retrieve and check category.
	std::vector<std::string>::const_iterator itFromCat =
		std::find(_pPrivate->vecCatIds.begin(), _pPrivate->vecCatIds.end(), strFromCatId);
	if (itFromCat == _pPrivate->vecCatIds.end())
	{
		if (!bSkipFrom) // No way if no such category and mustn't skip it.
		{
			GfLogError("GfTracks::getFirstUsableTrack(2) : No such category %s\n",
					   strFromCatId.c_str());
			return 0;
		}
		else // Otherwise, let's start by the first available category.
			itFromCat = _pPrivate->vecCatIds.begin();
	}

	int nCatInd = itFromCat - _pPrivate->vecCatIds.begin();

	GfTrack* pTrack = 0;

	if (bSkipFrom || !(pTrack = getFirstUsableTrack(_pPrivate->vecCatIds[nCatInd])))
	{
		const int nPrevCatInd = nCatInd;
		do
		{
			nCatInd =
				(nCatInd + nSearchDir + _pPrivate->vecCatIds.size()) % _pPrivate->vecCatIds.size();
			pTrack = getFirstUsableTrack(_pPrivate->vecCatIds[nCatInd]);
		}
		while (nCatInd != nPrevCatInd && !pTrack);
	}

	return pTrack;
}
	
void GfTracks::print(bool bVerbose) const
{
	GfLogTrace("Track base : %d categories, %d tracks\n",
			   _pPrivate->vecCatIds.size(), _pPrivate->vecTracks.size());
	std::vector<std::string>::const_iterator itCatId;
	for (itCatId = _pPrivate->vecCatIds.begin(); itCatId != _pPrivate->vecCatIds.end(); itCatId++)
	{
		GfLogTrace("  '%s' category :\n", itCatId->c_str());
		const std::vector<GfTrack*> vecTracksInCat = getTracksInCategory(*itCatId);
		std::vector<GfTrack*>::const_iterator itTrack;
		for (itTrack = vecTracksInCat.begin(); itTrack != vecTracksInCat.end(); itTrack++)
			if (bVerbose)
				GfLogTrace("    %-22s : %s\n", (*itTrack)->getName().c_str(),
						   (*itTrack)->getDescriptorFile().c_str());
			else
				GfLogTrace("    %-16s : %s\n", (*itTrack)->getId().c_str(),
						   (*itTrack)->getDescriptorFile().c_str());
	}
}

// GfTrack class ----------------------------------------------------------

GfTrack::GfTrack() : _fLength(-1), _fWidth(-1), _nMaxPitSlots(-1), _bUsable(false)
{
}

const std::string& GfTrack::getId() const
{
	return _strId;
}

const std::string& GfTrack::getName() const
{
	// Lazy load scheme : read files only when really needed, and only once.
	if (_strName.empty())
		load();
	return _strName;
}

const std::string& GfTrack::getDescription() const
{
	// Lazy load scheme : read files only when really needed, and only once.
	if (_strDesc.empty())
		load();
	return _strDesc;
}

const std::string& GfTrack::getAuthors() const
{
	// Lazy load scheme : read files only when really needed, and only once.
	if (_strAuthors.empty())
		load();
	return _strAuthors;
}

const std::string& GfTrack::getCategoryId() const
{
	return _strCatId;
}

const std::string& GfTrack::getCategoryName() const
{
	// Lazy load scheme : read files only when really needed, and only once.
	if (_strCatName.empty())
		// Getting the names of all the categories updates all the tracks.
		GfTracks::self()->getCategoryNames();

	// Not, it is no more empty.
	return _strCatName;
}

const std::string& GfTrack::getDescriptorFile() const
{
	return _strDescFile;
}

const std::string& GfTrack::getOutlineFile() const
{
	return _strOutlineFile;
}

const std::string& GfTrack::getPreviewFile() const
{
	return _strPreviewFile;
}


float GfTrack::getLength() const
{
	// Lazy load scheme : read files only when really needed, and only once.
	if (_fLength < 0)
		load();
	return _fLength;
}

float GfTrack::getWidth() const
{
	// Lazy load scheme : read files only when really needed, and only once.
	if (_fWidth < 0)
		load();
	return _fWidth;
}

int GfTrack::getMaxNumOfPitSlots() const
{
	// Lazy load scheme : read files only when really needed, and only once.
	if (_nMaxPitSlots < 0)
		load();
	return _nMaxPitSlots;
}

bool GfTrack::isUsable() const
{
	// Must load to know if really usable.
	if (!_bUsable && _strName.empty())
		load();
	return _bUsable;
}

void GfTrack::setId(const std::string& strId)
{
	_strId = strId;
}

void GfTrack::setName(const std::string& strName)
{
	_strName = strName;
}

void GfTrack::setCategoryId(const std::string& strCatId)
{
	_strCatId = strCatId;
}

void GfTrack::setCategoryName(const std::string& strCatName)
{
	_strCatName = strCatName;
}

void GfTrack::setDescriptorFile(const std::string& strDescFile)
{
	_strDescFile = strDescFile;
}

void GfTrack::setOutlineFile(const std::string& strOutlineFile)
{
	_strOutlineFile = strOutlineFile;
}

void GfTrack::setPreviewFile(const std::string& strPreviewFile)
{
	_strPreviewFile = strPreviewFile;
}

void GfTrack::setLength(float fLength)
{
	_fLength = fLength;
}

void GfTrack::setWidth(float fWidth)
{
	_fWidth = fWidth;
}

void GfTrack::setMaxNumOfPitSlots(int nPitSlots)
{
	_nMaxPitSlots = nPitSlots;
}

bool GfTrack::load() const
{
	// Check if the track loader is ready.
	ITrackLoader* piTrackLoader = GfTracks::self()->getTrackLoader();
    if (!piTrackLoader)
	{
		GfLogError("Track loader not yet initialized ; failed to load any track\n");
        return false;
    }
	
    // Load track data from the XML file.
    tTrack* pTrack = piTrackLoader->load(_strDescFile.c_str());
    if (!pTrack)
	{
		GfLogWarning("Unusable track %s : failed to build track data from %s\n",
					 _strId.c_str(), _strDescFile.c_str());
        return false;
    }

	// Check if the track 3D model file exists.
	std::ostringstream ossFileName;
	ossFileName << "tracks/" << _strCatId << '/' << _strId << '/'
				<< (pTrack->graphic.model3d ? pTrack->graphic.model3d : "track.ac");
    if (!GfFileExists(ossFileName.str().c_str()))
	{
		GfLogWarning("Unusable track %s : could not find 3D model %s\n",
					 _strId.c_str(), ossFileName.str().c_str());
        return false;
    }

	// All right now : let's read last needed infos.
	_strName = pTrack->name;
	_strDesc = pTrack->descr;
	_strAuthors = pTrack->authors;
	_fLength = pTrack->length;
	_fWidth = pTrack->width;
	_nMaxPitSlots = pTrack->pits.nMaxPits;

    // Unload track data.
    piTrackLoader->unload();

	// Now, the track seems usable (hm ... OK, we didn't check the 3D file contents ...).
	_bUsable = true;
	
	return true;
}
