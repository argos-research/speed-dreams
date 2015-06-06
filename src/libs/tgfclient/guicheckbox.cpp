/***************************************************************************

    file                 : guiedit.cpp
	created              : Nov 23 2009
    copyright            : (C) 2000 by Brian Gavin

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <cstdlib>
#include <cstring>

#include <SDL.h>

#include "tgfclient.h"

#include "gui.h"
#include "guifont.h"


void
gfuiCheckboxInit(void)
{
}

static void
gfuiChecked(void *idv)
{
	GfuiCheckboxSetChecked(GfuiScreen, (long)idv, false);
	GfuiUnSelectCurrent();

    tGfuiObject* object = gfuiGetObject(GfuiScreen, (long)idv);
    if (!object)
		return;

	tGfuiCheckbox* checkbox = &(object->u.checkbox);

	if (checkbox->onChange)
		checkbox->onChange(checkbox->pInfo);

}

static void
gfuiUnchecked(void *idv)
{
	GfuiCheckboxSetChecked(GfuiScreen, (long)idv, true);
	GfuiUnSelectCurrent();

    tGfuiObject* object = gfuiGetObject(GfuiScreen, (long)idv);
    if (!object)
		return;

	tGfuiCheckbox* checkbox = &(object->u.checkbox);

	checkbox = &(object->u.checkbox);

	if (checkbox->onChange)
		checkbox->onChange(checkbox->pInfo);

}

int
GfuiCheckboxCreate(void *scr, int font, int x, int y, int imagewidth, int imageheight,
				   const char *pszText, bool bChecked,
				   void* userData, tfuiCheckboxCallback onChange,
				   void *userDataOnFocus, tfuiCallback onFocus, tfuiCallback onFocusLost)
{
    tGfuiCheckbox	*checkbox;
    tGfuiObject		*object;
    tGfuiScreen		*screen = (tGfuiScreen*)scr;

    object = (tGfuiObject*)calloc(1, sizeof(tGfuiObject));
	object->widget = GFUI_CHECKBOX;
	object->focusMode = GFUI_FOCUS_NONE;
    object->id = screen->curId++;
    object->visible = 1;

	checkbox = &(object->u.checkbox);
	checkbox->onChange = onChange;
	checkbox->pInfo = new tCheckBoxInfo;
	checkbox->pInfo->bChecked = bChecked;
	checkbox->pInfo->userData = userData;
	checkbox->scr = scr;

	// Initialize the checked and unchecked button children.
	// Warning: All the images are supposed to be the same size.
	// TODO: Make graphic properties XML-customizable (images, ...)
	// Note: We avoid sharing the same userDataOnFocus among multiple controls
	//       (otherwise multiple frees at release time ...).
	checkbox->checkId =
		GfuiGrButtonCreate(scr, "data/img/checked.png", "data/img/checked.png",
						   "data/img/checked.png", "data/img/checked.png",
						   x, y, imagewidth, imageheight, GFUI_MIRROR_NONE, false, GFUI_MOUSE_UP,
						   (void*)(long)(object->id), gfuiChecked,
						   userDataOnFocus, onFocus, onFocusLost);

	checkbox->uncheckId =
		GfuiGrButtonCreate(scr, "data/img/unchecked.png", "data/img/unchecked.png",
						   "data/img/unchecked.png", "data/img/unchecked.png",
						   x, y, imagewidth, imageheight, GFUI_MIRROR_NONE, false, GFUI_MOUSE_UP,
						   (void*)(long)(object->id), gfuiUnchecked, 0, 0, 0);

	// Compute total height (text or buttons)
	tGfuiGrButton* pCheckedBut = &(gfuiGetObject(scr, checkbox->checkId)->u.grbutton);
	int height = gfuiFont[font]->getHeight();
	if (height < pCheckedBut->height)
		height = pCheckedBut->height;

	// Fix button y coordinate if text is higher than the buttons
	else
	{
		tGfuiGrButton* pUncheckedBut = &(gfuiGetObject(scr, checkbox->uncheckId)->u.grbutton);
		pCheckedBut->y = pUncheckedBut->y =
			y + (gfuiFont[font]->getHeight() - pCheckedBut->height) / 2;
	}

	int width = imagewidth + 5 + gfuiFont[font]->getWidth(pszText);

	// Bounding box
	object->xmin = x;
	object->xmax = x + width;
	object->ymin = y;
	object->ymax = y + height;

	// Initialize the label child (beware of y if the buttons are higher than the text).
	static const int hPadding = 5;
	const int xl = x + imagewidth + hPadding;
	int yl = y;
	if (height > gfuiFont[font]->getHeight())
		yl += (height -  gfuiFont[font]->getHeight()) / 2;

	checkbox->labelId =
		GfuiLabelCreate(scr, pszText, font, xl, yl, 0, GFUI_ALIGN_HL, strlen(pszText));

    gfuiAddObject(screen, object);

	GfuiCheckboxSetChecked(scr, object->id, bChecked);

    return object->id;
}



void
gfuiDrawCheckbox(tGfuiObject *obj)
{
	//Do nothing because children already draw themselves
}

void
GfuiCheckboxSetText(void* scr, int id, const char *text)
{
    tGfuiObject* object = gfuiGetObject(scr, id);
    if (!object || object->widget != GFUI_CHECKBOX)
		return;

	tGfuiCheckbox* checkbox = &(object->u.checkbox);

	GfuiLabelSetText(scr, checkbox->labelId, text);
}

void
GfuiCheckboxSetChecked(void* scr, int id, bool bChecked)
{
    tGfuiObject* object = gfuiGetObject(scr, id);
    if (!object || object->widget != GFUI_CHECKBOX)
		return;

	tGfuiCheckbox* checkbox = &(object->u.checkbox);

	checkbox->pInfo->bChecked = bChecked;
	GfuiVisibilitySet(scr, checkbox->checkId, bChecked);
	GfuiVisibilitySet(scr, checkbox->uncheckId, !bChecked);
}

void
GfuiCheckboxSetTextColor(void *scr, int id, const GfuiColor& color)
{
    tGfuiObject* object = gfuiGetObject(scr, id);
    if (!object || object->widget != GFUI_CHECKBOX)
		return;

	tGfuiCheckbox* checkbox = &(object->u.checkbox);

	GfuiLabelSetColor(scr, checkbox->labelId, color.toFloatRGBA());
}

void
gfuiReleaseCheckbox(tGfuiObject *obj)
{
    free(obj);
}
