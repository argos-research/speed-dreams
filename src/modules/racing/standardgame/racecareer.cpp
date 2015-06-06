/**************************************************************************

    file        : racecareer.cpp
    copyright   : (C) 2007 by Mart Kelder                 
    web         : http://speed-dreams.sourceforge.net   
    version     : $Id: racecareer.cpp 5353 2013-03-24 10:26:22Z pouillot $

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
    		Career race management 
*/

#include <ctime>

#include <portability.h>
#include <tgf.h>

#include <raceman.h>
#include <robot.h>

#include "racesituation.h"
#include "racecareer.h"


static char buf[1024];

typedef struct DriverInfo
{
	char *module;
	int extended;
	int idx;
	char *name;
	double skill;
	double *classPointList;
	double sortValue;
} tDriverInfo;

typedef struct TeamInfo
{
	char *name;
	char *car_dname;
	int nbDrivers;
	int curDriver;
	double sortPoints;
	tDriverInfo **drivers;
} tTeamInfo;

typedef struct GroupInfo
{
	int nbDrivers;
	int nbTeams;
	int curTeam;
	tTeamInfo *teams;
} tGroupInfo;

typedef struct ClassInfo
{
	int nbGroups;
	char *suffix;
	tGroupInfo *groups;
} tClassInfo;

typedef struct CareerInfo
{
	int nbClasses;
	tClassInfo *classes;
} tCareerInfo;

static int ReCareerUtilRand( int min, int max )
{
	return min + (int)floor( (double)( max - min + 1 ) * (double)( rand() / ( RAND_MAX + 1.0f ) ) );
}

static void ReCareerUtilManipString( char* string, int number, int bufLength )
{
	int numLength = 0;
	int num;
	int xx;
	int curPos = 0;

	while( string[curPos] != '\0' ) {
		if (string[curPos] != '%') {
			++curPos;
			continue;
		}
		switch (string[curPos + 1]) {
		case '%':
			memmove( &(string[curPos]), &(string[curPos+1]), ( bufLength - curPos - 1 ) * sizeof( char ) );
			++curPos;
			break;
		case 'A':
			memmove( &(string[curPos]), &(string[curPos+1]), ( bufLength - curPos - 1 ) * sizeof( char ) );
			string[curPos] = (char)( 'A' + number );
			++curPos;
			break;
		case 'a':
			memmove( &(string[curPos]), &(string[curPos+1]), ( bufLength - curPos - 1 ) * sizeof( char ) );
			string[curPos] = (char)( 'a' + number );
			++curPos;
			break;
		case '1':
			num = number + 1;
			numLength = 1;
			while( num >= 10 ) {
				num /= 10;
				++numLength;
			}

			num = number + 1;
			if( curPos + numLength < bufLength ) {
				memmove( &(string[curPos + numLength]), &(string[curPos + 2]), (bufLength - curPos - numLength) * sizeof( char ) );
				for( xx = numLength - 1; xx >= 0; --xx ) {
					string[ curPos + xx ] = (char)( '0' + ( num % 10 ) );
					num /= 10;
				}
				string[ bufLength - 1 ] = '\0';
				curPos += numLength;
			} else {
				string[ curPos ] = '.';
				string[ curPos + 1 ] = '.';
				curPos += 2;
			}
			break;
		}
	}
}

static void ReCareerNewResultXml( const char* filename, double date )
{
	void *results;

	snprintf( buf, 1024, filename, "results", "", "", "", "" );
	ReInfo->results = GfParmReadFile( buf, GFPARM_RMODE_STD | GFPARM_RMODE_CREAT );
	results = ReInfo->results;

	GfParmSetNum( results, RE_SECT_HEADER, RE_ATTR_DATE, NULL, (tdble)date );
	GfParmSetNum( results, RE_SECT_CURRENT, RE_ATTR_CUR_SEASON, NULL, 0 );
	GfParmWriteFile( NULL, results, NULL );
	ReInfo->mainResults = ReInfo->results;
}

static void ReCareerNewResultXmlSetFirst( char *firstParamFile )
{
	GfParmSetStr( ReInfo->mainResults, RE_SECT_CURRENT, RE_ATTR_CUR_FILE, firstParamFile );
	GfParmWriteFile( NULL, ReInfo->mainResults, NULL );
}

static void ReCareerNewResult( const char* filename, const char *suffix, const char *group, void *subparam, int nbDrivers )
{
	void *results;

	snprintf( buf, 1024, filename, "results", "_", suffix, group, "s" );
	results = GfParmReadFile( buf, GFPARM_RMODE_STD | GFPARM_RMODE_CREAT );

	GfParmSetStr(subparam, RM_SECT_SUBFILES, RM_ATTR_RESULTSUBFILE, buf);
	GfParmSetNum(results, RE_SECT_CURRENT, RE_ATTR_CUR_TRACK, NULL, 1);
	GfParmSetNum(results, RE_SECT_CURRENT, RE_ATTR_CUR_RACE, NULL, 1);
	GfParmSetNum(results, RE_SECT_CURRENT, RE_ATTR_CUR_DRIVER, NULL, 1);

	GfParmSetNum(results, RE_SECT_DRIVERS, RM_ATTR_MINNUM, NULL, (tdble)nbDrivers );
	GfParmSetNum(results, RE_SECT_DRIVERS, RM_ATTR_MAXNUM, NULL, (tdble)nbDrivers );

	GfParmWriteFile( NULL, results, NULL );
	GfParmReleaseHandle( results );
}

static void ReCareerInitDrivers( void *param )
{
	int nb;
	int rand_var;
	int xx;

	nb = GfParmGetEltNb( param, RM_SECT_FIRSTNAME );
	rand_var = (int)( (double)nb * ( (double)rand() / (double)((unsigned int)RAND_MAX + 1) ) );
	if( GfParmListSeekFirst( param, RM_SECT_FIRSTNAME ) == 0 )
		for( xx = 0; xx < rand_var && GfParmListSeekNext( param, RM_SECT_FIRSTNAME ) == 0; ++xx ); /* ; here intended */

	nb = GfParmGetEltNb( param, RM_SECT_LASTNAME );
	rand_var = (int)( (double)nb * ( (double)rand() / (double)((unsigned int)RAND_MAX + 1) ) );
	if( GfParmListSeekFirst( param, RM_SECT_LASTNAME ) == 0 )
		for( xx = 0; xx < rand_var && GfParmListSeekNext( param, RM_SECT_LASTNAME ) == 0; ++xx ); /* ; here intended */
}

static double ReCareerNewSkill( int nbGroups, int groupNb )
{
	double min = 9.0f * (double)(MAX(0, nbGroups - groupNb - 1))/(double)(MAX(1, nbGroups));
	double max = MIN( 10.0f, 11.0f * (double)(MAX(1, nbGroups - groupNb))/(double)(MAX(1,nbGroups)) );

	return min + (double)( max - min ) * (double)( rand() / ( RAND_MAX + 1.0f ) );
}

static void ReCareerNewAddDrivers( void *curParam, void *curResult, char *humans, int classNb )
{
	int xx;
	char *path2;
	int drivers;

	drivers = (int)GfParmGetNum( curResult, RM_SECT_DRIVERS, RM_ATTR_MAXNUM, NULL, 10 );
	GfParmListClean( curParam, RM_SECT_DRIVERS );

	GfLogDebug("ReCareerNewAddDrivers: %d drivers, with%s humans ...\n", drivers, *humans ? "" : "out");

	for (xx = 0; xx < drivers; ++xx) {
		if( *humans ) {
			do {
				if( strcmp( GfParmGetCurStr( ReInfo->params, RM_SECT_DRIVERS, RM_ATTR_MODULE, "" ), "human" ) == 0 )
					break;
				if( GfParmListSeekNext( ReInfo->params, RM_SECT_DRIVERS ) != 0 ) {
					*humans = FALSE;
					break;
				}
			} while( true );

			if( *humans ) {
				/* Current one is a human */
				GfLogDebug("  %d : human #%d, ext=%d\n", xx,
						   (int)GfParmGetCurNum(ReInfo->params, RM_SECT_DRIVERS, RM_ATTR_IDX, NULL, 1),
						   (int)GfParmGetCurNum(ReInfo->params, RM_SECT_DRIVERS, RM_ATTR_EXTENDED, NULL, 1));
				sprintf( buf, "%s/%d", RM_SECT_DRIVERS, xx + 1 );
				GfParmSetStr(curParam, buf, RM_ATTR_MODULE, "human");
				GfParmSetNum(curParam, buf, RM_ATTR_IDX, NULL, GfParmGetCurNum(ReInfo->params, RM_SECT_DRIVERS, RM_ATTR_IDX, NULL, 1 ) );
				GfParmSetNum(curParam, buf, RM_ATTR_EXTENDED, NULL, GfParmGetCurNum(ReInfo->params, RM_SECT_DRIVERS,
				                                                                    RM_ATTR_EXTENDED, NULL, 1) );
				sprintf( buf, "%s/%s/%d/%d/%s", RM_SECT_CLASSPOINTS, "human",
				                                (int)GfParmGetCurNum(ReInfo->params, RM_SECT_DRIVERS, RM_ATTR_EXTENDED, NULL, 1),
								(int)GfParmGetCurNum(ReInfo->params, RM_SECT_DRIVERS, RM_ATTR_IDX, NULL, 1 ),
								GfParmGetStr( curParam, RM_SECT_SUBFILES, RM_ATTR_SUFFIX, "" ) );
				GfParmSetNum( curResult, buf, RE_ATTR_POINTS, NULL,
				              0.05f * GfParmGetCurNum(ReInfo->params, RM_SECT_DRIVERS, RM_ATTR_IDX, NULL, 1) );

				if( GfParmListSeekNext( ReInfo->params, RM_SECT_DRIVERS ) != 0 )
					*humans = FALSE;
				continue; /* Goto next xx */
			}
		}

		/* Now it is certain that a bot should be added : no humans at this point */
		GfLogDebug("  %d : simplix #%d, ext=%d\n", xx, xx, 1);
		sprintf( buf, "%s/%d", RM_SECT_DRIVERS, xx + 1 );
		path2 = strdup( buf );
		GfParmSetStr(curParam, path2, RM_ATTR_MODULE, "simplix");
		GfParmSetNum(curParam, path2, RM_ATTR_IDX, NULL, (tdble)xx);
		GfParmSetNum(curParam, path2, RM_ATTR_EXTENDED, NULL, 1);
		free( path2 );
		sprintf( buf, "%s/%s/%d/%d", RM_SECT_DRIVERINFO, "simplix", 1, xx );
		path2 = strdup( buf );
		snprintf( buf, 1024, "%s %s", GfParmGetCurStr(ReInfo->params, RM_SECT_FIRSTNAME, RM_ATTR_NAME, "Foo"),
		                              GfParmGetCurStr(ReInfo->params, RM_SECT_LASTNAME, RM_ATTR_NAME, "Bar") );
		GfParmSetStr(curParam, path2, RM_ATTR_NAME, buf);
		if( GfParmListSeekNext(ReInfo->params, RM_SECT_FIRSTNAME) != 0 )
			GfParmListSeekFirst(ReInfo->params, RM_SECT_FIRSTNAME);
		if( GfParmListSeekNext(ReInfo->params, RM_SECT_LASTNAME) != 0 )
			GfParmListSeekFirst(ReInfo->params, RM_SECT_LASTNAME);
		GfParmSetNum(curParam, path2, RM_ATTR_SKILLLEVEL, NULL, (tdble) ReCareerNewSkill( GfParmGetEltNb( ReInfo->params, RM_SECT_CLASSES ), classNb ) );
	
		/* Add a driver to the result section */
		snprintf( buf, 1024, "%s/%s/%d/%d/%s", RE_SECT_CLASSPOINTS, "simplix", 1, xx, GfParmGetStr( curParam, RM_SECT_SUBFILES, RM_ATTR_SUFFIX, "" ) );
		GfParmSetNum( curResult, buf, RE_ATTR_POINTS, NULL, 1.0f );

		free(path2);
	}
}

static void ReCareerNewAddTeams( void *curParam, void *curResult, int curIndex, int nbGroups )
{
	int start = 0;
	int end = 0;
	int cur = 0;

	cur = GfParmGetEltNb( curParam, RM_SECT_TEAMS );
	if( cur == 0 )
		return;
	end = cur / nbGroups + ( ( cur % nbGroups ) > curIndex ? 1 : 0 );
	start = curIndex * ( cur / nbGroups ) + ( ( cur % nbGroups ) > curIndex ? curIndex : ( cur % nbGroups ) );
	end += start;
	cur = 0;

	GfParmListClean( curResult, RE_SECT_TEAMINFO );
	GfParmListSeekFirst( curParam, RM_SECT_TEAMS );
	do {
		if( cur >= start ) {
			snprintf( buf, 1024, "%s/%s", RE_SECT_TEAMINFO, GfParmGetCurStr( curParam, RM_SECT_TEAMS, RM_ATTR_NAME, "" ) );
			GfParmSetNum( curResult, buf, RE_ATTR_POINTS, NULL, 1.0f );
			if( GfParmIsFormula( curParam, RM_SECT_TEAMS, RM_ATTR_CARNAME ) == 0 )
				GfParmSetFormula( curResult, buf, RM_ATTR_CARNAME, GfParmGetCurFormula( curParam, RM_SECT_TEAMS, RM_ATTR_CARNAME ) );
			else
				GfParmSetStr( curResult, buf, RM_ATTR_CARNAME, GfParmGetCurStr( curParam, RM_SECT_TEAMS, RM_ATTR_CARNAME, "" ) );
		}
		++cur;
		GfParmListSeekNext( curParam, RM_SECT_TEAMS );
	} while( cur < end );

	GfParmListClean( curParam, RM_SECT_TEAMS );
	
}

static void ReCareerNewDrivers()
{
	void *curParam;
	void *curResult;
	void *tmp;
	int nbGroups;
	char humans;
	int xx;
	int classNb = 0;

	curParam = GfParmReadFile( GfParmGetStr( ReInfo->mainResults, RE_SECT_CURRENT, RE_ATTR_CUR_FILE, "" ), GFPARM_RMODE_STD );
	if( !curParam ) {
		GfLogError( "ReCareerNewDrivers: Could not open main Params\n" );
		return;
	}
	if( GfParmListSeekFirst(ReInfo->params, RM_SECT_DRIVERS) == 0 )
		humans = TRUE;
	else
		humans = FALSE;
	GfLogDebug("ReCareerNewDrivers: with%s humans (%s)\n", humans ? "" : "out", GfParmGetFileName(ReInfo->params));
	GfLogDebug("ReCareerNewDrivers: curParam=%s\n", GfParmGetFileName(curParam));

	if( GfParmListSeekFirst(ReInfo->params, RM_SECT_CLASSES) == 0 ) {
		do {
			nbGroups = (int)GfParmGetCurNum(ReInfo->params, RM_SECT_CLASSES, RM_ATTR_NBGROUPS, NULL, 1);
			GfLogDebug("ReCareerNewDrivers: class %s : %d groups\n",
					   GfParmGetCurStr(ReInfo->params, RM_SECT_CLASSES, RM_ATTR_SUBFILE_SUFFIX, "???"), nbGroups);
			for( xx  = 0; xx < nbGroups; ++xx ) {
				curResult = GfParmReadFile( GfParmGetStr( curParam, RM_SECT_SUBFILES, RM_ATTR_RESULTSUBFILE, "" ), GFPARM_RMODE_STD );
				if( !curResult )
				{
					GfLogError( "ReCareerNewDrivers: Could not read a subfile\n" );
				} else {
					GfLogDebug("ReCareerNewDrivers: group %d : curResult=%s\n", xx, GfParmGetFileName(curResult));
					ReCareerNewAddDrivers( curParam, curResult, &humans, classNb );
					ReCareerNewAddTeams( curParam, curResult, xx, nbGroups );
				}
				tmp = GfParmReadFile( GfParmGetStr( curParam, RM_SECT_SUBFILES, RM_ATTR_NEXTSUBFILE, "" ), GFPARM_RMODE_STD );
				if( !tmp ) {
					GfLogError( "ReCareerNewDrivers: Could not read next subparam\n" );
					break;
				}
				GfParmWriteFile( NULL, curResult, NULL );
				GfParmWriteFile( NULL, curParam, NULL );
				GfParmReleaseHandle( curResult );
				GfParmReleaseHandle( curParam );
				curParam = tmp;
			}
			++classNb;
		} while( GfParmListSeekNext(ReInfo->params, RM_SECT_CLASSES) == 0 );
	}
}

static void* ReCareerNewGroup( const char *filename, void *param, const char *groupAlpha, int drivers, int totalTracks, int groupNumber )
{
	void *subparam;
	char const *suffix;
	char *params_filename;

	/* Save the params in a new file and open it */
	snprintf( buf, 1024, filename, "params", "_", GfParmGetStr(param, RM_SECT_SUBFILES, RM_ATTR_SUFFIX, ""), groupAlpha, "s" );
	params_filename = strdup( buf );
	strncpy( buf, GfParmGetName(param), 1024 );
	ReCareerUtilManipString( buf, groupNumber, 1024 );
	GfParmWriteFile( params_filename, param, buf );
	subparam = GfParmReadFile( params_filename, GFPARM_RMODE_STD );
	free( params_filename );

	/* Make a new result file */
	suffix = GfParmGetStr(subparam, RM_SECT_SUBFILES, RM_ATTR_SUFFIX, "");
	ReCareerNewResult( filename, suffix, groupAlpha, subparam, drivers );

	/* Set name and description */
	GfParmSetVariable( subparam, RM_SECT_HEADER, "number", (tdble)groupNumber );
	strncpy( buf,  GfParmGetStr( subparam, RM_SECT_HEADER, RM_ATTR_NAME,  "" ), 1024 );
	GfParmSetStr( subparam, RM_SECT_HEADER, RM_ATTR_NAME, buf );
	strncpy( buf,  GfParmGetStr( subparam, RM_SECT_HEADER, RM_ATTR_DESCR,  "" ), 1024 );
	GfParmSetStr( subparam, RM_SECT_HEADER, RM_ATTR_DESCR, buf );
	GfParmRemoveVariable( subparam, RM_SECT_HEADER, "number" );

	/* Set that it is not the last subfile */
	GfParmSetStr( subparam, RM_SECT_SUBFILES, RM_ATTR_LASTSUBFILE, RM_VAL_NO );
	GfParmSetNum( subparam, RM_SECT_TRACKS, RM_ATTR_TOTALNUM, NULL, (tdble)totalTracks );
	snprintf( buf, 1024, "%s/%s/%s", RM_SECT_CLASSES, GfParmListGetCurEltName( ReInfo->params, RM_SECT_CLASSES ), RM_SECT_TRACKS );
	GfParmSetNum( subparam, RM_SECT_TRACKS, RM_ATTR_MINNUM, NULL, (float)((int)GfParmGetNum( ReInfo->params, buf, RM_ATTR_MINNUM, NULL, (tdble)1 )) );
	GfParmSetNum( subparam, RM_SECT_TRACKS, RM_ATTR_MAXNUM, NULL, (float)((int)GfParmGetNum( ReInfo->params, buf, RM_ATTR_MAXNUM, NULL, (tdble)totalTracks )) );

	return subparam;
}

static void* ReCareerNewClass( const char* filename, void *prevParam, void **firstParam, char first, int totalTracks )
{
	void *subparam;
	int nbGroups;
	int nbDrivers;
	void *curParam;
	char groupAlpha[3];
	int xx;

	/* Open subfile */
	snprintf( buf, 1024, "%sconfig/raceman/%s", GfLocalDir(), GfParmGetCurStr(ReInfo->params, RM_SECT_CLASSES, RM_ATTR_SUBFILE, "") );
	subparam = GfParmReadFile( buf, GFPARM_RMODE_STD );
	if( !subparam ) {
		GfLogError( "Subfile %s not found\n", buf );
		return prevParam;
	}
	
	nbGroups = (int)GfParmGetCurNum( ReInfo->params, RM_SECT_CLASSES, RM_ATTR_NBGROUPS, NULL, 1 );
	nbDrivers = (int)GfParmGetNum( ReInfo->params, RM_SECT_RACECARS, RM_ATTR_MAXNUM, NULL, 10 ) * nbGroups;
	if( first && nbDrivers < 10 )
		nbDrivers = 10; /* The lowest class should at least have 10 drivers, because there can be 10 humans. Note: This 10 limitation no longer exists */

	if( nbGroups == 1 ) {
		groupAlpha[ 0 ] = '\0';
		curParam = ReCareerNewGroup( filename, subparam, groupAlpha, nbDrivers, totalTracks, 0 );
		if( curParam ) {
			if( !*firstParam )
				*firstParam = curParam;
			if( prevParam ) {
				GfParmSetStr( curParam, RM_SECT_SUBFILES, RM_ATTR_PREVSUBFILE, GfParmGetFileName( prevParam ) );
				GfParmSetStr( prevParam, RM_SECT_SUBFILES, RM_ATTR_NEXTSUBFILE, GfParmGetFileName( curParam ) );
				if( prevParam != *firstParam ) {
					GfParmWriteFile( NULL, prevParam, NULL );
					GfParmReleaseHandle( prevParam );
				}
			}
			prevParam = curParam;
		}
	} else {
		groupAlpha[ 0 ] = '_';
		groupAlpha[ 2 ] = '\0';
		for( xx = 0; xx < nbGroups; ++xx ) {
			groupAlpha[ 1 ] = (char)( 'A' + xx );
			curParam = ReCareerNewGroup( filename, subparam, groupAlpha, nbDrivers / nbGroups + ( ( nbDrivers % nbGroups ) > xx ? 1 : 0 ),
			                             totalTracks, xx );
			if( curParam ) {
				if( !*firstParam )
					*firstParam = curParam;
				if( prevParam ) {
					GfParmSetStr( curParam, RM_SECT_SUBFILES, RM_ATTR_PREVSUBFILE, GfParmGetFileName( prevParam ) );
					GfParmSetStr( prevParam, RM_SECT_SUBFILES, RM_ATTR_NEXTSUBFILE, GfParmGetFileName( curParam ) );
					if( prevParam != *firstParam ) {
						GfParmWriteFile( NULL, prevParam, NULL );
						GfParmReleaseHandle( prevParam );
					}
				}
				prevParam = curParam;
			}
		}
	}

	return prevParam;
}

static void ReCareerNewParams( const char* filename, double date )
{
	void *firstParam = NULL;
	void *prevParam = NULL;
	char first = TRUE;
	int totalTracks = 1;

	if( GfParmListSeekFirst(ReInfo->params, RM_SECT_CLASSES) != 0 ) {
		GfLogError( "No classes defined\n" );
		return;
	}

	/* Calculate the maximum number of needed tracks */
	do {
		snprintf( buf, 1024, "%s/%s/%s", RM_SECT_CLASSES, GfParmListGetCurEltName(ReInfo->params, RM_SECT_CLASSES), RM_SECT_TRACKS );
		if( totalTracks < 0 || totalTracks < (int)GfParmGetNum(ReInfo->params, buf, RM_ATTR_MAXNUM, NULL, 1) )
			totalTracks = (int)GfParmGetNum(ReInfo->params, buf, RM_ATTR_MAXNUM, NULL, 1);
	} while( GfParmListSeekNext(ReInfo->params, RM_SECT_CLASSES) == 0 );

	GfParmListSeekFirst(ReInfo->params, RM_SECT_CLASSES);

	ReCareerNewResultXml( filename, date );

	do {
		prevParam = ReCareerNewClass( filename, prevParam, &firstParam, first, totalTracks );
		first = FALSE;
	} while( GfParmListSeekNext( ReInfo->params, RM_SECT_CLASSES ) == 0 );

	if( prevParam )
		GfParmSetStr( prevParam, RM_SECT_SUBFILES, RM_ATTR_LASTSUBFILE, RM_VAL_YES );

	if( firstParam ) {
		ReCareerNewResultXmlSetFirst( GfParmGetFileName( firstParam ) );
		GfParmSetStr( firstParam, RM_SECT_SUBFILES, RM_ATTR_PREVSUBFILE, GfParmGetFileName( prevParam ) );
		GfParmSetStr( prevParam, RM_SECT_SUBFILES, RM_ATTR_NEXTSUBFILE, GfParmGetFileName( firstParam ) );
		if( firstParam != prevParam ) {
			GfParmWriteFile( NULL, prevParam, NULL );
			GfParmReleaseHandle( prevParam );
		}
		GfParmWriteFile( NULL, firstParam, NULL );
		GfParmReleaseHandle( firstParam );
	}

	ReCareerInitDrivers(ReInfo->params);
	ReCareerNewDrivers();
}

void ReCareerNew()
{
	struct tm *stm;
	time_t t;
	char *filename;

	t = time(NULL);
	stm = localtime(&t);
	snprintf( buf, 1024, "%sresults/%s/%%s-%4d-%02d-%02d-%02d-%02d%%s%%s%%s.xml%%s", GfLocalDir(), ReInfo->_reFilename,
	          stm->tm_year + 1900, stm->tm_mon + 1, stm->tm_mday, stm->tm_hour, stm->tm_min );
	filename = strdup(buf); // Makes it possible to reuse buf
	ReCareerNewParams(filename, (double)t);
	free(filename);

	ReCareerNextSeason();
}

static int ReCareerNextTeamCompare( const void *team1_v, const void *team2_v )
{
	tTeamInfo* team1 = (tTeamInfo*)team1_v;
	tTeamInfo* team2 = (tTeamInfo*)team2_v;

	if( team1->sortPoints > team2->sortPoints )
		return -1;
	else if( team1->sortPoints == team2->sortPoints )
		return 0;
	else
		return 1;
}

static int ReCareerNextDriversCompare( const void *driver1, const void *driver2 )
{
	tDriverInfo* drv1 = *(tDriverInfo**)driver1;
	tDriverInfo* drv2 = *(tDriverInfo**)driver2;

	if( drv1->sortValue < drv2->sortValue )
		return -1;
	else if( drv1->sortValue == drv2->sortValue )
		return 0;
	else
		return 1;
}

void ReCareerNextAddTeams( tGroupInfo *group, void *curParam, void *curResults )
{
	int xx;
	
	group->nbDrivers = (int)GfParmGetNum( curResults, RM_SECT_DRIVERS, RM_ATTR_MAXNUM, NULL, 10 );
	group->nbTeams = GfParmGetEltNb( curResults, RE_SECT_TEAMINFO );
	group->curTeam = 0;
	group->teams = (tTeamInfo*)malloc( sizeof( tTeamInfo ) * group->nbTeams );

	GfParmListSeekFirst( curResults, RE_SECT_TEAMINFO );
	//GfLogDebug( "ReCareerNextAddTeams()\n" );
	for( xx = 0; xx < group->nbTeams; ++xx ) {
		group->teams[ xx ].name = strdup( GfParmListGetCurEltName( curResults, RE_SECT_TEAMINFO ) );
		group->teams[ xx ].car_dname = strdup( GfParmGetCurStr( curResults, RE_SECT_TEAMINFO, RM_ATTR_CARNAME, "" ) );
		group->teams[ xx ].nbDrivers = 0;
		group->teams[ xx ].curDriver = 0;
		group->teams[ xx ].sortPoints = GfParmGetCurNum( curResults, RE_SECT_TEAMINFO, RE_ATTR_POINTS, NULL, 0 );
		GfParmListSeekNext( curResults, RE_SECT_TEAMINFO );
	}

	// Add points from drivers belonging to this team
	if( GfParmListSeekFirst( curResults, RE_SECT_STANDINGS ) == 0 ) {
		do {
			for( xx = 0; xx < group->nbTeams; ++xx ) {
				if( strcmp( group->teams[ xx ].name, GfParmGetCurStr( curResults, RE_SECT_STANDINGS, RE_ATTR_NAME, "" ) ) == 0 ) {
					group->teams[ xx ].sortPoints += GfParmGetCurNum( curResults, RE_SECT_STANDINGS, RE_ATTR_POINTS, NULL, 0.0f );
				}
			}
		} while( GfParmListSeekNext( curResults, RE_SECT_STANDINGS ) );
	}

	//Half points
	for( xx = 0; xx < group->nbTeams; ++xx )
		group->teams[ xx ].sortPoints /= 2.0f;
}

void ReCareerNextAddDrivers( tDriverInfo ***drivers, int *listLength, tCareerInfo *info, void *curParam, void *curResults )
{
	tDriverInfo **newDrivers;
	int **classPosition;
	int curClass = -1;
	int newNb;
	int xx;
	int yy;
	int zz;

	newNb = GfParmGetEltNb( curParam, RM_SECT_DRIVERS );
	if( newNb == 0 )
		return; /* Nothing to add */
	newDrivers = (tDriverInfo**)malloc( sizeof( tDriverInfo* ) * ( *listLength + newNb ) );
	for( xx = 0; xx < *listLength; ++xx )
		newDrivers[ xx ] = (*drivers)[ xx ];
	
	classPosition = (int**)malloc( sizeof(int*) * newNb );

	GfLogDebug("ReCareerNextAddDrivers:\n");
	
	GfParmListSeekFirst( curParam, RM_SECT_DRIVERS );
	for( xx = *listLength; xx < *listLength + newNb; ++xx ) {
		newDrivers[ xx ] = (tDriverInfo*)malloc( sizeof( tDriverInfo ) );
		newDrivers[ xx ]->module = strdup( GfParmGetCurStr( curParam, RM_SECT_DRIVERS, RM_ATTR_MODULE, "" ) );
		newDrivers[ xx ]->extended = (int)GfParmGetCurNum( curParam, RM_SECT_DRIVERS, RM_ATTR_EXTENDED, NULL, 0 );
		newDrivers[ xx ]->idx = (int)GfParmGetCurNum( curParam, RM_SECT_DRIVERS, RM_ATTR_IDX, NULL, 0 );
		snprintf( buf, 1024, "%s/%s/%d/%d", RM_SECT_DRIVERINFO, newDrivers[ xx ]->module, newDrivers[ xx ]->extended, newDrivers[ xx ]->idx );
		newDrivers[ xx ]->name = strdup( GfParmGetStr( curParam, buf, RM_ATTR_NAME, "" ) );
		newDrivers[ xx ]->skill = GfParmGetNum( curParam, buf, RM_ATTR_SKILLLEVEL, NULL, 5.0f );
		newDrivers[ xx ]->classPointList = (double*)malloc( sizeof( double ) * info->nbClasses );
		newDrivers[ xx ]->sortValue = 0.0f;

		GfLogDebug("  * %s #%d (%s)%s\n", newDrivers[ xx ]->module, newDrivers[ xx ]->idx, newDrivers[ xx ]->name,
				   newDrivers[ xx ]->extended ? " extended" : "");

		/* Get class points */
		classPosition[ xx - *listLength ] = (int*)malloc( sizeof(int) * info->nbClasses );
		snprintf( buf, 1024, "%s/%s/%d/%d", RE_SECT_CLASSPOINTS, newDrivers[ xx ]->module, newDrivers[ xx ]->extended, newDrivers[ xx ]->idx );
		for( yy = 0; yy < info->nbClasses; ++yy )
		{
			newDrivers[ xx ]->classPointList[ yy ] = 0.0f;
			classPosition[ xx - *listLength ][ yy ] = 1;
		}
		if( GfParmListSeekFirst( curResults, buf ) == 0 ) {
			do {
				for( yy = 0; yy < info->nbClasses; ++yy ) {
					if( strcmp( info->classes[ yy ].suffix, GfParmListGetCurEltName( curResults, buf ) ) == 0 ) {
						newDrivers[ xx ]->classPointList[ yy ] = GfParmGetCurNum( curResults, buf, RE_ATTR_POINTS, NULL,
													   (tdble)newDrivers[ xx ]->classPointList[ yy ] );
						for( zz = 0; zz < xx - *listLength; ++zz )
						{
							if( newDrivers[ xx ]->classPointList[ yy ] < newDrivers[ zz ]->classPointList[ yy ] )
								++classPosition[ xx - *listLength ][ yy ];
							else if( newDrivers[ xx ]->classPointList[ yy ] > newDrivers[ zz ]->classPointList[ yy ] )
								++classPosition[ zz ][ yy ];
						}
						break;
					}
				}
			} while( GfParmListSeekNext( curResults, buf ) == 0 );
		}

		GfParmListSeekNext( curParam, RM_SECT_DRIVERS );
	}

	/* Find out what the current class is */
	for( xx = 0; xx < info->nbClasses; ++xx )
	{
		if( strcmp( info->classes[ xx ].suffix, GfParmGetStr( curParam, RM_SECT_SUBFILES, RM_ATTR_SUFFIX, "" ) ) == 0 )
		{
			curClass = xx;
			break;
		}
	}

	/* Now use the information we have to take the end-of-season points into account */
	for( xx = *listLength; xx < *listLength + newNb; ++xx )
	{
		GfParmSetVariable( curParam, RM_SECT_ENDOFSEASON, "ownClassPos", curClass >= 0 ? (tdble)classPosition[ xx - *listLength ][ curClass ] : (tdble)newNb );
		GfParmSetVariable( curParam, RM_SECT_ENDOFSEASON, "ownClassPoints",
		                   curClass >= 0 ? (tdble)newDrivers[ xx ]->classPointList[ curClass ] : 0.0f );

		if( GfParmListSeekFirst( curParam, RM_SECT_ENDOFSEASON_CLASSPOINTS ) == 0 )
		{
			do
			{
				for( yy = 0; yy < info->nbClasses; ++yy )
				{
					if( strcmp( info->classes[ yy ].suffix, GfParmGetCurStr( curParam, RM_SECT_ENDOFSEASON_CLASSPOINTS, RM_ATTR_SUFFIX, "" )) == 0 )
					{
						snprintf( buf, 1024, "%s/%s", RM_SECT_ENDOFSEASON_CLASSPOINTS,
						                              GfParmListGetCurEltName( curParam, RM_SECT_ENDOFSEASON_CLASSPOINTS ) );
						GfParmSetVariable( curParam, buf, "curClassPos", (tdble)classPosition[ xx - *listLength ][ yy ] );
						GfParmSetVariable( curParam, buf, "curClassPoints", (tdble)newDrivers[ xx ]->classPointList[ yy ] );

						newDrivers[ xx ]->classPointList[ yy ] = GfParmGetCurNum( curParam, RM_SECT_ENDOFSEASON_CLASSPOINTS,
						                                                          RM_ATTR_POINTS, (char*)NULL,
						                                                          (tdble)newDrivers[ xx ]->classPointList[yy]);

						GfParmRemoveVariable( curParam, buf, "curClassPos" );
						GfParmRemoveVariable( curParam, buf, "curClassPoints" );
					}
				}
			} while( GfParmListSeekNext( curParam, RM_SECT_ENDOFSEASON_CLASSPOINTS ) == 0 );
		}

		GfParmRemoveVariable( curParam, RM_SECT_ENDOFSEASON, "curClassPos" );
		GfParmRemoveVariable( curParam, RM_SECT_ENDOFSEASON, "curClassPoints" );
	}

	/* Cleanup allocated memory */
	for( xx = 0; xx < newNb; ++xx )
		free( classPosition[ xx ] );
	free( classPosition );

	if( *drivers )
		free( *drivers );

	/* Store new data */
	*drivers = newDrivers;
	*listLength += newNb;
}

void ReCareerNextRead( tCareerInfo *info, tDriverInfo ***driverList, int *driverListLength )
{
	char *firstfile;
	void *curParam;
	void *curResult;
	void *tmp;
	int xx;
	int yy;
	tClassInfo *prevClass = NULL;

	/* Init tCareerInfo */
	info->nbClasses = GfParmGetEltNb( ReInfo->mainParams, RM_SECT_CLASSES );
	info->classes = (tClassInfo*)malloc( sizeof( tClassInfo ) * info->nbClasses );
	GfParmListSeekFirst( ReInfo->mainParams, RM_SECT_CLASSES );
	for( xx = 0; xx < info->nbClasses; ++xx ) {
		info->classes[ xx ].suffix = strdup( GfParmGetCurStr( ReInfo->mainParams, RM_SECT_CLASSES, RM_ATTR_SUBFILE_SUFFIX, "" ) );
		info->classes[ xx ].nbGroups = (int)GfParmGetCurNum( ReInfo->mainParams, RM_SECT_CLASSES, RM_ATTR_NBGROUPS, NULL, 1 );
		info->classes[ xx ].groups = (tGroupInfo*)malloc( sizeof( tGroupInfo ) * info->classes[ xx ].nbGroups );
		for( yy = 0; yy < info->classes[ xx ].nbGroups; ++yy ) {
			info->classes[ xx ].groups[ yy ].nbDrivers = 0;
			info->classes[ xx ].groups[ yy ].nbTeams = 0;
			info->classes[ xx ].groups[ yy ].teams = NULL;
		}
		GfParmListSeekNext( ReInfo->mainParams, RM_SECT_CLASSES );
	}

	/* Init driverlist */
	*driverList = NULL;
	*driverListLength = 0;

	firstfile = strdup( GfParmGetStr( ReInfo->mainResults, RE_SECT_CURRENT, RE_ATTR_CUR_FILE, "" ) );
	curParam = GfParmReadFile( firstfile, GFPARM_RMODE_STD );
	yy = 0;
	do {
		curResult = GfParmReadFile( GfParmGetStr( curParam, RM_SECT_SUBFILES, RM_ATTR_RESULTSUBFILE, "" ), GFPARM_RMODE_STD );

		for( xx = 0; xx < info->nbClasses; ++xx ) {
			if( strcmp( GfParmGetStr( curParam, RM_SECT_SUBFILES, RM_ATTR_SUFFIX, "" ), info->classes[ xx ].suffix ) == 0 ) {
				if( prevClass == &info->classes[ xx ] && yy + 1 < info->classes[ xx ].nbGroups ) {
					++yy;
				} else {
					yy = 0;
					prevClass = &info->classes[ xx ];
				}

				ReCareerNextAddTeams( &info->classes[ xx ].groups[ yy ], curParam, curResult );
				ReCareerNextAddDrivers( driverList, driverListLength, info, curParam, curResult );
			}
		}

		GfParmReleaseHandle( curResult );
		tmp = GfParmReadFile( GfParmGetStr( curParam, RM_SECT_SUBFILES, RM_ATTR_NEXTSUBFILE, "" ), GFPARM_RMODE_STD );
		GfParmReleaseHandle( curParam );
		curParam = tmp;
	} while( curParam && strcmp( firstfile, GfParmGetFileName( curParam ) ) != 0 );
	
	if( curParam )
		GfParmReleaseHandle( curParam );
}

static void ReCareerNextCorrectIdx( tGroupInfo *group )
{
	int curTeam = 0;
	int curDriver = 0;
	int xx;
	int yy;
	char found;

	while( curTeam < group->nbTeams ) {
		if( curDriver >= group->teams[ curTeam ].nbDrivers ) {
			++curTeam;
			curDriver = 0;
			continue;
		}

		if( strcmp( group->teams[ curTeam ].drivers[ curDriver ]->module, "human" ) == 0 ) {
			/* Skip human drivers : idx should not change for those drivers */
			++curDriver;
			continue;
		}

		found = FALSE;
		/* Search first driver with same module and extended in indexes drivers (searching reverse) */
		for( xx = curTeam; xx >= 0 && !found; --xx ) {
			for( yy = ( xx == curTeam ? curDriver : group->teams[ xx ].nbDrivers ) - 1; yy >= 0; --yy ) {
				if( strcmp( group->teams[ curTeam ].drivers[ curDriver ]->module, group->teams[ xx ].drivers[ yy ]->module ) == 0 &&
				    group->teams[ curTeam ].drivers[ curDriver ]->extended == group->teams[ xx ].drivers[ yy ]->extended ) {
					group->teams[ curTeam ].drivers[ curDriver ]->idx = group->teams[ xx ].drivers[ yy ]->idx + 1;
					found = TRUE;
					break;
				}
			}
			if( found )
				break;
		}
		if( !found )
			group->teams[ curTeam ].drivers[ curDriver ]->idx = 0;

		++curDriver; /* Going to next team at start of while if this was the last driver of the team */
	}
}

static void ReCareerNextReorder( tCareerInfo *info, tDriverInfo ***driverList, int driverListLength )
{
	int xx;
	int yy;
	int zz;
	int arrayLength = driverListLength;
	int seatsAvailable;
	int groupIndex;
	tGroupInfo *curGroup;

	/* Reorder from the highest classes to the lowest classes */
	for( xx = info->nbClasses - 1; xx >= 0; --xx ) {
		seatsAvailable = 0;
		for( yy = 0; yy < arrayLength; ++yy )
			(*driverList)[ yy ]->sortValue = (*driverList)[ yy ]->classPointList[ xx ];
		for( yy = 0; yy < info->classes[ xx ].nbGroups; ++yy ) {
			seatsAvailable += info->classes[ xx ].groups[ yy ].nbDrivers;
			qsort( info->classes[ xx ].groups[ yy ].teams, info->classes[ xx ].groups[ yy ].nbTeams, sizeof( tTeamInfo ),
			       ReCareerNextTeamCompare );
			for( zz = 0; zz < info->classes[ xx ].groups[ yy ].nbTeams; ++zz ) {
				info->classes[ xx ].groups[ yy ].teams[ zz ].nbDrivers = 
				           info->classes[ xx ].groups[ yy ].nbDrivers / info->classes[ xx ].groups[ yy ].nbTeams +
				           ( ( info->classes[ xx ].groups[ yy ].nbDrivers % info->classes[ xx ].groups[ yy ].nbTeams ) > zz ? 1 : 0 );
				info->classes[ xx ].groups[ yy ].teams[ zz ].drivers = (tDriverInfo**)malloc( sizeof( tDriverInfo* ) * 
				                                                                info->classes[ xx ].groups[ yy ].teams[ zz ].nbDrivers );
			}
		}
		qsort( *driverList, arrayLength, sizeof( tDriverInfo* ), ReCareerNextDriversCompare );
		groupIndex = 0;
		for( yy = arrayLength - 1; yy >= arrayLength - seatsAvailable; --yy ) {
			/* First, higher the indices */
			while( info->classes[ xx ].groups[ groupIndex ].curTeam >= info->classes[ xx ].groups[ groupIndex ].nbTeams ) {
				++groupIndex;
				if( groupIndex >= info->classes[ xx ].nbGroups )
					groupIndex = 0;
			}
			curGroup = &info->classes[ xx ].groups[ groupIndex ];

			curGroup->teams[ curGroup->curTeam ].drivers[ curGroup->teams[ curGroup->curTeam ].curDriver ] = (*driverList)[ yy ];
			++ curGroup->teams[ curGroup->curTeam ].curDriver;
			if( curGroup->teams[ curGroup->curTeam ].curDriver >= curGroup->teams[ curGroup->curTeam ].nbDrivers ) {
				/* Team is now full */
				++ curGroup->curTeam;
			}
			/* Goto next group */
			++groupIndex;
			if( groupIndex >= info->classes[ xx ].nbGroups )
				groupIndex = 0;
		}

		arrayLength -= seatsAvailable;

		/* Correct idx */
		for( yy = 0; yy < info->classes[ xx ].nbGroups; ++yy )
			ReCareerNextCorrectIdx( &info->classes[ xx ].groups[ yy ] );
	}
}

static void ReCareerNextTracks( void *subparam )
{
	int number;
	int max, min;
	int total;
	char **trackEltNames;
	int nbAllowedTracks;
	int trackIndex;
	int curResultIndex = 1;
	int xx;
	char *path;

	max = (int)GfParmGetNum( subparam, RM_SECT_TRACKS, RM_ATTR_MAXNUM, NULL, 2 );
	min = (int)GfParmGetNum( subparam, RM_SECT_TRACKS, RM_ATTR_MINNUM, NULL, 1 );
	total = (int)GfParmGetNum( subparam, RM_SECT_TRACKS, RM_ATTR_TOTALNUM, NULL, 2 );
	number = ReCareerUtilRand( min, max );

	nbAllowedTracks = GfParmGetEltNb( subparam, RM_SECT_ALLOWEDTRACKS );
	trackEltNames = (char**)malloc( sizeof( char* ) * nbAllowedTracks );
	GfParmListSeekFirst( subparam, RM_SECT_ALLOWEDTRACKS );
	if( nbAllowedTracks < number )
		number = nbAllowedTracks;
	if( number < 2 )
		number = 2;
	for( xx = 0; xx < nbAllowedTracks; ++xx ) {
		trackEltNames[ xx ] = strdup( GfParmListGetCurEltName( subparam, RM_SECT_ALLOWEDTRACKS ) );
		GfParmListSeekNext( subparam, RM_SECT_ALLOWEDTRACKS );
	}

	GfParmSetNum( subparam, RM_SECT_TRACKS, RM_ATTR_NUMBER, NULL, (tdble)number );

	for( xx = 0; xx < number; ++xx ) {
		do {
			trackIndex = ReCareerUtilRand( 0, nbAllowedTracks - 1 );
		} while( !trackEltNames[ trackIndex ] );

		while( curResultIndex < (int)floor( (double)(xx * ( total - 1 )) / (double)( number - 1 ) + 0.5f ) + 1 ) {
			/* Free round */
			snprintf( buf, 1024, "%s/%d", RM_SECT_TRACKS, curResultIndex );
			GfParmSetStr( subparam, buf, RM_ATTR_NAME, "free" );
			GfParmSetStr( subparam, buf, RM_ATTR_CATEGORY, "free" );
			++curResultIndex;
		}

		snprintf( buf, 1024, "%s/%s", RM_SECT_ALLOWEDTRACKS, trackEltNames[ trackIndex ] );
		path = strdup( buf );
		snprintf( buf, 1024, "%s/%d", RM_SECT_TRACKS, curResultIndex );
	
		GfParmSetStr( subparam, buf, RM_ATTR_NAME, GfParmGetStr( subparam, path, RM_ATTR_NAME, "free" ) );
		GfParmSetStr( subparam, buf, RM_ATTR_CATEGORY, GfParmGetStr( subparam, path, RM_ATTR_CATEGORY, "free" ) );

		free( path );
		free( trackEltNames[ trackIndex ] );
		trackEltNames[ trackIndex ] = 0;

		++curResultIndex;
	}

	for( xx = 0; xx < nbAllowedTracks; ++xx )
		if( trackEltNames[ xx ] )
			free( trackEltNames[ xx ] );
	free( trackEltNames );
}

static void ReCareerNextWrite( tCareerInfo *info )
{
	int *curGroup;
	tGroupInfo *curGroupPtr;
	int xx;
	int yy;
	int zz;
	int uu;
	char *firstfile;
	void *curParam;
	void *curResult;
	void *tmp;

	curGroup = (int*)malloc( sizeof(int) * info->nbClasses );
	for( xx = 0; xx < info->nbClasses; ++xx )
		curGroup[ xx ] = 0;

	//GfLogDebug("ReCareerNextWrite:\n");

	curParam = GfParmReadFile( GfParmGetStr( ReInfo->mainResults, RE_SECT_CURRENT, RE_ATTR_CUR_FILE, "" ), GFPARM_RMODE_STD );
	firstfile = strdup( GfParmGetFileName( curParam ) );
	do {
		curResult = GfParmReadFile( GfParmGetStr( curParam, RM_SECT_SUBFILES, RM_ATTR_RESULTSUBFILE, "" ), GFPARM_RMODE_STD );

		GfParmListClean( curParam, RM_SECT_DRIVERS );
		GfParmListClean( curParam, RM_SECT_DRIVERINFO );
		GfParmListClean( curParam, RM_SECT_TRACKS );
		xx = 0;
		while( xx < info->nbClasses && !strcmp( info->classes[ xx ].suffix, GfParmGetStr( curParam, RM_SECT_SUBFILES, RM_ATTR_SUFFIX, "" ) ) == 0 ) {
			++xx;
		}
		if( xx >= info->nbClasses ) {
			GfLogError( "Could not found a class for suffix %s\n", GfParmGetStr( curParam, RM_SECT_SUBFILES, RM_ATTR_SUFFIX, "" ) );
			xx = 0;
		}
		curGroupPtr = &info->classes[ xx ].groups[ curGroup[ xx ] ];
		yy = 1;
		for( zz = 0; zz < curGroupPtr->nbTeams; ++zz ) {
			for( uu = 0; uu < curGroupPtr->teams[ zz ].nbDrivers; ++uu ) {
				/* Fill Drivers Section */
				snprintf( buf, 1024, "%s/%d", RM_SECT_DRIVERS, yy );
				GfParmSetStr( curParam, buf, RM_ATTR_MODULE, curGroupPtr->teams[ zz ].drivers[ uu ]->module );
				GfParmSetNum( curParam, buf, RM_ATTR_IDX, NULL, (tdble)curGroupPtr->teams[ zz ].drivers[ uu ]->idx );
				GfParmSetNum( curParam, buf, RM_ATTR_EXTENDED, NULL, (tdble)curGroupPtr->teams[ zz ].drivers[ uu ]->extended );

				//const tDriverInfo* pDriver = curGroupPtr->teams[ zz ].drivers[ uu ];
				//GfLogDebug("  * %s #%d (%s)%s\n", pDriver->module, pDriver->idx, pDriver->name,
				//		   pDriver->extended ? " extended" : "");
				
				/* Fill Driver Info */
				snprintf( buf, 1024, "%s/%s/%d/%d", RM_SECT_DRIVERINFO, curGroupPtr->teams[ zz ].drivers[ uu ]->module,
				                                                        curGroupPtr->teams[ zz ].drivers[ uu ]->extended,
											curGroupPtr->teams[ zz ].drivers[ uu ]->idx );
				GfParmSetStr( curParam, buf, RM_ATTR_NAME, curGroupPtr->teams[ zz ].drivers[ uu ]->name );
				GfParmSetStr( curParam, buf, RM_ATTR_CARNAME, curGroupPtr->teams[ zz ].car_dname );
				//GfLogDebug( "GfParmSetStr( %p, %s, %s, %s )\n", curParam, buf, RM_ATTR_CARNAME, curGroupPtr->teams[ zz ].car_dname );
				GfParmSetStr( curParam, buf, RM_ATTR_TEAMNAME, curGroupPtr->teams[ zz ].name );
				GfParmSetNum( curParam, buf, RM_ATTR_SKILLLEVEL, NULL, (tdble)curGroupPtr->teams[ zz ].drivers[ uu ]->skill );
				++yy;
			}
		}

		/* Add tracks */
		ReCareerNextTracks( curParam );

		/* Modify result file */
		GfParmListClean( curResult, RE_SECT_CLASSPOINTS );
		
		/* Write team points */
		uu = 0;
		if( GfParmListSeekFirst( curResult, RE_SECT_TEAMINFO ) == 0 && uu < curGroupPtr->nbTeams ) {
			do {
				GfParmSetCurNum( curResult, RE_SECT_TEAMINFO, RE_ATTR_POINTS, NULL,
				                 GfParmGetCurNum( curResult, RE_SECT_TEAMINFO, RE_ATTR_POINTS, NULL, (tdble)curGroupPtr->teams[ uu ].sortPoints ) );
				++uu;
			} while( GfParmListSeekNext( curResult, RE_SECT_TEAMINFO ) == 0 && uu < curGroupPtr->nbTeams );
		}
		
		/* Write files */
		GfParmWriteFile( NULL, curResult, NULL );
		GfParmReleaseHandle( curResult );
		GfParmWriteFile( NULL, curParam, NULL );
		tmp = GfParmReadFile( GfParmGetStr( curParam, RM_SECT_SUBFILES, RM_ATTR_NEXTSUBFILE, "" ), GFPARM_RMODE_STD );
		GfParmReleaseHandle( curParam );
		curParam = tmp;
		++curGroup[ xx ];
	} while( !strcmp( GfParmGetFileName( curParam ), firstfile ) == 0 );
	GfParmReleaseHandle( curParam );
	
	free( firstfile );
	free( curGroup );
}

static void ReCareerNextCleanup( tCareerInfo *info, tDriverInfo ***driverList, int /*driverListLentgh*/ )
{
	int classIndex;
	int groupIndex;
	int teamIndex;
	int driverIndex;

	/* Free driver list; all drivers are also in the team structure */
	free( *driverList );
	*driverList = NULL;

	for( classIndex = 0; classIndex < info->nbClasses; ++classIndex ) {
		for( groupIndex = 0; groupIndex < info->classes[ classIndex ].nbGroups; ++groupIndex ) {
			for( teamIndex = 0; teamIndex < info->classes[ classIndex ].groups[ groupIndex ].nbTeams; ++teamIndex ) {
				for( driverIndex = 0; driverIndex < info->classes[ classIndex ].groups[ groupIndex ].teams[ teamIndex ].nbDrivers;
				     ++driverIndex ) {
					free( info->classes[ classIndex ].groups[ groupIndex ].teams[ teamIndex ].drivers[ driverIndex ]->module );
					free( info->classes[ classIndex ].groups[ groupIndex ].teams[ teamIndex ].drivers[ driverIndex ]->name );
					free( info->classes[ classIndex ].groups[ groupIndex ].teams[ teamIndex ].drivers[ driverIndex ]->classPointList );
					free( info->classes[ classIndex ].groups[ groupIndex ].teams[ teamIndex ].drivers[ driverIndex ] );
				}
				free( info->classes[ classIndex ].groups[ groupIndex ].teams[ teamIndex ].name );
				free( info->classes[ classIndex ].groups[ groupIndex ].teams[ teamIndex ].car_dname );
				free( info->classes[ classIndex ].groups[ groupIndex ].teams[ teamIndex ].drivers );
			}
			free( info->classes[ classIndex ].groups[ groupIndex ].teams );
		}
		free( info->classes[ classIndex ].suffix );
		free( info->classes[ classIndex ].groups );
	}
	free( info->classes );
}

void ReCareerNextSeason()
{
	tCareerInfo info;
	tDriverInfo **driverList = NULL;
	int driverListLength = 0;

	/* Make season number one higher */
	GfParmSetNum( ReInfo->mainResults, RE_SECT_CURRENT, RE_ATTR_CUR_SEASON, NULL,
	              GfParmGetNum( ReInfo->mainResults, RE_SECT_CURRENT, RE_ATTR_CUR_SEASON, NULL, 0 ) + 1 );
	/* Read data */
	ReCareerNextRead( &info, &driverList, &driverListLength );
	/* Reorder data (promote / degradate) */
	ReCareerNextReorder( &info, &driverList, driverListLength );
	/* Write new season settings to file */
	ReCareerNextWrite( &info );
	/* Clean temporaly data */
	ReCareerNextCleanup( &info, &driverList, driverListLength );
}

