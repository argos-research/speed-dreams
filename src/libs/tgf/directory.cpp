/***************************************************************************
                        directory.cpp -- directory management                       
                             -------------------                                         
    created              : Fri Aug 13 21:58:55 CEST 1999
    copyright            : (C) 1999 by                          
    email                : torcs@free.fr   
    version              : $Id: directory.cpp 5356 2013-03-24 12:42:47Z pouillot $                                  
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
    		This is used for directory manipulation.
    @author	<a href=mailto:torcs@free.fr>Eric Espie</a>
    @version	$Id: directory.cpp 5356 2013-03-24 12:42:47Z pouillot $
    @ingroup	dir
*/

#include <cstdlib>
#include <cerrno>
#include <sys/stat.h>

#include <portability.h>

#include "tgf.h"
#include "os.h"

void
gfDirInit(void)
{
}


/** Get the list of files of a given directory
    @ingroup	dir
    @param	dir	directory name
    @return	The list of files
 */
tFList * GfDirGetList(const char *dir)
{
	if (GfOs.dirGetList) {
		return GfOs.dirGetList(dir);
	} else {
		return (tFList*)NULL;
	}
}


/** Get the list of files of a given directory
    @ingroup	dir
    @param	dir	directory name
    @param	prefix	filtering prefix for file names
    @param	suffix	filtering suffix for file name
    @return	The list of files
 */
tFList * GfDirGetListFiltered(const char *dir, const char *prefix, const char *suffix)
{
	if (GfOs.dirGetListFiltered) {
		return GfOs.dirGetListFiltered(dir, prefix, suffix);
	} else {
		return (tFList*)NULL;
	}
}

/** Free a directory list
    @ingroup	dir
    @param	list	List of files
    @param	freeUserData	User function used to free the user data
    @return	none
*/
void GfDirFreeList(tFList *list, tfDirfreeUserData freeUserData, bool freeName, bool freeDispName)
{
	if (list) {
		// The list contains at least one element, checked above.
		tFList *rl = list;
		do {
			tFList *tmp = rl;
			rl = rl->next;
			if (freeUserData && tmp->userData) {
				freeUserData(tmp->userData);
			}
			if (freeName) {
				freez(tmp->name);
			}
			if (freeDispName) {
				freez(tmp->dispName);
			}
			free(tmp);
		} while (rl != list);
	}

	list = NULL;
}

/** Check if a directory exists
    @ingroup	dir
    @param	pszName	Path-name of the directory
    @return	true if the directory exists, false otherwise.
 */
bool GfDirExists(const char* pszName)
{
	if (!pszName || strlen(pszName) == 0)
		return false;
	
	struct stat st;

#ifdef WIN32
	if (pszName[strlen(pszName)-1] == '/' || pszName[strlen(pszName)-1] == '\\')
	{
		// Windows stat() does not supports traling (anti-)slashes ... no comment please.
		// TODO: Take of the possible _multiple_ trailing (anti-)slashes ...
		char* pszNameNoTrailSlash = strdup(pszName);
		pszNameNoTrailSlash[strlen(pszName)-1] = 0;
		const bool bAnswer = (stat(pszNameNoTrailSlash, &st) ? false : true);
		free(pszNameNoTrailSlash);
		return bAnswer;
	}
	else
#endif
		return stat(pszName, &st) ? false : true;
}

/** Create a directory and the parents if needed
    @ingroup	dir
    @param	dir	full directory path-name
    @return	GF_DIR_CREATED on success, GF_DIR_CREATION_FAILED otherwise.
 */
int GfDirCreate(const char *path)
{
	if (path == NULL) {
		return GF_DIR_CREATION_FAILED;
	}

	static const int nPathBufSize = 1024;
	char buf[nPathBufSize];
	strncpy(buf, path, nPathBufSize);

#ifdef WIN32

	// Translate path.
	static const char cPathSeparator = '\\';
	int i;
	for (i = 0; i < nPathBufSize && buf[i] != '\0'; i++)
		if (buf[i] == '/')
			buf[i] = cPathSeparator;
	
#else // WIN32

// mkdir with u+rwx access grants by default
#ifdef mkdir
# undef mkdir
#endif
#define mkdir(x) mkdir((x), S_IRWXU)

	static const char cPathSeparator = '/';

#endif // WIN32

	// Try to create the requested folder.
	int err = mkdir(buf);

	// If this fails, try and create the parent one (recursive), and the retry.
	if (err == -1 && errno == ENOENT)
	{
		// Try the parent one (recursive).
		char *end = strrchr(buf, cPathSeparator);
		*end = '\0';
		GfDirCreate(buf);

		// Retry.
		*end = cPathSeparator;
		err = mkdir(buf);
	}
	
	return (err == -1 && errno != EEXIST) ? GF_DIR_CREATION_FAILED : GF_DIR_CREATED;
}

