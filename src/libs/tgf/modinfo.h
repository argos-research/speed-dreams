/***************************************************************************
                   modinfo.h -- Tools for module interface management

    created              : Fri Aug 13 22:32:14 CEST 1999
    copyright            : (C) 2008 by Jean-Philippe Meuret                         
    email                : jpmeuret@free.fr
    version              : $Id: modinfo.h 3893 2011-09-18 15:52:42Z pouillot $
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
    	Tools for module interface management.
    @author	<a href=mailto:jpmeuret@free.fr>Jean-Philippe Meuret</a>
    @version	$Id: modinfo.h 3893 2011-09-18 15:52:42Z pouillot $
*/


#ifndef __MODINFO__H__
#define __MODINFO__H__

/** Maximum number of interfaces in one "legacy" module 
    (no limit for "Unlimited Number of Interfaces" modules)
    @see	ModList
 */
#define MAX_MOD_ITF 10
#define GfModInfoDefaultMaxItf MAX_MOD_ITF

/* Name of the module function where run-time informations are exchanged (new scheme) 
   This function is called before moduleInitialize if present */
#define GfModInfoWelcomeFuncName "moduleWelcome"

/* Name of the module function entry point (new scheme) */
#define GfModInfoInitializeFuncName "moduleInitialize"

/* Name of the module function exit point (new scheme) */
#define GfModInfoTerminateFuncName "moduleTerminate"

/** Welcome information that a module receives at load time */
typedef struct ModWelcomeIn {
    unsigned int         itfVerMajor;   /**< Major version of the module interface scheme */
    unsigned int         itfVerMinor;   /**< Minor version of the module interface scheme */
    const char		*name;		/**< name = identifier of the module */
} tModWelcomeIn;

/** Welcome information that a module gives back at load time */
typedef struct ModWelcomeOut {
    unsigned int	maxNbItf;	/**< Max number of interfaces */
} tModWelcomeOut;

/** initialisation of the function table 
    @see	ModInfo
*/
typedef int (*tfModPrivInit)(int index, void *);

/** Module interface information structure */
typedef struct ModInfo {
    const char		*name;		/**< name of the module (short) (NULL if no module) */
    const char		*desc;		/**< description of the module (can be long) */
    tfModPrivInit	fctInit;	/**< init function */
    unsigned int	gfId;		/**< supported framework version */
    int			index;		/**< index if multiple interface in one dll */
    int			prio;		/**< priority if needed */
    int			magic;		/**< magic number for integrity check */
} tModInfo;

/** Internal module interface information structure (see GfModInfoDuplicate) 
    WARNING: Must have the same fields ; only const-ness may differ */
typedef struct ModInfoNC {
    char		*name;		/**< name of the module (short) (NULL if no module) */
    char		*desc;		/**< description of the module (can be long) */
    tfModPrivInit	fctInit;	/**< init function */
    unsigned int	gfId;		/**< supported framework version */
    int			index;		/**< index if multiple interface in one dll */
    int			prio;		/**< priority if needed */
    int			magic;		/**< magic number for integrity check */
} tModInfoNC;

/** Shared library handle type */
typedef void* tSOHandle;

#ifdef WIN32
#define SOHandle(h) ((HMODULE)(h))
#else
#define SOHandle(h) ((void*)(h))
#endif


/** list of module interfaces */
typedef struct ModList {
    int			modInfoSize;	/**< module max nb of interfaces */
    tModInfoNC		*modInfo;	/**< module interfaces info array, full or 0 terminated */
    tSOHandle		handle;		/**< handle of loaded shared lib */
    char		*sopath;	/**< path name of shared lib file */
    struct ModList	*next;		/**< next module in list */
} tModList;


/* Interface of module function where run-time informations are exchanged 
   This function is called before moduleInitialize if present */
typedef int (*tfModInfoWelcome)(const tModWelcomeIn*, tModWelcomeOut*);

/* Interface of module initialization function */
typedef int (*tfModInfoInitialize)(tModInfo *);  /* second/first function called in the module */

/* Interface of module termination function */
typedef int (*tfModInfoTerminate)(void);	/* last function called in the module */


/********************************************
 * Tools for Dynamic Modules initialization *
 ********************************************/

/* Allocate the module interfaces info array */
TGF_API tModInfo *GfModInfoAllocate(int maxItf);

/* Free the module interfaces info array */
TGF_API void GfModInfoFree(tModInfo *array);

/* Free the module interfaces info array */
TGF_API void GfModInfoFreeNC(tModInfoNC *array, int maxItf);

/* Duplicate a module interfaces info array from a const one to a non-const one */
TGF_API tModInfoNC* GfModInfoDuplicate(const tModInfo *source, int maxItf);

/* Initialize the module with given handle and library file path */
TGF_API int GfModInitialize(tSOHandle soHandle, const char *soPath, 
							unsigned int gfid, tModList **mod);

/* Terminate the module with given handle and library file path */
TGF_API int GfModTerminate(tSOHandle soHandle, const char *soPath);

#endif /* __MODINFO__H__ */
