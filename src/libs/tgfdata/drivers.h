/***************************************************************************

    file                 : drivers.h
    created              : Sun Nov 21 19:00:00 CET 2010
    copyright            : (C) 2010 by Jean-Philippe MEURET
    web                  : speed-dreams.sourceforge.net
    version              : $Id: drivers.h 3893 2011-09-18 15:52:42Z pouillot $

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
		Singleton holding information on the available drivers
    @defgroup	tgfdata	Data manager for the client gaming framework.
*/

#ifndef __TGFDRIVERS_H__
#define __TGFDRIVERS_H__

#include <string>
#include <vector>

#include "tgfdata.h"

class GfCar;


class TGFDATA_API GfDriverSkin
{
public:

	GfDriverSkin(const std::string& strName = "");
	
public:

	int getTargets() const;
	const std::string& getName() const;
	const std::string& getCarPreviewFileName() const;

	void setTargets(int bfTargets); // Overwrite any previous target.
	void addTargets(int nTargets); // Bit-or.
	void setName(const std::string& strName);
	void setCarPreviewFileName(const std::string& strFileName);

protected:

	int          _bfTargets; // Skin targets bit-field (see car.h for possible values)
	std::string  _strName;  // Skin name (empty if standard skin)
	std::string  _strCarPreviewFileName;  // Car preview for this skin name (empty if none)
};

class TGFDATA_API GfDriver
{
public:

	GfDriver(const std::string& strModuleName, int nItfIndex,
			 const std::string& strName, void* hparmRobot);

	void load(void* hparmRobot);
	
	const std::string& getName() const;
	const std::string& getModuleName() const;
	int getInterfaceIndex() const;
	bool isHuman() const;
	bool isNetwork() const;
	const GfCar* getCar() const;
	const std::string& getType() const;
	const GfDriverSkin& getSkin() const;

	bool matchesTypeAndCategory(const std::string& strType = "",
								const std::string& strCarCatId = "") const;

	int getSupportedFeatures() const;
	
	double getSkillLevel() const;

	void setCar(const GfCar* pCar);
	void setSkin(const GfDriverSkin& skin);

	//! Compute a driver type from a driver module name.
	static std::string getType(const std::string& strModName);

	//! Get the possible skins using the same search path as the graphics engine.
	std::vector<GfDriverSkin> getPossibleSkins(const std::string& strAltCarId = "") const;

	//! Retrieve the skin with given name is the given list
	static std::vector<GfDriverSkin>::iterator findSkin(std::vector<GfDriverSkin>& vecSkins,
														const std::string& strName);

protected:

	//! Get the possible skins in a given folder.
	void getPossibleSkinsInFolder(const std::string& strCarId,
								  const std::string& strFolderPath,
								  std::vector<GfDriverSkin>& vecPossSkins) const;
	
protected:

	std::string  _strName;      // User friendly name (ex: "Yuuki Kyousou").
	std::string  _strModName;   // Module shared library / folder name (ex: "simplix_sc")
	int          _nItfIndex;    // Index of associated interface in the module
    bool         _bIsHuman;	    // true for humans
	const GfCar* _pCar;         // Car
	GfDriverSkin _skin;         // Skin

	mutable std::string _strType;     // Type ~ module type (ex: "simplix", "usr")

	double _fSkillLevel; // From 0 (pro) to 10 (rookie).
	int _nFeatures; // Bit mask built with RM_FEATURE_*
};

class TGFDATA_API GfDrivers
{
public:

	// Accessor to the unique instance of the singleton.
	static GfDrivers* self();
	static void shutdown();

	// Reload drivers data from files.
	void reload();

	unsigned getCount() const;
 	const std::vector<std::string>& getTypes() const;

 	GfDriver* getDriver(const std::string& strModName, int nItfIndex) const;
	GfDriver* getDriverWithName(const std::string& strName) const;
	
 	std::vector<GfDriver*> getDriversWithTypeAndCategory(const std::string& strType = "",
														 const std::string& strCarCatId = "") const;

 	void print() const;

protected:

	// Protected constructor and destructor : clients can not use them.
	GfDrivers();
	~GfDrivers();

	// Clear all data.
	void clear();
	
protected:

	// The singleton itself.
	static GfDrivers* _pSelf;

	// Its private data.
	class Private;
	Private* _pPrivate;
};

#endif /* __TGFDRIVERS_H__ */

