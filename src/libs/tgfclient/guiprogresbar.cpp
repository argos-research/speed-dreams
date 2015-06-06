/***************************************************************************

    file                 : guiprogressbar.cpp
	created              : Feb 17 2010
    copyright            : (C) 2010 by Brian Gavin

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

#include "gui.h"


/** Create a new progress bar
    This kind of image is not clickable.
    @ingroup	gui
    @param	scr	Screen where to add the label
    @param	x	Position of the left of the image on the screen
    @param	y	Position of the bottom of the image on the screen
    @param	w	Width of the image on the screen
    @param	h	Height of the image on the screen
    @param	pszBgImg	Filename on the image use for behind progress bar (png/jpeg)
    @param	pszImg	Filename on the image use for progress bar will be scaled bar (png/jpeg)
    @param	outlineColor	Color of the outline
    @param	min	Min value
    @param	max	Max value
    @param	value	Initial value
    @param	userDataOnFocus	Parameter to the Focus (and lost) callback
    @param	onFocus		Focus callback function
    @param	onFocusLost	Focus Lost callback function
	@return	Control Id
		<br>-1 Error
    @warning	the image must be square and its size must be a power of 2.
*/
int GfuiProgressbarCreate(void *scr, int x, int y, int w, int h,
						  const char *pszBgImg, const char *pszImg,
						  const float* outlineColor,
						  float min, float max, float value,
						  void *userDataOnFocus, tfuiCallback onFocus, tfuiCallback onFocusLost)
{
	tGfuiProgressbar *progress;
	tGfuiObject *object;
	tGfuiScreen *screen = (tGfuiScreen*)scr;

	object = (tGfuiObject*)calloc(1, sizeof(tGfuiObject));
	object->widget = GFUI_PROGRESSBAR;
	object->focusMode = (onFocus || onFocusLost) ? GFUI_FOCUS_MOUSE_MOVE : GFUI_FOCUS_NONE;
	object->visible = 1;
	object->id = screen->curId++;

	progress = &(object->u.progressbar);

	progress->fgImage = GfTexReadTexture(pszImg);
	if (!progress->fgImage) {
		free(object);
		return -1;
	}

	progress->bgImage = GfTexReadTexture(pszBgImg);
	if (!progress->bgImage) {
		free(object);
		return -1;
	}

    progress->outlineColor = GfuiColor::build(outlineColor ? outlineColor : gfuiColors[GFUI_PROGRESSCOLOR]);

	progress->min = min;
	progress->max = max;
	if (value > progress->max)
		value = progress->max;
	else if (value < progress->min)
		value = progress->min;
	progress->value = value;

    progress->userDataOnFocus = userDataOnFocus;
    progress->onFocus = onFocus;
    progress->onFocusLost = onFocusLost;

	object->xmin = x;
	object->xmax = x + w;
	object->ymin = y;
	object->ymax = y + h;

	gfuiAddObject(screen, object);

	return object->id;
}

void GfuiProgressbarSetValue(void *scr, int id, float value)
{
    tGfuiObject *curObject;
    tGfuiScreen	*screen = (tGfuiScreen*)scr;

    curObject = screen->objects;
    if (curObject) {
		do {
			curObject = curObject->next;
			if (curObject->id == id) {
				if (curObject->widget == GFUI_PROGRESSBAR) {
					if (value > curObject->u.progressbar.max)
						value = curObject->u.progressbar.max;
					else if (value < curObject->u.progressbar.min)
						value = curObject->u.progressbar.min;
					curObject->u.progressbar.value = value;
				}
				return;
			}
		} while (curObject != screen->objects);
    }
}

void
gfuiReleaseProgressbar(tGfuiObject *obj)
{
	tGfuiProgressbar *progress;

	progress = &(obj->u.progressbar);
	GfTexFreeTexture(progress->fgImage);
	freez(progress->userDataOnFocus);

	free(obj);
}

void
gfuiDrawProgressbar(tGfuiObject *obj)
{
	tGfuiProgressbar *progress;

	progress = &(obj->u.progressbar);

    // glColor4f(1.0, 1.0, 1.0, 0.25);
    // glBegin(GL_QUADS);
    // glVertex2i(obj->xmin, obj->ymin);
    // glVertex2i(obj->xmin, obj->ymax);
    // glVertex2i(obj->xmax, obj->ymax);
    // glVertex2i(obj->xmax, obj->ymin);
    // glEnd();

	// Calculate and draw progress bar
	const float width = obj->xmax - obj->xmin;
	const float range = progress->max - progress->min;
	const float umax = (progress->value - progress->min) / range;
	const float endx = obj->xmin + width*umax;

    glEnable(GL_TEXTURE_2D);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	// Foreground image from min to value
	glBindTexture(GL_TEXTURE_2D, progress->fgImage);

    glBegin(GL_TRIANGLE_STRIP);
	//glColor4f(1.0, 1.0, 1.0, 1.0);
	glTexCoord2f(0.0, 0.0); glVertex2f(obj->xmin, obj->ymin);
	glTexCoord2f(0.0, 1.0); glVertex2f(obj->xmin, obj->ymax);
	glTexCoord2f(umax, 0.0); glVertex2f(endx, obj->ymin);
	glTexCoord2f(umax, 1.0); glVertex2f(endx, obj->ymax);
    glEnd();

	// Background image from value to max
	glBindTexture(GL_TEXTURE_2D, progress->bgImage);

    glBegin(GL_TRIANGLE_STRIP);
	//glColor4f(1.0, 1.0, 1.0, 1.0);
	glTexCoord2f(umax, 0.0); glVertex2f(endx, obj->ymin);
	glTexCoord2f(umax, 1.0); glVertex2f(endx, obj->ymax);
	glTexCoord2f(1.0, 0.0); glVertex2f(obj->xmax, obj->ymin);
	glTexCoord2f(1.0, 1.0); glVertex2f(obj->xmax, obj->ymax);
    glEnd();

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);

	// Draw outline
    glColor4fv(progress->outlineColor.toFloatRGBA());
	glBegin(GL_LINE_STRIP);
    glVertex2i(obj->xmin, obj->ymin);
    glVertex2i(obj->xmin, obj->ymax);
    glVertex2i(obj->xmax, obj->ymax);
    glVertex2i(obj->xmax, obj->ymin);
    glVertex2i(obj->xmin, obj->ymin);
    glEnd();
}
