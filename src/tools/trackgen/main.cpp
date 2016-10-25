/***************************************************************************

    file                 : trackgen.cpp
    created              : Sat Dec 23 09:27:43 CET 2000
    copyright            : (C) 2000 by Eric Espie
    email                : torcs@free.fr
    version              : $Id: main.cpp 5349 2013-03-23 17:59:22Z pouillot $

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
    		
    @author	<a href=mailto:torcs@free.fr>Eric Espie</a>
    @version	$Id: main.cpp 5349 2013-03-23 17:59:22Z pouillot $
*/
#include <cstdio>
#include <ctype.h>

#include <sstream>
#include <fstream>

#ifndef WIN32
#include <unistd.h>
#endif

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifndef WIN32
#include <getopt.h>
#endif

#include <plib/ul.h>
#include <plib/ssg.h>
#include <SDL.h>

#ifdef WIN32
#ifndef HAVE_CONFIG_H
#define HAVE_CONFIG_H
#endif
#endif

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <tgf.hpp>
#include <portability.h>
#include <itrackloader.h>

#include "ac3d.h"
#include "easymesh.h"
#include "objects.h"
#include "elevation.h"
#include "trackgen.h"

float	GridStep = 40.0;
float	TrackStep = 5.0;
float	Margin = 100.0;
float	ExtHeight = 5.0;

int	HeightSteps = 30;

int	Bump = 0;
int	UseBorder = 1;

char		*OutputFileName;
char		*TrackName;
char		*TrackCategory;

void		*TrackHandle;
void		*CfgHandle;

tTrack		*Track;
ITrackLoader*	PiTrackLoader;

int		TrackOnly;
int		JustCalculate;
int		MergeAll;
int		MergeTerrain;

int		ExportSUMO;

static char	buf[1024];
static char	buf2[1024];
static char	trackdef[1024];

char		*OutTrackName;
char		*OutMeshName;

tModList	*modlist = NULL;

int		DoSaveElevation;
char		*ElevationFile;

class Application : public GfApplication
{
public:

  //! Constructor.
  Application();

  //! Initialization.
  virtual void initialize(bool bLoggingEnabled, int argc = 0, char **argv = 0);

  //! Parse the command line options.
  // TODO: Move to the GfApplication way of parsing options ?
  bool parseOptions();
	
  void generate();
};

//! Constructor.
Application::Application()
  : GfApplication("TrackGen", "1.5.2.1", "Terrain generator for tracks")
{
}

void Application::initialize(bool bLoggingEnabled, int argc, char **argv)
{
  // Base initialization first.
  GfApplication::initialize(bLoggingEnabled, argc, argv);
	
  // Specific options.
  registerOption("c", "category", /* nHasValue = */ true);
  registerOption("n", "name", /* nHasValue = */ true);
  registerOption("b", "bump", /* nHasValue = */ false);
  registerOption("B", "noborder", /* nHasValue = */ false);
  registerOption("a", "all", /* nHasValue = */ false);
  registerOption("z", "calc", /* nHasValue = */ false);
  registerOption("s", "split", /* nHasValue = */ false);
  registerOption("S", "splitall", /* nHasValue = */ false);
  registerOption("E", "saveelev", /* nHasValue = */ true);
  registerOption("H", "height4", /* nHasValue = */ true);
  registerOption("e", "export", /* nHasValue =*/ false);

  // Help on specific options.
  addOptionsHelpSyntaxLine("-c|--category <cat> -n|--name <name> [-b|bump] [-B|--noborder]");
  addOptionsHelpSyntaxLine("[-a|--all] [-z|--calc] [-s|split] [-S|splitall]");
  addOptionsHelpSyntaxLine("[-E|--saveelev <#ef> [-H|height4 <#hs>]]");
  addOptionsHelpSyntaxLine("[-e|--export]");
	
  addOptionsHelpExplainLine("<cat>    : track category (road, speedway, dirt...)");
  addOptionsHelpExplainLine("<name>   : track name");
  addOptionsHelpExplainLine("bump     : draw bump track");
  addOptionsHelpExplainLine("noborder : don't use terrain border "
			    "(relief supplied int clockwise, ext CC)");
  addOptionsHelpExplainLine("all      : draw all (default is track only)");
  addOptionsHelpExplainLine("calc     : only calculate track parameters and exit");
  addOptionsHelpExplainLine("export   : export track for usage with SUMO");
  addOptionsHelpExplainLine("split    : split the track and the terrain");
  addOptionsHelpExplainLine("splitall : split all");
  addOptionsHelpExplainLine("<#ef>    : # of the elevation file to save");
  addOptionsHelpExplainLine("  0: all elevation files");
  addOptionsHelpExplainLine("  1: elevation file of terrain + track");
  addOptionsHelpExplainLine("  2: elevation file of terrain with track white");
  addOptionsHelpExplainLine("  3: track only");
  addOptionsHelpExplainLine("  4: track elevations with height steps");
  addOptionsHelpExplainLine("<#hs> : nb of height steps for 4th elevation file [30]");
}

// Parse the command line options.
bool Application::parseOptions()
{
  // Parse command line for registered options, and interpret standard ones.
  if (!GfApplication::parseOptions())
    return false;

  // Then interpret the specific ones.
  TrackName = NULL;
  TrackCategory = NULL;
  TrackOnly = 1;
  JustCalculate = 0;
  ExportSUMO = 0;
  MergeAll = 1;
  MergeTerrain = 1;
  DoSaveElevation = -1;

  std::list<Option>::const_iterator itOpt;
  for (itOpt = _lstOptions.begin(); itOpt != _lstOptions.end(); itOpt++)
    {
      // Not found in the command line => ignore / default value.
      if (!itOpt->bFound)
	continue;
		
      if (itOpt->strLongName == "all")
        {
	  TrackOnly = 0;
	}
      else if (itOpt->strLongName == "calc")
	{
	  JustCalculate = 1;
	}
      else if (itOpt->strLongName == "export")
	{
	  ExportSUMO = 1;
	}
      else if (itOpt->strLongName == "bump")
	{
	  Bump = 1;
	}
      else if (itOpt->strLongName == "split")
	{
	  MergeAll = 0;
	  MergeTerrain = 1;
	}
      else if (itOpt->strLongName == "splitall")
	{
	  MergeAll = 0;
	  MergeTerrain = 0;
	}
      else if (itOpt->strLongName == "noborder")
	{
	  UseBorder = 0;
	}
      else if (itOpt->strLongName == "name")
	{
	  TrackName = strdup(itOpt->strValue.c_str());
	}
      else if (itOpt->strLongName == "saveelev")
	{
	  DoSaveElevation = strtol(itOpt->strValue.c_str(), NULL, 0);
	  TrackOnly = 0;
	}
      else if (itOpt->strLongName == "category")
	{
	  TrackCategory = strdup(itOpt->strValue.c_str());
	}
      else if (itOpt->strLongName == "steps4")
	{
	  HeightSteps = strtol(itOpt->strValue.c_str(), NULL, 0);
	}
    }

  if (!TrackName || !TrackCategory)
    {
      printUsage("No track name or category specified");
      return false;
    }

  return true;
}

void Application::generate()
{
  const char *extName;
  FILE *outfd = NULL;

  // Get the trackgen paramaters.
  sprintf(buf, "%s", CFG_FILE);
  CfgHandle = GfParmReadFile(buf, GFPARM_RMODE_STD | GFPARM_RMODE_CREAT);

  // Load and initialize the track loader module.
  GfLogInfo("Loading Track Loader ...\n");
  std::ostringstream ossModLibName;
  ossModLibName << GfLibDir() << "modules/track/" << "trackv1" << '.' << DLLEXT;
  GfModule* pmodTrkLoader = GfModule::load(ossModLibName.str());

  // Check that it implements ITrackLoader.
  ITrackLoader* PiTrackLoader = 0;
  if (pmodTrkLoader)
    PiTrackLoader = pmodTrkLoader->getInterface<ITrackLoader>();
  if (!PiTrackLoader)
    return;

  // This is the track definition.
  sprintf(trackdef, "%stracks/%s/%s/%s.xml", GfDataDir(), TrackCategory, TrackName, TrackName);
  TrackHandle = GfParmReadFile(trackdef, GFPARM_RMODE_STD);
  if (!TrackHandle) {
    fprintf(stderr, "Cannot find %s\n", trackdef);
    ::exit(1);
  }

  // Build the track structure with graphic extensions.
  Track = PiTrackLoader->load(trackdef, true);

  if (ExportSUMO) {
    std::fstream fh;
    char fnb[1024];
    snprintf(fnb, sizeof(fnb), "%stracks/%s/%s/%s.csv", GfDataDir(), TrackCategory, TrackName, TrackName);
    fh.open(fnb, std::ios::out);

    trackSeg* cur = Track->seg;
    while(cur->next != Track->seg) {
      fh << cur->vertex[0].x << "," << cur->vertex[0].y << ",";
      fh << cur->vertex[1].x << "," << cur->vertex[1].y << "\n";
      cur = cur->next;
    }
    fh.close();
    return;
  }

  if (!JustCalculate) {
    // Get the output file radix.
    sprintf(buf2, "%stracks/%s/%s/%s", GfDataDir(), Track->category, Track->internalname, Track->internalname);
    OutputFileName = strdup(buf2);

    // Number of groups for the complete track.
    if (TrackOnly) {
      sprintf(buf2, "%s.ac", OutputFileName);
      // Track.
      outfd = Ac3dOpen(buf2, 1);
    } else if (MergeAll) {
      sprintf(buf2, "%s.ac", OutputFileName);
      // track + terrain + objects.
      outfd = Ac3dOpen(buf2, 2 + GetObjectsNb(TrackHandle));
    }

    // Main Track.
    if (Bump) {
      extName = "trk-bump";
    } else {
      extName = "trk";
    }

    sprintf(buf2, "%s-%s.ac", OutputFileName, extName);
    OutTrackName = strdup(buf2);
  }

  if (JustCalculate){
    CalculateTrack(Track, TrackHandle, Bump);
    return;
  }

  GenerateTrack(Track, TrackHandle, OutTrackName, outfd, Bump);

  if (TrackOnly) {
    return;
  }

  // Terrain.
  if (MergeTerrain && !MergeAll) {
    sprintf(buf2, "%s.ac", OutputFileName);
    /* terrain + objects  */
    outfd = Ac3dOpen(buf2, 1 + GetObjectsNb(TrackHandle));
  }

  extName = "msh";
  sprintf(buf2, "%s-%s.ac", OutputFileName, extName);
  OutMeshName = strdup(buf2);

  GenerateTerrain(Track, TrackHandle, OutMeshName, outfd, DoSaveElevation);

  if (DoSaveElevation != -1) {
    if (outfd) {
      Ac3dClose(outfd);
    }
    switch (DoSaveElevation) {
    case 0:
    case 1:
      sprintf(buf2, "%s.ac", OutputFileName);
      sprintf(buf, "%s-elv.png", OutputFileName);
      SaveElevation(Track, TrackHandle, buf, buf2, 1);
      if (DoSaveElevation) {
	break;
      }
    case 2:
      sprintf(buf, "%s-elv2.png", OutputFileName);
      SaveElevation(Track, TrackHandle, buf, OutMeshName, 1);
      if (DoSaveElevation) {
	break;
      }
    case 3:
      sprintf(buf, "%s-elv3.png", OutputFileName);
      SaveElevation(Track, TrackHandle, buf, OutMeshName, 0);
      if (DoSaveElevation) {
	break;
      }
    case 4:
      sprintf(buf, "%s-elv4.png", OutputFileName);
      SaveElevation(Track, TrackHandle, buf, OutTrackName, 2);
      break;
    }
    return;
  }

  GenerateObjects(Track, TrackHandle, CfgHandle, outfd, OutMeshName);
}


int main(int argc, char **argv)
{
  // Create and initialize the application
  Application app;
  app.initialize(/*bLoggingEnabled=*/true, argc, argv);
	
  // Parse the command line options
  if (!app.parseOptions())
    return 1;

  // If "data dir" specified in any way, cd to it.
  if(chdir(GfDataDir()))
    {
      GfLogError("Could not start %s : failed to cd to the datadir '%s' (%s)\n",
		 app.name().c_str(), GfDataDir(), strerror(errno));
      return 1;
    }
	
  // Do the requested job.
  app.generate();
	
  // That's all.
  return 0;
}
