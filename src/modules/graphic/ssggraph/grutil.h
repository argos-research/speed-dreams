/***************************************************************************

    file                 : grutil.h
    created              : Wed Nov  1 22:35:08 CET 2000
    copyright            : (C) 2000 by Eric Espie
    email                : torcs@free.fr
    version              : $Id: grutil.h 4012 2011-10-29 11:01:26Z pouillot $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _GRUTIL_H_
#define _GRUTIL_H_

#include <cstdio>

#include "grtexture.h"

#if 1
#define TRACE_GL(msg) { GLenum rc; if ((rc = glGetError()) != GL_NO_ERROR) GfLogWarning("%s %s\n", msg, gluErrorString(rc)); }
#else
#define TRACE_GL(msg)
#endif

#ifdef DEBUG
#define DBG_SET_NAME(base, name, index, subindex)		\
{								\
    char __buf__[256];						\
    if (subindex != -1) {					\
        sprintf(__buf__, "%s-%d-%d", name, index, subindex);	\
    } else {							\
	sprintf(__buf__, "%s-%d", name, index);			\
    }								\
    (base)->setName((const char *)__buf__);			\
}
#else
#define DBG_SET_NAME(base, name, index, subindex)
#endif
 

/* Vars to set before calling grSsgLoadTexCb */
extern float grGammaValue;
extern int	 grMipMap;

extern char *grFilePath;	/* Multiple path (: separated) used to search for files */

extern int grGetFilename(const char *filename, const char *filepath, char *buf);
extern cgrMultiTexState* grSsgEnvTexState(const char *img,
										  cgrMultiTexState::tfnTexScheme fnTexScheme,
										  int errIfNotFound = TRUE);
extern ssgState* grSsgLoadTexState(const char *img, int errIfNotFound = TRUE);
extern ssgState* grSsgLoadTexStateEx(const char *img, const char *filepath,
									 int wrap, int mipmap, int errIfNotFound = TRUE);
extern void grShutdownState(void);
extern void grWriteTime(float *color, int font, int x, int y, int width, tdble sec, int sgn);
extern void grWriteTimeBuf(char *buf, tdble sec, int sgn);
extern float grGetHOT(float x, float y);

inline float urandom() { return(((float)rand() / (1.0 + (float)RAND_MAX)));}

#endif /* _GRUTIL_H_ */ 
