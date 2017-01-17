/***************************************************************************
                    tgfclient.h -- Interface file for The Gaming Framework
                             -------------------
    created              : Fri Aug 13 22:32:14 CEST 1999
    copyright            : (C) 1999 by Eric Espie
    email                : torcs@free.fr
    version              : $Id: tgfclient.h 6393 2016-03-27 16:23:23Z beaglejoe $
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
        The Gaming Framework API (client part).
    @author     <a href=mailto:torcs@free.fr>Eric Espie</a>
    @version    $Id: tgfclient.h 6393 2016-03-27 16:23:23Z beaglejoe $
*/

#ifndef __TGFCLIENT__H__
#define __TGFCLIENT__H__

#include <string>
#include <vector>
#include <map>

#ifdef _MSC_VER
// Disable useless MSVC warnings
#  pragma warning (disable:4244)
#  pragma warning (disable:4996)
#  pragma warning (disable:4305)
#  pragma warning (disable:4251) // class XXX needs a DLL interface ...
#endif

#if defined(__APPLE__) && !defined(USE_MACPORTS)
#  include <js.h>
#else
#  include <plib/js.h>
#endif

#include <SDL.h>
#if 0
#if SDL_MAJOR_VERSION >= 2
#include <SDL_keyboard.h>
#else
#include <SDL_keysym.h>
#endif
#endif

#include <tgf.hpp>

#include "guiscreen.h"

#if SDL_MAJOR_VERSION >= 2
extern SDL_Window* 	GfuiWindow;
#endif


// DLL exported symbols declarator for Windows.
#ifdef WIN32
# ifdef TGFCLIENT_DLL
#  define TGFCLIENT_API __declspec(dllexport)
# else
#  define TGFCLIENT_API __declspec(dllimport)
# endif
#else
# define TGFCLIENT_API
#endif


/****************************** 
 * Initialization / Shutdown  *
 ******************************/

TGFCLIENT_API void GfuiInit(void);
TGFCLIENT_API void GfuiShutdown(void);


/******************** 
 * Screen Interface *
 ********************/

typedef struct ScreenSize
{
    int width;  // Width in pixels.
    int height; // Height in pixels.
} tScreenSize;

#if SDL_MAJOR_VERSION >= 2
TGFCLIENT_API SDL_Window* GfScrGetMainWindow();//{return GfuiWindow;};
#endif
TGFCLIENT_API bool GfScrInit(int nWinWidth = -1, int nWinHeight = -1, int nFullScreen = -1);
TGFCLIENT_API void GfScrShutdown(void);
TGFCLIENT_API void GfScrGetSize(int *scrW, int *scrH, int *viewW, int *viewH);
TGFCLIENT_API bool GfScrToggleFullScreen();
TGFCLIENT_API unsigned char* GfScrCaptureAsImage(int* viewW, int *viewH);
TGFCLIENT_API int GfScrCaptureAsPNG(const char *filename);

TGFCLIENT_API int* GfScrGetSupportedColorDepths(int* pnDepths);
TGFCLIENT_API int* GfScrGetDefaultColorDepths(int* pnDepths);
TGFCLIENT_API tScreenSize* GfScrGetSupportedSizes(int nColorDepth, bool bFullScreen, int* pnSizes);
TGFCLIENT_API tScreenSize* GfScrGetDefaultSizes(int* pnSizes);

/*****************************
 * GUI interface (low-level) *
 *****************************/

/* Widget type */
#define GFUI_LABEL      0
#define GFUI_BUTTON     1
#define GFUI_GRBUTTON   2
#define GFUI_SCROLLIST  3
#define GFUI_SCROLLBAR  4
#define GFUI_EDITBOX    5
#define GFUI_COMBOBOX	6
#define GFUI_CHECKBOX	7
#define GFUI_PROGRESSBAR 8

/* Alignment */
#define GFUI_ALIGN_HMASK  0x03
#define GFUI_ALIGN_HL  0x00
#define GFUI_ALIGN_HC  0x01
#define GFUI_ALIGN_HR  0x02

/* Mirror symetry */
#define GFUI_MIRROR_NONE  0x00 // No symetry (as is)
#define GFUI_MIRROR_HORI  0x01 // Vertical symetry
#define GFUI_MIRROR_VERT  0x02 // Horizontal symetry

// No more used for the moment (but keep it).
// #define GFUI_ALIGN_VMASK  0x30
// #define GFUI_ALIGN_VB  0x00
// #define GFUI_ALIGN_VC  0x10
// #define GFUI_ALIGN_VT  0x20

// No more used for the moment (but keep it).
// #define GFUI_ALIGN_HL_VB  0x00
// #define GFUI_ALIGN_HL_VC  0x10
// #define GFUI_ALIGN_HL_VT  0x20
// #define GFUI_ALIGN_HC_VB  0x01
// #define GFUI_ALIGN_HC_VC  0x11
// #define GFUI_ALIGN_HC_VT  0x21
// #define GFUI_ALIGN_HR_VB  0x02
// #define GFUI_ALIGN_HR_VC  0x12
// #define GFUI_ALIGN_HR_VT  0x22

/* Mouse action */
#define GFUI_MOUSE_UP   0
#define GFUI_MOUSE_DOWN 1

/* Keyboard action */
#define GFUI_KEY_UP     0
#define GFUI_KEY_DOWN   1

/* Scroll Bar position */
#define GFUI_SB_NONE    0
#define GFUI_SB_RIGHT   1
#define GFUI_SB_LEFT    2
#define GFUI_SB_TOP     3
#define GFUI_SB_BOTTOM  4

/* Scroll bar orientation */
#define GFUI_HORI_SCROLLBAR     0
#define GFUI_VERT_SCROLLBAR     1

// Values for creating menu controls from templates (indicate template value).
#define GFUI_TPL_TEXT         (const char*)-1
#define GFUI_TPL_TIP          (const char*)-1
#define GFUI_TPL_FONTID       (int)-1
#define GFUI_TPL_X            (int)INT_MAX
#define GFUI_TPL_Y            (int)INT_MAX
#define GFUI_TPL_WIDTH        (int)INT_MAX
#define GFUI_TPL_HEIGHT       (int)INT_MAX
#define GFUI_TPL_MAXLEN       (int)-1
#define GFUI_TPL_ALIGN        (int)-1
#define GFUI_TPL_COLOR        (const float*)-1
#define GFUI_TPL_FOCUSCOLOR   (const float*)-1
#define GFUI_TPL_PUSHEDCOLOR  (const float*)-1

// Some keyboard key modifier codes, to avoid SDLK constants everywhere.
// Note: Don't care with the L here, for tgfclient, it means L or R (no difference).
#define GFUIM_NONE       KMOD_NONE
#define GFUIM_CTRL       KMOD_LCTRL
#define GFUIM_SHIFT      KMOD_LSHIFT
#define GFUIM_ALT        KMOD_LALT
#if SDL_MAJOR_VERSION >= 2
#define GFUIM_META       KMOD_LGUI
#else
#define GFUIM_META       KMOD_LMETA
#endif

// Some keyboard key / special key codes, to avoid SDLK constants everywhere.
#define GFUIK_BACKSPACE	SDLK_BACKSPACE
#define GFUIK_TAB	SDLK_TAB
#define GFUIK_SPACE	SDLK_SPACE
#define GFUIK_CLEAR	SDLK_CLEAR
#define GFUIK_RETURN	SDLK_RETURN
#define GFUIK_PAUSE	SDLK_PAUSE
#define GFUIK_ESCAPE	SDLK_ESCAPE
#define GFUIK_DELETE	SDLK_DELETE

#define GFUIK_UP	SDLK_UP
#define GFUIK_DOWN	SDLK_DOWN
#define GFUIK_RIGHT	SDLK_RIGHT
#define GFUIK_LEFT	SDLK_LEFT
#define GFUIK_INSERT	SDLK_INSERT
#define GFUIK_HOME	SDLK_HOME
#define GFUIK_END	SDLK_END
#define GFUIK_PAGEUP	SDLK_PAGEUP
#define GFUIK_PAGEDOWN	SDLK_PAGEDOWN

#define GFUIK_F1	SDLK_F1
#define GFUIK_F2	SDLK_F2
#define GFUIK_F3	SDLK_F3
#define GFUIK_F4	SDLK_F4
#define GFUIK_F5	SDLK_F5
#define GFUIK_F6	SDLK_F6
#define GFUIK_F7	SDLK_F7
#define GFUIK_F8	SDLK_F8
#define GFUIK_F9	SDLK_F9
#define GFUIK_F10	SDLK_F10
#define GFUIK_F11	SDLK_F11
#define GFUIK_F12	SDLK_F12
#define GFUIK_F13	SDLK_F13
#define GFUIK_F14	SDLK_F14
#define GFUIK_F15	SDLK_F15

// Add needed other GFUIK_* here or above.

// Maximun value of a key code (Has to be the least greater  2^N - 1 >= SDLK_LAST)
#define GFUIK_MAX	GF_MAX_KEYCODE

#if (GFUIK_MAX < SDLK_LAST)
# error SDLK_MAX has grown too much, please increase GF_MAX_KEYCODE to the least greater power of 2 minus 1.
#endif


/** Scroll bar call-back information */
typedef struct ScrollBarInfo
{
    int         pos;            /**< Current scroll bar position */
    void        *userData;      /**< Associated user data */
} tScrollBarInfo;

/** Combo-box call-back information */
typedef struct ComboBoxInfo
{
	unsigned int nPos;                    /**< Currently selected choice */
	std::vector<std::string> vecChoices;  /**< Possible choices */
    void *userData;                       /**< Associated user data */
} tComboBoxInfo;


/** Check-box call-back information */
typedef struct CheckBoxInfo
{
    bool         bChecked;      /**< Check-box state */
    void        *userData;      /**< Associated user data */
} tCheckBoxInfo;

typedef void (*tfuiCallback)(void * /* userData */);
typedef void (*tfuiSBCallback)(tScrollBarInfo *);
typedef int (*tfuiKeyCallback)(int key, int modifier, int state);  /**< return 1 to prevent normal key computing */
typedef void (*tfuiComboboxCallback)(tComboBoxInfo *);
typedef void (*tfuiCheckboxCallback)(tCheckBoxInfo *);


/* Event loop callback functions (should be called explicitly if the corresponding
   event loop callback is overriden after a call to GfuiActivateScreen */
TGFCLIENT_API void GfuiDisplay(void);
TGFCLIENT_API void GfuiDisplayNothing(void);
TGFCLIENT_API void GfuiIdle(void);
TGFCLIENT_API void GfuiIdleMenu(void);

struct RmInfo;
typedef struct RmInfo tRmInfo;
TGFCLIENT_API void (*tfuiIdleCB(tRmInfo *info))(void);

/* Screen management */
TGFCLIENT_API void* GfuiScreenCreate(float *bgColor = 0, 
									 void *userDataOnActivate = 0, tfuiCallback onActivate = 0, 
									 void *userDataOnDeactivate = 0, tfuiCallback onDeactivate = 0, 
									 int mouseAllowed = 1);
TGFCLIENT_API void GfuiScreenRelease(void *screen);
TGFCLIENT_API void GfuiScreenActivate(void *screen);
TGFCLIENT_API int  GfuiScreenIsActive(void *screen);
TGFCLIENT_API void* GfuiGetScreen(void);
TGFCLIENT_API void GfuiScreenReplace(void *screen);
TGFCLIENT_API void GfuiScreenDeactivate(void);
TGFCLIENT_API void* GfuiHookCreate(void *userDataOnActivate, tfuiCallback onActivate);
TGFCLIENT_API void GfuiHookRelease(void *hook);
TGFCLIENT_API void GfuiAddKey(void *scr, int key, const char *descr,
							  void *userData, tfuiCallback onKeyPressed, tfuiCallback onKeyReleased);
TGFCLIENT_API void GfuiAddKey(void *scr, int key, int modifier, const char *descr,
							  void *userData, tfuiCallback onKeyPressed, tfuiCallback onKeyReleased);
TGFCLIENT_API void GfuiRegisterKey(int key, const char *descr,
								   void *userData, tfuiCallback onKeyPressed, tfuiCallback onKeyReleased);
TGFCLIENT_API void GfuiSetKeyAutoRepeat(void *scr, int on);
TGFCLIENT_API void GfuiHelpScreen(void *targetScreen);
TGFCLIENT_API void GfuiHelpScreen(void *targetScreen, void *returnScreen);
TGFCLIENT_API void GfuiScreenShot(void *notused);
TGFCLIENT_API void GfuiScreenAddBgImg(void *scr, const char *filename);
TGFCLIENT_API void GfuiScreenAddMusic(void *scr, const char *filename);
TGFCLIENT_API void GfuiKeyEventRegister(void *scr, tfuiKeyCallback onKeyAction);
TGFCLIENT_API void GfuiKeyEventRegisterCurrent(tfuiKeyCallback onKeyAction);
TGFCLIENT_API void GfuiInitWindowPositionAndSize(int x, int y, int w, int h);

TGFCLIENT_API bool GfuiRemoveKey(void *scr, int key, const char *descr);
TGFCLIENT_API bool GfuiRemoveKey(void *scr, int key, int modifier, const char *descr);

TGFCLIENT_API void GfuiRedraw(void);
TGFCLIENT_API void GfuiSwapBuffers(void);

class TGFCLIENT_API GfuiMenuScreen
{
public:
	GfuiMenuScreen(const char* pszXMLDescFile);
	virtual ~GfuiMenuScreen();

	void createMenu(float* bgColor = 0, 
					void* userDataOnActivate = 0, tfuiCallback onActivate = 0, 
					void* userDataOnDeactivate = 0, tfuiCallback onDeactivate = 0, 
					int mouseAllowed = 0);
	void setMenuHandle(void* hdle);
	void* getMenuHandle() const;
	void setPreviousMenuHandle(void* hdle);
	void* getPreviousMenuHandle() const;

	bool openXMLDescriptor();

	bool createStaticControls();

	int createButtonControl(const char* pszName, void* userData, tfuiCallback onPush,
							void* userDataOnFocus = 0, tfuiCallback onFocus = 0,
							tfuiCallback onFocusLost = 0);
	int createTextButtonControl(const char* pszName, void* userDataOnPush, tfuiCallback onPush,
								void* userDataOnFocus, tfuiCallback onFocus,
								tfuiCallback onFocusLost,
								bool bFromTemplate = false,
								const char* text = GFUI_TPL_TEXT, const char* tip = GFUI_TPL_TIP,
								int x = GFUI_TPL_X, int y = GFUI_TPL_Y,
								int width = GFUI_TPL_WIDTH,
								int font = GFUI_TPL_FONTID, int textHAlign = GFUI_TPL_ALIGN, 
								const float* fgColor = GFUI_TPL_COLOR,
								const float* fgFocusColor = GFUI_TPL_FOCUSCOLOR,
								const float* fgPushedColor = GFUI_TPL_PUSHEDCOLOR);
	int createImageButtonControl(const char* pszName,
								 void* userDataOnPush, tfuiCallback onPush,
								 void* userDataOnFocus, tfuiCallback onFocus, tfuiCallback onFocusLost,
								 bool bFromTemplate = false,
								 const char* tip = GFUI_TPL_TEXT,
								 int x = GFUI_TPL_X, int y = GFUI_TPL_Y,
								 int width = GFUI_TPL_WIDTH, int height = GFUI_TPL_HEIGHT);
	int createStaticImageControl(const char* pszName);
	int createLabelControl(const char* pszName,
						   bool bFromTemplate = false,
						   const char* pszText = GFUI_TPL_TEXT,
						   int nX = GFUI_TPL_X, int nY = GFUI_TPL_Y,
						   int nFontId = GFUI_TPL_FONTID, int nWidth = GFUI_TPL_WIDTH,
						   int nHAlignId = GFUI_TPL_ALIGN, int nMaxLen = GFUI_TPL_MAXLEN, 
						   const float* aFgColor = GFUI_TPL_COLOR,
						   const float* aFgFocusColor = GFUI_TPL_FOCUSCOLOR);
	int createEditControl(const char* pszName, void* userDataOnFocus, tfuiCallback onFocus,
						  tfuiCallback onFocusLost);
	int createScrollListControl(const char* pszName,void* userData, tfuiCallback onSelect);
	int createComboboxControl(const char* pszName, void* userData, tfuiComboboxCallback onChange);
	int createCheckboxControl(const char* pszName, void* userData, tfuiCheckboxCallback onChange);
	int createProgressbarControl(const char* pszName);
	
	int getDynamicControlId(const char* pszName) const;

	void addDefaultShortcuts();
	void addShortcut(int key, const char* descr, void* userData,
					 tfuiCallback onKeyPressed, tfuiCallback onKeyReleased);
	
	bool closeXMLDescriptor();
	
	void runMenu();

private:
	struct gfuiMenuPrivateData* m_priv;
};

/* Mouse management */
typedef struct MouseInfo
{
    int X;
    int Y;
    int button[7];
} tMouseInfo;

TGFCLIENT_API tMouseInfo* GfuiMouseInfo(void);
TGFCLIENT_API void GfuiMouseSetPos(int x, int y);
TGFCLIENT_API void GfuiMouseHide(void);
TGFCLIENT_API void GfuiMouseShow(void);
TGFCLIENT_API void GfuiMouseToggleVisibility(void);
TGFCLIENT_API void GfuiMouseSetHWPresent(void);
TGFCLIENT_API bool GfuiMouseIsHWPresent(void);


/* Color management */
class TGFCLIENT_API GfuiColor
{
  public:
	static GfuiColor build(float r, float g, float b, float a = 1.0);
	static GfuiColor build(const char* pszARGB); // Ex: Semi-translucent tangerine "0x80F28500"
	static GfuiColor build(const float* color);
	static GfuiColor build(int index); // index from GFUI_* "named" indexes in gui.h.
    inline const float *toFloatRGBA() const { return (float*)this; }
  public:
    float red, green, blue, alpha; // Each inside [0, 1].
};

/* All widgets */
#define GFUI_VISIBLE    1       /**< Object visibility flag  */
#define GFUI_INVISIBLE  0       /**< Object invisibility flag  */
TGFCLIENT_API int GfuiVisibilitySet(void* scr, int id, int visible);
#define GFUI_DISABLE    1
#define GFUI_ENABLE     0
TGFCLIENT_API int GfuiEnable(void* scr, int id, int flag);
TGFCLIENT_API void GfuiUnSelectCurrent(void);

/* Font management */
// Font Ids
#define GFUI_FONT_BIG           0
#define GFUI_FONT_LARGE         1
#define GFUI_FONT_MEDIUM        2
#define GFUI_FONT_SMALL         3
#define GFUI_FONT_BIG_C         4
#define GFUI_FONT_LARGE_C       5
#define GFUI_FONT_MEDIUM_C      6
#define GFUI_FONT_SMALL_C       7
#define GFUI_FONT_BIG_T         8
#define GFUI_FONT_LARGE_T       9
#define GFUI_FONT_MEDIUM_T     10
#define GFUI_FONT_SMALL_T      11
#define GFUI_FONT_DIGIT        12

#define GFUI_FONT_NB	       13 // Warning: Keep always this sync'd with GFUI_FONT_* above !

TGFCLIENT_API int  GfuiFontHeight(int font);
TGFCLIENT_API int  GfuiFontWidth(int font, const char* text);
TGFCLIENT_API void GfuiDrawString(const char* text, float* fgColor, int font,
								  int x, int y, int width = 0, int hAlign = GFUI_ALIGN_HL);

/* Labels */
TGFCLIENT_API int GfuiLabelCreate(void* scr, const char* text,
								  int font, int x, int y, int width, int hAlign, int maxlen, 
								  const float* fgColor = 0, const float* fgFocusColor = 0,
								  void* userDataOnFocus = 0, tfuiCallback onFocus = 0, tfuiCallback onFocusLost = 0);

TGFCLIENT_API void GfuiSetTipPosition(int x,int y);
TGFCLIENT_API int GfuiTipCreate(void* scr, const char* text, int maxlen);

TGFCLIENT_API void GfuiLabelSetText(void* scr, int id, const char* text);
TGFCLIENT_API void GfuiLabelSetColor(void* scr, int id, const float* color);

/* Buttons */
#define GFUI_BTNSZ      300
TGFCLIENT_API int GfuiButtonCreate(void* scr, const char* text, int font,
								   int x, int y, int width, int textHAlign, int mouse,
								   void* userDataOnPush, tfuiCallback onPush, 
								   void* userDataOnFocus, tfuiCallback onFocus, tfuiCallback onFocusLost);
TGFCLIENT_API int GfuiButtonStateCreate(void* scr, const char* text, int font, int x, int y,
										int width, int textHAlign, int mouse,
										void* userDataOnPush, tfuiCallback onPush, 
										void* userDataOnFocus, tfuiCallback onFocus, tfuiCallback onFocusLost);
TGFCLIENT_API int GfuiGrButtonCreate(void* scr, const char* disabled, const char* enabled,
									 const char* focused, const char* pushed,
									 int x, int y, int width, int height,
									 int mirror, bool padding, int mouse,
									 void* userDataOnPush, tfuiCallback onPush, 
									 void* userDataOnFocus, tfuiCallback onFocus, tfuiCallback onFocusLost);

TGFCLIENT_API void GfuiButtonShowBox(void* scr, int id,bool bShow);
TGFCLIENT_API void GfuiButtonSetColors(void *scr, int id, const GfuiColor& color,
									   const GfuiColor& focusColor, const GfuiColor& pushColor);
TGFCLIENT_API void GfuiButtonSetImage(void* scr, int id, int x, int y, int w, int h,
									  const char* disableFile, const char* enableFile,
									  const char*focusedFile, const char* pushedFile);

/* Progress bars */
TGFCLIENT_API int GfuiProgressbarCreate(void* scr, int x, int y, int w, int h,
										const char* pszProgressbackImg, const char* progressbarimg,
										const float* outlineColor,
										float min, float max, float value, 
										void* userDataOnFocus, tfuiCallback onFocus, tfuiCallback onFocusLost);
TGFCLIENT_API void GfuiProgressbarSetValue(void* scr, int id, float value);

/* Combo Boxes */
TGFCLIENT_API int GfuiComboboxCreate(void* scr, int font, int x, int y, int width,
									 int arrowsWidth, int arrowsHeight,
									 const char* pszText, int maxlen,
									 const float* fgColor, const float* fgFocusColor,
									 void* userData, tfuiComboboxCallback onChange,
									 void* userDataOnFocus, tfuiCallback onFocus, tfuiCallback onFocusLost);
TGFCLIENT_API unsigned int GfuiComboboxAddText(void* scr, int id, const char* text);
TGFCLIENT_API void GfuiComboboxSetTextColor(void* scr, int id, const GfuiColor& color);
TGFCLIENT_API void GfuiComboboxSetSelectedIndex(void* scr, int id, unsigned int index);
TGFCLIENT_API void GfuiComboboxSetPosition(void* scr, int id, unsigned int pos);
TGFCLIENT_API unsigned GfuiComboboxGetPosition(void* scr, int id);
TGFCLIENT_API const char* GfuiComboboxGetText(void* scr, int id);
TGFCLIENT_API void GfuiComboboxClear(void* scr, int id);
TGFCLIENT_API unsigned GfuiComboboxGetNumberOfChoices(void* scr, int id);

/* Check Boxes */
TGFCLIENT_API int GfuiCheckboxCreate(void* scr, int font, int x, int y,
									 int imagewidth, int imageheight,
									 const char* pszText, bool bChecked,
									 void* userData, tfuiCheckboxCallback onChange,
									 void* userDataOnFocus, tfuiCallback onFocus, tfuiCallback onFocusLost);
TGFCLIENT_API void GfuiCheckboxSetChecked(void* scr, int id, bool bChecked);
TGFCLIENT_API void GfuiCheckboxSetText(void* scr, int id, const char *text);
TGFCLIENT_API void GfuiCheckboxSetTextColor(void* scr, int id, const GfuiColor& color);


TGFCLIENT_API void GfuiButtonSetText(void* scr, int id, const char* text);
TGFCLIENT_API int GfuiButtonGetFocused(void);

/* Edit Boxes */
TGFCLIENT_API int GfuiEditboxCreate(void* scr, const char* text, int font,
									int x, int y, int width, int maxlen, int align,
									void* userDataOnFocus, tfuiCallback onFocus, tfuiCallback onFocusLost);
TGFCLIENT_API int GfuiEditboxGetFocused(void);
TGFCLIENT_API char* GfuiEditboxGetString(void* scr, int id);
TGFCLIENT_API void GfuiEditboxSetString(void* scr, int id, const char* text);
TGFCLIENT_API void GfuiEditboxSetColors(void *scr, int id, const GfuiColor& color,
										const GfuiColor& focusColor, const GfuiColor& disabledColor);
TGFCLIENT_API void GfuiEditboxSetBGColors(void *scr, int id, const GfuiColor& color,
										  const GfuiColor& focusColor, const GfuiColor& disabledColor);

/* Scroll lists */
TGFCLIENT_API int GfuiScrollListCreate(void* scr, int font, int x, int y, int width, int height,
									   int scrollBarPos, int scrollBarWidth, int scrollBarButHeight,
									   void* userDataOnSelect, tfuiCallback onSelect);
TGFCLIENT_API int GfuiScrollListInsertElement(void* scr, int Id, const char* element, int index, void* userData);
TGFCLIENT_API int GfuiScrollListMoveSelectedElement(void* scr, int Id, int delta);
TGFCLIENT_API const char* GfuiScrollListExtractSelectedElement(void* scr, int Id, void** userData);
TGFCLIENT_API const char* GfuiScrollListExtractElement(void* scr, int Id, int index, void** userData);
TGFCLIENT_API void GfuiScrollListClear(void* scr, int Id);
TGFCLIENT_API int GfuiScrollListGetSelectedElementIndex(void *scr, int id);
TGFCLIENT_API const char* GfuiScrollListGetSelectedElement(void* scr, int Id, void** userData);
TGFCLIENT_API bool GfuiScrollListSetSelectedElement(void* scr, int Id, unsigned int selectElement);
TGFCLIENT_API bool GfuiScrollListClearSelection(void* scr, int Id);
TGFCLIENT_API const char* GfuiScrollListGetElement(void* scr, int Id, int index, void** userData);
TGFCLIENT_API int GfuiScrollListGetNumberOfElements(void* scr, int Id);
TGFCLIENT_API void GfuiScrollListShowElement(void* scr, int Id, int index);
TGFCLIENT_API void GfuiScrollListSetColors(void* scr, int id, const GfuiColor& color, const GfuiColor& selectColor);

/* Scroll bars */
TGFCLIENT_API int GfuiScrollBarCreate(void* scr, int x, int y,
									  int length, int thickness, int butLength,
									  int orientation, int position,
									  int min, int max, int len, int start, 
									  void* userData, tfuiSBCallback onScroll);
TGFCLIENT_API void GfuiScrollBarPosSet(void* scr, int id, int min, int max, int len, int start);
TGFCLIENT_API int GfuiScrollBarPosGet(void* scr, int id);

/* Images */
TGFCLIENT_API int GfuiStaticImageCreate(void* scr, int x, int y, int w, int h, const char* name,
										bool canDeform = true);
TGFCLIENT_API void GfuiStaticImageSet(void* scr, int id, const char* name, unsigned index = 0);
TGFCLIENT_API void GfuiStaticImageSetActive(void* scr, int id, int index);
TGFCLIENT_API void GfuiStaticImageSetDeformable(void *scr, int id, bool canDeform = true);

/*****************************
 * Menu Management Interface *
 *****************************/

TGFCLIENT_API void  GfuiMenuDefaultKeysAdd(void* scr);

/*******************************************
 * New XML based Menu Management Interface *
 *******************************************/

TGFCLIENT_API void* GfuiMenuLoad(const char* pFilePath);
TGFCLIENT_API bool GfuiMenuCreateStaticControls(void* hscr, void* hparm);

TGFCLIENT_API int GfuiMenuCreateButtonControl(void* hscr, void* hparm, const char* pszName,
											  void* userDataOnPush, tfuiCallback onPush,
											  void* userDataOnFocus = 0, tfuiCallback onFocus = 0, tfuiCallback onFocusLost = 0);
TGFCLIENT_API int GfuiMenuCreateTextButtonControl(void* hscr, void* hparm, const char* pszName,
												  void* userDataOnPush, tfuiCallback onPush,
												  void* userDataOnFocus = 0, tfuiCallback onFocus = 0, tfuiCallback onFocusLost = 0,
												  bool bFromTemplate = false,
												  const char* text = GFUI_TPL_TEXT,
												  const char* tip = GFUI_TPL_TIP,
												  int x = GFUI_TPL_X, int y = GFUI_TPL_Y,
												  int width = GFUI_TPL_WIDTH,
												  int font = GFUI_TPL_FONTID, int textHAlign = GFUI_TPL_ALIGN, 
												  const float* fgColor = GFUI_TPL_COLOR,
												  const float* fgFocusColor = GFUI_TPL_FOCUSCOLOR,
												  const float* fgPushedColor = GFUI_TPL_PUSHEDCOLOR);
TGFCLIENT_API int GfuiMenuCreateImageButtonControl(void* hscr, void* hparm, const char* pszName,
												   void* userDataOnPush, tfuiCallback onPush,
												   void* userDataOnFocus = 0, tfuiCallback onFocus = 0, tfuiCallback onFocusLost = 0,
												   bool bFromTemplate = false,
												   const char* tip = GFUI_TPL_TIP,
												   int x = GFUI_TPL_X, int y = GFUI_TPL_Y,
												   int width = GFUI_TPL_WIDTH,
												   int height = GFUI_TPL_HEIGHT);
TGFCLIENT_API int GfuiMenuCreateStaticImageControl(void* hscr, void* hparm, const char* pszName);
TGFCLIENT_API int GfuiMenuCreateLabelControl(void* hscr, void* hparm, const char* pszName,
											 bool bFromTemplate = false,
											 const char* text = GFUI_TPL_TEXT,
											 int x = GFUI_TPL_X, int y = GFUI_TPL_Y,
											 int font = GFUI_TPL_FONTID,
											 int width = GFUI_TPL_WIDTH, 
											 int hAlign = GFUI_TPL_ALIGN,
											 int maxlen = GFUI_TPL_MAXLEN, 
											 const float* fgColor = GFUI_TPL_COLOR,
											 const float* fgFocusColor = GFUI_TPL_FOCUSCOLOR);
TGFCLIENT_API int GfuiMenuCreateEditControl(void* hscr, void* hparm, const char* pszName,
											void* userDataOnFocus, tfuiCallback onFocus, tfuiCallback onFocusLost);
TGFCLIENT_API int GfuiMenuCreateScrollListControl(void* hscr, void* hparm, const char* pszName,
												  void* userData, tfuiCallback onSelect);
TGFCLIENT_API int GfuiMenuCreateComboboxControl(void* hscr, void* hparm, const char* pszName,
												void* userData, tfuiComboboxCallback onChange);
TGFCLIENT_API int GfuiMenuCreateCheckboxControl(void* hscr, void* hparm, const char* pszName,
												void* userData, tfuiCheckboxCallback onChange);
TGFCLIENT_API int GfuiMenuCreateProgressbarControl(void* hscr, void* hparm, const char* pszName);

TGFCLIENT_API tdble GfuiMenuGetNumProperty(void* hparm, const char* pszName,
										   tdble nDefVal, const char* pszUnit = 0);
TGFCLIENT_API const char* GfuiMenuGetStrProperty(void* hparm, const char* pszName,
												 const char* pszDefVal);

/*****************************
 * Texture / image interface *
 *****************************/

TGFCLIENT_API unsigned char* GfTexReadImageFromFile(const char* filename, float screen_gamma, int* pWidth, int* pHeight, int* pPow2Width = 0, int* pPow2Height = 0);
TGFCLIENT_API unsigned char* GfTexReadImageFromPNG(const char* filename, float screen_gamma, int* pWidth, int* pHeight, int* pPow2Width = 0, int* pPow2Height = 0, bool useGammaCorrection = true);
TGFCLIENT_API unsigned char* GfTexReadImageFromJPEG(const char* filename, float screen_gamma, int* pWidth, int* pHeight, int* pPow2Width = 0, int* pPow2Height = 0);

TGFCLIENT_API int GfTexWriteImageToPNG(unsigned char* img, const char* filename, int width, int height);
TGFCLIENT_API void GfTexFreeTexture(unsigned glTexId);
TGFCLIENT_API unsigned GfTexReadTexture(const char* filename, int* pWidth = 0, int* pHeight = 0,
										int* pPow2Width = 0, int* pPow2Height = 0);


/*********************
 * Control interface *
 *********************/

#define GFCTRL_TYPE_NOT_AFFECTED        0
#define GFCTRL_TYPE_JOY_AXIS            1
#define GFCTRL_TYPE_JOY_BUT             2
#define GFCTRL_TYPE_KEYBOARD            3
#define GFCTRL_TYPE_MOUSE_BUT           4
#define GFCTRL_TYPE_MOUSE_AXIS          5
#define GFCTRL_TYPE_JOY_ATOB            6

typedef struct
{
    int         index;
    int         type;
} tCtrlRef;


#define GFCTRL_JOY_UNTESTED     -1
#define GFCTRL_JOY_NONE         0
#define GFCTRL_JOY_PRESENT      1

#define GFCTRL_JOY_NUMBER       8 /* Max number of managed joysticks */
#define GFCTRL_JOY_MAX_BUTTONS  32       /* Size of integer so don't change please */
#if SDL_JOYSTICK
#define GFCTRL_JOY_MAX_AXES     12
#else
#define GFCTRL_JOY_MAX_AXES      _JS_MAX_AXES
#endif

/** Joystick Information Structure */
typedef struct
{
#if SDL_JOYSTICK
    int         oldb[GFCTRL_JOY_MAX_BUTTONS * GFCTRL_JOY_NUMBER];
#else
    int         oldb[GFCTRL_JOY_NUMBER];
#endif
    float       ax[GFCTRL_JOY_MAX_AXES * GFCTRL_JOY_NUMBER];         /**< Axis values */
    int         edgeup[GFCTRL_JOY_MAX_BUTTONS * GFCTRL_JOY_NUMBER];  /**< Button transition from down (pressed) to up */
    int         edgedn[GFCTRL_JOY_MAX_BUTTONS * GFCTRL_JOY_NUMBER];  /**< Button transition from up to down */
    int         levelup[GFCTRL_JOY_MAX_BUTTONS * GFCTRL_JOY_NUMBER]; /**< Button state (1 = up) */
} tCtrlJoyInfo;

TGFCLIENT_API int GfctrlJoyIsAnyPresent(void);
TGFCLIENT_API tCtrlJoyInfo* GfctrlJoyCreate(void);
TGFCLIENT_API void GfctrlJoyRelease(tCtrlJoyInfo* joyInfo);
TGFCLIENT_API int GfctrlJoyGetCurrentStates(tCtrlJoyInfo* joyInfo);
 #if SDL_JOYSTICK
TGFCLIENT_API void gfctrlJoyConstantForce(int index, unsigned int level, int dir);
TGFCLIENT_API void gfctrlJoyRumble(int index, float level);
TGFCLIENT_API void GfctrlJoySetAxis(int joy, int axis, float value);
TGFCLIENT_API void GfctrlJoySetButton(int joy, int button, int value);
#endif


/** Mouse information structure */
typedef struct
{
    int         edgeup[7];      /**< Button transition from down (pressed) to up */
    int         edgedn[7];      /**< Button transition from up to down */
    int         button[7];      /**< Button state (1 = up) */
    float       ax[4];          /**< mouse axis position (mouse considered as a joystick) */
} tCtrlMouseInfo;

TGFCLIENT_API tCtrlMouseInfo* GfctrlMouseCreate(void);
TGFCLIENT_API void GfctrlMouseRelease(tCtrlMouseInfo* mouseInfo);
TGFCLIENT_API int GfctrlMouseGetCurrentState(tCtrlMouseInfo* mouseInfo);
TGFCLIENT_API void GfctrlMouseCenter(void);
TGFCLIENT_API void GfctrlMouseInitCenter(void);

TGFCLIENT_API tCtrlRef* GfctrlGetRefByName(const char* name);
TGFCLIENT_API const char* GfctrlGetNameByRef(int type, int index);

//****************************************
// The GUI event loop class

class TGFCLIENT_API GfuiEventLoop : public GfEventLoop
{
  public: // Member functions.

	//! Constructor
	GfuiEventLoop();

	//! Destructor
	virtual ~GfuiEventLoop();

	//! The real event loop function : 1) process events, 2) do the 'computing' job, 3) do the 'displaying' job.
	virtual void operator()(void);

	//! Set the "mouse button pressed" callback function.
	void setMouseButtonCB(void (*func)(int button, int state, int x, int y));

	//! Set the "mouse motion with button pressed" callback function.
	void setMouseMotionCB(void (*func)(int x, int y));

	//! Set the "mouse motion without button pressed" callback function.
	void setMousePassiveMotionCB(void (*func)(int x, int y));

#if SDL_JOYSTICK
	//! Set the "joystick axis moved" callback function.
	void setJoystickAxisCB(void (*func)(int joy, int axis, float value));

	//! Set the "joystick button pressed" callback function.
	void setJoystickButtonCB(void (*func)(int joy, int button, int value));
#endif

	//! Set the "redisplay/refresh" callback function. 
	void setRedisplayCB(void (*func)(void));

	//! Set the "reshape" callback function with given new screen/window geometry.
	void setReshapeCB(void (*func)(int width, int height));

	//! Post a "redisplay/refresh" event to the event loop. 
	void postRedisplay(void);

	//! Force a call to the "redisplay/refresh" callback function. 
	void forceRedisplay();

  protected:
	
	//! Process a keyboard event.
	void injectKeyboardEvent(int code, int modifier, int state,
							 int unicode, int x = 0, int y = 0);
	
	//! Process a mouse motion event.
	void injectMouseMotionEvent(int state, int x, int y);
	
	//! Process a mouse button event.
	void injectMouseButtonEvent(int button, int state, int x, int y);

#if SDL_JOYSTICK
	//! Process a joystick axis event.
	void injectJoystickAxisEvent(int joy, int axis, float value);

	//! Process a joystick button event.
	void injectJoystickButtonEvent(int joy, int button, int value);
#endif

	//! Process a redisplay event.
	void redisplay();
	
  private: // Member data.

	//! Private data (pimp pattern).
	class Private;
	Private* _pPrivate;
};

//****************************************
// GUI Application base class

class TGFCLIENT_API GfuiApplication : public GfApplication
{
 public:

	//! Constructor.
    GfuiApplication(const char* pszName, const char* pszVersion, const char* pszDesc);

	//! Destructor.
	virtual ~GfuiApplication();
	
	//! Initialization.
    virtual void initialize(bool bLoggingEnabled, int argc = 0, char **argv = 0);

	//! Parse the command line options (updates _lstOptionsLeft).
	bool parseOptions();

	//! Setup the window / screen (+ menu infrastructure if specified) (with given size specs if >= 0, or from screen.xml).
	bool setupWindow(bool bNoMenu = false, int nWinWidth = -1, int nWinHeight = -1, int nFullScreen = -1);

	//! Application event loop.
	GfuiEventLoop& eventLoop();
	
	//! Restart the app.
	virtual void restart();

 protected:

	//! True if setupWindow was successfull.
	bool _bWindowUp;
};


//! Shortcut to the application singleton.
inline GfuiApplication& GfuiApp()
{
	return dynamic_cast<GfuiApplication&>(GfApplication::self());
}

// TODO: Use dynamic array for OwnerOfScreens
#define MAXSCREENS 100
void RegisterScreens(void* screen);
void FreeScreens();
void FreeScreen(void* screen);
void ScreenRelease(void* screen);
void UnregisterScreens(void* screen);

#endif /* __TGFCLIENT__H__ */


