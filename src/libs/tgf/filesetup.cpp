/***************************************************************************
                 filesetup.cpp -- Versioned settings XML files installation
                             -------------------                                         
    created              : 2009
    author               : Mart Kelder
    web                  : http://speed-dreams.sourceforge.net   
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
    		Versioned settings XML files installation at run-time
    @author	Mart Kelder
    @ingroup	tgf
*/

#include <cstdio>
#include <cerrno>
#include <sys/stat.h>

#include "tgf.h"
#include "portability.h"


static bool gfFileSetupCopy( char* dataLocation, char* localLocation, int major, int minor, void *localHandle, int count )
{
	bool status;
	
	// Copy the source file to its target place.
	if( !( status = GfFileCopy( dataLocation, localLocation ) ) )
		return status;

	// Update local version.xml file.
	if( localHandle )
	{
		if( count < 0 )
		{
			GfParmSetCurStr( localHandle, "versions", "Data location", dataLocation );
			GfParmSetCurStr( localHandle, "versions", "Local location", localLocation );
			GfParmSetCurNum( localHandle, "versions", "Major version", NULL, (tdble)major );
			GfParmSetCurNum( localHandle, "versions", "Minor version", NULL, (tdble)minor );
		}
		else
		{
			char buf[32];
			snprintf( buf, 30, "versions/%d", count );
			GfParmSetStr( localHandle, buf, "Data location", dataLocation );
			GfParmSetStr( localHandle, buf, "Local location", localLocation );
			GfParmSetNum( localHandle, buf, "Major version", NULL, (tdble)major );
			GfParmSetNum( localHandle, buf, "Minor version", NULL, (tdble)minor );
		}
	}

	return status;
}

void GfFileSetup()
{
	void *dataVersionHandle;
	void *localVersionHandle;
	char *filename;
	size_t filenameLength;
	char *dataLocation;
	char *localLocation;
	char *absLocalLocation;
	char *absDataLocation;
	bool *isIndexUsed;
	int isIndexUsedLen;
	int index;
	bool anyLocalChange, fileFound;
	int major;
	int minor;
	struct stat st;
	const char* pszVersionFileName = "version.xml";
	
	// Try and open version.xml from GfDataDir() .
	filenameLength = strlen(GfDataDir()) + strlen(pszVersionFileName) + 2;
	filename = (char*)malloc( sizeof(char) * filenameLength );
	sprintf( filename, "%s%s", GfDataDir(), pszVersionFileName );
	dataVersionHandle = GfParmReadFile( filename, GFPARM_RMODE_STD );

	// If it failed, let's try GfBinDir() (in case running from build tree, not installed one).
	if( !dataVersionHandle )
	{
		free( filename );
		filenameLength = strlen(GfBinDir()) + strlen(pszVersionFileName) + 2;
		filename = (char*)malloc( sizeof(char) * filenameLength );
		sprintf( filename, "%s%s", GfBinDir(), pszVersionFileName );
		dataVersionHandle = GfParmReadFile( filename, GFPARM_RMODE_STD );
	}
	
	// Exit if version.xml not found.
	if( !dataVersionHandle )
	{
		GfLogWarning("No readable reference %s found ; will not check / update user settings",
					 pszVersionFileName);
		free( filename );
		return;
	}

	// Exit if nothing inside.
	if( GfParmListSeekFirst( dataVersionHandle, "versions" ) != 0 )
	{
		GfLogWarning("%s contains no user settings version info ; will not check / update user settings",
					 filename);
		free( filename );
		GfParmReleaseHandle( dataVersionHandle );
		return;
	}

	// Create LocalDir (user settings root) if not already done.
	GfDirCreate( GfLocalDir() );

	// Open local (user settings) version.xml (create it if not there).
	if( filenameLength < strlen(GfLocalDir()) + 12 )
	{
		free( filename );
		filenameLength = strlen(GfLocalDir()) + strlen(pszVersionFileName) + 2;
		filename = (char*)malloc( sizeof(char) * filenameLength );
	}

	sprintf( filename, "%s%s", GfLocalDir(), pszVersionFileName );
	anyLocalChange = !GfFileExists(filename);
	localVersionHandle = GfParmReadFile( filename, GFPARM_RMODE_CREAT );

	// Exit if open/creation failed.
	if( !localVersionHandle )
	{
		GfLogWarning("%s not found / readable ; will not check / update user settings",
					 filename);
		free( filename );
		GfParmReleaseHandle( dataVersionHandle );
		return;
	}

	// Setup the index of the XML files referenced in the local version.xml.
	isIndexUsedLen = GfParmGetEltNb( localVersionHandle, "versions" )
		             + GfParmGetEltNb( dataVersionHandle, "versions" ) + 2;
	isIndexUsed = (bool*)malloc( sizeof(bool) * isIndexUsedLen );
	for( index = 0; index < isIndexUsedLen; index++ )
		isIndexUsed[index] = false;
	if( GfParmListSeekFirst( localVersionHandle, "versions" ) == 0 )
	{
		do
		{
			index = atoi( GfParmListGetCurEltName( localVersionHandle, "versions" ) );
			if( 0 <= index && index < isIndexUsedLen )
				isIndexUsed[index] = true;
		} while( GfParmListSeekNext( localVersionHandle, "versions" ) == 0 );
	}

	// For each file referenced in the installation version.xml
	do
	{
		fileFound = false;

		// Get its installation path (data), user settings path (local),
		// and new major and minor version numbers
		dataLocation = strdup( GfParmGetCurStr( dataVersionHandle, "versions", "Data location", "" ) );
		localLocation = strdup( GfParmGetCurStr( dataVersionHandle, "versions", "Local location", "" ) );
		major = (int)GfParmGetCurNum( dataVersionHandle, "versions", "Major version", NULL, 0 );
		minor = (int)GfParmGetCurNum( dataVersionHandle, "versions", "Minor version", NULL, 0 );

		absLocalLocation = (char*)malloc( sizeof(char)*(strlen(GfLocalDir())+strlen(localLocation)+3) );
		sprintf( absLocalLocation, "%s%s", GfLocalDir(), localLocation );

		absDataLocation = (char*)malloc( sizeof(char)*(strlen(GfDataDir())+strlen(dataLocation)+3) );
		sprintf( absDataLocation, "%s%s", GfDataDir(), dataLocation );

		GfLogTrace("Checking %s : user settings version ", localLocation);

		// Search for its old major and minor version numbers in the user settings.
		if( GfParmListSeekFirst( localVersionHandle, "versions" ) == 0 )
		{
			do
			{
				if( strcmp( absLocalLocation, GfParmGetCurStr( localVersionHandle, "versions", "Local location", "" ) ) == 0 )
				{
					fileFound = true;
					const int locMinor = (int)GfParmGetCurNum( localVersionHandle, "versions", "Minor version", NULL, 0 );
					const int locMajor = (int)GfParmGetCurNum( localVersionHandle, "versions", "Major version", NULL, 0 );

					GfLogTrace("%d.%d is ", locMajor, locMinor);

					if( locMajor != major || locMinor < minor)
					{
						GfLogTrace("obsolete (installed one is %d.%d) => updating ...\n",
								   major, minor);
						if ( gfFileSetupCopy( absDataLocation, absLocalLocation, major, minor, localVersionHandle, -1 ) )
							anyLocalChange = true;
					}
					else
					{
					    GfLogTrace("up-to-date");
						if (stat(absLocalLocation, &st))
						{
							GfLogTrace(", but not there => installing ...\n");
							if ( gfFileSetupCopy( absDataLocation, absLocalLocation, major, minor, localVersionHandle, -1 ) )
								anyLocalChange = true;
						}
						else
							GfLogTrace(".\n");
					}
					
					break;
				}
			} while( GfParmListSeekNext( localVersionHandle, "versions" ) == 0 );
		}

		if( !fileFound)
		{
			index = 0;
			while( isIndexUsed[index] )
				++index;
			GfLogTrace("not found => installing ...\n");
			if ( gfFileSetupCopy( absDataLocation, absLocalLocation, major, minor, localVersionHandle, index ) )
				anyLocalChange = true;
			isIndexUsed[index] = true;
		}

		free( dataLocation );
		free( localLocation );
		free( absDataLocation );
		free( absLocalLocation );

	} while( GfParmListSeekNext( dataVersionHandle, "versions" ) == 0 );

	// Write the user settings version.xml if changed.
	if (anyLocalChange)
		GfParmWriteFile( NULL, localVersionHandle, "versions" );

	GfParmReleaseHandle( localVersionHandle );
	GfParmReleaseHandle( dataVersionHandle );
	free( isIndexUsed );
	free( filename );
}

