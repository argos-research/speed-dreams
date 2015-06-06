/***************************************************************************

    file                 : tracks.h
    created              : Sun Nov 21 19:00:00 CET 2010
    copyright            : (C) 2010 by Jean-Philippe MEURET
    web                  : speed-dreams.sourceforge.net
    version              : $Id: tracks.h 4902 2012-08-27 10:04:20Z kmetykog $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
 
/**
    @file   
		Singleton holding information on the available tracks and categories.
    @defgroup	tgfclientdata	Data manager for the client gaming framework.
*/

#ifndef __TGFTRACKS_H__
#define __TGFTRACKS_H__

#include <string>
#include <vector>

#include <itrackloader.h>

#include "tgfdata.h"


// Information on one track.
class TGFDATA_API GfTrack
{
public:

	GfTrack();
	
	const std::string& getId() const;
	const std::string& getName() const;
	const std::string& getDescription() const;
	const std::string& getAuthors() const;
	const std::string& getCategoryId() const;
	const std::string& getCategoryName() const;
	const std::string& getDescriptorFile() const;
	const std::string& getOutlineFile() const;
	const std::string& getPreviewFile() const;
	float getLength() const;
	float getWidth() const;
	int getMaxNumOfPitSlots() const;

	bool isUsable() const;
	
	void setId(const std::string& strId);
	void setName(const std::string& strName);
	void setDescription(const std::string& strDesc);
	void setAuthors(const std::string& strName);
	void setCategoryId(const std::string& strCatId);
	void setCategoryName(const std::string& strCatName);
	void setDescriptorFile(const std::string& strDescFile);
	void setOutlineFile(const std::string& strOutlineFile);
	void setPreviewFile(const std::string& strPreviewFile);
	void setLength(float fLength);
	void setWidth(float fWidth);
	void setMaxNumOfPitSlots(int nPitSlots);

protected:

	bool load() const;

protected:

	std::string _strId;              // XML file / folder name (ex: "goldstone-sand")
 	mutable std::string _strName;    // User friendly name (ex: "Goldstone Sand").
	std::string _strCatId;           // Category XML file / folder name (ex: "circuit").
	mutable std::string _strCatName; // Category user friendly name (ex: "Circuit").
	mutable std::string _strAuthors; // Name of the authors
	std::string _strDescFile;        // Path-name of the XML descriptor file.
	std::string _strOutlineFile;     // Path-name of the outline image file.
	std::string _strPreviewFile;     // Path-name of the preview image file.
 	mutable std::string _strDesc;    // Description.
	mutable float _fLength;          // Length (m).
	mutable float _fWidth;           // Width (m).
	mutable int _nMaxPitSlots;       // Max. number of pit slots (m).

	mutable bool _bUsable;           // False if anything wrong.
};


// Information on all the available tracks (singleton pattern).
class TGFDATA_API GfTracks
{
public:

	// Accessor to the unique instance of the singleton.
	static GfTracks* self();
	static void shutdown();

	ITrackLoader* getTrackLoader() const;
	void setTrackLoader(ITrackLoader* piTrackLoader);
	
 	const std::vector<std::string>& getCategoryIds() const;
 	const std::vector<std::string>& getCategoryNames() const;

 	GfTrack* getTrack(const std::string& strId) const;
 	GfTrack* getTrackWithName(const std::string& strName) const;

 	std::vector<GfTrack*> getTracksInCategory(const std::string& strCatId = "") const;
 	std::vector<std::string> getTrackIdsInCategory(const std::string& strCatId = "") const;
 	std::vector<std::string> getTrackNamesInCategory(const std::string& strCatId = "") const;
	
	GfTrack* getFirstUsableTrack(const std::string& strCatId = "",
								 const std::string& strFromTrackId = "",
								 int nSearchDir = +1, bool bSkipFrom = false) const;
	GfTrack* getFirstUsableTrack(const std::string& strFromCatId,
								 int nSearchDir, bool bSkipFrom = false) const;
	
 	void print(bool bVerbose = false) const;

protected:

	// Protected constructor and destructor : clients can not use them.
	GfTracks();
	~GfTracks();
	
protected:

	// The singleton itself.
	static GfTracks* _pSelf;

	// Its private data.
	class Private;
	mutable Private* _pPrivate;
};

#endif /* __TGFTRACKS_H__ */

