/**************************************************************************

    file        : rtutil.cpp
    copyright   : (C) 2007 by Mart Kelder                 
    web         : http://speed-dreams.sourceforge.net   

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
    		Robot tools utilities 
    @ingroup	robottools
*/

#include <portability.h>
#include <tgf.h>

#include "robottools.h"

#define BUFFERSIZE 256

void RtGetCarindexString( int index, const char *bot_dname, char extended, char *result, int resultLength )
{
	char buffer[ BUFFERSIZE ];
	void *carnames_xml;

	if( !extended )
	{
		snprintf( result, resultLength, "%d", index );
	}
	else
	{
		snprintf( buffer, BUFFERSIZE, "%sdrivers/curcarnames.xml", GfLocalDir() );
		buffer[ BUFFERSIZE - 1 ] = '\0';
		carnames_xml = GfParmReadFile( buffer, GFPARM_RMODE_STD );
		if( carnames_xml )
		{
			snprintf( buffer, resultLength, "drivers/%s/%d", bot_dname, index );
			result = strncpy( result, GfParmGetStr( carnames_xml, buffer, "car name", "" ), resultLength );
			GfParmReleaseHandle( carnames_xml );
		}
		else
		{
			result[ 0 ] = '\0';
		}
	}

	result[ resultLength - 1 ] = '\0';
}

