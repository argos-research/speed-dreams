/***************************************************************************
               menu.h -- Interface file for Dynamic Menu Management                                   
                             -------------------                                         
    created              : Fri Aug 13 22:24:24 CEST 1999
    copyright            : (C) 1999 by Eric Espie                         
    email                : torcs@free.fr   
    version              : $Id: guimenu.h 5107 2013-01-22 23:34:44Z torcs-ng $                                  
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
 
 
#ifndef __MENU__H__
#define __MENU__H__


typedef struct MenuCallbackInfo
{
    void*	screen;
    int		labelId;
} tMenuCallbackInfo;


extern void gfuiInitMenu(void);

extern int gfuiMenuGetFontId(const char* pszFontName);
extern int gfuiMenuGetAlignment(const char* pszAlH); //, const char* pszAlV = 0);
extern int gfuiMenuGetScrollBarPosition(const char* pszSBPos);
extern bool gfuiMenuGetBoolean(const char* pszValue, bool bDefault = false);

// XML element names for menu descriptor XML files.
#define GFMNU_SECT_STATIC_CONTROLS	"static controls"
#define GFMNU_SECT_DYNAMIC_CONTROLS	"dynamic controls"
#define GFMNU_SECT_TEMPLATE_CONTROLS	"template controls"
#define GFMNU_SECT_PROPERTIES	"properties"
#define GFMNU_SECT_MUSIC	"music"

#define GFMNU_ATTR_TYPE "type"

#define GFMNU_TYPE_LABEL "label"
#define GFMNU_TYPE_STATIC_IMAGE "static image"
#define GFMNU_TYPE_BACKGROUND_IMAGE "background image"
#define GFMNU_TYPE_TEXT_BUTTON "text button"
#define GFMNU_TYPE_IMAGE_BUTTON "image button"
#define GFMNU_TYPE_EDIT_BOX "edit box"
#define GFMNU_TYPE_CHECK_BOX "check box"
#define GFMNU_TYPE_COMBO_BOX "combo box"
#define GFMNU_TYPE_SCROLL_LIST "scroll list"
#define GFMNU_TYPE_PROGRESS_BAR "progress bar"

#define GFMNU_ATTR_TEXT "text"
#define GFMNU_ATTR_TIP "tip"
#define GFMNU_ATTR_MAX_LEN "max len"

#define GFMNU_ATTR_X "x"
#define GFMNU_ATTR_Y "y"
#define GFMNU_ATTR_WIDTH "width"
#define GFMNU_ATTR_HEIGHT "height"

#define GFMNU_ATTR_H_ALIGN "h align"
//#define GFMNU_ATTR_V_ALIGN "v align"

#define GFMNU_ATTR_FONT "font"
				  
#define GFMNU_ATTR_COLOR "color"
#define GFMNU_ATTR_COLOR_DISABLED "disabled color"
#define GFMNU_ATTR_COLOR_FOCUSED "focused color"
#define GFMNU_ATTR_COLOR_PUSHED "pushed color"
#define GFMNU_ATTR_COLOR_SELECTED "selected color"

#define GFMNU_ATTR_BG_COLOR "bg color"
#define GFMNU_ATTR_BG_COLOR_DISABLED "disabled bg color"
#define GFMNU_ATTR_BG_COLOR_FOCUSED "focused bg color"

#define GFMNU_ATTR_IMAGE "image"
#define GFMNU_ATTR_IMAGE_ENABLED "enabled image"
#define GFMNU_ATTR_IMAGE_DISABLED "disabled image"
#define GFMNU_ATTR_IMAGE_FOCUSED "focused image"
#define GFMNU_ATTR_IMAGE_PUSHED "pushed image"

#define GFMNU_ATTR_BG_IMAGE "bg image"

#define GFMNU_ATTR_IMAGE_X "image x"
#define GFMNU_ATTR_IMAGE_Y "image y"
#define GFMNU_ATTR_IMAGE_WIDTH "image width"
#define GFMNU_ATTR_IMAGE_HEIGHT "image height"

#define GFMNU_ATTR_ARROWS_WIDTH "arrows width"
#define GFMNU_ATTR_ARROWS_HEIGHT "arrows height"

#define GFMNU_ATTR_SCROLLBAR_POS "scrollbar pos"
#define GFMNU_ATTR_SCROLLBAR_WIDTH "scrollbar width"
#define GFMNU_ATTR_SCROLLBAR_BUTTONS_HEIGHT "scrollbar buttons height"

#define GFMNU_ATTR_CAN_DEFORM "can deform"

#define GFMNU_ATTR_BOX_SHOW "show box"

#define GFMNU_ATTR_CHECKED "checked"

#define GFMNU_ATTR_MIN "min"
#define GFMNU_ATTR_MAX "max"
#define GFMNU_ATTR_VALUE "value"

#define GFMNU_VAL_YES "yes"
#define GFMNU_VAL_NO "no"
#define GFMNU_VAL_TRUE "true"
#define GFMNU_VAL_FALSE "false"

#define GFMNU_VAL_LEFT "left"
#define GFMNU_VAL_RIGHT "right"
#define GFMNU_VAL_CENTER "center"

#define GFMNU_ATTR_MUSIC_FILE "music file"

#endif /* __MENU__H__ */ 



