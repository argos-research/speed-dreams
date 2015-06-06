/***************************************************************************

    file                 : race.h
    created              : Sun Nov 21 19:00:00 CET 2010
    copyright            : (C) 2010 by Jean-Philippe MEURET
    web                  : speed-dreams.sourceforge.net
    version              : $Id: race.h 5696 2013-10-01 22:15:36Z torcs-ng $

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
		Singleton holding information on a race (mainly the starting grid)
    @defgroup	tgfdata	Data manager for the client gaming framework.
*/


#ifndef __TGFRACE_H__
#define __TGFRACE_H__

#include <string>
#include <vector>

#include "tgfdata.h"


class GfRaceManager;
class GfDriver;
class GfTrack;


class TGFDATA_API GfRace
{
public:

	//! Constructor.
	GfRace();

	//! Destructor.
	~GfRace();

	//! Load from the given race manager params and results file if specified.
	void load(GfRaceManager* pRaceMan, bool bKeepHumans = true, void* hparmResults = 0);

	//! Clear the race.
	void clear();
	
	//! Store to the race manager params file.
	void store();

	//! Is the race data consistent with the params from which it was loaded ?
	bool isDirty() const;

	//! Force dirtyness.
	void setDirty(bool bIsDirty = true);
	
	GfRaceManager* getManager() const;

	enum ETimeOfDaySpec { eTimeDawn, eTimeMorning, eTimeNoon, eTimeAfternoon,
						  eTimeDusk, eTimeNight, eTimeNow, eTimeReal, eTimeFromTrack, eTimeRandom,
						  eTime24hr,
						  nTimeSpecNumber }; // Last = invalid value = nb of valid ones.
	enum ECloudsSpec { eCloudsNone, eCloudsFew, eCloudsScarce, eCloudsMany, eCloudsFull,
					   eCloudsRandom, nCloudsSpecNumber}; // Last = invalid value = nb of valid ones.
	enum ERainSpec { eRainNone, eRainLittle, eRainMedium, eRainHeavy, eRainRandom,
					 nRainSpecNumber }; // Last = invalid value = nb of valid ones.
	class Parameters
	{
	  public:
		unsigned bfOptions; // Bit field of configurable parameters (RM_CONF_* values).
		int nLaps;
		int nDistance; // km
		int nDuration; // s
		unsigned bfDisplayMode;
		ETimeOfDaySpec eTimeOfDaySpec;
		ECloudsSpec eCloudsSpec;
		ERainSpec eRainSpec;
	};
	
	Parameters* getParameters(const std::string& strSessionName) const;

	int getSupportedFeatures() const;

	bool acceptsDriverType(const std::string& strType) const;
	const std::vector<std::string>& getAcceptedDriverTypes() const;
	bool acceptsCarCategory(const std::string& strCatId) const;
	const std::vector<std::string>& getAcceptedCarCategoryIds() const;

	//! For all sessions to "results-only" mode (SimuSimu mode unchanged).
	void forceResultsOnly();

	unsigned getCompetitorsCount() const;
	const std::vector<GfDriver*>& getCompetitors() const;
	bool hasHumanCompetitors() const;
	bool acceptsMoreCompetitors() const;
	bool appendCompetitor(GfDriver* pComp);
	bool removeCompetitor(GfDriver* pComp);
	bool moveCompetitor(GfDriver* pComp, int nDeltaPlace);
	bool removeAllCompetitors();
	bool shuffleCompetitors();

 	GfDriver* getCompetitor(const std::string& strModName, int nItfIndex) const;

	bool isCompetitorFocused(const GfDriver* pComp) const;
	GfDriver* getFocusedCompetitor() const;
	void setFocusedCompetitor(const GfDriver* pComp);

	GfTrack* getTrack() const;

	const std::string& getSessionName() const;

	void* getResultsDescriptorHandle() const;
	
 	void print() const;

protected:
	
	// Its private data.
	class Private;
	Private* _pPrivate;
};

#endif /* __TGFRACE_H__ */

