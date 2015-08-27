/***************************************************************************

file                 : autonet.cpp
created              : Sat Nov 13 23:19:31 EST 2010
copyright            : (C) 2002 Utsav

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

#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cmath>

#include <algorithm>

#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>

#include <tgf.h>
#include <track.h>
#include <car.h>
#include <raceman.h>
#include <robottools.h>
#include <robot.h>
#include <gpsSensor.h>
#include <positionTracker.h>

#include "serial.h"
#include "timer.h"

struct SensorData
{
    bool isPositionTracked;
    bool isSpeedTracked;
    vec2 leadPos;
    vec2 ownPos;
    vec2 cornerFrontRight;
    vec2 cornerFrontLeft;
    vec2 cornerRearRight;
    vec2 cornerRearLeft;
    float leadSpeed;
    float ownSpeed;
    int curGear;
};

struct CommandData
{
    float steer;
    float accel;
    float brakeFL; //Front left
    float brakeFR; //Front right
    float brakeRL; //Rear left
    float brakeRR; //Rear right
    int gear;
    //bool followModeCmd;
};

static tTrack *curTrack;
static int serialfd;

static void initTrack(int index, tTrack* track, void *carHandle, void **carParmHandle, tSituation *s);
static void newrace(int index, tCarElt* car, tSituation *s);
static void drive(int index, tCarElt* car, tSituation *s);
static void endrace(int index, tCarElt *car, tSituation *s);
static void shutdown(int index);
static int  InitFuncPt(int index, void *pt);

static bool updateFollowMode();


static tCarElt *car;
static CommandData g_cd = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0};
static float clutch = 0;
static int followCmd = 0;

static bool followMode = false;
static int lastFollowModeCmd = 0;

static SensorData g_sd = {false, false, vec2(0,0), vec2(0,0), vec2(0,0), vec2(0,0), vec2(0,0), vec2(0,0), 0, 0, 0};
// distance threshold 50 m
static PositionTracker g_tracker(50.0);
static GPSSensor gps = GPSSensor();



static timer_t timerid;
volatile static bool signalStopSendData = false;
volatile static bool serialDataStopped = true;

/*
 * Module entry point
 */
extern "C" int autonet(tModInfo *modInfo)
{
    memset(modInfo, 0, 10*sizeof(tModInfo));

    modInfo->name    = "autonet";		/* name of the module (short) */
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

void serialData(int signum)
{
    uint8_t a[512]; // Large buffer to read in all pending bytes from serial
    uint8_t calcChkSum = 0;
    // sizeof(SensorData)
    uint8_t b[71]; // 2 + 68 + 1 (checksum vals + sizeof(SensorData) + checksum)
    int i;
    int cdSize = sizeof(CommandData);
    int sdSize = sizeof(SensorData);

    if(signalStopSendData)
    {
        serialDataStopped = true;
        return;
    }

    if(serialPortRead(serialfd, a, 1) == 1) {
        if(a[0] == 0xAA) {
            if(serialPortRead(serialfd, a, 1) == 1) {
                if(a[0] == 0xCC) {
                    calcChkSum = 0xAA ^ 0xCC;
                    if(serialPortRead(serialfd, a, 512) >= cdSize + 1) {

                        for(i = 0; i < cdSize; i++) {
                            calcChkSum = calcChkSum ^ a[i];
                        }
                        if(calcChkSum == a[cdSize]) {
                            std::memcpy(reinterpret_cast<uint8_t*>(&g_cd), a, cdSize);
                            //lastFollowModeCmd = followCmd;
                            //followCmd = g_cd.followModeCmd;
                        }
                    }
                }
            }
        }
    }
    b[0] = 0xAA;
    b[1] = 0xCC;
    std::memcpy(&b[2], reinterpret_cast<uint8_t*>(&g_sd), sdSize);

    calcChkSum = 0;
    for(i = 0; i < sdSize + 2; i++){
        calcChkSum = calcChkSum ^ b[i];
    }
    b[2 + sdSize] = calcChkSum;

    serialPortWrite(serialfd, b, 2 + sdSize + 1);

    updateFollowMode();
}

/* Start a new race. */
static void newrace(int index, tCarElt* car_local, tSituation *s)
{
    serialfd = serialPortOpen("/dev/ttyUSB0", 115200);
    car = car_local;
    timerid = timerInit(serialData, 2000000);
    serialDataStopped = false;
    enableTimerSignal();
}

/* Drive during race. */
static void drive(int index, tCarElt* car, tSituation *s)
{
    memset((void *)&car->ctrl, 0, sizeof(tCarCtrl));

    gps.update(car);
    vec2 myPos = gps.getPosition();
    printf("Autonet's position according to GPS is (%f, %f)\n", myPos.x, myPos.y);

    //printf("steer: %f, yaw_rate: %f\n", angle, car->_yaw_rate);
#if 1
    const float SC = 1.0;
    /* Auto-steer */
    tdble angle = RtTrackSideTgAngleL(&(car->_trkPos)) - car->_yaw;
    NORM_PI_PI(angle); // put the angle back in the range from -PI to PI
    angle -= SC*car->_trkPos.toMiddle/car->_trkPos.seg->width;
    angle = angle/car->_steerLock;
    tdble accel = getSpeedDepAccel(car->_speed_x, 1.0, 0.1, 8, 18, 20);
                    // 0   60  100 150 200 250 km/h
    int gear = getSpeedDepGear(car->_speed_x, car->_gearCmd);

    tdble brake = 0.0;
#endif

    //if(followMode)
    if(true)
    {
        g_tracker.updatePosition(car, s, curTrack);

        // Update sensor data
        g_sd.isPositionTracked = g_tracker.isPositionTracked();
        g_sd.isSpeedTracked = g_tracker.isSpeedTracked();
        g_sd.leadPos = g_tracker.getCurLeadPos();
        g_sd.ownPos = vec2(car->_pos_X, car->_pos_Y);
        g_sd.cornerFrontRight = vec2(car->_corner_x(FRNT_RGT), car->_corner_y(FRNT_RGT));
        g_sd.cornerFrontLeft = vec2(car->_corner_x(FRNT_LFT), car->_corner_y(FRNT_LFT));
        g_sd.cornerRearRight = vec2(car->_corner_x(REAR_RGT), car->_corner_y(REAR_RGT));
        g_sd.cornerRearLeft = vec2(car->_corner_x(REAR_LFT), car->_corner_y(REAR_LFT));
        g_sd.leadSpeed = g_tracker.getSpeed(s->deltaTime);
        g_sd.ownSpeed = car->_speed_x;
        g_sd.curGear = car->_gearCmd;
    }

    // if(followMode && g_tracker.isPositionTracked())
    if(true && g_tracker.isPositionTracked())
    {
        car->_steerCmd = g_cd.steer;
        car->_accelCmd = g_cd.accel;
        car->_singleWheelBrakeMode = 1;
        car->_brakeFLCmd = g_cd.brakeFL;
        car->_brakeFRCmd = g_cd.brakeFR;
        car->_brakeRLCmd = g_cd.brakeRL;
        car->_brakeRRCmd = g_cd.brakeRR;
        car->_brakeCmd = (g_cd.brakeFL + g_cd.brakeFR + g_cd.brakeRL + g_cd.brakeRR) / 4.0; // For display in speed dreams
        car->_gearCmd = g_cd.gear;
    }
    else // If not following anybody -> use algorithm for following track
    {
        // set the values
        car->_steerCmd = angle;
        car->_accelCmd = accel;
        car->_singleWheelBrakeMode = 0;
        car->_brakeCmd = brake;

        car->_gearCmd = gear;
        car->_clutchCmd = clutch;
    }
}

/* End of the current race */
static void endrace(int index, tCarElt *car, tSituation *s)
{
    printf("endRace\n");
}

/* Called before the module is unloaded */
static void shutdown(int index)
{
    signalStopSendData = true;
    while(!serialDataStopped);
    disableTimerSignal();
    timerEnd(timerid);
    serialPortClose(serialfd);
    printf("Done with shutdown\n");
}

// Checks values of the transferred followMode signals and switches the followMode if the signal switches from enabled to disabled
// Compares to the behavior when releasing a keyboard button
static bool updateFollowMode()
{
    if(lastFollowModeCmd == 1 && followCmd == 0) // follow mode signal is disabled
    {
        followMode = !followMode; // switch follow mode
    }
    return followMode;
}
