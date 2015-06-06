/***************************************************************************

    file                 : guiscrollist.cpp
    created              : Mon Aug 23 19:22:49 CEST 1999
    copyright            : (C) 1999 by Eric Espie                         
    email                : torcs@free.fr   
    version              : $Id: guiscrollist.cpp 4975 2012-09-30 21:54:35Z pouillot $                                  

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
	GUI scroll-list management.
    @author	<a href=mailto:torcs@free.fr>Eric Espie</a>
    @version	$Id: guiscrollist.cpp 4975 2012-09-30 21:54:35Z pouillot $
    @ingroup	gui
*/

#include <cstdlib>
#include <cstdio>
#include <cstring>

#include "tgfclient.h"
#include "gui.h"
#include "guifont.h"


void
gfuiInitScrollList(void)
{
}

static void
gfuiScroll(tScrollBarInfo *sinfo)
{
    tGfuiObject		*object;
    tGfuiScrollList	*scrollist;
    
    object = gfuiGetObject(GfuiScreen, (long)(sinfo->userData));
    if (object == NULL) {
		return;
    }
    if (object->widget != GFUI_SCROLLIST) {
		return;
    }
    scrollist = &(object->u.scrollist);

    scrollist->firstVisible = sinfo->pos;
    if ((scrollist->selectedElt < scrollist->firstVisible) ||
		(scrollist->selectedElt > scrollist->firstVisible + scrollist->nbVisible)) {
		scrollist->selectedElt = -1;
    }
}

void
gfuiScrollListNextElt (tGfuiObject *object)
{
    tGfuiScrollList	*scrollist;

    scrollist = &(object->u.scrollist);

    scrollist->selectedElt++;
    if (scrollist->selectedElt == scrollist->nbElts) {
		scrollist->selectedElt = scrollist->nbElts - 1;
		return;
    }
    if (scrollist->onSelect) {
		scrollist->onSelect(scrollist->userDataOnSelect);
    }
    if (scrollist->selectedElt == scrollist->firstVisible + scrollist->nbVisible) {
		/* Scroll down */
		if (scrollist->firstVisible + scrollist->nbVisible < scrollist->nbElts) {
			scrollist->firstVisible++;
			if (scrollist->scrollBar) {
				GfuiScrollBarPosSet(GfuiScreen, scrollist->scrollBar, 0, MAX(scrollist->nbElts - scrollist->nbVisible, 0),
									scrollist->nbVisible, scrollist->firstVisible);
			}
		}
    }
}

void
gfuiScrollListPrevElt (tGfuiObject *object)
{
    tGfuiScrollList	*scrollist;

    scrollist = &(object->u.scrollist);

    scrollist->selectedElt--;
    if (scrollist->selectedElt < 0) {
		scrollist->selectedElt = 0;
		return;
    }
    if (scrollist->onSelect) {
		scrollist->onSelect(scrollist->userDataOnSelect);
    }
    if (scrollist->selectedElt < scrollist->firstVisible) {
		/* Scroll down */
		if (scrollist->firstVisible > 0) {
			scrollist->firstVisible--;
			if (scrollist->scrollBar) {
				GfuiScrollBarPosSet(GfuiScreen, scrollist->scrollBar, 0, MAX(scrollist->nbElts - scrollist->nbVisible, 0),
									scrollist->nbVisible, scrollist->firstVisible);
			}
		}
    }
}


/** Create a new scroll list.
    @ingroup	gui
    @param	scr	Current screen
    @param	font	Current font
    @param	x	X Position (pixels)
    @param	y	Y Position (pixels)
    @param	width	Total width of the box (pixels)
    @param	height	Total height of the box (pixels)
    @param	scrollBarPos	Position of the scrollbar:
				<br>GFUI_SB_NONE	No scroll bar
				<br>GFUI_SB_RIGHT	Right scroll bar
				<br>GFUI_SB_LEFT	Left scroll bar
    @param	scrollBarWidth	Width of the scroll-bar (pixels)
    @param	scrollBarButHeight	Height of the scroll-bar buttons (pixels)
    @param	userDataOnSelect	User data to pass to the onSelect callback
    @param	onSelect		Callback when the selection is done 
    @return	Scroll List Id
*/
int
GfuiScrollListCreate(void *scr, int font, int x, int y, int width, int height,
					 int scrollBarPos, int scrollBarWidth, int scrollBarButHeight,
					 void *userDataOnSelect, tfuiCallback onSelect)
{
    tGfuiScrollList	*scrollist;
    tGfuiObject		*object;
    tGfuiScreen		*screen = (tGfuiScreen*)scr;

    object = (tGfuiObject*)calloc(1, sizeof(tGfuiObject));
    object->widget = GFUI_SCROLLIST;
    object->focusMode = GFUI_FOCUS_MOUSE_MOVE;
    object->id = screen->curId++;
    object->visible = 1;

    object->xmin = x;
    object->xmax = x + width;
    object->ymin = y;
    object->ymax = y + height;

    scrollist = &(object->u.scrollist);
    scrollist->fgColor[0] = GfuiColor::build(GFUI_FGSCROLLIST);
    scrollist->bgColor[0] = GfuiColor::build(GFUI_BGSCROLLIST);
    scrollist->fgSelectColor[0] = GfuiColor::build(GFUI_FGSELSCROLLIST);
    scrollist->bgSelectColor[0] = GfuiColor::build(GFUI_BGSELSCROLLIST);
    scrollist->font = gfuiFont[font];
    scrollist->nbVisible = height / scrollist->font->getHeight();
    scrollist->selectedElt = -1;
    scrollist->userDataOnSelect = userDataOnSelect;
    scrollist->onSelect = onSelect;

    switch (scrollBarPos) {
		case GFUI_SB_RIGHT:
			scrollist->scrollBar =
				GfuiScrollBarCreate(scr, x + width, y,
									height, scrollBarWidth, scrollBarButHeight,
									GFUI_VERT_SCROLLBAR, GFUI_SB_RIGHT,
									0, 10, 10, 10, (void *)(long)(object->id), gfuiScroll);
			break;
		case GFUI_SB_LEFT:
			scrollist->scrollBar =
				GfuiScrollBarCreate(scr, x - scrollBarWidth, y,
									height, scrollBarWidth, scrollBarButHeight,
									GFUI_VERT_SCROLLBAR, GFUI_SB_LEFT, 
									0, 10, 10, 10, (void *)(long)(object->id), gfuiScroll);
			break;
		case GFUI_SB_NONE:
		default:
			break;
    }
	
    gfuiAddObject(screen, object);

	return object->id;
}

static void
gfuiScrollListInsElt(tGfuiScrollList *scrollist, tGfuiListElement *elt, int index)
{
    tGfuiListElement	*cur;
    int			i;

    if (scrollist->elts == NULL) {
		scrollist->elts = elt;
		elt->next = elt;
		elt->prev = elt;
    } else {
		cur = scrollist->elts;
		i = 0;
		do {
			if (i == index) {
				break;
			}
			cur = cur->next;
			i++;
		} while (cur != scrollist->elts);
	
		elt->next = cur->next;
		cur->next = elt;
		elt->prev = cur;
		elt->next->prev = elt;
		if ((cur == scrollist->elts) && (index != 0)) {
			scrollist->elts = elt;
		}
    }
}

static tGfuiListElement *
gfuiScrollListRemElt(tGfuiScrollList *scrollist, int index)
{
    tGfuiListElement	*cur;
    int			i;

    if (scrollist->elts == NULL) {
		return (tGfuiListElement *)NULL;
    }
    cur = scrollist->elts;
    i = 0;
    do {
		cur = cur->next;
		if (i == index) {
			break;
		}
		i++;
    } while (cur != scrollist->elts);

    cur->next->prev = cur->prev;
    cur->prev->next = cur->next;
    if (cur == scrollist->elts) {
		if (cur->next == cur) {
			scrollist->elts = (tGfuiListElement *)NULL;
		} else {
			scrollist->elts = cur->prev;
		}
    }
    
    return cur;
}

/** Set the selected element from the scroll list.
    @ingroup	gui
    @param	scr		Current screen
    @param	id		Scroll list Id
    @param	selectElement   Index of the element to select
    @return	<br>false if no such element, true otherwise
*/
bool
GfuiScrollListSetSelectedElement(void *scr, int id, unsigned int selectElement)
{
    tGfuiObject* object = gfuiGetObject(scr, id);
    if (!object || object->widget != GFUI_SCROLLIST)
		return -1;
	
	tGfuiScrollList* scrollist = &(object->u.scrollist);

    if (selectElement >= (unsigned)scrollist->nbElts)
		return false;

    scrollist->selectedElt = selectElement;
    
    if (scrollist->onSelect)
		scrollist->onSelect(scrollist->userDataOnSelect);

    return true;
}

/** Clear the selection of the scroll list (no more selected element).
    @ingroup	gui
    @param	scr		Current screen
    @param	id		Scroll list Id
    @return	<br>false if any error, true otherwise
*/
bool
GfuiScrollListClearSelection(void *scr, int id)
{
    tGfuiObject* object = gfuiGetObject(scr, id);
    if (!object || object->widget != GFUI_SCROLLIST)
		return -1;
	
	tGfuiScrollList* scrollist = &(object->u.scrollist);

    scrollist->selectedElt = -1;
    
    return true;
}

/** Get the selected element from the scroll list.
    @ingroup	gui
    @param	scr		Current screen
    @param	id		Scroll list Id
    @return	Index of the retrieved element
	<br>-1 if none selected
*/
int
GfuiScrollListGetSelectedElementIndex(void *scr, int id)
{
    tGfuiObject* object = gfuiGetObject(scr, id);
    if (!object || object->widget != GFUI_SCROLLIST)
		return -1;
	
	tGfuiScrollList* scrollist = &(object->u.scrollist);

    if (scrollist->elts == NULL)
		return -1;

    return scrollist->selectedElt;
}

/** Get the selected element from the scroll list.
    @ingroup	gui
    @param	scr		Current screen
    @param	id		Scroll list Id
    @param	userData	address of the userData of the element to retrieve
    @return	Name of the retrieved element
	<br>NULL if Error
*/
const char *
GfuiScrollListGetSelectedElement(void *scr, int id, void **userData)
{
    tGfuiObject* object = gfuiGetObject(scr, id);
    if (!object || object->widget != GFUI_SCROLLIST)
		return (const char*)NULL;
	
	tGfuiScrollList* scrollist = &(object->u.scrollist);

    if (scrollist->selectedElt == -1)
		return (const char*)NULL;

    if (scrollist->elts == NULL)
		return (const char*)NULL;

	tGfuiListElement* elt = scrollist->elts;
    int i = 0;
    do {
		elt = elt->next;
		if (i == scrollist->selectedElt) {
			break;
		}
		i++;
    } while (elt != scrollist->elts);
    
    const char* name = elt->name;
    *userData = elt->userData;
    
    return name;
}

/** Get the number of elements from the scroll list.
    @ingroup	gui
    @param	scr		Current screen
    @param	id		Scroll list Id
    @return	Current number of elements
	<br>-1 if Error
*/
int
GfuiScrollListGetNumberOfElements(void *scr, int id)
{
    tGfuiObject* object = gfuiGetObject(scr, id);
    if (!object || object->widget != GFUI_SCROLLIST)
		return -1;
	
	tGfuiScrollList* scrollist = &(object->u.scrollist);
		
	return scrollist->nbElts;
}

/** Get the specified element from the scroll list.
    @ingroup	gui
    @param	scr		Current screen
    @param	id		Scroll list Id
    @param	index		Position where to get the element
    @param	userData	address of the userData of the element to retrieve
    @return	Name of the retrieved element
	<br>NULL if Error
*/
const char *
GfuiScrollListGetElement(void *scr, int id, int index, void **userData)
{
    tGfuiObject* object = gfuiGetObject(scr, id);
    if (!object || object->widget != GFUI_SCROLLIST)
		return (const char*)NULL;
	
	tGfuiScrollList* scrollist = &(object->u.scrollist);

    if ((index < 0) || (index > scrollist->nbElts - 1)) {
		return (const char*)NULL;
    }

    if (scrollist->elts == NULL) {
		return (const char*)NULL;
    }
    tGfuiListElement* elt = scrollist->elts;
    int i = 0;
    do {
		elt = elt->next;
		if (i == index) {
			break;
		}
		i++;
    } while (elt != scrollist->elts);    

    const char* name = elt->name;
    *userData = elt->userData;
    
    return name;
}

/** Extract the selected element from the scroll list (removed).
    @ingroup	gui
    @param	scr		Current screen
    @param	id		Scroll list Id
    @param	userData	address of the userData of the element to retrieve
    @return	Name of the extracted element
	<br>NULL if Error
*/
const char *
GfuiScrollListExtractSelectedElement(void *scr, int id, void **userData)
{
    tGfuiObject* object = gfuiGetObject(scr, id);
    if (!object || object->widget != GFUI_SCROLLIST)
		return (const char*)NULL;
	
	tGfuiScrollList* scrollist = &(object->u.scrollist);

    if (scrollist->selectedElt == -1)
		return (const char*)NULL;

    tGfuiListElement* elt = gfuiScrollListRemElt(scrollist, scrollist->selectedElt);
    
    scrollist->nbElts--;
    if (scrollist->selectedElt > scrollist->nbElts - 1) {
		scrollist->selectedElt--;
    }

    const char* name = elt->name;
    *userData = elt->userData;
    free(elt);
    
    return name;
}

/** Extract the specified element from the scroll list.
    @ingroup	gui
    @param	scr		Current screen
    @param	id		Scroll list Id
    @param	index		Position where to extract the element
    @param	userData	address of the userData of the element to retrieve
    @return	Name of the extracted element
	<br>NULL if Error
*/
const char *
GfuiScrollListExtractElement(void *scr, int id, int index, void **userData)
{
    tGfuiObject* object = gfuiGetObject(scr, id);
    if (!object || object->widget != GFUI_SCROLLIST)
		return (const char*)NULL;
	
	tGfuiScrollList* scrollist = &(object->u.scrollist);

    if (index < 0 || index > scrollist->nbElts - 1)
		return (const char*)NULL;

    tGfuiListElement* elt = gfuiScrollListRemElt(scrollist, index);
    
    scrollist->nbElts--;
    if (scrollist->selectedElt > scrollist->nbElts - 1) {
		scrollist->selectedElt--;
    }

    const char* name = elt->name;
    *userData = elt->userData;
    free(elt);
    
    return name;
}

void
GfuiScrollListClear(void *scr, int id)
{
    tGfuiObject* object = gfuiGetObject(scr, id);
    if (!object || object->widget != GFUI_SCROLLIST)
		return;
	
	tGfuiScrollList* scrollist = &(object->u.scrollist);

    tGfuiListElement *elt;
    while ((elt = gfuiScrollListRemElt(scrollist, 0))) {
		free(elt);
    }
	
	scrollist->nbElts = 0;
    scrollist->selectedElt = -1;
}

/** Insert an element in a scroll list.
    @ingroup	gui
    @param	scr		Current screen
    @param	id		Scroll list Id
    @param	element		New element
    @param	index		Position where to insert the element
    @param	userData	User defined data
    @return	<tt>0 ... </tt>Ok
	<br><tt>-1 .. </tt>Error
*/
int
GfuiScrollListInsertElement(void *scr, int id, const char *element, int index, void *userData)
{
	tGfuiObject* object = gfuiGetObject(scr, id);
    if (!object || object->widget != GFUI_SCROLLIST)
		return -1;
	
	tGfuiScrollList* scrollist = &(object->u.scrollist);
    
    tGfuiListElement* elt = (tGfuiListElement*)calloc(1, sizeof(tGfuiListElement));
    elt->name = element;
    elt->label = elt->name;  /* TODO LENGTH !!!!!*/
    elt->userData = userData;
    elt->index = index;

    gfuiScrollListInsElt(scrollist, elt, index);

    scrollist->nbElts++;
    if (scrollist->scrollBar)
		GfuiScrollBarPosSet(scr, scrollist->scrollBar, 0, MAX(scrollist->nbElts - scrollist->nbVisible, 0),
							scrollist->nbVisible, scrollist->firstVisible);

    return 0;
}

/** Scroll the list in order to show a given element.
    @ingroup	gui
    @param	scr		Current screen
    @param	id		Scroll list Id
    @param	index		Position of the element to show
*/
void
GfuiScrollListShowElement(void *scr, int id, int index)
{
    tGfuiObject* object = gfuiGetObject(scr, id);
	if (!object || object->widget != GFUI_SCROLLIST)
		return;

    tGfuiScrollList* scrollist = &(object->u.scrollist);
    
    if (scrollist->nbElts <= 0)
		return;
    
    if (index < 0)
		index = 0;
    else if (index >= scrollist->nbElts)
		index = scrollist->nbElts - 1;

    int oldFirstVisible = scrollist->firstVisible;
    if (index < scrollist->firstVisible)
        scrollist->firstVisible = index;
    else if (index >= scrollist->firstVisible + scrollist->nbVisible)
        scrollist->firstVisible = index - scrollist->nbVisible + 1;

    if (scrollist->firstVisible != oldFirstVisible && scrollist->scrollBar)
        GfuiScrollBarPosSet(scr, scrollist->scrollBar, 0, MAX(scrollist->nbElts - scrollist->nbVisible, 0),
							scrollist->nbVisible, scrollist->firstVisible);
}

void
GfuiScrollListSetColors(void *scr, int id, const GfuiColor& color, const GfuiColor& selectColor)
{
    tGfuiObject* object = gfuiGetObject(scr, id);
	if (!object || object->widget != GFUI_SCROLLIST)
		return;

    tGfuiScrollList* scrollist = &(object->u.scrollist);
    
	// Note : Only [0] used.
	if (color.alpha)
		scrollist->fgColor[0] = color;
	if (selectColor.alpha)
		scrollist->fgSelectColor[0] = selectColor;
}


void
gfuiDrawScrollist(tGfuiObject *obj)
{
	tGfuiScrollList* scrollist = &(obj->u.scrollist);

	GfuiColor fgColor;
	GfuiColor bgColor;
	if (scrollist->selectedElt < 0)
	{
		fgColor = scrollist->fgColor[0];
		bgColor = scrollist->bgColor[0];
	}
	else
	{
		fgColor = scrollist->fgSelectColor[0];
		bgColor = scrollist->bgSelectColor[0];
	}

	if (bgColor.alpha) {
		glBegin(GL_QUADS);
		glColor4fv(bgColor.toFloatRGBA());
		glVertex2i(obj->xmin, obj->ymin);
		glVertex2i(obj->xmin, obj->ymax);
		glVertex2i(obj->xmax, obj->ymax);
		glVertex2i(obj->xmax, obj->ymin);
		glEnd();
	}

	glBegin(GL_LINE_STRIP);
	glColor4fv(fgColor.toFloatRGBA());
	glVertex2i(obj->xmin, obj->ymin);
	glVertex2i(obj->xmin, obj->ymax);
	glVertex2i(obj->xmax, obj->ymax);
	glVertex2i(obj->xmax, obj->ymin);
	glVertex2i(obj->xmin, obj->ymin);
	glEnd();


	const int h = scrollist->font->getHeight();
	const int x = obj->xmin;
	int y = obj->ymax;
	int index = 0;
	tGfuiListElement* elt = scrollist->elts;
	if (elt) {
		do {
			elt = elt->next;
			if (index < scrollist->firstVisible) {
				index++;
				continue;
			}
			if (index == scrollist->selectedElt) {
				glColor4fv(scrollist->fgSelectColor[0].toFloatRGBA());
			} else {
				glColor4fv(scrollist->fgColor[0].toFloatRGBA());
			}
			index++;
			if (index > (scrollist->firstVisible + scrollist->nbVisible)) {
				break;
			}
			y -= h;
			gfuiDrawString(x+5, y, scrollist->font, elt->label);
		} while (elt != scrollist->elts);
	}
}

// Make all scroll-lists having no more selected element
void
gfuiScrollListDeselectAll(void)
{
    tGfuiObject* curObject = GfuiScreen->objects;
    if (curObject) {
		do {
			curObject = curObject->next;
			if (curObject->widget == GFUI_SCROLLIST)
				curObject->u.scrollist.selectedElt = -1;
		} while (curObject != GfuiScreen->objects);
    }
}


void
gfuiScrollListAction(int mouse)
{
    tGfuiObject		*object;
    tGfuiScrollList	*scrollist;
    int			relY;

    if (mouse)
		return;
	
	/* button down */
	gfuiScrollListDeselectAll();
	object = GfuiScreen->hasFocus;
	scrollist = &(object->u.scrollist);
	relY = object->ymax - GfuiMouse.Y;
	relY = scrollist->firstVisible + relY / scrollist->font->getHeight() + 1;
	if (relY > scrollist->nbElts) {
		scrollist->selectedElt = -1;
		return;
	}

	scrollist->selectedElt = relY - 1;
	
	if (scrollist->onSelect)
		scrollist->onSelect(scrollist->userDataOnSelect);
}

/** Move the selected element within the scroll list.
    @ingroup	gui
    @param	scr		Current screen
    @param	id		Scroll list Id
    @param	delta		displacement
    @return	<tt>0 ... </tt>Ok
	<br><tt>-1 .. </tt>Error
*/
int
GfuiScrollListMoveSelectedElement(void *scr, int id, int delta)
{
    tGfuiObject		*object;
    tGfuiScrollList	*scrollist;
    int			newPos;
    tGfuiListElement	*elt;
    
    object = gfuiGetObject(scr, id);
    if (!object || object->widget != GFUI_SCROLLIST)
		return -1;

    scrollist = &(object->u.scrollist);

    if (scrollist->selectedElt == -1)
		return -1;
    
    newPos = scrollist->selectedElt + delta;
    
    if (newPos < 0 || newPos > scrollist->nbElts - 1)
		return -1;
    
    elt = gfuiScrollListRemElt(scrollist, scrollist->selectedElt);
    
    gfuiScrollListInsElt(scrollist, elt, newPos);
    
    scrollist->selectedElt = newPos;

    if (scrollist->selectedElt == scrollist->firstVisible + scrollist->nbVisible) {
		/* Scroll down */
		if (scrollist->firstVisible + scrollist->nbVisible < scrollist->nbElts) {
			scrollist->firstVisible++;
			if (scrollist->scrollBar) {
				GfuiScrollBarPosSet(GfuiScreen, scrollist->scrollBar, 0, MAX(scrollist->nbElts - scrollist->nbVisible, 0),
									scrollist->nbVisible, scrollist->firstVisible);
			}
		}
    } else if (scrollist->selectedElt < scrollist->firstVisible) {
		/* Scroll down */
		if (scrollist->firstVisible > 0) {
			scrollist->firstVisible--;
			if (scrollist->scrollBar) {
				GfuiScrollBarPosSet(GfuiScreen, scrollist->scrollBar, 0, MAX(scrollist->nbElts - scrollist->nbVisible, 0),
									scrollist->nbVisible, scrollist->firstVisible);
			}
		}
    }
    
    return 0;
}


void
gfuiReleaseScrollist(tGfuiObject *curObject)
{
    tGfuiScrollList	*scrollist;
    tGfuiListElement	*elt;

    scrollist = &(curObject->u.scrollist);
    while ((elt = gfuiScrollListRemElt(scrollist, 0)) != NULL) {
		free(elt);
    }
    free(curObject);
}

