/***************************************************************************
                                guiobject.cpp                      
                             -------------------                                         
    created              : Fri Aug 13 22:25:06 CEST 1999
    copyright            : (C) 1999 by Eric Espie                         
    email                : torcs@free.fr   
    version              : $Id: guiobject.cpp 4213 2011-11-27 14:18:30Z pouillot $                                  
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#include <cstring>
#include <string>

#include <portability.h> // snprintf

#include "tgfclient.h"
#include "gui.h"


// Mouse cursor graphic properties.
static int NMouseCursorXOffset = 0;
static int NMouseCursorYOffset = 0;
static int NMouseCursorHeight = 20;
static int NMouseCursorWidth = 20;
static GLuint NMouseCursorTexture = 0;


void
gfuiInitObject(void)
{
	//Read mouse pointer settings
	char buf[512];
	snprintf(buf, sizeof(buf), "%s%s", GfLocalDir(), GFSCR_CONF_FILE);
	void *param = GfParmReadFile(buf, GFPARM_RMODE_STD | GFPARM_RMODE_CREAT);

	NMouseCursorXOffset =
		(int)GfParmGetNum(param, GFSCR_SECT_MOUSECURSOR, GFSCR_ATT_XOFFSET, (char*)NULL, 0.0);
	NMouseCursorYOffset =
		(int)GfParmGetNum(param, GFSCR_SECT_MOUSECURSOR, GFSCR_ATT_YOFFSET, (char*)NULL, 0.0);
	NMouseCursorHeight =
		(int)GfParmGetNum(param, GFSCR_SECT_MOUSECURSOR, GFSCR_ATT_HEIGHT, (char*)NULL, 20.0);
	NMouseCursorWidth =
		(int)GfParmGetNum(param, GFSCR_SECT_MOUSECURSOR, GFSCR_ATT_WIDTH, (char*)NULL, 20.0);

	const char* pszImageFile =
		GfParmGetStr(param, GFSCR_SECT_MOUSECURSOR, GFSCR_ATT_IMAGEFILE, "data/img/mouse.png");
	snprintf(buf, sizeof(buf), "%s%s", GfDataDir(), pszImageFile);
	NMouseCursorTexture = GfTexReadTexture(buf);
}

void 
gfuiDrawString(int x, int y, GfuiFontClass *font, const char *string)
{
    glEnable(GL_TEXTURE_2D);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.1) ;
    font->drawString(x, y, string);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_TEXTURE_2D);
}

void GfuiDrawString(const char *text, float *fgColor, int font,
					int x, int y, int width, int hAlign)
{
	glEnable(GL_TEXTURE_2D);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.1) ;
	glColor4fv(fgColor);
	switch(hAlign & GFUI_ALIGN_HMASK) {
		case GFUI_ALIGN_HL:
			gfuiFont[font]->drawString(x, y, text);
			break;
		case GFUI_ALIGN_HC:
			gfuiFont[font]->drawString(x + (width - gfuiFont[font]->getWidth(text)) / 2, y, text);
			break;
		case GFUI_ALIGN_HR:
			gfuiFont[font]->drawString(x + width - gfuiFont[font]->getWidth(text), y, text);
			break;
	}
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_TEXTURE_2D);
}

int GfuiFontHeight(int font)
{
    return gfuiFont[font]->getHeight();
}

int GfuiFontWidth(int font, const char *text)
{
    return gfuiFont[font]->getWidth(text);
}

void
GfuiDrawCursor()
{
	const int xmin = NMouseCursorXOffset + GfuiMouse.X;
	const int ymin = NMouseCursorYOffset + GfuiMouse.Y;
	const int xmax = xmin + NMouseCursorWidth;
	const int ymax = ymin - NMouseCursorHeight;

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	//set color to mix with image
	glColor3f(1.0,1.0,1.0);

	glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	glBindTexture(GL_TEXTURE_2D, NMouseCursorTexture);
	glBegin(GL_QUADS);

	glTexCoord2f(0.0, 1.0);
	glVertex2i(xmin, ymin);
	
	glTexCoord2f(0.0, 0.0);
	glVertex2i(xmin, ymax);
	
	glTexCoord2f(1.0, 0.0);
	glVertex2i(xmax, ymax);
	
	glTexCoord2f(1.0, 1.0);
	glVertex2i(xmax, ymin);

	glEnd();

	glDisable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void
GfuiDraw(tGfuiObject *obj)
{
    if (obj->visible) {
	switch (obj->widget){
	case GFUI_LABEL:
	    gfuiDrawLabel(obj);
	    break;
	
	case GFUI_BUTTON:
	    gfuiDrawButton(obj);
	    break;

	case GFUI_GRBUTTON:
	    gfuiDrawGrButton(obj);
	    break;

	case GFUI_SCROLLIST:
	    gfuiDrawScrollist(obj);
	    break;
	
	case GFUI_EDITBOX:
	    gfuiDrawEditbox(obj);
	    break;

	case GFUI_IMAGE:
	    gfuiDrawImage(obj);
	    break;

	case GFUI_COMBOBOX:
		gfuiDrawCombobox(obj);
		break;

	case GFUI_CHECKBOX:
		gfuiDrawCheckbox(obj);
		break;

	case GFUI_PROGRESSBAR:
		gfuiDrawProgressbar(obj);
		break;
	}
    }
}


static int
gfuiMouseIn(tGfuiObject *obj)
{
    return (GfuiMouse.X >= obj->xmin && GfuiMouse.X <= obj->xmax
			&& GfuiMouse.Y >= obj->ymin && GfuiMouse.Y <= obj->ymax) ? 1 : 0;
}

/** Remove the focus on the current element.
    @ingroup	gui
*/
void
GfuiUnSelectCurrent(void)
{
    tGfuiButton		*button;
    tGfuiEditbox	*editbox;
    tGfuiGrButton	*grbutton;
    tGfuiObject		*obj;

    obj = GfuiScreen->hasFocus;
    if (obj == NULL) {
	return;
    }
    GfuiScreen->hasFocus = (tGfuiObject*)NULL;
    obj->focus = 0;
    switch (obj->widget) {
    case GFUI_BUTTON:
	button = &(obj->u.button);
	button->state = GFUI_BTN_RELEASED;
	if (button->onFocusLost != NULL) {
	    button->onFocusLost(button->userDataOnFocus);
	}
	break;
    case GFUI_GRBUTTON:
	grbutton = &(obj->u.grbutton);
	grbutton->state = GFUI_BTN_RELEASED;
	if (grbutton->onFocusLost != NULL) {
	    grbutton->onFocusLost(grbutton->userDataOnFocus);
	}
	break;
    case GFUI_EDITBOX:
	editbox = &(obj->u.editbox);
	editbox->state = GFUI_BTN_RELEASED;	
	if (editbox->onFocusLost != NULL) {
	    editbox->onFocusLost(editbox->userDataOnFocus);
	}
	break;
    }
}

static void
gfuiLoseFocus(tGfuiObject *obj)
{
    GfuiScreen->hasFocus = (tGfuiObject*)NULL;
    obj->focus = 0;

    switch (obj->widget)
	{
		case GFUI_BUTTON:
		{
			tGfuiButton* button = &(obj->u.button);
			button->state = GFUI_BTN_RELEASED;
			if (button->onFocusLost)
				button->onFocusLost(button->userDataOnFocus);
		}
		break;

		case GFUI_GRBUTTON:
		{
			tGfuiGrButton* grbutton = &(obj->u.grbutton);
			grbutton->state = GFUI_BTN_RELEASED;
			if (grbutton->onFocusLost)
				grbutton->onFocusLost(grbutton->userDataOnFocus);
		}
		break;

		case GFUI_EDITBOX:
		{
			tGfuiEditbox* editbox = &(obj->u.editbox);
			editbox->state = GFUI_BTN_RELEASED;	
			if (editbox->onFocusLost)
				editbox->onFocusLost(editbox->userDataOnFocus);
		}
		break;

		case GFUI_PROGRESSBAR:
		{
			tGfuiProgressbar* progress = &(obj->u.progressbar);
			if (progress->onFocusLost)
				progress->onFocusLost(progress->userDataOnFocus);
		}
		break;

		case GFUI_LABEL:
		{
			tGfuiLabel* label = &(obj->u.label);
			if (label->onFocusLost)
				label->onFocusLost(label->userDataOnFocus);
		}
		break;
		
 		case GFUI_COMBOBOX:
		{
			tGfuiCombobox* combo = &(obj->u.combobox);
			if (combo->onFocusLost)
				combo->onFocusLost(combo->userDataOnFocus);
		}
		break;
   }
}

static void
gfuiSetFocus(tGfuiObject *obj)
{
    if (GfuiScreen->hasFocus)
		gfuiLoseFocus(GfuiScreen->hasFocus);

    GfuiScreen->hasFocus = obj;
    obj->focus = 1;

    switch (obj->widget)
	{
		case GFUI_BUTTON:
		{
			tGfuiButton* button = &(obj->u.button);
			if (button->onFocus)
				button->onFocus(button->userDataOnFocus);
		}
		break;

		case GFUI_GRBUTTON:
		{
			tGfuiGrButton* grbutton = &(obj->u.grbutton);
			if (grbutton->onFocus)
				grbutton->onFocus(grbutton->userDataOnFocus);
		}
		break;

		case GFUI_EDITBOX:
		{
			tGfuiEditbox* editbox = &(obj->u.editbox);
			if (editbox->onFocus)
				editbox->onFocus(editbox->userDataOnFocus);
		}
		break;

		case GFUI_PROGRESSBAR:
		{
			tGfuiProgressbar* progress = &(obj->u.progressbar);
			if (progress->onFocus)
				progress->onFocus(progress->userDataOnFocus);
		}
		break;
		
		case GFUI_LABEL:
		{
			tGfuiLabel* label = &(obj->u.label);
			if (label->onFocus)
				label->onFocus(label->userDataOnFocus);
		}
		break;
		
		case GFUI_COMBOBOX:
		{
			tGfuiCombobox* combo = &(obj->u.combobox);
			if (combo->onFocus)
				combo->onFocus(combo->userDataOnFocus);
		}
		break;
    }
}

void
gfuiUpdateFocus(void)
{
    tGfuiObject *curObject;
    
    curObject = GfuiScreen->hasFocus;
    if (curObject != NULL) {
	if (gfuiMouseIn(curObject)) {
	    return; /* focus has not changed */
	}
	if (curObject->focusMode != GFUI_FOCUS_MOUSE_CLICK) {
	    gfuiLoseFocus(GfuiScreen->hasFocus);
	    GfuiScreen->hasFocus = (tGfuiObject*)NULL;
	}
    }
    
    /* Search for a new focused object */
    curObject = GfuiScreen->objects;
    if (curObject != NULL) {
	do {
	    curObject = curObject->next;
	    if ((curObject->visible == 0) ||
		(curObject->focusMode == GFUI_FOCUS_NONE) ||
		((curObject->focusMode == GFUI_FOCUS_MOUSE_CLICK) && (GfuiScreen->mouse == 0))) {
		continue;
	    }
	    if (gfuiMouseIn(curObject)) {
		gfuiSetFocus(curObject);
		break;
	    }
	} while (curObject != GfuiScreen->objects);
    }
}

void
gfuiSelectNext(void * /* dummy */)
{
    tGfuiObject *startObject;
    tGfuiObject *curObject;
    
    startObject = GfuiScreen->hasFocus;
    if (startObject == NULL) {
	startObject = GfuiScreen->objects;
    }
    if (startObject == NULL) {
	return;
    }
    curObject = startObject;
    do {
	switch (curObject->widget) {
	case GFUI_SCROLLIST:
	    gfuiScrollListNextElt(curObject);
	    break;
	    
	default:
	    curObject = curObject->next;
	    if ((curObject->focusMode != GFUI_FOCUS_NONE) &&
		(curObject->state != GFUI_DISABLE) &&
		(curObject->visible)) {
		gfuiSetFocus(curObject);
		return;
	    }
	    break;
	}
    } while (curObject != startObject);    
}

void
gfuiSelectPrev(void * /* dummy */)
{
    tGfuiObject *startObject;
    tGfuiObject *curObject;
    
    startObject = GfuiScreen->hasFocus;
    if (startObject == NULL) {
	startObject = GfuiScreen->objects;
	if (startObject == NULL) {
	    return;
	}
	startObject = startObject->next;
    }
    curObject = startObject;
    do {
	switch (curObject->widget) {
	case GFUI_SCROLLIST:
	    gfuiScrollListPrevElt(curObject);
	    break;

	default:
	    curObject = curObject->prev;
	    if ((curObject->focusMode != GFUI_FOCUS_NONE) &&
		(curObject->state != GFUI_DISABLE) &&
		(curObject->visible)) {
		gfuiSetFocus(curObject);
		return;
	    }
	    break;
	}
    } while (curObject != startObject);
}

void 
gfuiSelectId(void *scr, int id)
{
    tGfuiObject *curObject;
    tGfuiScreen	*screen = (tGfuiScreen*)scr;
    
    curObject = screen->objects;
    if (curObject != NULL) {
	do {
	    curObject = curObject->next;
	    if (curObject->id == id) {
		gfuiSetFocus(curObject);
		break;
	    }
	} while (curObject != screen->objects);
    }
}

/** Set/unset the visibility attribute of an object.
    @param	scr	Screen    
    @param	id	Object id
    @param	visible	GFUI_VISIBLE or GFUI_INVISIBLE
    @return	<tt>0 ... </tt>Ok
		<br><tt>-1 .. </tt>Error
    @ingroup	gui
 */
int
GfuiVisibilitySet(void *scr, int id, int visible)
{
    tGfuiObject *curObject;
    
    curObject = gfuiGetObject(scr, id);
    if (curObject == NULL) {
	return -1;
    }
    switch(visible) {
    case GFUI_VISIBLE:
	curObject->visible = GFUI_VISIBLE;
	break;
    case GFUI_INVISIBLE:
	curObject->visible = GFUI_INVISIBLE;
	curObject->focus = 0;
	break;
    default:
	return -1;
    }
    return 0;
}

/** Enable / Disable an object
    @param	scr	Screen    
    @param	id	Object id
    @param	flag	GFUI_ENABLE or GFUI_DISABLE
    @return	<tt>0 ... </tt>Ok
		<br><tt>-1 .. </tt>Error
    @ingroup	gui
 */
int 
GfuiEnable(void *scr, int id, int flag)
{
    tGfuiObject *curObject;

    curObject = gfuiGetObject(scr, id);
    if (!curObject)
		return -1;

    switch(flag)
	{
		case GFUI_ENABLE:
			curObject->state = GFUI_ENABLE;
			break;
		case GFUI_DISABLE:
			curObject->state = GFUI_DISABLE;
			break;
		default:
			return -1;
    }

	switch (curObject->widget)
	{
		case GFUI_BUTTON:
			if (curObject->state == GFUI_DISABLE) 
				curObject->u.button.state = GFUI_BTN_DISABLE;
			else
				curObject->u.button.state = GFUI_BTN_RELEASED;
			break;

		default:
			break;
	}

    return 0;
}

void
gfuiMouseAction(void *vaction)
{
    tGfuiObject *curObject;
    long	action = (long)vaction;

    curObject = GfuiScreen->hasFocus;
    if (curObject != NULL) {
	switch (curObject->widget) {
	case GFUI_BUTTON:
	    gfuiButtonAction((int)action);
	    break;
	case GFUI_GRBUTTON:
	    gfuiGrButtonAction((int)action);
	    break;
	case GFUI_SCROLLIST:
	    gfuiScrollListAction((int)action);
	    break;
	case GFUI_EDITBOX:
	    gfuiEditboxAction((int)action);
	    break;
	case GFUI_COMBOBOX:
	    gfuiComboboxAction((int)action);
	    break;
	}
    }
}

void
gfuiAddObject(tGfuiScreen *screen, tGfuiObject *object)
{
    if (screen->objects == NULL) {
	screen->objects = object;
	object->next = object;
	object->prev = object;
    } else {
	object->next = screen->objects->next;
	object->prev = screen->objects;
	screen->objects->next = object;
	object->next->prev = object;
	screen->objects = object;
    }
}

tGfuiObject * 
gfuiGetObject(void *scr, int id)
{
    tGfuiObject *curObject;
    tGfuiScreen	*screen = (tGfuiScreen*)scr;
    
    curObject = screen->objects;
    if (curObject != NULL) {
	do {
	    curObject = curObject->next;
	    if (curObject->id == id) {
		return curObject;
	    }
	} while (curObject != screen->objects);
    }
    return (tGfuiObject *)NULL;
}


void
gfuiReleaseObject(tGfuiObject *curObject)
{
    switch (curObject->widget){
    case GFUI_LABEL:
	gfuiReleaseLabel(curObject);
	break;
	
    case GFUI_BUTTON:
	gfuiReleaseButton(curObject);
	break;
	
    case GFUI_GRBUTTON:
	gfuiReleaseGrButton(curObject);
	break;
	
    case GFUI_SCROLLIST:
	gfuiReleaseScrollist(curObject);
	break;

    case GFUI_SCROLLBAR:
	gfuiReleaseScrollbar(curObject);
	break;

    case GFUI_EDITBOX:
	gfuiReleaseEditbox(curObject);
	break;

    case GFUI_IMAGE:
	gfuiReleaseImage(curObject);
	break;

    case GFUI_COMBOBOX:
	gfuiReleaseCombobox(curObject);
	break;

	case GFUI_CHECKBOX:
	gfuiReleaseCheckbox(curObject);
	break;

	case GFUI_PROGRESSBAR:
	gfuiReleaseProgressbar(curObject);
	break;

    }
}
