/***************************************************************************

    file                 : ACC.cpp
    created              : Do 8. Feb 12:48:50 CET 2018
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

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>

#include <positionTracker.h>

#include <SensorDataOut.pb.h>
#include <CommandDataIn.pb.h>

#define listen_addr "0.0.0.0"
#define listen_port 9002

GfLogger* PLogACC = 0;

static tTrack	*curTrack;

static void initTrack(int index, tTrack* track, void *carHandle, void **carParmHandle, tSituation *s); 
static void newrace(int index, tCarElt* car, tSituation *s); 
static void drive(int index, tCarElt* car, tSituation *s); 
static void endrace(int index, tCarElt *car, tSituation *s);
static void shutdown(int index);
static int  InitFuncPt(int index, void *pt);

static int serverSock = -1, clientSock = -1;

static PositionTracker g_tracker(60.0);

uint32_t msg_len;
std::string sdo_str;

/* standard SD interface */
extern "C" int moduleWelcome(const tModWelcomeIn* welcomeIn, tModWelcomeOut* welcomeOut) {
	welcomeOut->maxNbItf = 1;

	return 0;
}

extern "C" int moduleInitialize(tModInfo *ModInfo) {
	PLogACC = GfLogger::instance("ACC");
	memset(ModInfo, 0, sizeof(tModInfo));
	
	ModInfo->name    = "ACC";      /* name of the module (short) */
	ModInfo->desc    = "";         /* description of the module (can be long) */
	ModInfo->fctInit = InitFuncPt; /* init function */
	ModInfo->gfId    = ROB_IDENT;  /* supported framework version */
	ModInfo->index   = 1;

	return 0;
}

extern "C" int moduleTerminate() {
	return 0;
}


/* Module interface initialization. */
static int InitFuncPt(int index, void *pt) { 
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
static void initTrack(int index, tTrack* track, void *carHandle, void **carParmHandle, tSituation *s) { 
    curTrack = track;
    *carParmHandle = NULL;
} 

/* Start a new race. */
static void newrace(int index, tCarElt* car, tSituation *s) {
	if ((serverSock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		PLogACC->error("socket failed! %s\n", strerror(errno));
	}

	struct sockaddr_in srv_addr, cli_addr;
	bzero(&srv_addr, sizeof(srv_addr));
	srv_addr.sin_family = AF_INET;
	srv_addr.sin_addr.s_addr = inet_addr(listen_addr);
	srv_addr.sin_port = htons(listen_port);

	if (bind(serverSock, (struct sockaddr *)&srv_addr, sizeof(srv_addr)) == -1) {
		PLogACC->error("bind failed! %s\n", strerror(errno));
	}

	/* wait for S/A VM to connect */
	if (listen(serverSock, 5) == -1) {
		PLogACC->error("listen failed! %s\n", strerror(errno));
	}

	socklen_t cli_len = sizeof(cli_addr);
	if ((clientSock = accept(serverSock, (struct sockaddr *)&cli_addr, &cli_len)) == -1) {
		PLogACC->error("accept failed! %s\n", strerror(errno));
	}

	/* Disable Nagle's algorithm */
	int flag = 1;
   	int result = setsockopt(clientSock, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));
	if (result == -1) {
		PLogACC->error("setsockopt failed! %s\n", strerror(errno));
	}
}

static void readAllBytes(void *buf, int socket, unsigned int size) {
	int offset = 0;
	int ret = 0;

	do {
		ret = read(socket, buf + offset, size - offset);
		if (ret == -1) {
			PLogACC->error("read failed: %s", strerror(errno));
		} else {
			offset += ret;
		}
	} while(offset != size);
}

/* Drive during race. */
static void drive(int index, tCarElt* car, tSituation *s) {
	/* ACC */
	g_tracker.updatePosition(car, s, curTrack);

	/* SensorDataOut */
	protobuf::SensorDataOut sdo;
	sdo.set_ispositiontracked(g_tracker.isPositionTracked());
	sdo.set_isspeedtracked(g_tracker.isSpeedTracked());
	protobuf::SensorDataOut_vec2 *leadpos = sdo.mutable_leadpos();
	leadpos->set_x(g_tracker.getCurLeadPos().x);
	leadpos->set_y(g_tracker.getCurLeadPos().y);
	protobuf::SensorDataOut_vec2 *ownpos = sdo.mutable_ownpos();
	ownpos->set_x(car->_pos_X);
	ownpos->set_y(car->_pos_Y);
	protobuf::SensorDataOut_vec2 *cornerfrontright = sdo.mutable_cornerfrontright();
	cornerfrontright->set_x(car->_corner_x(FRNT_RGT));
	cornerfrontright->set_y(car->_corner_y(FRNT_RGT));
	protobuf::SensorDataOut_vec2 *cornerfrontleft = sdo.mutable_cornerfrontleft();
	cornerfrontleft->set_x(car->_corner_x(FRNT_LFT));
	cornerfrontleft->set_y(car->_corner_y(FRNT_LFT));
	protobuf::SensorDataOut_vec2 *cornerrearright = sdo.mutable_cornerrearright();
	cornerrearright->set_x(car->_corner_x(REAR_RGT));
	cornerrearright->set_y(car->_corner_y(REAR_RGT));
	protobuf::SensorDataOut_vec2 *cornerrearleft = sdo.mutable_cornerrearleft();
	cornerrearleft->set_x(car->_corner_x(REAR_LFT));
	cornerrearleft->set_y(car->_corner_y(REAR_LFT));
	sdo.set_leadspeed(g_tracker.getSpeed(s->deltaTime));
	sdo.set_ownspeed(car->_speed_x);
	sdo.set_curgear(car->_gearCmd);
	sdo.set_steerlock(car->_steerLock);
	sdo.set_enginerpm(car->_enginerpm);
	sdo.set_enginerpmmax(car->_enginerpmMax);
	sdo.set_steer(car->_steerCmd);
	sdo.set_brakefl(car->_brakeFLCmd);
	sdo.set_brakefr(car->_brakeFRCmd);
	sdo.set_brakerl(car->_brakeRLCmd);
	sdo.set_brakerr(car->_brakeRRCmd);

	/* send SensorDataOut */

	sdo_str.clear();                       // clear sdo_str
	msg_len = htonl(sdo.ByteSizeLong());   // get sdo length in bytes (network byte order)
	sdo_str.append((const char*)&msg_len,
				   sizeof(msg_len));       // append message length
	sdo.AppendToString(&sdo_str);          // append sdo
	msg_len = sdo_str.size();              // get #bytes to transmit

	int ret = write(clientSock, sdo_str.c_str(), msg_len);
	if (ret == -1) {
		PLogACC->error("write sdo failed! %s\n", strerror(errno));
	} else if (ret != msg_len) {
		PLogACC->error("write sdo failed to send complete message! %d vs. %d\n",
					   ret,
					   msg_len);
	}

	/* reset ctrl commands */
	memset((void *)&car->ctrl, 0, sizeof(tCarCtrl));

	/* SensorDataIn */
	protobuf::CommandDataIn cdi;

	readAllBytes(&msg_len, clientSock, sizeof(msg_len));
	msg_len = ntohl(msg_len);

	char buffer[msg_len] = { '\0' };
	readAllBytes(buffer, clientSock, msg_len);

	cdi.ParseFromArray(buffer, msg_len);

	car->_steerCmd = cdi.steer();
	car->_accelCmd = cdi.accel();
	car->_singleWheelBrakeMode = 1;
	car->_brakeFLCmd = cdi.brakefl();
	car->_brakeFRCmd = cdi.brakefr();
	car->_brakeRLCmd = cdi.brakerl();
	car->_brakeRRCmd = cdi.brakerr();
	car->_brakeCmd = cdi.brakefl() + cdi.brakefr() + cdi.brakerl() + cdi.brakerr() / 4.0;
	car->_gearCmd = cdi.gear();
}

/* End of the current race */
static void endrace(int index, tCarElt *car, tSituation *s) {
}

/* Called before the module is unloaded */
static void shutdown(int index) {
}
