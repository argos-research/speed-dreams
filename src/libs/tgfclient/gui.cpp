/***************************************************************************
                               gui.cpp -- gui                   
                             -------------------                                         
    created              : Fri Aug 13 22:01:33 CEST 1999
    copyright            : (C) 1999 by Eric Espie                         
    email                : torcs@free.fr   
    version              : $Id: gui.cpp 6285 2015-11-28 18:30:04Z beaglejoe $                                  
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
    		This API is used to manage all the menu screens.
    @author	<a href=mailto:torcs@free.fr>Eric Espie</a>
    @version	$Id: gui.cpp 6285 2015-11-28 18:30:04Z beaglejoe $
    @ingroup	gui
*/
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <string>
#include <ctime>

#include <SDL.h>
#include <SDL_syswm.h>

#include <raceman.h>

#include <portability.h>
#include <tgf.hpp>

#include "tgfclient.h"
#include "gui.h"
#include "guimenu.h"
#include "musicplayer.h"

#ifdef WIN32
PFNGLUSEPROGRAMOBJECTARBPROC glUseProgram = NULL;
PFNGLACTIVETEXTUREARBPROC   glActiveTextureARB ;
#endif


#if SDL_MAJOR_VERSION >= 2
SDL_Window* 	GfuiWindow = NULL;
#endif

tGfuiScreen	*GfuiScreen;	/* current screen */
static int	GfuiMouseVisible = 1;
tMouseInfo	GfuiMouse;

int		GfuiMouseHW = 0;

float		gfuiColors[GFUI_COLORNB][4];
static		char buf[1024];


static int	ScrW, ScrH, ViewW, ViewH;

static tdble DelayRepeat;
static double LastTimeClick;

static const tdble REPEAT1 = 1.0;
static const tdble REPEAT2 = 0.2;


static void
gfuiInitColor(void)
{
	static const char *rgba[4] =
		{ GFSCR_ATTR_RED, GFSCR_ATTR_GREEN, GFSCR_ATTR_BLUE, GFSCR_ATTR_ALPHA };
	
	static const char *clr[GFUI_COLORNB] =
	{
		GFSCR_ELT_BGCOLOR,
		GFSCR_ELT_BGBTNFOCUS, GFSCR_ELT_BGBTNCLICK,	GFSCR_ELT_BGBTNENABLED, GFSCR_ELT_BGBTNDISABLED,
		GFSCR_ELT_BTNFOCUS, GFSCR_ELT_BTNCLICK, GFSCR_ELT_BTNENABLED, GFSCR_ELT_BTNDISABLED,
		GFSCR_ELT_LABELCOLOR, GFSCR_ELT_TIPCOLOR,
		GFSCR_ELT_BGSCROLLIST, GFSCR_ELT_SCROLLIST, GFSCR_ELT_BGSELSCROLLIST, GFSCR_ELT_SELSCROLLIST,
		GFSCR_ELT_BGEDITFOCUS, GFSCR_ELT_BGEDITENABLED, GFSCR_ELT_BGEDITDISABLED,
		GFSCR_ELT_EDITFOCUS, GFSCR_ELT_EDITENABLED, GFSCR_ELT_EDITDISABLED,
		GFSCR_ELT_EDITCURSORCLR,
		GFSCR_ELT_BASECOLORBGIMAGE,
		GFSCR_ELT_PROGRESSCOLOR
	};

	sprintf(buf, "%s%s", GfLocalDir(), GFSCR_CONF_FILE);
	void* hdle = GfParmReadFile(buf, GFPARM_RMODE_STD | GFPARM_RMODE_CREAT);

	for (int i = 0; i < GFUI_COLORNB; i++) {
		sprintf(buf, "%s/%s/%s", GFSCR_SECT_MENUSETTINGS, GFSCR_LIST_COLORS, clr[i]);
		for (int j = 0; j < 4; j++) {
			gfuiColors[i][j] = GfParmGetNum(hdle, buf, rgba[j], (char*)NULL, 1.0);
		}
	}

	GfParmReleaseHandle(hdle);
	
	// Remove the X11/Windows cursor if required.
	if (!GfuiMouseHW)
		SDL_ShowCursor(SDL_DISABLE);
	
	GfuiMouseVisible = 1;
}


void
gfuiInit(void)
{
	gfuiInitObject();
	gfuiInitColor();
	gfuiLoadFonts();
	gfuiInitButton();
	gfuiInitCombobox();
	gfuiInitEditbox();
	gfuiInitScrollBar();
	gfuiInitScrollList();
	gfuiInitLabel();
	gfuiInitHelp();
	gfuiInitMenu();
	initMusic();

#ifdef WIN32
	glUseProgram = (PFNGLUSEPROGRAMOBJECTARBPROC)wglGetProcAddress("glUseProgram");
    glActiveTextureARB = (PFNGLACTIVETEXTUREARBPROC)wglGetProcAddress("glActiveTextureARB");
#endif
	gfctrlJoyInit(); // Not here ; done later on the fly, when really needed.
}

void
gfuiShutdown(void)
{
	gfctrlJoyShutdown();
	gfuiFreeFonts();
	shutdownMusic();
}

GfuiColor 
GfuiColor::build(const float* afColor)
{
	 if (afColor)
		 return build(afColor[0], afColor[1], afColor[2], afColor[3]);
	 else
		 return build(0, 0, 0, 0);
}

// index from GFUI_* "named" indexes above.
GfuiColor
GfuiColor::build(int index)
{
	return build(gfuiColors[index]);
}

GfuiColor
GfuiColor::build(float r, float g, float b, float a)
{
	GfuiColor c;
	
	c.red = r;
	c.green = g;
	c.blue = b;
	c.alpha = a;
	
	return c;
}

// Expects a 32 bit unsigned integer string, 10/8/16 base, C syntax)
// Ex: "0xE47A96C2", "1285698774", "0765223412563"
GfuiColor
GfuiColor::build(const char* pszARGB)
{
	GfuiColor color;

	if (pszARGB)
	{
		char* pszMore = (char*)pszARGB;
		unsigned long uColor = strtoul(pszARGB, &pszMore, 0);
		if (*pszMore == '\0')
		{
			// Blue channel.
			color.blue = (uColor & 0xFF) / 255.0;
		
			// Green channel.
			uColor >>= 8;
			color.green = (uColor & 0xFF) / 255.0;
		
			// Red channel.
			uColor >>= 8;
			color.red = (uColor & 0xFF) / 255.0;

			// Alpha channel : assume 1.0 if not specified or 0x00.
			uColor >>= 8;
			color.alpha = (uColor & 0xFF) ? (uColor & 0xFF) / 255.0 : 1.0;
		}
		else
		{
			color = build(1, 1, 1, 1);
			GfLogWarning("Bad color ARGB string '%s'; assuming white\n", pszARGB);
		}
	}
	else
	{
		color = build(0, 0, 0, 0);
	}

	// GfLogDebug("GfuiColor::build(%s) = r=%f, g=%f, b=%f, a=%f\n",
	// 		   pszARGB ? pszARGB : "<null>", color.red, color.green, color.blue, color.alpha);
	
	return color;
}

/** Dummy display function for the event loop.
    Declare this function to the event loop if nothing is to be displayed
	by the redisplay mechanism.
     @ingroup	gui
*/

void
GfuiDisplayNothing(void)
{
}

/** Idle function for the GUI to be called during idle loops of the event loop.
     @ingroup	gui
  */
void
GfuiIdle(void)
{
	double curtime = GfTimeClock();
	
	if ((curtime - LastTimeClick) > DelayRepeat) {
		DelayRepeat = REPEAT2;
		LastTimeClick = curtime;
		if (GfuiScreen->mouse == 1) {
			/* button down */
			gfuiUpdateFocus();
			gfuiMouseAction((void*)0);
			GfuiApp().eventLoop().postRedisplay();
		}
	}
}

/** Display callback for the GUI event loop (redraws the current screen and swaps buffers).
     @ingroup	gui
 */
void
GfuiDisplay(void)
{
	GfuiRedraw();
	GfuiSwapBuffers();
}

/** Redraw the current screen
     @ingroup	gui
 */
void
GfuiRedraw(void)
{
	tGfuiObject	*curObj;
	
	glUseProgram(0);
	glActiveTextureARB(GL_TEXTURE0);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_CULL_FACE);
	glDisable(GL_ALPHA_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	GfScrGetSize(&ScrW, &ScrH, &ViewW, &ViewH);
	
	glViewport((ScrW-ViewW) / 2, (ScrH-ViewH) / 2, ViewW, ViewH);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, GfuiScreen->width, 0, GfuiScreen->height);
	glMatrixMode(GL_MODELVIEW);  
	glLoadIdentity();
	glMatrixMode(GL_TEXTURE);  
	glLoadIdentity();
	glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );
	
	
	if (GfuiScreen->bgColor.alpha != 0.0) {
		glClearColor(GfuiScreen->bgColor.red,
			     GfuiScreen->bgColor.green,
			     GfuiScreen->bgColor.blue,
			     GfuiScreen->bgColor.alpha);
		glClear(GL_COLOR_BUFFER_BIT);
	}

	// Display backround image if any.
	if (GfuiScreen->bgImage) {

		// Prepare texture display.
		glDisable(GL_BLEND);
		glEnable(GL_TEXTURE_2D);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE);

		glColor3f(gfuiColors[GFUI_BASECOLORBGIMAGE][0], 
				  gfuiColors[GFUI_BASECOLORBGIMAGE][1],
				  gfuiColors[GFUI_BASECOLORBGIMAGE][2]);

		glBindTexture(GL_TEXTURE_2D, GfuiScreen->bgImage);

		// Get real 2^N x 2^P texture size (may have been 0 padded at load time
		// if the original image was not 2^N x 2^P)
		// This 2^N x 2^P stuff is needed by some low-end OpenGL hardware/drivers.
		int bgPow2Width = 1, bgPow2Height = 1;
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &bgPow2Width);
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &bgPow2Height);

		// Compute the initial width of the right area and the height of the bottom area
		// of the texture that will not be displayed
		// (We display only the top left rectangle of the 2^N x 2^P texture
		//  that corresponds to the original image).
		GLfloat tx1 = 0.0f;
		GLfloat tx2 = GfuiScreen->bgWidth / (GLfloat)bgPow2Width;

		GLfloat ty1 = 1.0f - GfuiScreen->bgHeight / (GLfloat)bgPow2Height;
 		GLfloat ty2 = 1.0;

		// Compute the width/height of the symetrical left/right / top/bottom
		// areas of original image that will need to be clipped
		// in order to keep its aspect ratio.
		const GLfloat rfactor = GfuiScreen->bgWidth * (GLfloat)ViewH
		                        / GfuiScreen->bgHeight / (GLfloat)ViewW;

		if (rfactor >= 1.0f) {
			// If aspect ratio of view is smaller than image's one, "cut off" sides.
			const GLfloat tdx = GfuiScreen->bgWidth * (rfactor - 1.0f) / bgPow2Width / 2.0f;
			tx1 += tdx;
			tx2 -= tdx;
		} else {
			// If aspect ratio of view is larger than image's one, 
			// "cut off" top and bottom.
			const GLfloat tdy = GfuiScreen->bgHeight * rfactor / bgPow2Height / 2.0f;
			ty2 = (ty1+1)/2 + tdy;
			ty1 = (ty1+1)/2 - tdy;
		}

		// Display texture.

		glBegin(GL_QUADS);

		glTexCoord2f(tx1, ty1); glVertex3f(0.0, 0.0, 0.0);
		glTexCoord2f(tx1, ty2); glVertex3f(0.0, GfuiScreen->height, 0.0);
		glTexCoord2f(tx2, ty2); glVertex3f(GfuiScreen->width, GfuiScreen->height, 0.0);
		glTexCoord2f(tx2, ty1); glVertex3f(GfuiScreen->width, 0.0, 0.0);

		glEnd();
		glDisable(GL_TEXTURE_2D);
		

		glEnable(GL_BLEND);
	}
	
	// Display other screen objects
	curObj = GfuiScreen->objects;
	if (curObj) 
	{
		do 
		{
			curObj = curObj->next;
			GfuiDraw(curObj);
		} while (curObj != GfuiScreen->objects);
	}
	
	// Display mouse cursor if needed/specified
	if (!GfuiMouseHW && GfuiMouseVisible && GfuiScreen->mouseAllowed) 
		GfuiDrawCursor();

	glDisable(GL_BLEND);
}

/** Hide the mouse cursor
    @ingroup	gui
    @return	none
*/
void
GfuiMouseHide(void)
{
	if (GfuiScreen)
		GfuiScreen->mouseAllowed = 0;
}

/** Show the mouse cursor
    @ingroup	gui
    @return	none
*/
void
GfuiMouseShow(void)
{
	if (GfuiScreen)
		GfuiScreen->mouseAllowed = 1;
}

/** Toggle the mouse cursor visibility
    @ingroup	gui
    @return	none
*/
void
GfuiMouseToggleVisibility(void)
{
	if (GfuiScreen)
		GfuiScreen->mouseAllowed = 1 - GfuiScreen->mouseAllowed;
}

/** Force the hardware mouse pointer
    @ingroup	ctrl
    @return	<tt>0 ... </tt>Ok
		<br><tt>-1 .. </tt>Error
*/
void
GfuiMouseSetHWPresent(void)
{
    GfuiMouseHW = 1;
}

/** Query if the hardware mouse pointer is activated
    @ingroup	ctrl
    @return	<tt>0 ... </tt>Ok
		<br><tt>-1 .. </tt>Error
*/
bool
GfuiMouseIsHWPresent(void)
{
    return GfuiMouseHW != 0;
}

static void
gfuiKeyboardDown(int key, int modifier, int /* x */, int /* y */)
{
	tGfuiKey	*curKey;
	tGfuiObject	*obj;
	
	/* User-preempted key */
	if (GfuiScreen->onKeyAction && GfuiScreen->onKeyAction(key, modifier, GFUI_KEY_DOWN)) 
		return;
	
	/* Now look at the user's defined keys */
	if (GfuiScreen->userKeys) {
		curKey = GfuiScreen->userKeys;
		do 
		{
			curKey = curKey->next;
			// Ignore Shift modifier when printable unicode,
			// as the unicode generator already took care of it.
			if (curKey->key == key
				&& (curKey->modifier == modifier
#if SDL_MAJOR_VERSION < 2
					|| (curKey->modifier == (modifier & (~GFUIM_SHIFT))
						&& key >= ' ' && key <= 'z')
#else
				|| (curKey->modifier == (modifier & (~GFUIM_SHIFT))
					&& isprint(key))
#endif
				))
			{
				if (curKey->onPress)
					curKey->onPress(curKey->userData);
				break;
			}
		} while (curKey != GfuiScreen->userKeys);
	}

	// Edit-box management (TODO: Should at least partly have priority on user's keys).
	obj = GfuiScreen->hasFocus;
	if (obj) 
	{
		switch (obj->widget) 
		{
			case GFUI_EDITBOX:
				gfuiEditboxKey(obj, key, modifier);
				break;
		}
	}

	GfuiApp().eventLoop().postRedisplay();
}

static void
gfuiKeyboardUp(int key, int modifier, int /* x */, int /* y */)
{
	tGfuiKey	*curKey;
	
	/* User-preempted key */
	if (GfuiScreen->onKeyAction && GfuiScreen->onKeyAction(key, modifier, GFUI_KEY_UP)) 
		return;
	
	/* Now look at the user's defined keys */
	if (GfuiScreen->userKeys) 
	{
		curKey = GfuiScreen->userKeys;
		do 
		{
			curKey = curKey->next;
			if (curKey->key == key
				&& (curKey->modifier == modifier
					|| (curKey->modifier == (modifier & (~GFUIM_SHIFT))
						&& key >= ' ' && key <= 'z'))) 
			{
				if (curKey->onRelease)
					curKey->onRelease(curKey->userData);
				break;
			}
		} while (curKey != GfuiScreen->userKeys);
	}
	
	GfuiApp().eventLoop().postRedisplay();
}

/** Get the mouse information (position and buttons)
    @ingroup	gui
    @return	mouse information
*/
tMouseInfo *GfuiMouseInfo(void)
{
	return &GfuiMouse;
}

/** Set the mouse position
    @ingroup	gui
    @param	x	mouse x pos
    @param	y	mouse y pos
    @return	none
*/
void GfuiMouseSetPos(int x, int y)
{
	if (GfuiScreen)
	{
#if SDL_MAJOR_VERSION >= 2
		SDL_WarpMouseInWindow(GfuiWindow, x,y);
#else
		SDL_WarpMouse(x,y);
#endif
		GfuiMouse.X = (x - (ScrW - ViewW)/2) * (int)GfuiScreen->width / ViewW;
		GfuiMouse.Y = (ViewH - y + (ScrH - ViewH)/2) * (int)GfuiScreen->height / ViewH;
	}
}


static void
gfuiMouseButton(int button, int state, int x, int y)
{
	/* Consider all SDL supported buttons */
	if (button >= 1 && button <= 7) 
	{
		GfuiMouse.X = (x - (ScrW - ViewW)/2) * (int)GfuiScreen->width / ViewW;
		GfuiMouse.Y = (ViewH - y + (ScrH - ViewH)/2) * (int)GfuiScreen->height / ViewH;

#if SDL_MAJOR_VERSION >= 2
		if (button == SDL_MOUSEWHEEL) {
#else
		if (button == SDL_BUTTON_WHEELUP || button == SDL_BUTTON_WHEELDOWN) {
#endif
			// Up/down happens very quickly, leaving no time for the system to see them 
			// this just toggle every down event
			if (state == SDL_PRESSED) {
				GfuiMouse.button[button-1] = (GfuiMouse.button[button-1] == 0);
			}
		} else {
			GfuiMouse.button[button-1] = (state == SDL_PRESSED); /* SDL 1st button has index 1 */
	
			DelayRepeat = REPEAT1;
			LastTimeClick = GfTimeClock();

			if (state == SDL_PRESSED)
			{
				GfuiScreen->mouse = 1;
				gfuiUpdateFocus();
				gfuiMouseAction((void*)0);
			} 
			else 
			{
				GfuiScreen->mouse = 0;
				gfuiUpdateFocus();
				gfuiMouseAction((void*)1);
			}
		}
		GfuiApp().eventLoop().postRedisplay();
	}
}

static void
gfuiMouseMotion(int x, int y)
{
	GfuiMouse.X = (x - (ScrW - ViewW)/2) * (int)GfuiScreen->width / ViewW;
	GfuiMouse.Y = (ViewH - y + (ScrH - ViewH)/2) * (int)GfuiScreen->height / ViewH;
	gfuiUpdateFocus();
	gfuiMouseAction((void*)(long)(1 - GfuiScreen->mouse));
	GfuiApp().eventLoop().postRedisplay();
	DelayRepeat = REPEAT1;
}

static void
gfuiMousePassiveMotion(int x, int y)
{
	GfuiMouse.X = (x - (ScrW - ViewW)/2) * (int)GfuiScreen->width / ViewW;
	GfuiMouse.Y = (ViewH - y + (ScrH - ViewH)/2) * (int)GfuiScreen->height / ViewH;
	gfuiUpdateFocus();
	GfuiApp().eventLoop().postRedisplay();
}

/** Tell if the screen is active or not.
    @ingroup	gui
    @param	screen	Screen to activate
    @return	1 if active and 0 if not.
 */
int
GfuiScreenIsActive(void *screen)
{
	return GfuiScreen == screen;
}

/** Get the screen.
    @ingroup	gui
    @param	
    @return	screen handle.
 */
void*
GfuiGetScreen(void)
{
	return GfuiScreen;
}

/** Activate a screen and make it current.
    @ingroup	gui
    @param	screen	Screen to activate
    @warning	The current screen at the call time is deactivated.
 */
void
GfuiScreenActivate(void *screen)
{
	if (GfuiScreen && GfuiScreen->onDeactivate) 
		GfuiScreen->onDeactivate(GfuiScreen->userDeactData);
	
	GfuiScreen = (tGfuiScreen*)screen;

	playMusic(GfuiScreen->musicFilename);
	
  	GfuiApp().eventLoop().setKeyboardDownCB(gfuiKeyboardDown);
	GfuiApp().eventLoop().setKeyboardUpCB(gfuiKeyboardUp);
   	GfuiApp().eventLoop().setMouseButtonCB(gfuiMouseButton);
	GfuiApp().eventLoop().setMouseMotionCB(gfuiMouseMotion);
	GfuiApp().eventLoop().setMousePassiveMotionCB(gfuiMousePassiveMotion);
	GfuiApp().eventLoop().setRecomputeCB(0);

#if SDL_MAJOR_VERSION < 2
	if (GfuiScreen->keyAutoRepeat)
		SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
	else
		SDL_EnableKeyRepeat(0, 0);
//#else
#if SDL_JOYSTICK
	GfuiApp().eventLoop().setJoystickAxisCB(GfctrlJoySetAxis);
	GfuiApp().eventLoop().setJoystickButtonCB(GfctrlJoySetButton);
#endif
#endif

	if (GfuiScreen->onlyCallback == 0) 
	{
		if (GfuiScreen->hasFocus == NULL) 
		{
			gfuiSelectNext(NULL);
		}
		GfuiApp().eventLoop().setRedisplayCB(GfuiDisplay);
	} 
	else 
	{
		GfuiApp().eventLoop().setRedisplayCB(GfuiDisplayNothing);
	}
	
	if (GfuiScreen->onActivate)
		GfuiScreen->onActivate(GfuiScreen->userActData);
	
	if (GfuiScreen->onlyCallback == 0) 
	{
		GfuiDisplay();
		GfuiApp().eventLoop().postRedisplay();
	}
}


/** Activate a screen and make it current plus release the current screen.
    @ingroup	gui
    @param	screen	Screen to activate
    @warning	The current screen at the call time is deactivated.
 */
void
GfuiScreenReplace(void *screen)
{
	tGfuiScreen	*oldScreen = GfuiScreen;

	if (oldScreen) 
//		GfuiScreenRelease(oldScreen);
	GfuiScreenActivate(screen);
}

/** Deactivate the current screen.
    @ingroup	gui
 */
void
GfuiScreenDeactivate(void)
{
	if (GfuiScreen->onDeactivate)
		GfuiScreen->onDeactivate(GfuiScreen->userDeactData);
	
	GfuiScreen = (tGfuiScreen*)NULL;
	
  	GfuiApp().eventLoop().setKeyboardDownCB(0);
 	GfuiApp().eventLoop().setKeyboardUpCB(0);
	GfuiApp().eventLoop().setMouseButtonCB(0);
  	GfuiApp().eventLoop().setMouseMotionCB(0);
	GfuiApp().eventLoop().setMousePassiveMotionCB(0);
 	GfuiApp().eventLoop().setRecomputeCB(0);
 	GfuiApp().eventLoop().setRedisplayCB(GfuiDisplayNothing);
}

/** Create a screen.
    @ingroup	gui
    @param	bgColor			pointer on color array (RGBA) (if NULL default color is used)
    @param	userDataOnActivate	Parameter to the activate function
    @param	onActivate		Function called when the screen is activated
    @param	userDataOnDeactivate	Parameter to the deactivate function
    @param	onDeactivate		Function called when the screen is deactivated
    @param	mouseAllowed		Flag to tell if the mouse cursor can be displayed
    @return	New screen instance
		<br>NULL if Error
    @bug	Only black background work well
 */
void *
GfuiScreenCreate(float *bgColor,
				 void *userDataOnActivate, tfuiCallback onActivate, 
				 void *userDataOnDeactivate, tfuiCallback onDeactivate,
				 int mouseAllowed)
{
	tGfuiScreen	*screen;
	
	screen = (tGfuiScreen*)calloc(1, sizeof(tGfuiScreen));
	
	screen->width = 640.0;
	screen->height = 480.0;
	
	screen->bgColor = bgColor ? GfuiColor::build(bgColor) : GfuiColor::build(GFUI_BGCOLOR);

	screen->onActivate = onActivate;
	screen->userActData = userDataOnActivate;
	screen->onDeactivate = onDeactivate;
	screen->userDeactData = userDataOnDeactivate;
	
	screen->mouseAllowed = mouseAllowed;
	
	screen->keyAutoRepeat = 1; // Default key auto-repeat on.
 
	RegisterScreens(screen);

	return (void*)screen;
}

/** Release the given screen.
    @ingroup	gui
    @param	scr	Screen to release
    @warning	If the screen was activated, it is deactivated.
 */
void
GfuiScreenRelease(void *scr)
{
	tGfuiObject *curObject;
	tGfuiObject *nextObject;
	tGfuiKey *curKey;
	tGfuiKey *nextKey;
	tGfuiScreen *screen = (tGfuiScreen*)scr;

	UnregisterScreens(screen);

	if (GfuiScreen == screen) {
		GfuiScreenDeactivate();
	}

	if (screen->bgImage != 0) {
		glDeleteTextures(1, &screen->bgImage);
	}

	curObject = screen->objects;
	if (curObject != NULL) {
		do {
			nextObject = curObject->next;
			gfuiReleaseObject(curObject);
			curObject = nextObject;
		} while (curObject != screen->objects);
	}

	curKey = screen->userKeys;
	if (curKey != NULL) {
		do {
			nextKey = curKey->next;
			free(curKey->name);
			free(curKey->descr);
			free(curKey);
			curKey = nextKey;
		} while (curKey != screen->userKeys);
	}
	if(screen->musicFilename != NULL) {
		free(screen->musicFilename);
	}
	free(screen);
}

/** Create a callback hook.
    @ingroup	gui
    @param	userDataOnActivate	Parameter to the activate function
    @param	onActivate		Function called when the screen is activated
    @return	New hook instance
		<br>NULL if Error
 */
void *
GfuiHookCreate(void *userDataOnActivate, tfuiCallback onActivate)
{
	tGfuiScreen	*screen;
	
	screen = (tGfuiScreen*)calloc(1, sizeof(tGfuiScreen));
	screen->onActivate = onActivate;
	screen->userActData = userDataOnActivate;
	screen->onlyCallback = 1;

	RegisterScreens(screen);
	
	return (void*)screen;
}

/** Release the given hook.
    @ingroup	gui
    @param	hook	Hook to release
 */
void
GfuiHookRelease(void *hook)
{
	UnregisterScreens(hook);
	free(hook);
}

void
GfuiKeyEventRegister(void *scr, tfuiKeyCallback onKeyAction)
{
	if (!scr)
		return;
	
	tGfuiScreen	*screen = (tGfuiScreen*)scr;
	
	screen->onKeyAction = onKeyAction;
}


void
GfuiKeyEventRegisterCurrent(tfuiKeyCallback onKeyAction)
{
	GfuiKeyEventRegister(GfuiScreen, onKeyAction);
}


/** Add a Keyboard callback to the current screen.
    @ingroup	gui
    @param	key		Key code (SDL key sym, including ASCII codes, like 'a' or '_'
	                          see tgfclient::GFUIK_* constants for special keys)
    @param	descr		Description for help screen
    @param	userData	Parameter to the callback function
    @param	onKeyPressed	Callback function called when the specified key is pressed
    @param	onKeyReleased	Callback function
 */
void
GfuiRegisterKey(int key, const char *descr, void *userData,
				tfuiCallback onKeyPressed, tfuiCallback onKeyReleased)
{
	GfuiAddKey(GfuiScreen, key, descr, userData, onKeyPressed, onKeyReleased);
}

/** Add a Keyboard shortcut to the screen (Warning: no onKeyRelease support for special keys tgfclient::GFUIK_*).
    @ingroup	gui
    @param	scr		Target screen
    @param	key		Key code : the ASCII code when possible (for 'a', '_', '[' ...), or else the tgfclient::GFUIK_* value for special keys) ; Always in [0, GFUIK_MAX]
    @param	modifier	Key modifiers (GFUIM_NONE or GFUIM_XX|GFUIM_YY|...)
    @param	descr		Description for help screen
    @param	userData	Parameter to the callback function
    @param	onKeyPressed	Callback function
    @param	onKeyReleased	Callback function
 */
void
GfuiAddKey(void *scr, int key, const char *descr, void *userData,
		   tfuiCallback onKeyPressed, tfuiCallback onKeyReleased)
{
	GfuiAddKey(scr, key, GFUIM_NONE, descr, userData, onKeyPressed, onKeyReleased);
}

void
GfuiAddKey(void *scr, int key, int modifier, const char *descr, void *userData,
		   tfuiCallback onKeyPressed, tfuiCallback onKeyReleased)
{
	if (!scr)
		return;
	
	// Allocate a key entry for the key list
	tGfuiKey* curKey = (tGfuiKey*)calloc(1, sizeof(tGfuiKey));
	curKey->key = key;
	curKey->modifier = modifier;
	curKey->userData = userData;
	curKey->onPress = onKeyPressed;
	curKey->onRelease = onKeyReleased;

	// Set the key description.
	curKey->descr = descr ? strdup(descr) : strdup("");

	// Set the key user-friendly name.
	// 1) The modifiers (see GfEventLoop : we never get a KMOD_RXX here, only KMOD_LXX)
	char pszModString[32];
	pszModString[0] = '\0';
	if (modifier)
	{
		if (modifier & GFUIM_SHIFT)
			strncat(pszModString, "Shift-", sizeof(pszModString));
		if (modifier & GFUIM_CTRL)
			strncat(pszModString, "Ctrl-", sizeof(pszModString));
		if (modifier & GFUIM_ALT)
			strncat(pszModString, "Alt-", sizeof(pszModString));
		if (modifier & GFUIM_META)
			strncat(pszModString, "Meta-", sizeof(pszModString));
	}

	// 2) The key itself
	char pszKeyString[16];
	switch(key) {
		case GFUIK_BACKSPACE:
			strncpy(pszKeyString, "Backspace", sizeof(pszKeyString));
			break;
		case GFUIK_TAB:
			strncpy(pszKeyString, "Tab", sizeof(pszKeyString));
			break;
		case GFUIK_RETURN:
			strncpy(pszKeyString, "Enter", sizeof(pszKeyString));
			break;
		case GFUIK_ESCAPE:
			strncpy(pszKeyString, "Escape", sizeof(pszKeyString));
			break;
		case GFUIK_SPACE:
			strncpy(pszKeyString, "Space", sizeof(pszKeyString));
			break;
		case GFUIK_F1:
			strncpy(pszKeyString, "F1", sizeof(pszKeyString));
			break;
		case GFUIK_F2:
			strncpy(pszKeyString, "F2", sizeof(pszKeyString));
			break;
		case GFUIK_F3:
			strncpy(pszKeyString, "F3", sizeof(pszKeyString));
			break;
		case GFUIK_F4:
			strncpy(pszKeyString, "F4", sizeof(pszKeyString));
			break;
		case GFUIK_F5:
			strncpy(pszKeyString, "F5", sizeof(pszKeyString));
			break;
		case GFUIK_F6:
			strncpy(pszKeyString, "F6", sizeof(pszKeyString));
			break;
		case GFUIK_F7:
			strncpy(pszKeyString, "F7", sizeof(pszKeyString));
			break;
		case GFUIK_F8:
			strncpy(pszKeyString, "F8", sizeof(pszKeyString));
			break;
		case GFUIK_F9:
			strncpy(pszKeyString, "F9", sizeof(pszKeyString));
			break;
		case GFUIK_F10:
			strncpy(pszKeyString, "F10", sizeof(pszKeyString));
			break;
		case GFUIK_F11:
			strncpy(pszKeyString, "F11", sizeof(pszKeyString));
			break;
		case GFUIK_F12:
			strncpy(pszKeyString, "F12", sizeof(pszKeyString));
			break;
		case GFUIK_LEFT:
			strncpy(pszKeyString, "Left Arrow", sizeof(pszKeyString));
			break;
		case GFUIK_UP:
			strncpy(pszKeyString, "Up Arrow", sizeof(pszKeyString));
			break;
		case GFUIK_RIGHT:
			strncpy(pszKeyString, "Right Arrow", sizeof(pszKeyString));
			break;
		case GFUIK_DOWN:
			strncpy(pszKeyString, "Down Arrow", sizeof(pszKeyString));
			break;
		case GFUIK_PAGEUP:
			strncpy(pszKeyString, "Page Up", sizeof(pszKeyString));
			break;
		case GFUIK_PAGEDOWN:
			strncpy(pszKeyString, "Page Down", sizeof(pszKeyString));
			break;
		case GFUIK_HOME:
			strncpy(pszKeyString, "Home", sizeof(pszKeyString));
			break;
		case GFUIK_END:
			strncpy(pszKeyString, "End", sizeof(pszKeyString));
			break;
		case GFUIK_INSERT:
			strncpy(pszKeyString, "Insert", sizeof(pszKeyString));
			break;
		case GFUIK_DELETE:
			strncpy(pszKeyString, "Delete", sizeof(pszKeyString));
			break;
		case GFUIK_CLEAR:
			strncpy(pszKeyString, "Clear", sizeof(pszKeyString));
			break;
		case GFUIK_PAUSE:
			strncpy(pszKeyString, "Pause", sizeof(pszKeyString));
			break;
		default:
			if (key >= ' ' && key < 127)
				snprintf(pszKeyString, sizeof(pszKeyString), "%c", (char)key);
			else
				snprintf(pszKeyString, sizeof(pszKeyString), "0x%X", key);
			break;
	}

	// 3) Concatenate the modifiers string and the key string
	curKey->name = (char*)malloc((1+strlen(pszModString)+strlen(pszKeyString))*sizeof(char));
	snprintf(curKey->name, (1+strlen(pszModString)+strlen(pszKeyString))*sizeof(char),
			 "%s%s", pszModString, pszKeyString);

	//GfLogDebug("GfuiAddKey(k=%d, m=%d, d=%s) : mod='%s', key='%s', UFN='%s'\n",
	//		   key, modifier, descr, pszModString, pszKeyString, curKey->name);
	
	// Add the new key entry in the key list if not already in,
	// or else replace the previous definition.
	tGfuiScreen* screen = (tGfuiScreen*)scr;
	if (!screen->userKeys) {
		screen->userKeys = curKey->next = curKey;
	} else {
		// Search in the list for a definition for the same key.
		bool bFound = false;
		tGfuiKey* curKey2 = screen->userKeys;
		do {
			// Found => replace with the new definition.
			if (curKey2->key == key && curKey2->modifier == modifier) {
				free(curKey2->name);
				curKey2->name = curKey->name;
				free(curKey2->descr);
				curKey2->descr = curKey->descr;
				curKey2->userData = curKey->userData;
				curKey2->onPress = curKey->onPress;
				curKey2->onRelease = curKey->onRelease;
				free(curKey);
				bFound = true;
				break;
			}
		} while ((curKey2 = curKey2->next) != screen->userKeys);

		// Not found => add at the beginning of the list.
		if (!bFound) {
			curKey->next = screen->userKeys->next;
			screen->userKeys->next = curKey;
			screen->userKeys = curKey;
		}
	}
}

/** Remove a Keyboard shortcut from the screen 
	 @ingroup	gui
	 @param	scr		Target screen
	 @param	key		Key code : the ASCII code when possible (for 'a', '_', '[' ...), or else the tgfclient::GFUIK_* value for special keys) ; Always in [0, GFUIK_MAX]
	 @param	modifier	Key modifiers (GFUIM_NONE or GFUIM_XX|GFUIM_YY|...)
	 @param	descr		Description for help screen
	 @return	true for success false if key not found
 */
 bool 
GfuiRemoveKey(void *scr, int key, const char *descr)
 {
	 return GfuiRemoveKey(scr, key, GFUIM_NONE, descr);
 }

bool 
GfuiRemoveKey(void *scr, int key, int modifier, const char *descr)
{
	bool bFound = false;

	tGfuiScreen* screen = (tGfuiScreen*)scr;
	if ((screen)&&(screen->userKeys))
	{
		tGfuiKey* prevKey = screen->userKeys;
		tGfuiKey* checkKey = screen->userKeys;
		do
		{
			// Try to find the key: the key, modifier, and description (descr) MUST match
			if (checkKey->key == key && checkKey->modifier == modifier)
			{
				// if there are descriptions then they MUST match
				if ((checkKey->descr) && (descr))
				{
					if (0 != strncmp(descr,checkKey->descr,strlen(descr)))
					{
						continue;
					}
				}

				bFound = true;

				// unlink the removed key
				prevKey->next = checkKey->next;
				
				// First key in list
				if (prevKey == screen->userKeys)
				{
					// only one key in list: set list to null
					if (screen->userKeys == screen->userKeys->next)
					{
						screen->userKeys = 0;
					}
					else
					{
						// Goto the end of the list and set end->next to the new head of the list
						tGfuiKey* lastKey = screen->userKeys;
						while(lastKey->next != screen->userKeys)
						{
							lastKey = lastKey->next;
						}
						lastKey->next = screen->userKeys = prevKey->next;
					}
				}

				// free the memory used by the removed key
				FREEZ(checkKey->name);
				FREEZ(checkKey->descr);
				FREEZ(checkKey);
				break;
			}
		} while (prevKey = checkKey,(checkKey = checkKey->next) != screen->userKeys);
	}
	return bFound;
}

/** Enable/disable the key auto-repeat for the given screen.
    @ingroup	screen
    @param	scr		Screen
    @param	on		Flag
 */
void
GfuiSetKeyAutoRepeat(void *scr, int on)
{
	tGfuiScreen	*screen = (tGfuiScreen*)scr;
	screen->keyAutoRepeat = on;
}

/** Save a screen shot in png format.
    @ingroup	screen
 */
void
GfuiScreenShot(void * /* notused */)
{
	char path[256];
	snprintf(path, sizeof(path), "%sscreenshots", GfLocalDir());

	// Ensure that screenshot directory exists.
	if (GfDirCreate(path) == GF_DIR_CREATED)
	{
		time_t t = time(NULL);
		struct tm *stm = localtime(&t);
		char buf[sizeof(path)+64];
		snprintf(buf, sizeof(buf), "%s/sd-%4d%02d%02d%02d%02d%02d.png",
				 path,
				 stm->tm_year+1900,
				 stm->tm_mon+1,
				 stm->tm_mday,
				 stm->tm_hour,
				 stm->tm_min,
				 stm->tm_sec);
		GfScrCaptureAsPNG(buf);
	}
}

/** Add an image background to a screen.
    @ingroup	gui
    @param	scr		Screen
    @param	filename	file name of the bg image (PNG or JPEG)
    @return	None.
 */
void
GfuiScreenAddBgImg(void *scr, const char *filename)
{
	tGfuiScreen	*screen = (tGfuiScreen*)scr;
	int pow2Width, pow2Height;
	
	if (screen->bgImage) {
		GfTexFreeTexture(screen->bgImage);
	}

	// Note: Here, we save the original image size (may be not 2^N x 2^P)
	//       in order to be able to hide padding pixels added in texture to enforce 2^N x 2^P.
	//       and we request this 2^N x 2^P enforcment by passing &pow2Width and &pow2Height.
	screen->bgImage =
		GfTexReadTexture(filename, &screen->bgWidth, &screen->bgHeight, &pow2Width, &pow2Height);
}

/** Add the background music for the given screen.
    @ingroup	screen
    @param	scr		Screen
    @param	filename	file name of the music (OGG)
    @return	None.

 */
void
GfuiScreenAddMusic(void *scr, const char *filename)
{
	tGfuiScreen	*screen = (tGfuiScreen*)scr;
	if(screen->musicFilename != NULL) {
		free(screen->musicFilename);
	}
	if(0 != filename){
		screen->musicFilename = (char*)malloc(1+strlen(filename));
		if(0 != screen->musicFilename){
			strcpy(screen->musicFilename, filename);
		}
	}
}

/** Initialize window position
    @ingroup	gui
    @param	x		Left x position in the screen (pixels)
    @param	y		Top y position in the screen (pixels)
    @param	w		Width (pixels)
    @param	h		Height (pixels)
*/

void 
GfuiInitWindowPositionAndSize(int x, int y, int w, int h)
{
	// No need to resize, already done when setting the video mode.
	SDL_SysWMinfo wmInfo;
	SDL_VERSION(&wmInfo.version);
#if SDL_MAJOR_VERSION >= 2
	if (SDL_GetWindowWMInfo(GfuiWindow, &wmInfo)) {
#else
	if (SDL_GetWMInfo(&wmInfo)) {
#endif
#ifdef WIN32
#if SDL_MAJOR_VERSION >= 2
		SetWindowPos(wmInfo.info.win.window, HWND_TOP, x, y, 0, 0, SWP_NOSIZE);
#else
		SetWindowPos(wmInfo.window, HWND_TOP, x, y, 0, 0, SWP_NOSIZE);
#endif
#else
		// TODO.
		GfLogWarning("GfuiInitWindowPositionAndSize not yet implemented under non-Windows OSes\n");
#endif // WIN32
	}
	else{
		GfLogWarning("SDL_GetWindowWMInfo() failed: SDL_GetError() returns: %s\n", SDL_GetError());
	}
}

/** Swap display buffers (double buffering)
    
    @ingroup	gui
*/

void 
GfuiSwapBuffers(void)
{
#if SDL_MAJOR_VERSION >= 2
	SDL_GL_SwapWindow(GfuiWindow);
#else
	SDL_GL_SwapBuffers();
#endif
}
