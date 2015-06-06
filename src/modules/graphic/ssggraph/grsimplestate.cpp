/***************************************************************************

    file                 : grsimplestate.cpp
    created              : Wed Jun 1 14:56:31 CET 2005
    copyright            : (C) 2005 by Bernhard Wymann
    version              : $Id: grsimplestate.cpp 4374 2012-01-07 22:20:37Z pouillot $

***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <tgf.h> // GfLog*

#include "grsimplestate.h"

#include "grtexture.h" // doMipMap


// cgrSimpleState class ========================================================

cgrSimpleState::cgrSimpleState()
: ssgSimpleState()
{
}

void cgrSimpleState::setTexture(ssgTexture *tex)
{
	ssgSimpleState::setTexture(tex);
}

void cgrSimpleState::setTexture(const char *fname, int wrapu, int wrapv, int mipmap)
{
	mipmap = doMipMap(fname, mipmap);
	
	ssgSimpleState::setTexture(fname, wrapu, wrapv, mipmap);
}

void cgrSimpleState::setTexture(GLuint tex)
{
	GfLogWarning("Obsolete call: setTexture(GLuint tex)\n");
	
	ssgSimpleState::setTexture(tex);
}
