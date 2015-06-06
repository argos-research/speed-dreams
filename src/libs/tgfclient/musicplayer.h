#ifndef __musicplayer_h__
#define __musicplayer_h__

/***************************************************************************

    file                 : musicplayer.h
    created              : Fri Dec 23 17:35:18 CET 2011
    copyright            : (C) 2011 Bernhard Wymann
    email                : berniw@bluewin.ch
    version              : $Id: musicplayer.h 5108 2013-01-25 20:47:12Z torcs-ng $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

//#define MM_SOUND_PARM_CFG			"config/sound.xml"
//#define MM_SCT_SOUND				"Menu Music"
//#define MM_ATT_SOUND_ENABLE			"enable"
//#define MM_VAL_SOUND_ENABLED		"enabled"
//#define MM_VAL_SOUND_DISABLED		"disabled"

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

TGFCLIENT_API void initMusic();
TGFCLIENT_API void shutdownMusic();
TGFCLIENT_API void playMusic(char* filename);

#endif //__musicplayer_h__