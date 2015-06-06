/***************************************************************************
                      legacymodule.cpp -- Legacy dynamic module management                                
                             -------------------                                         
    created              : Fri Aug 13 22:25:53 CEST 1999
    copyright            : (C) 1999 by Eric Espie                         
    email                : torcs@free.fr   
    version              : $Id: legacymodule.cpp 3453 2011-03-20 08:50:13Z pouillot $                                  
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
    		Legacy dynamic module management.
    		This is the interface to load/unload the shared libraries (or DLLs).
			It is only kept for supporting the robot modules which are implemented
			the old TORCS way (or the "welcome" way introduced in the early Speed Dreams)
		<br>Two modes are allowed, the access by filename, of the access by entire directory.
		<br>When the directory mode is used, the filenames are not known by advance, this
		<br>allow more flexibility at runtime.
		<br>
		<br>The generic information can be retrieved, without keeping the DLL loaded, through GfModInfo and GfModInfoDir.
		<br>
		<br>The gfid parameter is used to differentiate the modules using different includes.
		<br>This functionality is not used yet.
		<br>
		<br>Loaded module information is stored in a linked list in the following way :
		<br>- the list can be empty at the beginning, but this is not needed,
		<br>- GfModInfo and GfModLoad add loaded module info at the head of the list,
		<br>  in order to have easy access to this info on return
		<br>- GfModInfoDir and GfModLoadDir keep the list sorted by module priority
		<br>- For a given list, if a DLL is requested to be loaded multiple times,
		<br>  it will be loaded only once, unless different path-names
		<br>  are used each time (ex: with absolute and the relative path-name)
		<br>
		<br>The process of "loading a module" named "mod" includes the following actions :
		<br>- load the associated DLL
		<br>- ask the DLL if it holds a "modMaxNbItf" entry, and call it if yes,
		<br>  to get the maximum number of interfaces of the module (default = 10)
		<br>- allocate the array of "module interface info" structures with that size
		<br>- call the (mandatory) module entry "mod" with such allocated array as a parameter,
		<br>  in order for the module to initialize its internal data
		<br>
		<br>This API is not used for shared libraries linked statically at compilation time.
    @author	<a href=mailto:torcs@free.fr>Eric Espie</a>
    @version	$Id: legacymodule.cpp 3453 2011-03-20 08:50:13Z pouillot $
    @ingroup	module
*/
#include "tgf.h"
#include "os.h"

void
gfModInit(void)
{
}

/** Load the specified DLL.
    @ingroup	module
    @param	gfid	Mask for version checking
    @param	dllname	File name of the DLL
    @param	modlist	List of module description structure where to add loaded module info
    @return	>=0 Number of modules loaded
		<br>-1 Error
    @warning	The loaded module info is added/moved to the head of modlist
    @see	tModList
 */
int
GfModLoad(unsigned int gfid, const char *dllname, tModList **modlist)
{
    if (GfOs.modLoad) {
	return GfOs.modLoad(gfid, dllname, modlist);
    } else {
	return -1;
    }
}

/** Load the DLLs in the specified directory.
    @ingroup	module
    @param	gfid	Mask for version checking
    @param	dir	Directory name where to find the DLLs
    @param	modlist	List of module description structure where to add loaded modules info
    @return	>=0 Number of modules loaded
		<br>-1 Error
    @warning	modlist is kept sorted by module priority
 */
int
GfModLoadDir(unsigned int gfid, const char *dir, tModList **modlist)
{
    if (GfOs.modLoadDir) {
	return GfOs.modLoadDir(gfid, dir, modlist);
    } else {
	return -1;
    }
}

/** Unload the DLLs of a list.
    @ingroup	module
    @param	modlist	List of DLLs to unload
    @return	0 Ok
		<br>-1 Error
 */
int
GfModUnloadList(tModList **modlist)
{
    if (GfOs.modUnloadList) {
	return GfOs.modUnloadList(modlist);
    } else {
	return -1;
    }
}

/** Get the generic information of the specified DLL (unload the DLL afterwards).
    @ingroup	module
    @param	gfid	Mask for version control
    @param	dllname	File name of the DLL
    @param	modlist	List of module description structure where to add loaded modules info
    @return	>=0	Number of modules infoed
		<br>-1 Error
    @warning	The loaded module info is added/moved to the head of modlist
 */
int
GfModInfo(unsigned int gfid, const char *dllname, tModList **modlist)
{
    if (GfOs.modInfo) {
	return GfOs.modInfo(gfid, dllname, modlist);
    } else {
	return -1;
    }
}

/** Get the generic module information of the DLLs of the specified directory (unload the DLLs afterwards).
    @ingroup	module
    @param	gfid	Mask for version checking
    @param	dir	Directory name where to find the DLLs
    @param	level	if 0, load dir/ *.so/dll ; if 1, load dir/(subdir)/(subdir).so/dll for any (subdir)
    @param	modlist	List of module description structure where to add loaded modules info
    @return	>=0	Number of modules infoed
		<br>-1 Error
    @warning	modlist is kept sorted by module priority
 */
int
GfModInfoDir(unsigned int gfid, const char *dir, int level, tModList **modlist)
{
    if (GfOs.modInfoDir) {
	return GfOs.modInfoDir(gfid, dir, level, modlist);
    } else {
	return -1;
    }
}
