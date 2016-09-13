/***************************************************************************

    file                 : mod.cpp
    created              : Wed May 14 19:53:00 CET 2003
    copyright            : (C) 2003-2004 by Bernhard Wymann
    email                : berniw@bluewin.ch
    version              : $Id: mod.cpp 5950 2015-04-05 19:34:04Z torcs-ng $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "mod.h"


void AddMod( LRLMod *mod, int divstart, int divend, double dval, int ival )
{
    if (!mod) return;

    mod->data[mod->used].divstart = divstart;
    mod->data[mod->used].divend = divend;
    mod->data[mod->used].dval = dval;
    mod->data[mod->used].ival = ival;
    mod->used++;
}


double GetModD( LRLMod *mod, int div )
{
    int i;

    if (!mod)
        return 0.0;

    for (i=0; i<mod->used; i++)
    {
        if (div >= mod->data[i].divstart && div <= mod->data[i].divend)
            return mod->data[i].dval;
    }
    return 0.0;
}

int GetModI( LRLMod *mod, int div )
{
    int i;

    if (!mod)
        return 0;

    for (i=0; i<mod->used; i++)
    {
        if (div >= mod->data[i].divstart && div <= mod->data[i].divend)
            return mod->data[i].ival;
    }
    return 0;
}
