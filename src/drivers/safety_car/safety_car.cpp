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
#include <math.h>        // sqrt()

#include <tgf.h>         // NORM_PI_PI()
#include <modinfo.h>     // tModInfo struct typedef
#include <car.h>         // tCarElt struct typedef
#include <track.h>       // tTrack struct typedef
#include <raceman.h>     // tSituation struct typedef
#include <robottools.h>  // RtTrackSideTgAngleL()
#include <robot.h>       // RobotItf struct typedef




static void initTrack(int index, tTrack* track, void *carHandle, void **carParmHandle, tSituation *s);
static void newrace(int index, tCarElt* car, tSituation *s);
static void resumerace(int index, tCarElt* car, tSituation *s);
static void drive(int index, tCarElt* car, tSituation *s);
static void endrace(int index, tCarElt *car, tSituation *s);
static void shutdown(int index);
static int  InitFuncPt(int index, void *pt);


static tCarElt *car;
static tTrack *curTrack;

/*
 * Module entry point
 */
extern "C" int safety_car(tModInfo *modInfo)
{
    memset(modInfo, 0, 10*sizeof(tModInfo));

    modInfo->name    = "safety_car";		/* name of the module (short) */
    modInfo->desc    = "";	            /* description of the module (can be long) */
    modInfo->fctInit = InitFuncPt;		/* init function */
    modInfo->gfId    = ROB_IDENT;		/* supported framework version */
    modInfo->index   = 1;

    return 0;
}

/* Module interface initialization. */
static int InitFuncPt(int index, void *pt)
{
    tRobotItf *itf  = (tRobotItf *)pt;

    itf->rbNewTrack = initTrack; /* Give the robot the track view called */
                                 /* for every track change or new race */
    itf->rbNewRace  = newrace; 	 /* Start a new race */
    itf->rbResumeRace = resumerace;
    itf->rbDrive    = drive;	 /* Drive during race */
    itf->rbPitCmd   = NULL;
    itf->rbEndRace  = endrace;	 /* End of the current race */
    itf->rbShutdown = shutdown;	 /* Called before the module is unloaded */
    itf->index      = index; 	 /* Index used if multiple interfaces */
    return 0;
}

/* Called for every track change or new race. */
static void initTrack(int index, tTrack* track, void *carHandle, void **carParmHandle, tSituation *s)
{
    curTrack = track;
    *carParmHandle = NULL;
}

/* Start a new race. */
static void newrace(int index, tCarElt* car_local, tSituation *s)
{
    car = car_local;
}

/* Start a new race. */
static void resumerace(int index, tCarElt* car_local, tSituation *s)
{
    printf("Resume\n");
}

/* Drive during race. */
static void drive(int index, tCarElt* car, tSituation *s)
{
    memset((void *)&car->ctrl, 0, sizeof(tCarCtrl));

    //printf("steer: %f, yaw_rate: %f\n", angle, car->_yaw_rate);

    const tdble SC = 1.0;
    /* Auto-steer */
    tdble angle = RtTrackSideTgAngleL(&(car->_trkPos)) - car->_yaw;
    NORM_PI_PI(angle); // put the angle back in the range from -PI to PI
    angle -= SC * car->_trkPos.toMiddle/car->_trkPos.seg->width;
    angle = angle/car->_steerLock;

    // car, maxAccel, startAccel, incUntilSpeed, maxUntilSpeed, decUntilSpeed
    tdble accel = getSpeedDepAccel(car->_speed_x, 1.0, 0.3, 6, 18, 20);

    int gear = 1;

    // set the values
    car->_steerCmd = angle;
    car->_gearCmd = gear;
    car->_accelCmd = accel;

    // brake commands
    car->_brakeCmd = 0.0; // Just for display in TORCS

    car->_clutchCmd = 0.0;
}

/* End of the current race */
static void endrace(int index, tCarElt *car, tSituation *s)
{
    printf("endRace\n");
}

/* Called before the module is unloaded */
static void shutdown(int index)
{
    printf("Done with shutdown\n");
}
