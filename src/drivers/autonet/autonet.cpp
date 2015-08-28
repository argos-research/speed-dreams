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

#include "socket.h"
#include "timer.h"

struct SensorDataOut
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

struct CommandDataIn
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

struct SensorDataIn
{
    float engineTemp;
    float engineRPM;
};

static void initTrack(int index, tTrack* track, void *carHandle, void **carParmHandle, tSituation *s);
static void newrace(int index, tCarElt* car, tSituation *s);
static void drive(int index, tCarElt* car, tSituation *s);
static void endrace(int index, tCarElt *car, tSituation *s);
static void shutdown(int index);
static int  InitFuncPt(int index, void *pt);

static bool updateFollowMode();

static tTrack *curTrack;
static tCarElt *car;
static CommandDataIn g_cdIn = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0};
static SensorDataIn g_sdIn = {0.0, 0.0};
static float clutch = 0;
static int followCmd = 0;

static bool followMode = false;
static int lastFollowModeCmd = 0;

static SensorDataOut g_sdOut = {false, false, vec2(0,0), vec2(0,0), vec2(0,0), vec2(0,0), vec2(0,0), vec2(0,0), 0, 0, 0};
// distance threshold 50 m
static PositionTracker g_tracker(50.0);
static GPSSensor gps = GPSSensor();

static timer_t timerid;
static int sockfd;
static int newsockfd;


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

void socketData(int signum)
{
    uint8_t a[512]; // Large buffer to read in all pending bytes from socket
    uint8_t calcChkSum = 0;
    // sizeof(SensorData)
    uint8_t b[71]; // 2 + 68 + 1 (checksum vals + sizeof(SensorData) + checksum)
    int i;
    int cdInSize = sizeof(CommandDataIn);
    int sdInSize = sizeof(SensorDataIn);
    int sdOutSize = sizeof(SensorDataOut);

    if(socketRead(newsockfd, a, 1) == 1) {
        if(a[0] == 0xAA) {
            if(socketRead(newsockfd, a, 1) == 1) {
                if(a[0] == 0xCC) {
                    calcChkSum = 0xAA ^ 0xCC;
                    if(socketRead(newsockfd, a, 512) >= cdInSize + sdInSize + 1) {

                        for(i = 0; i < cdInSize + sdInSize; i++) {
                            calcChkSum = calcChkSum ^ a[i];
                        }
                        if(calcChkSum == a[cdInSize + sdInSize]) {
                            std::memcpy(reinterpret_cast<uint8_t*>(&g_cdIn), a, cdInSize);
                            //lastFollowModeCmd = followCmd;
                            //followCmd = g_cd.followModeCmd;
                            std::memcpy(reinterpret_cast<uint8_t*>(&g_sdIn), &a[cdInSize], sdInSize);
                        }
                    }
                }
            }
        }
    }

    b[0] = 0xAA;
    b[1] = 0xCC;
    std::memcpy(&b[2], reinterpret_cast<uint8_t*>(&g_sdOut), sdOutSize);

    calcChkSum = 0;
    for(i = 0; i < sdOutSize + 2; i++){
        calcChkSum = calcChkSum ^ b[i];
    }
    b[2 + sdOutSize] = calcChkSum;

    socketWrite(newsockfd, b, 2 + sdOutSize + 1);

    //updateFollowMode();
}

/* Start a new race. */
static void newrace(int index, tCarElt* car_local, tSituation *s)
{
    sockfd = createSocket(66666);
    newsockfd = waitForConnection(sockfd);
    car = car_local;
    timerid = timerInit(socketData, 2000000);
    enableTimerSignal();
}

/* Drive during race. */
static void drive(int index, tCarElt* car, tSituation *s)
{
    memset((void *)&car->ctrl, 0, sizeof(tCarCtrl));

    //Print gps sensor data
    gps.update(car);
    vec2 myPos = gps.getPosition();
    printf("Autonet's position according to GPS is (%f, %f)\n", myPos.x, myPos.y);

    //Print sensor data
    printf("Engine Temperature: %f\n", g_sdIn.engineTemp);
    printf("Engine RPM: %f\n", g_sdIn.engineRPM);
    printf("Current Gear: %d\n", g_cdIn.gear);
    printf("Current Acceleration: %f\n", g_cdIn.accel);
    printf("Current Brake Acceleration: %f\n", (g_cdIn.brakeFL + g_cdIn.brakeFR + g_cdIn.brakeRL + g_cdIn.brakeRR) / 4.0);
    printf("Current Speed: %f\n", car->_speed_x);
    printf("Current Steering Angle: %f\n", RAD2DEG(g_cdIn.steer));

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
        g_sdOut.isPositionTracked = g_tracker.isPositionTracked();
        g_sdOut.isSpeedTracked = g_tracker.isSpeedTracked();
        g_sdOut.leadPos = g_tracker.getCurLeadPos();
        g_sdOut.ownPos = vec2(car->_pos_X, car->_pos_Y);
        g_sdOut.cornerFrontRight = vec2(car->_corner_x(FRNT_RGT), car->_corner_y(FRNT_RGT));
        g_sdOut.cornerFrontLeft = vec2(car->_corner_x(FRNT_LFT), car->_corner_y(FRNT_LFT));
        g_sdOut.cornerRearRight = vec2(car->_corner_x(REAR_RGT), car->_corner_y(REAR_RGT));
        g_sdOut.cornerRearLeft = vec2(car->_corner_x(REAR_LFT), car->_corner_y(REAR_LFT));
        g_sdOut.leadSpeed = g_tracker.getSpeed(s->deltaTime);
        g_sdOut.ownSpeed = car->_speed_x;
        g_sdOut.curGear = car->_gearCmd;
    }

    // if(followMode && g_tracker.isPositionTracked())
    if(true && g_tracker.isPositionTracked())
    {
        car->_steerCmd = g_cdIn.steer;
        car->_accelCmd = g_cdIn.accel;
        car->_singleWheelBrakeMode = 1;
        car->_brakeFLCmd = g_cdIn.brakeFL;
        car->_brakeFRCmd = g_cdIn.brakeFR;
        car->_brakeRLCmd = g_cdIn.brakeRL;
        car->_brakeRRCmd = g_cdIn.brakeRR;
        car->_brakeCmd = (g_cdIn.brakeFL + g_cdIn.brakeFR + g_cdIn.brakeRL + g_cdIn.brakeRR) / 4.0; // For display in speed dreams
        car->_gearCmd = g_cdIn.gear;
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
    disableTimerSignal();
    timerEnd(timerid);
    closeSocket(sockfd, newsockfd);
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
