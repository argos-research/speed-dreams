/***************************************************************************

    file                 : usr.cpp
    created              : Wed Jan 8 18:31:16 CET 2003
    copyright            : (C) 2002-2004 Bernhard Wymann
    email                : berniw@bluewin.ch
    version              : $Id: usr.cpp 5631 2013-07-26 21:32:55Z torcs-ng $

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
#include <robot.h>

#include <sys/socket.h>
#include <arpa/inet.h>

#include "json.hpp"
using json = nlohmann::json;

static tTrack	*curTrack;
static int sockfd;

#include "driver.h"

// Traditional TORCS Interface
static void initTrack(int index, tTrack* track, void *carHandle,
                      void **carParmHandle, tSituation *s);
static void newRace(int index, tCarElt* car, tSituation *s);
static void dataexchange(int index, tCarElt* car, tSituation *s);
static void drive(int index, tCarElt* car, tSituation *s);
static int  pitcmd(int index, tCarElt* car, tSituation *s);
static void shutdown(int index);
static void endRace(int index, tCarElt *car, tSituation *s);
static int  InitFuncPt(int index, void *pt);
extern "C" int usr(tModInfo *modInfo);

// Speed Dreams Interface
static const int BUFSIZE = 256;   // Buffer size for path/filename buffers
static const int MAXNBBOTS = 20;  // Set value to max capacity
static char const* defaultBotName[MAXNBBOTS] =
{
  "driver 1",  "driver 2",  "driver 3",  "driver 4",  "driver 5",
  "driver 6",  "driver 7",  "driver 8",  "driver 9",  "driver 10",
  "driver 11", "driver 12", "driver 13", "driver 14", "driver 15",
  "driver 16", "driver 17", "driver 18", "driver 19", "driver 20"
};

static char const* defaultBotDesc[MAXNBBOTS] =
{
  "driver 1",  "driver 2",  "driver 3",  "driver 4",  "driver 5",
  "driver 6",  "driver 7",  "driver 8",  "driver 9",  "driver 10",
  "driver 11", "driver 12", "driver 13", "driver 14", "driver 15",
  "driver 16", "driver 17", "driver 18", "driver 19", "driver 20"
};

// Max length of a drivername
static const int DRIVERLEN = 32;
// Buffer for driver's names defined in robot's xml-file
static char DriverNames[DRIVERLEN * MAXNBBOTS];
// Buffer for driver's descriptions defined in robot's xml-file
static char DriverDescs[DRIVERLEN * MAXNBBOTS];
// Buffer for car name defined in robot's xml-file
static char CarNames[DRIVERLEN * MAXNBBOTS];
// Array of drivers
static Driver *driver[MAXNBBOTS];

// Number of drivers defined in robot's xml-file
static int NBBOTS = 0;                           // Still unknown
// Robot's name
static char nameBuffer[BUFSIZE];                 // Buffer for robot's name
static const char* robot_name = nameBuffer;       // Pointer to robot's name
// Robot's xml-filename
static char pathBuffer[BUFSIZE];                 // Buffer for xml-filename
static const char* xml_path = pathBuffer;        // Pointer to xml-filename

// Save start index offset from robot's xml file
static int indexOffset = 0;
// Marker for undefined drivers to be able to comment out drivers
// in the robot's xml-file between others, not only at the end of the list
char undefined[] = "undefined";

static int robot_type;  //Decide if TRB, SC, GP36, LS or some other driver

// The "USR" logger instance
GfLogger* PLogUSR = 0;

////////////////////////////////////////////////////////////
// Utility functions
////////////////////////////////////////////////////////////

// Set robots's name
static void setRobotName(const char *name)
{
  strncpy(nameBuffer, name, BUFSIZE);
  GfOut("Robot Name: >%s<\n", robot_name);
}


// name: getFileHandle
// Obtains the file handle for the robot XML file,
//  trying the installation path first, then
//  the global one, if the previous attempt failed.
// @param
// @return file handler for the robot XML file
void* getFileHandle()
{
  // First we try to use the directories relative to the installation path
  snprintf(pathBuffer, BUFSIZE, "%sdrivers/%s/%s.xml",
            GetLocalDir(), robot_name, robot_name);

  // Test local installation path
  void *robot_settings = GfParmReadFile(xml_path, GFPARM_RMODE_STD);

  if (!robot_settings)
  {
    // If not found, use global installation path
    snprintf(pathBuffer, BUFSIZE, "%sdrivers/%s/%s.xml",
            GetDataDir(), robot_name, robot_name);
    robot_settings = GfParmReadFile(xml_path, GFPARM_RMODE_STD);
  }

  return robot_settings;
}


////////////////////////////////////////////////////////////
// Carset specific init functions
////////////////////////////////////////////////////////////

// Schismatic init for usr_trb1
void SetupUSR_trb1()
{
  // Add usr_trb1 specific initialization here
  robot_type = USR_TRB1;
};

// Schismatic init for usr_ls2
void SetupUSR_ls2()
{
  // Add usr_ls2 specific initialization here
  robot_type = USR_LS2;
};


// Schismatic init for usr_sc
void SetupUSR_sc()
{
  // Add usr_sc specific initialization here
  robot_type = USR_SC;
};


// Schismatic init for usr_ls1
void SetupUSR_ls1()
{
  // Add usr_ls1 specific initialization here
  robot_type = USR_LS1;
};


// Schismatic init for usr_36GP
void SetupUSR_36GP()
{
  // Add usr_36GP specific initialization here
  robot_type = USR_36GP;
};

// Schismatic init for usr_rs
void SetupUSR_rs()
{
  // Add usr_RS specific initialization here
  robot_type = USR_RS;
};

// Schismatic init for usr_lp1
void SetupUSR_lp1()
{
  // Add usr_LP1 specific initialization here
  robot_type = USR_LP1;
};

// Schismatic init for usr_mpa1
void SetupUSR_mpa1()
{
  // Add usr_mpa1 specific initialization here
  robot_type = USR_MPA1;
};


////////////////////////////////////////////////////////////
// Carset specific entry points (functions)
////////////////////////////////////////////////////////////

// Schismatic entry point for usr_trb1
extern "C" int usr_trb1(tModInfo *ModInfo)
{
  int ret = -1;
  setRobotName("usr_trb1");
  robot_type = USR_TRB1;
  void *robot_settings = getFileHandle();
  if (robot_settings)
  {
    ret = usr(ModInfo);
  }
  
  return ret;
}


// Schismatic entry point for usr_sc
extern "C" int usr_sc(tModInfo *ModInfo) {
  int ret = -1;
  setRobotName("usr_sc");
  robot_type = USR_SC;
  void *robot_settings = getFileHandle();
  if (robot_settings) {
    ret = usr(ModInfo);
  }
  
  return ret;
}


// Schismatic entry point for usr_ls2
extern "C" int usr_ls2(tModInfo *ModInfo)
{
  int ret = -1;
  setRobotName("usr_ls2");
  robot_type = USR_LS2;
  void *robot_settings = getFileHandle();
  if (robot_settings)
  {
    ret = usr(ModInfo);
  }
  
  return ret;
}

// Schismatic entry point for usr_ls1
extern "C" int usr_ls1(tModInfo *ModInfo) {
  int ret = -1;
  setRobotName("usr_ls1");
  robot_type = USR_LS1;
  void *robot_settings = getFileHandle();
  if (robot_settings) {
    ret = usr(ModInfo);
  }
  
  return ret;
}

// Schismatic entry point for usr_mpa1
extern "C" int usr_mpa1(tModInfo *ModInfo)
{
  int ret = -1;
  setRobotName("usr_mpa1");
  robot_type = USR_MPA1;
  void *robot_settings = getFileHandle();
  if (robot_settings)
  {
    ret = usr(ModInfo);
  }

  return ret;
}

// Schismatic entry point for usr_36GP
extern "C" int usr_36GP(tModInfo *ModInfo)
{
  int ret = -1;
  setRobotName("usr_36GP");
  robot_type = USR_36GP;
  void *robot_settings = getFileHandle();
  if (robot_settings)
  {
    ret = usr(ModInfo);
  }
  
  return ret;
}

// Schismatic entry point for usr_rs
extern "C" int usr_rs(tModInfo *ModInfo)
{
  int ret = -1;
  setRobotName("usr_rs");
  robot_type = USR_RS;
  void *robot_settings = getFileHandle();
  if (robot_settings)
  {
    ret = usr(ModInfo);
  }
  
  return ret;
}

// Schismatic entry point for usr_LP1
extern "C" int usr_lp1(tModInfo *ModInfo)
{
  int ret = -1;
  setRobotName("usr_lp1");
  robot_type = USR_LP1;
  void *robot_settings = getFileHandle();
  if (robot_settings)
  {
    ret = usr(ModInfo);
  }

  return ret;
}

////////////////////////////////////////////////////////////
// General entry point (new scheme)
////////////////////////////////////////////////////////////
// Module entry point (new fixed name scheme), step #1.
// Extended for use with schismatic robots
extern "C" int moduleWelcome(const tModWelcomeIn* welcomeIn,
                              tModWelcomeOut* welcomeOut)
{
  PLogUSR = GfLogger::instance("USR");
  LogUSR.debug("\n#Interface Version: %d.%d\n",
      welcomeIn->itfVerMajor,welcomeIn->itfVerMinor);
  // Save module name and loadDir, and determine module XML file pathname.
  setRobotName(welcomeIn->name);

  // Filehandle for robot's xml-file
  void *robot_settings = getFileHandle();
  GfOut("Robot XML-Path: %s\n\n", xml_path);

  // Let's look what we have to provide here
  if (robot_settings)
  {
    char section_buf[BUFSIZE];
    const char *section = section_buf;
    snprintf(section_buf, BUFSIZE, "%s/%s/%d",
              ROB_SECT_ROBOTS, ROB_LIST_INDEX, 0);

    // Try to get first driver from index 0
    const char *driver_name = GfParmGetStrNC(robot_settings, section,
              ROB_ATTR_NAME, undefined);

    // Check whether index 0 is used as start index
    if (strncmp(driver_name, undefined, strlen(undefined)) != 0)
    {
      // Teams xml file uses index 0, 1, ..., N - 1
      indexOffset = 0;
    }
    else
    {
      // Teams xml file uses index 1, 2, ..., N
      indexOffset = 1;
    }

    // Loop over all possible drivers, clear all buffers,
    // save defined driver names and desc.
    for (int i = 0; i < MAXNBBOTS; ++i)
    {
      // Clear buffers
      memset(&DriverNames[i * DRIVERLEN], 0, DRIVERLEN);
      memset(&DriverDescs[i * DRIVERLEN], 0, DRIVERLEN);
      memset(&CarNames[i * DRIVERLEN], 0, DRIVERLEN);

      snprintf(section_buf, BUFSIZE, "%s/%s/%d",
                ROB_SECT_ROBOTS, ROB_LIST_INDEX, i + indexOffset);
      const char *driver_name = GfParmGetStr(robot_settings, section,
                                            ROB_ATTR_NAME, undefined);

      if (strncmp(driver_name, undefined, strlen(undefined)) != 0)
      {
        // This driver is defined in robot's xml-file
        strncpy(&DriverNames[i * DRIVERLEN], driver_name, DRIVERLEN - 1);
        const char *driver_desc = GfParmGetStr(robot_settings, section,
                                            ROB_ATTR_DESC, defaultBotDesc[i]);
        strncpy(&DriverDescs[i * DRIVERLEN], driver_desc, DRIVERLEN - 1);

        const char *car_name =
                GfParmGetStr(robot_settings, section, ROB_ATTR_CAR, "nocar");
        strncpy(&CarNames[i * DRIVERLEN], car_name, DRIVERLEN - 1);

        NBBOTS = i + 1;
      }
    }
  }
  else
  {
    // For schismatic robots NBBOTS is unknown!
    // Handle error here
    NBBOTS = 0;
    // But this is not considered a real failure of moduleWelcome !
  }
  GfOut("NBBOTS: %d (of %d)\n", NBBOTS, MAXNBBOTS);

  if (strncmp(robot_name, "usr_trb1", strlen("usr_trb1")) == 0)
    SetupUSR_trb1();
  else if (strncmp(robot_name,"usr_sc", strlen("usr_sc")) == 0)
    SetupUSR_sc();
  else if (strncmp(robot_name,"usr_ls1", strlen("usr_ls1")) == 0)
    SetupUSR_ls1();
  else if (strncmp(robot_name,"usr_ls2", strlen("usr_ls2")) == 0)
    SetupUSR_ls2();
  else if (strncmp(robot_name,"usr_36GP", strlen("usr_36GP")) == 0)
    SetupUSR_36GP();
  else if (strncmp(robot_name,"usr_rs", strlen("usr_rs")) == 0)
    SetupUSR_rs();
  else if (strncmp(robot_name,"usr_lp1", strlen("usr_lp1")) == 0)
    SetupUSR_lp1();
  else if (strncmp(robot_name,"usr_mpa1", strlen("usr_mpa1")) == 0)
    SetupUSR_mpa1();


  // Set max nb of interfaces to return.
  welcomeOut->maxNbItf = NBBOTS;

  return 0;
}


// Module entry point (new fixed name scheme).
extern "C" int moduleInitialize(tModInfo *modInfo)
{
  GfOut("\n\nusr::moduleInitialize, from %s ...\n", xml_path);
  GfOut("NBBOTS: %d (of %d)\n", NBBOTS, MAXNBBOTS);

  // Clear all structures.
  memset(modInfo, 0, NBBOTS*sizeof(tModInfo));
  int i;

  for (i = 0; i < NBBOTS; ++i)
  {
    modInfo[i].name = &DriverNames[i * DRIVERLEN];
    modInfo[i].desc = &DriverDescs[i * DRIVERLEN];
    modInfo[i].fctInit = InitFuncPt;   // Init function.
    modInfo[i].gfId    = ROB_IDENT;    // Supported framework version.
    modInfo[i].index   = i + indexOffset;  // Indices depend on xml-file
  }

  GfOut("... Initialized %d from %s\n\n\n", i, xml_path);

  return 0;
}


// Module exit point (new fixed name scheme).
extern "C" int moduleTerminate()
{
    GfOut("Terminated usr\n");
    return 0;
}


// Module interface initialization.
static int InitFuncPt(int index, void *pt)
{
  tRobotItf *itf = reinterpret_cast<tRobotItf*>(pt);

  // Create robot instance for index.
  driver[index-indexOffset] = new Driver(index, robot_type);
  driver[index-indexOffset]->SetBotName(&CarNames[(index-indexOffset)*DRIVERLEN]);

  itf->rbNewTrack = initTrack;  // Give the robot the track view called.
  itf->rbNewRace  = newRace;    // Start a new race.
  itf->rbDrive    = drive;    // Drive during race.
  itf->rbPitCmd   = pitcmd;   // Pit commands.
  itf->rbEndRace  = endRace;    // End of the current race.
  itf->rbShutdown = shutdown;   // Called before the module is unloaded.
  itf->index      = index;    // Index used if multiple interfaces.

  return 0;
}


// Called for every track change or new race.
static void initTrack(int index, tTrack* track, void *carHandle,
                      void **carParmHandle, tSituation *s)
{
  curTrack = track;
  driver[index-indexOffset]->initTrack(track, carHandle, carParmHandle, s);
}

static void dataexchange(int index, tCarElt* car, tSituation *s) {
  //memset(&car->ctrl, 0, sizeof(tCarCtrl));

  /* calculate yaw in degrees
   * http://answers.ros.org/question/141366/convert-the-yaw-euler-angle-into-into-the-range-0-360/
   */
  double yaw = car->_yaw * 180.0 / M_PI;
  if(yaw < 0) yaw += 360.0;

  // json j;
  // j["veh0"] = {
  //   {"rpm", car->priv.enginerpm},
  //   {"speed", car->_speed_x},
  //   {"gear", car->priv.gear},
  //   {"pos", car->race.distRaced},
  //   {"trackLength", curTrack->length},
  //   {"angle", yaw},
  // };

  json j;
  j["veh0"] = {
    {"pos", car->race.distRaced},
    {"x", car->_pos_X},
    {"y", car->_pos_Y},
    {"z", car->_pos_Z},
    {"speed", car->_speed_x},
    {"gear", car->_gear},
    {"angle", yaw},
    {"trackLength", curTrack->length},
    {"rpm", car->priv.enginerpm},
    {"fsX", curTrack->seg[0].vertex[2].x},
    {"fsY", curTrack->seg[0].vertex[2].y}
  };

  write(sockfd, j.dump().c_str(), strlen(j.dump().c_str()) + 1);
}



// Start a new race.
static void newRace(int index, tCarElt* car, tSituation *s)
{
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(struct sockaddr_in));

  server_addr.sin_family = AF_INET;
  //server_addr.sin_addr.s_addr = inet_addr("131.159.208.114");
  server_addr.sin_addr.s_addr = inet_addr("10.0.2.208");
  server_addr.sin_port = htons(2000);

  connect(sockfd, (struct sockaddr*)&server_addr, sizeof(struct sockaddr_in));

  driver[index-indexOffset]->newRace(car, s);
}


// Drive during race.
static void drive(int index, tCarElt* car, tSituation *s)
{
  dataexchange(index, car, s);
  driver[index-indexOffset]->drive(s);
}


// Pitstop callback.
static int pitcmd(int index, tCarElt* car, tSituation *s) {
  return driver[index-indexOffset]->pitCommand(s);
}


// End of the current race.
static void endRace(int index, tCarElt *car, tSituation *s)
{
  driver[index-indexOffset]->endRace(s);
}


// Called before the module is unloaded.
static void shutdown(int index)
{
  driver[index-indexOffset]->shutdown();
  delete driver[index-indexOffset];
}


////////////////////////////////////////////////////////////
// Ye Olde Interface
////////////////////////////////////////////////////////////

// Module entry point (Torcs backward compatibility scheme).
extern "C" int usr(tModInfo *modInfo)
{
  NBBOTS = 10;
  memset(DriverNames, 0, NBBOTS * DRIVERLEN);
  memset(DriverDescs, 0, NBBOTS * DRIVERLEN);

  // Filehandle for robot's xml-file
  void *robot_settings = getFileHandle();

  // Let's look what we have to provide here
  if (robot_settings)
  {
    char SectionBuf[BUFSIZE];
    char *Section = SectionBuf;

    snprintf(SectionBuf, BUFSIZE, "%s/%s/%d",
              ROB_SECT_ROBOTS, ROB_LIST_INDEX, 0);

    for (int i = 0; i < NBBOTS; ++i)
    {
      const char *DriverName = GfParmGetStr(robot_settings, Section,
                              ROB_ATTR_NAME, defaultBotName[i]);
      strncpy(&DriverNames[i * DRIVERLEN], DriverName, DRIVERLEN - 1);
      const char *DriverDesc = GfParmGetStr(robot_settings, Section,
                              ROB_ATTR_DESC, defaultBotDesc[i]);
      strncpy(&DriverDescs[i * DRIVERLEN], DriverDesc, DRIVERLEN - 1);
    }
  }
  return moduleInitialize(modInfo);
}


// Module exit point (Torcs backward compatibility scheme).
extern "C" int usrShut()
{
  return moduleTerminate();
}
