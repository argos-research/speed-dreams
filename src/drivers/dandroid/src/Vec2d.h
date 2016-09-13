/***************************************************************************

    file        : Vec2d.h
    created     : 9 Apr 2006
    copyright   : (C) 2006 Tim Foden

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#ifndef _VEC2D_H_
#define _VEC2D_H_

#include "torcs_or_sd.h"

#ifdef DANDROID_TORCS
#include <tmath/v2_t.h>
#else
#include <v2_t.h>
#endif

#include <tgf.h>

class Vec2d : public v2t<double>
{
  public:
  Vec2d() {}
  Vec2d( const v2t<double>& v ) : v2t<double>(v) {}
  Vec2d( double x, double y ) : v2t<double>(x, y) {};

  Vec2d&	operator=( const v2t<double>& v )
  {
    v2t<double>::operator=(v);
    return *this;
  }
};

#endif // _VEC2D_H_
