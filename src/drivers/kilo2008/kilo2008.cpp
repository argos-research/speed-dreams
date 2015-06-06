/*
 *      kilo2008.cpp
 *      
 *      Copyright 2009 kilo aka Gabor Kmetyko <kg.kilo@gmail.com>
 *      Based on work by Bernhard Wymann and Andrew Sumner.
 *      
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *      
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *      
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *      MA 02110-1301, USA.
 * 
 *      $Id: kilo2008.cpp 5322 2013-03-16 10:28:12Z pouillot $
 * 
 */

#ifdef _WIN32
#include <windows.h>
#endif

#include <robot.h>  // ROB_IDENT
#include <string>
#include <sstream>
#include <vector>
#include <utility>
#include "src/drivers/kilo2008/kdriver.h"

using ::std::string;
using ::std::stringstream;
using ::std::vector;
using ::std::pair;

// TORCS interface
static void initTrack(int index, tTrack* track, void *carHandle,
                        void **carParmHandle, tSituation *s);
static void newRace(int index, tCarElt* car, tSituation *s);
static void drive(int index, tCarElt* car, tSituation *s);
static int pitcmd(int index, tCarElt* car, tSituation *s);
static void shutdown(int index);
static int InitFuncPt(int index, void *pt);
static void endRace(int index, tCarElt *car, tSituation *s);

// SD interface
static const int MAXNBBOTS = 20;
static const string defaultBotName[MAXNBBOTS] = {  // NOLINT(runtime/string)
  "kilo 1",  "kilo 2",  "kilo 3",  "kilo 4",  "kilo 5",
  "kilo 6",  "kilo 7",  "kilo 8",  "kilo 9",  "kilo 10",
  "kilo 11", "kilo 12", "kilo 13", "kilo 14", "kilo 15",
  "kilo 16", "kilo 17", "kilo 18", "kilo 19", "kilo 20"
};

static const string defaultBotDesc[MAXNBBOTS] = {  // NOLINT(runtime/string)
  "kilo 1",  "kilo 2",  "kilo 3",  "kilo 4",  "kilo 5",
  "kilo 6",  "kilo 7",  "kilo 8",  "kilo 9",  "kilo 10",
  "kilo 11", "kilo 12", "kilo 13", "kilo 14", "kilo 15",
  "kilo 16", "kilo 17", "kilo 18", "kilo 19", "kilo 20"
};

// Drivers info: pair(first:Name, second:Desc)
static vector< pair<string, string> > Drivers;
static KDriver *driver[MAXNBBOTS];  // Array of drivers

// Number of drivers defined in robot's xml-file
static int NBBOTS = 0;      // Still unknown
static string nameBuffer;   // Robot's name // NOLINT(runtime/string)
static string pathBuffer;   // Robot's xml-filename // NOLINT(runtime/string)

// Save start index offset from robot's xml file
static int indexOffset = 0;
// Marker for undefined drivers to be able to comment out drivers
// in the robot's xml-file between others, not only at the end of the list
const char *sUndefined = "undefined";
stringstream ssBuf;  // common use



////////////////////////////////
// Utility
////////////////////////////////

// Schismatic init
void setupKilo2008() {
  // Add kilo2008 specific initialization here
}  // setupKilo2008


// Set robots's name and xml file pathname
static void setRobotName(const string name) {
  ssBuf.str(string());
  ssBuf << "drivers/" << name << "/" << name << ".xml";
  nameBuffer = name;
  pathBuffer = ssBuf.str();
}  // setRobotName


////////////////////////////////////////////////////////////////
// SD Interface (new, fixed name scheme, from Andrew's USR code)
////////////////////////////////////////////////////////////////

// Module entry point (new fixed name scheme).
// Extended for use with schismatic robots
extern "C" int moduleWelcome(const tModWelcomeIn* welcomeIn,
                              tModWelcomeOut* welcomeOut) {
  // Save module name and loadDir, and determine module XML file pathname.
  setRobotName(welcomeIn->name);

  // Filehandle for robot's xml-file
  void *pRobotSettings = GfParmReadFile(pathBuffer.c_str(), GFPARM_RMODE_STD);

  if (pRobotSettings) {  // robot settings XML could be read
    NBBOTS = 0;
    ssBuf.str(string());
    ssBuf << ROB_SECT_ROBOTS << "/" << ROB_LIST_INDEX << "/" << 0;

    // Try to get first driver from index 0
    const string sDriverName = GfParmGetStrNC(pRobotSettings,
                                    ssBuf.str().c_str(),
                                    ROB_ATTR_NAME,
                                    const_cast<char*>(sUndefined));

    // Check whether index 0 is used as start index
    if (sDriverName != sUndefined) {
      // Teams xml file uses index 0, 1, ..., N - 1
      indexOffset = 0;
    } else {
      // Teams xml file uses index 1, 2, ..., N
      indexOffset = 1;
    }

    // Loop over all possible drivers, clear all buffers,
    // save defined driver names and descriptions.
    Drivers.clear();
    for (int i = indexOffset; i < MAXNBBOTS + indexOffset; ++i) {
      ssBuf.str(string());  // Clear buffer
      ssBuf << ROB_SECT_ROBOTS << "/"
              << ROB_LIST_INDEX << "/"
              << i;

      string sDriverName = GfParmGetStr(pRobotSettings, ssBuf.str().c_str(),
                                          ROB_ATTR_NAME, sUndefined);

      if (sDriverName != sUndefined) {
        // This driver is defined in robot's xml-file
        string sDriverDesc = GfParmGetStr(pRobotSettings, ssBuf.str().c_str(),
                                    ROB_ATTR_DESC, defaultBotDesc[i].c_str());
        Drivers.push_back(make_pair(sDriverName, sDriverDesc));
        ++NBBOTS;
      }  // if driver is defined
    }  // for i
  } else {  // if robot settings XML could not be read
    // For schismatic robots NBBOTS is unknown! Handle error here
    NBBOTS = 0;
    // But this is not considered a real failure of moduleWelcome !
  }  // if pRobotSettings

  setupKilo2008();

  // Set max nb of interfaces to return.
  welcomeOut->maxNbItf = NBBOTS;

  return 0;
}  // moduleWelcome


// Module entry point (new fixed name scheme).
extern "C" int moduleInitialize(tModInfo *modInfo) {
  // Clear all structures.
  memset(modInfo, 0, NBBOTS * sizeof(tModInfo));
  for (int i = 0; i < NBBOTS; i++) {
    modInfo[i].name = Drivers[i].first.c_str();
    modInfo[i].desc = Drivers[i].second.c_str();
    modInfo[i].fctInit = InitFuncPt;       // Init function.
    modInfo[i].gfId    = ROB_IDENT;        // Supported framework version.
    modInfo[i].index   = i + indexOffset;  // Indices from robot's xml-file.
  }  // for i

  return 0;
}  // moduleInitialize


// Module exit point (new fixed name scheme).
extern "C" int moduleTerminate() {
  return 0;
}  // moduleTerminate




////////////////////////////////////////////////////////////////
// TORCS backward compatibility scheme, from Andrew's USR code
////////////////////////////////////////////////////////////////

// Module entry point
extern "C" int kilo2008(tModInfo *modInfo) {
  NBBOTS = 10;
  Drivers.clear();
  pathBuffer = "drivers/kilo2008/kilo2008.xml";
  nameBuffer = "kilo2008";

  // Filehandle for robot's xml-file
  void *pRobotSettings = GfParmReadFile(pathBuffer.c_str(), GFPARM_RMODE_STD);

  if (pRobotSettings) {  // Let's look what we have to provide here
    ssBuf.str(string());
    ssBuf << ROB_SECT_ROBOTS << "/" << ROB_LIST_INDEX << "/" << 0;

    for (int i = 0; i < NBBOTS; i++) {
      string sDriverName = GfParmGetStr(pRobotSettings, ssBuf.str().c_str(),
                                    ROB_ATTR_NAME, defaultBotName[i].c_str());
      string sDriverDesc = GfParmGetStr(pRobotSettings, ssBuf.str().c_str(),
                                    ROB_ATTR_DESC, defaultBotDesc[i].c_str());
      Drivers.push_back(make_pair(sDriverName, sDriverDesc));
    }  // for i
  }  // if RobotSettings

  return moduleInitialize(modInfo);
}  // kilo2008


// Module exit point (TORCS backward compatibility scheme).
extern "C" int kilo2008Shut() {
  return moduleTerminate();
}  // kilo2008Shut


// Module interface initialization.
static int InitFuncPt(int index, void *pt) {
  tRobotItf *itf = static_cast<tRobotItf *>(pt);

  // Create robot instance for index.
  driver[index - 1] = new KDriver(index);
  driver[index - 1]->bot = "kilo2008";
  itf->rbNewTrack = initTrack;    // Give the robot the track view called.
  itf->rbNewRace  = newRace;      // Start a new race.
  itf->rbDrive    = drive;        // Drive during race.
  itf->rbPitCmd   = pitcmd;       // Pit commands.
  itf->rbEndRace  = endRace;      // End of the current race.
  itf->rbShutdown = shutdown;     // Called before the module is unloaded.
  itf->index      = index;        // Index used if multiple interfaces.
  return 0;
}  // InitFuncPt


// Called for every track change or new race.
static void initTrack(int index, tTrack* track, void *carHandle,
                      void **carParmHandle, tSituation *s) {
  driver[index-1]->initTrack(track, carHandle, carParmHandle, s);
}


// Start a new race.
static void newRace(int index, tCarElt* car, tSituation *s) {
  driver[index-1]->newRace(car, s);
}


// Drive during race.
static void drive(int index, tCarElt* car, tSituation *s) {
  driver[index-1]->drive(s);
}


// Pitstop callback.
static int pitcmd(int index, tCarElt* car, tSituation *s) {
  return driver[index-1]->pitCommand(s);
}


// End of the current race.
static void endRace(int index, tCarElt *car, tSituation *s) {
  driver[index-1]->endRace(s);
}


// Called before the module is unloaded.
static void shutdown(int index) {
  delete driver[index-1];
}
