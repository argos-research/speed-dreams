/***************************************************************************
                  trackselect.cpp -- interactive track selection
                             -------------------
    created              : Mon Aug 16 21:43:00 CEST 1999
    copyright            : (C) 1999 by Eric Espie
    email                : torcs@free.fr
    version              : $Id: trackselect.cpp 4478 2012-02-05 13:30:30Z pouillot $
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
    @ingroup    racemantools
    @author <a href=mailto:torcs@free.fr>Eric Espie</a>
    @version    $Id: trackselect.cpp 4478 2012-02-05 13:30:30Z pouillot $
*/

#include <sstream>
#include <iterator>
#include <iomanip>

#include <raceman.h>

#include <tgfclient.h>
#include <portability.h>

#include <racemanagers.h>
#include <race.h>
#include <tracks.h>

#include "racescreens.h"


// Screen handle.
static void *ScrHandle;

// Track select menu data.
static tRmTrackSelect *MenuData;

// The currently selected track.
GfTrack* PCurTrack;

// Menu controls.
static int PrevCategoryArrowId;
static int NextCategoryArrowId;
static int CategoryEditId;
static int PrevTrackArrowId;
static int NextTrackArrowId;
static int NameEditId;
static int OutlineImageId;
static int AuthorsLabelId;
static int LengthLabelId;
static int WidthLabelId;
static int DescLine1LabelId;
static int DescLine2LabelId;
static int MaxPitsLabelId;

// Max length of track description lines (loaded from menu properties).
static unsigned DescLinesMaxLen = 35;

/** 
 * rmtsWordWrap
 * 
 * Cuts the input string into two, according to the line length given.
 * 
 * @param   str Input string
 * @param   str1    First line should be placed in here
 * @param   str2    Second line should be placed in here (if needed)
 * @param   length  Line length limit where wrapping should occur
 */
static void
rmtsWordWrap(const std::string str, std::string &str1, std::string &str2, unsigned length)
{
	//istream_iterator iterates through the container
	//using whitespaces as delimiters, so it is an ideal tool
	//for cutting strings into separate words.
	std::istringstream istr(str);
	std::istream_iterator<std::string> it(istr);
	std::istream_iterator<std::string> end;

	//str1 + next word still below line length limit?
	while (it != end && (str1.size() + (*it).size()) < length) {
		str1 += *it;    //concatenate next word and a space to str1
		str1 += " ";    //as the iterator eats the whitespace...
		it++;
	}//while
	
	if (str.size() >= length)    //If input string was longer than required,
		str2 = str.substr(str1.size()); //put the rest in str2.
}//rmtsWordWrap

static void
rmtsUpdateTrackInfo(void)
{
	if (!PCurTrack)
		return;
	
	// Update GUI with track info.
	// 0) Track category and name.
	GfuiLabelSetText(ScrHandle, CategoryEditId, PCurTrack->getCategoryName().c_str());
	GfuiLabelSetText(ScrHandle, NameEditId, PCurTrack->getName().c_str());
	
	// 1) Track description, optionally wrapped in 2 lines
	std::string strDescLine1, strDescLine2;
	rmtsWordWrap(PCurTrack->getDescription(), strDescLine1, strDescLine2, DescLinesMaxLen);
	GfuiLabelSetText(ScrHandle, DescLine1LabelId, strDescLine1.c_str());
	GfuiLabelSetText(ScrHandle, DescLine2LabelId, strDescLine2.c_str());

	// 2) Authors
	GfuiLabelSetText(ScrHandle, AuthorsLabelId, PCurTrack->getAuthors().c_str());

	// 3) Width.
	std::ostringstream ossData;
	ossData << std::fixed << std::setprecision(0) << PCurTrack->getWidth() << " m";
	GfuiLabelSetText(ScrHandle, WidthLabelId, ossData.str().c_str());
	
	// 4) Length.
	ossData.str("");
	ossData << PCurTrack->getLength() << " m";
	GfuiLabelSetText(ScrHandle, LengthLabelId, ossData.str().c_str());

	// 5) Max number of pits slots.
	ossData.str("");
	if (PCurTrack->getMaxNumOfPitSlots())
		ossData << PCurTrack->getMaxNumOfPitSlots();
	else
		ossData << "None";
	GfuiLabelSetText(ScrHandle, MaxPitsLabelId, ossData.str().c_str());

	// 6) Outline image.
	GfuiStaticImageSet(ScrHandle, OutlineImageId, PCurTrack->getOutlineFile().c_str());

	// 7) Preview image (background).
	GfuiScreenAddBgImg(ScrHandle, PCurTrack->getPreviewFile().c_str());
}

static void
rmtsDeactivate(void *screen)
{
	GfuiScreenRelease(ScrHandle);

	if (screen) {
		GfuiScreenActivate(screen);
	}
}

static void
rmtsActivate(void * /* dummy */)
{
	GfLogTrace("Entering Track Select menu\n");

	// Disable track category combo-box arrows if only one category available
	// (Note: "Available" does not mean "usable", but this should be enough for releases
	//        where everything installed is supposed to be usable).
	if (GfTracks::self()->getCategoryIds().size() <= 1)
	{
		GfuiEnable(ScrHandle, PrevCategoryArrowId, GFUI_DISABLE);
		GfuiEnable(ScrHandle, NextCategoryArrowId, GFUI_DISABLE);
	}
	
	// Disable track combo-box arrows if only one track available in the current category.
	if (GfTracks::self()->getTracksInCategory(PCurTrack->getCategoryId()).size() <= 1)
	{
		GfuiEnable(ScrHandle, PrevTrackArrowId, GFUI_DISABLE);
		GfuiEnable(ScrHandle, NextTrackArrowId, GFUI_DISABLE);
	}

	// Update GUI (current track).
	rmtsUpdateTrackInfo();
}

/* Select next/previous track from currently selected track category */
static void
rmtsTrackPrevNext(void *vsel)
{
 	const int nSearchDir = (long)vsel > 0 ? +1 : -1;

	// Select next usable track in the current catergory in the requested direction.
	PCurTrack = GfTracks::self()->getFirstUsableTrack(PCurTrack->getCategoryId(),
													  PCurTrack->getId(), nSearchDir, true);

	// Update GUI
	rmtsUpdateTrackInfo();
}


/* Select next/previous track category */
static void
rmtsTrackCatPrevNext(void *vsel)
{
 	const int nSearchDir = (long)vsel > 0 ? +1 : -1;

	// Select first usable track in the next catergory in the requested direction.
	PCurTrack = GfTracks::self()->getFirstUsableTrack(PCurTrack->getCategoryId(),
													  nSearchDir, true);

	// Update GUI
	rmtsUpdateTrackInfo();
	
	// Disable track combo-box arrows if only one track available in this category.
	if (PCurTrack)
	{
		const int nEnableTrkChange =
			GfTracks::self()->getTracksInCategory(PCurTrack->getCategoryId()).size() > 1
			? GFUI_ENABLE : GFUI_DISABLE;
		GfuiEnable(ScrHandle, PrevTrackArrowId, nEnableTrkChange);
		GfuiEnable(ScrHandle, NextTrackArrowId, nEnableTrkChange);
	}
}


static void
rmtsSelect(void * /* dummy */)
{
	// Save currently selected track into the race manager.
	MenuData->pRace->getManager()->setEventTrack(0, PCurTrack);

	// Next screen.
	rmtsDeactivate(MenuData->nextScreen);
}


static void
rmtsAddKeys(void)
{
	GfuiAddKey(ScrHandle, GFUIK_RETURN, "Select Track", NULL, rmtsSelect, NULL);
	GfuiAddKey(ScrHandle, GFUIK_ESCAPE, "Cancel Selection", MenuData->prevScreen, rmtsDeactivate, NULL);
	GfuiAddKey(ScrHandle, GFUIK_LEFT, "Previous Track", (void*)-1, rmtsTrackPrevNext, NULL);
	GfuiAddKey(ScrHandle, GFUIK_RIGHT, "Next Track", (void*)+1, rmtsTrackPrevNext, NULL);
	GfuiAddKey(ScrHandle, GFUIK_F1, "Help", ScrHandle, GfuiHelpScreen, NULL);
	GfuiAddKey(ScrHandle, GFUIK_F12, "Screen-Shot", NULL, GfuiScreenShot, NULL);
	GfuiAddKey(ScrHandle, GFUIK_UP, "Previous Track Category", (void*)-1, rmtsTrackCatPrevNext, NULL);
	GfuiAddKey(ScrHandle, GFUIK_DOWN, "Next Track Category", (void*)+1, rmtsTrackCatPrevNext, NULL);
}


/** Interactive track selection
    @param  vs  Pointer on a tRmTrackSelect structure (cast to void *)
    @warning    The race manager's parameters are updated but not saved to the file.
    @ingroup    racemantools
 */
void
RmTrackSelect(void *vs)
{
	MenuData = (tRmTrackSelect*)vs;

	// Get currently selected track for the current race type
	// (or the first usable one in the selected category).
	PCurTrack = MenuData->pRace->getTrack();
	const std::string strReqTrackId = PCurTrack->getId();
	const std::string strReqTrackCatId = PCurTrack->getCategoryId();
	PCurTrack =
		GfTracks::self()->getFirstUsableTrack(PCurTrack->getCategoryId(), PCurTrack->getId());
	if (PCurTrack && PCurTrack->getId() != strReqTrackId)
		GfLogWarning("Could not find / use selected track %s (%s) ; using %s (%s)\n", 
					 strReqTrackId.c_str(), strReqTrackCatId.c_str(),
					 PCurTrack->getId().c_str(), PCurTrack->getCategoryId().c_str());

	// If not usable, try and get the first usable track going ahead in categories
	if (!PCurTrack)
	{
		PCurTrack = GfTracks::self()->getFirstUsableTrack(strReqTrackCatId, +1, true);
		if (PCurTrack)
			GfLogWarning("Could not find / use selected track %s and category %s unusable"
						 " ; using %s (%s)\n",
						 strReqTrackId.c_str(), strReqTrackCatId.c_str(),
						 PCurTrack->getId().c_str(), PCurTrack->getCategoryId().c_str());
	}
	
	// If no usable category/track found, ... return
	if (!PCurTrack)
	{
		GfLogError("No available track for any category ; quitting Track Select menu\n");
		return; // or exit(1) abruptly ?
	}

	// Create screen menu and controls.
	ScrHandle =
		GfuiScreenCreate((float*)NULL, NULL, rmtsActivate, NULL, (tfuiCallback)NULL, 1);

	void *hparmMenu = GfuiMenuLoad("trackselectmenu.xml");
	GfuiMenuCreateStaticControls( ScrHandle, hparmMenu);

	PrevCategoryArrowId =
		GfuiMenuCreateButtonControl(ScrHandle, hparmMenu, "trackcatleftarrow",(void*)-1, rmtsTrackCatPrevNext);
	NextCategoryArrowId =
		GfuiMenuCreateButtonControl(ScrHandle, hparmMenu, "trackcatrightarrow",(void*)1, rmtsTrackCatPrevNext);
	CategoryEditId = GfuiMenuCreateLabelControl(ScrHandle, hparmMenu, "trackcatlabel");

	PrevTrackArrowId =
		GfuiMenuCreateButtonControl(ScrHandle, hparmMenu, "trackleftarrow", (void*)-1, rmtsTrackPrevNext);
	NextTrackArrowId =
		GfuiMenuCreateButtonControl(ScrHandle, hparmMenu, "trackrightarrow", (void*)1, rmtsTrackPrevNext);
	NameEditId = GfuiMenuCreateLabelControl(ScrHandle, hparmMenu, "tracklabel");

	OutlineImageId = GfuiMenuCreateStaticImageControl(ScrHandle, hparmMenu, "outlineimage");

	GfuiMenuCreateButtonControl(ScrHandle, hparmMenu, "nextbutton", NULL, rmtsSelect);
	GfuiMenuCreateButtonControl(ScrHandle, hparmMenu, "backbutton", MenuData->prevScreen, rmtsDeactivate);

	DescLine1LabelId = GfuiMenuCreateLabelControl(ScrHandle, hparmMenu, "description1label");
	DescLine2LabelId = GfuiMenuCreateLabelControl(ScrHandle, hparmMenu, "description2label");
	LengthLabelId = GfuiMenuCreateLabelControl(ScrHandle, hparmMenu, "lengthlabel");
	WidthLabelId = GfuiMenuCreateLabelControl(ScrHandle, hparmMenu, "widthlabel");
	MaxPitsLabelId = GfuiMenuCreateLabelControl(ScrHandle, hparmMenu, "pitslabel");
	AuthorsLabelId = GfuiMenuCreateLabelControl(ScrHandle, hparmMenu, "authorslabel");

	// Load menu properties.
	DescLinesMaxLen = (unsigned)GfuiMenuGetNumProperty(hparmMenu, "nDescLinesMaxLen", 35);
	
	GfParmReleaseHandle(hparmMenu);

	// Keyboard shortcuts.
	rmtsAddKeys();

	// Let's go !
	GfuiScreenActivate(ScrHandle);
}
