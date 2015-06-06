/***************************************************************************
                          menu.cpp -- menu management                            
                             -------------------                                         
    created              : Fri Aug 13 22:23:19 CEST 1999
    copyright            : (C) 1999 by Eric Espie                         
    email                : torcs@free.fr   
    version              : $Id: guimenu.cpp 5108 2013-01-25 20:47:12Z torcs-ng $                                  
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
                GUI menu management.
    @author     <a href=mailto:torcs@free.fr>Eric Espie</a>
    @version    $Id: guimenu.cpp 5108 2013-01-25 20:47:12Z torcs-ng $
    @ingroup    gui
*/


#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>
#include <map>

#include <portability.h>

#include "tgfclient.h"
#include "gui.h"
#include "guimenu.h"


void
gfuiInitMenu(void)
{
}

static void
onFocusShowTip(void* cbinfo)
{
    GfuiVisibilitySet(((tMenuCallbackInfo*)cbinfo)->screen,
					  ((tMenuCallbackInfo*)cbinfo)->labelId, GFUI_VISIBLE);
}

static void
onFocusLostHideTip(void* cbinfo)
{
    GfuiVisibilitySet(((tMenuCallbackInfo*)cbinfo)->screen,
					  ((tMenuCallbackInfo*)cbinfo)->labelId, GFUI_INVISIBLE);
}

/***********************************************************************************
 * Menu XML descriptor management
*/

/** Add the default menu keyboard callback to a screen.
    The keys are:
    <br><tt>F1 .......... </tt>Help
    <br><tt>Enter ....... </tt>Perform Action
    <br><tt>Up Arrow .... </tt>Select Previous Entry
    <br><tt>Down Arrow .. </tt>Select Next Entry
    <br><tt>Page Up ..... </tt>Select Previous Entry
    <br><tt>Page Down ... </tt>Select Next Entry
    <br><tt>Tab ......... </tt>Select Next Entry
    <br><tt>F12 ......... </tt>Screen shot
    @ingroup    gui
    @param      scr     Screen Id
 */
void
GfuiMenuDefaultKeysAdd(void* scr)
{
    GfuiAddKey(scr, GFUIK_TAB, "Select Next Entry", NULL, gfuiSelectNext, NULL);
    GfuiAddKey(scr, GFUIK_RETURN, "Perform Action", (void*)2, gfuiMouseAction, NULL);
    GfuiAddKey(scr, GFUIK_UP, "Select Previous Entry", NULL, gfuiSelectPrev, NULL);
    GfuiAddKey(scr, GFUIK_DOWN, "Select Next Entry", NULL, gfuiSelectNext, NULL);
    GfuiAddKey(scr, GFUIK_PAGEUP, "Select Previous Entry", NULL, gfuiSelectPrev, NULL);
    GfuiAddKey(scr, GFUIK_PAGEDOWN, "Select Next Entry", NULL, gfuiSelectNext, NULL);
    GfuiAddKey(scr, GFUIK_F1, "Help", scr, GfuiHelpScreen, NULL);
    GfuiAddKey(scr, GFUIK_F12, "Screen-Shot", NULL, GfuiScreenShot, NULL);
}

// Font size map : Gives the integer size from the size name.
typedef std::map<std::string, int> TMapFontSize;
static const TMapFontSize::value_type AMapFontSize[] = 
{ 
    TMapFontSize::value_type("big",      GFUI_FONT_BIG),
    TMapFontSize::value_type("large",    GFUI_FONT_LARGE),
    TMapFontSize::value_type("medium",   GFUI_FONT_MEDIUM),
    TMapFontSize::value_type("small",    GFUI_FONT_SMALL),

    TMapFontSize::value_type("big_c",    GFUI_FONT_BIG_C),
    TMapFontSize::value_type("large_c",  GFUI_FONT_LARGE_C),
    TMapFontSize::value_type("medium_c", GFUI_FONT_MEDIUM_C),
    TMapFontSize::value_type("small_c",  GFUI_FONT_SMALL_C),

    TMapFontSize::value_type("big_t",    GFUI_FONT_BIG_T),
    TMapFontSize::value_type("large_t",  GFUI_FONT_LARGE_T),
    TMapFontSize::value_type("medium_t", GFUI_FONT_MEDIUM_T),
    TMapFontSize::value_type("small_t",  GFUI_FONT_SMALL_T),

    TMapFontSize::value_type("digit",    GFUI_FONT_DIGIT),
};

static const TMapFontSize MapFontSize(AMapFontSize, AMapFontSize + sizeof(AMapFontSize) / sizeof(TMapFontSize::value_type)); 

int
gfuiMenuGetFontId(const char* pszFontName)
{
    const TMapFontSize::const_iterator itFontId = MapFontSize.find(pszFontName);
    
    if (itFontId != MapFontSize.end())
        return (*itFontId).second;
    else
        return GFUI_FONT_MEDIUM; // Default font.
}

// Alignment map : Gives the integer bit field value from the name.
// typedef std::map<std::string, int> TMapAlign;
// static const TMapAlign::value_type AMapAlign[] = 
// { 
//     TMapAlign::value_type("left.bottom",   GFUI_ALIGN_HL_VB),
//     TMapAlign::value_type("center.bottom", GFUI_ALIGN_HC_VB),
//     TMapAlign::value_type("right.bottom",  GFUI_ALIGN_HR_VB),
//     TMapAlign::value_type("left.center",   GFUI_ALIGN_HL_VC),
//     TMapAlign::value_type("center.center", GFUI_ALIGN_HC_VC),
//     TMapAlign::value_type("right.center",  GFUI_ALIGN_HR_VC),
//     TMapAlign::value_type("left.top",      GFUI_ALIGN_HL_VT),
//     TMapAlign::value_type("center.top",    GFUI_ALIGN_HC_VT),
//     TMapAlign::value_type("right.top",     GFUI_ALIGN_HR_VT)
// };

// static const TMapAlign MapAlign(AMapAlign, AMapAlign + sizeof(AMapAlign) / sizeof(TMapAlign::value_type)); 

// Horizontal alignment map : Gives the integer bit field value from the name.
typedef std::map<std::string, int> TMapHorizAlign;
static const TMapHorizAlign::value_type AMapHorizAlign[] = 
{ 
    TMapHorizAlign::value_type(GFMNU_VAL_LEFT,   GFUI_ALIGN_HL),
    TMapHorizAlign::value_type(GFMNU_VAL_CENTER, GFUI_ALIGN_HC),
    TMapHorizAlign::value_type(GFMNU_VAL_RIGHT,  GFUI_ALIGN_HR)
};

static const TMapHorizAlign MapHorizAlign(AMapHorizAlign, AMapHorizAlign + sizeof(AMapHorizAlign) / sizeof(TMapHorizAlign::value_type)); 

int 
gfuiMenuGetAlignment(const char* pszHAlign) //, const char* pszVAlign)
{
    std::string strAlign(pszHAlign);
    if (strlen(pszHAlign) == 0)
        strAlign += "left"; // Default horizontal alignment
	
	// if (pszVAlign)
	// {
	// 	strAlign += '.';
	// 	strAlign += pszVAlign;
	// 	if (!pszVAlign || strlen(pszVAlign) == 0)
	// 		strAlign += "bottom"; // Default bottom alignment
    
	// 	const TMapAlign::const_iterator itAlign = MapAlign.find(strAlign);
		
	// 	if (itAlign != MapAlign.end())
	// 		return (*itAlign).second;
	// 	else
	// 		return GFUI_ALIGN_HL_VB; // Default alignment.
	// }
	// else
	{
		const TMapHorizAlign::const_iterator itHorizAlign = MapHorizAlign.find(strAlign);
    
		if (itHorizAlign != MapHorizAlign.end())
			return (*itHorizAlign).second;
		else
			return GFUI_ALIGN_HL; // Default horizontal alignement = left.
	}
}

// Scrollbar position map : Gives the integer value from the name.
typedef std::map<std::string, int> TMapScrollBarPos;
static const TMapScrollBarPos::value_type AMapScrollBarPos[] = 
{ 
    TMapScrollBarPos::value_type("none",  GFUI_SB_NONE),
    TMapScrollBarPos::value_type("left",  GFUI_SB_LEFT),
    TMapScrollBarPos::value_type("right", GFUI_SB_RIGHT),
};

static const TMapScrollBarPos MapScrollBarPos(AMapScrollBarPos, AMapScrollBarPos + sizeof(AMapScrollBarPos) / sizeof(TMapScrollBarPos::value_type)); 

int 
gfuiMenuGetScrollBarPosition(const char* pszSBPos)
{
    const TMapScrollBarPos::const_iterator itScrollBarPos = MapScrollBarPos.find(pszSBPos);
    
    if (itScrollBarPos != MapScrollBarPos.end())
        return (*itScrollBarPos).second;
    else
        return GFUI_SB_NONE; // Default.
}

bool 
gfuiMenuGetBoolean(const char* pszValue, bool bDefault)
{
	if (pszValue)
	{
		if (!strcmp(pszValue, GFMNU_VAL_YES) || !strcmp(pszValue, GFMNU_VAL_TRUE))
			return true;
		else if (!strcmp(pszValue, GFMNU_VAL_NO) || !strcmp(pszValue, GFMNU_VAL_FALSE))
			return false;
	}

	return bDefault;
}

static bool 
getControlBoolean(void* hparm, const char* pszPath, const char* pszFieldName, bool bDefault)
{
	return gfuiMenuGetBoolean(GfParmGetStr(hparm, pszPath, pszFieldName, 0), bDefault);
}

static GfuiColor
getControlColor(void* hparm, const char* pszPath, const char* pszField)
{
	return GfuiColor::build(GfParmGetStr(hparm, pszPath, pszField, 0));
}

static int 
createStaticImage(void* hscr, void* hparm, const char* pszName)
{
	const char* pszImage = GfParmGetStr(hparm, pszName, GFMNU_ATTR_IMAGE, "");

	const int x = (int)GfParmGetNum(hparm, pszName, GFMNU_ATTR_X, NULL, 0.0);
	const int y = (int)GfParmGetNum(hparm, pszName, GFMNU_ATTR_Y, NULL, 0.0);
	const int w = (int)GfParmGetNum(hparm, pszName, GFMNU_ATTR_WIDTH, NULL, 100.0);
	const int h = (int)GfParmGetNum(hparm, pszName, GFMNU_ATTR_HEIGHT, NULL, 100.0);

	const bool canDeform = getControlBoolean(hparm, pszName, GFMNU_ATTR_CAN_DEFORM, true);

	int id = GfuiStaticImageCreate(hscr, x, y, w, h, pszImage, canDeform);

	char pszImageFieldName[32];
	for (int i = 1; i < GFUI_MAXSTATICIMAGES;i++)
	{
		sprintf(pszImageFieldName, GFMNU_ATTR_IMAGE" %d", i);
		const char* pszFileName = GfParmGetStr(hparm, pszName, pszImageFieldName, 0);
		if (pszFileName)
			GfuiStaticImageSet(hscr, id, pszFileName, i);
		else
			break; // Assumes an indexed image list, with no hole inside.
	}

	return id;
}
static int 
createMusic(void* hscr, void* hparm)
{
	const char* pszMusic = GfParmGetStr(hparm, GFMNU_SECT_MUSIC, GFMNU_ATTR_MUSIC_FILE, 0);
	GfuiScreenAddMusic(hscr, pszMusic);
	return 1;
}

static int 
createBackgroundImage(void* hscr, void* hparm, const char* pszName)
{
	const char* pszImage = GfParmGetStr(hparm, pszName, GFMNU_ATTR_IMAGE, "");
	GfuiScreenAddBgImg(hscr, pszImage);
	return 1;
}

int
GfuiMenuCreateStaticImageControl(void* hscr, void* hparm, const char* pszName)
{
	std::string strControlPath(GFMNU_SECT_DYNAMIC_CONTROLS"/");
	strControlPath += pszName;
        
	return createStaticImage(hscr, hparm, strControlPath.c_str());
}

static int 
createLabel(void* hscr, void* hparm, const char* pszPath,
			bool bFromTemplate = false,
			const char* text = GFUI_TPL_TEXT, int x = GFUI_TPL_X, int y = GFUI_TPL_Y,
			int font = GFUI_TPL_FONTID, int width = GFUI_TPL_WIDTH, int hAlign = GFUI_TPL_ALIGN,
			int maxlen = GFUI_TPL_MAXLEN, 
			const float* aFgColor = GFUI_TPL_COLOR,
			const float* aFgFocusColor = GFUI_TPL_FOCUSCOLOR)
{
	if (strcmp(GfParmGetStr(hparm, pszPath, GFMNU_ATTR_TYPE, ""), GFMNU_TYPE_LABEL))
	{
		GfLogError("Failed to create label control '%s' : section not found or not a '%s'\n",
				   pszPath, GFMNU_TYPE_LABEL);
		return -1;
	}
        
	const char* pszText =
		bFromTemplate && text != GFUI_TPL_TEXT
		? text : GfParmGetStr(hparm, pszPath, GFMNU_ATTR_TEXT, "");
	const int nX = 
		bFromTemplate && x != GFUI_TPL_X
		? x : (int)GfParmGetNum(hparm, pszPath, GFMNU_ATTR_X, NULL, 0);
	const int nY = 
		bFromTemplate && y != GFUI_TPL_Y
		? y : (int)GfParmGetNum(hparm, pszPath, GFMNU_ATTR_Y, NULL, 0);
	const int nWidth =
		bFromTemplate && width != GFUI_TPL_WIDTH
		? width : (int)GfParmGetNum(hparm, pszPath, GFMNU_ATTR_WIDTH, NULL, 0);
	const int nFontId = 
		bFromTemplate && font != GFUI_TPL_FONTID
		? font : gfuiMenuGetFontId(GfParmGetStr(hparm, pszPath, GFMNU_ATTR_FONT, ""));
	const char* pszAlignH = GfParmGetStr(hparm, pszPath, GFMNU_ATTR_H_ALIGN, "");
	//const char* pszAlignV = GfParmGetStr(hparm, pszPath, GFMNU_ATTR_V_ALIGN, "");
	const int nAlign = 
		bFromTemplate && hAlign != GFUI_TPL_ALIGN
		? hAlign : gfuiMenuGetAlignment(pszAlignH); //, pszAlignV);
	int nMaxLen = 
		bFromTemplate && maxlen != GFUI_TPL_MAXLEN
		? maxlen : (int)GfParmGetNum(hparm, pszPath, GFMNU_ATTR_MAX_LEN, NULL, 0);
	
	GfuiColor color;
	const float* aColor = 0;
	if (bFromTemplate && aFgColor != GFUI_TPL_COLOR)
		aColor = aFgColor;
	else
	{
		color = getControlColor(hparm, pszPath, GFMNU_ATTR_COLOR);
		if (color.alpha)
			aColor = color.toFloatRGBA();
	}

	GfuiColor focusColor;
	const float* aFocusColor = 0;
	if (bFromTemplate && aFgFocusColor != GFUI_TPL_FOCUSCOLOR)
		aFocusColor = aFgFocusColor;
	else
	{
		focusColor = getControlColor(hparm, pszPath, GFMNU_ATTR_COLOR_FOCUSED);
		if (focusColor.alpha)
			aFocusColor = focusColor.toFloatRGBA();
	}

	void* userDataOnFocus = 0;
	tfuiCallback onFocus = 0;
	tfuiCallback onFocusLost = 0;
	const char* pszTip = GfParmGetStr(hparm, pszPath, GFMNU_ATTR_TIP, 0);
	if (pszTip && strlen(pszTip) > 0)
	{
		tMenuCallbackInfo * cbinfo = (tMenuCallbackInfo*)calloc(1, sizeof(tMenuCallbackInfo));
		cbinfo->screen = hscr;
		cbinfo->labelId = GfuiTipCreate(hscr, pszTip, strlen(pszTip));
		GfuiVisibilitySet(hscr, cbinfo->labelId, GFUI_INVISIBLE);

		userDataOnFocus = (void*)cbinfo;
		onFocus = onFocusShowTip;
		onFocusLost = onFocusLostHideTip;
	}

	int labelId = GfuiLabelCreate(hscr, pszText, nFontId, nX, nY, nWidth, nAlign, nMaxLen,
								  aColor, aFocusColor, userDataOnFocus, onFocus, onFocusLost);

    return labelId;
}


int 
GfuiMenuCreateLabelControl(void* hscr, void* hparm, const char* pszName,
						   bool bFromTemplate,
						   const char* text, int x, int y, int font,
						   int width, int hAlign, int maxlen, 
						   const float* fgColor, const float* fgFocusColor)
{
	std::string strControlPath =
		bFromTemplate ? GFMNU_SECT_TEMPLATE_CONTROLS"/" : GFMNU_SECT_DYNAMIC_CONTROLS"/";
	strControlPath += pszName;

	return createLabel(hscr, hparm, strControlPath.c_str(), bFromTemplate,
					   text, x, y, font, width, hAlign, maxlen, fgColor, fgFocusColor);
}

static int 
createTextButton(void* hscr, void* hparm, const char* pszPath,
				 void* userDataOnPush, tfuiCallback onPush,
				 void* userDataOnFocus, tfuiCallback onFocus, tfuiCallback onFocusLost,
				 bool bFromTemplate = false,
				 const char* text = GFUI_TPL_TEXT,
				 const char* tip = GFUI_TPL_TIP,
				 int x = GFUI_TPL_X, int y = GFUI_TPL_Y,
				 int width = GFUI_TPL_WIDTH,
				 int font = GFUI_TPL_FONTID, int textHAlign = GFUI_TPL_ALIGN,
				 const float* aFgColor = GFUI_TPL_COLOR,
				 const float* aFgFocusColor = GFUI_TPL_FOCUSCOLOR,
				 const float* aFgPushedColor = GFUI_TPL_PUSHEDCOLOR)
{
	if (strcmp(GfParmGetStr(hparm, pszPath, GFMNU_ATTR_TYPE, ""), GFMNU_TYPE_TEXT_BUTTON))
	{
		GfLogError("Failed to create text button control '%s' : section not found or not a '%s'\n",
				   pszPath, GFMNU_TYPE_TEXT_BUTTON);
		return -1;
	}
        
	const char* pszText =
		bFromTemplate && text != GFUI_TPL_TEXT
		? text : GfParmGetStr(hparm, pszPath, GFMNU_ATTR_TEXT, "");
	const char* pszTip =
		bFromTemplate && tip != GFUI_TPL_TIP
		? tip : GfParmGetStr(hparm, pszPath, GFMNU_ATTR_TIP, "");
	const int nX = 
		bFromTemplate && x != GFUI_TPL_X
		? x : (int)GfParmGetNum(hparm, pszPath, GFMNU_ATTR_X, NULL, 0);
	const int nY = 
		bFromTemplate && y != GFUI_TPL_Y
		? y : (int)GfParmGetNum(hparm, pszPath, GFMNU_ATTR_Y, NULL, 0);
	int nWidth = 
		bFromTemplate && width != GFUI_TPL_WIDTH
		? width : (int)GfParmGetNum(hparm, pszPath, GFMNU_ATTR_WIDTH, NULL, 0);
	if (nWidth <= 0)
		nWidth = GFUI_BTNSZ; // TODO: Get default from screen.xml
	const int nFontId = 
		bFromTemplate && font != GFUI_TPL_FONTID
		? font : gfuiMenuGetFontId(GfParmGetStr(hparm, pszPath, GFMNU_ATTR_FONT, ""));
	const char* pszAlignH = GfParmGetStr(hparm, pszPath, GFMNU_ATTR_H_ALIGN, "");
	//const char* pszAlignV = GfParmGetStr(hparm, pszPath, GFMNU_ATTR_V_ALIGN, "");
	const int nAlign = 
		bFromTemplate && textHAlign != GFUI_TPL_ALIGN
		? textHAlign : gfuiMenuGetAlignment(pszAlignH); //, pszAlignV);
	
	GfuiColor color;
	const float* aColor = 0;
	if (bFromTemplate && aFgColor != GFUI_TPL_COLOR)
		aColor = aFgColor;
	else
	{
		color = getControlColor(hparm, pszPath, GFMNU_ATTR_COLOR);
		if (color.alpha)
			aColor = color.toFloatRGBA();
	}

	GfuiColor focusColor;
	const float* aFocusColor = 0;
	if (bFromTemplate && aFgFocusColor != GFUI_TPL_FOCUSCOLOR)
		aFocusColor = aFgFocusColor;
	else
	{
		focusColor = getControlColor(hparm, pszPath, GFMNU_ATTR_COLOR_FOCUSED);
		if (focusColor.alpha)
			aFocusColor = focusColor.toFloatRGBA();
	}

	GfuiColor pushedColor;
	const float* aPushedColor = 0;
	if (bFromTemplate && aFgPushedColor != GFUI_TPL_PUSHEDCOLOR)
		aPushedColor = aFgPushedColor;
	else
	{
		pushedColor = getControlColor(hparm, pszPath, GFMNU_ATTR_COLOR_PUSHED);
		if (pushedColor.alpha)
			aPushedColor = pushedColor.toFloatRGBA();
	}

	if (pszTip && strlen(pszTip) > 0)
	{
		tMenuCallbackInfo * cbinfo = (tMenuCallbackInfo*)calloc(1, sizeof(tMenuCallbackInfo));
		cbinfo->screen = hscr;
		cbinfo->labelId = GfuiTipCreate(hscr, pszTip, strlen(pszTip));
		GfuiVisibilitySet(hscr, cbinfo->labelId, GFUI_INVISIBLE);

		// TODO: In this case, we crudely overwrite onFocus/onFocusLost !
		userDataOnFocus = (void*)cbinfo;
		onFocus = onFocusShowTip;
		onFocusLost = onFocusLostHideTip;
	}

	const bool bShowbox = getControlBoolean(hparm, pszPath, GFMNU_ATTR_BOX_SHOW, true);

	const char* pszDisabledImage = GfParmGetStr(hparm, pszPath, GFMNU_ATTR_IMAGE_DISABLED, 0);
	const char* pszEnabledImage = GfParmGetStr(hparm, pszPath, GFMNU_ATTR_IMAGE_ENABLED, 0);
	const char* pszFocusedImage = GfParmGetStr(hparm, pszPath, GFMNU_ATTR_IMAGE_FOCUSED, 0);
	const char* pszPushedImage = GfParmGetStr(hparm, pszPath, GFMNU_ATTR_IMAGE_PUSHED, 0);

	const int imgX = (int)GfParmGetNum(hparm, pszPath, GFMNU_ATTR_IMAGE_X, NULL, 0.0);
	const int imgY = (int)GfParmGetNum(hparm, pszPath, GFMNU_ATTR_IMAGE_Y, NULL, 0.0);
	const int imgWidth = (int)GfParmGetNum(hparm, pszPath, GFMNU_ATTR_IMAGE_WIDTH, NULL, 20.0);
	const int imgHeight = (int)GfParmGetNum(hparm, pszPath, GFMNU_ATTR_IMAGE_HEIGHT, NULL, 20.0);

	int butId = GfuiButtonCreate(hscr, pszText, nFontId,
								 nX, nY, nWidth, nAlign, GFUI_MOUSE_UP,
								 userDataOnPush, onPush,
								 userDataOnFocus, onFocus, onFocusLost);

	GfuiButtonShowBox(hscr, butId, bShowbox);

	if (pszDisabledImage || pszEnabledImage || pszFocusedImage || pszPushedImage)
		GfuiButtonSetImage(hscr, butId, imgX, imgY, imgWidth, imgHeight,
						   pszDisabledImage, pszEnabledImage,
						   pszFocusedImage, pszPushedImage);

	GfuiButtonSetColors(hscr, butId,
						GfuiColor::build(aColor), GfuiColor::build(aFocusColor),
						GfuiColor::build(aPushedColor));
        
	return butId;
}

static int 
createImageButton(void* hscr, void* hparm, const char* pszPath,
				  void* userDataOnPush, tfuiCallback onPush,
				  void* userDataOnFocus, tfuiCallback onFocus, tfuiCallback onFocusLost,
				  bool bFromTemplate = false,
				  const char* tip = GFUI_TPL_TIP,
				  int x = GFUI_TPL_X, int y = GFUI_TPL_Y,
				  int width = GFUI_TPL_WIDTH, int height = GFUI_TPL_HEIGHT)
{
	if (strcmp(GfParmGetStr(hparm, pszPath, GFMNU_ATTR_TYPE, ""), GFMNU_TYPE_IMAGE_BUTTON))
	{
		GfLogError("Failed to create image button control '%s' : section not found or not an '%s'\n",
				   pszPath, GFMNU_TYPE_IMAGE_BUTTON);
		return -1;
	}
        
	const char* pszTip =
		bFromTemplate && tip != GFUI_TPL_TIP
		? tip : GfParmGetStr(hparm, pszPath, GFMNU_ATTR_TIP, "");
	const int nX = 
		bFromTemplate && x != GFUI_TPL_X
		? x : (int)GfParmGetNum(hparm, pszPath, GFMNU_ATTR_X, NULL, 0);
	const int nY = 
		bFromTemplate && y != GFUI_TPL_Y
		? y : (int)GfParmGetNum(hparm, pszPath, GFMNU_ATTR_Y, NULL, 0);
	int nWidth = 
		bFromTemplate && width != GFUI_TPL_WIDTH
		? width : (int)GfParmGetNum(hparm, pszPath, GFMNU_ATTR_WIDTH, NULL, 0);
	int nHeight = 
		bFromTemplate && height != GFUI_TPL_HEIGHT
		? height : (int)GfParmGetNum(hparm, pszPath, GFMNU_ATTR_HEIGHT, NULL, 0);

	if (strlen(pszTip) > 0)
	{
		tMenuCallbackInfo * cbinfo = (tMenuCallbackInfo*)calloc(1, sizeof(tMenuCallbackInfo));
		cbinfo->screen = hscr;
		cbinfo->labelId = GfuiTipCreate(hscr, pszTip, strlen(pszTip));
		GfuiVisibilitySet(hscr, cbinfo->labelId, GFUI_INVISIBLE);

		// TODO: In this case, we crudely overwrite onFocus/onFocusLost !
		userDataOnFocus = (void*)cbinfo;
		onFocus = onFocusShowTip;
		onFocusLost = onFocusLostHideTip;
	}

	const char* pszDisabledImage = GfParmGetStr(hparm, pszPath, GFMNU_ATTR_IMAGE_DISABLED, "");
	const char* pszEnabledImage = GfParmGetStr(hparm, pszPath, GFMNU_ATTR_IMAGE_ENABLED, "");
	const char* pszFocusedImage = GfParmGetStr(hparm, pszPath, GFMNU_ATTR_IMAGE_FOCUSED, "");
	const char* pszPushedImage = GfParmGetStr(hparm, pszPath, GFMNU_ATTR_IMAGE_PUSHED, "");

	int butId =
		GfuiGrButtonCreate(hscr,
						   pszDisabledImage, pszEnabledImage, pszFocusedImage, pszPushedImage,
						   nX, nY, nWidth, nHeight, GFUI_MIRROR_NONE, true, GFUI_MOUSE_UP,
						   userDataOnPush, onPush,
						   userDataOnFocus, onFocus, onFocusLost);

	return butId;
}

int 
GfuiMenuCreateTextButtonControl(void* hscr, void* hparm, const char* pszName,
								void* userDataOnPush, tfuiCallback onPush,
								void* userDataOnFocus, tfuiCallback onFocus, tfuiCallback onFocusLost,
								bool bFromTemplate,
								const char* text, const char* tip,
								int x, int y, int width, int font, int textHAlign,
								const float* fgColor, const float* fgFocusColor, const float* fgPushedColor)
{
	std::string strControlPath =
		bFromTemplate ? GFMNU_SECT_TEMPLATE_CONTROLS"/" : GFMNU_SECT_DYNAMIC_CONTROLS"/";
	strControlPath += pszName;

	return createTextButton(hscr, hparm, strControlPath.c_str(),
							userDataOnPush, onPush,
							userDataOnFocus, onFocus, onFocusLost,
							bFromTemplate,
							text, tip, x, y, width, font, textHAlign,
							fgColor, fgFocusColor, fgPushedColor);
}

int 
GfuiMenuCreateImageButtonControl(void* hscr, void* hparm, const char* pszName,
								 void* userDataOnPush, tfuiCallback onPush,
								 void* userDataOnFocus, tfuiCallback onFocus, tfuiCallback onFocusLost,
								 bool bFromTemplate,
								 const char* tip, int x, int y, int width, int height)
{
	std::string strControlPath =
		bFromTemplate ? GFMNU_SECT_TEMPLATE_CONTROLS"/" : GFMNU_SECT_DYNAMIC_CONTROLS"/";
	strControlPath += pszName;

	return createImageButton(hscr, hparm, strControlPath.c_str(),
							 userDataOnPush, onPush,
							 userDataOnFocus, onFocus, onFocusLost,
							 bFromTemplate,
							 tip, x, y, width, height);
}

int 
GfuiMenuCreateButtonControl(void* hscr, void* hparm, const char* pszName,
							void* userDataOnPush, tfuiCallback onPush,
							void* userDataOnFocus, tfuiCallback onFocus, tfuiCallback onFocusLost)
{
	std::string strControlPath = GFMNU_SECT_DYNAMIC_CONTROLS"/";
	strControlPath += pszName;

	const char* pszType = GfParmGetStr(hparm, strControlPath.c_str(), GFMNU_ATTR_TYPE, "");
	if (!strcmp(pszType, GFMNU_TYPE_TEXT_BUTTON))
		return createTextButton(hscr, hparm, strControlPath.c_str(),
								userDataOnPush, onPush,
								userDataOnFocus, onFocus, onFocusLost);
	else if(!strcmp(pszType, GFMNU_TYPE_IMAGE_BUTTON))
		return createImageButton(hscr, hparm, strControlPath.c_str(),
								 userDataOnPush, onPush,
								 userDataOnFocus, onFocus, onFocusLost);
	else
		GfLogError("Failed to create button control '%s' of unknown type '%s'\n",
				   pszName, pszType);

	return -1;
}

int 
GfuiMenuCreateEditControl(void* hscr, void* hparm, const char* pszName,
						  void* userDataOnFocus, tfuiCallback onFocus, tfuiCallback onFocusLost)
{
	std::string strControlPath(GFMNU_SECT_DYNAMIC_CONTROLS"/");
	strControlPath += pszName;

	const char* pszType = GfParmGetStr(hparm, strControlPath.c_str(), GFMNU_ATTR_TYPE, "");
	if (strcmp(pszType, GFMNU_TYPE_EDIT_BOX))
	{
		GfLogError("Failed to create control '%s' : section not found or not an '%s' \n",
				   pszName, GFMNU_TYPE_EDIT_BOX);
		return -1;
	}

	// TODO : Add real support for tips (the onFocus/onFocusLost system is already used
	//        for user input management)
	//         const char* pszTip = GfParmGetStr(hparm, pszName, GFMNU_ATTR_TIP, "");
	//         if (strlen(pszTip) > 0)
	//         {
	//                 tMenuCallbackInfo * cbinfo = (tMenuCallbackInfo*)calloc(1, sizeof(tMenuCallbackInfo));
	//                 cbinfo->screen = hscr;
	//                 cbinfo->labelId = GfuiTipCreate(hscr, pszTip, strlen(pszTip));
	//                 GfuiVisibilitySet(hscr, cbinfo->labelId, GFUI_INVISIBLE);
	//
	//                 // TODO: In this case, we simply ignore onFocus/onFocusLost !
	//                 userDataOnFocus = (void*)cbinfo;
	//                 onFocus = onFocusShowTip;
	//                 onFocusLost = onFocusLostHideTip;
	//         }

	const char* pszText = GfParmGetStr(hparm, strControlPath.c_str(), GFMNU_ATTR_TEXT, "");
	const int x = (int)GfParmGetNum(hparm, strControlPath.c_str(), GFMNU_ATTR_X, NULL, 0.0);
	const int y = (int)GfParmGetNum(hparm, strControlPath.c_str(), GFMNU_ATTR_Y, NULL, 0.0);
	const char* pszFontName = GfParmGetStr(hparm, strControlPath.c_str(), GFMNU_ATTR_FONT, "");
	const int font = gfuiMenuGetFontId(pszFontName);
	const int width = (int)GfParmGetNum(hparm, strControlPath.c_str(), GFMNU_ATTR_WIDTH, NULL, 0.0);
	const int maxlen = (int)GfParmGetNum(hparm, strControlPath.c_str(), GFMNU_ATTR_MAX_LEN, NULL, 0.0);
	const char* pszAlignH = GfParmGetStr(hparm, strControlPath.c_str(), GFMNU_ATTR_H_ALIGN, "");
	const int align = gfuiMenuGetAlignment(pszAlignH);

	const GfuiColor c = getControlColor(hparm, strControlPath.c_str(), GFMNU_ATTR_COLOR);
	const GfuiColor fc = getControlColor(hparm, strControlPath.c_str(), GFMNU_ATTR_COLOR_FOCUSED);
	const GfuiColor dc = getControlColor(hparm, strControlPath.c_str(), GFMNU_ATTR_COLOR_DISABLED);
	const GfuiColor bc = getControlColor(hparm, strControlPath.c_str(), GFMNU_ATTR_BG_COLOR);
	const GfuiColor bfc = getControlColor(hparm, strControlPath.c_str(), GFMNU_ATTR_BG_COLOR_FOCUSED);
	const GfuiColor bdc = getControlColor(hparm, strControlPath.c_str(), GFMNU_ATTR_BG_COLOR_DISABLED);

	int id = GfuiEditboxCreate(hscr, pszText, font, x, y, width, maxlen, align,
							   userDataOnFocus, onFocus, onFocusLost);

	GfuiEditboxSetColors(hscr, id, c, fc, dc);
	GfuiEditboxSetBGColors(hscr, id, bc, bfc, bdc);

	return id;
}

int 
GfuiMenuCreateComboboxControl(void* hscr, void* hparm, const char* pszName,
							  void* userData, tfuiComboboxCallback onChange)
{
	std::string strControlPath(GFMNU_SECT_DYNAMIC_CONTROLS"/");
	strControlPath += pszName;

	const std::string strType = GfParmGetStr(hparm, strControlPath.c_str(), GFMNU_ATTR_TYPE, "");
	if (strType != GFMNU_TYPE_COMBO_BOX)
	{
		GfLogError("Failed to create control '%s' : section not found or not an '%s' \n",
				   pszName, GFMNU_TYPE_COMBO_BOX);
		return -1;
	}

	int id = -1;
	
	const int x = (int)GfParmGetNum(hparm,strControlPath.c_str(), GFMNU_ATTR_X, NULL, 0.0);
	const int y = (int)GfParmGetNum(hparm,strControlPath.c_str(), GFMNU_ATTR_Y, NULL, 0.0);

	std::string strFontName = GfParmGetStr(hparm, strControlPath.c_str(), GFMNU_ATTR_FONT, "");
	const int font = gfuiMenuGetFontId(strFontName.c_str());

	int width = (int)GfParmGetNum(hparm,strControlPath.c_str(), GFMNU_ATTR_WIDTH, NULL, 0.0);
	if (width == 0)
	    width = 200;

	const int nArrowsWidth = (int)GfParmGetNum(hparm, strControlPath.c_str(), GFMNU_ATTR_ARROWS_WIDTH, NULL, 0);
	const int nArrowsHeight = (int)GfParmGetNum(hparm, strControlPath.c_str(), GFMNU_ATTR_ARROWS_HEIGHT, NULL, 0);

    const char* pszText = GfParmGetStr(hparm, strControlPath.c_str(), GFMNU_ATTR_TEXT, "");

	const int maxlen = (int)GfParmGetNum(hparm, strControlPath.c_str(), GFMNU_ATTR_MAX_LEN, 0, 0);

	const char* pszTip = GfParmGetStr(hparm, strControlPath.c_str(), GFMNU_ATTR_TIP, 0);
	
	void* userDataOnFocus = 0;
	tfuiCallback onFocus = 0;
	tfuiCallback onFocusLost = 0;
	if (pszTip && strlen(pszTip) > 0)
	{
		tMenuCallbackInfo * cbinfo = (tMenuCallbackInfo*)calloc(1, sizeof(tMenuCallbackInfo));
		cbinfo->screen = hscr;
		cbinfo->labelId = GfuiTipCreate(hscr, pszTip, strlen(pszTip));
		GfuiVisibilitySet(hscr, cbinfo->labelId, GFUI_INVISIBLE);
		
		userDataOnFocus = (void*)cbinfo;
		onFocus = onFocusShowTip;
		onFocusLost = onFocusLostHideTip;
	}

	const float* aColor = 0;
	const GfuiColor color = getControlColor(hparm, strControlPath.c_str(), GFMNU_ATTR_COLOR);
	if (color.alpha)
		aColor = color.toFloatRGBA();
	
	const float* aFocusColor = 0;
	const GfuiColor focusColor =
		getControlColor(hparm, strControlPath.c_str(), GFMNU_ATTR_COLOR_FOCUSED);
	if (focusColor.alpha)
		aFocusColor = focusColor.toFloatRGBA();
	
	id = GfuiComboboxCreate(hscr, font, x, y, width, nArrowsWidth, nArrowsHeight,
							pszText, maxlen, aColor, aFocusColor,
							userData, onChange, userDataOnFocus, onFocus, onFocusLost);

	return id;
}

int 
GfuiMenuCreateScrollListControl(void* hscr, void* hparm, const char* pszName,void* userData, tfuiCallback onSelect)
{
	std::string strControlPath(GFMNU_SECT_DYNAMIC_CONTROLS"/");
	strControlPath += pszName;

	const char* pszType = GfParmGetStr(hparm, strControlPath.c_str(), GFMNU_ATTR_TYPE, "");
	if (strcmp(pszType, GFMNU_TYPE_SCROLL_LIST))
	{
		GfLogError("Failed to create control '%s' : section not found or not a '%s' \n",
				   pszName, GFMNU_TYPE_SCROLL_LIST);
		return -1;
	}

	const int x = (int)GfParmGetNum(hparm, strControlPath.c_str(), GFMNU_ATTR_X, NULL, 0.0);
	const int y = (int)GfParmGetNum(hparm, strControlPath.c_str(), GFMNU_ATTR_Y, NULL, 0.0);
	const int w = (int)GfParmGetNum(hparm, strControlPath.c_str(), GFMNU_ATTR_WIDTH, NULL, 100.0);
	const int h = (int)GfParmGetNum(hparm, strControlPath.c_str(), GFMNU_ATTR_HEIGHT, NULL, 100.0);
        
	const char* pszFontName = GfParmGetStr(hparm, strControlPath.c_str(), GFMNU_ATTR_FONT, "");
	const int font = gfuiMenuGetFontId(pszFontName);

	const char* pszScrollBarPos =
		GfParmGetStr(hparm, strControlPath.c_str(), GFMNU_ATTR_SCROLLBAR_POS, "none");
	const int scrollbarPos = gfuiMenuGetScrollBarPosition(pszScrollBarPos);
	const int scrollbarWidth =
		(int)GfParmGetNum(hparm, strControlPath.c_str(), GFMNU_ATTR_SCROLLBAR_WIDTH, NULL, 20.0);
	const int scrollBarButHeight =
		(int)GfParmGetNum(hparm, strControlPath.c_str(), GFMNU_ATTR_SCROLLBAR_BUTTONS_HEIGHT, NULL, 20.0);

	const GfuiColor c = getControlColor(hparm, pszName, GFMNU_ATTR_COLOR);
	const GfuiColor sc = getControlColor(hparm, pszName, GFMNU_ATTR_COLOR_SELECTED);
        
	int id = GfuiScrollListCreate(hscr, font, x, y, w, h,
								  scrollbarPos, scrollbarWidth, scrollBarButHeight,
								  userData, onSelect);

	GfuiScrollListSetColors(hscr, id, c, sc);

	return id;
}

int 
GfuiMenuCreateCheckboxControl(void* hscr, void* hparm, const char* pszName,void* userData,tfuiCheckboxCallback onChange)
{
	std::string strControlPath(GFMNU_SECT_DYNAMIC_CONTROLS"/");
	strControlPath += pszName;

	const std::string strType = GfParmGetStr(hparm, strControlPath.c_str(), GFMNU_ATTR_TYPE, "");
	if (strType != GFMNU_TYPE_CHECK_BOX)
	{
		GfLogError("Failed to create control '%s' : section not found or not an '%s' \n",
				   pszName, GFMNU_TYPE_CHECK_BOX);
		return -1;
	}

	int id = -1;
	
	const int x = (int)GfParmGetNum(hparm, strControlPath.c_str(), GFMNU_ATTR_X, NULL, 0.0);
	const int y = (int)GfParmGetNum(hparm, strControlPath.c_str(), GFMNU_ATTR_Y, NULL, 0.0);

	std::string strFontName = GfParmGetStr(hparm, strControlPath.c_str(), GFMNU_ATTR_FONT, "");
	const int font = gfuiMenuGetFontId(strFontName.c_str());

    const char* pszText = GfParmGetStr(hparm, strControlPath.c_str(), GFMNU_ATTR_TEXT, "");

	int imagewidth =
		(int)GfParmGetNum(hparm, strControlPath.c_str(), GFMNU_ATTR_IMAGE_WIDTH, NULL, 0.0);
	if (imagewidth <= 0)
	    imagewidth = 30; // TODO: Get default from screen.xml

	int imageheight =
		(int)GfParmGetNum(hparm, strControlPath.c_str(), GFMNU_ATTR_IMAGE_HEIGHT, NULL, 0.0);
	if (imageheight <= 0)
	    imageheight = 30; // TODO: Get default from screen.xml

    const bool bChecked =
		getControlBoolean(hparm, strControlPath.c_str(), GFMNU_ATTR_CHECKED, false);

	const char* pszTip = GfParmGetStr(hparm, strControlPath.c_str(), GFMNU_ATTR_TIP, "");
	
	void* userDataOnFocus = 0;
	tfuiCallback onFocus = 0;
	tfuiCallback onFocusLost = 0;
	if (strlen(pszTip) > 0)
	{
		tMenuCallbackInfo * cbinfo = (tMenuCallbackInfo*)calloc(1, sizeof(tMenuCallbackInfo));
		cbinfo->screen = hscr;
		cbinfo->labelId = GfuiTipCreate(hscr, pszTip, strlen(pszTip));
		GfuiVisibilitySet(hscr, cbinfo->labelId, GFUI_INVISIBLE);
		
		userDataOnFocus = (void*)cbinfo;
		onFocus = onFocusShowTip;
		onFocusLost = onFocusLostHideTip;
	}


	id = GfuiCheckboxCreate(hscr, font, x, y, imagewidth, imageheight,
							pszText, bChecked, userData, onChange,
							userDataOnFocus, onFocus, onFocusLost);

	GfuiColor c = getControlColor(hparm, pszName, GFMNU_ATTR_COLOR);
	if (c.alpha)
		GfuiCheckboxSetTextColor(hscr, id, c);

	return id;
}


int 
GfuiMenuCreateProgressbarControl(void* hscr, void* hparm, const char* pszName)
{
	std::string strControlPath(GFMNU_SECT_DYNAMIC_CONTROLS"/");
	strControlPath += pszName;
	
	const std::string strType = GfParmGetStr(hparm, strControlPath.c_str(), GFMNU_ATTR_TYPE, "");
	if (strType != GFMNU_TYPE_PROGRESS_BAR)
	{
		GfLogError("Failed to create control '%s' : section not found or not an '%s' \n",
				   pszName, GFMNU_TYPE_PROGRESS_BAR);
		return -1;
	}
	
	const char* pszImage =
		GfParmGetStr(hparm, strControlPath.c_str(), GFMNU_ATTR_IMAGE, "data/img/progressbar.png");
	const char* pszBgImage =
		GfParmGetStr(hparm, strControlPath.c_str(), GFMNU_ATTR_BG_IMAGE, "data/img/progressbar-bg.png");
	
	const float* aOutlineColor = 0;
	const GfuiColor color = getControlColor(hparm, strControlPath.c_str(), GFMNU_ATTR_COLOR);
	if (color.alpha)
		aOutlineColor = color.toFloatRGBA();

	const int x = (int)GfParmGetNum(hparm, strControlPath.c_str(), GFMNU_ATTR_X, NULL, 0.0);
	const int y = (int)GfParmGetNum(hparm, strControlPath.c_str(), GFMNU_ATTR_Y, NULL, 0.0);
	const int w = (int)GfParmGetNum(hparm, strControlPath.c_str(), GFMNU_ATTR_WIDTH, NULL, 100.0);
	const int h = (int)GfParmGetNum(hparm, strControlPath.c_str(), GFMNU_ATTR_HEIGHT, NULL, 20.0);
	
	const float min = GfParmGetNum(hparm, strControlPath.c_str(), GFMNU_ATTR_MIN, NULL, 0.0);
	const float max = GfParmGetNum(hparm, strControlPath.c_str(), GFMNU_ATTR_MAX, NULL, 100.0);
	const float value = GfParmGetNum(hparm, strControlPath.c_str(), GFMNU_ATTR_VALUE, NULL, 50.0);
	
	const char* pszTip = GfParmGetStr(hparm, strControlPath.c_str(), GFMNU_ATTR_TIP, "");
	
	void* userDataOnFocus = 0;
	tfuiCallback onFocus = 0;
	tfuiCallback onFocusLost = 0;
	if (strlen(pszTip) > 0)
	{
		tMenuCallbackInfo * cbinfo = (tMenuCallbackInfo*)calloc(1, sizeof(tMenuCallbackInfo));
		cbinfo->screen = hscr;
		cbinfo->labelId = GfuiTipCreate(hscr, pszTip, strlen(pszTip));
		GfuiVisibilitySet(hscr, cbinfo->labelId, GFUI_INVISIBLE);
		
		userDataOnFocus = (void*)cbinfo;
		onFocus = onFocusShowTip;
		onFocusLost = onFocusLostHideTip;
	}

	int id = GfuiProgressbarCreate(hscr, x, y, w, h, pszBgImage, pszImage, aOutlineColor,
								   min, max, value, userDataOnFocus, onFocus, onFocusLost);
	
	return id;
}

bool 
GfuiMenuCreateStaticControls(void* hscr, void* hparm)
{
	if (!hparm)
	{
		GfLogError("Failed to create static controls (XML menu descriptor not yet loaded)\n");
		return false;
	}

    char buf[32];

    for (int i = 1; i <= GfParmGetEltNb(hparm, GFMNU_SECT_STATIC_CONTROLS); i++)
    {
		snprintf(buf, sizeof(buf), GFMNU_SECT_STATIC_CONTROLS"/%d", i);
		const char* pszType = GfParmGetStr(hparm, buf, GFMNU_ATTR_TYPE, "");
    
		if (!strcmp(pszType, GFMNU_TYPE_LABEL))
		{
			createLabel(hscr, hparm, buf);
		}
		else if (!strcmp(pszType, GFMNU_TYPE_STATIC_IMAGE))
		{
			createStaticImage(hscr, hparm, buf);
		}
		else if (!strcmp(pszType, GFMNU_TYPE_BACKGROUND_IMAGE))
		{
			createBackgroundImage(hscr, hparm, buf);
		}
		else
		{
			GfLogWarning("Failed to create static control '%s' of unknown type '%s'\n",
						 buf, pszType);
		}
    }
	 
	 // while not truly a static control (visually), each menu/screen can have
	 // background music. As 'GfuiMenuCreateStaticControls()' is called on load
	 // of each menu, this was deemed the least intrusive place to add the
	 // music filename to the screen's struct.
	 createMusic(hscr,hparm);
    return true;
}

void* 
GfuiMenuLoad(const char* pszMenuPath)
{
	std::string strPath("data/menu/");
	strPath += pszMenuPath;
        
	char buf[512];
	sprintf(buf, "%s%s", GfDataDir(), strPath.c_str());

	return GfParmReadFile(buf, GFPARM_RMODE_STD);
}

tdble
GfuiMenuGetNumProperty(void* hparm, const char* pszName, tdble nDefVal, const char* pszUnit)
{
	return GfParmGetNum(hparm, GFMNU_SECT_PROPERTIES, pszName, pszUnit, nDefVal);
}

const char*
GfuiMenuGetStrProperty(void* hparm, const char* pszName, const char* pszDefVal)
{
	return GfParmGetStr(hparm, GFMNU_SECT_PROPERTIES, pszName, pszDefVal);
}

//===================================================================================
// GfuiMenuScreen class implementation

struct gfuiMenuPrivateData
{
	void* menuHdle;
	void* prevMenuHdle;
	std::string strXMLDescFileName;
	void* xmlDescParmHdle;
	std::map<std::string, int> mapControlIds;
};


GfuiMenuScreen::GfuiMenuScreen(const char* pszXMLDescFile)
: m_priv(new gfuiMenuPrivateData)
{
	m_priv->menuHdle = 0;
	m_priv->prevMenuHdle = 0;
	m_priv->strXMLDescFileName = pszXMLDescFile;
	m_priv->xmlDescParmHdle = 0;
	m_priv->prevMenuHdle = 0;
}

GfuiMenuScreen::~GfuiMenuScreen()
{
	closeXMLDescriptor();
	if (m_priv->menuHdle)
		GfuiScreenRelease(m_priv->menuHdle);
	delete m_priv;
}

void GfuiMenuScreen::createMenu(float* bgColor, 
								void* userDataOnActivate, tfuiCallback onActivate, 
								void* userDataOnDeactivate, tfuiCallback onDeactivate, 
								int mouseAllowed)
{
	m_priv->menuHdle = GfuiScreenCreate(bgColor, userDataOnActivate, onActivate, 
										  userDataOnDeactivate, onDeactivate, mouseAllowed);
}

void GfuiMenuScreen::setMenuHandle(void* hdle)
{
	m_priv->menuHdle = hdle;
}

void* GfuiMenuScreen::getMenuHandle() const
{
	return m_priv->menuHdle;
}

void GfuiMenuScreen::setPreviousMenuHandle(void* hdle)
{
	m_priv->prevMenuHdle = hdle;
}

void* GfuiMenuScreen::getPreviousMenuHandle() const
{
	return m_priv->prevMenuHdle;
}

bool GfuiMenuScreen::openXMLDescriptor()
{
    m_priv->xmlDescParmHdle = GfuiMenuLoad(m_priv->strXMLDescFileName.c_str());
	return m_priv->xmlDescParmHdle != 0;
}

bool GfuiMenuScreen::closeXMLDescriptor()
{
	if (m_priv->xmlDescParmHdle)
	{
		GfParmReleaseHandle(m_priv->xmlDescParmHdle);
		m_priv->xmlDescParmHdle = 0;
		return true;
	}
	return false;
}

bool GfuiMenuScreen::createStaticControls()
{
	if (!m_priv->xmlDescParmHdle && !openXMLDescriptor())
		return false;
	
	return m_priv->menuHdle && m_priv->xmlDescParmHdle
		   && ::GfuiMenuCreateStaticControls(m_priv->menuHdle, m_priv->xmlDescParmHdle);
}

int GfuiMenuScreen::createButtonControl(const char* pszName,
										void* userDataOnPush, tfuiCallback onPush,
										void* userDataOnFocus, tfuiCallback onFocus, tfuiCallback onFocusLost)
{
	if (!m_priv->xmlDescParmHdle && !openXMLDescriptor())
		return -1;
	
	if (m_priv->mapControlIds.find(pszName) == m_priv->mapControlIds.end())
	{
		const int nCtrlId =
			::GfuiMenuCreateButtonControl(m_priv->menuHdle, m_priv->xmlDescParmHdle, pszName,
										  userDataOnPush, onPush,
										  userDataOnFocus, onFocus, onFocusLost);
		if (nCtrlId >= 0)
			m_priv->mapControlIds[pszName] = nCtrlId;

		return nCtrlId;
	}

	GfLogError("Failed to create button control '%s' : duplicate name\n", pszName);
	return -1;
}

int GfuiMenuScreen::createTextButtonControl(const char* pszName, void* userDataOnPush,
											tfuiCallback onPush,
											void* userDataOnFocus, tfuiCallback onFocus,
											tfuiCallback onFocusLost,
											bool bFromTemplate,
											const char* tip, const char* text,
											int x, int y, int width, int font, int textHAlign, 
											const float* fgColor, const float* fgFocusColor, const float* fgPushedColor)
{
	if (!m_priv->xmlDescParmHdle && !openXMLDescriptor())
		return -1;
	
	if (m_priv->mapControlIds.find(pszName) == m_priv->mapControlIds.end())
	{
		const int nCtrlId =
			::GfuiMenuCreateTextButtonControl(m_priv->menuHdle, m_priv->xmlDescParmHdle, pszName,
											  userDataOnPush, onPush,
											  userDataOnFocus, onFocus, onFocusLost,
											  bFromTemplate,
											  text, tip, x, y, width, font, textHAlign,
											  fgColor, fgFocusColor, fgPushedColor);
		if (nCtrlId >= 0)
			m_priv->mapControlIds[pszName] = nCtrlId;

		return nCtrlId;
	}

	GfLogError("Failed to create text button control '%s' : duplicate name\n", pszName);
	return -1;
}

int GfuiMenuScreen::createImageButtonControl(const char* pszName,
											 void* userDataOnPush, tfuiCallback onPush,
											 void* userDataOnFocus, tfuiCallback onFocus, tfuiCallback onFocusLost,
											 bool bFromTemplate,
											 const char* tip,
											 int x, int y, int width, int height)
{
	if (!m_priv->xmlDescParmHdle && !openXMLDescriptor())
		return -1;
	
	if (m_priv->mapControlIds.find(pszName) == m_priv->mapControlIds.end())
	{
		const int nCtrlId =
			::GfuiMenuCreateImageButtonControl(m_priv->menuHdle, m_priv->xmlDescParmHdle, pszName,
											   userDataOnPush, onPush,
											   userDataOnFocus, onFocus, onFocusLost,
											   bFromTemplate,
											   tip, x, y, width, height);
		if (nCtrlId >= 0)
			m_priv->mapControlIds[pszName] = nCtrlId;

		return nCtrlId;
	}

	GfLogError("Failed to create image button control '%s' : duplicate name\n", pszName);
	return -1;
}

int GfuiMenuScreen::createStaticImageControl(const char* pszName)
{
	if (!m_priv->xmlDescParmHdle && !openXMLDescriptor())
		return -1;

	if (m_priv->mapControlIds.find(pszName) == m_priv->mapControlIds.end())
	{
		const int nCtrlId =
			::GfuiMenuCreateStaticImageControl(m_priv->menuHdle, m_priv->xmlDescParmHdle, pszName);
		if (nCtrlId >= 0)
			m_priv->mapControlIds[pszName] = nCtrlId;

		return nCtrlId;
	}

	GfLogError("Failed to create static image control '%s' : duplicate name\n", pszName);
	return -1;
}

int GfuiMenuScreen::createLabelControl(const char* pszName, bool bFromTemplate,
									   const char* pszText, int nX, int nY,
									   int nFontId, int nWidth, int nHAlignId, int nMaxLen, 
									   const float* aFgColor, const float* aFgFocusColor)
{
	if (!m_priv->xmlDescParmHdle && !openXMLDescriptor())
		return -1;
	
	if (m_priv->mapControlIds.find(pszName) == m_priv->mapControlIds.end())
	{
		const int nCtrlId =
			::GfuiMenuCreateLabelControl(m_priv->menuHdle, m_priv->xmlDescParmHdle, pszName,
										 bFromTemplate, pszText, nX, nY,
										 nFontId, nWidth, nHAlignId, 
										 nMaxLen, aFgColor, aFgFocusColor);
		if (nCtrlId >= 0)
			m_priv->mapControlIds[pszName] = nCtrlId;

		return nCtrlId;
	}

	GfLogError("Failed to create label control '%s' : duplicate name\n", pszName);
	return -1;
}

int GfuiMenuScreen::createEditControl(const char* pszName,
									  void* userDataOnFocus, tfuiCallback onFocus,
									  tfuiCallback onFocusLost)
{
	if (!m_priv->xmlDescParmHdle && !openXMLDescriptor())
		return -1;
	
	if (m_priv->mapControlIds.find(pszName) == m_priv->mapControlIds.end())
	{
		const int nCtrlId =
			::GfuiMenuCreateEditControl(m_priv->menuHdle, m_priv->xmlDescParmHdle, pszName,
							  userDataOnFocus, onFocus, onFocusLost);
		if (nCtrlId >= 0)
			m_priv->mapControlIds[pszName] = nCtrlId;

		return nCtrlId;
	}

	GfLogError("Failed to create edit control '%s' : duplicate name\n", pszName);
	return -1;
}

int GfuiMenuScreen::createScrollListControl(const char* pszName,
											void* userData, tfuiCallback onSelect)
{
	if (!m_priv->xmlDescParmHdle && !openXMLDescriptor())
		return -1;
	
	if (m_priv->mapControlIds.find(pszName) == m_priv->mapControlIds.end())
	{
		const int nCtrlId =
			::GfuiMenuCreateScrollListControl(m_priv->menuHdle, m_priv->xmlDescParmHdle, pszName,
											  userData, onSelect);
		if (nCtrlId >= 0)
			m_priv->mapControlIds[pszName] = nCtrlId;

		return nCtrlId;
	}

	GfLogError("Failed to create scroll-list control '%s' : duplicate name\n", pszName);
	return -1;
}

int GfuiMenuScreen::createComboboxControl(const char* pszName,
										  void* userData, tfuiComboboxCallback onChange)
{
	if (!m_priv->xmlDescParmHdle && !openXMLDescriptor())
		return -1;
	
	if (m_priv->mapControlIds.find(pszName) == m_priv->mapControlIds.end())
	{
		const int nCtrlId =
			::GfuiMenuCreateComboboxControl(m_priv->menuHdle, m_priv->xmlDescParmHdle, pszName,
											userData, onChange);
		if (nCtrlId >= 0)
			m_priv->mapControlIds[pszName] = nCtrlId;

		return nCtrlId;
	}

	GfLogError("Failed to create combo-box control '%s' : duplicate name\n", pszName);
	return -1;
}

int GfuiMenuScreen::createCheckboxControl(const char* pszName,
										  void* userData, tfuiCheckboxCallback onChange)
{
	if (!m_priv->xmlDescParmHdle && !openXMLDescriptor())
		return -1;
	
	if (m_priv->mapControlIds.find(pszName) == m_priv->mapControlIds.end())
	{
		const int nCtrlId = 
			::GfuiMenuCreateCheckboxControl(m_priv->menuHdle, m_priv->xmlDescParmHdle, pszName,
											userData, onChange);
		if (nCtrlId >= 0)
			m_priv->mapControlIds[pszName] = nCtrlId;

		return nCtrlId;
	}

	GfLogError("Failed to create check-box control '%s' : duplicate name\n", pszName);
	return -1;
}

int GfuiMenuScreen::createProgressbarControl(const char* pszName)
{
	if (!m_priv->xmlDescParmHdle && !openXMLDescriptor())
		return -1;
	
	if (m_priv->mapControlIds.find(pszName) == m_priv->mapControlIds.end())
	{
		const int nCtrlId =
			::GfuiMenuCreateProgressbarControl(m_priv->menuHdle, m_priv->xmlDescParmHdle, pszName);
		if (nCtrlId >= 0)
			m_priv->mapControlIds[pszName] = nCtrlId;

		return nCtrlId;
	}

	GfLogError("Failed to create progress-bar control '%s' : duplicate name\n", pszName);
	return -1;
}

void GfuiMenuScreen::addDefaultShortcuts()
{
	if (!m_priv->xmlDescParmHdle && !openXMLDescriptor())
		return;
	
	GfuiMenuDefaultKeysAdd(m_priv->menuHdle);
}
		
void GfuiMenuScreen::addShortcut(int key, const char* descr, void* userData,
								 tfuiCallback onKeyPressed, tfuiCallback onKeyReleased)
{
	if (!m_priv->xmlDescParmHdle && !openXMLDescriptor())
		return;
	
	GfuiAddKey(m_priv->menuHdle, key, descr, userData, onKeyPressed, onKeyReleased);
}


int GfuiMenuScreen::getDynamicControlId(const char* pszName) const
{
	std::map<std::string, int>::const_iterator iterCtrlId = m_priv->mapControlIds.find(pszName);

	return iterCtrlId == m_priv->mapControlIds.end() ? -1 : (*iterCtrlId).second;
}

void GfuiMenuScreen::runMenu()
{
	if (!m_priv->xmlDescParmHdle && !openXMLDescriptor())
		return;
	
	GfuiScreenActivate(m_priv->menuHdle);
}
