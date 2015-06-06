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

#include "gui.h"


void
gfuiInitCombobox(void)
{
}

static void
gfuiLeftArrow(void *idv)
{
    tGfuiObject		*object;
    tGfuiCombobox	*combobox;

    object = gfuiGetObject(GfuiScreen, (long)idv);
    if (!object)
		return;

	combobox = &(object->u.combobox);

	if (combobox->pInfo->vecChoices.empty())
		return;

	if (combobox->pInfo->nPos > 0)
		combobox->pInfo->nPos--;
	else
		combobox->pInfo->nPos = combobox->pInfo->vecChoices.size() - 1;

	gfuiLabelSetText(&combobox->label,
					 combobox->pInfo->vecChoices[combobox->pInfo->nPos].c_str());

	if (combobox->onChange)
		combobox->onChange(combobox->pInfo);
}

static void
gfuiRightArrow(void *idv)
{
    tGfuiObject		*object;
    tGfuiCombobox	*combobox;

    object = gfuiGetObject(GfuiScreen, (long)idv);
    if (!object)
		return;

	combobox = &(object->u.combobox);

	if (combobox->pInfo->vecChoices.empty())
		return;

	if (combobox->pInfo->nPos < combobox->pInfo->vecChoices.size() - 1)
		combobox->pInfo->nPos++;
	else
		combobox->pInfo->nPos = 0;

	gfuiLabelSetText(&combobox->label,
					 combobox->pInfo->vecChoices[combobox->pInfo->nPos].c_str());

	if (combobox->onChange)
		combobox->onChange(combobox->pInfo);
}

/** Add combo-box like control to a screen.
    @ingroup	gui
    @param	scr		Screen
    @param	font	Font id
    @param	x		X position on screen (0 = left)
    @param	y		Y position on screen (0 = bottom)
    @param	width		Width on the screen
    @param	arrowsWidth		Width of the arrow buttons on the screen (0 = image width)
    @param	arrowsHeight	Height of the arrow buttons on the screen (0 = image height)
    @param	pszText	Text to display
    @param	maxlen	Maximum length of the dsiaplyed text
                    <br>0 for the text length.
    @param	fgColor	Pointer on static RGBA color array (0 => default)
    @param	fgFocusColor	Pointer on static RGBA focused color array (0 => fgColor)
    @param	userData	User data attached to the combo-box
    @param	onChange	Change callback function
    @param	userDataOnFocus	Parameter to the Focus (and lost) callback
    @param	onFocus		Focus callback function
    @param	onFocusLost	Focus Lost callback function
    @return	ComboBox Id
		<br>-1 Error
 */

int
GfuiComboboxCreate(void *scr, int font, int x, int y, int width,
				   int arrowsWidth, int arrowsHeight,
				   const char *pszText, int maxlen,
				   const float *fgColor, const float *fgFocusColor,
				   void *userData, tfuiComboboxCallback onChange,
				   void *userDataOnFocus, tfuiCallback onFocus, tfuiCallback onFocusLost)
{
    tGfuiCombobox	*combobox;
    tGfuiObject		*object;
    tGfuiScreen		*screen = (tGfuiScreen*)scr;

    object = (tGfuiObject*)calloc(1, sizeof(tGfuiObject));
	object->widget = GFUI_COMBOBOX;
	object->focusMode = GFUI_FOCUS_MOUSE_MOVE;
    object->id = screen->curId++;
    object->visible = 1;

	combobox = &(object->u.combobox);

    combobox->userDataOnFocus = userDataOnFocus;
    combobox->onFocus = onFocus;
    combobox->onFocusLost = onFocusLost;
	combobox->onChange = onChange;

    combobox->pInfo = new tComboBoxInfo;
	combobox->pInfo->nPos = 0;
	combobox->pInfo->userData = userData;
	combobox->scr = scr;

	// Initialize the left and right arrow button children.
	// Warning: All the arrow images are supposed to be the same size.
	// TODO: Make image files customizable.
	gfuiGrButtonInit(&combobox->leftButton,
					 "data/img/arrow-left-disabled.png", "data/img/arrow-left.png",
					 "data/img/arrow-left-focused.png", "data/img/arrow-left-pushed.png",
					 x, y, arrowsWidth, arrowsHeight, GFUI_MIRROR_NONE,
					 GFUI_MOUSE_UP,	 (void*)(long)(object->id), gfuiLeftArrow, 0, 0, 0);
	gfuiGrButtonInit(&combobox->rightButton,
					 "data/img/arrow-right-disabled.png", "data/img/arrow-right.png",
					 "data/img/arrow-right-focused.png", "data/img/arrow-right-pushed.png",
					 x + width - combobox->leftButton.width, y,
					 arrowsWidth, arrowsHeight, GFUI_MIRROR_NONE,
					 GFUI_MOUSE_UP, (void*)(long)(object->id), gfuiRightArrow, 0, 0, 0);

	// Compute total height (text or buttons)
	int height = gfuiFont[font]->getHeight();
	if (height < combobox->leftButton.height)
		height = combobox->leftButton.height;

	// Fix button y coordinate if text is higher than the buttons
	else
		combobox->leftButton.y = combobox->rightButton.y =
			y + (gfuiFont[font]->getHeight() - combobox->leftButton.height) / 2;

	// Bounding box
	object->xmin = x;
	object->xmax = x + width;
	object->ymin = y;
	object->ymax = y + height;

	// Initialize the label child (beware of y if the buttons are higher than the text).
	int yl = y;
	if (height > gfuiFont[font]->getHeight())
		yl += (height - gfuiFont[font]->getHeight()) / 2;

	gfuiLabelInit(&combobox->label, pszText, maxlen,
				  x + combobox->leftButton.width, yl,
				  width - 2 * combobox->leftButton.width, GFUI_ALIGN_HC,
				  font, 0, fgColor, 0, fgFocusColor, 0, 0, 0);

	// Add the combo control to the display list.
    gfuiAddObject(screen, object);

    return object->id;
}

void
gfuiDrawCombobox(tGfuiObject *obj)
{
	gfuiLabelDraw(&obj->u.combobox.label,
				  obj->focus ? obj->u.combobox.label.fgFocusColor : obj->u.combobox.label.fgColor);
	gfuiGrButtonDraw(&obj->u.combobox.leftButton, obj->state, obj->focus);
	gfuiGrButtonDraw(&obj->u.combobox.rightButton, obj->state, obj->focus);
}

static tGfuiCombobox*
gfuiGetCombobox(void *scr, int id)
{
    tGfuiObject* object = gfuiGetObject(scr, id);

    if (object && object->widget == GFUI_COMBOBOX)
		return &(object->u.combobox);

	return 0;
}

unsigned int
GfuiComboboxAddText(void *scr, int id, const char *text)
{
	unsigned int index = 0;
    tGfuiObject* object = gfuiGetObject(scr, id);

    if (object && object->widget == GFUI_COMBOBOX)
	{
		tGfuiCombobox* combo = &(object->u.combobox);
		combo->pInfo->vecChoices.push_back(text);
		index = combo->pInfo->vecChoices.size();
		gfuiLabelSetText(&combo->label,
						 combo->pInfo->vecChoices[combo->pInfo->nPos].c_str());
    }

	return index;
}

void
GfuiComboboxSetSelectedIndex(void *scr, int id, unsigned int index)
{
    tGfuiObject* object = gfuiGetObject(scr, id);

    if (object && object->widget == GFUI_COMBOBOX)
	{
		tGfuiCombobox* combo = &(object->u.combobox);
		if (index < combo->pInfo->vecChoices.size())
		{
			combo->pInfo->nPos = index;
			gfuiLabelSetText(&combo->label,
							 combo->pInfo->vecChoices[combo->pInfo->nPos].c_str());
		}
	}
}

void
GfuiComboboxSetTextColor(void *scr, int id, const GfuiColor& color)
{
    tGfuiObject* object = gfuiGetObject(scr, id);

    if (object && object->widget == GFUI_COMBOBOX)
	{
		tGfuiCombobox* combo = &(object->u.combobox);
		gfuiLabelSetColor(&combo->label, color.toFloatRGBA());
	}
}



void
GfuiComboboxSetPosition(void *scr, int id, unsigned int pos)
{
	tGfuiCombobox* combo = gfuiGetCombobox(scr, id);

    if (combo)
		combo->pInfo->nPos = pos;
}

unsigned int
GfuiComboboxGetPosition(void *scr, int id)
{
	unsigned int index = 0;

	tGfuiCombobox* combo = gfuiGetCombobox(scr, id);

    if (combo)
		index = combo->pInfo->nPos;

	return index;
}

const char*
GfuiComboboxGetText(void *scr, int id)
{
	const char* pszText = 0;

	tGfuiCombobox* combo = gfuiGetCombobox(scr, id);

    if (combo && combo->pInfo->nPos < combo->pInfo->vecChoices.size())
		pszText = combo->pInfo->vecChoices[combo->pInfo->nPos].c_str();

	return pszText;
}

unsigned
GfuiComboboxGetNumberOfChoices(void *scr, int id)
{
	unsigned nChoices = 0;

	tGfuiCombobox* combo = gfuiGetCombobox(scr, id);

    if (combo)
		nChoices = combo->pInfo->vecChoices.size();

	return nChoices;
}


void
GfuiComboboxClear(void *scr, int id)
{
    tGfuiObject* object = gfuiGetObject(scr, id);

    if (object && object->widget == GFUI_COMBOBOX)
	{
		tGfuiCombobox* combo = &(object->u.combobox);
		combo->pInfo->nPos = 0;
		combo->pInfo->vecChoices.clear();
		gfuiLabelSetText(&combo->label, "");
	}
}

/** Handles the button action.
    @ingroup	gui
    @param	action	action
 */
void
gfuiComboboxAction(int action)
{
	if (GfuiScreen->hasFocus->state == GFUI_DISABLE)
		return;

	tGfuiCombobox* combo = &(GfuiScreen->hasFocus->u.combobox);

	if (action == 2) { /* enter key */
		if (gfuiGrButtonMouseIn(&combo->leftButton)) {
			if (combo->leftButton.onPush)
				combo->leftButton.onPush(combo->leftButton.userDataOnPush);
		} else if (gfuiGrButtonMouseIn(&combo->rightButton)) {
			if (combo->rightButton.onPush)
				combo->rightButton.onPush(combo->rightButton.userDataOnPush);
		}
	} else if (action == 1) { /* mouse up */
		if (gfuiGrButtonMouseIn(&combo->leftButton)) {
			combo->leftButton.state = GFUI_BTN_RELEASED;
			if (combo->leftButton.mouseBehaviour == GFUI_MOUSE_UP && combo->leftButton.onPush)
				combo->leftButton.onPush(combo->leftButton.userDataOnPush);
		} else if (gfuiGrButtonMouseIn(&combo->rightButton)) {
			combo->rightButton.state = GFUI_BTN_RELEASED;
			if (combo->rightButton.mouseBehaviour == GFUI_MOUSE_UP && combo->rightButton.onPush)
				combo->rightButton.onPush(combo->rightButton.userDataOnPush);
		}
	} else { /* mouse down */
		if (gfuiGrButtonMouseIn(&combo->leftButton)) {
			combo->leftButton.state = GFUI_BTN_PUSHED;
			if (combo->leftButton.mouseBehaviour == GFUI_MOUSE_DOWN && combo->leftButton.onPush)
				combo->leftButton.onPush(combo->leftButton.userDataOnPush);
		} else if (gfuiGrButtonMouseIn(&combo->rightButton)) {
			combo->rightButton.state = GFUI_BTN_PUSHED;
			if (combo->rightButton.mouseBehaviour == GFUI_MOUSE_DOWN && combo->rightButton.onPush)
				combo->rightButton.onPush(combo->rightButton.userDataOnPush);
		}
	}
}//gfuiComboboxAction

void
gfuiReleaseCombobox(tGfuiObject *obj)
{
	delete obj->u.combobox.pInfo;
	freez(obj->u.combobox.userDataOnFocus);
    free(obj);
}
