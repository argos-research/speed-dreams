/***************************************************************************

    file                 : racemanagers.h
    created              : December 2010
    copyright            : (C) 2010 Jean-Philippe Meuret
    web                  : speed-dreams.sourceforge.net
    version              : $Id: racemanagers.h 4902 2012-08-27 10:04:20Z kmetykog $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef __TGFRACEMANAGERS__H__
#define __TGFRACEMANAGERS__H__

#include <string>
#include <vector>

#include "tgfdata.h"

class GfTrack;


/** @file   
    		Singleton holding information on the available race managers
    @defgroup	tgfdata	Data manager for the client gaming framework.
*/

class TGFDATA_API GfRaceManager
{
public:
	
	GfRaceManager(const std::string& strId, void* hparmHandle);
	void reset(void* hparmHandle, bool bClosePrevHdle = false);

	~GfRaceManager();

	const std::string& getId() const;
	void* getDescriptorHandle() const;
	std::string getDescriptorFileName() const;

	const std::string& getName() const;
	const std::string& getType() const;
	const std::string& getSubType() const;
	int getPriority() const;

	bool isNetwork() const;
	bool acceptsDriverType(const std::string& strType) const;
	const std::vector<std::string>& getAcceptedDriverTypes() const;
	bool acceptsCarCategory(const std::string& strCatId) const;
	const std::vector<std::string>& getAcceptedCarCategoryIds() const;
	
	unsigned getEventCount() const;
	bool isMultiEvent() const;
	GfTrack* getEventTrack(unsigned nEventIndex); // index in [0, nEvents[
	void setEventTrack(unsigned nEventIndx, GfTrack* pTrack); // index in [0, nEvents[
	GfTrack* getPreviousEventTrack(unsigned nEventIndex); // index in [0, nEvents[

	const std::vector<std::string>& getSessionNames() const;
	unsigned getSessionCount() const;
	const std::string& getSessionName(unsigned nIndex) const;
	
	const std::string& getSavedConfigsDir() const;
	bool hasSavedConfigsFiles() const;
	const std::string& getResultsDir() const;
	bool hasResultsFiles() const;
	
	bool hasSubFiles() const;

	//! Save data to params (in-memory, no file written to disk).
	void store();
	
	//! Is the race manager data consistent with the params from which it was loaded ?
	bool isDirty() const;
	
protected:

	//! Load remaining info from params (called by accessors, lazy mode).
	void load() const;

 protected:
	
	std::string _strId; // XML file name (ex: quickrace, singleevent-endurance, championship-sc)
	void*       _hparmHandle; // Params handle to the descriptor file.

	std::string _strName; // User friendly full name (ex: Quick Race, Supercar Championship).
	std::string _strType; // User friendly type name (ex: Quick Race, Single Event, Championship).
	std::string _strSubType; // User friendly sub-type name (ex: "", Endurance, Challenge, Supercars").
	int         _nPriority; // Gives the order of the buttons in the race select menu

	std::vector<std::string> _vecAcceptedDriverTypes;
	std::vector<std::string> _vecAcceptedCarCategoryIds;

	mutable bool _bHasSubFiles; // True if multiple configuration files are used (ex: Career mode).
	
	// Saved configs and results files dirs.
	mutable std::string _strSavedConfigsDir;
	mutable std::string _strResultsDir;
	
	mutable std::vector<std::string> _vecEventTrackIds; // Id of the track for each scheduled event.
	mutable std::vector<std::string> _vecSessionNames; // Name and order of sessions for each event.

	mutable bool _bIsDirty; // True if no change occurred since last reset().
};

class TGFDATA_API GfRaceManagers
{
public:

	// Accessor to the unique instance of the singleton.
	static GfRaceManagers* self();
	static void shutdown();
	
	const std::vector<std::string>& getTypes() const;

	GfRaceManager* getRaceManager(const std::string& strId) const;
	GfRaceManager* getRaceManagerWithName(const std::string& strName) const;

	std::vector<GfRaceManager*> getRaceManagersWithType(const std::string& strType = "") const;
	
	void print(bool bVerbose = false) const;

protected:

	// Protected constructor and destructor : clients can not use them.
	GfRaceManagers();
	~GfRaceManagers();
	
protected:

	// The singleton itself.
	static GfRaceManagers* _pSelf;

	// Its private data.
	class Private;
	Private* _pPrivate;
};

#endif /* __TGFRACEMANAGERS__H__ */
