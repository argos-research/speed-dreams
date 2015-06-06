/***************************************************************************

    file        : portability.h
    created     : Fri Jul 8 15:19:34 CET 2005
    copyright   : (C) 2005 Bernhard Wymann
    email       : berniw@bluewin.ch
    version     : $Id: portability.h 5308 2013-03-12 14:55:59Z beaglejoe $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _SD_PORTABILITY_H_
#define _SD_PORTABILITY_H_

#include <cstdlib>
#include <cstring>

#define _USE_MATH_DEFINES
#include <cmath>

#ifdef _MSC_VER
#include <direct.h>
#include <process.h>
#define _WINSOCKAPI_   /* Prevent inclusion of winsock.h in windows.h */
#else
#include <unistd.h>
#endif

#ifdef _MSC_VER
#ifndef HAVE_CONFIG_H
#define HAVE_CONFIG_H
#endif
#endif

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

// Missing isnan (for Apple).
#ifndef HAVE_ISNAN

#ifndef isnan
#define isnan(x) ((x) != (x))
#endif

#endif

// DLL exported symbols declarator for Windows.
#ifdef WIN32
# ifdef portability_EXPORTS
#  define PORTABILITY_API __declspec(dllexport)
# else
#  define PORTABILITY_API __declspec(dllimport)
# endif
#else
# define PORTABILITY_API
#endif

// Missing strndup, defined in portability.cpp (for FreeBSD).
#ifndef HAVE_STRNDUP

PORTABILITY_API char *strndup(const char *str, int len);

#endif // HAVE_STRNDUP

// Missing strtok_r, defined in portability.cpp (for MinGW).
#ifndef HAVE_STRTOK_R

PORTABILITY_API char *strtok_r(char *str, const char *delim, char **nextp);

#endif // HAVE_STRTOK_R


// Apple platform ---------------------------------------------------
#ifdef __APPLE__

// Nothing special for the moment.

#endif

// Windows platform -------------------------------------------------
// Posix functions special names with MS compilers.
// Notes about MSVC compilers detection :
// * _MSC_VER define should be prefered to WIN32/_WIN32
// * MSVC    6 : 1200 <= _MSC_VER < 1300
// * MSVC 2003 : 1300 <= _MSC_VER < 1400
// * MSVC 2005 : 1400 <= _MSC_VER < 1500
// * MSVC 2008 : 1500 <= _MSC_VER < 1600
// * MSVC 2010 : 1600 <= _MSC_VER
#ifdef _MSC_VER

#define isnan _isnan

// Ticket #663 - MSVC implementation of snprintf is not safe
// We provide our own version of the function,
// that ensures 0 ending for the string.
PORTABILITY_API int SD_snprintf(char *str, size_t size, const char *format, ...);

#include <cstdarg>
#include <cstdio>
#define snprintf SD_snprintf

#define access _access

// Workaround for sucking MSVC "access" function in C lib :
// * define F_OK, R_OK, W_OK and X_OK.
// * X_OK : no "executable" bit under Windows => use "R_OK" 
#ifdef F_OK
#undef F_OK
#endif
#define F_OK 0x0

#ifdef W_OK
#undef W_OK
#endif
#define W_OK 0x2

#ifdef R_OK
#undef R_OK
#endif
#define R_OK 0x4

#ifdef X_OK
#undef X_OK
#endif
#define X_OK R_OK

// For MSVC 2005 and older (2008 already defines these)
#if _MSC_VER < 1500

#define vsnprintf _vsnprintf

#endif // _MSC_VER < 1500

// For MSVC 2005 and newer
#if _MSC_VER >= 1400

#ifdef strdup
#undef strdup
#endif
#define strdup _strdup

#define stricmp _stricmp
#define strnicmp _strnicmp

#define strncasecmp strnicmp
#define strcasecmp stricmp

#define strtok_r strtok_s

#define chdir _chdir
#define getcwd _getcwd
#define chmod _chmod
#define hypot _hypot

#ifdef mkdir
#undef mkdir
#endif
#define mkdir(x) _mkdir(x)

#define execvp _execvp
#define execlp _execlp

#endif // _MSC_VER >= 1400

// TODO: Remove this, now useless (see above, cmath included).
// Constants from cmath / math.h
// Note: Defining _USE_MATH_DEFINES before including cmath / math.h
//       normally defines them, but it is not very handy.
#ifndef M_PI

#define M_E        2.71828182845904523536
#define M_LOG2E    1.44269504088896340736
#define M_LOG10E   0.434294481903251827651
#define M_LN2      0.693147180559945309417
#define M_LN10     2.30258509299404568402
#define M_PI       3.14159265358979323846
#define M_PI_2     1.57079632679489661923
#define M_PI_4     0.785398163397448309616
#define M_1_PI     0.318309886183790671538
#define M_2_PI     0.636619772367581343076
#define M_2_SQRTPI 1.12837916709551257390
#define M_SQRT2    1.41421356237309504880
#define M_SQRT1_2  0.707106781186547524401

#endif

#endif // _MSC_VER

// MinGW compiler ---------------------------------------------------
#ifdef __MINGW32__

#define isnan(x) ((x) != (x))

#endif

#endif // _SD_PORTABILITY_H_

