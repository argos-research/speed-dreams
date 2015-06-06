/***************************************************************************

    file                 : guiimage.cpp
    created              : Wed May  1 10:29:28 CEST 2002
    copyright            : (C) 2001 by Eric Espie
    email                : eric.espie@torcs.org
    version              : $Id: guiimage.cpp 4621 2012-03-29 18:32:32Z pouillot $

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
    		GUI Images management
    @author	<a href=mailto:eric.espie@torcs.org>Eric Espie</a>
    @version	$Id: guiimage.cpp 4621 2012-03-29 18:32:32Z pouillot $
    @ingroup	gui
*/

#include <cstdlib>

#include "tgfclient.h"

#include "gui.h"


/** Create a new static image (source image doesn't need to be a square or of POT sizes).
    This kind of image is not clickable.
    @ingroup	gui
    @param	scr	Screen where to add the label
    @param	x	Position of the left of the image on the screen
    @param	y	Position of the bottom of the image on the screen
    @param	w	Width of the image on the screen
    @param	h	Height of the image on the screen
    @param	name	Filename on the source image (png)
	@param canDeform If true, full X and Y autoscale, otherwise, keep aspect ratio but cut sides or top/bottom to fill target w,h rectangle (default: true)
    @return	Image Id
		<br>-1 Error*/
int GfuiStaticImageCreate(void *scr, int x, int y, int w, int h, const char *name,
						  bool canDeform)
{
	int pow2Width, pow2Height;
	tGfuiImage *image;
	tGfuiObject *object;
	tGfuiScreen *screen = (tGfuiScreen*)scr;

	object = (tGfuiObject*)calloc(1, sizeof(tGfuiObject));
	object->widget = GFUI_IMAGE;
	object->focusMode = GFUI_FOCUS_NONE;
	object->visible = 1;
	object->id = screen->curId++;

	image = &(object->u.image);
	for (int i=0;i<GFUI_MAXSTATICIMAGES;i++)
		image->texture[i] = 0;

	image->activeimage = 0;
	
	image->canDeform = canDeform;
	
	// We don't use returned POT width and height, but passing non NULL pointers
	// for them enforces POT sizes for the loaded texture.
	image->texture[0] =
		GfTexReadTexture(name, &image->srcWidth, &image->srcHeight, &pow2Width, &pow2Height);

	if (!image->texture) {
		free(object);
		return -1;
	}

	object->xmin = x;
	object->xmax = x + w;
	object->ymin = y;
	object->ymax = y + h;

	gfuiAddObject(screen, object);

	return object->id;
}

/** Replace an image by another one (source image doesn't need to be a square or of POT sizes).
    @ingroup	gui
    @param	scr	Screen where the image is displayed
    @param	id	Image Id
    @param	name	Filename of the source image (PNG or JPEG)
    @param	index	Target index for the texture (defaults to 0)
    @return	none
*/
void GfuiStaticImageSet(void *scr, int id, const char *name, unsigned index)
{
	int pow2Width, pow2Height;
	tGfuiObject *curObject;
	tGfuiScreen *screen = (tGfuiScreen*)scr;
	tGfuiImage *image;

	curObject = screen->objects;
	if (curObject) {
		do {
			curObject = curObject->next;
			if (curObject->id == id) {
				if (curObject->widget == GFUI_IMAGE) {
					image = &(curObject->u.image);
					GfTexFreeTexture(image->texture[index]);
					// We don't use returned POT width and height, but passing non NULL pointers
					// for them enforces POT sizes for the loaded texture.
					image->texture[index] =
						GfTexReadTexture(name, &image->srcWidth, &image->srcHeight,
										 &pow2Width, &pow2Height);
				}
			return;
			}
		} while (curObject != screen->objects);
	}
}

/** Set active image by its index.
    @ingroup	gui
    @param	scr	Screen where the image is displayed
    @param	id	Image Id
    @param	index	Target image index
    @return	none
*/
void GfuiStaticImageSetActive(void *scr, int id, int index)
{
	tGfuiObject *curObject;
	tGfuiScreen *screen = (tGfuiScreen*)scr;
	tGfuiImage *image;

	curObject = screen->objects;
	if (curObject) {
		do {
			curObject = curObject->next;
			if (curObject->id == id) {
				if (curObject->widget == GFUI_IMAGE) {
					image = &(curObject->u.image);
					image->activeimage = index;
				}
			return;
			}
		} while (curObject != screen->objects);
	}
}

/** Set "can deform" property.
    @ingroup	gui
    @param	scr	Screen where the image is displayed
    @param	id	Image Id
    @param	canDeform	Target value
    @return	none
*/
void GfuiStaticImageSetDeformable(void *scr, int id, bool canDeform)
{
	tGfuiObject *curObject;
	tGfuiScreen *screen = (tGfuiScreen*)scr;
	tGfuiImage *image;

	curObject = screen->objects;
	if (curObject) {
		do {
			curObject = curObject->next;
			if (curObject->id == id) {
				if (curObject->widget == GFUI_IMAGE) {
					image = &(curObject->u.image);
					image->canDeform = canDeform;
				}
			return;
			}
		} while (curObject != screen->objects);
	}
}

void
gfuiReleaseImage(tGfuiObject *obj)
{
	tGfuiImage	*image;

	image = &(obj->u.image);

	for (int i=0;i<GFUI_MAXSTATICIMAGES;i++)
		GfTexFreeTexture(image->texture[i]);

	free(obj);
}

void
gfuiDrawImage(tGfuiObject *obj)
{
    tGfuiImage	*image;

    image = &(obj->u.image);

	// Prepare texture display.
	glEnable(GL_TEXTURE_2D);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glBindTexture(GL_TEXTURE_2D, image->texture[image->activeimage]);
	glColor4f(1.0, 0.0, 1.0, 1.0);

	// Get real 2^N x 2^P texture size (may have been 0 padded at load time
	// if the original image was not 2^N x 2^P)
	// This 2^N x 2^P stuff is needed by some low-end OpenGL hardware/drivers.
	int texPow2Width = 1, texPow2Height = 1;
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &texPow2Width);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &texPow2Height);

	// Compute the width of the right area and the height of the bottom area
	// of the texture that will not be displayed
	// (We display only the top left rectangle of the 2^N x 2^P texture
	//  that corresponds to the original image).
	GLfloat tx1 = 0.0f;
	GLfloat tx2 = image->srcWidth / (GLfloat)texPow2Width;

	GLfloat ty1 = 1.0f - image->srcHeight / (GLfloat)texPow2Height;
	GLfloat ty2 = 1.0;

	// If the aspect ratio of the image has to be kept, compute the width/height of the symetrical
	// left/right / top/bottom areas of the original image that will need to be clipped
	// in order to keep its aspect ratio.
	if (!image->canDeform)
	{
		const GLfloat rfactor = image->srcWidth * (GLfloat)(obj->ymax - obj->ymin)
			                    / image->srcHeight / (GLfloat)(obj->xmax - obj->xmin);
		
		if (rfactor >= 1.0f) {
			// If aspect ratio of image widget is smaller than source image's one, "cut off" sides.
			const GLfloat tdx = image->srcWidth * (rfactor - 1.0f) / texPow2Width / 2.0f;
			tx1 += tdx;
			tx2 -= tdx;
		} else {
			// If aspect ratio of view is larger than image's one, "cut off" top and bottom.
			const GLfloat tdy = image->srcHeight * (1.0f / rfactor - 1.0f) / texPow2Height / 2.0f;
			ty1 += tdy;
			ty2 -= tdy;
		}
	}

// 	GfLogDebug("gfuiDrawImage(glId=%lu) : screen(xl=%d, xr=%d, yb=%d, yt=%d), "
// 			   "image(w=%d, h=%d, w2=%d, h2=%d) ...\n",
// 			   obj, obj->xmin, obj->xmax, obj->ymin, obj->ymax,
// 			   image->srcWidth, image->srcHeight, texPow2Width, texPow2Height);
// 	GfLogDebug("... tx1=%.2f, tx2=%.2f, ty1=%.2f, ty2=%.2f\n",
// 			   tx1, tx2, ty1, ty2);

	// Display texture.
	glBegin(GL_TRIANGLE_STRIP);
	
	glTexCoord2f(tx1, ty1); glVertex2f(obj->xmin, obj->ymin);
	glTexCoord2f(tx1, ty2); glVertex2f(obj->xmin, obj->ymax);
	glTexCoord2f(tx2, ty1); glVertex2f(obj->xmax, obj->ymin);
	glTexCoord2f(tx2, ty2); glVertex2f(obj->xmax, obj->ymax);

	glEnd();

	glDisable(GL_TEXTURE_2D);
}
