/***************************************************************************

    file                 : sumo.cpp
    created              : Mo 07. Dec 11:00:00 CEST 2015
    copyright            : (C) 2015 Alexander Weidinger

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

#include <sys/socket.h>
#include <arpa/inet.h>

#include <humandriver.h>
#include <ssd2.pb.h>

#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>

using namespace google::protobuf::io;

GfLogger* PLogSUMO = 0;

static HumanDriver robot("human");

static tTrack	*curTrack;

static void initTrack(int index, tTrack* track, void *carHandle, void **carParmHandle, tSituation *s);
static void newrace(int index, tCarElt* car, tSituation *s);
static void dataexchange(int index, tCarElt* car, tSituation *s);
static void drive_at(int index, tCarElt* car, tSituation *s);
static void drive_mt(int index, tCarElt* car, tSituation *s);
static void endrace(int index, tCarElt *car, tSituation *s);
static void shutdown(int index);
static int  InitFuncPt(int index, void *pt);

/* new SD2 api */
extern "C" int moduleWelcome(const tModWelcomeIn* welcomeIn, tModWelcomeOut* welcomeOut) {
  welcomeOut->maxNbItf = 1;
  return 0;
}
extern "C" int moduleInitialize(tModInfo *ModInfo) {
  PLogSUMO = GfLogger::instance("SUMO");
  memset(ModInfo, 0, sizeof(tModInfo));

  ModInfo->name    = "SUMO";		/* name of the module (short) */
  ModInfo->desc    = "";	/* description of the module (can be long) */
  ModInfo->fctInit = InitFuncPt;		/* init function */
  ModInfo->gfId    = ROB_IDENT;		/* supported framework version */
  ModInfo->index   = 1;

  return 0;
}
extern "C" int moduleTerminate() {
  return 0;
}

/* Module interface initialization. */
static int InitFuncPt(int index, void *pt) {
  tRobotItf *itf  = (tRobotItf *)pt;

  robot.init_context(index);

  itf->rbNewTrack = initTrack; /* Give the robot the track view called */
  /* for every track change or new race */
  itf->rbNewRace  = newrace; 	 /* Start a new race */
  itf->rbDrive    = robot.uses_at(index) ? drive_at : drive_mt; /* Drive during race */
  itf->rbEndRace  = endrace;	 /* End of the current race */
  itf->rbShutdown = shutdown;	 /* Called before the module is unloaded */
  itf->index      = index; 	 /* Index used if multiple interfaces */

  return 0;
}

/* Called for every track change or new race. */
static void initTrack(int index, tTrack* track, void *carHandle, void **carParmHandle, tSituation *s) {
  curTrack = track;
  *carParmHandle = NULL;

  robot.init_track(index, track, carHandle, carParmHandle, s);
}

static int sockfd;

/* Start a new race. */
static void newrace(int index, tCarElt* car, tSituation *s) {
  PLogSUMO->debug("Initializing SUMO!\n");

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(struct sockaddr_in));

  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  server_addr.sin_port = htons(2000);

  connect(sockfd, (struct sockaddr*)&server_addr, sizeof(struct sockaddr_in));

  robot.new_race(index, car, s);
}

static void dataexchange(int index, tCarElt* car, tSituation *s) {
  /* calculate vehicle yaw in degrees
   * http://answers.ros.org/question/141366/convert-the-yaw-euler-angle-into-into-the-range-0-360/
   */
  float yaw = car->_yaw * 180.0 / M_PI;
  if(yaw < 0) yaw += 360.0;

  /* construct message vehicle */
  vehicle veh;
  veh.set_name("veh0");

  veh.set_dist(car->race.distRaced);
  veh.set_angle(yaw);
  veh.set_length(curTrack->length);

  position *pos = veh.mutable_pos();
  pos->set_x(car->_pos_X);
  pos->set_y(car->_pos_Y);
  pos->set_z(car->_pos_Z);

  firstSegment *fs = veh.mutable_fs();
  fs->set_lx(curTrack->seg[0].vertex[0].x);
  fs->set_ly(curTrack->seg[0].vertex[0].y);
  fs->set_rx(curTrack->seg[0].vertex[1].x);
  fs->set_ry(curTrack->seg[0].vertex[1].y);

  int size = veh.ByteSize();
  void *data = malloc(size);
  veh.SerializeToArray(data, size);

  /* send length of packet and packet */
  send(sockfd, &size, sizeof(int), 0);
  send(sockfd, data, size, 0);
}

static void drive_mt(int index, tCarElt* car, tSituation *s) {
  dataexchange(index, car, s);
  robot.drive_mt(index, car, s);
}

static void drive_at(int index, tCarElt* car, tSituation *s) {
  dataexchange(index, car, s);
  robot.drive_at(index, car, s);
}

/* End of the current race */
static void endrace(int index, tCarElt *car, tSituation *s) {
}

/* Called before the module is unloaded */
static void shutdown(int index) {
  robot.shutdown(index);
}
