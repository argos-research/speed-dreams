//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
// unitmain.cpp
//--------------------------------------------------------------------------*
// A robot for SPEED-DREAMS-Version 2.X simuV4
//--------------------------------------------------------------------------*
// Interface to SPEED-DREAMS
// 
// File         : unitmain.cpp 
// Created      : 2008.01.27
// Last changed : 2013.07.05
// Copyright    : © 2007-2013 Wolf-Dieter Beelitz
// eMail        : wdb@wdbee.de
// Version      : 4.01.000
//--------------------------------------------------------------------------*
// V4.01.000:
// New code for avoiding and overtaking 
//--------------------------------------------------------------------------*
// V4.00.002:
// Modifications for Supercars
// Slow down at narrow turns more than normal (Needed for Aalborg)
// Individual parameter SlowRadius
// Limit side use to inner side to halve of outer side
//--------------------------------------------------------------------------*
// V4.00.000 (SimuV4)(Single Wheel Braking, Air Brake):
// New Logging used
// Rescaled braking to allow lower max brake pressure
//--------------------------------------------------------------------------*
// V3.06.000 (SimuV2.1)(Genetic Parameter Optimisation):
// Additional parameter to control loading of racinglines
// Renamed parameter "start fuel" to "initial fuel"
// Deleted old teammanager
// Clear old racingline if optimisation is working
// Switch off skilling if optimisation is working
// Deleted old TORCS related code
//--------------------------------------------------------------------------*
// V3.05.001 (SimuV2.1):
// Separated hairpin calculations
// Driving while rain with ls2-bavaria-g3gtr
//--------------------------------------------------------------------------*
// V3.04.000 (SimuV2.1):
// Skilling for career mode
//--------------------------------------------------------------------------*
// V3.03.000 (SimuV2.1):
// Braking for GP36 cars
//--------------------------------------------------------------------------*
// V3.02.000 (SimuV2.1):
// Reworked racingline calculation
//--------------------------------------------------------------------------*
// V3.01.001 (SimuV2.1):
// Reworked racingline structure to reduce file size (83% less).
// Support for ls2 cars
//
// V3.01.000 (SimuV2.1):
// Needed changes to be able to control cars for simuV2.1
// Totally reworked pitting
// - Still work in progress
//--------------------------------------------------------------------------*
// V2.00.01 (Speed Dreams - Career mode):
// Uses new Speed Dreams Interfaces and was extended to use career mode
//--------------------------------------------------------------------------*
// V2.00 (Speed Dreams):
// Uses new Speed Dreams Interfaces
//--------------------------------------------------------------------------*
// V1.10:
// Features of the advanced TORCS Interface:
// Initialization runs once only, see "simplix(tModInfo *ModInfo)"
// Allways gives back the names of drivers as defined in teams xml file!
// Checks and handles pitsharing state enabled/disabled for endurance races.
// 
// Eigenschaften des erweiterten TORCS Interfaces:
// Die Initialisierung wird nur einmal ausgeführt, siehe dazu 
// "simplix(tModInfo *ModInfo)"
// Die DLL gibt die Namen der Fahrer immer so an TORCS zurück, wie sie in 
// der XML-Datei des Teams angegeben sind!
// Wertet den Pitsharing-Status aus und handelt danach bei Endurance Rennen
//--------------------------------------------------------------------------*
// This program was developed and tested on windows XP
// There are no known Bugs, but:
// Who uses the files accepts, that no responsibility is adopted
// for bugs, dammages, aftereffects or consequential losses.
//
// Das Programm wurde unter Windows XP entwickelt und getestet.
// Fehler sind nicht bekannt, dennoch gilt:
// Wer die Dateien verwendet erkennt an, dass für Fehler, Schäden,
// Folgefehler oder Folgeschäden keine Haftung übernommen wird.
//--------------------------------------------------------------------------*
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// Im übrigen gilt für die Nutzung und/oder Weitergabe die
// GNU GPL (General Public License)
// Version 2 oder nach eigener Wahl eine spätere Version.
//--------------------------------------------------------------------------*

#include <tgf.h>
#include <track.h>
#include <car.h>
#include <raceman.h>
#include <robottools.h>
#include <timeanalysis.h>
#include <robot.h>

#include "unitglobal.h"
#include "unitcommon.h"

#include "unitdriver.h"

//==========================================================================*
// Prototypes of routines(functions/procedures), provided
// for communication with TORCS using the traditional Interface
// Prototypen der Routinen(Funktionen/Prozeduren), die wir für die
// Kommunikation mit TORCS über das traditionale Interface bereitstellen
//--------------------------------------------------------------------------*
static void InitTrack
  (int index,
  tTrack* track,
  void *carHandle,
  void **carParmHandle,
  tSituation *s);
static void NewRace
  (int index,
  tCarElt* car,
  tSituation *s);
static void Drive
  (int index,
  tCarElt* car,
  tSituation *s);
static int PitCmd
  (int index,
  tCarElt* car,
  tSituation *s);
static void Shutdown
  (int index);
static int InitFuncPt
  (int index,
  void *pt);
static void EndRace
  (int index,
  tCarElt *car,
  tSituation *s);
static void Shutdown
  (int index);
//==========================================================================*

//==========================================================================*
// Speed Dreams-Interface
//--------------------------------------------------------------------------*
static const int MAXNBBOTS = MAX_NBBOTS;         // Number of drivers/robots
static const int BUFSIZE = 256;

// Default driver names
static char const* defaultBotName[MAXNBBOTS] = {
	"driver 1",  "driver 2",  "driver 3",  "driver 4",  "driver 5",
	"driver 6",  "driver 7",  "driver 8",  "driver 9",  "driver 10", 
	"driver 11", "driver 12", "driver 13", "driver 14", "driver 15",
	"driver 16", "driver 17", "driver 18", "driver 19", "driver 20" 
};

// Default driver descriptions
static char const* defaultBotDesc[MAXNBBOTS] = {
	"driver 1",  "driver 2",  "driver 3",  "driver 4",  "driver 5",
	"driver 6",  "driver 7",  "driver 8",  "driver 9",  "driver 10", 
	"driver 11", "driver 12", "driver 13", "driver 14", "driver 15",
	"driver 16", "driver 17", "driver 18", "driver 19", "driver 20" 
};

// Max length of a drivers name
static const int DRIVERLEN = 32;                 
// Max length of a drivers description
static const int DESCRPLEN = 256;                 
// Pointer to buffer for driver's names defined in robot's xml-file
static char *DriverNames; 
// Pointer to buffer for driver's descriptions defined in robot's xml-file
static char *DriverDescs; 

// Number of drivers defined in robot's xml-file
static int NBBOTS = 0;                           // Still unknown
// Robot's name
static char BufName[BUFSIZE];                    // Buffer for robot's name
static const char* RobName = BufName;            // Pointer to robot's name
// Robot's relative dir
static char BufPathDirRel[BUFSIZE];              // Robot's dir relative
static const char* RobPathDirRel = BufPathDirRel;// to installation dir
// Robot's relative xml-filename
static char BufPathXMLRel[BUFSIZE];              // Robot's xml-filename
static const char* RobPathXMLRel = BufPathXMLRel;// relative to install. dir
// Robot's absolute dir
static char BufPathDir[BUFSIZE];                 // Robot's dir 
// Robot's absolute xml-filename
static char BufPathXML[BUFSIZE];                 // Robot's xml-filename
static const char* RobPathXML = BufPathXML;      // Pointer to xml-filename

// Save start index offset from robot's xml file
static int IndexOffset = 0;

// Marker for undefined drivers to be able to comment out drivers 
// in the robot's xml-file between others, not only at the end of the list
char undefined[] = "undefined";      

// The "Simplix" logger instance
GfLogger* PLogSimplix = 0;
//==========================================================================*

//==========================================================================*
//  Robot of this modul
//  Roboter des Moduls
//--------------------------------------------------------------------------*
static TCommonData gCommonData;
static int cRobotType;

typedef struct stInstanceInfo
{
	TDriver *cRobot;
	double cTicks;
	double cMinTicks;
	double cMaxTicks;
	int cTickCount;
	int cLongSteps;
	int cCriticalSteps;
	int cUnusedCount;
} tInstanceInfo;

//#undef ROB_SECT_ARBITRARY
#ifdef ROB_SECT_ARBITRARY
static tInstanceInfo *cInstances;
static int cInstancesCount;
#else //ROB_SECT_ARBITRARY
static tInstanceInfo cInstances[MAXNBBOTS];
#endif //ROB_SECT_ARBITRARY

//==========================================================================*

//==========================================================================*
// Get filehandle for robot's xml-file
//--------------------------------------------------------------------------*
void* GetFileHandle(const char* RobotName)
{
    void* RobotSettings = NULL;

	strncpy(BufName, RobotName, BUFSIZE);       // Save robot's name
    snprintf(BufPathDirRel, BUFSIZE,             // Robot's directory  
		"drivers/%s",RobotName);                 // relative to installation
    snprintf(BufPathXMLRel, BUFSIZE,             // Robot's xml-filename
		"drivers/%s/%s.xml",RobotName,RobotName);// relative to installation

	// Test local installation path
    snprintf(BufPathXML, BUFSIZE, "%s%s",         
		GetLocalDir(), RobPathXMLRel);
	snprintf(BufPathDir, BUFSIZE, "%s%s", 
		GetLocalDir(), RobPathDirRel);
	RobotSettings = GfParmReadFile
		(RobPathXML, GFPARM_RMODE_STD );

	if (!RobotSettings)
	{
	  // If not found, use global installation path
	  snprintf(BufPathXML, BUFSIZE, "%s%s", 
		  GetDataDir(), RobPathXMLRel);
  	  snprintf(BufPathDir, BUFSIZE, "%s%s", 
		  GetDataDir(), RobPathDirRel);
	  RobotSettings = GfParmReadFile
		  (RobPathXML, GFPARM_RMODE_STD );
	}
	return RobotSettings;
}
//==========================================================================*

//==========================================================================*
// Set parameters 
//--------------------------------------------------------------------------*
void SetParameters(int N, char const* DefaultCarType)
{
  NBBOTS = N;
  TDriver::NBBOTS = N;                                    // Used nbr of cars
  TDriver::MyBotName = BufName;                           // Name of this bot 
  TDriver::ROBOT_DIR = BufPathDir;                        // Path to dll
  TDriver::SECT_PRIV = "simplix private";                 // Private section
  TDriver::DEFAULTCARTYPE  = DefaultCarType;              // Default car type
  TDriver::Learning = true;
};
//==========================================================================*

//==========================================================================*
// Schismatic entry point for simplix
//--------------------------------------------------------------------------*
void SetUpSimplix()
{
    cRobotType = RTYPE_SIMPLIX;
    SetParameters(NBBOTS, "car1-trb1");
    TDriver::AdvancedParameters = true;
    TDriver::SkillingFactor = 0.1f;         // Skilling factor for career-mode
};
//==========================================================================*

//==========================================================================*
// Schismatic entry point for simplix_mpa1
//--------------------------------------------------------------------------*
void SetUpSimplix_mpa1()
{
	cRobotType = RTYPE_SIMPLIX_MPA1;
	SetParameters(NBBOTS, "indycar01");
    TDriver::AdvancedParameters = true;
    TDriver::UseBrakeLimit = false;
    TDriver::UseMPA1Skilling = true;            
    TDriver::SkillingFactor = 0.1f;         // Skilling factor for career-mode
	TDriver::UseRacinglineParameters = true;
};
//==========================================================================*

//==========================================================================*
// Schismatic entry point for simplix_trb1
//--------------------------------------------------------------------------*
void SetUpSimplix_trb1()
{
    cRobotType = RTYPE_SIMPLIX_TRB1;
    SetParameters(NBBOTS, "car1-trb1");
    TDriver::SkillingFactor = 0.1f;         // Skilling factor for career-mode
	TDriver::UseRacinglineParameters = true;
};
//==========================================================================*

//==========================================================================*
// Schismatic entry point for simplix_sc
//--------------------------------------------------------------------------*
void SetUpSimplix_sc()
{
    cRobotType = RTYPE_SIMPLIX_SC;
    SetParameters(NBBOTS, "sc996");
    TDriver::UseSCSkilling = true;                 // Use supercar skilling
    TDriver::SkillingFactor = 0.1f;         // Skilling factor for career-mode
	TDriver::UseRacinglineParameters = true;
};
//==========================================================================*

//==========================================================================*
// Schismatic entry point for simplix_36GP
//--------------------------------------------------------------------------*
void SetUpSimplix_36GP()
{
    cRobotType = RTYPE_SIMPLIX_36GP;
    SetParameters(NBBOTS, "36GP-alfa12c");
    TDriver::AdvancedParameters = true;
    TDriver::UseBrakeLimit = true;
    TDriver::UseGPBrakeLimit = true;
    TDriver::SkillingFactor = 0.1f;         // Skilling factor for career-mode
};
//==========================================================================*

//==========================================================================*
// Schismatic entry point for simplix_ls1
//--------------------------------------------------------------------------*
void SetUpSimplix_ls1()
{
	cRobotType = RTYPE_SIMPLIX_LS1;
	SetParameters(NBBOTS, "ls1-archer-r9");
    TDriver::AdvancedParameters = true;
    //TDriver::UseBrakeLimit = true;
    TDriver::UseBrakeLimit = false;
    TDriver::SkillingFactor = 0.1f;         // Skilling factor for career-mode
	TDriver::UseRacinglineParameters = true;
};
//==========================================================================*

//==========================================================================*
// Schismatic entry point for simplix_ls2
//--------------------------------------------------------------------------*
void SetUpSimplix_ls2()
{
	cRobotType = RTYPE_SIMPLIX_LS2;
	SetParameters(NBBOTS, "ls2-bavaria-g3gtr");
    TDriver::AdvancedParameters = true;
    TDriver::UseBrakeLimit = true;
	TDriver::UseRacinglineParameters = true;
    TDriver::SkillingFactor = 0.1f;         // Skilling factor for career-mode
};
//==========================================================================*

//==========================================================================*
// Schismatic entry point for simplix_MP5
//--------------------------------------------------------------------------*
void SetUpSimplix_mp5()
{
	cRobotType = RTYPE_SIMPLIX_MP5;
	SetParameters(NBBOTS, "mp5");
    TDriver::AdvancedParameters = true;
    TDriver::UseBrakeLimit = true;
	//TDriver::UseSCSkilling = true; 
    TDriver::SkillingFactor = 0.1f;         // Skilling factor for career-mode
};

//==========================================================================*

//==========================================================================*
// Schismatic entry point for simplix_lp1
//--------------------------------------------------------------------------*
void SetUpSimplix_lp1()
{
    cRobotType = RTYPE_SIMPLIX_LP1;
    SetParameters(NBBOTS, "lp1-vieringe-vr8");
    TDriver::SkillingFactor = 0.1f;         // Skilling factor for career-mode
};
//==========================================================================*

//==========================================================================*
// Schismatic entry point for simplix_ref
//--------------------------------------------------------------------------*
void SetUpSimplix_ref()
{
    cRobotType = RTYPE_SIMPLIX_REF;
    SetParameters(NBBOTS, "ref.sector-p4");
	TDriver::UseRacinglineParameters = true;
	TDriver::UseWingControl = true;
};
//==========================================================================*

//==========================================================================*
// Handle module entry for Speed Dreams Interface V1.00 (new fixed name scheme)
//--------------------------------------------------------------------------*
int moduleWelcomeV1_00
  (const tModWelcomeIn* welcomeIn, tModWelcomeOut* welcomeOut)
{
	PLogSimplix = GfLogger::instance("Simplix");
	LogSimplix.debug("\n#Interface Version: %d.%d\n",
		welcomeIn->itfVerMajor,welcomeIn->itfVerMinor);

	// Get filehandle for robot's xml-file
	void* RobotSettings = GetFileHandle(welcomeIn->name);
	// Let's look what we have to provide here
	if (RobotSettings)
	{
		LogSimplix.debug("#Robot name      : %s\n",RobName);
 		LogSimplix.debug("#Robot directory : %s\n",RobPathDirRel);
		LogSimplix.debug("#Robot XML-file  : %s\n",RobPathXMLRel);

		char Buffer[BUFSIZE];
		char *Section = Buffer;

		// To get the number of drivers defined in the
		// robot team definition file we have to count
		// the number of sections within Robots/index!
		snprintf(Buffer, BUFSIZE, "%s/%s", 
			ROB_SECT_ROBOTS, ROB_LIST_INDEX);
		NBBOTS = GfParmGetEltNb(RobotSettings,Buffer);
		LogSimplix.debug("#Nbr of drivers  : %d\n",NBBOTS);

		DriverNames = (char *) calloc(NBBOTS,DRIVERLEN);
		DriverDescs = (char *) calloc(NBBOTS,DESCRPLEN);

		// Setup a path to the first driver section 
		// assuming that it starts with the index 0
		snprintf(Buffer, BUFSIZE, "%s/%s/%d", 
			ROB_SECT_ROBOTS, ROB_LIST_INDEX, 0);

		// Try to get first driver from index 0
		const char *DriverName = GfParmGetStr( RobotSettings, 
			Section, (char *) ROB_ATTR_NAME, undefined);

		// Check wether index 0 is used as start index
		if (strncmp(DriverName,undefined,strlen(undefined)) != 0)
		{
			// Teams xml file uses index 0, 1, ..., N - 1
            IndexOffset = 0; 
		}
		else
		{
			// Teams xml file uses index 1, 2, ..., N
            IndexOffset = 1; 
		}

		// Loop over all possible drivers, clear all buffers, 
		// save defined driver names and desc.
	    int I = 0;
		int N = 0;
		int M = 0;
//		for (I = 0; I < MAXNBBOTS; I++)
		while (N < NBBOTS)
		{
			snprintf(Section, BUFSIZE, "%s/%s/%d", 
				ROB_SECT_ROBOTS, ROB_LIST_INDEX, I + IndexOffset );
			const char *DriverName = GfParmGetStr( RobotSettings, Section, 
				(char *) ROB_ATTR_NAME,undefined);

	        if (strncmp(DriverName,undefined,strlen(undefined)) != 0)
			{   // This driver is defined in robot's xml-file
				strncpy(&DriverNames[I*DRIVERLEN], DriverName, DRIVERLEN-1);
			    const char *DriverDesc = GfParmGetStr(RobotSettings, Section, 
					(char *) ROB_ATTR_DESC, defaultBotDesc[I]);
				strncpy(&DriverDescs[I*DESCRPLEN], DriverDesc, DESCRPLEN-1);
				LogSimplix.debug("#Driver %d: %s (%s)\n",I,DriverName,DriverDesc);
				N++;
			}
			else
			{
				// There is an index skipped in the robots team definition file
				// Therefore we have to get additional memory to store the data
				M++;
				DriverNames = (char *) realloc(DriverNames,(NBBOTS+M)*DRIVERLEN);
				memset(&DriverNames[I*DRIVERLEN], 0, DRIVERLEN);
				DriverDescs = (char *) realloc(DriverDescs,(NBBOTS+M)*DESCRPLEN);
				memset(&DriverDescs[I*DESCRPLEN], 0, DESCRPLEN);
				LogSimplix.debug("#Driver %d: %s (%s)\n",I,&DriverNames[I*DRIVERLEN],&DriverDescs[I*DESCRPLEN]);
			}

			I++;
		}
		GfParmReleaseHandle(RobotSettings);
	}
	else
	{
		// Handle error here
 	    LogSimplix.debug("#Robot XML-Path not found: (%s) or (%s) %s\n\n",
			GetLocalDir(),GetDataDir(),RobPathXMLRel);

		NBBOTS = 0;
		// But this is not considered a real failure of moduleWelcome !
	}

	// Handle additional settings for wellknown identities
	if (strncmp(RobName,"simplix_trb1",strlen("simplix_trb1")) == 0)
		SetUpSimplix_trb1();
	else if (strncmp(RobName,"simplix_sc",strlen("simplix_sc")) == 0)
		SetUpSimplix_sc();
	else if (strncmp(RobName,"simplix_36GP",strlen("simplix_36GP")) == 0)
		SetUpSimplix_36GP();
	else if (strncmp(RobName,"simplix_mpa1",strlen("simplix_mpa1")) == 0)
		SetUpSimplix_mpa1();
	else if (strncmp(RobName,"simplix_ls1",strlen("simplix_ls1")) == 0)
		SetUpSimplix_ls1();
	else if (strncmp(RobName,"simplix_ls2",strlen("simplix_ls2")) == 0)
		SetUpSimplix_ls2();
	else if (strncmp(RobName,"simplix_mp5",strlen("simplix_mp5")) == 0)
		SetUpSimplix_mp5();
	else if (strncmp(RobName,"simplix_lp1", strlen("simplix_lp1")) == 0)
		SetUpSimplix_lp1();
	else if (strncmp(RobName,"simplix_ref", strlen("simplix_ref")) == 0)
		SetUpSimplix_ref();
	else 
		SetUpSimplix();

	// Set max nb of interfaces to return.
	welcomeOut->maxNbItf = NBBOTS;

	return 0;
}
//==========================================================================*

//==========================================================================*
// Module entry point (new fixed name scheme).
// Extended for use with schismatic robots and checked interface versions
//--------------------------------------------------------------------------*
extern "C" int moduleWelcome
  (const tModWelcomeIn* welcomeIn, tModWelcomeOut* welcomeOut)
{
	if (welcomeIn->itfVerMajor >= 1)
	{
		if (welcomeIn->itfVerMinor > 0)
			// For future use add updated versions here
			return moduleWelcomeV1_00(welcomeIn, welcomeOut);
		else
			// Initial version
			return moduleWelcomeV1_00(welcomeIn, welcomeOut);
	}

	LogSimplix.debug("\n#Unhandled Interface Version: %d.%d\n",
  		welcomeIn->itfVerMajor,welcomeIn->itfVerMinor);
	welcomeOut->maxNbItf = 0;
	return -1;
}
//==========================================================================*

//==========================================================================*
// Module entry point (new fixed name scheme).
// Tells TORCS, who we are, how we want to be called and 
// what we are able to do.
// Teilt TORCS mit, wer wir sind, wie wir angesprochen werden wollen und
// was wir können.
//--------------------------------------------------------------------------*
extern "C" int moduleInitialize(tModInfo *ModInfo)
{
  LogSimplix.debug("\n#Initialize from %s ...\n",RobPathXML);
  LogSimplix.debug("#NBBOTS: %d (of %d)\n",NBBOTS,MAXNBBOTS);

#ifdef ROB_SECT_ARBITRARY
  // Clear all structures.
  memset(ModInfo, 0, (NBBOTS+1)*sizeof(tModInfo));

  int I;
  for (I = 0; I < TDriver::NBBOTS; I++) 
  {
    ModInfo[I].name = &DriverNames[I*DRIVERLEN]; // Tell customisable name
    ModInfo[I].desc = &DriverDescs[I*DESCRPLEN]; // Tell customisable desc.
    ModInfo[I].fctInit = InitFuncPt;             // Common used functions
    ModInfo[I].gfId = ROB_IDENT;                 // Robot identity
    ModInfo[I].index = I+IndexOffset;            // Drivers index
  }
  ModInfo[NBBOTS].name = RobName;
  ModInfo[NBBOTS].desc = RobName;
  ModInfo[NBBOTS].fctInit = InitFuncPt;
  ModInfo[NBBOTS].gfId = ROB_IDENT;
  ModInfo[NBBOTS].index = NBBOTS+IndexOffset;
#else //ROB_SECT_ARBITRARY
  // Clear all structures.
  memset(ModInfo, 0, NBBOTS*sizeof(tModInfo));

  int I;
  for (I = 0; I < TDriver::NBBOTS; I++) 
  {
    ModInfo[I].name = &DriverNames[I*DRIVERLEN]; // Tell customisable name
    ModInfo[I].desc = &DriverDescs[I*DESCRPLEN]; // Tell customisable desc.
    ModInfo[I].fctInit = InitFuncPt;             // Common used functions
    ModInfo[I].gfId = ROB_IDENT;                 // Robot identity
    ModInfo[I].index = I+IndexOffset;            // Drivers index
  }
#endif //ROB_SECT_ARBITRARY

  LogSimplix.debug("# ... Initialized\n\n");

  return 0;
}
//==========================================================================*

//==========================================================================*
// Module exit point (new fixed name scheme).
//--------------------------------------------------------------------------*
extern "C" int moduleTerminate()
{
	LogSimplix.debug("#Terminated %s\n\n",RobName);

  if (DriverNames)
    free(DriverNames); 
  DriverNames = NULL;

  if (DriverDescs)
    free(DriverDescs); 
  DriverDescs = NULL;
	
  return 0;
}
//==========================================================================*

//==========================================================================*
// Module entry point (Torcs backward compatibility scheme).
//--------------------------------------------------------------------------*
int simplixEntryPoint(tModInfo *ModInfo, void *RobotSettings)
{
    LogSimplix.debug("\n#Torcs backward compatibility scheme used\n");
    NBBOTS = MIN(10,NBBOTS);

    memset(ModInfo, 0, NBBOTS*sizeof(tModInfo));
	DriverNames = (char *) calloc(10,DRIVERLEN);
	DriverDescs = (char *) calloc(10,DESCRPLEN);
    memset(DriverNames, 0, 10*DRIVERLEN);
    memset(DriverDescs, 0, 10*DESCRPLEN);

    char SectionBuf[BUFSIZE];
	char *Section = SectionBuf;

	snprintf( SectionBuf, BUFSIZE, "%s/%s/%d", 
		ROB_SECT_ROBOTS, ROB_LIST_INDEX, 0);

    int I;
    for (I = 0; I < NBBOTS; I++) 
    {
	  snprintf( SectionBuf, BUFSIZE, "%s/%s/%d", 
		  ROB_SECT_ROBOTS, ROB_LIST_INDEX, I + IndexOffset );
	  const char *DriverName = GfParmGetStr( RobotSettings, 
		  Section, (char *) ROB_ATTR_NAME, defaultBotName[I]);
	  strncpy(&DriverNames[I*DRIVERLEN], DriverName, DRIVERLEN-1);
      const char *DriverDesc = GfParmGetStr( RobotSettings, 
		  Section, (char *) ROB_ATTR_DESC, defaultBotDesc[I]);
	  strncpy(&DriverDescs[I*DESCRPLEN], DriverDesc, DESCRPLEN-1);
    }

    return moduleInitialize(ModInfo);
}
//==========================================================================*

//==========================================================================*
// Module exit point (Torcs backward compatibility scheme).
//--------------------------------------------------------------------------*
extern "C" int simplixShut()
{
    return moduleTerminate();
}
//==========================================================================*

//==========================================================================*
// TORCS: Initialization
// TOCRS: Initialisierung
//
// After clarification of the general calling (calling this func.), 
// we tell TORCS our functions to provide the requested services:
//
// Nach Klärung der generellen Ansprache (Aufruf dieser Fkt), teilen wir
// TORCS nun noch mit, mit welchen Funktionen wir die angeforderten
// Leistungen erbringen werden:
//
// Die geforderten Leistungen müssen erbracht werden ...
// RbNewTrack: ... wenn Torcs eine neue Rennstrecke bereitstellt
// RbNewRace:  ... wenn Torcs ein neues Rennen startet
// RbDrive:    ... wenn das Rennen gefahren wird
// RbPitCmd:   ... wenn wir einen Boxenstop machen
// RbEndRace:  ... wenn das Rennen ist beendet
// RbShutDown: ... wenn der ggf. angefallene Schrott beseitigt werden muss
//--------------------------------------------------------------------------*
static int InitFuncPt(int Index, void *Pt)
{
  tRobotItf *Itf = (tRobotItf *)Pt;              // Get typed pointer

  Itf->rbNewTrack = InitTrack;                   // Store function pointers 
  Itf->rbNewRace  = NewRace;
  Itf->rbDrive    = Drive;
  Itf->rbPitCmd   = PitCmd;
  Itf->rbEndRace  = EndRace;
  Itf->rbShutdown = Shutdown;
  Itf->index      = Index;                       // Store index

#ifdef ROB_SECT_ARBITRARY
  int xx;
  tInstanceInfo *copy;

  //Make sure enough data is allocated
  if (cInstancesCount <= Index-IndexOffset) {
    copy = new tInstanceInfo[Index-IndexOffset+1];
    for (xx = 0; xx < cInstancesCount; ++xx)
      copy[xx] = cInstances[xx];
    for (xx = cInstancesCount; xx < Index-IndexOffset+1; ++xx)
      copy[xx].cRobot = NULL;
    if (cInstancesCount > 0)
      delete []cInstances;
    cInstances = copy;
    cInstancesCount = Index-IndexOffset+1;
  }
#endif

  void* RobotSettings =								// Open robot team definition
	  GetFileHandle(TDriver::MyBotName);

  cInstances[Index-IndexOffset].cRobot =            // Create a driver
	  new TDriver(Index-IndexOffset);
  cInstances[Index-IndexOffset].cRobot->SetBotName  // Store customized name
	  (RobotSettings,								// from Robot's xml-file and
	  &DriverNames[(Index-IndexOffset)*DRIVERLEN]);	// not from drivers xml-file!  

  if (cRobotType == RTYPE_SIMPLIX)
  {
	LogSimplix.debug("#cRobotType == RTYPE_SIMPLIX\n");
    cInstances[Index-IndexOffset].cRobot->CalcSkillingFoo = &TDriver::CalcSkilling_simplix;
    cInstances[Index-IndexOffset].cRobot->CalcFrictionFoo = &TDriver::CalcFriction_simplix_Identity;
    cInstances[Index-IndexOffset].cRobot->CalcCrvFoo = &TDriver::CalcCrv_simplix;
    cInstances[Index-IndexOffset].cRobot->CalcHairpinFoo = &TDriver::CalcHairpin_simplix;
    cInstances[Index-IndexOffset].cRobot->ScaleSide(0.95f,0.95f);
    cInstances[Index-IndexOffset].cRobot->SideBorderOuter(0.20f);
  }
  else if (cRobotType == RTYPE_SIMPLIX_TRB1)
  {
    LogSimplix.debug("#cRobotType == RTYPE_SIMPLIX_TRB1\n");
/*
    cInstances[Index-IndexOffset].cRobot->CalcSkillingFoo = &TDriver::CalcSkilling_simplix;
    cInstances[Index-IndexOffset].cRobot->CalcFrictionFoo = &TDriver::CalcFriction_simplix_TRB1;
    cInstances[Index-IndexOffset].cRobot->CalcCrvFoo = &TDriver::CalcCrv_simplix_Identity;
    cInstances[Index-IndexOffset].cRobot->CalcHairpinFoo = &TDriver::CalcHairpin_simplix;
    cInstances[Index-IndexOffset].cRobot->ScaleSide(0.95f,0.95f);
    cInstances[Index-IndexOffset].cRobot->SideBorderOuter(0.20f);
*/
	cInstances[Index-IndexOffset].cRobot->CalcSkillingFoo = &TDriver::CalcSkilling_simplix;
    cInstances[Index-IndexOffset].cRobot->CalcFrictionFoo = &TDriver::CalcFriction_simplix_Identity;
    cInstances[Index-IndexOffset].cRobot->CalcCrvFoo = &TDriver::CalcCrv_simplix_Identity;
    cInstances[Index-IndexOffset].cRobot->CalcHairpinFoo = &TDriver::CalcHairpin_simplix_Identity;
    cInstances[Index-IndexOffset].cRobot->ScaleSide(0.95f,0.95f);
    cInstances[Index-IndexOffset].cRobot->SideBorderOuter(0.20f);
  }
  else if (cRobotType == RTYPE_SIMPLIX_SC)
  {
    LogSimplix.debug("#cRobotType == RTYPE_SIMPLIX_SC\n");
    cInstances[Index-IndexOffset].cRobot->CalcSkillingFoo = &TDriver::CalcSkilling_simplix_SC;
    cInstances[Index-IndexOffset].cRobot->CalcFrictionFoo = &TDriver::CalcFriction_simplix_Identity;
    cInstances[Index-IndexOffset].cRobot->CalcCrvFoo = &TDriver::CalcCrv_simplix_SC;
//    cInstances[Index-IndexOffset].cRobot->CalcCrvFoo = &TDriver::CalcCrv_simplix_Identity;
    cInstances[Index-IndexOffset].cRobot->CalcHairpinFoo = &TDriver::CalcHairpin_simplix_Identity;
    cInstances[Index-IndexOffset].cRobot->ScaleSide(0.95f,0.95f);
    cInstances[Index-IndexOffset].cRobot->SideBorderOuter(0.10f);
  }
  else if (cRobotType == RTYPE_SIMPLIX_36GP)
  {
    LogSimplix.debug("#cRobotType == RTYPE_SIMPLIX_36GP\n");
    cInstances[Index-IndexOffset].cRobot->CalcSkillingFoo = &TDriver::CalcSkilling_simplix;
    cInstances[Index-IndexOffset].cRobot->CalcFrictionFoo = &TDriver::CalcFriction_simplix_Identity;
    cInstances[Index-IndexOffset].cRobot->CalcCrvFoo = &TDriver::CalcCrv_simplix_36GP;
    cInstances[Index-IndexOffset].cRobot->CalcHairpinFoo = &TDriver::CalcHairpin_simplix;
    cInstances[Index-IndexOffset].cRobot->ScaleSide(0.95f,0.95f);
    cInstances[Index-IndexOffset].cRobot->SideBorderOuter(0.75f);
    cInstances[Index-IndexOffset].cRobot->UseFilterAccel();
  }  
  else if (cRobotType == RTYPE_SIMPLIX_MPA1)
  {
    LogSimplix.debug("#cRobotType == RTYPE_SIMPLIX_MPA1\n");
    cInstances[Index-IndexOffset].cRobot->CalcSkillingFoo = &TDriver::CalcSkilling_simplix_MPA1;
    cInstances[Index-IndexOffset].cRobot->CalcFrictionFoo = &TDriver::CalcFriction_simplix_Identity;
    cInstances[Index-IndexOffset].cRobot->CalcCrvFoo = &TDriver::CalcCrv_simplix_Identity;
    cInstances[Index-IndexOffset].cRobot->CalcHairpinFoo = &TDriver::CalcHairpin_simplix;
    cInstances[Index-IndexOffset].cRobot->ScaleSide(0.95f,0.95f);
    cInstances[Index-IndexOffset].cRobot->SideBorderOuter(0.00f);
    cInstances[Index-IndexOffset].cRobot->UseFilterAccel();
  }
  else if (cRobotType == RTYPE_SIMPLIX_LS1)
  {
    LogSimplix.debug("#cRobotType == RTYPE_SIMPLIX_LS1\n");
    cInstances[Index-IndexOffset].cRobot->CalcSkillingFoo = &TDriver::CalcSkilling_simplix_LS1;
    cInstances[Index-IndexOffset].cRobot->CalcFrictionFoo = &TDriver::CalcFriction_simplix_Identity;
    cInstances[Index-IndexOffset].cRobot->CalcCrvFoo = &TDriver::CalcCrv_simplix_Identity;
    cInstances[Index-IndexOffset].cRobot->CalcHairpinFoo = &TDriver::CalcHairpin_simplix_Identity;
    cInstances[Index-IndexOffset].cRobot->ScaleSide(0.95f,0.95f);
    cInstances[Index-IndexOffset].cRobot->SideBorderOuter(0.20f);
    cInstances[Index-IndexOffset].cRobot->UseFilterAccel();
  }
  else if (cRobotType == RTYPE_SIMPLIX_LS2)
  {
    LogSimplix.debug("#cRobotType == RTYPE_SIMPLIX_LS2\n");
    cInstances[Index-IndexOffset].cRobot->CalcSkillingFoo = &TDriver::CalcSkilling_simplix_LS2;
    cInstances[Index-IndexOffset].cRobot->CalcFrictionFoo = &TDriver::CalcFriction_simplix_LS2;
    cInstances[Index-IndexOffset].cRobot->CalcCrvFoo = &TDriver::CalcCrv_simplix_Identity;
    cInstances[Index-IndexOffset].cRobot->CalcHairpinFoo = &TDriver::CalcHairpin_simplix_Identity;
    cInstances[Index-IndexOffset].cRobot->ScaleSide(0.95f,0.95f);
    cInstances[Index-IndexOffset].cRobot->SideBorderOuter(0.20f);
  }
  else if (cRobotType == RTYPE_SIMPLIX_MP5)
  {
    LogSimplix.debug("#cRobotType == RTYPE_SIMPLIX_MP5\n");
    cInstances[Index-IndexOffset].cRobot->CalcSkillingFoo = &TDriver::CalcSkilling_simplix;
    cInstances[Index-IndexOffset].cRobot->CalcFrictionFoo = &TDriver::CalcFriction_simplix_Identity;
    cInstances[Index-IndexOffset].cRobot->CalcCrvFoo = &TDriver::CalcCrv_simplix_Identity;
    cInstances[Index-IndexOffset].cRobot->CalcHairpinFoo = &TDriver::CalcHairpin_simplix;
    cInstances[Index-IndexOffset].cRobot->ScaleSide(0.95f,0.95f);
    cInstances[Index-IndexOffset].cRobot->SideBorderOuter(0.20f);
  }
  else if (cRobotType == RTYPE_SIMPLIX_LP1)
  {
    LogSimplix.debug("#cRobotType == RTYPE_SIMPLIX_LP1\n");
    cInstances[Index-IndexOffset].cRobot->CalcSkillingFoo = &TDriver::CalcSkilling_simplix;
    cInstances[Index-IndexOffset].cRobot->CalcFrictionFoo = &TDriver::CalcFriction_simplix_LP1;
    cInstances[Index-IndexOffset].cRobot->CalcCrvFoo = &TDriver::CalcCrv_simplix_Identity;
    cInstances[Index-IndexOffset].cRobot->CalcHairpinFoo = &TDriver::CalcHairpin_simplix;
    cInstances[Index-IndexOffset].cRobot->ScaleSide(0.95f,0.95f);
    cInstances[Index-IndexOffset].cRobot->SideBorderOuter(0.20f);
  }
  else if (cRobotType == RTYPE_SIMPLIX_REF)
  {
    LogSimplix.debug("#cRobotType == RTYPE_SIMPLIX_REF\n");
    cInstances[Index-IndexOffset].cRobot->CalcSkillingFoo = &TDriver::CalcSkilling_simplix;
    cInstances[Index-IndexOffset].cRobot->CalcFrictionFoo = &TDriver::CalcFriction_simplix_REF;
    cInstances[Index-IndexOffset].cRobot->CalcCrvFoo = &TDriver::CalcCrv_simplix;
    cInstances[Index-IndexOffset].cRobot->CalcHairpinFoo = &TDriver::CalcHairpin_simplix;
    cInstances[Index-IndexOffset].cRobot->ScaleSide(0.95f,0.95f);
    cInstances[Index-IndexOffset].cRobot->SideBorderOuter(0.20f);
  }

  return 0;
}
//==========================================================================*

//==========================================================================*
// TORCS: New track
// TOCRS: Neue Rennstrecke
//--------------------------------------------------------------------------*
static void InitTrack(int Index,
  tTrack* Track,void *CarHandle,void **CarParmHandle, tSituation *S)
{
  // Init common used data
  cInstances[Index-IndexOffset].cRobot->SetCommonData
	  (&gCommonData,cRobotType);    
  cInstances[Index-IndexOffset].cRobot->InitTrack
	  (Track,CarHandle,CarParmHandle, S);
}
//==========================================================================*

//==========================================================================*
// TORCS: New Race starts
// TOCRS: Neues Rennen beginnt
//--------------------------------------------------------------------------*
static void NewRace(int Index, tCarElt* Car, tSituation *S)
{
  RtInitTimer(); // Check existance of Performance Counter Hardware

  cInstances[Index-IndexOffset].cTicks = 0.0;               // Initialize counters
  cInstances[Index-IndexOffset].cMinTicks = FLT_MAX;        // and time data 
  cInstances[Index-IndexOffset].cMaxTicks = 0.0;
  cInstances[Index-IndexOffset].cTickCount = 0;
  cInstances[Index-IndexOffset].cLongSteps = 0;
  cInstances[Index-IndexOffset].cCriticalSteps = 0;
  cInstances[Index-IndexOffset].cUnusedCount = 0;
  
  cInstances[Index-IndexOffset].cRobot->NewRace(Car, S);
  cInstances[Index-IndexOffset].cRobot->CurrSimTime = -10.0;
}
//==========================================================================*

//==========================================================================*
// TORCS-Callback: Drive
// TOCRS-Callback: Rennen fahren
//
// Attention: This procedure is called very frequent and fast in succession!
// Therefore we don't throw debug messages here!
// To find basic bugs, it may be usefull to do it anyhow!

// Achtung: Diese Prozedur wird sehr häufig und schnell nacheinander
// aufgerufen. Deshalb geben wir hier in der Regel keine Debug-Texte aus!
// Zur Fehlersuche kann das aber mal sinnvoll sein.
//--------------------------------------------------------------------------*
static void Drive(int Index, tCarElt* Car, tSituation *S)
{
  //LogSimplix.debug("#>>> TDriver::Drive\n");
  if (cInstances[Index-IndexOffset].cRobot->CurrSimTime < S->currentTime)
//  if (cInstances[Index-IndexOffset].cRobot->CurrSimTime + 0.03 < S->currentTime)
  {
    //LogSimplix.debug("#Drive\n");
	double StartTimeStamp = RtTimeStamp(); 

    cInstances[Index-IndexOffset].cRobot->CurrSimTime =     // Update current time
		S->currentTime; 
    cInstances[Index-IndexOffset].cRobot->Update(Car,S);    // Update info about opp.
    if (cInstances[Index-IndexOffset].cRobot->IsStuck())    // Check if we are stuck  
  	  cInstances[Index-IndexOffset].cRobot->Unstuck();      //   Unstuck 
	else                                         // or
	  cInstances[Index-IndexOffset].cRobot->Drive();        //   Drive

	double Duration = RtDuration(StartTimeStamp);

	if (cInstances[Index-IndexOffset].cTickCount > 0)       // Collect used time 
	{
	  if (Duration > 1.0)
        cInstances[Index-IndexOffset].cLongSteps++;
	  if (Duration > 2.0)
        cInstances[Index-IndexOffset].cCriticalSteps++;
	  if (cInstances[Index-IndexOffset].cMinTicks > Duration)
	    cInstances[Index-IndexOffset].cMinTicks = Duration;
	  if (cInstances[Index-IndexOffset].cMaxTicks < Duration)
	    cInstances[Index-IndexOffset].cMaxTicks = Duration;
	}
	cInstances[Index-IndexOffset].cTickCount++;
  	cInstances[Index-IndexOffset].cTicks += Duration;
  }
  else
  {
    //LogSimplix.debug("#DriveLast\n");
    cInstances[Index-IndexOffset].cUnusedCount++;
    cInstances[Index-IndexOffset].cRobot->DriveLast();      // Use last drive commands
  }
  //LogSimplix.debug("#<<< TDriver::Drive\n");
}
//==========================================================================*

//==========================================================================*
// TORCS: Pitstop (Car is in pit!)
// TOCRS: Boxenstop (Wagen steht in der Box!)
//--------------------------------------------------------------------------*
static int PitCmd(int Index, tCarElt* Car, tSituation *S)
{
  // Dummy: use parameters
  if ((Index < 0) || (Car == NULL) || (S == NULL))
    LogSimplix.debug("PitCmd\n");
  return cInstances[Index-IndexOffset].cRobot->PitCmd();
}
//==========================================================================*

//==========================================================================*
// TORCS: Race ended
// TOCRS: Rennen ist beendet
//--------------------------------------------------------------------------*
static void EndRace(int Index, tCarElt *Car, tSituation *S)
{
  // Dummy: use parameters
  if ((Index < 0) || (Car == NULL) || (S == NULL))
	  Index = 0;

  LogSimplix.debug("EndRace\n");
  cInstances[Index-IndexOffset].cRobot->EndRace();
}
//==========================================================================*

//==========================================================================*
// TORCS: Cleanup
// TOCRS: Aufräumen
//--------------------------------------------------------------------------*
static void Shutdown(int Index)
{
#ifdef ROB_SECT_ARBITRARY
  int count;
  int xx;
  tInstanceInfo *copy;
#endif //ROB_SECT_ARBITRARY

  LogSimplix.debug("\n\n#Clock\n");
  LogSimplix.debug("#Total Time used: %g sec\n",cInstances[Index-IndexOffset].cTicks/1000.0);
  LogSimplix.debug("#Min   Time used: %g msec\n",cInstances[Index-IndexOffset].cMinTicks);
  LogSimplix.debug("#Max   Time used: %g msec\n",cInstances[Index-IndexOffset].cMaxTicks);
  LogSimplix.debug("#Mean  Time used: %g msec\n",cInstances[Index-IndexOffset].cTicks/cInstances[Index-IndexOffset].cTickCount);
  LogSimplix.debug("#Long Time Steps: %d\n",cInstances[Index-IndexOffset].cLongSteps);
  LogSimplix.debug("#Critical Steps : %d\n",cInstances[Index-IndexOffset].cCriticalSteps);
  LogSimplix.debug("#Unused Steps   : %d\n",cInstances[Index-IndexOffset].cUnusedCount);
  LogSimplix.debug("\n");
  LogSimplix.debug("\n");

  cInstances[Index-IndexOffset].cRobot->Shutdown();
  delete cInstances[Index-IndexOffset].cRobot;
  cInstances[Index-IndexOffset].cRobot = NULL;

#ifdef ROB_SECT_ARBITRARY
  //Check if this was the highest index
  if (cInstancesCount == Index-IndexOffset+1)
  {
    //Now make the cInstances array smaller
    //Count the number of robots which are still in the array
    count = 0;
    for (xx = 0; xx < Index-IndexOffset+1; ++xx)
    {
      if (cInstances[xx].cRobot)
        count = xx+1;
    }

    if (count>0)
    {
      //We have robots left: make a new array
      copy = new tInstanceInfo[count];
      for (xx = 0; xx < count; ++xx)
        copy[xx] = cInstances[xx];
    }
    else
    {
      copy = NULL;
    }
    delete []cInstances;
    cInstances = copy;
    cInstancesCount = count;
  }
#endif //ROB_SECT_ARBITRARY
}
//==========================================================================*

//==========================================================================*
// Module entry point (Torcs backward compatibility scheme).
//--------------------------------------------------------------------------*
extern "C" int simplix(tModInfo *ModInfo)
{
  void *RobotSettings = GetFileHandle("simplix");
  if (!RobotSettings)
	  return -1;
  
  SetParameters(1, "car1-trb1");
  return simplixEntryPoint(ModInfo,RobotSettings);
}
//==========================================================================*

//--------------------------------------------------------------------------*
// end of file unitmain.cpp
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
