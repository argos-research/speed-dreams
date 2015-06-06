/***************************************************************************
                        file.cpp -- directory management                       
                             -------------------                                         
    created              : Thu Oct 12 21:58:55 CEST 2010
    copyright            : (C) 2010 by Mart Kelder, Jean-Philippe Meuret
    web                  : http://www.speed-reams.org
    version              : $Id: file.cpp 5043 2012-11-11 19:38:09Z pouillot $
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
    		This is used for file manipulations.
    @author	Mart Kelder, Jean-Philippe Meuret
    @version	$Id: file.cpp 5043 2012-11-11 19:38:09Z pouillot $
    @ingroup	file
*/

#include <cstdio>
#include <cerrno>
#include <sys/stat.h>

#ifdef WIN32
#include <io.h>
#endif

#include <portability.h>

#include "tgf.h"


/** Get the path-name of the directory containing a file
    @ingroup	file
    @param	pszFileName	path-name of the file
    @return	Path-name of the directory, allocated on the heap (the caller must free it).
 */
char* GfFileGetDirName(const char* pszFileName)
{
	// Allocate and initialize the target string
	char* pszDirName = strdup(pszFileName);

	// Replace '\\' by '/' under Windows
#ifdef WIN32
	for (int i = 0; pszDirName[i]; i++)
		if (pszDirName[i] == '\\')
			pszDirName[i] = '/';
#endif

	// Search for the last '/'.
	char *lastSlash = strrchr(pszDirName, '/');

	// If found, we've got the end of the directory name.
	if (lastSlash)
	{
		// But keep the '/' if it is the first one of an absolute path-name.
		if (lastSlash != pszDirName)
#ifdef WIN32
			if (*(lastSlash-1) != ':')
#endif
				*lastSlash = '\0';
	}

	// If no '/' found, empty directory name.
	else
		*pszDirName = '\0';

	//GfLogDebug("GfFileGetDirName(%s) = %s\n", pszFileName, pszDirName);
	
	// That's all.
	return pszDirName;
}

/** Check if a file exists
    @ingroup	file
    @param	pszName	Path-name of the file
    @return	true if the file exists, false otherwise.
 */
bool GfFileExists(const char* pszName)
{
	struct stat st;
	return stat(pszName, &st) ? false : true;
}

/** Copy a file to a target path-name
    @ingroup	file
    @param	pszSrcName	Source file path-name
    @param	pszSrcName	Target file path-name for the copy
    @return	true upon success, false otherwise.
 */
bool GfFileCopy(const char* pszSrcName, const char* pszTgtName)
{
	static const size_t maxBufSize = 1024;
	char buf[maxBufSize];
	FILE *in;
	FILE *out;
	size_t size;
	size_t writeSize;
	int errnum;
	bool res = true;
	
	// Create the target local directory (and parents) if not already done
	// (first, we have to deduce its path from the target file path-name).
	// TODO: Use GfFileGetDirName
	strncpy(buf, pszTgtName, strlen(pszTgtName)+1);
#ifdef WIN32
	for (size_t i = 0; i < maxBufSize && buf[i] != '\0'; i++)
		if (buf[i] == '\\')
			buf[i] = '/';
#endif
	char *lastSlash = strrchr(buf, '/');
	if (lastSlash)
	{
	  *lastSlash = '\0';
	  GfDirCreate( buf );
	}

	// Set target file access attributes to "read/write for the current user",
	// if the target file exists (quite paranoid, but sometimes needed).
	struct stat st;
	if (! stat(pszTgtName, &st) && chmod( pszTgtName, 0640 ))
	{
		const int errnum = errno; // Get errno before it is overwritten by some system call.
		GfLogWarning("Failed to set 0640 attributes to %s (%s)\n",
					 pszTgtName, strerror(errnum));
	}

	// Open the source and the target file.
	if( ( in = fopen( pszSrcName, "rb" ) ) == NULL )
	{
		errnum = errno; // Get errno before it is overwritten by some system call.
		GfLogError("Could not open %s in 'rb' mode when copying it to %s (%s).\n",
				   pszSrcName, pszTgtName, strerror(errnum));
		return false;
	}
	if( ( out = fopen( pszTgtName, "wb" ) ) == NULL )
	{
		errnum = errno; // Get errno before it is overwritten by some system call.
		GfLogError("Could not open %s in 'wb' mode when creating it from %s (%s).\n",
				   pszTgtName, pszSrcName, strerror(errnum));
		fclose( in );
		return false;
	}

	// Do the byte to byte copy.
	GfLogDebug("Copying %s to %s\n", pszSrcName, pszTgtName);

	while( !feof( in ) )
	{
		size = fread( buf, 1, 1024, in );
		if( size > 0 )
		{
			writeSize = fwrite( buf, 1, size, out );
			if( ferror( out ) )
			{
				errnum = errno; // Get errno before it is overwritten by some system call.
				GfLogError("Failed to write data to %s when creating it from %s (%s).\n",
						   pszTgtName, pszSrcName, strerror(errnum));
				res = false;
				break;
			}
			else if( size != writeSize )
			{
				GfLogError("Failed to write all data to %s when creating it from %s.\n", pszTgtName, pszSrcName );
				res = false;
				break;
			}
		}
		else if( ferror( in ) )
		{
			errnum = errno; // Get errno before it is overwritten by some system call.
			GfLogError("Failed to read data from %s when copying it to %s (%s).\n",
					   pszSrcName, pszTgtName, strerror(errnum));
			res = false;
			break;
		}
	}

	fclose( in );
	fclose( out );

	// Set target file access attributes to "read/write for the current user".
	if (chmod( pszTgtName, 0640 ))
	{
		const int errnum = errno; // Get errno before it is overwritten by some system call.
		GfLogWarning("Failed to set 0640 attributes to %s (%s)\n",
					 pszTgtName, strerror(errnum));
	}

	return res;
}

