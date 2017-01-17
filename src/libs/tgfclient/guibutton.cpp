/***************************************************************************
                             guibutton.cpp                             
                             -------------------                                         
    created              : Fri Aug 13 22:18:21 CEST 1999
    copyright            : (C) 1999 by Eric Espie                         
    email                : torcs@free.fr   
    version              : $Id: guibutton.cpp 5839 2014-11-15 15:05:25Z wdbee $                                  
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
    		GUI Buttons Management.
    @author	<a href=mailto:torcs@free.fr>Eric Espie</a>
    @version	$Id: guibutton.cpp 5839 2014-11-15 15:05:25Z wdbee $
    @ingroup	gui
*/

#include "gui.h"

// Text buttons padding (outside of the text label)
static int NHTxtPadding = 10;
static int NVTxtPadding = 5;

// Image buttons padding (outside of the image)
static int NHImgPadding = 0;
static int NVImgPadding = 0;

void
gfuiInitButton(void)
{
	char path[512];

	// Get default layout properties from the screen config file.
	// 1) Tips.
	sprintf(path, "%s%s", GfLocalDir(), GFSCR_CONF_FILE);
	void* hparmScr = GfParmReadFile(path, GFPARM_RMODE_STD | GFPARM_RMODE_CREAT);

	// 2) Text buttons.
	sprintf(path, "%s/%s", GFSCR_SECT_MENUSETTINGS, GFSCR_SECT_TEXTBUTTON);
	NHTxtPadding = (int)GfParmGetNum(hparmScr, path, GFSCR_ATT_HPADDING, 0, 10.0);
	NVTxtPadding = (int)GfParmGetNum(hparmScr, path, GFSCR_ATT_VPADDING, 0,  5.0);

	// 3) Image buttons.
	sprintf(path, "%s/%s", GFSCR_SECT_MENUSETTINGS, GFSCR_SECT_IMAGEBUTTON);
	NHImgPadding = (int)GfParmGetNum(hparmScr, path, GFSCR_ATT_HPADDING, 0, 0.0);
	NVImgPadding = (int)GfParmGetNum(hparmScr, path, GFSCR_ATT_VPADDING, 0, 0.0);
}

/** Initialize an image button.
    @ingroup	gui
    @param	button		The button to initialize
    @param	disabled	filename of the image when the button is disabled
    @param	enabled		filename of the image when the button is enabled
    @param	focused		filename of the image when the button is focused
    @param	pushed		filename of the image when the button is pushed
    @param	x		X position on screen (0 = left)
    @param	y		Y position on screen (0 = bottom)
    @param	width		Width on the screen (0 = image width)
    @param	height		Height on the screen (0 = image height)
    @param	mirror		Left/right + up/down mirror when drawing the images
                        <br>(Or-combination of GFUI_MIRROR_*)
    @param	mouse		Mouse behavior:
                        <br>GFUI_MOUSE_UP Action performed when the mouse right button is released
                        <br>GFUI_MOUSE_DOWN Action performed when the mouse right button is pushed
    @param	userDataOnPush	Parameter to the Push callback
    @param	onPush		Push callback function
    @param	userDataOnFocus	Parameter to the Focus (and lost) callback
    @param	onFocus		Focus callback function
    @param	onFocusLost	Focus Lost callback function
    @return	Button Id
		<br>-1 Error
 */
void
gfuiGrButtonInit(tGfuiGrButton* button, const char *disabled, const char *enabled,
				 const char *focused, const char *pushed,
				 int x, int y, int width, int height, int mirror, int mouse,
				 void *userDataOnPush, tfuiCallback onPush, 
				 void *userDataOnFocus, tfuiCallback onFocus, tfuiCallback onFocusLost)
{
    button->state = GFUI_BTN_RELEASED;

	int w, h;
    button->disabled = GfTexReadTexture(disabled, &w, &h);
    button->enabled = GfTexReadTexture(enabled, &w, &h);
    button->focused = GfTexReadTexture(focused, &w, &h);
    button->pushed = GfTexReadTexture(pushed, &w, &h);
	
	// Warning: All the images are supposed to be the same size.
	button->width = width <= 0 ? w : width;
	button->height = height <= 0 ? h : height;
	button->x = x;
	button->y = y;
	button->mirror = mirror;
    button->buttonType = GFUI_BTN_PUSH;
    button->mouseBehaviour = mouse;

    button->userDataOnPush = userDataOnPush;
    button->onPush = onPush;
    button->userDataOnFocus = userDataOnFocus;
    button->onFocus = onFocus;
    button->onFocusLost = onFocusLost;
}

/** Add an image button to a screen.
    @ingroup	gui
    @param	scr		Screen
    @param	disabled	filename of the image when the button is disabled
    @param	enabled		filename of the image when the button is enabled
    @param	focused		filename of the image when the button is focused
    @param	pushed		filename of the image when the button is pushed
    @param	x		X position on screen (0 = left)
    @param	y		Y position on screen (0 = bottom)
    @param	width		Width on the screen (0 = image width + h-padding)
    @param	height		Height on the screen (0 = image height + v-padding)
    @param	mirror		Left/right + up/down mirror when drawing the images
                        <br>(Or-combination of GFUI_MIRROR_*)
    @param	padding		Padding flag
    @param	mouse		Mouse behavior:
                        <br>GFUI_MOUSE_UP Action performed when the mouse right button is released
                        <br>GFUI_MOUSE_DOWN Action performed when the mouse right button is pushed
    @param	userDataOnPush	Parameter to the Push callback
    @param	onPush		Push callback function
    @param	userDataOnFocus	Parameter to the Focus (and lost) callback
    @param	onFocus		Focus callback function
    @param	onFocusLost	Focus Lost callback function
    @return	Button Id
		<br>-1 Error
 */

int
GfuiGrButtonCreate(void *scr, const char *disabled, const char *enabled,
				   const char *focused, const char *pushed,
				   int x, int y, int width, int height, int mirror, bool padding, int mouse,
				   void *userDataOnPush, tfuiCallback onPush, 
				   void *userDataOnFocus, tfuiCallback onFocus, tfuiCallback onFocusLost)
{
    tGfuiScreen* screen = (tGfuiScreen*)scr;
    
    tGfuiObject* object = (tGfuiObject*)calloc(1, sizeof(tGfuiObject));
    object->widget = GFUI_GRBUTTON;
    object->focusMode = GFUI_FOCUS_MOUSE_MOVE;
    object->id = screen->curId++;
    object->visible = 1;

	const int hPadding = padding ? NHImgPadding : 0;
	const int vPadding = padding ? NVImgPadding : 0;
	
    tGfuiGrButton* button = &(object->u.grbutton);
	gfuiGrButtonInit(button, disabled, enabled, focused, pushed,
					 x + hPadding, y + vPadding,
					 width - 2 * hPadding, height - 2 * vPadding, mirror, mouse,
					 userDataOnPush, onPush, userDataOnFocus, onFocus, onFocusLost);

	object->xmin = x;
	object->xmax = x + button->width + 2 * hPadding;
	object->ymin = y;
	object->ymax = y + button->height + 2 * vPadding;

    gfuiAddObject(screen, object);
	
    return object->id;
}

/** Add a state button to a screen.
    @ingroup	gui
    @param	scr		Screen
    @param	text		Button label
    @param	font		Font id
    @param	x		X position on screen
    @param	y		Y position on screen (0 = bottom)
    @param	width		width of the button (0 = text size)
    @param	textAlign		Horizontal text alignment inside the button:
    			<br>GFUI_ALIGN_HR	right
    			<br>GFUI_ALIGN_HC	center
    			<br>GFUI_ALIGN_HL	left
    @param	mouse		Mouse behavior:
    			<br>GFUI_MOUSE_UP Action performed when the mouse right button is released
				<br>GFUI_MOUSE_DOWN Action performed when the mouse right button is pushed
    @param	userDataOnPush	Parameter to the Push callback
    @param	onPush		Push callback function
    @param	userDataOnFocus	Parameter to the Focus (and lost) callback
    @param	onFocus		Focus callback function
    @param	onFocusLost	Focus Lost callback function
    @return	Button Id
		<br>-1 Error
 */
int
GfuiButtonStateCreate(void *scr, const char *text, int font, int x, int y, int width,
					  int textHAlign, int mouse,
					  void *userDataOnPush, tfuiCallback onPush, 
					  void *userDataOnFocus, tfuiCallback onFocus, tfuiCallback onFocusLost)
{
	int id = GfuiButtonCreate(scr, text, font, x, y, width, textHAlign, mouse,
							  userDataOnPush, onPush, userDataOnFocus, onFocus, onFocusLost);

	tGfuiScreen	*screen = (tGfuiScreen*)scr;
	tGfuiObject *curObject = screen->objects;
	if (curObject) {
		do {
			curObject = curObject->next;
			if (curObject->id == id) {
				if (curObject->widget == GFUI_BUTTON) {
					tGfuiButton	*button = &(curObject->u.button);
					button->buttonType = GFUI_BTN_STATE;
				}
			return id;
			}
		} while (curObject != screen->objects);
	}
	return id;
}//GfuiButtonStateCreate


/** Add a text button to a screen.
    @ingroup	gui
    @param	scr		Screen
    @param	text		Button label
    @param	font		Font id
    @param	x		X position on screen
    @param	y		Y position on screen (0 = bottom)
    @param	width		width of the button (0 = text size)
    @param	textHAlign		Horizontal text alignment inside the button:
    			<br>GFUI_ALIGN_HR	right
    			<br>GFUI_ALIGN_HC	center
    			<br>GFUI_ALIGN_HL	left
    @param	mouse		Mouse behavior:
    			<br>GFUI_MOUSE_UP Action performed when the mouse right button is released
				<br>GFUI_MOUSE_DOWN Action performed when the mouse right button is pushed
    @param	userDataOnPush	Parameter to the Push callback
    @param	onPush		Push callback function
    @param	userDataOnFocus	Parameter to the Focus (and lost) callback
    @param	onFocus		Focus callback function
    @param	onFocusLost	Focus Lost callback function
    @return	Button Id
		<br>-1 Error
 */
int
GfuiButtonCreate(void *scr, const char *text, int font, int x, int y, int width, int textHAlign,
				 int mouse, void *userDataOnPush, tfuiCallback onPush, 
				 void *userDataOnFocus, tfuiCallback onFocus, tfuiCallback onFocusLost)
{
    tGfuiButton	*button;
    tGfuiObject	*object;
    tGfuiScreen	*screen = (tGfuiScreen*)scr;

    object = (tGfuiObject*)calloc(1, sizeof(tGfuiObject));
    object->widget = GFUI_BUTTON;
    object->focusMode = GFUI_FOCUS_MOUSE_MOVE;
    object->id = screen->curId++;
    object->visible = 1;
    
    button = &(object->u.button);
    button->state = GFUI_BTN_RELEASED;
    button->userDataOnPush = userDataOnPush;
    button->onPush = onPush;
    button->userDataOnFocus = userDataOnFocus;
    button->onFocus = onFocus;
    button->onFocusLost = onFocusLost;
    button->mouseBehaviour = mouse;
    button->buttonType = GFUI_BTN_PUSH;
    button->bShowBox = true;

    button->disabled = 0;
    button->enabled = 0;
    button->focused = 0;
    button->pushed = 0;

    button->bgColor[0] = GfuiColor::build(GFUI_BGBTNDISABLED);
    button->bgColor[1] = GfuiColor::build(GFUI_BGBTNENABLED);
    button->bgColor[2] = GfuiColor::build(GFUI_BGBTNCLICK);
    button->bgFocusColor[0] = GfuiColor::build(GFUI_BGBTNDISABLED);
    button->bgFocusColor[1] = GfuiColor::build(GFUI_BGBTNFOCUS);
    button->bgFocusColor[2] = GfuiColor::build(GFUI_BGBTNCLICK);
    button->fgColor[0] = GfuiColor::build(GFUI_BTNDISABLED);
    button->fgColor[1] = GfuiColor::build(GFUI_BTNENABLED);
    button->fgColor[2] = GfuiColor::build(GFUI_BTNCLICK);
    button->fgFocusColor[0] = GfuiColor::build(GFUI_BTNDISABLED);
    button->fgFocusColor[1] = GfuiColor::build(GFUI_BTNFOCUS);
	button->fgFocusColor[2] = GfuiColor::build(GFUI_BTNCLICK);

	button->imgX = 0;
	button->imgY = 0;
	button->imgWidth = 0;
	button->imgHeight = 0;

	gfuiLabelInit(&button->label, text, 0, x + NHTxtPadding, y + NVTxtPadding,
				  width - 2 * NHTxtPadding, textHAlign, font, 0, 0, 0, 0, 0, 0, 0);
	
    if (width <= 0)
		width = button->label.width + 2*NHTxtPadding;

	object->xmin = x;
	object->ymin = y;
	object->xmax = x + width;
	object->ymax = y + gfuiFont[font]->getHeight() + 2*NVTxtPadding;

	gfuiAddObject(screen, object);
	
    return object->id;
}

/** Change the label of a button.
    @ingroup	gui
    @param	scr	Screen
    @param	id	Button Id
    @param	text	New label of the button
 */
void
GfuiButtonSetText(void *scr, int id, const char *text)
{
	tGfuiObject *object = gfuiGetObject(scr, id);
	if (object && object->widget == GFUI_BUTTON)
		gfuiLabelSetText(&(object->u.button.label), text);
}//GfuiButtonSetText


/** Show/hide the framing box around a button.
    @ingroup	gui
    @param	scr	Screen
    @param	id	Button Id
    @param	bShow	True-Show frame, False-Dont show
 */
void
GfuiButtonShowBox(void *scr, int id, bool bShow)
{
	tGfuiObject *object = gfuiGetObject(scr, id);
	if (object && object->widget == GFUI_BUTTON)
		object->u.button.bShowBox = bShow;
}//GfuiButtonShowBox


/** Change the foregound colours of a button.
    @ingroup	gui
    @param	scr	Screen
    @param	id	Button Id
    @param	color	New colour
    @param	focusColor	New focused-state colour
    @param	pushColor	New pushed-state colour
 */
void
GfuiButtonSetColors(void *scr, int id,
					const GfuiColor& color, const GfuiColor& focusColor, const GfuiColor& pushColor)
{
	tGfuiObject *object = gfuiGetObject(scr, id);
	if (object && object->widget == GFUI_BUTTON)
	{
		if (color.alpha)
			object->u.button.fgColor[1] = color;
		if (focusColor.alpha)
			object->u.button.fgFocusColor[1] = focusColor;
		if (pushColor.alpha)
			object->u.button.fgFocusColor[2] = pushColor;
	}
}//GfuiButtonSetColor

/** Define text button images for different states.
    @ingroup	gui
    @param	scr	Screen
    @param	id	Button Id
    @param	x		X position on screen
    @param	y		Y position on screen (0 = bottom)
    @param	w		width of the button image
    @param	h		height of the button image
    @param	disableFile	file that holds the disabled button image version
    @param	enableFile	file that holds the enabled button image version
    @param	focusedFile	file that holds the focused button image version
    @param	pushedFile	file that holds the pushed button image version
*/
void
GfuiButtonSetImage(void *scr, int id, int x, int y, int w, int h,
				   const char *disabledFile, const char *enabledFile,
				   const char *focusedFile, const char *pushedFile)
{
	GLuint disabled = 0;
	GLuint enabled = 0;
	GLuint focused = 0;
	GLuint pushed = 0;

	if (disabledFile && strlen(disabledFile) > 0)
		disabled = GfTexReadTexture(disabledFile);
	if (enabledFile && strlen(enabledFile) > 0)
		enabled = GfTexReadTexture(enabledFile);
	if (focusedFile && strlen(focusedFile) > 0)
		focused = GfTexReadTexture(focusedFile);
	if (pushedFile && strlen(pushedFile) > 0)
		pushed = GfTexReadTexture(pushedFile);

	tGfuiObject *object = gfuiGetObject(scr, id);
	if (object && object->widget == GFUI_BUTTON)
	{
		object->u.button.disabled = disabled;
		object->u.button.enabled = enabled;
		object->u.button.focused = focused;
		object->u.button.pushed = pushed;
		object->u.button.imgX = x;
		object->u.button.imgY = y;
		object->u.button.imgWidth = w;
		object->u.button.imgHeight = h;
	}//if curObject
}//GfuiButtonSetImage


/** Actually draw the given text button object.
    @ingroup	gui
    @param	obj	button object to draw
 */
void
gfuiDrawButton(tGfuiObject *obj)
{
	GfuiColor fgColor;
	GfuiColor bgColor;

	// Determine the fore/background colors, according to the state/focus.
	tGfuiButton *button = &(obj->u.button);
	if (obj->state == GFUI_DISABLE)
		button->state = GFUI_BTN_DISABLE;

	if (obj->focus)
	{
		fgColor = button->fgFocusColor[button->state];
		bgColor = button->bgFocusColor[button->state];
	}
	else
	{
		fgColor = button->fgColor[button->state];
		bgColor = button->bgColor[button->state];
	}//if obj->focus

	// Draw the bounding box / background if specified and visible.
	if (bgColor.alpha && button->bShowBox)
	{
		glColor4fv(bgColor.toFloatRGBA());
		glBegin(GL_QUADS);
		glVertex2i(obj->xmin, obj->ymin);
		glVertex2i(obj->xmin, obj->ymax);
		glVertex2i(obj->xmax, obj->ymax);
		glVertex2i(obj->xmax, obj->ymin);
		glEnd();
		glColor4fv(fgColor.toFloatRGBA());
		glBegin(GL_LINE_STRIP);
		glVertex2i(obj->xmin, obj->ymin);
		glVertex2i(obj->xmin, obj->ymax);
		glVertex2i(obj->xmax, obj->ymax);
		glVertex2i(obj->xmax, obj->ymin);
		glVertex2i(obj->xmin, obj->ymin);
		glEnd();	
	}//if (bgColor && button->bShowBox)

	// Draw the image if any, according to the state/focus.
	GLuint img = 0;
	if (obj->state == GFUI_DISABLE)
		img = button->disabled;
	else if (button->state == GFUI_BTN_PUSHED)
		img = button->pushed;
	else if (obj->focus)
		img = button->focused;
	else
		img = button->enabled;

	if (img)
	{
		const int x1 = obj->xmin + button->imgX;
		const int x2 = x1 + button->imgWidth;
		const int y1 = obj->ymin + button->imgY;
		const int y2 = y1 + button->imgHeight;

		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glColor3f(1.0, 1.0, 1.0); //set color to mix with image

		glEnable(GL_BLEND);
		glEnable(GL_TEXTURE_2D);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glBindTexture(GL_TEXTURE_2D, img);
		glBegin(GL_QUADS);

		glTexCoord2f(0.0, 0.0);
		glVertex2i(x1, y1);

		glTexCoord2f(0.0, 1.0);
		glVertex2i(x1, y2);

		glTexCoord2f(1.0, 1.0);
		glVertex2i(x2, y2);

		glTexCoord2f(1.0, 0.0);
		glVertex2i(x2, y1);

		glEnd();
		glDisable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	// Draw the label.
	tGfuiLabel *label = &(button->label);
	gfuiLabelDraw(label, fgColor);
}//gfuiDrawButton


/** Actually draw the given image button.
    @ingroup	gui
    @param	obj	graphical button to draw
 */
void
gfuiGrButtonDraw(tGfuiGrButton *button, int state, int focus)
{
	// Determine the image to draw, according to the state/focus.
	GLuint img;
	if (state == GFUI_DISABLE)
		img = button->disabled;
	else if (button->state == GFUI_BTN_PUSHED)
		img = button->pushed;
	else if (focus)
		img = button->focused;
	else
		img = button->enabled;

	// Draw the image.
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glColor3f(1.0, 1.0, 1.0); //set color to mix with image

	glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	glBindTexture(GL_TEXTURE_2D, img);
	glBegin(GL_QUADS);

	const float h = (button->mirror & GFUI_MIRROR_HORI) ? 1.0 : 0.0;
	const float v = (button->mirror & GFUI_MIRROR_VERT) ? 1.0 : 0.0;
	
	//glTexCoord2f(0.0, 0.0);
	glTexCoord2f(v, h); //H : 0, 1; V : 1, 0 ; HV : 1, 1
	glVertex2i(button->x, button->y);
	
	//glTexCoord2f(0.0, 1.0);
	glTexCoord2f(v, 1.0 - h); // H : 0, 0; V : 1, 1 ; HV : 1, 0
	glVertex2i(button->x, button->y + button->height);
	
	//glTexCoord2f(1.0, 1.0);
	glTexCoord2f(1.0 - v, 1.0 - h); // H : 1, 0; V : 0, 1 ; HV : 0, 0
	glVertex2i(button->x + button->width, button->y + button->height);
	
	//glTexCoord2f(1.0, 0.0);
	glTexCoord2f(1.0 - v, h); // H : 1, 1; V : 0, 0 ; HV : 0, 1
	glVertex2i(button->x + button->width, button->y);

	glEnd();
	glDisable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);

/* What is this supposed to do ?
	int sw, sh, vw, vh;
	GfScrGetSize(&sw, &sh, &vw, &vh);
	glRasterPos2i(obj->xmin, obj->ymin);
	glPixelZoom((float)vw / (float)GfuiScreen->width, (float)vh / (float)GfuiScreen->height);
	glDrawPixels(button->width, button->height, GL_RGBA, GL_UNSIGNED_BYTE, img);
*/
}

/** Actually draw the given image button object.
    @ingroup	gui
    @param	obj	button object to draw
 */
void
gfuiDrawGrButton(tGfuiObject *obj)
{
	gfuiGrButtonDraw(&obj->u.grbutton, obj->state, obj->focus);
}//gfuiDrawGrButton

/** Check if the mouse is in the given graphical button
    @ingroup	gui
    @param	button	graphical button object to check
 */
bool
gfuiGrButtonMouseIn(tGfuiGrButton *button)
{
    return
		GfuiMouse.X >= button->x && GfuiMouse.X <= button->x + button->width
		&& GfuiMouse.Y >= button->y && GfuiMouse.Y <= button->y + button->height;
}

/** Handles the graphical button action.
    @ingroup	gui
    @param	action	action
 */
void
gfuiGrButtonAction(int action) {

	if (GfuiScreen->hasFocus->state == GFUI_DISABLE) 
		return;

	tGfuiGrButton	*button = &(GfuiScreen->hasFocus->u.grbutton);

	switch (button->buttonType) {
		case GFUI_BTN_PUSH:
			if (action == 2) { /* enter key */
				if (button->onPush) {
					button->onPush(button->userDataOnPush);
				}
			} else if (action == 1) { /* mouse up */
				if (button->state != GFUI_BTN_RELEASED) {
					button->state = GFUI_BTN_RELEASED;
					if (button->mouseBehaviour == GFUI_MOUSE_UP) {
						if (button->onPush) {
							button->onPush(button->userDataOnPush);
						}
					}
				}
			} else { /* mouse down */
				if (button->state != GFUI_BTN_PUSHED) {
					button->state = GFUI_BTN_PUSHED;
					if (button->mouseBehaviour == GFUI_MOUSE_DOWN) {
						if (button->onPush) {
							button->onPush(button->userDataOnPush);
						}
					}
				}
			}
		break;	//GFUI_BTN_PUSH

		case GFUI_BTN_STATE:
			if (action == 2) { /* enter key */
				if (button->state == GFUI_BTN_RELEASED) {
					button->state = GFUI_BTN_PUSHED;
					if (button->onPush) {
						button->onPush(button->userDataOnPush);
					}
				} else {
					button->state = GFUI_BTN_RELEASED;
				}
			} else if (action == 1) { /* mouse up */
				if (button->mouseBehaviour == GFUI_MOUSE_UP) {
					if (button->state == GFUI_BTN_RELEASED) {
						button->state = GFUI_BTN_PUSHED;
						if (button->onPush) {
							button->onPush(button->userDataOnPush);
						}
					} else {
						button->state = GFUI_BTN_RELEASED;
					}
				}
			} else { /* mouse down */
				if (button->mouseBehaviour == GFUI_MOUSE_DOWN) {
					if (button->state == GFUI_BTN_RELEASED) {
						button->state = GFUI_BTN_PUSHED;
						if (button->onPush) {
							button->onPush(button->userDataOnPush);
						}
					} else {
						button->state = GFUI_BTN_RELEASED;
					}
				}
			}
		break;	//GFUI_BTN_STATE
	}//switch
}//guifGrButtonAction


/** Handles the button action.
    @ingroup	gui
    @param	action	action
 */
void
gfuiButtonAction(int action)
{
	if (GfuiScreen->hasFocus->state == GFUI_DISABLE) 
		return;

	tGfuiButton	*button = &(GfuiScreen->hasFocus->u.button);

	switch (button->buttonType) {
		case GFUI_BTN_PUSH:
			if (action == 2) { /* enter key */
				if (button->onPush) {
					button->onPush(button->userDataOnPush);
				}
			} else if (action == 1) { /* mouse up */
				button->state = GFUI_BTN_RELEASED;
				if (button->mouseBehaviour == GFUI_MOUSE_UP) {
					if (button->onPush) {
						button->onPush(button->userDataOnPush);
					}
				}
			} else { /* mouse down */
				button->state = GFUI_BTN_PUSHED;
				if (button->mouseBehaviour == GFUI_MOUSE_DOWN) {
					if (button->onPush) {
						button->onPush(button->userDataOnPush);
					}
				}
			}
		break;	//GFUI_BTN_PUSH

		case GFUI_BTN_STATE:
			if (action == 2) { /* enter key */
				if (button->state == GFUI_BTN_RELEASED) {
					button->state = GFUI_BTN_PUSHED;
					if (button->onPush) {
						button->onPush(button->userDataOnPush);
					}
				} else {
					button->state = GFUI_BTN_RELEASED;
				}
			} else if (action == 1) { /* mouse up */
				if (button->mouseBehaviour == GFUI_MOUSE_UP) {
					if (button->state == GFUI_BTN_RELEASED) {
						button->state = GFUI_BTN_PUSHED;
						if (button->onPush) {
							button->onPush(button->userDataOnPush);
						}
					} else {
						button->state = GFUI_BTN_RELEASED;
					}
				}
			} else { /* mouse down */
				if (button->mouseBehaviour == GFUI_MOUSE_DOWN) {
					if (button->state == GFUI_BTN_RELEASED) {
						button->state = GFUI_BTN_PUSHED;
						if (button->onPush) {
							button->onPush(button->userDataOnPush);
						}
					} else {
						button->state = GFUI_BTN_RELEASED;
					}
				}
			}
		break;	//GFUI_BTN_STATE
	}//switch
}//gfuiButtonAction


/** Releases all resources connected with a button.
    @ingroup	gui
    @param	obj	button object to release
 */
void
gfuiReleaseButton(tGfuiObject *obj)
{
	tGfuiButton *button = &(obj->u.button);

	GfTexFreeTexture(button->disabled);
	GfTexFreeTexture(button->enabled);
	GfTexFreeTexture(button->focused);
	GfTexFreeTexture(button->pushed);

	tGfuiLabel *label = &(button->label);

	freez(button->userDataOnFocus);
	free(label->text);
	free(obj);
}//gfuiReleaseButton


/** Releases all resources connected with a graphical button.
    @ingroup	gui
    @param	obj	button object to release
 */
void
gfuiReleaseGrButton(tGfuiObject *obj)
{
	tGfuiGrButton	*button = &(obj->u.grbutton);

	GfTexFreeTexture(button->disabled);
	GfTexFreeTexture(button->enabled);
	GfTexFreeTexture(button->focused);
	GfTexFreeTexture(button->pushed);

	freez(button->userDataOnFocus);
	free(obj);
}//gfuiReleaseGrButton
