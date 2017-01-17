/***************************************************************************

    file                 : cardata.h
    created              : Thu Sep 23 12:31:33 CET 2004
    copyright            : (C) 2004 Bernhard Wymann
    email                : berniw@bluewin.ch
    version              : $Id: cardata.h 5950 2015-04-05 19:34:04Z torcs-ng $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

/*
	This class holds global facts about cars, therefore no data relative to
	each other (for that is the class Opponents/Opponent responsible).
*/

#ifndef _BT_CARDATA_H_
#define _BT_CARDATA_H_

#include <stdio.h>
#include <math.h>
#include <car.h>
#include <track.h>
#include <robottools.h>
#include <raceman.h>

#include "linalg.h"


class SingleCardata
{
  public:
  void init(CarElt *car);

  inline float getSpeedInTrackDirection() { return speed; }
  inline float getWidthOnTrack() { return width; }
  inline float getLengthOnTrack() { return length; }
  inline float getTrackangle() { return trackangle; }
  inline float getCarAngle() { return angle; }
  inline float getTrueSpeed() { return truespeed; }

  inline bool thisCar(tCarElt *car) { return (car == this->car); }
  inline tPosd *getCorner1() { return corner1; }
  inline tPosd *getCorner2() { return corner2; }
  inline tPosd *getLastSpeed() { return lastspeed; }
  inline double getSpeedDeltaX() { return (car->_speed_X - lastspeed[1].ax)*0.7 + (lastspeed[1].ax-lastspeed[2].ax)/4; }
        inline double getSpeedDeltaY() { return (car->_speed_Y - lastspeed[1].ay)*0.7 + (lastspeed[1].ay-lastspeed[2].ay)/4; }
		inline float toLftWall() { return tolftwall; }
		inline float toRgtWall() { return torgtwall; }
		inline float getDistFromStart() { return car->_distFromStartLine; }

		void update();
		void updateWalls();
		void evalTrueSpeed();

	protected:
		static float getSpeed(tCarElt *car, float trackangle);

        float speed;            // speed in direction of the track.
		float truespeed;		// speed accounting for bends
        float width;            // the cars needed width on the track.
        float length;           // the cars needed length on the track.
        float trackangle;       // Track angle at the opponents position.
        float angle;            // The angle of the car relative to the track tangent.
		float tolftwall;        // how far to the nearest left barrier
		float torgtwall;        // how far to the nearest Right barrier

        tPosd corner1[4];
        tPosd corner2[4];
        tPosd lastspeed[3];

        tCarElt *car;           // For identification.
};


// TODO: use singleton pattern.
class Cardata {
	public:
		Cardata(tSituation *s);
		~Cardata();

		void update();
		SingleCardata *findCar(tCarElt *car);
		int getNCars() { return ncars; }
		SingleCardata *getCarData(int i) { return data + i; }

	protected:
		SingleCardata *data;	// Array with car data.
		int ncars;				// # of elements in data.
};


#endif // _BT_CARDATA_H_
