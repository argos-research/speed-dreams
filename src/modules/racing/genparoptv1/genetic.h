/***************************************************************************

    file                 : genetic.h
    created              : Tue Nov 04 17:45:00 CET 2010
    copyright            : (C) 2010-2013 by Wolf-Dieter Beelitz
    email                : wdbee@users.sourceforge.net

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
    @defgroup	genetictools	Tools for genetic parameter optimization.
    Collection of functions for genetic parameter optimization.
*/

#ifndef __GENETIC_H__
#define __GENETIC_H__

#include "car.h"

// Additional section and parameter definitions
#define SECT_TOC		"Table of Content"
#define SECT_GLOBAL		"Global"
#define SECT_LOCAL		"Local Groups"
#define SECT_DEFINE		"Definition"
#define SECT_PARAM		"Parameter"

#define SECT_PH_LEFT	"Left"
#define SECT_PH_RGHT	"Right"

#define PRM_ACTIVE		"active"
#define PRM_TWOSIDE		"twosided" // 0: no 1: same sign; -1 opposite sign
#define PRM_LABEL		"label"
#define PRM_SECT		"section"
#define PRM_SUBSECT		"subsection"
#define PRM_PRM			"parameter"
#define PRM_UNIT		"unit"
#define PRM_RANGE		"range"
#define PRM_WEIGHT		"weight"
#define PRM_SCALE		"scale"
#define PRM_ROUND		"round"
#define PRM_NAME		"name"

#define PRM_AUTHOR		"author"
#define PRM_PRIVATE		"private"
#define PRM_LOOPS		"optimisation loops"
#define PRM_DAMAGES		"weight of damages"
#define PRM_INITIAL		"get initial value"

#define PRV_OPTI	    "genetic optimisation"

//  
// Genetic parameters are handled in a single array 
// (See TGeneticParameter** GP;).
//
// At start there are global parameters. 
// Global parameters are car setup parameters like wing angles 
// or robot parameters that are used for the whole track.
//
// This block is followed by one ore more parts defining local
// parameters. The definition of the part of the track that has
// to use a set of such local parameters (let's call it section) 
// is not contained here, just because we do not need it, 
// it has to be handled by the consumer (robot).
// 
// We have the number of parts stored here, so we can handle
// each group of parameters defined in a part based on the 
// index offset to the first parameter of the group and we 
// know the number of parameters per section and the number of 
// sections defined in a part.
// 
// To store these information we use a strucure per part.
// (See tgenPart).

//
// Forewarding of classes
//
class TGeneticParameterTOC;		// Table of content
class TGeneticParameterCounter;	// Local parameter counter
class TGeneticParameter;		// Optimization parameter

//
// Structure to bundle handling data of parts or parameters.
// Used to allow the definition of more than one part of parameters 
// that are defined for subsections of the track as USR or DanDroid do.
//  
typedef struct genPart
{
	int Offset;			// Index offset of first parameter
	int NbrOfSect;		// Number of sections in this part
	int Count;			// Number of parameters per section
	bool Active;		// Block active
	char* Parameter;	// Name of attribute
	char* Subsection;	// Name of subsection
} tgenPart;

//
// Global data for control of parameter optimization
//
// This structure contains all values from the current race 
// we need for genetic parameter optimization
//
typedef struct genData
{
	// Pointer to the current car
	tCarElt* car;			// Pointer to car data

	// Basic data to access setups
	void* Handle;			// Handle to carsetup file
	char* TrackName;		// name of the track selected
	char* CarType;			// car type to create path to setup file
	char* RobotName;		// robot name to create path to setup
	char* AuthorName;		// name of author of setup
	char* PrivateSection;	// name of robot private section

	// Filenames used
	const char* XmlFileName;
	const char* OptFileName;

	// Weather
	int WeatherCode;		// Well known for rain/water at track surface

	// Strategic data, car setup depending on optimization state
    // int Type;			// 0: race; 1: qualifying
	float MaxFuel;			// Default = 60
	float TotalWeight;		// Total of parameters individual weight
	bool First;				// First race with unchanged parameters
	bool GetInitialVal;		// Allow to get initial value from setup

	// Race result data
	double BestLapTime;				// Best laptime (without penalties)
	double WeightedBestLapTime;	    // Best laptime increased by time penalty for damages
    double LastWeightedBestLapTime;	// Last best value

	int DamagesTotal;				// Total of damages taken in race
	int LastDamagesTotal;			// Last total
	double WeightOfDamages;			// Factor to weight damages as time penalties

	//double TopSpeed;				// Max velocity
	//double LastTopSpeed;			// Last max value

	//double MinSpeed;				// Min velocity
    //double LastMinSpeed;			// Last min value

	//double MaxYaw; ...
	//double LastMaxYaw; ...

	//double FuelConsumption; ...
	//double LastFuelConsumption; ...

	// Counters
	int Loops;		// Number of optimisation loops still to do
	float Scale;	// Divisor for Scale factor
	int MaxSelected;// Max number of parameters to select
	int NbrOfParam;	// Number of parameters in total
	int NbrOfParts; // Number of parts used

	// Parts
    tgenPart* Part;			// Pointer to first part structure

	// Parameters
	TGeneticParameter** GP; // Pointer to first parameter

	// Buffers for strings						// FILENAME_MAX = Windows(MAX_PATH = 260)
	char TrackNameBuffer[FILENAME_MAX+1];		// Buffer for trackname 
	char CarTypeBuffer[FILENAME_MAX+1];			// ... for car type
	char RobotNameBuffer[FILENAME_MAX+1];		// ... for robotname
	char AuthorNameBuffer[FILENAME_MAX+1];		// ... name of setup author
	char PrivateSectionBuffer[FILENAME_MAX+1];	// ... name of setup author
	char BufferXML[FILENAME_MAX+1];				// ... name of xml car setup file
	char BufferOPT[FILENAME_MAX+1];				// ... name of opt car setup file

} tgenData;

//
// Class TGeneticParameter
//
// This class holds the meta data of a generic parameter
// used to describe the access to the parameter in the setup files
// and the range allowed for variation while optimization.
//
class TGeneticParameter
{
  public:
	TGeneticParameter();  // Default construtor
	TGeneticParameter     // Constructor
	(
		void* MetaDataFile,			// File handle to get the data
		float MinDef,				// Defines the minimum allowed value
		float ValDef,				// Defines the default value
		float MaxDef,				// Defines the maximum allowed value
		const char *LabelName,		// Gives a short label for console output
		const char *SectionName,	// Section to be used reading in the setup file
		const char *ParameterName,	// Name of data line used reading in the setup file 
		const char *UnitName,		// Defines the unit used for the value
		float ParamWeight,			// Weight of parameter for random selection
		float ParamScale,			// Scales the random variation of the value
		float ParamRound,			// Defines the rounding 
									// (to be able to write it in xml without loss of information)
		int TwoSided = 0			// If there are left and right parameters to be set

	);

	virtual ~TGeneticParameter(); // Destructor

	void DisplayParameter(); // Display parameter definitions at console
	void DisplayStatistik(); // Display statistics at console

	int Set(	// Write meta data to the configuration file
		const char* Part, int Index); 
	int Get(	// Read meta data from the configuration file
		bool First = false, const char* Part = NULL); 

	int GetVal(	// Read initial value from setup file
		void* SetupHandle, bool First = false, bool Local = false); 
	int SetVal(	// Write data to car setup file 
		void* SetupHandle, int Index = 0);

  public:
	void* Handle;	// Handle to read/write data to/from car setup file

    bool Active;	// Allow random selection while optimization
	float Min;		// Min allowed
	float Max;		// Max allowed
	float Val;		// Current value

	float LastVal;	// Last value
	float OptVal;	// Value used getting the best race result till now

	float Def;		// Default value to use if not specified

	float Weight;	// Weigth/TotalWeight = probability to be selected
	float Range;	// Range (Max - Min)
	float Scale;	// Scale random variation
	float Round;	// Define rounding to avoid minimal parameter changes
					// that cannot be stored in the xml file

	// Statistics
	int Tries;		// Number of selections 
	int Changed;	// Number of successfull variations
	bool Selected;	// Parameter is in current selection 
					// (to avoid multiple variations)

	bool LeftRight;
	bool SameSign;

	char *Label;  
	char *Section;
	char *Parameter;
	char *Unit;

	// Common data (class variables)
	static tgenData Data;	// Structure with all data
							// needed for optimization
};


//
// Class Genetic Parameter Part
//
// TGeneticParameterPart
//
// Meta data of parts
// defining how to get it from the setupfile
//
class TGeneticParameterPart
{
  public:
	TGeneticParameterPart();				// Default constructor
	TGeneticParameterPart					// Constructor
	(
		void* MetaDataFile,					// Handle to read the data from
		const char* ShortLabel = NULL,		// Short label for console output
		const char* SectionName = NULL,		// Section to pick up data in the xml file
		const char* ParameterName = NULL,	// Name of data line to use 
		const char* SubSectionName = NULL	// Subsection to pick up data in the xml file
	);

	virtual ~TGeneticParameterPart();		// Destructor

	int Set(int Index);	// Write meta data to configuration file 
	int Get(int Index);	// Read meta data from configuration file 

  public:
	void* Handle;		// Handle to read/write data

    bool Active;		// Allow selection of the part while optimization

	char *Label;
	char *Section;
	char *Subsection;
	char *Parameter;

};


//
// Class Genetic Parameter Table of Content
//
// TGeneticParameterTOC
//
// Table of contents of an xml file for genetic parameters meta data
//
class TGeneticParameterTOC
{
  public:
	TGeneticParameterTOC();			// Default constructor
	TGeneticParameterTOC			// Constructor
	(
		void* MetaDataFile,			// Handle to read /write data
		char* Author = NULL,		// Name of author of setup
		char* Private = NULL,		// Name of private data section
		int Loops = 1000,			// Number of optimisation loops
		float WeightDamages = 1.0,	// Weight of damages as time penalty
		bool GetInitialVal = true   // Read initial value from setup
	);

	virtual ~TGeneticParameterTOC(); // Destructor

	int Set(); // Write table of content to configuration file 
	int Get(); // Read table of content from configuration file 

  public:
	void* Handle;					// Handle to read /write data
	char* Author;					// Name of author of setup
	char* Private;					// Name of private data section
	int OptimisationLoops;			// Number of optimisation loops
	float WeightOfDamages;	        // Weight of damages as time penalty
	bool GetInitialVal;				// Read initial value from setup

};

//==========================================================================*

#endif /* __GENETIC_H__ */
