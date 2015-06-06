/***************************************************************************

	file				 : creditsmenu.cpp
	created			  : Tue Mar 3 12:00:00 CEST 2009
	copyright			: (C) 2009 by Jean-Philippe Meuret
	web				  : speed-dreams.sourceforge.net

***************************************************************************/

/***************************************************************************
 *																		 *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or	 *
 *   (at your option) any later version.								   *
 *																		 *
 ***************************************************************************/

#include <cstring>
#include <map>
#include <vector>

#include <tgfclient.h>


// Description of a columns in the credits XML file
typedef struct
{
	// Displayed name.
	const char *name;

	// Display width (in screen pixels 640x400).
	int	width;

} tColumnDesc;


// Parameter structure for the page change callback.
typedef struct
{
	// Handle of the screen of the page that is about to be closed/released.
	void	*prevPageScrHdle;

	// Index of the starting chapter for the requested page.
	int		startChapterIndex;

	// Index of the starting record for the requested page.
	int		startRecordIndex;

} tPageChangeRequest;

// Previous and next parameters for the page change callback.
static tPageChangeRequest	NextPageRequest;
static tPageChangeRequest	PrevPageRequest;


// Handle of the screen to return when exiting from this credits pages.
static void* RetScrHdle = 0;

// Internal function declarations.
static void* creditsPageCreate(int startChapterIndex, int startRecordIndex);

// Page change callback.
static void creditsPageChange(void *vpcr)
{
	tPageChangeRequest *pcr = (tPageChangeRequest*)vpcr;

	// Get the screen handle of the currently activated page.
	void* prevPageScrHdle = pcr->prevPageScrHdle;

	// Create and activate requested page screen : it is now the currently activated page.
	GfuiScreenActivate(creditsPageCreate(pcr->startChapterIndex, pcr->startRecordIndex));

	// Release the previously activated page screen.
	GfuiScreenRelease(prevPageScrHdle);
}

// Create a credits page screen for given chapter and starting record.
static void* creditsPageCreate(int startChapterIndex, int startRecordIndex)
{
	static const unsigned maxBufSize = 256;
	static char	buf[maxBufSize];
	static char	buf2[maxBufSize];

	// Open and parse credits file
	sprintf(buf, "%s%s", GfDataDir(), "data/credits.xml");
	void* hparmCredits = GfParmReadFile(buf, GFPARM_RMODE_REREAD);
	if (!hparmCredits)
		return 0;

	// Get the number of chapters
	const int nChapters = (int)GfParmGetEltNb(hparmCredits, "chapters");
	if (startChapterIndex < 0 || startChapterIndex >= nChapters)
		return 0;

	// Get requested chapter info
	sprintf(buf, "chapters/%d", startChapterIndex);
	const char* chapName = GfParmGetStr(hparmCredits, buf, "name", "<no name>");

	sprintf(buf, "chapters/%d/records", startChapterIndex);
	const int nRecordsInChapter = (int)GfParmGetEltNb(hparmCredits, buf);
	if (startRecordIndex >= nRecordsInChapter)
		return 0;

	// Create screen, load menu XML descriptor and create static controls.
	void* hscrPage = GfuiScreenCreate();

	void *hmenu = GfuiMenuLoad("creditsmenu.xml");

	GfuiMenuCreateStaticControls(hscrPage, hmenu);

	// Create title label from chapter name
	const int titleId = GfuiMenuCreateLabelControl(hscrPage, hmenu, "subtitle");
	GfuiLabelSetText(hscrPage, titleId, chapName);

	// Get menu properties.
	const int nMaxLinesPerPage =
		(int)GfuiMenuGetNumProperty(hmenu, "nMaxLinesPerPage", 16);
	const int yBottom1stLine =
		(int)GfuiMenuGetNumProperty(hmenu, "yBottom1stLine", 400);
	const int xLeft1stCol = (int)GfuiMenuGetNumProperty(hmenu, "xLeft1stCol", 20);
	const int xRightLastCol = (int)GfuiMenuGetNumProperty(hmenu, "xRightLastCol", 620);
	const int xRecordLineShift = (int)GfuiMenuGetNumProperty(hmenu, "xRecordLineShift", 10);
	const int yLineShift = (int)GfuiMenuGetNumProperty(hmenu, "yLineShift", 17);
	const int yRecordShift = (int)GfuiMenuGetNumProperty(hmenu, "yRecordShift", 20);

	// Get columns info (names, width)
	// (each chapter record may need more than 1 screen line, given the column width sum ...)
	sprintf(buf, "chapters/%d/columns", startChapterIndex);
	const int nColsInChapter = (int)GfParmGetEltNb(hparmCredits, buf);
	if (nColsInChapter <= 0)
		return 0;

	std::map<const char*, tColumnDesc> columns;
	std::vector<const char*> orderedColumnIds;

	int x0 = xLeft1stCol;
	int x = x0;
	int y = yBottom1stLine;
	int nLinesPerRecord = 1;
	sprintf(buf, "chapters/%d/columns", startChapterIndex);
	GfParmListSeekFirst(hparmCredits, buf);
	do
	{
		tColumnDesc column;
		column.name = GfParmGetCurStr(hparmCredits, buf, "name", "<no name>");
		column.width = GfParmGetCurNum(hparmCredits, buf, "width", 0, 20);
		if (x >= xRightLastCol)
		{
			// We need 1 more screen line for the current credits record ?
			x0 += xRecordLineShift;
			x = x0;
			//y -= yLineShift;
			nLinesPerRecord++;
		}

		const char* colId = GfParmListGetCurEltName(hparmCredits, buf);
		// GfuiLabelCreate(hscrPage, column.name, GFUI_FONT_MEDIUM_C,
		// 				x, y, GFUI_ALIGN_HL, 0, colNameColor);
		x += column.width;
		orderedColumnIds.push_back(colId);
		columns.insert(std::pair<const char*, tColumnDesc>(colId, column));
	}
	while (GfParmListSeekNext(hparmCredits, buf) == 0);

	// Display each column of each record
	const int nMaxRecordsPerPage = nMaxLinesPerPage / nLinesPerRecord;
	if (startRecordIndex < 0)
		startRecordIndex = nMaxRecordsPerPage * ((nRecordsInChapter - 1) / nMaxRecordsPerPage);
	int nRecordInd = startRecordIndex;
	int nLines = 0;
	for (; nRecordInd < nRecordsInChapter && nRecordInd - startRecordIndex < nMaxRecordsPerPage;
		 nRecordInd++)
	{
		x0 = x = xLeft1stCol;
		sprintf(buf, "chapters/%d/records/%d", startChapterIndex, nRecordInd);
		std::vector<const char*>::const_iterator colIdIter;
		for (colIdIter = orderedColumnIds.begin(); colIdIter != orderedColumnIds.end(); colIdIter++)
		{
			const char* colValue = GfParmGetStr(hparmCredits, buf, *colIdIter, "");
			if (x >= xRightLastCol)
			{
				// We need 1 more screen line for the current credits record ?
				x0 += xRecordLineShift;
				x = x0;
				y -= yLineShift;
				nLines++;
			}
			sprintf(buf2, "%d.%s", startChapterIndex, *colIdIter);
			GfuiMenuCreateLabelControl(hscrPage, hmenu, buf2, true, // from template
									   colValue, x, y);
			x += columns[*colIdIter].width;
		}
		y -= yRecordShift;
	}

	// Close credits file
	GfParmReleaseHandle(hparmCredits);

	// Create "Previous page" button, and disable it if not the first page.
	const int nPrevButId =
		GfuiMenuCreateButtonControl(hscrPage, hmenu, "previous page arrow",
									(void*)&PrevPageRequest, creditsPageChange);
	if (startRecordIndex > 0 || startChapterIndex > 0)
	{
		PrevPageRequest.prevPageScrHdle = hscrPage;
		if (startRecordIndex > 0)
		{
			PrevPageRequest.startChapterIndex = startChapterIndex;
			PrevPageRequest.startRecordIndex  = startRecordIndex - nMaxRecordsPerPage;
		}
		else
		{
			PrevPageRequest.startChapterIndex = startChapterIndex - 1;
			PrevPageRequest.startRecordIndex  = -1;
		}
		GfuiAddKey(hscrPage, GFUIK_PAGEUP, "Previous page",
				   (void*)&PrevPageRequest, creditsPageChange, NULL);
	}
	else
	{
		GfuiEnable(hscrPage, nPrevButId, GFUI_DISABLE);
	}

	// Add "Continue" button (credits screen exit).
	GfuiMenuCreateButtonControl(hscrPage, hmenu, "back button",
								RetScrHdle, GfuiScreenReplace);

	// Add "Next page" button, and disable it  if not the last page.
	const int nNextButId =
		GfuiMenuCreateButtonControl(hscrPage, hmenu, "next page arrow",
									(void*)&NextPageRequest, creditsPageChange);
	if (nRecordInd < nRecordsInChapter || startChapterIndex + 1 < nChapters)
	{
		NextPageRequest.prevPageScrHdle = hscrPage;
		if (nRecordInd < nRecordsInChapter)
		{
			NextPageRequest.startChapterIndex = startChapterIndex;
			NextPageRequest.startRecordIndex  = startRecordIndex + nMaxRecordsPerPage;
		}
		else
		{
			NextPageRequest.startChapterIndex = startChapterIndex + 1;
			NextPageRequest.startRecordIndex  = 0;
		}
		GfuiAddKey(hscrPage, GFUIK_PAGEDOWN, "Next Page",
				   (void*)&NextPageRequest, creditsPageChange, NULL);
	}
	else
	{
		GfuiEnable(hscrPage, nNextButId, GFUI_DISABLE);
	}

	GfParmReleaseHandle(hmenu);

	// Add standard keyboard shortcuts.
	GfuiAddKey(hscrPage, GFUIK_ESCAPE, "Return to previous menu",
			   RetScrHdle, GfuiScreenReplace, NULL);
	GfuiAddKey(hscrPage, GFUIK_RETURN, "Return to previous menu",
			   RetScrHdle, GfuiScreenReplace, NULL);
	GfuiAddKey(hscrPage, GFUIK_F1, "Help",
			   hscrPage, GfuiHelpScreen, NULL);
	GfuiAddKey(hscrPage, GFUIK_F12, "Take a Screen Shot",
			   NULL, GfuiScreenShot, NULL);

	return hscrPage;
}

// Menu entry.
void CreditsMenuActivate(void *retScrHdle)
{
	// Store return screen handle.
	RetScrHdle = retScrHdle;

	// Create first page screen and return its handle.
	GfuiScreenActivate(creditsPageCreate(0, 0));
}
