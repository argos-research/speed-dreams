/***************************************************************************
                        Os.h --- os specific functions interface                                
                             -------------------                                         
    created              : Fri Aug 13 22:27:29 CEST 1999
    copyright            : (C) 1999 by Eric Espie                         
    email                : torcs@free.fr   
    version              : $Id: os.h 5067 2012-12-15 17:35:33Z pouillot $                                  
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
 
#ifndef _OS__H_
#define _OS__H_

#include "tgf.h"


/* dynamic module (dll) interfaces */
typedef int (*tfModLoad)(unsigned int, const char*, tModList **);
typedef int (*tfModLoadDir)(unsigned int, const char*, tModList **);
typedef int (*tfModUnloadList)(tModList **);
typedef int (*tfModGetInfo)(unsigned int, const char*, tModList **);
typedef int (*tfModGetInfoDir)(unsigned int, const char*, int, tModList **);
typedef int (*tfModFreeInfoList)(tModList **);

/* directory interface */
typedef tFList *(*tfDirGetList)(const char *);
typedef tFList *(*tfDirGetListFiltered)(const char *, const char *, const char *);

/* time interface */
typedef double (*tfTimeClock)(void);

/* System interface */
typedef unsigned (*tfSysGetNumberOfCPUs)(void);
typedef bool (*tfSysSetThreadAffinity)(int nCPUId);
typedef bool (*tfSysGetOSInfo)(std::string&, int&, int&, int&, int&);

typedef struct {
    tfModLoad			modLoad;
    tfModLoadDir		modLoadDir;
    tfModUnloadList		modUnloadList;
    tfModGetInfo		modInfo;
    tfModGetInfoDir		modInfoDir;
    tfModFreeInfoList		modFreeInfoList;
    tfDirGetList		dirGetList;
    tfDirGetListFiltered	dirGetListFiltered;
    tfTimeClock			timeClock;
    tfSysSetThreadAffinity	sysSetThreadAffinity;
    tfSysGetNumberOfCPUs	sysGetNumberOfCPUs;
    tfSysGetOSInfo		sysGetOSInfo;
} tGfOs;

TGF_API extern tGfOs GfOs;

#endif /* _OS__H_ */
