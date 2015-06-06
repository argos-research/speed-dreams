/***************************************************************************

    file                 : cars.h
    created              : July 2009
    copyright            : (C) 2009 Brian Gavin, 2010 Jean-Philippe Meuret
    web                  : speed-dreams.sourceforge.net
    version              : $Id: cars.h 3893 2011-09-18 15:52:42Z pouillot $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef __TGFCARS__H__
#define __TGFCARS__H__

#include <string>
#include <vector>

#include <tgf.h>

#include "tgfdata.h"


/** @file   
		Singleton holding information on the available cars and categories
    @defgroup	tgfdata	Data manager for the client gaming framework.
*/

class TGFDATA_API GfCar
{
public:
	
	GfCar(const std::string& strId, const std::string& strCatId,
		  const std::string& strCatName, void* hparmCar);

	const std::string& getId() const;
	const std::string& getName() const;
	const std::string& getCategoryId() const;
	const std::string& getCategoryName() const;
	const std::string& getDescriptorFileName() const;

	enum EDriveTrain { eRWD, eFWD, e4WD, eNDriveTrains };
	EDriveTrain getDriveTrain() const;
	
	unsigned getGearsCount() const;
	
	bool isTurboCharged() const;
	
	unsigned getCylinders() const;
	
	tdble getEngineCapacity() const;
	
	enum EEngineShape { eV, eL, eH, eW, eNEngineShapes };
	EEngineShape getEngineShape() const;
	
	enum EEnginePosition { eFront, eFrontMid, eMid, eRearMid, eRear, eNEnginePositions };
	EEnginePosition getEnginePosition() const;
	
	tdble getMaxPower() const;
	tdble getMaxPowerSpeed() const;
	tdble getMaxTorque() const;
	tdble getMaxTorqueSpeed() const;
	tdble getMass() const;
	tdble getFrontRearMassRatio() const;

	tdble getTopSpeed() const;
	tdble getLowSpeedGrip() const;
	tdble getHighSpeedGrip() const;
	tdble getInvertedZAxisInertia() const;
	
	void load(void* hparmCar);

protected:
	
	std::string _strId; // XML file / folder name (ex: "sc-boxer-96")
	std::string _strName; // User friendly name (ex: "SC Boxer 96").
	std::string _strCatId; // Category XML file / folder name (ex: "LS-GT1").
	std::string _strCatName; // User friendly category name (ex: "Long day Series GT1").
	std::string _strDescFile; // Path-name of the XML descriptor file.

	EDriveTrain _eDriveTrain;
	unsigned _nGears; // Number of gears.
	bool _bTurboCharged; // TODO: Move to an enum (Turbo, Compressor, ...)
	tdble _fEngineCapacity; // litres
	unsigned _nCylinders;
	EEngineShape _eEngineShape;
	EEnginePosition _eEnginePosition;
	tdble _fMaxPower, _fMaxPowerSpeed; // Engine max power (SI) and associated engine speed.
	tdble _fMaxTorque, _fMaxTorqueSpeed; // Engine max torque (Nm) and associated engine speed.
	tdble _fMass; // Total mass (kg).
	tdble _fFrontRearMassRatio; // Front to rear mass ratio (no unit, inside ]0,1[).
	
	tdble _fTopSpeed; // Theorical top speed (m/s)
	tdble _fLowSpeedGrip; // Mechanical grip (~mu*g, but with front/rear mass repartition)
	tdble _fHighSpeedGrip; // Aerodynamic grip (same + with aero down-force)
	tdble _fInvertedZAxisInertia; // (SI)
};

class TGFDATA_API GfCars
{
public:

	// Accessor to the unique instance of the singleton.
	static GfCars* self();
	static void shutdown();
	
	const std::vector<std::string>& getCategoryIds() const;
	const std::vector<std::string>& getCategoryNames() const;

	GfCar* getCar(const std::string& strId) const;
	GfCar* getCarWithName(const std::string& strName) const;

	std::vector<GfCar*> getCarsInCategory(const std::string& strCatId = "") const;
	std::vector<GfCar*> getCarsInCategoryWithName(const std::string& strCatName = "") const;
	std::vector<std::string> getCarIdsInCategory(const std::string& strCatId = "") const;
	std::vector<std::string> getCarNamesInCategory(const std::string& strCatId = "") const;
	
	void print() const;

protected:

	// Protected constructor and destructor : clients can not use them.
	GfCars();
	~GfCars();

protected:

	// The singleton itself.
	static GfCars* _pSelf;

	// Its private data.
	class Private;
	Private* _pPrivate;
};

#endif /* __TGFCARS__H__ */
