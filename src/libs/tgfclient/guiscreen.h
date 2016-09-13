/***************************************************************************

    file        : guiscreen.h
    created     : Sat Apr 19 23:37:41 CEST 2003
    copyright   : (C) 2003 by Eric Espie                        
    email       : eric.espie@torcs.org   
    version     : $Id: guiscreen.h 5917 2015-03-22 18:47:51Z torcs-ng $

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
    		Constants for screen / Open GL features config file
    @author	<a href=mailto:torcs@free.fr>Eric Espie</a>
    @version	$Id: guiscreen.h 5917 2015-03-22 18:47:51Z torcs-ng $
*/

#ifndef _GUISCREEN_H_
#define _GUISCREEN_H_

#define GFSCR_CONF_FILE                             "config/screen.xml"

#define GFSCR_SECT_VALIDPROPS                       "Validated Screen Properties"
#define GFSCR_SECT_INTESTPROPS                      "In-Test Screen Properties"

#define GFSCR_ATT_BPP                               "bpp"
#define GFSCR_ATT_WIN_X                             "window width"
#define GFSCR_ATT_WIN_Y                             "window height"
#define GFSCR_ATT_MAXREFRESH                        "maximum refresh frequency"
#define GFSCR_ATT_FSCR                              "full-screen"
#define GFSCR_VAL_YES                               "yes"
#define GFSCR_VAL_NO                                "no"
#define GFSCR_ATT_GAMMA                             "gamma"
#define GFSCR_ATT_ALPHACHANNEL                      "alpha channel"

#define GFSCR_ATT_TESTSTATE                         "test state"
#define GFSCR_VAL_TODO                              "to do"
#define GFSCR_VAL_INPROGRESS                        "in progress"
#define GFSCR_VAL_FAILED                            "failed"

#define GFSCR_ATT_VDETECT                           "video mode detect"
#define GFSCR_VAL_VDETECT_AUTO                      "auto"
#define GFSCR_VAL_VDETECT_MANUAL                    "manual"

#define GFSCR_ATT_VINIT                             "video mode init"
#define GFSCR_VAL_VINIT_COMPATIBLE                  "compatible"
#define GFSCR_VAL_VINIT_BEST                        "best"

// Default settngs or menus / controls
#define GFSCR_SECT_MENUSETTINGS                     "Menu Settings"

#define GFSCR_LIST_COLORS                           "colors"

#define GFSCR_ELT_BGCOLOR                           "background"
#define GFSCR_ELT_BGBTNFOCUS                        "background focused button"
#define GFSCR_ELT_BGBTNCLICK                        "background pushed button"
#define GFSCR_ELT_BGBTNENABLED                      "background enabled button"
#define GFSCR_ELT_BGBTNDISABLED                     "background disabled button"
#define GFSCR_ELT_BTNFOCUS                          "focused button"
#define GFSCR_ELT_BTNCLICK                          "pushed button"
#define GFSCR_ELT_BTNENABLED                        "enabled button"
#define GFSCR_ELT_BTNDISABLED                       "disabled button"
#define GFSCR_ELT_LABELCOLOR                        "label"
#define GFSCR_ELT_TIPCOLOR                          "tip"
#define GFSCR_ELT_BGSCROLLIST                       "background scroll list"
#define GFSCR_ELT_SCROLLIST                         "scroll list"
#define GFSCR_ELT_BGSELSCROLLIST                    "background selected scroll list"
#define GFSCR_ELT_SELSCROLLIST                      "selected scroll list"
#define GFSCR_ELT_BGEDITFOCUS                       "background focused editbox"
#define GFSCR_ELT_BGEDITENABLED                     "background enabled editbox"
#define GFSCR_ELT_BGEDITDISABLED                    "background disabled editbox"
#define GFSCR_ELT_EDITFOCUS                         "focused editbox"
#define GFSCR_ELT_EDITENABLED                       "enabled editbox"
#define GFSCR_ELT_EDITDISABLED                      "disabled editbox"
#define GFSCR_ELT_EDITCURSORCLR                     "editbox cursor color"
#define GFSCR_ELT_BASECOLORBGIMAGE                  "base color background image"
#define GFSCR_ELT_PROGRESSCOLOR                     "progress outline color"

#define GFSCR_ATTR_RED                              "red"
#define GFSCR_ATTR_GREEN                            "green"
#define GFSCR_ATTR_BLUE                             "blue"
#define GFSCR_ATTR_ALPHA                            "alpha"

#define GFSCR_SECT_TIP                              "tip"

#define GFSCR_ATT_FONT                              "font"
#define GFSCR_ATT_ALIGN                             "align"
#define GFSCR_ATT_X                                 "x"
#define GFSCR_ATT_Y                                 "y"
#define GFSCR_ATT_WIDTH                             "width"

#define GFSCR_SECT_EDITBOX                          "editbox"

#define GFSCR_ATT_HPADDING                          "h padding"
#define GFSCR_ATT_VPADDING                          "v padding"

#define GFSCR_SECT_TEXTBUTTON                       "text button"

#define GFSCR_SECT_IMAGEBUTTON                      "image button"

// Mouse cursor
#define GFSCR_SECT_MOUSECURSOR                      "Mouse Cursor"

#define GFSCR_ATT_IMAGEFILE                         "image file"
#define GFSCR_ATT_WIDTH                             "width"
#define GFSCR_ATT_HEIGHT                            "height"
#define GFSCR_ATT_XOFFSET                           "x offset"
#define GFSCR_ATT_YOFFSET                           "y offset"

// Open GL user-selected features
#define GFSCR_SECT_GLSELFEATURES                    "OpenGL Selected Features"

#define GFSCR_ATT_TEXTURECOMPRESSION                "texture compression"
#define GFSCR_ATT_TEXTURECOMPRESSION_ENABLED        "enabled"
#define GFSCR_ATT_TEXTURECOMPRESSION_DISABLED       "disabled"

#define GFSCR_ATT_BUMPMAPPING                       "bump mapping"
#define GFSCR_ATT_BUMPMAPPING_ENABLED               "enabled"
#define GFSCR_ATT_BUMPMAPPING_DISABLED              "disabled"

#define GFSCR_ATT_MAXTEXTURESIZE                    "max texture size"

#define GFSCR_ATT_MULTITEXTURING                    "multi-texturing"
#define GFSCR_ATT_MULTITEXTURING_ENABLED            "enabled"
#define GFSCR_ATT_MULTITEXTURING_DISABLED           "disabled"

#define GFSCR_ATT_MULTISAMPLING                     "multi-sampling"
#define GFSCR_ATT_MULTISAMPLING_ENABLED             "enabled"
#define GFSCR_ATT_MULTISAMPLING_DISABLED            "disabled"

#define GFSCR_ATT_STEREOVISION                      "stereo-vision"
#define GFSCR_ATT_STEREOVISION_ENABLED              "enabled"
#define GFSCR_ATT_STEREOVISION_DISABLED             "disabled"

#define GFSCR_ATT_ANISOTROPICFILTERING              "anisotropic filtering"
#define GFSCR_ATT_ANISOTROPICFILTERING_HIGH         "high"
#define GFSCR_ATT_ANISOTROPICFILTERING_MEDIUM       "medium"
#define GFSCR_ATT_ANISOTROPICFILTERING_DISABLED     "disabled"


#define GFSCR_ATT_MULTISAMPLINGSAMPLES              "multi-sampling samples"

// Open GL auto-detected features
#define GFSCR_SECT_GLDETFEATURES                    "OpenGL Detected Features"

#define GFSCR_ATT_DOUBLEBUFFER                      "double buffer"
#define GFSCR_ATT_COLORDEPTH                        "color depth"
#define GFSCR_ATT_ALPHADEPTH                        "alpha depth"
#define GFSCR_ATT_RECTANGLETEXTURES                 "rectangle textures"
#define GFSCR_ATT_NONPOTTEXTURES                    "non-pot textures"
#define GFSCR_ATT_MULTITEXTURINGUNITS               "multi-texturing units"

// Open GL detection specs
#define GFSCR_SECT_GLDETSPECS                       "OpenGL Detection Specs"

#endif /* _GUISCREEN_H_ */



