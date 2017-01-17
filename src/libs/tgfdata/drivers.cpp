/***************************************************************************

    file                 : drivers.cpp
    created              : Sun Nov 21 19:00:00 CET 2010
    copyright            : (C) 2010 by Jean-Philippe MEURET
    web                  : speed-dreams.sourceforge.net
    version              : $Id: drivers.cpp 6143 2015-09-24 16:49:32Z torcs-ng $
                      
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <cctype>
#include <map>
#include <sstream>
#include <algorithm>

#include <portability.h>

#include <robot.h>

#include "cars.h"
#include "drivers.h"


// Private data for GfDrivers				  
class GfDrivers::Private
{
public:
	
	// One GfDriver structure for each driver (order = sorted directory one).
	std::vector<GfDriver*> vecDrivers;

	// Map for quick access to GfDriver by { module name, interface index }
	typedef std::map<std::pair<std::string, int>, GfDriver*> TMapDriversByKey;
	TMapDriversByKey mapDriversByKey;

	// Vector of driver types.
	std::vector<std::string> vecTypes;
	
	// Vector of driver car categories.
	std::vector<std::string> vecCarCategoryIds;
};


GfDrivers* GfDrivers::_pSelf = 0;

GfDrivers *GfDrivers::self()
{
	if (!_pSelf)
		_pSelf = new GfDrivers;
	
	return _pSelf;
}

void GfDrivers::shutdown()
{
	delete _pSelf;
	_pSelf = 0;
}

GfDrivers::~GfDrivers()
{
	clear();
	
	delete _pPrivate;
	_pPrivate = 0;
}

void GfDrivers::clear()
{
	_pPrivate->mapDriversByKey.clear();
	_pPrivate->vecTypes.clear();
	_pPrivate->vecCarCategoryIds.clear();

	std::vector<GfDriver*>::const_iterator itDriver;
	for (itDriver = _pPrivate->vecDrivers.begin();
		 itDriver != _pPrivate->vecDrivers.end(); itDriver++)
		delete *itDriver;
	_pPrivate->vecDrivers.clear();
}

void GfDrivers::reload()
{
	// Clear all.
	clear();
	
	// (Re)Load robot modules from the "drivers" installed folder.
	std::string strDriversDirName(GfLibDir());
	strDriversDirName += "drivers";

    tModList* lstDriverModules = 0;
    const int nDriverModules = GfModInfoDir(CAR_IDENT, strDriversDirName.c_str(), 1, &lstDriverModules);
	if (nDriverModules <= 0 || !lstDriverModules)
	{
		GfLogFatal("Could not load any driver module from %s", strDriversDirName.c_str());
		return;
	}
	
	// For each module found, load drivers information.
    tModList *pCurModule = lstDriverModules;
	do
	{
		pCurModule = pCurModule->next;

		// Determine the module name.
		std::string strModName(pCurModule->sopath);
		strModName.erase(strlen(pCurModule->sopath) - strlen(DLLEXT) - 1); // Truncate file ext.
		const size_t nLastSlashInd = strModName.rfind('/');
		if (nLastSlashInd != std::string::npos)
			strModName = strModName.substr(nLastSlashInd+1); // Remove heading folder path.

		// Load the module XML descriptor file (try  user settings first, and then installed one)
		std::ostringstream ossRobotFileName;
		ossRobotFileName << GfLocalDir() << "drivers/" << strModName
						 << '/' << strModName << PARAMEXT;
		void *hparmRobot =
			GfParmReadFile(ossRobotFileName.str().c_str(), GFPARM_RMODE_STD | GFPARM_RMODE_REREAD);
		if (!hparmRobot)
		{
			ossRobotFileName.str("");
			ossRobotFileName << "drivers/" << strModName << '/' << strModName << PARAMEXT;
			hparmRobot =
				GfParmReadFile(ossRobotFileName.str().c_str(), GFPARM_RMODE_STD | GFPARM_RMODE_REREAD);
		}
		if (!hparmRobot)
		{
			// Do not waste log with drivers that do not exists or do not have to exist!
			if (strncmp("dandroid",strModName.c_str(),8) == 0)
				continue;
			else if (strncmp("usr",strModName.c_str(),3) == 0)
				continue;
			else if (strncmp("replay",strModName.c_str(),6) == 0)
				continue;

			GfLogError("No usable '%s' driver (%s.xml not found or not readable)\n",
					   strModName.c_str(), strModName.c_str());
			continue;
		}

		// For each driver (= interface) "in" the module
		for (int nItfInd = 0; nItfInd < pCurModule->modInfoSize; nItfInd++)
		{
			// Ignore undefined drivers or showing an empty name
			if (!pCurModule->modInfo[nItfInd].name || pCurModule->modInfo[nItfInd].name[0] == '\0')
			{
				GfLogInfo("Ignoring '%s' driver #%d (not defined or empty name)\n",
							 strModName.c_str(), nItfInd);
				continue;
			}

			// Create the driver and load info from the XML file.
			GfDriver* pDriver = new GfDriver(strModName, pCurModule->modInfo[nItfInd].index,
											 pCurModule->modInfo[nItfInd].name, hparmRobot);

			// For human drivers, if the car was not found, select the 1st possible one.
			if (!pDriver->getCar() && pDriver->isHuman())
			{
				GfCar* pSubstCar = GfCars::self()->getCarsInCategory()[0]; // Should never fail.
				if (pSubstCar)
				{
					std::ostringstream ossDrvSecPath;
					ossDrvSecPath << ROB_SECT_ROBOTS << '/' << ROB_LIST_INDEX << '/'
								  << pCurModule->modInfo[nItfInd].index;
					const char* pszCarId =
						GfParmGetStr(hparmRobot, ossDrvSecPath.str().c_str(), ROB_ATTR_CAR, "");
					GfLogWarning("Changing '%s' driver '%s' (#%d) 's car to %s (default one '%s' not available)\n",
								 strModName.c_str(), pCurModule->modInfo[nItfInd].name,
								 pCurModule->modInfo[nItfInd].index, pSubstCar->getId().c_str(),
								 pszCarId);
					pDriver->setCar(pSubstCar);
				}
			}
			
			// Keep the driver only if he drives an existing car.
			if (pDriver->getCar())
			{
				// Update the GfDrivers singleton.
				_pPrivate->vecDrivers.push_back(pDriver);
				const std::pair<std::string, int> driverKey(pDriver->getModuleName(),
															pDriver->getInterfaceIndex());
				_pPrivate->mapDriversByKey[driverKey] = pDriver;
				if (std::find(_pPrivate->vecTypes.begin(), _pPrivate->vecTypes.end(),
							  pDriver->getType()) == _pPrivate->vecTypes.end())
					_pPrivate->vecTypes.push_back(pDriver->getType());
				if (std::find(_pPrivate->vecCarCategoryIds.begin(), _pPrivate->vecCarCategoryIds.end(),
							  pDriver->getCar()->getCategoryId()) == _pPrivate->vecCarCategoryIds.end())
					_pPrivate->vecCarCategoryIds.push_back(pDriver->getCar()->getCategoryId());
			}
			else
			{
				delete pDriver;
				
				std::ostringstream ossDrvSecPath;
				ossDrvSecPath << ROB_SECT_ROBOTS << '/' << ROB_LIST_INDEX << '/'
							  << pCurModule->modInfo[nItfInd].index;
				const char* pszCarId =
					GfParmGetStr(hparmRobot, ossDrvSecPath.str().c_str(), ROB_ATTR_CAR, "");
				GfLogInfo("Ignoring '%s' driver '%s' (#%d) (not defined or default car '%s' not available)\n",
							 strModName.c_str(), pCurModule->modInfo[nItfInd].name,
							 pCurModule->modInfo[nItfInd].index, pszCarId);
			}
		}
		
		// Close driver module descriptor file if open
		GfParmReleaseHandle(hparmRobot);
		
	} while (pCurModule != lstDriverModules);

	// Free the module list.
    GfModFreeInfoList(&lstDriverModules);

	// Sort the car category ids and driver types vectors.
	std::sort(_pPrivate->vecCarCategoryIds.begin(), _pPrivate->vecCarCategoryIds.end());
	std::sort(_pPrivate->vecTypes.begin(), _pPrivate->vecTypes.end());

	// Trace what we got.
	print();
}

GfDrivers::GfDrivers()
{
	_pPrivate = new GfDrivers::Private;

	reload();
}

unsigned GfDrivers::getCount() const
{
	return _pPrivate->vecDrivers.size();
}

const std::vector<std::string>& GfDrivers::getTypes() const
{
	return _pPrivate->vecTypes;
}

GfDriver* GfDrivers::getDriver(const std::string& strModName, int nItfIndex) const
{
	const std::pair<std::string, int> driverKey(strModName, nItfIndex);
	Private::TMapDriversByKey::iterator itDriver =
		_pPrivate->mapDriversByKey.find(driverKey);
	if (itDriver != _pPrivate->mapDriversByKey.end())
		return itDriver->second;
	
	return 0;
}

std::vector<GfDriver*> GfDrivers::getDriversWithTypeAndCategory(const std::string& strType,
																const std::string& strCarCatId) const
{
	std::vector<GfDriver*> vecSelDrivers;
	std::vector<GfDriver*>::iterator itDriver;
	for (itDriver = _pPrivate->vecDrivers.begin();
		 itDriver != _pPrivate->vecDrivers.end(); itDriver++)
		if ((*itDriver)->matchesTypeAndCategory(strType, strCarCatId))
			vecSelDrivers.push_back(*itDriver);

	return vecSelDrivers;
}

void GfDrivers::print() const
{
	GfLogTrace("Driver base : %d types, %d car categories, %d drivers\n",
			   _pPrivate->vecTypes.size(), _pPrivate->vecCarCategoryIds.size(),
			   _pPrivate->vecDrivers.size());
	
	std::vector<std::string>::const_iterator itType;
	for (itType = _pPrivate->vecTypes.begin(); itType != _pPrivate->vecTypes.end(); itType++)
	{
		GfLogTrace("  '%s' type :\n", itType->c_str());
		std::vector<std::string>::const_iterator itCarCatId;
		for (itCarCatId = _pPrivate->vecCarCategoryIds.begin();
			 itCarCatId != _pPrivate->vecCarCategoryIds.end(); itCarCatId++)
		{
			const std::vector<GfDriver*> vecDrivers =
				getDriversWithTypeAndCategory(*itType, *itCarCatId);
			if (vecDrivers.empty())
				continue;
			GfLogTrace("      '%s' car category :\n", itCarCatId->c_str());
			std::vector<GfDriver*>::const_iterator itDriver;
			for (itDriver = vecDrivers.begin(); itDriver != vecDrivers.end(); itDriver++)
				GfLogTrace("          %-24s : %s, %02X-featured\n",
						   (*itDriver)->getName().c_str(),
						   (*itDriver)->getCar()->getName().c_str(),
						   (*itDriver)->getSupportedFeatures());
		}
	}
}

// GfDriverSkin class ---------------------------------------------------------------

GfDriverSkin::GfDriverSkin(const std::string& strName)
: _bfTargets(0), _strName(strName)
{
}

int GfDriverSkin::getTargets() const
{
	return _bfTargets;
}

const std::string& GfDriverSkin::getName() const
{
	return _strName;
}

const std::string& GfDriverSkin::getCarPreviewFileName() const
{
	return _strCarPreviewFileName;
}

void GfDriverSkin::setTargets(int bfTargets)
{
	_bfTargets = bfTargets;
}

void GfDriverSkin::addTargets(int bfTargets)
{
	_bfTargets |= bfTargets;
}

void GfDriverSkin::setName(const std::string& strName)
{
	_strName = strName;
}

void GfDriverSkin::setCarPreviewFileName(const std::string& strFileName)
{
	_strCarPreviewFileName = strFileName;
}

// GfDriver class -------------------------------------------------------------------

// Skill level related constants.
static const char *ASkillLevelStrings[] =
	{ ROB_VAL_ARCADE, ROB_VAL_SEMI_ROOKIE, ROB_VAL_ROOKIE, ROB_VAL_AMATEUR, ROB_VAL_SEMI_PRO, ROB_VAL_PRO };
static const int NbSkillLevels = sizeof(ASkillLevelStrings) / sizeof(ASkillLevelStrings[0]);
static const double ASkillLevelValues[NbSkillLevels] = { 30.0, 20.0, 10.0, 7.0, 3.0, 0.0 };

// Robot drivers features related constants.
struct RobotFeature
{
	const char *pszName;
	int nValue;
};

static RobotFeature RobotFeatures[] =
{
	{ ROB_VAL_FEATURE_PENALTIES, RM_FEATURE_PENALTIES },
	{ ROB_VAL_FEATURE_TIMEDSESSION, RM_FEATURE_TIMEDSESSION },
	{ ROB_VAL_FEATURE_WETTRACK, RM_FEATURE_WETTRACK },
	
	/* Career mode features not yet resurrected (robots need work to support them).
	   { ROB_VAL_FEATURE_SC, RM_FEATURE_SC | RM_FEATURE_YELLOW | RM_FEATURE_PENALTIES },
	   { ROB_VAL_FEATURE_YELLOW, RM_FEATURE_YELLOW | RM_FEATURE_PENALTIES },
	   { ROB_VAL_FEATURE_RED, RM_FEATURE_RED },
	   { ROB_VAL_FEATURE_BLUE, RM_FEATURE_BLUE },
	   { ROB_VAL_FEATURE_PITEXIT, RM_FEATURE_PITEXIT | RM_FEATURE_PENALTIES },
	   { ROB_VAL_FEATURE_TIMEDSESSION, RM_FEATURE_TIMEDSESSION },
	   { ROB_VAL_FEATURE_PENALTIES, RM_FEATURE_PENALTIES }
	*/
};
static const int NRobotFeatures = sizeof(RobotFeatures) / sizeof(RobotFeatures[0]);


GfDriver::GfDriver(const std::string& strModName, int nItfIndex,
				   const std::string& strName, void* hparmRobot)
: _strName(strName), _strModName(strModName), _nItfIndex(nItfIndex),
  _bIsHuman(false), _pCar(0), _fSkillLevel(-1.0), _nFeatures(0)
{
	load(hparmRobot);
}

void GfDriver::load(void* hparmRobot)
{
	std::ostringstream ossDrvSecPath;
	ossDrvSecPath << ROB_SECT_ROBOTS << '/' << ROB_LIST_INDEX << '/' << _nItfIndex;

	// Humanity.
	_bIsHuman =
		strcmp(GfParmGetStr(hparmRobot, ossDrvSecPath.str().c_str(), ROB_ATTR_TYPE, ROB_VAL_ROBOT),
			   ROB_VAL_ROBOT) != 0;

	// Skill level.
    const char* pszKillLevel =
		GfParmGetStr(hparmRobot, ossDrvSecPath.str().c_str(), ROB_ATTR_LEVEL, ROB_VAL_SEMI_PRO);
    for(int nLevelInd = 0; nLevelInd < NbSkillLevels; nLevelInd++)
	{
		if (!strcmp(ASkillLevelStrings[nLevelInd], pszKillLevel))
		{
			_fSkillLevel = ASkillLevelValues[nLevelInd];
			break;
		}
    }

	// Supported features.
	if (_bIsHuman)
	{
		_nFeatures = RM_FEATURE_TIMEDSESSION | RM_FEATURE_WETTRACK;
		if (_fSkillLevel <= ASkillLevelValues[3]) // Pro (TODO: Create enum for that !)
			_nFeatures |= RM_FEATURE_PENALTIES;
	}
	else
	{
		_nFeatures = 0;
		char* pszDrvFeatures =
			strdup(GfParmGetStr(hparmRobot, ossDrvSecPath.str().c_str(), ROB_ATTR_FEATURES, ""));
		for (char* pszFeature = strtok(pszDrvFeatures, ";");
			 pszFeature != 0; pszFeature = strtok(NULL, ";"))
		{
			for (int nFeatInd = 0; nFeatInd < NRobotFeatures; nFeatInd++)
				if (!strcmp(pszFeature, RobotFeatures[nFeatInd].pszName))
				{
					_nFeatures |= RobotFeatures[nFeatInd].nValue;
					break;
				}
		}
		free(pszDrvFeatures);
	}

	// Driven car.
	const char* pszCarId = GfParmGetStr(hparmRobot, ossDrvSecPath.str().c_str(), ROB_ATTR_CAR, "");
	_pCar = GfCars::self()->getCar(pszCarId);
}

const std::string& GfDriver::getName() const
{
	return _strName;
}

const std::string& GfDriver::getModuleName() const
{
	return _strModName;
}

int GfDriver::getInterfaceIndex() const
{
	return _nItfIndex;
}

bool GfDriver::isHuman() const
{
	return _bIsHuman;
}

bool GfDriver::isNetwork() const
{
	return _strModName == "networkhuman";
}

const GfCar* GfDriver::getCar() const
{
	return _pCar;
}

// GfCar* GfDriver::getCar()
// {
// 	return _pCar;
// }

const GfDriverSkin& GfDriver::getSkin() const
{
	return _skin;
}

std::string GfDriver::getType(const std::string& strModName)
{
	std::string strType;
	
	// Parse module name for last '_' char : assumed to be the separator between type
	// and instance name for ubiquitous robots (ex: simplix)
	const size_t nTruncPos = strModName.rfind('_');
	if (nTruncPos == std::string::npos)
		strType = strModName; // Copy.
	else
		strType = strModName.substr(0, nTruncPos); // Copy + truncate.

	return strType;
}

const std::string& GfDriver::getType() const
{
	if (_strType.empty())
		_strType = getType(_strModName);
	
	return _strType;
	
}

bool GfDriver::matchesTypeAndCategory(const std::string& strType,
									  const std::string& strCarCatId) const
{
	return (strType.empty() || getType() == strType)
		   && (strCarCatId.empty() || getCar()->getCategoryId() == strCarCatId);
}

double GfDriver::getSkillLevel() const
{
	return _fSkillLevel;
}

int GfDriver::getSupportedFeatures() const
{
	return _nFeatures;
}

void GfDriver::setCar(const GfCar* pCar)
{
	_pCar = pCar;
}

void GfDriver::setSkin(const GfDriverSkin& skin)
{
	_skin = skin;
}

static const char* pszLiveryTexExt = ".png";
static const char* pszPreviewTexSufx = "-preview.jpg";
static const char* pszInteriorTexExt = ".png";
static const char* pszInteriorTexSufx = "-int";
static const char* pszDriverTexName = "driver"; // Warning: Must be consistent with <car>.ac/acc
static const char* pszDriverTexExt = ".png";
static const char* pszLogoTexName = "logo"; // Warning: Must be consistent with grscene.cpp
static const char* pszLogoTexExt = ".png";
static const char* pszWheel3DTexName = "wheel3d"; // Warning: Must be consistent with wheel<i>.ac/acc
static const char* pszWheel3DTexExt = ".png";

static const char* apszExcludedSkinNamePrefixes[] = { "rpm", "speed", "int" };
static const int nExcludedSkinNamePrefixes = sizeof(apszExcludedSkinNamePrefixes) / sizeof(char*);


std::vector<GfDriverSkin>::iterator GfDriver::findSkin(std::vector<GfDriverSkin>& vecSkins,
													   const std::string& strName)
{
	std::vector<GfDriverSkin>::iterator itSkin;
	for (itSkin = vecSkins.begin(); itSkin != vecSkins.end(); itSkin++)
	{
		if (itSkin->getName() == strName)
			return itSkin;
	}

	return vecSkins.end();
}


void GfDriver::getPossibleSkinsInFolder(const std::string& strCarId,
										const std::string& strFolderPath,
										std::vector<GfDriverSkin>& vecPossSkins) const
{
	//GfLogDebug("  getPossibleSkinsInFolder(%s, %s) ...\n",
	//		   strCarId.c_str(), strFolderPath.c_str());

	// Search for skinned livery files, and associated preview files if any.
	tFList *pLiveryFileList =
		GfDirGetListFiltered(strFolderPath.c_str(), strCarId.c_str(), pszLiveryTexExt);
	if (pLiveryFileList)
	{
		tFList *pCurLiveryFile = pLiveryFileList;
		do
		{
			pCurLiveryFile = pCurLiveryFile->next;

			// Extract the skin name from the livery file name.
			const int nSkinNameLen = // Expecting "<car name>-<skin name>.png"
				strlen(pCurLiveryFile->name) - strCarId.length() - 1 - strlen(pszLiveryTexExt);
			std::string strSkinName;
			if (nSkinNameLen > 0) // Otherwise, default/standard "<car name>.png"
			{
				strSkinName =
					std::string(pCurLiveryFile->name)
					.substr(strCarId.length() + 1, nSkinNameLen);
				
				// Ignore skins with an excluded prefix.
				int nExclPrfxInd = 0;
				for (; nExclPrfxInd < nExcludedSkinNamePrefixes; nExclPrfxInd++)
					if (strSkinName.find(apszExcludedSkinNamePrefixes[nExclPrfxInd]) == 0)
						break;
				if (nExclPrfxInd < nExcludedSkinNamePrefixes)
					continue;
			}
			
			// Ignore skins that are already in the list (path search priority).
			if (findSkin(vecPossSkins, strSkinName) == vecPossSkins.end())
			{
				// Create the new skin.
				GfDriverSkin skin(strSkinName);

				// Add the whole car livery to the skin targets.
				skin.addTargets(RM_CAR_SKIN_TARGET_WHOLE_LIVERY);
				
				GfLogDebug("  Found %s%s livery\n",
						   strSkinName.empty() ? "standard" : strSkinName.c_str(),
						   strSkinName.empty() ? "" : "-skinned");
				
				// Add associated preview image, without really checking file existence
				// (warn only ; up to the client GUI to do what to do if it doesn't exist).
				std::ostringstream ossPreviewName;
				ossPreviewName << strFolderPath << '/' << strCarId;
				if (!strSkinName.empty())
					ossPreviewName << '-' << strSkinName;
				ossPreviewName << pszPreviewTexSufx;
				skin.setCarPreviewFileName(ossPreviewName.str());

				if (!GfFileExists(ossPreviewName.str().c_str()))
					GfLogWarning("Preview file not found for %s %s skin (%s)\n",
								 strCarId.c_str(), strSkinName.c_str(), ossPreviewName.str().c_str());
				//else
				//	GfLogDebug("* found skin=%s, preview=%s\n",
				//			   strSkinName.c_str(), ossPreviewName.str().c_str());

				// Add the new skin to the list.
				vecPossSkins.push_back(skin);
			}

		}
		while (pCurLiveryFile != pLiveryFileList);
	}
	
	GfDirFreeList(pLiveryFileList, NULL, true, true);
	
	// Search for skinned interior files, if any.
	std::string strInteriorPrefix(strCarId);
	strInteriorPrefix += pszInteriorTexSufx;
	tFList *pIntFileList =
		GfDirGetListFiltered(strFolderPath.c_str(), strInteriorPrefix.c_str(), pszInteriorTexExt);
	if (pIntFileList)
	{
		tFList *pCurIntFile = pIntFileList;
		do
		{
			pCurIntFile = pCurIntFile->next;

			// Extract the skin name from the interior file name.
			const int nSkinNameLen = // Expecting "<car name>-int-<skin name>.png"
				strlen(pCurIntFile->name) - strInteriorPrefix.length()
				- 1 - strlen(pszInteriorTexExt);
			std::string strSkinName;
			if (nSkinNameLen > 0)
			{
				strSkinName =
					std::string(pCurIntFile->name)
					.substr(strInteriorPrefix.length() + 1, nSkinNameLen);

				// If a skin with such name already exists in the list, update it.
				std::vector<GfDriverSkin>::iterator itSkin =
					findSkin(vecPossSkins, strSkinName);
				if (itSkin != vecPossSkins.end())
				{
					itSkin->addTargets(RM_CAR_SKIN_TARGET_INTERIOR);
					GfLogDebug("  Found %s-skinned interior (targets:%x)\n",
							   strSkinName.c_str(), itSkin->getTargets());
				}
			}
		}
		while (pCurIntFile != pIntFileList);
	}
	
	GfDirFreeList(pIntFileList, NULL, true, true);
	
	// Search for skinned logo files if any.
	tFList *pLogoFileList =
		GfDirGetListFiltered(strFolderPath.c_str(), pszLogoTexName, pszLogoTexExt);
	if (pLogoFileList)
	{
		tFList *pCurLogoFile = pLogoFileList;
		do
		{
			pCurLogoFile = pCurLogoFile->next;

			// Extract the skin name from the logo file name.
			const int nSkinNameLen = // Expecting "logo-<skin name>.png"
				strlen(pCurLogoFile->name) - strlen(pszLogoTexName)
				- 1 - strlen(pszLogoTexExt);
			if (nSkinNameLen > 0)
			{
				const std::string strSkinName =
					std::string(pCurLogoFile->name)
					.substr(strlen(pszLogoTexName) + 1, nSkinNameLen);
			
				// If a skin with such name already exists in the list, update it.
				std::vector<GfDriverSkin>::iterator itSkin = findSkin(vecPossSkins, strSkinName);
				if (itSkin != vecPossSkins.end())
				{
					itSkin->addTargets(RM_CAR_SKIN_TARGET_PIT_DOOR);
					GfLogDebug("  Found %s-skinned logo (targets:%x)\n",
							   strSkinName.c_str(), itSkin->getTargets());
				}
			}
				
		}
		while (pCurLogoFile != pLogoFileList);
	}
	
	GfDirFreeList(pLogoFileList, NULL, true, true);
	
	// Search for skinned 3D wheel files if any.
	tFList *pWheel3DFileList =
		GfDirGetListFiltered(strFolderPath.c_str(), pszWheel3DTexName, pszWheel3DTexExt);
	if (pWheel3DFileList)
	{
		tFList *pCurWheel3DFile = pWheel3DFileList;
		do
		{
			pCurWheel3DFile = pCurWheel3DFile->next;

			// Extract the skin name from the 3D wheel texture file name.
			const int nSkinNameLen = // Expecting "wheel3d-<skin name>.png"
				strlen(pCurWheel3DFile->name) - strlen(pszWheel3DTexName)
				- 1 - strlen(pszWheel3DTexExt);
			if (nSkinNameLen > 0)
			{
				const std::string strSkinName =
					std::string(pCurWheel3DFile->name)
					.substr(strlen(pszWheel3DTexName) + 1, nSkinNameLen);
			
				// If a skin with such name already exists in the list, update it.
				std::vector<GfDriverSkin>::iterator itSkin = findSkin(vecPossSkins, strSkinName);
				if (itSkin != vecPossSkins.end())
				{
					itSkin->addTargets(RM_CAR_SKIN_TARGET_3D_WHEELS);
					GfLogDebug("  Found %s-skinned 3D wheels (targets:%x)\n",
							   strSkinName.c_str(), itSkin->getTargets());
				}
			}
		}
		while (pCurWheel3DFile != pWheel3DFileList);
	}
	
	GfDirFreeList(pWheel3DFileList, NULL, true, true);
	
	// Search for skinned driver files if any.
	tFList *pDriverFileList =
		GfDirGetListFiltered(strFolderPath.c_str(), pszDriverTexName, pszDriverTexExt);
	if (pDriverFileList)
	{
		tFList *pCurDriverFile = pDriverFileList;
		do
		{
			pCurDriverFile = pCurDriverFile->next;

			// Extract the skin name from the 3D wheel texture file name.
			const int nSkinNameLen = // Expecting "driver-<skin name>.png"
				strlen(pCurDriverFile->name) - strlen(pszDriverTexName)
				- 1 - strlen(pszDriverTexExt);
			if (nSkinNameLen > 0)
			{
				const std::string strSkinName =
					std::string(pCurDriverFile->name)
					.substr(strlen(pszDriverTexName) + 1, nSkinNameLen);
			
				// If a skin with such name already exists in the list, update it.
				std::vector<GfDriverSkin>::iterator itSkin = findSkin(vecPossSkins, strSkinName);
				if (itSkin != vecPossSkins.end())
				{
					itSkin->addTargets(RM_CAR_SKIN_TARGET_DRIVER);
					GfLogDebug("  Found %s-skinned driver (targets:%x)\n",
							   strSkinName.c_str(), itSkin->getTargets());
				}
			}
		}
		while (pCurDriverFile != pDriverFileList);
	}
	
	GfDirFreeList(pDriverFileList, NULL, true, true);
}

std::vector<GfDriverSkin> GfDriver::getPossibleSkins(const std::string& strAltCarId) const
{
	const std::string strCarId = strAltCarId.empty() ? _pCar->getId() : strAltCarId;

	GfLogDebug("Checking skins for %s ...\n", strCarId.c_str());

	// Clear the skin and preview lists.
	std::vector<GfDriverSkin> vecPossSkins;

	// Get/check skins/skin targets/previews from the directories in the search path
	// WARNING: Must be consistent with the search paths used in grcar.cpp, grboard.cpp,
	//          grscene.cpp ... etc ... but it is not currently 100% achieved
	//          (pit door logos are not searched by the graphics engine
	//           in the car-dedicated folders ... so they may be "over-detected" here).
	std::ostringstream ossDirPath;
	ossDirPath << GfLocalDir() << "drivers/" << _strModName
			   << '/' << _nItfIndex << '/' << strCarId;
	getPossibleSkinsInFolder(strCarId, ossDirPath.str(), vecPossSkins);

	ossDirPath.str("");
	ossDirPath << GfLocalDir() << "drivers/" << _strModName
			   << '/' << _nItfIndex;
	getPossibleSkinsInFolder(strCarId, ossDirPath.str(), vecPossSkins);

	ossDirPath.str("");
	ossDirPath << GfLocalDir() << "drivers/" << _strModName
			   << '/' << strCarId;
	getPossibleSkinsInFolder(strCarId, ossDirPath.str(), vecPossSkins);

	ossDirPath.str("");
	ossDirPath << GfLocalDir() << "drivers/" << _strModName;
	getPossibleSkinsInFolder(strCarId, ossDirPath.str(), vecPossSkins);

	ossDirPath.str("");
	ossDirPath << "drivers/" << _strModName
			   << '/' << _nItfIndex << '/' << strCarId;
	getPossibleSkinsInFolder(strCarId, ossDirPath.str(), vecPossSkins);

	ossDirPath.str("");
	ossDirPath << "drivers/" << _strModName
			   << '/' << _nItfIndex;
	getPossibleSkinsInFolder(strCarId, ossDirPath.str(), vecPossSkins);

	ossDirPath.str("");
	ossDirPath << "drivers/" << _strModName
			   << '/' << strCarId;
	getPossibleSkinsInFolder(strCarId, ossDirPath.str(), vecPossSkins);

	ossDirPath.str("");
	ossDirPath << "drivers/" << _strModName;
	getPossibleSkinsInFolder(strCarId, ossDirPath.str(), vecPossSkins);

	ossDirPath.str("");
	ossDirPath << "cars/models/" << strCarId;
	getPossibleSkinsInFolder(strCarId, ossDirPath.str(), vecPossSkins);

	// If we have at least 1 skin, make sure that, if the standard one is inside,
	// it is the first one.
	if (!vecPossSkins.empty())
	{
		std::vector<GfDriverSkin>::iterator itSkin;
		for (itSkin = vecPossSkins.begin(); itSkin != vecPossSkins.end(); itSkin++)
		{
			if (itSkin->getName().empty() && itSkin != vecPossSkins.begin())
			{
				GfDriverSkin stdSkin = *itSkin;
				vecPossSkins.erase(itSkin);
				vecPossSkins.insert(vecPossSkins.begin(), stdSkin);
				break;
			}
		}
	}
	
	// If no skin was found, add the car's standard one
	// (that way, the skin list will never be empty, and that's safer)
	else
	{
		GfLogError("No skin at all found for '%s/%d/%s' : adding dummy '%s' one\n",
				   _strModName.c_str(), _nItfIndex, strCarId.c_str(), "standard");
		
		GfDriverSkin stdSkin;
		std::ostringstream ossPreviewName;
		ossPreviewName << "cars/models/" << strCarId << '/' << strCarId << pszPreviewTexSufx;
		stdSkin.setCarPreviewFileName(ossPreviewName.str());

		if (!GfFileExists(ossPreviewName.str().c_str()))
			GfLogWarning("No preview file %s found for dummy '%s' skin\n",
						 ossPreviewName.str().c_str(), "standard");

		vecPossSkins.push_back(stdSkin);
	}

	return vecPossSkins;
}

