/***************************************************************************

    file                 : grsimplestate.h
    created              : Wed Jun 1 14:56:31 CET 2005
    copyright            : (C) 2005 by Bernhard Wymann
    version              : $Id: grsimplestate.h 4374 2012-01-07 22:20:37Z pouillot $

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

#ifndef _GRSIMPLESTATE_H_
#define _GRSIMPLESTATE_H_

#include <plib/ssg.h>	// ssgXXX


// Simple state (a ssgSimpleState with mipmapping).
class cgrSimpleState : public ssgSimpleState
{
 public:
		
	cgrSimpleState();
	
	virtual void setTexture(ssgTexture *tex);

	virtual void setTexture(const char *fname,
							int wrapu = TRUE, int wrapv = TRUE, int mipmap = TRUE);

	virtual void setTexture(GLuint tex);
};

#endif // _GRSIMPLESTATE_H_
