/***************************************************************************
                   modinfo.h -- Tools for legacy module interface management

    created              : Fri Aug 13 22:32:14 CEST 1999
    copyright            : (C) 2008 by Jean-Philippe Meuret                         
    email                : jpmeuret@free.fr
    version              : $Id: modinfo.cpp 5357 2013-03-24 13:39:48Z pouillot $
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

/** @file   
	Tools for legacy module interface management.
    @author	<a href=mailto:jpmeuret@free.fr>Jean-Philippe Meuret</a>
    @version	$Id: modinfo.cpp 5357 2013-03-24 13:39:48Z pouillot $
*/

#include <cstring>
#include <climits>

#ifdef WIN32
#include <windows.h>
#include <direct.h>
#else
#include <dlfcn.h>
#endif

#include <portability.h>

#include "tgf.h"


#ifdef WIN32
#define dlsym   GetProcAddress
#define dlerror GetLastError
#endif

static const size_t SOFileExtLen = strlen("."DLLEXT);

/* Allocate the module interfaces info array */
tModInfo *GfModInfoAllocate(int maxItf)
{
    tModInfo *array = (tModInfo*)calloc(maxItf+1, sizeof(tModInfo));
    if (!array)
        GfLogError("GfModInfoAllocate: Failed to allocate tModInfo array (maxItf=%d)\n", maxItf);

    return array;
}

/* Free the module interfaces info array */
void GfModInfoFree(tModInfo *array)
{
    if (!array)
		GfLogError("GfModInfoFree: Null pointer\n");

	free(array);
}

/**
 * Copies something of type tModList into something of type tModListNC
 * This function allocates new space for parts of tModList that are const.
 * @param constArray The source tModList
 * @param maxItf     The max number of interfaces of the module
 */
tModInfoNC *GfModInfoDuplicate(const tModInfo *constArray, int maxItf)
{
    int itfInd;

    // Allocate target array.
    tModInfoNC *array = (tModInfoNC*)calloc(maxItf + 1, sizeof(tModInfoNC));
    if (!constArray)
        GfLogError("GfModInfoAllocate: Failed to allocate tModInfoNC array (maxItf=%d)\n", maxItf);

    // Copy constArray to array (null name indicates unused interface = end of useful array).
    memset(array, 0, (maxItf+1)*sizeof(tModInfo));
    for( itfInd = 0; itfInd < maxItf + 1; ++itfInd )
    {
		// Note: the the last item in the list is the template
		//       for the "generated" instances of robots.
        if( !constArray[itfInd].name )
		{
			if( itfInd >= maxItf )
				break;
			// Go to the last item of the list
			itfInd = maxItf - 1;
			continue;
		}
        array[itfInd].name    = constArray[itfInd].name ? strdup(constArray[itfInd].name) : 0;
        array[itfInd].desc    = constArray[itfInd].desc ? strdup(constArray[itfInd].desc) : 0;
        array[itfInd].fctInit = constArray[itfInd].fctInit;
        array[itfInd].gfId    = constArray[itfInd].gfId;
        array[itfInd].index   = constArray[itfInd].index;
        array[itfInd].prio    = constArray[itfInd].prio;
        array[itfInd].magic   = constArray[itfInd].magic;
    }

    return array;
}

/* Free the module interfaces info array */
void GfModInfoFreeNC(tModInfoNC *array, int maxItf)
{
    int itfInd;

    if (!array)
    {
		GfLogError("GfModInfoFreeNC: Null pointer\n");
		return;
    }

	for( itfInd = 0; itfInd < maxItf + 1; ++itfInd )
	{
		// Note: the the last item in the list is the template
		//       for the "generated" instances of robots.
 		if( !array[itfInd].name )
		{
			if( itfInd >= maxItf )
				break;
			//Go to the last item of the list
			itfInd = maxItf - 1;
			continue;
		}
		if (array[itfInd].name)
			free(array[itfInd].name);
		if (array[itfInd].desc)
			free(array[itfInd].desc);
	}
	
	free(array);
}

/*
 * Function
 *	GfModInitialize
 *
 * Description
 *	Initialize the module with given handle and library file path
 *
 * Parameters
 *	soHandle (in)	handle of the loaded shared library
 *	soPath   (in)	path of the loaded shared library
 *	gfid     (in)	id of the gaming framework that the module MUST implement to be initialized
 *	                (Not taken into account if == GfIdAny)
 *	mod      (in)	address of module entry to allocate and initialize  
 *	                (0 if any error occured or modules not retained for bad GFId)
 *
 * Return
 *	0	if initialization succeeded (even if the module was rejected for bad GFId)
 *	-1	if any error occured
 *
 * Remarks
 *	
 */
int GfModInitialize(tSOHandle soHandle, const char *soPath, unsigned int gfid, tModList **mod)
{
    tfModInfoWelcome    fModInfoWelcome; /* function to exchange  information with the module at welcome time */
    tfModInfoInitialize	fModInfoInit = 0;	 /* init function of the module */
    int   initSts = 0;	 /* returned status */
    int	  retained = 1;
    char  soName[256];
    char  soDir[1024];
    char* lastSlash;
    
    /* Allocate module entry in list */
    if (!(*mod = (tModList*)calloc(1, sizeof(tModList))))
    {
		GfLogError("GfModInitialize: Failed to allocate tModList for module %s\n", soPath);
		return -1;
    }
    
    /* Determine the shared library / DLL name and load dir */
    strcpy(soDir, soPath);
    lastSlash = strrchr(soDir, '/');
    if (lastSlash)
    {
		strcpy(soName, lastSlash+1);
		*lastSlash = 0;
    }
    else
    {
		strcpy(soName, soPath);
		*soDir = 0;
    }
    soName[strlen(soName) - SOFileExtLen] = 0; /* cut so file extension */

    /* Welcome the module : exchange informations with it :
       1) Call the dedicated module function if present */
    if ((fModInfoWelcome = (tfModInfoWelcome)dlsym(SOHandle(soHandle), GfModInfoWelcomeFuncName)) != 0) 
    {
        /* Prepare information to give to the module */
		tModWelcomeIn welcomeIn;
		welcomeIn.itfVerMajor = 1;
		welcomeIn.itfVerMinor = 0;
		welcomeIn.name = soName;

		/* Prepare a place for the module-given information */
		tModWelcomeOut welcomeOut;

		/* Call the welcome function */
		if ((initSts = fModInfoWelcome(&welcomeIn, &welcomeOut)) != 0)
		{
			GfLogError("GfModInitialize: Module welcome function failed %s\n", soPath);
		}
		else
		{
			/* Save information given by the module */
			(*mod)->modInfoSize = welcomeOut.maxNbItf;
		}
    } 

    /* 2) If not present, default number of interfaces (backward compatibility) */
    else
    {
        (*mod)->modInfoSize = GfModInfoDefaultMaxItf;
    }

    /* Get module initialization function if welcome succeeded :
       1) Try the new sheme (fixed name) */
    if (initSts == 0)
	{
		fModInfoInit =
			(tfModInfoInitialize)dlsym(SOHandle(soHandle), GfModInfoInitializeFuncName);
		//GfLogDebug("GfModInitialize: fModInfoInit(%s) @ %p\n",
		//		   GfModInfoInitializeFuncName, fModInfoInit);
	}
	/* 2) Backward compatibility (dll name) */
	if (initSts == 0 && !fModInfoInit)
	{
		fModInfoInit = (tfModInfoInitialize)dlsym(SOHandle(soHandle), soName);
		//GfLogDebug("GfModInitialize: fModInfoInit(%s) @ %p\n", soName, fModInfoInit);
	}
	
    /* Call module initialization function if welcome succeeded and init function found */
    if (initSts == 0 && fModInfoInit) 
    {
		/* Allocate module interfaces info array according to the size we got */
		tModInfo* constModInfo;
		if ((constModInfo = GfModInfoAllocate((*mod)->modInfoSize)) != 0) 
		{
			/* Library loaded, init function exists, call it... */
			if ((initSts = fModInfoInit(constModInfo)) == 0)
			{
				/* Duplicate strings in each interface, in case the module gave us static data ! */
				if (((*mod)->modInfo = GfModInfoDuplicate(constModInfo, (*mod)->modInfoSize)) != 0) 
				{
					/* Reject module if not of requested gaming framework Id */
					if (gfid != GfIdAny && (*mod)->modInfo[0].gfId != gfid) 
					{
						GfLogTrace("GfModInitialize: Module not retained %s\n", soPath);
						GfModInfoFreeNC((*mod)->modInfo, (*mod)->modInfoSize);
						retained = 0;
					}
		    
					/* Free the module info data returned by the module (we have a copy) */
					GfModInfoFree(constModInfo);
				}
				else
				{
					initSts = -1;
				}
			} 
			else
			{
				GfLogError("GfModInitialize: Module init function failed %s\n", soPath);
			}
		}
		else
		{
			initSts = -1;
		}
    } 

    /* If init function not found, we have a problem ... */
    else
    {
		GfLogError("GfModInitialize: Module init function %s not found\n", 
				soPath);
		initSts = -1;
    }

    /* Store other module information */
    if (initSts == 0 && retained)
    {
        GfOut("Initialized module %s (maxItf=%d)\n", soPath, (*mod)->modInfoSize);
		(*mod)->handle = (tSOHandle)soHandle;
		(*mod)->sopath = strdup(soPath);
    }
    else
    {
        free(*mod);
		*mod = 0;
    }

    return initSts;
}

/*
 * Function
 *	GfModTerminate
 *
 * Description
 *	Terminate the module with given handle and library file path
 *
 * Parameters
 *	soHandle (in)	handle of the loaded shared library
 *	soPath   (in)	path of the loaded shared library
 *
 * Return
 *	0	if termination succeeded
 *	-1	if any error occured
 *
 * Remarks
 *	
 */
int GfModTerminate(tSOHandle soHandle, const char *soPath)
{
    tfModInfoTerminate fModInfoTerm;	/* Termination function of the module */
    int			termSts = 0;	/* Returned status */
    
    /* Get the module termination function if any :
       1) Try the new sheme (fixed name) */
    if ((fModInfoTerm = (tfModInfoTerminate)dlsym(SOHandle(soHandle), GfModInfoTerminateFuncName)) == 0)
    {
		/* 2) Backward compatibility (dll name + "Shut") */
		char soName[256];
		const char* lastSlash = strrchr(soPath, '/');
		if (lastSlash)
			strcpy(soName, lastSlash+1);
		else
			strcpy(soName, soPath);
		strcpy(&soName[strlen(soName) - SOFileExtLen], "Shut"); /* cut so file ext */
		fModInfoTerm = (tfModInfoTerminate)dlsym(SOHandle(soHandle), soName);
    }

    /* Call the module termination function if any */
    if (fModInfoTerm)
		termSts = fModInfoTerm();
    
    GfOut("Terminated module %s\n", soPath);

    return termSts;
}

