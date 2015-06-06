/***************************************************************************
                        guilabel.cpp -- labels management                           
                             -------------------                                         
    created              : Fri Aug 13 22:22:12 CEST 1999
    copyright            : (C) 1999 by Eric Espie                         
    email                : torcs@free.fr   
    version              : $Id: guilabel.cpp 4408 2012-01-14 10:48:57Z wdbee $                                  
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
    		GUI labels management.
    @author	<a href=mailto:torcs@free.fr>Eric Espie</a>
    @version	$Id: guilabel.cpp 4408 2012-01-14 10:48:57Z wdbee $
    @ingroup	gui
*/

#include <cstdlib>
#include <cstring>

#include "gui.h"
#include "guimenu.h"

#include "portability.h"


static int NTipX = 10;
static int NTipY = 10;
static int NTipWidth = 620;
static int NTipFontId = GFUI_FONT_SMALL;
static int NTipAlign = GFUI_ALIGN_HC;


void
gfuiInitLabel(void)
{
	char path[512];

	// Get tip layout properties from the screen config file.
	sprintf(path, "%s%s", GfLocalDir(), GFSCR_CONF_FILE);
	void* hparmScr = GfParmReadFile(path, GFPARM_RMODE_STD | GFPARM_RMODE_CREAT);

	sprintf(path, "%s/%s", GFSCR_SECT_MENUSETTINGS, GFSCR_SECT_TIP);
	NTipX = (int)GfParmGetNum(hparmScr, path, GFSCR_ATT_X, 0, 10.0);
	NTipY = (int)GfParmGetNum(hparmScr, path, GFSCR_ATT_Y, 0, 10.0);
	NTipWidth = (int)GfParmGetNum(hparmScr, path, GFSCR_ATT_WIDTH, 0, 620.0);
	NTipFontId = gfuiMenuGetFontId(GfParmGetStr(hparmScr, path, GFSCR_ATT_FONT, "small"));
	NTipAlign = gfuiMenuGetAlignment(GfParmGetStr(hparmScr, path, GFSCR_ATT_ALIGN, "center"));

	GfParmReleaseHandle(hparmScr);
}

/** Initialize a label
    @ingroup	gui
    @param	label	The label to initialize
    @param	text	Text to display
    @param	maxlen	Maximum length of the displayed text (used when the label is changed)
                    <br>0 for the text length.
    @param	x	Position of the label on the screen (pixels)
    @param	y	Position of the label on the screen (pixels)
    @param	width	Width of the label on the screen (pixels, for alignment maintenance)
    @param	align	Horizontal alignment:
    			<br>GFUI_ALIGN_HR	right
    			<br>GFUI_ALIGN_HC	center
    			<br>GFUI_ALIGN_HL	left
    @param	width	Width (pixels) of the display bounding box
                    <br>0 for the actual text width (from the font specs).
    @param	font	Font id
    @param	bgColor	Pointer to static RGBA background color array
                    <br>0 for gfuiColors[GFUI_BGCOLOR] 
    @param	fgColor	Pointer on static RGBA foreground color array
                    <br>0 for gfuiColors[GFUI_LABELCOLOR] 
    @param	bgFocusColor	Pointer to static RGBA focused background color array
                    <br>0 for bgColor
    @param	fgFocusColor	Pointer on static RGBA focused foreground color array
                    <br>0 for fgColor
    @param	userDataOnFocus	User data given to the onFocus[Lost] call back functions
    @param	onFocus	Call back function called when getting the focus
    @param	onFocusLost	Call back function called when loosing the focus
    @return	None
 */
void
gfuiLabelInit(tGfuiLabel *label, const char *text, int maxlen,
			  int x, int y, int width, int align, int font,
			  const float *bgColor, const float *fgColor,
			  const float *bgFocusColor, const float *fgFocusColor,
			  void *userDataOnFocus, tfuiCallback onFocus, tfuiCallback onFocusLost)
{
    if (maxlen <= 0)
		maxlen = strlen(text);
    label->text = (char*)calloc(maxlen+1, sizeof(char));
    strncpy(label->text, text, maxlen);
    label->maxlen = maxlen;
    
    label->bgColor = GfuiColor::build(bgColor ? bgColor : gfuiColors[GFUI_BGCOLOR]);
    label->fgColor = GfuiColor::build(fgColor ? fgColor : gfuiColors[GFUI_LABELCOLOR]);
    label->bgFocusColor = bgFocusColor ? GfuiColor::build(bgFocusColor) : label->bgColor;
    label->fgFocusColor = fgFocusColor ? GfuiColor::build(fgFocusColor) : label->fgColor;
	
    label->font = gfuiFont[font];
    if (width <= 0)
		width = gfuiFont[font]->getWidth(text);
	label->width = width;
	label->align = align;
	label->x = x;
	label->y = y;
	
    label->userDataOnFocus = userDataOnFocus;
    label->onFocus = onFocus;
    label->onFocusLost = onFocusLost;
	
	// GfLogDebug("gfuiLabelInit('%s', font %d) : height=%d\n", text, font, gfuiFont[font]->getHeight());
}

/** Create a new label
    @ingroup	gui
    @param	scr	Screen where to add the label
    @param	text	Text of the label
    @param	font	Font id
    @param	x	Position of the label on the screen
    @param	y	Position of the label on the screen
    @param	align	Horizontal alignment:
    			<br>GFUI_ALIGN_HR	right
    			<br>GFUI_ALIGN_HC	center
    			<br>GFUI_ALIGN_HL	left
    @param	maxlen	Maximum length of the button string (used when the label is changed)
    			<br>0 for the text length.
    @param	fgColor	Pointer on static RGBA color array (0 => default)
    @param	fgFocusColor	Pointer on static RGBA focused color array (0 => fgColor)
    @param	userDataOnFocus	User data given to the onFocus[Lost] call back functions
    @param	onFocus	Call back function called when getting the focus
    @param	onFocusLost	Call back function called when loosing the focus
    @return	label Id
    @see	GfuiSetLabelText
 */
int 
GfuiLabelCreate(void *scr, const char *text, int font, int x, int y,
				int width, int align, int maxlen,
				const float *fgColor, const float *fgFocusColor,
				void *userDataOnFocus, tfuiCallback onFocus, tfuiCallback onFocusLost)
{
    tGfuiLabel	*label;
    tGfuiObject	*object;
	tGfuiScreen	*screen = (tGfuiScreen*)scr;

    object = (tGfuiObject*)calloc(1, sizeof(tGfuiObject));
    object->widget = GFUI_LABEL;
    object->focusMode = (onFocus || onFocusLost) ? GFUI_FOCUS_MOUSE_MOVE : GFUI_FOCUS_NONE;
    object->visible = 1;
    object->id = screen->curId++;
    
    label = &(object->u.label);
	gfuiLabelInit(label, text, maxlen, x, y, width, align, font,
				  screen->bgColor.toFloatRGBA(), fgColor,
				  screen->bgColor.toFloatRGBA(), fgFocusColor, 
				  userDataOnFocus, onFocus, onFocusLost);

	width = label->width;
	const int height = gfuiFont[font]->getHeight();
	
	object->xmin = x;
	object->ymin = y;

	object->xmax = object->xmin + width;
	object->ymax = object->ymin + height;

    gfuiAddObject(screen, object);

    return object->id;
}

/** Add a Tip (generally associated with a button).
    @param	scr	Screen where to add the label
    @param	text	Text of the label
    @param	maxlen	Maximum length of the button string (used when the label is changed)
    @return	label Id
    @see	GfuiSetLabelText
 */
int
GfuiTipCreate(void *scr, const char *text, int maxlen)
{
    return GfuiLabelCreate(scr, text, NTipFontId, NTipX, NTipY, NTipWidth,
						   NTipAlign, maxlen, gfuiColors[GFUI_TIPCOLOR]);
}

/** Change the text of a label.
    @ingroup	gui
    @param	label	Label itself
    @param	obj	Label object
    @param	text	Text of the label
    @warning	The maximum length is set at the label creation
    @see	GfuiAddLabel
 */
void
gfuiLabelSetText(tGfuiLabel *label, const char *text)
{
    if (!text)
		return;

	// Reallocate label->text if maxlen is nul (in case label->text is empty).
 	if (label->maxlen <= 0)
	{
		free(label->text);
		label->maxlen = strlen(text);
		label->text = (char*)calloc(label->maxlen+1, sizeof(char));
	}
	
	// Update the text.
	strncpy(label->text, text, label->maxlen);
}

/** Change the text of a label.
    @ingroup	gui
    @param	scr	Screen of the label
    @param	id	Id of the label
    @param	text	Text of the label
    @warning	The maximum length is set at the label creation
    @see	GfuiAddLabel
 */
void
GfuiLabelSetText(void *scr, int id, const char *text)
{
    tGfuiObject* object = gfuiGetObject(scr, id);
    
    if (object && object->widget == GFUI_LABEL)
	{
		tGfuiLabel* label = &(object->u.label);
		
		// Update the text.
		gfuiLabelSetText(label, text);
	}
}

/** Change the color of a label.
    @ingroup	gui
    @param	label	Label to modify
    @param	color	an array of 4 floats (RGBA)
    @see	GfuiAddLabel
 */
void
gfuiLabelSetColor(tGfuiLabel *label, const float *color)
{
	label->fgColor = GfuiColor::build(color);
}

/** Change the color of a label object.
    @ingroup	gui
    @param	scr	Screen where to add the label
    @param	id	Id of the label
    @param	color	an array of 4 floats (RGBA)
    @see	GfuiAddLabel
 */
void
GfuiLabelSetColor(void *scr, int id, const float* color)
{
    tGfuiObject* object = gfuiGetObject(scr, id);
    
    if (object && object->widget == GFUI_LABEL)
		gfuiLabelSetColor(&object->u.label, color);
}


/** Determine the actual text X coordinate.
    @ingroup	gui
    @param	obj	label to query
    @return	X coordinate of the text
 */
int
gfuiLabelGetTextX(tGfuiLabel *label)
{
	int xText = label->x;
    switch(label->align & GFUI_ALIGN_HMASK)
	{
		case GFUI_ALIGN_HL:
			// Nothing more to add / substract.
			break;

		case GFUI_ALIGN_HC:
			xText += (label->width - label->font->getWidth(label->text)) / 2;
			break;

		case GFUI_ALIGN_HR:
			xText += label->width - label->font->getWidth(label->text);
			break;

		default:
			break;
	}

	return xText;
}

/** Actually draw the given label.
    @ingroup	gui
    @param	obj	label to draw
 */
void
gfuiLabelDraw(tGfuiLabel *label, const GfuiColor& color)
{
    int of = 0;
    int fw = (int) label->font->getWidth("o");

    // Prevent strtok weirdness
    char *save;
    char text[128];
    strncpy(text, (char *) label->text, 128);

    char *p = strtok_r(text, "\t", &save);

    while (p != NULL)
    {
	// Select the right color from the state/focus.
	glColor4fv(color.toFloatRGBA());

	// Determine the actual (bottom left corner) coordinates where to draw the text.
	int x;
	switch(label->align & GFUI_ALIGN_HMASK)
	{
		default:
		case GFUI_ALIGN_HL:
			x = label->x + (fw * of);
			break;

		case GFUI_ALIGN_HC:
			x = label->x + (fw * of) + (label->width - label->font->getWidth(label->text)) / 2;
			break;

		case GFUI_ALIGN_HR:
			x = label->x + (fw * of) + label->width - label->font->getWidth(label->text);
			break;
	}

	gfuiDrawString(x, label->y, label->font, p);
	of += strlen(p) + 1;
	p = strtok_r(NULL, "\t", &save);
    }
}

/** Actually draw the given label object.
    @ingroup	gui
    @param	obj	label object to draw
 */
void
gfuiDrawLabel(tGfuiObject *obj)
{
    tGfuiLabel *label = &(obj->u.label);

	// Draw the background if visible.
    if (label->bgColor.alpha)
	{
		glColor4fv(obj->focus ? label->bgFocusColor.toFloatRGBA() : label->bgColor.toFloatRGBA());
		glBegin(GL_QUADS);
		glVertex2i(obj->xmin, obj->ymin);
		glVertex2i(obj->xmin, obj->ymax);
		glVertex2i(obj->xmax, obj->ymax);
		glVertex2i(obj->xmax, obj->ymin);
		glEnd();
    }

	// Draw the label text itself.
    gfuiLabelDraw(label, obj->focus ? label->fgFocusColor : label->fgColor);
}

void
gfuiReleaseLabel(tGfuiObject *obj)
{
    tGfuiLabel	*label;

    label = &(obj->u.label);

	freez(label->userDataOnFocus);
    free(label->text);
    free(obj);
}
