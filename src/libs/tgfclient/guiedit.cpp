/***************************************************************************

    file                 : guiedit.cpp
    created              : Mon Apr 24 10:23:28 CEST 2000
    copyright            : (C) 2000 by Eric Espie
    email                : torcs@free.fr
    version              : $Id: guiedit.cpp 3712 2011-07-10 10:50:58Z pouillot $

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
    		GUI Edit Box Management.
    @author	<a href=mailto:torcs@free.fr>Eric Espie</a>
    @version	$Id: guiedit.cpp 3712 2011-07-10 10:50:58Z pouillot $
    @ingroup	gui
*/

#include <cstdlib>
#include <cstring>

#include "tgfclient.h"
#include "gui.h"
#include "guifont.h"


static int NHPadding = 10;
static int NVPadding = 5;


void
gfuiInitEditbox(void)
{
	char path[512];

	// Get tip layout properties from the screen config file.
	sprintf(path, "%s%s", GfLocalDir(), GFSCR_CONF_FILE);
	void* hparmScr = GfParmReadFile(path, GFPARM_RMODE_STD | GFPARM_RMODE_CREAT);

	sprintf(path, "%s/%s", GFSCR_SECT_MENUSETTINGS, GFSCR_SECT_EDITBOX);
	NHPadding = (int)GfParmGetNum(hparmScr, path, GFSCR_ATT_HPADDING, 0, 10.0);
	NVPadding = (int)GfParmGetNum(hparmScr, path, GFSCR_ATT_VPADDING, 0,  5.0);
}

/* recalc cursorx with cursorIdx */
static void
gfuiEditboxRecalcCursor(tGfuiObject *obj)
{
    tGfuiEditbox* editbox = &(obj->u.editbox);
    tGfuiLabel* label = &(editbox->label);
	
    char buf[256];
    strncpy(buf, label->text, editbox->cursorIdx);
    buf[editbox->cursorIdx] = '\0';
	
	editbox->cursorx = gfuiLabelGetTextX(label) + label->font->getWidth(buf);
}

/** Add a editbox to a screen.
    @ingroup	gui
    @param	scr		Screen
    @param	text		Editbox start text
    @param	font		Font id
    @param	x		X position on screen
    @param	y		Y position on screen (0 = bottom)
    @param	width		width of the editbox (0 = text size)
    @param	maxlen		Max lenght of text (0 = text size)
    @param	userDataOnFocus	Parameter to the Focus (and lost) callback
    @param	onFocus		Focus callback function
    @param	onFocusLost	Focus Lost callback function
    @return	Editbox Id
		<br>-1 Error
 */
int
GfuiEditboxCreate(void *scr, const char *text, int font,
				  int x, int y, int width, int maxlen, int align,
				  void *userDataOnFocus, tfuiCallback onFocus, tfuiCallback onFocusLost)
{
    tGfuiScreen* screen = (tGfuiScreen*)scr;

    tGfuiObject* object = (tGfuiObject*)calloc(1, sizeof(tGfuiObject));
    object->widget = GFUI_EDITBOX;
    object->focusMode = GFUI_FOCUS_MOUSE_CLICK;
    object->id = screen->curId++;
    object->visible = 1;
    
    tGfuiEditbox* editbox = &(object->u.editbox);
    editbox->state = GFUI_BTN_RELEASED;
    editbox->userDataOnFocus = userDataOnFocus;
    editbox->onFocus = onFocus;
    editbox->onFocusLost = onFocusLost;

    editbox->bgColor[0] = GfuiColor::build(GFUI_BGEDITDISABLED);
    editbox->bgColor[1] = GfuiColor::build(GFUI_BGEDITENABLED);
    editbox->bgColor[2] = GfuiColor::build(GFUI_BGEDITENABLED);
    editbox->bgFocusColor[0] = GfuiColor::build(GFUI_BGEDITDISABLED);
    editbox->bgFocusColor[1] = GfuiColor::build(GFUI_BGEDITFOCUS);
    editbox->bgFocusColor[2] = GfuiColor::build(GFUI_BGEDITFOCUS);
	
    editbox->fgColor[0] = GfuiColor::build(GFUI_EDITDISABLED);
    editbox->fgColor[1] = GfuiColor::build(GFUI_EDITENABLED);
    editbox->fgColor[2] = GfuiColor::build(GFUI_EDITENABLED);
    editbox->fgFocusColor[0] = GfuiColor::build(GFUI_EDITDISABLED);
    editbox->fgFocusColor[1] = GfuiColor::build(GFUI_EDITFOCUS);
	editbox->fgFocusColor[2] = GfuiColor::build(GFUI_EDITFOCUS);

    editbox->cursorColor[0] = GfuiColor::build(GFUI_EDITCURSORCLR);
    editbox->cursorColor[1] = GfuiColor::build(GFUI_EDITCURSORCLR);
    editbox->cursorColor[2] = GfuiColor::build(GFUI_EDITCURSORCLR);
    
    tGfuiLabel* label = &(editbox->label);
	gfuiLabelInit(label, text, maxlen, x + NHPadding, y + NVPadding,
				  width - 2 * NHPadding, align, font, 0, 0, 0, 0, 0, 0, 0);
	
	maxlen = label->maxlen;

    if (width <= 0)
	{
		char* buf = (char*)malloc(maxlen+1);
		for (int i = 0; i < maxlen; i++)
			buf[i] = 'W';
		buf[maxlen] = '\0';
		width = gfuiFont[font]->getWidth((const char *)buf) + 2*NHPadding;
		free(buf);
    }

    object->xmin = x;
    object->ymin = y;
    object->xmax = x + width;
    object->ymax = y + gfuiFont[font]->getHeight() + 2*NVPadding;
	
    editbox->cursory1 = object->ymin + NVPadding / 2;
    editbox->cursory2 = object->ymax - NVPadding / 2;
	editbox->cursorIdx = (int)strlen(label->text);
	gfuiEditboxRecalcCursor(object);
    
    gfuiAddObject(screen, object);

    return object->id;
}


void
GfuiEditboxSetColors(void *scr, int id, const GfuiColor& color,
					 const GfuiColor& focusedColor, const GfuiColor& disabledColor)
{
    tGfuiObject* object = gfuiGetObject(scr, id);
	if (!object || object->widget != GFUI_EDITBOX)
		return;
	
	if (color.alpha)
	{
		object->u.editbox.fgColor[1] = color;
		object->u.editbox.fgColor[2] = color;
	}
	if (disabledColor.alpha)
	{
		object->u.editbox.fgColor[0] = disabledColor;
		object->u.editbox.fgFocusColor[0] = disabledColor;
	}
	if (focusedColor.alpha)
	{
		object->u.editbox.fgFocusColor[1] = focusedColor;
		object->u.editbox.fgFocusColor[2] = focusedColor;
	}
}

void
GfuiEditboxSetBGColors(void *scr, int id, const GfuiColor& color,
					   const GfuiColor& focusedColor, const GfuiColor& disabledColor)
{
    tGfuiObject* object = gfuiGetObject(scr, id);

	if (!object || object->widget != GFUI_EDITBOX)
		return;

	if (color.alpha)
	{
		object->u.editbox.bgColor[1] = color;
		object->u.editbox.bgColor[2] = color;
	}
	if (disabledColor.alpha)
	{
		object->u.editbox.bgColor[0] = disabledColor;
		object->u.editbox.bgFocusColor[0] = disabledColor;
	}
	if (focusedColor.alpha)
	{
		object->u.editbox.bgFocusColor[1] = focusedColor;
		object->u.editbox.bgFocusColor[2] = focusedColor;
	}
}

void
gfuiDrawEditbox(tGfuiObject *obj)
{

    tGfuiEditbox* editbox = &(obj->u.editbox);
	editbox->state = (obj->state == GFUI_DISABLE) ? GFUI_BTN_DISABLE : GFUI_BTN_RELEASED;

    GfuiColor fgColor, bgColor;
    if (obj->focus)
	{
		fgColor = editbox->fgFocusColor[editbox->state];
		bgColor = editbox->bgFocusColor[editbox->state];
    }
	else
	{
		fgColor = editbox->fgColor[editbox->state];
		bgColor = editbox->bgColor[editbox->state];
    }

	// Draw background
    glColor4fv(bgColor.toFloatRGBA());
    glBegin(GL_QUADS);
    glVertex2i(obj->xmin, obj->ymin);
    glVertex2i(obj->xmin, obj->ymax);
    glVertex2i(obj->xmax, obj->ymax);
    glVertex2i(obj->xmax, obj->ymin);
    glEnd();

	// Draw box
	// GfLogDebug("gfuiDrawEditbox: state=%d, fgColor=(%f,%f,%f,%f)\n",
	// 		   editbox->state, fgColor.red, fgColor.green, fgColor.blue, fgColor.alpha);
    glColor4fv(fgColor.toFloatRGBA());
    glBegin(GL_LINE_STRIP);
    glVertex2i(obj->xmin, obj->ymin);
    glVertex2i(obj->xmin, obj->ymax);
    glVertex2i(obj->xmax, obj->ymax);
    glVertex2i(obj->xmax, obj->ymin);
    glVertex2i(obj->xmin, obj->ymin);
    glEnd();	
    
	// Draw label
    tGfuiLabel* label = &(editbox->label);
    gfuiLabelDraw(label, fgColor);
    
	// Draw cursor if enabled and focused
    if (obj->state != GFUI_DISABLE && obj->focus)
	{
		glColor3fv(editbox->cursorColor[editbox->state].toFloatRGBA());
		glBegin(GL_LINES);
		glVertex2i(editbox->cursorx, editbox->cursory1);
		glVertex2i(editbox->cursorx, editbox->cursory2);
		glEnd();
    }
}


void
gfuiEditboxAction(int mouse)
{
    tGfuiObject* object = GfuiScreen->hasFocus;
    if (object->state == GFUI_DISABLE)
		return;

	/* enter key */
    if (mouse == 2)
		gfuiSelectNext(GfuiScreen);

	/* mouse down */
	else if (mouse == 0)
	{
		tGfuiEditbox* editbox = &(object->u.editbox);
		tGfuiLabel* label = &(editbox->label);
		
		/* Set the cursor position */
		const int relX = GfuiMouse.X - gfuiLabelGetTextX(label);
		char buf[256];
		unsigned i;
		for (i = 0; i < strlen(label->text); i++)
		{
			buf[i] = label->text[i];
			buf[i+1] = '\0';
			if (relX < label->font->getWidth((const char *)buf))
				break;
		}
		editbox->cursorIdx = i;

		// Recompute cursorx from cursorIdx
		gfuiEditboxRecalcCursor(object);
    }
}

void
gfuiEditboxKey(tGfuiObject *obj, int key, int modifier)
{
    if (obj->state == GFUI_DISABLE)
		return;

    tGfuiEditbox* editbox = &(obj->u.editbox);
    tGfuiLabel* label = &(editbox->label);

	if (!(modifier & (KMOD_CTRL|KMOD_ALT)))
	{
		char* p1, *p2;
		int	i1, i2;
		switch (key) 
		{
			// Move keys.
			case GFUIK_RIGHT:
				editbox->cursorIdx++;
				if (editbox->cursorIdx > (int)strlen(label->text)) 
				{
					editbox->cursorIdx--;
				}
				break;
			case GFUIK_LEFT:
				editbox->cursorIdx--;
				if (editbox->cursorIdx < 0) 
				{
					editbox->cursorIdx = 0;
				}
				break;
			case GFUIK_HOME:
				editbox->cursorIdx = 0;
				break;
			case GFUIK_END:
				editbox->cursorIdx = (int)strlen(label->text);
				break;

				// Edition keys
			case GFUIK_DELETE:
				if (editbox->cursorIdx < (int)strlen(label->text)) 
				{
					p1 = &(label->text[editbox->cursorIdx]);
					p2 = &(label->text[editbox->cursorIdx+1]);
					while ( *p1 != '\0' ) 
						*p1++ = *p2++;
				}
				break;
			case GFUIK_BACKSPACE:
				if (editbox->cursorIdx > 0) 
				{
					p1 = &(label->text[editbox->cursorIdx-1]);
					p2 = &(label->text[editbox->cursorIdx]);
					while ( *p1 != '\0' ) 
						*p1++ = *p2++;
					editbox->cursorIdx--;
				}
				break;
		
			default:
				// Normal char keys
				if (key >= ' ' && key < 127) 
				{
					if ((int)strlen(label->text) < label->maxlen) 
					{
						i2 = (int)strlen(label->text) + 1;
						i1 = i2 - 1;
						while (i2 > editbox->cursorIdx) 
						{
							label->text[i2] = label->text[i1];
							i1--;
							i2--;
						}
						label->text[editbox->cursorIdx] = key;
						editbox->cursorIdx++;
					}
				}
		}
	}

    gfuiEditboxRecalcCursor(obj);
}

/** Get the string
    @ingroup	gui
     @param	scr		Screen
    @param	id		Edit box Id
    @return	Corresponding string.
 */
char *
GfuiEditboxGetString(void *scr, int id)
{
	tGfuiObject* curObject = gfuiGetObject(scr, id);
    
    if (!curObject || curObject->widget != GFUI_EDITBOX)
		return 0;

    tGfuiEditbox* editbox = &(curObject->u.editbox);
    tGfuiLabel* label = &(editbox->label);
    
    return label->text;
}

/** Set a new string.
    @ingroup	gui
    @param	scr		Screen
    @param	id		Edit box Id
    @param	text		text to set
    @return	none
 */
void GfuiEditboxSetString(void *scr, int id, const char *text)
{
    tGfuiObject* curObject = gfuiGetObject(scr, id);
    
    if (!curObject || curObject->widget != GFUI_EDITBOX) 
		return;

    tGfuiEditbox* editbox = &(curObject->u.editbox);
    tGfuiLabel* label = &(editbox->label);

    strncpy(label->text, text, label->maxlen);
}


void
gfuiReleaseEditbox(tGfuiObject *curObject)
{
    tGfuiEditbox* editbox = &(curObject->u.editbox);
    tGfuiLabel* label = &(editbox->label);
	
    free(label->text);
    free(curObject);
}

