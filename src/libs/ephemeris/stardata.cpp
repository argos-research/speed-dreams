/****************************************************************************

    file                 : stardata.cpp
    created              : Sun Aug 02 22:54:56 CET 2012
    copyright            : (C) 2000  Curtis L. Olson - (C)2012 Xavier Bertaux
    email                : bertauxx@yahoo.fr
    version              : $Id: stardata.cpp 4811 2012-07-29 17:16:37Z torcs-ng $

 ****************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <string>
#include <fstream>

#include "stardata.h"

using namespace std;

// Constructor
ePhStarData::ePhStarData( const char *path )
{
    load(path);
}

// Destructor
ePhStarData::~ePhStarData() 
{
}

bool ePhStarData::load( const char *path ) 
{
    	_stars.clear();
    	
    	int i = 0;
    	int num = 3000;

    	FILE * f;
  	double ra, dec, mag;
	
    	char name;

  	f = fopen (path, "r");
  	if (f == NULL)
  	{
    		//cout << "Impossible d'ouvrir le fichier en lecture !" << endl;
  	}
  	else
   	{
    		for (i = 0; i < num; i++)
    		{
     			fscanf (f, "%s %lf %lf %lf ", &name, &ra, &dec, &mag);
     			_stars.push_back(SGVec3d(ra, dec, mag));
     		}

	}
	
	fclose (f);    	
    	return true;
}
