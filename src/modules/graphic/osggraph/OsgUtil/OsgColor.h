/***************************************************************************

    file                     : OsgColor.h
    created                  : Sat Jan 13 00:00:41 CEST 2013
    copyright                : (C) 2012 by Xavier Bertaux
    email                    : bertauxx@yahoo.fr
    version                  : $Id: OsgColor.h 4693 2013-01-12 03:12:09Z torcs-ng $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _OSGCOLORS_H
#define _OSGCOLORS_H 1

#include <math.h>

#if defined (sgi)
const float system_gamma = 2.0/1.7;

#else	// others
const float system_gamma = 2.5;
#endif

// simple architecture independant gamma correction function.
inline void sd_gamma_correct_rgb(float *color,
                              float reff = 2.5, float system = system_gamma)
{
    if (reff == system)
       return;

    float tmp = reff/system;
    color[0] = pow(color[0], tmp);
    color[1] = pow(color[1], tmp);
    color[2] = pow(color[2], tmp);
};

inline void sd_gamma_correct_c(float *color,
                            float reff = 2.5, float system = system_gamma)
{
   if (reff == system)
      return;

   *color = pow(*color, reff/system);
};

inline void sd_gamma_restore_rgb(float *color,
                              float reff = 2.5, float system = system_gamma)
{
    if (reff == system)
       return;

    float tmp = system/reff;
    color[0] = pow(color[0], tmp);
    color[1] = pow(color[1], tmp);
    color[2] = pow(color[2], tmp);
};

inline void sd_gamma_restore_c(float *color,
                            float reff = 2.5, float system = system_gamma)
{
   if (reff == system)
      return;

   *color = pow(*color, system/reff);
};

#endif // _OSGCOLORS_H

