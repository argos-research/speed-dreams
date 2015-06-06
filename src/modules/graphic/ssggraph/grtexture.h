/***************************************************************************

    file                 : grtexture.h
    created              : Wed Jun 1 14:56:31 CET 2005
    copyright            : (C) 2005 by Bernhard Wymann
    version              : $Id: grtexture.h 4903 2012-08-27 11:31:33Z kmetykog $

***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

/*
	This classes/methods are used to handle texture compression and
	textures which are shared among multiple objects. In the long term
	they should obsolete parts of grutil.cpp.
*/

#ifndef _GRTEXTURE_H_
#define _GRTEXTURE_H_

#include <plib/ssg.h>	// ssgXXX
#include <tgfclient.h>

#include "grsimplestate.h"	// cgrSimpleState
#include "grmultitexstate.h"	// cgrMultiTexState
#include "loadsgi.h"	// ssgSGIHeader


extern int doMipMap(const char *tfname, int mipmap);

extern bool grMakeMipMaps(GLubyte *image, int xsize, int ysize, int zsize, int mipmap);

// The state factory.
// TODO: really manage shared textures (see obsolete grutil.cpp parts).
class cgrStateFactory
{
 public:

	cgrSimpleState* getSimpleState();

	cgrMultiTexState* getMultiTexState(cgrMultiTexState::tfnTexScheme fnTexScheme
									   = cgrMultiTexState::modulate);
};

extern cgrStateFactory* grStateFactory;

// Register customized loader in plib.
extern void grRegisterCustomSGILoader(void);

extern bool grLoadPngTexture(const char *fname, ssgTextureInfo* info);
extern bool grLoadJpegTexture(const char *fname, ssgTextureInfo* info);

// SGI loader class to call customized ssgMakeMipMaps. This is necessary because
// of plib architecture which does not allow to customize the mipmap
// generation.
class cgrSGIHeader : public ssgSGIHeader
{
 public:

	cgrSGIHeader(const char *fname, ssgTextureInfo* info);

};

#endif // _GRTEXTURE_H_
