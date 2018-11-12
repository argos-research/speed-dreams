/*
* \author Alexander Weidinger
* \date   2017-07-06
*/
#ifndef _SENSOBST_H_
#define _SENSOBST_H_

#include <stdio.h>
#include <math.h>
#include <car.h>
#include <robottools.h>
#include <raceman.h>
#include <list>

typedef struct point {
	double x, y;
} point;

class SingleObstacleSensor {
public:
	SingleObstacleSensor(tCarElt *car, double angle, double move_x, double move_y, double range);
	~SingleObstacleSensor();

protected:
	tCarElt *car;
	double angle;
	double range;
	double move_x;
	double move_y;
	double distance = -1;

public:
	double getMove_x() {
		return move_x;
	}

	double getMove_y() {
		return move_y;
	}

	double getAngle() {
		return angle;
	}

	double getRange() {
		return range;
	}

	double getDistance() {
		return distance;
	}

	void setDistance(double dist) {
		distance = dist;
	}
};

class ObstacleSensors {
public:
	ObstacleSensors(tTrack* track, tCarElt* car);
	~ObstacleSensors();

protected:
	std::list<SingleObstacleSensor> sensors;
	tCarElt* myc;

private:
	double distance(point p1, point p2);
	bool is_between(point p1, point p2, point p);
	double normRand(double avg, double std);

public:
	void addSensor(tCarElt *car, double angle, double move_x, double move_y, double range);
	void sensors_update(tSituation *situation);

	std::list<SingleObstacleSensor> getSensorsList() {
		return sensors;
	}
};

#endif
