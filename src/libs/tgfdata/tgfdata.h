/***************************************************************************
                    tgfdata.h -- The Gaming Framework Data Manager API
                             -------------------                                         
    created              : Sat Dec 11 22:32:14 CEST 2010
    copyright            : (C) 2010 Jean-Philippe Meuret
    web                  : http:://www.speed-dreams.org   
    version              : $Id: tgfdata.h 4902 2012-08-27 10:04:20Z kmetykog $

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
    	The Gaming Framework Data Manager API.
    @version	$Id: tgfdata.h 4902 2012-08-27 10:04:20Z kmetykog $
*/

#ifndef __TGFDATA__H__
#define __TGFDATA__H__

// DLL exported symbols declarator for Windows.
#ifdef WIN32
# ifdef TGFDATA_DLL
#  define TGFDATA_API __declspec(dllexport)
# else
#  define TGFDATA_API __declspec(dllimport)
# endif
#else
# define TGFDATA_API
#endif

#ifdef _MSC_VER
// Disable useless MSVC warnings
#  pragma warning (disable:4251) // class XXX needs a DLL interface ...
#endif

// Global management of the race data manager.
class TGFDATA_API GfData
{
 public:
	static void initialize();
	static void shutdown();
};

#endif /* __TGFDATA__H__ */


