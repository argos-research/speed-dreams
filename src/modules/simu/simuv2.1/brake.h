/***************************************************************************

    file                 : brake.h
    created              : Sun Mar 19 00:05:34 CET 2000
    copyright            : (C) 2000 by Eric Espie
    email                : torcs@free.fr
    version              : $Id: brake.h 2917 2010-10-17 19:03:40Z pouillot $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _BRAKE_H_
#define _BRAKE_H_

typedef struct
{
    tdble	pressure;
    tdble	Tq;
    tdble	coeff;
    tdble	I;
    tdble	radius;
    tdble	temp;
} tBrake;

typedef struct
{
    tdble	rep;	/* front/rear repartition */ 
    tdble	coeff;
    tdble   ebrake_pressure;
} tBrakeSyst;



#endif /* _BRAKE_H_ */ 



