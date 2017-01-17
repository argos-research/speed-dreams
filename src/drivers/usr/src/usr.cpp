/***************************************************************************

    file                 : usr.cpp
    created              : Wed Jan 8 18:31:16 CET 2003
    copyright            : (C) 2002-2004 Bernhard Wymann
    email                : berniw@bluewin.ch
    version              : $Id: usr.cpp 6066 2015-08-09 18:11:28Z torcs-ng $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifdef _WIN32
#include <windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <tgf.h>
#include <track.h>
#include <car.h>
#include <raceman.h>
#include <robottools.h>
#include <timeanalysis.h>
#include <robot.h>

#include "globaldefs.h"
#include "driver.h"

// Traditional TORCS Interface
static void initTrack(int index, tTrack* track, void *carHandle, void **carParmHandle, tSituation *s);
static void newRace(int index, tCarElt* car, tSituation *s);
static void drive(int index, tCarElt* car, tSituation *s);
static int pitCmd(int index, tCarElt* car, tSituation *s);
static void shutdown(int index);
static int initFuncPt(int index, void *pt);
static void endRace(int index, tCarElt *car, tSituation *s);

extern "C" int usr(tModInfo *modInfo);

//==========================================================================*
// Speed Dreams-Interface
//--------------------------------------------------------------------------*
static const int MAXNBBOTS = MAX_NBBOTS;         // Number of drivers/robots
static const int BUFSIZE = 256;

// Default driver names
static char const* defaultBotName[MAXNBBOTS] =
{
    "driver 1",  "driver 2",  "driver 3",  "driver 4",  "driver 5",
    "driver 6",  "driver 7",  "driver 8",  "driver 9",  "driver 10",
    "driver 11", "driver 12", "driver 13", "driver 14", "driver 15",
    "driver 16", "driver 17", "driver 18", "driver 19", "driver 20"
};

// Default driver descriptions
static char const* defaultBotDesc[MAXNBBOTS] =
{
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

static int m_RobotType;  //Decide if TRB, SC, GP36, LS or some other driver

// The "USR" logger instance
GfLogger* PLogUSR = 0;

typedef struct m_InstanceInfo
{
    Driver  *m_Robot;
    double  m_Ticks;
    double  m_MinTicks;
    double  m_MaxTicks;
    int     m_TickCount;
    int     m_LongSteps;
    int     m_CriticalSteps;
    int     m_UnusedCount;
} tInstanceInfo;

//#undef ROB_SECT_ARBITRARY
#ifdef ROB_SECT_ARBITRARY
static tInstanceInfo *m_Instances;
static int m_InstancesCount;
#else //ROB_SECT_ARBITRARY
static tInstanceInfo m_Instances[MAXNBBOTS];
#endif //ROB_SECT_ARBITRARY

////////////////////////////////////////////////////////////
// Utility functions
////////////////////////////////////////////////////////////

// name: getFileHandle
// Obtains the file handle for the robot XML file,
//  trying the installation path first, then
//  the global one, if the previous attempt failed.
// @param
// @return file handler for the robot XML file
void* GetFileHandle(const char *RobotName)
{
    void* RobotSettings = NULL;

    strncpy(BufName, RobotName, BUFSIZE);											// Save robot's name
    snprintf(BufPathDirRel, BUFSIZE, "drivers/%s",RobotName);						// relative to installation
    snprintf(BufPathXMLRel, BUFSIZE, "drivers/%s/%s.xml", RobotName, RobotName);		// relative to installation

    // Test local installation path
    snprintf(BufPathXML, BUFSIZE, "%s%s", GetLocalDir(), RobPathXMLRel);
    snprintf(BufPathDir, BUFSIZE, "%s%s", GetLocalDir(), RobPathDirRel);
    RobotSettings = GfParmReadFile(RobPathXML, GFPARM_RMODE_STD );

    if (!RobotSettings)
    {
        // If not found, use global installation path
        snprintf(BufPathXML, BUFSIZE, "%s%s", GetDataDir(), RobPathXMLRel);
        snprintf(BufPathDir, BUFSIZE, "%s%s", GetDataDir(), RobPathDirRel);
        RobotSettings = GfParmReadFile(RobPathXML, GFPARM_RMODE_STD );
    }

    return RobotSettings;
}

////////////////////////////////////////////////////////////
// Set parameters
////////////////////////////////////////////////////////////
void SetParameters(int N, char const* DefaultCarType)
{
    NBBOTS = N;
    Driver::NBBOTS = N;                                    // Used nbr of cars
    Driver::MyBotName = BufName;                           // Name of this bot
    Driver::ROBOT_DIR = BufPathDir;                        // Path to dll
    Driver::SECT_PRIV = "private";                         // Private section
    Driver::DEFAULTCARTYPE  = DefaultCarType;              // Default car type
};

////////////////////////////////////////////////////////////
// Schismatic entry point for simplix
////////////////////////////////////////////////////////////
void SetupUSR()
{
    m_RobotType = RTYPE_USR;
    SetParameters(NBBOTS, "car1-trb1");
    Driver::UseWingControl = true;
};

////////////////////////////////////////////////////////////
// Schismatic entry point for usr_mpa1
////////////////////////////////////////////////////////////
void SetupUSR_mpa1()
{
    m_RobotType = RTYPE_USR_MPA1;
    SetParameters(NBBOTS, "mpa1-murasama");
};

////////////////////////////////////////////////////////////
// Schismatic entry point for usr_mpa11
////////////////////////////////////////////////////////////
void SetupUSR_mpa11()
{
    m_RobotType = RTYPE_USR_MPA11;
    SetParameters(NBBOTS, "mpa11-murasama");
};

////////////////////////////////////////////////////////////
// Schismatic entry point for usr_mpa12
////////////////////////////////////////////////////////////
void SetupUSR_mpa12()
{
    m_RobotType = RTYPE_USR_MPA12;
    SetParameters(NBBOTS, "mpa12-murasama");
};

////////////////////////////////////////////////////////////
// Schismatic entry point for usr_trb1
////////////////////////////////////////////////////////////
void SetupUSR_trb1()
{
    m_RobotType = RTYPE_USR_TRB1;
    SetParameters(NBBOTS, "car1-trb1");
};

////////////////////////////////////////////////////////////
// Schismatic entry point for usr_sc
////////////////////////////////////////////////////////////
void SetupUSR_sc()
{
    m_RobotType = RTYPE_USR_SC;
    SetParameters(NBBOTS, "sc-cavallo-360");
};

////////////////////////////////////////////////////////////
// Schismatic entry point for usr_srw
////////////////////////////////////////////////////////////
void SetupUSR_srw()
{
    m_RobotType = RTYPE_USR_SRW;
    Driver::RobotType = m_RobotType;
    SetParameters(NBBOTS, "srw-sector-p4");
    Driver::UseWingControl = true;
};

////////////////////////////////////////////////////////////
// Schismatic entry point for usr_36GP
////////////////////////////////////////////////////////////
void SetupUSR_36GP()
{
    m_RobotType = RTYPE_USR_36GP;
    SetParameters(NBBOTS, "36GP-alfa12c");
};

////////////////////////////////////////////////////////////
// Schismatic entry point for usr_ls1
////////////////////////////////////////////////////////////
void SetupUSR_ls1()
{
    m_RobotType = RTYPE_USR_LS1;
    SetParameters(NBBOTS, "ls1-archer-r9");
};

////////////////////////////////////////////////////////////
// Schismatic entry point for usr_ls2
////////////////////////////////////////////////////////////
void SetupUSR_ls2()
{
    m_RobotType = RTYPE_USR_LS2;
    SetParameters(NBBOTS, "ls2-bavaria-g3gtr");
};

////////////////////////////////////////////////////////////
// Schismatic entry point for usr_MP5
////////////////////////////////////////////////////////////
void SetupUSR_mp5()
{
    m_RobotType = RTYPE_USR_MP5;
    SetParameters(NBBOTS, "mp5");
};

////////////////////////////////////////////////////////////
// Schismatic entry point for usr_lp1
////////////////////////////////////////////////////////////
void SetupUSR_lp1()
{
    m_RobotType = RTYPE_USR_LP1;
    SetParameters(NBBOTS, "lp1-vieringe-vr8");
};

////////////////////////////////////////////////////////////
// Schismatic entry point for usr_ref
////////////////////////////////////////////////////////////
void SetupUSR_ref()
{
    m_RobotType = RTYPE_USR_REF;
    SetParameters(NBBOTS, "ref-sector-p4");
    Driver::UseWingControl = true;
};

////////////////////////////////////////////////////////////
// Handle module entry for Speed Dreams Interface V1.00 (new fixed name scheme)
////////////////////////////////////////////////////////////
int moduleWelcomeV1_00
(const tModWelcomeIn* welcomeIn, tModWelcomeOut* welcomeOut)
{
    PLogUSR = GfLogger::instance("USR");
    LogUSR.debug("\n#Interface Version: %d.%d\n", welcomeIn->itfVerMajor, welcomeIn->itfVerMinor);

    // Get filehandle for robot's xml-file
    void* RobotSettings = GetFileHandle(welcomeIn->name);
    // Let's look what we have to provide here
    if (RobotSettings)
    {
        LogUSR.debug("#Robot name      : %s\n",RobName);
        LogUSR.debug("#Robot directory : %s\n",RobPathDirRel);
        LogUSR.debug("#Robot XML-file  : %s\n",RobPathXMLRel);

        char Buffer[BUFSIZE];
        char *Section = Buffer;

        // To get the number of drivers defined in the
        // robot team definition file we have to count
        // the number of sections within Robots/index!
        snprintf(Buffer, BUFSIZE, "%s/%s", ROB_SECT_ROBOTS, ROB_LIST_INDEX);
        NBBOTS = GfParmGetEltNb(RobotSettings, Buffer);
        LogUSR.debug("#Nbr of drivers  : %d\n", NBBOTS);

        DriverNames = (char *) calloc(NBBOTS,DRIVERLEN);
        DriverDescs = (char *) calloc(NBBOTS,DESCRPLEN);

        // Setup a path to the first driver section
        // assuming that it starts with the index 0
        snprintf(Buffer, BUFSIZE, "%s/%s/%d", ROB_SECT_ROBOTS, ROB_LIST_INDEX, 0);

        // Try to get first driver from index 0
        const char *DriverName = GfParmGetStr( RobotSettings, Section, (char *) ROB_ATTR_NAME, undefined);

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

        while (N < NBBOTS)
        {
            snprintf(Section, BUFSIZE, "%s/%s/%d", ROB_SECT_ROBOTS, ROB_LIST_INDEX, I + IndexOffset );
            const char *DriverName = GfParmGetStr( RobotSettings, Section, (char *) ROB_ATTR_NAME,undefined);

            if (strncmp(DriverName,undefined,strlen(undefined)) != 0)
            {   // This driver is defined in robot's xml-file
                strncpy(&DriverNames[I*DRIVERLEN], DriverName, DRIVERLEN-1);
                const char *DriverDesc = GfParmGetStr(RobotSettings, Section, (char *) ROB_ATTR_DESC, defaultBotDesc[I]);
                strncpy(&DriverDescs[I*DESCRPLEN], DriverDesc, DESCRPLEN-1);

                LogUSR.debug("#Driver %d: %s (%s)\n",I,DriverName, DriverDesc);

                N++;
            }
            else
            {
                // There is an index skipped in the robots team definition file
                // Therefore we have to get additional memory to store the data
                M++;

                DriverNames = (char *) realloc(DriverNames, (NBBOTS+M)*DRIVERLEN);
                memset(&DriverNames[I*DRIVERLEN], 0, DRIVERLEN);

                DriverDescs = (char *) realloc(DriverDescs,(NBBOTS+M)*DESCRPLEN);
                memset(&DriverDescs[I*DESCRPLEN], 0, DESCRPLEN);

                LogUSR.debug("#Driver %d: %s (%s)\n", I, &DriverNames[I*DRIVERLEN], &DriverDescs[I*DESCRPLEN]);
            }

            I++;
        }

        GfParmReleaseHandle(RobotSettings);
    }
    else
    {
        // Handle error here
        LogUSR.debug("#Robot XML-Path not found: (%s) or (%s) %s\n\n", GetLocalDir(), GetDataDir(), RobPathXMLRel);

        NBBOTS = 0;
        // But this is not considered a real failure of moduleWelcome !
    }

    // Handle additional settings for wellknown identities
    if (strncmp(RobName,"usr_trb1", strlen("usr_trb1")) == 0)
        SetupUSR_trb1();
    else if (strncmp(RobName,"usr_sc", strlen("usr_sc")) == 0)
        SetupUSR_sc();
    else if (strncmp(RobName,"usr_srw", strlen("usr_srw")) == 0)
        SetupUSR_srw();
    else if (strncmp(RobName,"usr_36GP", strlen("usr_36GP")) == 0)
        SetupUSR_36GP();
    else if (strncmp(RobName,"usr_mpa1", strlen("usr_mpa1")) == 0)
        SetupUSR_mpa1();
    else if (strncmp(RobName,"usr_mpa11", strlen("usr_mpa11")) == 0)
        SetupUSR_mpa11();
    else if (strncmp(RobName,"usr_mpa12", strlen("usr_mpa12")) == 0)
        SetupUSR_mpa12();
    else if (strncmp(RobName,"usr_ls1", strlen("usr_ls1")) == 0)
        SetupUSR_ls1();
    else if (strncmp(RobName,"usr_ls2", strlen("usr_ls2")) == 0)
        SetupUSR_ls2();
    else if (strncmp(RobName,"usr_mp5", strlen("usr_mp5")) == 0)
        SetupUSR_mp5();
    else if (strncmp(RobName,"usr_lp1", strlen("usr_lp1")) == 0)
        SetupUSR_lp1();
    else if (strncmp(RobName,"usr_ref", strlen("usr_ref")) == 0)
        SetupUSR_ref();
    else
        SetupUSR();

    // Set max nb of interfaces to return.
    welcomeOut->maxNbItf = NBBOTS;

    return 0;
}

////////////////////////////////////////////////////////////
// Module entry point (new fixed name scheme).
// Extended for use with schismatic robots and checked interface versions
////////////////////////////////////////////////////////////
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

    LogUSR.debug("\n#Unhandled Interface Version: %d.%d\n",	welcomeIn->itfVerMajor, welcomeIn->itfVerMinor);

    welcomeOut->maxNbItf = 0;

    return -1;
}

////////////////////////////////////////////////////////////
// Module entry point (new fixed name scheme).
// Tells TORCS, who we are, how we want to be called and
// what we are able to do.
////////////////////////////////////////////////////////////
extern "C" int moduleInitialize(tModInfo *ModInfo)
{
    LogUSR.debug("\n#Initialize from %s ...\n", RobPathXML);
    LogUSR.debug("#NBBOTS: %d (of %d)\n", NBBOTS, MAXNBBOTS);

#ifdef ROB_SECT_ARBITRARY
    // Clear all structures.
    memset(ModInfo, 0, (NBBOTS+1)*sizeof(tModInfo));

    int I;
    for (I = 0; I < Driver::NBBOTS; I++)
    {
        ModInfo[I].name = &DriverNames[I*DRIVERLEN]; // Tell customisable name
        ModInfo[I].desc = &DriverDescs[I*DESCRPLEN]; // Tell customisable desc.
        ModInfo[I].fctInit = initFuncPt;             // Common used functions
        ModInfo[I].gfId = ROB_IDENT;                 // Robot identity
        ModInfo[I].index = I+IndexOffset;            // Drivers index
    }

    ModInfo[NBBOTS].name = RobName;
    ModInfo[NBBOTS].desc = RobName;
    ModInfo[NBBOTS].fctInit = initFuncPt;
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

    LogUSR.debug("# ... Initialized\n\n");

    return 0;
}

////////////////////////////////////////////////////////////
// Module exit point (new fixed name scheme).
////////////////////////////////////////////////////////////
extern "C" int moduleTerminate()
{
    LogUSR.debug("#Terminated %s\n\n", RobName);

    if (DriverNames)
        free(DriverNames);

    DriverNames = NULL;

    if (DriverDescs)
        free(DriverDescs);

    DriverDescs = NULL;

    return 0;
}

////////////////////////////////////////////////////////////
// Module entry point (Torcs backward compatibility scheme).
////////////////////////////////////////////////////////////
int usrEntryPoint(tModInfo *ModInfo, void *RobotSettings)
{
    LogUSR.debug("\n#Torcs backward compatibility scheme used\n");
    NBBOTS = MIN(10,NBBOTS);

    memset(ModInfo, 0, NBBOTS*sizeof(tModInfo));
    DriverNames = (char *) calloc(10,DRIVERLEN);
    DriverDescs = (char *) calloc(10,DESCRPLEN);
    memset(DriverNames, 0, 10*DRIVERLEN);
    memset(DriverDescs, 0, 10*DESCRPLEN);

    char SectionBuf[BUFSIZE];
    char *Section = SectionBuf;

    snprintf( SectionBuf, BUFSIZE, "%s/%s/%d", ROB_SECT_ROBOTS, ROB_LIST_INDEX, 0);

    int I;
    for (I = 0; I < NBBOTS; I++)
    {
        snprintf( SectionBuf, BUFSIZE, "%s/%s/%d", ROB_SECT_ROBOTS, ROB_LIST_INDEX, I + IndexOffset );
        const char *DriverName = GfParmGetStr( RobotSettings, Section, (char *) ROB_ATTR_NAME, defaultBotName[I]);

        strncpy(&DriverNames[I*DRIVERLEN], DriverName, DRIVERLEN-1);
        const char *DriverDesc = GfParmGetStr( RobotSettings, Section, (char *) ROB_ATTR_DESC, defaultBotDesc[I]);

        strncpy(&DriverDescs[I*DESCRPLEN], DriverDesc, DESCRPLEN-1);
    }

    GfParmReleaseHandle(RobotSettings);

    return moduleInitialize(ModInfo);
}

////////////////////////////////////////////////////////////
// Module exit point (Torcs backward compatibility scheme).
////////////////////////////////////////////////////////////
extern "C" int usrShut()
{
    return moduleTerminate();
}

////////////////////////////////////////////////////////////
// TORCS: Initialization
//
// After clarification of the general calling (calling this func.),
// we tell TORCS our functions to provide the requested services:
////////////////////////////////////////////////////////////
static int initFuncPt(int Index, void *Pt)
{
    tRobotItf *Itf = (tRobotItf *)Pt;              // Get typed pointer

    Itf->rbNewTrack = initTrack;                   // Store function pointers
    Itf->rbNewRace  = newRace;
    Itf->rbDrive    = drive;
    Itf->rbPitCmd   = pitCmd;
    Itf->rbEndRace  = endRace;
    Itf->rbShutdown = shutdown;
    Itf->index      = Index;                       // Store index

#ifdef ROB_SECT_ARBITRARY
    int xx;
    tInstanceInfo *copy;

    //Make sure enough data is allocated
    if (m_InstancesCount <= Index-IndexOffset)
    {
        copy = new tInstanceInfo[Index-IndexOffset+1];
        for (xx = 0; xx < m_InstancesCount; ++xx)
            copy[xx] = m_Instances[xx];

        for (xx = m_InstancesCount; xx < Index-IndexOffset+1; ++xx)
            copy[xx].m_Robot = NULL;

        if (m_InstancesCount > 0)
            delete []m_Instances;

        m_Instances = copy;
        m_InstancesCount = Index-IndexOffset+1;
    }
#endif

    void* RobotSettings =	GetFileHandle(Driver::MyBotName);

    m_Instances[Index-IndexOffset].m_Robot = new Driver(Index-IndexOffset);
    m_Instances[Index-IndexOffset].m_Robot->SetBotName( RobotSettings, &DriverNames[(Index-IndexOffset)*DRIVERLEN]);

    if (m_RobotType == RTYPE_USR)
    {
        LogUSR.debug("#RobotType == RTYPE_USR\n");
    }
    else if (m_RobotType == RTYPE_USR_TRB1)
    {
        LogUSR.debug("#RobotType == RTYPE_USR_TRB1\n");
    }
    else if (m_RobotType == RTYPE_USR_SC)
    {
        LogUSR.debug("#RobotType == RTYPE_USR_SC\n");
    }
    else if (m_RobotType == RTYPE_USR_SRW)
    {
        LogUSR.debug("#RobotType == RTYPE_USR_SRW\n");
    }
    else if (m_RobotType == RTYPE_USR_36GP)
    {
        LogUSR.debug("#RobotType == RTYPE_USR_36GP\n");
    }
    else if (m_RobotType == RTYPE_USR_MPA1)
    {
        LogUSR.debug("#RobotType == RTYPE_USR_MPA1\n");
    }
    else if (m_RobotType == RTYPE_USR_MPA11)
    {
        LogUSR.debug("#RobotType == RTYPE_USR_MPA11\n");
    }
    else if (m_RobotType == RTYPE_USR_MPA12)
    {
        LogUSR.debug("#RobotType == RTYPE_USR_MPA12\n");
    }
    else if (m_RobotType == RTYPE_USR_LS1)
    {
        LogUSR.debug("#RobotType == RTYPE_USR_LS1\n");
    }
    else if (m_RobotType == RTYPE_USR_LS2)
    {
        LogUSR.debug("#RobotType == RTYPE_USR_LS2\n");
    }
    else if (m_RobotType == RTYPE_USR_MP5)
    {
        LogUSR.debug("#RobotType == RTYPE_USR_MP5\n");
    }
    else if (m_RobotType == RTYPE_USR_LP1)
    {
        LogUSR.debug("#RobotType == RTYPE_USR_LP1\n");
    }
    else if (m_RobotType == RTYPE_USR_REF)
    {
        LogUSR.debug("#RobotType == RTYPE_USR_REF\n");
    }

    GfParmReleaseHandle(RobotSettings);

    return 0;
}

////////////////////////////////////////////////////////////
// TORCS: New track
////////////////////////////////////////////////////////////
static void initTrack(int Index, tTrack* Track, void *CarHandle, void **CarParmHandle, tSituation *S)
{
    // Init common used data
    //m_Instances[Index-IndexOffset].m_Robot->SetCommonData( &gCommonData, cRobotType );
    m_Instances[Index-IndexOffset].m_Robot->initTrack( Track, CarHandle, CarParmHandle, S);
}


////////////////////////////////////////////////////////////
// TORCS: New Race starts
////////////////////////////////////////////////////////////
static void newRace(int Index, tCarElt* Car, tSituation *S)
{
    RtInitTimer(); // Check existance of Performance Counter Hardware

    m_Instances[Index-IndexOffset].m_Ticks = 0.0;               // Initialize counters
    m_Instances[Index-IndexOffset].m_MinTicks = FLT_MAX;        // and time data
    m_Instances[Index-IndexOffset].m_MaxTicks = 0.0;
    m_Instances[Index-IndexOffset].m_TickCount = 0;
    m_Instances[Index-IndexOffset].m_LongSteps = 0;
    m_Instances[Index-IndexOffset].m_CriticalSteps = 0;
    m_Instances[Index-IndexOffset].m_UnusedCount = 0;

    m_Instances[Index-IndexOffset].m_Robot->newRace(Car, S);
    m_Instances[Index-IndexOffset].m_Robot->CurrSimTime = -10.0;
}

////////////////////////////////////////////////////////////
// TORCS-Callback: Drive
//
// Attention: This procedure is called very frequent and fast in succession!
// Therefore we don't throw debug messages here!
// To find basic bugs, it may be usefull to do it anyhow!
////////////////////////////////////////////////////////////
static void drive(int Index, tCarElt* Car, tSituation *S)
{
    //LogUSR.debug("#>>> TDriver::Drive\n");
    if (m_Instances[Index-IndexOffset].m_Robot->CurrSimTime < S->currentTime)
        //  if (cInstances[Index-IndexOffset].cRobot->CurrSimTime + 0.03 < S->currentTime)
    {
        //LogUSR.debug("#Drive\n");
        double StartTimeStamp = RtTimeStamp();

        m_Instances[Index-IndexOffset].m_Robot->CurrSimTime = S->currentTime;
        //m_Instances[Index-IndexOffset].m_Robot->update(S);    // Update info about opp.
        /*if (m_Instances[Index-IndexOffset].m_Robot->IsStuck())    // Check if we are stuck
            m_Instances[Index-IndexOffset].m_Robot->Unstuck();      //   Unstuck
        else  */                                       // or
            m_Instances[Index-IndexOffset].m_Robot->drive(S);        //   Drive

        double Duration = RtDuration(StartTimeStamp);

        if (m_Instances[Index-IndexOffset].m_TickCount > 0)       // Collect used time
        {
            if (Duration > 1.0)
                m_Instances[Index-IndexOffset].m_LongSteps++;
            if (Duration > 2.0)
                m_Instances[Index-IndexOffset].m_CriticalSteps++;
            if (m_Instances[Index-IndexOffset].m_MinTicks > Duration)
                m_Instances[Index-IndexOffset].m_MinTicks = Duration;
            if (m_Instances[Index-IndexOffset].m_MaxTicks < Duration)
                m_Instances[Index-IndexOffset].m_MaxTicks = Duration;
        }
        m_Instances[Index-IndexOffset].m_TickCount++;
        m_Instances[Index-IndexOffset].m_Ticks += Duration;
    }
    else
    {
        //LogUSR.debug("#DriveLast\n");
        m_Instances[Index-IndexOffset].m_UnusedCount++;
        //m_Instances[Index-IndexOffset].m_Robot->DriveLast();      // Use last drive commands
    }
    //LogUSR.debug("#<<< TDriver::Drive\n");
}

////////////////////////////////////////////////////////////
// TORCS: Pitstop (Car is in pit!)
////////////////////////////////////////////////////////////
static int pitCmd(int Index, tCarElt* Car, tSituation *S)
{
    // Dummy: use parameters
    if ((Index < 0) || (Car == NULL) || (S == NULL))
        LogUSR.debug("PitCmd\n");

    return m_Instances[Index-IndexOffset].m_Robot->pitCommand(S);
}

////////////////////////////////////////////////////////////
// TORCS: Race ended
////////////////////////////////////////////////////////////
static void endRace(int Index, tCarElt *Car, tSituation *S)
{
    // Dummy: use parameters
    if ((Index < 0) || (Car == NULL) || (S == NULL))
        Index = 0;

    LogUSR.debug("EndRace\n");
    m_Instances[Index-IndexOffset].m_Robot->endRace(S);
}

////////////////////////////////////////////////////////////
// TORCS: Cleanup
////////////////////////////////////////////////////////////
static void shutdown(int Index)
{
#ifdef ROB_SECT_ARBITRARY
    int count;
    int xx;
    tInstanceInfo *copy;
#endif //ROB_SECT_ARBITRARY

    LogUSR.debug("\n\n#Clock\n");
    LogUSR.debug("#Total Time used: %g sec\n", m_Instances[Index-IndexOffset].m_Ticks/1000.0);
    LogUSR.debug("#Min   Time used: %g msec\n", m_Instances[Index-IndexOffset].m_MinTicks);
    LogUSR.debug("#Max   Time used: %g msec\n", m_Instances[Index-IndexOffset].m_MaxTicks);
    LogUSR.debug("#Mean  Time used: %g msec\n", m_Instances[Index-IndexOffset].m_Ticks/m_Instances[Index-IndexOffset].m_TickCount);
    LogUSR.debug("#Long Time Steps: %d\n", m_Instances[Index-IndexOffset].m_LongSteps);
    LogUSR.debug("#Critical Steps : %d\n", m_Instances[Index-IndexOffset].m_CriticalSteps);
    LogUSR.debug("#Unused Steps   : %d\n", m_Instances[Index-IndexOffset].m_UnusedCount);
    LogUSR.debug("\n");
    LogUSR.debug("\n");

    m_Instances[Index-IndexOffset].m_Robot->shutdown();
    delete m_Instances[Index-IndexOffset].m_Robot;
    m_Instances[Index-IndexOffset].m_Robot = NULL;

#ifdef ROB_SECT_ARBITRARY
    //Check if this was the highest index
    if (m_InstancesCount == Index-IndexOffset+1)
    {
        //Now make the cInstances array smaller
        //Count the number of robots which are still in the array
        count = 0;
        for (xx = 0; xx < Index-IndexOffset+1; ++xx)
        {
            if (m_Instances[xx].m_Robot)
                count = xx+1;
        }

        if (count>0)
        {
            //We have robots left: make a new array
            copy = new tInstanceInfo[count];
            for (xx = 0; xx < count; ++xx)
                copy[xx] = m_Instances[xx];
        }
        else
        {
            copy = NULL;
        }
        delete []m_Instances;
        m_Instances = copy;
        m_InstancesCount = count;
    }
#endif //ROB_SECT_ARBITRARY
}

////////////////////////////////////////////////////////////
// Module entry point (Torcs backward compatibility scheme).
////////////////////////////////////////////////////////////
extern "C" int usr(tModInfo *ModInfo)
{
    void *RobotSettings = GetFileHandle("usr");
    if (!RobotSettings)
        return -1;

    SetParameters(1, "car1-trb1");

    return usrEntryPoint(ModInfo, RobotSettings);
}
