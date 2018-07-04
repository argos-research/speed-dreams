/***************************************************************************

    file                 : argos_AI1.cpp
    created              : Di 3. Jul 15:24:53 CEST 2018
    copyright            : (C) 2002 Alexander Weidinger

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifdef _WIN32
#include <windows.h>
#endif

#include <stdio.h>
#include <stdlib.h> 
#include <string.h> 
#include <math.h>

#include <tgf.h> 
#include <track.h> 
#include <car.h> 
#include <raceman.h> 
#include <robottools.h>
#include <robot.h>

#include <obstacleSensors.h>

#include <pistache/endpoint.h>

using namespace Pistache;

static ObstacleSensors *obstSens;

static float keepLR = 4.0;
static double desired_speed = 44 / 3.6;

struct RESTHandler : public Http::Handler {
	HTTP_PROTOTYPE(RESTHandler)

	void onRequest(const Http::Request& req, Http::ResponseWriter resp) {
		if (req.resource() == "/moveLeft") {
			if (keepLR == -4.0) {
				resp.send(Http::Code::Forbidden);
			} else {
				keepLR -= 4.0;
				resp.send(Http::Code::Ok);
			}
		} else if (req.resource() == "/moveRight") {
			if (keepLR == 4.0) {
				resp.send(Http::Code::Forbidden);
			} else {
				keepLR += 4.0;
				resp.send(Http::Code::Ok);
			}
		} else if (req.resource() == "/setSpeed") {
			desired_speed = std::stoi(req.body()) / 3.6;
			resp.send(Http::Code::Ok);
		} else if (req.resource() == "/getSensor/0") {
			resp.send(Http::Code::Ok, std::to_string(std::next(obstSens->getSensorsList().begin(), 0)->getDistance()));
		} else if (req.resource() == "/getSensor/1") {
			resp.send(Http::Code::Ok, std::to_string(std::next(obstSens->getSensorsList().begin(), 1)->getDistance()));
		} else if (req.resource() == "/getSensor/2") {
			resp.send(Http::Code::Ok, std::to_string(std::next(obstSens->getSensorsList().begin(), 2)->getDistance()));
		} else if (req.resource() == "/getLane") {
			resp.send(Http::Code::Ok, std::to_string(int(keepLR - 4.0) / -4));
		} else if (req.resource() == "/getNumLanes") {
			resp.send(Http::Code::Ok, std::to_string(3));
		}
	};
};

static tTrack	*curTrack;

static void initTrack(int index, tTrack* track, void *carHandle, void **carParmHandle, tSituation *s); 
static void newrace(int index, tCarElt* car, tSituation *s); 
static void drive(int index, tCarElt* car, tSituation *s); 
static void endrace(int index, tCarElt *car, tSituation *s);
static void shutdown(int index);
static int  InitFuncPt(int index, void *pt); 


Http::Endpoint* server;

/* 
 * Module entry point  
 */ 
extern "C" int 
argos_AI1(tModInfo *modInfo) 
{
    memset(modInfo, 0, 10*sizeof(tModInfo));

    modInfo->name    = "argos_AI1";		/* name of the module (short) */
    modInfo->desc    = "";	/* description of the module (can be long) */
    modInfo->fctInit = InitFuncPt;		/* init function */
    modInfo->gfId    = ROB_IDENT;		/* supported framework version */
    modInfo->index   = 1;

    return 0; 
} 

/* Module interface initialization. */
static int 
InitFuncPt(int index, void *pt) 
{ 
    tRobotItf *itf  = (tRobotItf *)pt; 

    itf->rbNewTrack = initTrack; /* Give the robot the track view called */ 
				 /* for every track change or new race */ 
    itf->rbNewRace  = newrace; 	 /* Start a new race */
    itf->rbDrive    = drive;	 /* Drive during race */
    itf->rbPitCmd   = NULL;
    itf->rbEndRace  = endrace;	 /* End of the current race */
    itf->rbShutdown = shutdown;	 /* Called before the module is unloaded */
    itf->index      = index; 	 /* Index used if multiple interfaces */
    return 0; 
} 

/* Called for every track change or new race. */
static void  
initTrack(int index, tTrack* track, void *carHandle, void **carParmHandle, tSituation *s) 
{ 
    curTrack = track;
    *carParmHandle = NULL; 
} 

/* Start a new race. */
static void  
newrace(int index, tCarElt* car, tSituation *s) 
{
	Address addr(Ipv4::any(), Port(9080));
	auto opts = Http::Endpoint::options().threads(1);

	server = new Http::Endpoint(addr);
	server->init(opts);
	server->setHandler(Http::make_handler<RESTHandler>());
	server->serveThreaded();

	obstSens = new ObstacleSensors(NULL, car);
	// front
	obstSens->addSensor(car, 0, car->_dimension_x/2, 0, 20);
	// rear right
	obstSens->addSensor(car, 135, -car->_dimension_x/2, -car->_dimension_y/2, 20);
	// rear left
	obstSens->addSensor(car, -135, -car->_dimension_x/2, car->_dimension_y/2, 20);
} 

/* Drive during race. */
static void  
drive(int index, tCarElt* car, tSituation *s) 
{ 
    memset((void *)&car->ctrl, 0, sizeof(tCarCtrl)); 
    car->ctrl.brakeCmd = 1.0; /* all brakes on ... */ 
    /*  
     * add the driving code here to modify the 
     * car->_steerCmd 
     * car->_accelCmd 
     * car->_brakeCmd 
     * car->_gearCmd 
     * car->_clutchCmd 
     */
	obstSens->sensors_update(s);

	std::list<SingleObstacleSensor> sensors_list = obstSens->getSensorsList();
	for(std::list<SingleObstacleSensor>::iterator it = sensors_list.begin(); it != sensors_list.end(); ++it) {
		printf("Sensors #%d: %f\n", std::distance(sensors_list.begin(), it), (*it).getDistance());
	}

	float angle;
	const float SC = 1.0;

	angle = RtTrackSideTgAngleL(&(car->_trkPos)) - car->_yaw;
	NORM_PI_PI(angle); // put the angle back in the range from -PI to PI
	angle -= SC*(car->_trkPos.toMiddle+keepLR)/car->_trkPos.seg->width;

	// set up the values to return
	car->ctrl.steer = angle / car->_steerLock;
	car->ctrl.gear = 1;

        if (car->_speed_x>desired_speed) {
           car->ctrl.brakeCmd=0.5;
           car->ctrl.accelCmd=0.0;
        }
        else if  (car->_speed_x<desired_speed) {
           car->ctrl.accelCmd=0.5;
           car->ctrl.brakeCmd=0.0;
        }
}

/* End of the current race */
static void
endrace(int index, tCarElt *car, tSituation *s)
{
}

/* Called before the module is unloaded */
static void
shutdown(int index)
{
}

