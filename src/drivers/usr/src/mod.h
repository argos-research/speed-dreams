/***************************************************************************

    file                 : mod.h
    created              : Wed May 14 19:53:00 CET 2003
    copyright            : (C) 2003-2004 by Bernhard Wymann
    email                : berniw@bluewin.ch
    version              : $Id: mod.h 5950 2015-04-05 19:34:04Z torcs-ng $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _MOD_H_
#define _MOD_H_

#define LMOD_DATA 200

typedef struct
{
    double dval;
    int ival;
    int divstart;
    int divend;
} LRLModData;

typedef struct {
    LRLModData data[LMOD_DATA];
    int used;
} LRLMod;

extern void AddMod( LRLMod *mod, int divstart, int divend, double dval, int ival );
extern double GetModD( LRLMod *mod, int div );
extern int GetModI( LRLMod *mod, int div );

#endif
