/***************************************************************************

    created              : Sat Mar 18 23:16:38 CET 2000
    copyright            : (C) 2000 by Eric Espie
    email                : torcs@free.fr
    version              : $Id$

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

    @author	<a href=mailto:torcs@free.fr>Eric Espie</a>
    @version	$Id$
*/

/*
 * 2013/3/21 Tom Low-Shang
 *
 * Merged original contents of:
 *
 * drivers/human/human.cpp,
 * drivers/human/human.h,
 * drivers/human/pref.cpp,
 * drivers/human/pref.h,
 *
 * except for CMD_* defines which are in interfaces/playerpref.h.
 *
 * Functions have been renamed for the class interface. Where possible,
 * function bodies have not been not been changed, except for indentation.
 */

// Flags for selecting the "binary control steering code" variant :
// * Old variant
#define OLD 0
// * Jepz variant
#define JEPZ 1
// * JPM variant
#define JPM 2
// * if neither JEPZ or JPM variant are selected, the old code is used.
#define BINCTRL_STEERING JPM

#include <map>
#include <vector>
#include <string>

#include <tgfclient.h>
#include <portability.h>
#include "robottools.h"	//Rt*
#include <robot.h>
#include <playerpref.h>
#include <car.h>

#include "humandriver.h"

typedef enum { eTransAuto, eTransSeq, eTransGrid, eTransHbox } eTransmission;

typedef enum { eRWD, eFWD, e4WD } eDriveTrain;

typedef struct {
    const char	*name;		/* Name of the control */
    int		type;		/* Type of control (analog, joy button, keyboard) */
    int		val;		/* control index */
    const char	*minName;	/* Name of the min attribute */
    float	min;		/* min value for analog control */
    float	minVal;		/* another min value for analog control*/
    const char	*maxName;	/* Name of the max attribute */
    float	max;		/* max value for analog control */
    const char	*sensName;	/* Name of the sens attribute */
    float	sens;		/* sensitivity of control */
    const char	*powName;	/* Name of the pow attribute */
    float	pow;		/* power of curve command = pow(control, power) */
    const char	*spdSensName;	/* Name of the speed sensitivity attribute */
    float	spdSens;	/* speed sensitivity */
    const char	*deadZoneName;	/* Name of the dead zone attribute */
    float	deadZone;	/* dead zone (% of the available zone) */
} tControlCmd;

typedef struct HumanContext
{
    int			nbPitStops;
    int			lastPitStopLap;
    bool 		autoReverseEngaged;
    tdble		shiftThld[MAX_GEARS+1];
    tdble		gear;
    tdble		distToStart;
    float		clutchTime;
    float		maxClutchTime;
    float		clutchdelay;
    float		antiLock;
    float		antiSlip;
    int			lap;
    float		prevLeftSteer;
    float		prevRightSteer;
    float		paccel;
    float		pbrake;
    bool		manual;
    eTransmission	transmission;
    int			nbPitStopProg;
    bool		paramAsr;
    bool		paramAbs;
    bool		relButNeutral;
    bool		seqShftAllowNeutral;
    bool		seqShftAllowReverse;
    bool		autoReverse;
    eDriveTrain	driveTrain;
    bool		autoClutch;
    tControlCmd		*cmdControl;
    bool		mouseControlUsed;
    int			lightCmd;

    // simuV4 ...
    bool		useESP;
    float		brakeRep;
    float		brakeCorr;
    float		brakeFront;
    float		brakeRear;
    float		brakeLeft;
    float		brakeRight;
    // ... simuV4

} tHumanContext;

static const int FuelReserve = 3;
static const tdble MaxFuelPerMeter = 0.0008;	// [kg/m] fuel consumption.

static void updateKeys(void);
static void SetFuelAtRaceStart(tTrack *track, void **carParmHandle, tSituation *s, int idx);
static char	sstring[1024];
static char	buf[1024];

static tTrack *curTrack;

static const float color[] = {0.0, 0.0, 1.0, 1.0};

static bool joyPresent = false;
static tCtrlJoyInfo	*joyInfo = NULL;
static tCtrlMouseInfo	*mouseInfo = NULL;
static int ControlsUpdaterIndex = -1;

static std::vector<tHumanContext*> HCtx;

static bool speedLimiter = false;
static tdble speedLimit;

typedef struct
{
    int state;
    int edgeDn;
    int edgeUp;
} tKeyInfo;

// Keyboard map for all players
// (key code => index of the associated command in keyInfo / lastReadKeyState).
typedef std::map<int,int> tKeyMap;
static tKeyMap mapKeys;
static int keyIndex = 0;

// Last read state for each possible player command key.
static int lastReadKeyState[GFUIK_MAX+1];

// Up-to-date info for each possible player command key (state, edge up, edge down)
static tKeyInfo keyInfo[GFUIK_MAX+1];

static bool init_keybd = true;
static bool init_mouse = false;
static bool resume_keybd = true;
static tdble lastKeyUpdate = -10.0;

static void *PrefHdle = NULL;

// Local copy of human driver names (each one allocated by moduleInitialize).
static std::vector<char*> VecNames;

// Number of human drivers (initialized by moduleWelcome).
static int NbDrivers = -1;

// List of permited gear changes with hbox transmission
// prevents mis-selection with thumbstick
// Note 'N' selectable from any gear from ..... 654321NR ... to :
const static int hboxChanges[] = {   0x02, // 0b00000010,  // R
    0x0B, // 0b00001011,  // 1
    0x17, // 0b00010111,  // 2
    0x2A, // 0b00101010,  // 3
    0x52, // 0b01010010,  // 4
    0xA2, // 0b10100010,  // 5
    0x42  // 0b01000010   // 6
};

static int preGear = 0;

static const tControlCmd CmdControlRef[] = {
    {HM_ATT_UP_SHFT,    GFCTRL_TYPE_JOY_BUT,       0, HM_ATT_UP_SHFT_MIN,    0.0, 0.0, HM_ATT_UP_SHFT_MAX,    1.0, NULL, 0.0, NULL, 0.0, NULL, 0.0, NULL, 0.0},
    {HM_ATT_DN_SHFT,    GFCTRL_TYPE_JOY_BUT,       1, HM_ATT_DN_SHFT_MIN,    0.0, 0.0, HM_ATT_DN_SHFT_MAX,    1.0, NULL, 0.0, NULL, 0.0, NULL, 0.0, NULL, 0.0},
    {HM_ATT_ASR_CMD,    GFCTRL_TYPE_JOY_BUT,       2, HM_ATT_ASR_MIN,        0.0, 0.0, HM_ATT_ASR_MAX,        1.0, NULL, 0.0, NULL, 0.0, NULL, 0.0, NULL, 0.0},
    {HM_ATT_ABS_CMD,    GFCTRL_TYPE_JOY_BUT,       3, HM_ATT_ABS_MIN,        0.0, 0.0, HM_ATT_ABS_MAX,        1.0, NULL, 0.0, NULL, 0.0, NULL, 0.0, NULL, 0.0},
    {HM_ATT_GEAR_R,     GFCTRL_TYPE_NOT_AFFECTED, -1, HM_ATT_GEAR_R_MIN,     0.0, 0.0, HM_ATT_GEAR_R_MAX,     1.0, NULL, 0.0, NULL, 0.0, NULL, 0.0, NULL, 0.0},
    {HM_ATT_GEAR_N,     GFCTRL_TYPE_NOT_AFFECTED, -1, HM_ATT_GEAR_N_MIN,     0.0, 0.0, HM_ATT_GEAR_N_MAX,     1.0, NULL, 0.0, NULL, 0.0, NULL, 0.0, NULL, 0.0},
    {HM_ATT_GEAR_1,     GFCTRL_TYPE_NOT_AFFECTED, -1, NULL, 0.0, 0.0, NULL, 0.0, NULL, 0.0, NULL, 0.0, NULL, 0.0, NULL, 0.0},
    {HM_ATT_GEAR_2,     GFCTRL_TYPE_NOT_AFFECTED, -1, NULL, 0.0, 0.0, NULL, 0.0, NULL, 0.0, NULL, 0.0, NULL, 0.0, NULL, 0.0},
    {HM_ATT_GEAR_3,     GFCTRL_TYPE_NOT_AFFECTED, -1, NULL, 0.0, 0.0, NULL, 0.0, NULL, 0.0, NULL, 0.0, NULL, 0.0, NULL, 0.0},
    {HM_ATT_GEAR_4,     GFCTRL_TYPE_NOT_AFFECTED, -1, NULL, 0.0, 0.0, NULL, 0.0, NULL, 0.0, NULL, 0.0, NULL, 0.0, NULL, 0.0},
    {HM_ATT_GEAR_5,     GFCTRL_TYPE_NOT_AFFECTED, -1, NULL, 0.0, 0.0, NULL, 0.0, NULL, 0.0, NULL, 0.0, NULL, 0.0, NULL, 0.0},
    {HM_ATT_GEAR_6,     GFCTRL_TYPE_NOT_AFFECTED, -1, NULL, 0.0, 0.0, NULL, 0.0, NULL, 0.0, NULL, 0.0, NULL, 0.0, NULL, 0.0},

    {HM_ATT_THROTTLE,   GFCTRL_TYPE_JOY_AXIS,      1, HM_ATT_THROTTLE_MIN,   0.0, 0.0, HM_ATT_THROTTLE_MAX,   1.0, HM_ATT_THROTTLE_SENS, 1.0, HM_ATT_THROTTLE_POW, 2.0, NULL, 0.0, NULL, 0.0},
    {HM_ATT_BRAKE,      GFCTRL_TYPE_JOY_AXIS,      1, HM_ATT_BRAKE_MIN,      0.0, 0.0, HM_ATT_BRAKE_MAX,      1.0, HM_ATT_BRAKE_SENS,    1.0, HM_ATT_BRAKE_POW,    2.0, NULL, 0.0, NULL, 0.0},
    {HM_ATT_LEFTSTEER,  GFCTRL_TYPE_JOY_AXIS,      0, HM_ATT_LEFTSTEER_MIN,  0.0, 0.0, HM_ATT_LEFTSTEER_MAX,  1.0, HM_ATT_STEER_SENS,    2.0, HM_ATT_LEFTSTEER_POW,    1.0, HM_ATT_STEER_SPD, 0.0, HM_ATT_STEER_DEAD, 0.0},
    {HM_ATT_RIGHTSTEER, GFCTRL_TYPE_JOY_AXIS,      0, HM_ATT_RIGHTSTEER_MIN, 0.0, 0.0, HM_ATT_RIGHTSTEER_MAX, 1.0, HM_ATT_STEER_SENS,    2.0, HM_ATT_RIGHTSTEER_POW,    1.0, HM_ATT_STEER_SPD, 0.0, HM_ATT_STEER_DEAD, 0.0},
    {HM_ATT_LIGHT1_CMD, GFCTRL_TYPE_NOT_AFFECTED, -1, HM_ATT_LIGHT1_MIN,     0.0, 0.0, HM_ATT_LIGHT1_MAX,     1.0, NULL, 0.0, NULL, 0.0, NULL, 0.0, NULL, 0.0},
    {HM_ATT_CLUTCH,     GFCTRL_TYPE_NOT_AFFECTED, -1, HM_ATT_CLUTCH_MIN,     0.0, 0.0, HM_ATT_CLUTCH_MAX,     1.0, HM_ATT_CLUTCH_SENS,    1.0, HM_ATT_CLUTCH_POW,    2.0, NULL, 0.0, NULL, 0.0},
    {HM_ATT_SPDLIM_CMD, GFCTRL_TYPE_NOT_AFFECTED, -1, HM_ATT_SPDLIM_MIN,     0.0, 0.0, HM_ATT_SPDLIM_MAX,     1.0, NULL, 0.0, NULL, 0.0, NULL, 0.0, NULL, 0.0},
    {HM_ATT_EBRAKE_CMD, GFCTRL_TYPE_JOY_BUT,      19, HM_ATT_EBRAKE_MIN,     0.0, 0.0, HM_ATT_EBRAKE_MAX,     0.0, NULL, 0.0, NULL, 0.0, NULL, 0.0, NULL, 0.0},
    {HM_ATT_HBOX_X,     GFCTRL_TYPE_NOT_AFFECTED, -1, NULL, 0.0, 0.0, NULL, 0.0, NULL, 0.0, NULL, 0.0, NULL, 0.0, NULL, 0.0},
    {HM_ATT_HBOX_Y,     GFCTRL_TYPE_NOT_AFFECTED, -1, NULL, 0.0, 0.0, NULL, 0.0, NULL, 0.0, NULL, 0.0, NULL, 0.0, NULL, 0.0},
    {HM_ATT_LEFTGLANCE, GFCTRL_TYPE_NOT_AFFECTED, -1, HM_ATT_L_GLANCE_MIN,   0.0, 0.0, HM_ATT_L_GLANCE_MAX,   0.0, NULL, 0.0, NULL, 0.0, NULL, 0.0, NULL, 0.0},
    {HM_ATT_RIGHTGLANCE,GFCTRL_TYPE_NOT_AFFECTED, -1, HM_ATT_R_GLANCE_MIN,   0.0, 0.0, HM_ATT_R_GLANCE_MAX,   0.0, NULL, 0.0, NULL, 0.0, NULL, 0.0, NULL, 0.0}
};

static const int NbCmdControl = sizeof(CmdControlRef) / sizeof(CmdControlRef[0]);

typedef struct
{
    const char	*settings;
    const char	*parmName;
} tCtrl;


static tCtrl	controlList[] = {
    {HM_SECT_JSPREF,    HM_VAL_JOYSTICK},
    {HM_SECT_KEYBPREF,  HM_VAL_KEYBOARD},
    {HM_SECT_MOUSEPREF, HM_VAL_MOUSE}
};
static const int nbControl = sizeof(controlList) / sizeof(controlList[0]);

static const std::string Yn[] = {HM_VAL_YES, HM_VAL_NO};

/*
 * Changes from original:
 *
 * Deallocates cmdControl memory.
 */
void HumanDriver::shutdown(const int index)
{
    int idx = index - 1;

    free(VecNames[idx]);
    VecNames[idx] = 0;

    if (HCtx[idx]->cmdControl)
    {
        free(HCtx[idx]->cmdControl);
    }
    free (HCtx[idx]);
    HCtx[idx] = 0;

    resume_keybd = true;
}

/*
 * Original function: InitFuncPt
 *
 * Changes from original:
 *
 * Remove robot interface function assignments which still happens in robot
 * InitFuncPt().
 *
 * Allocate cmdControl memory. Networkhuman does not read preferences for
 * remote driver leaving cmdControl uninitialized, which crashes updateKeys().
 *
 * Add updater_index parameter. Networkhuman does not call drive functions
 * for remote player so the controls update may not run. The parameter allows
 * robot code to set ControlsUpdaterIndex.
 *
 * Modify logic for ControlsUpdaterIndex assignment.
 */
void HumanDriver::init_context(int index, int updater_index)
{
    const int idx = index - 1;

    // Choose this driver as the one who will exclusively read the controls state
    // (if no other was choosen in this race).
    if (ControlsUpdaterIndex < 0)
    {
        if (updater_index)
        {
            ControlsUpdaterIndex = updater_index;
        }
        else
        {
            ControlsUpdaterIndex = index;
        }
    }

    // Initialize mouse and joystick controls backend if not already done.
    if (!joyInfo)
    {
        joyInfo = GfctrlJoyCreate();
        if (joyInfo) {
            joyPresent = true;
        }//if joyInfo
    }

    if (!mouseInfo)
    {
        mouseInfo = GfctrlMouseCreate();
    }

    /* Allocate a new context for that player */
    if ((int)HCtx.size() < idx + 1)
        HCtx.resize(idx + 1);
    HCtx[idx] = (tHumanContext *) calloc (1, sizeof (tHumanContext));

    HCtx[idx]->cmdControl = (tControlCmd *)calloc(NbCmdControl,
            sizeof (tControlCmd));

    HCtx[idx]->antiLock = 1.0;
    HCtx[idx]->antiSlip = 1.0;

    // simuV4 ...
    HCtx[idx]->useESP = false;
    HCtx[idx]->brakeRep = 0.5f;
    HCtx[idx]->brakeCorr = 0.03f;
    HCtx[idx]->brakeFront = 1.0f;
    HCtx[idx]->brakeRear = 1.0f;
    HCtx[idx]->brakeLeft = 1.0f;
    HCtx[idx]->brakeRight = 1.0f;
    // ... simuV4

    read_prefs(index);
}

/*
 * Original function: moduleWelcome
 *
 * Changes from original:
 *
 * Remove parameters.
 *
 * Return NbDrivers.
 *
 * Replace hard coded robot name.
 */
int HumanDriver::count_drivers()
{
    // Open and load the drivers params file
    snprintf(buf, sizeof(buf), "%sdrivers/%s/%s.xml", GfLocalDir(),
            robotname, robotname);
    void *drvInfo = GfParmReadFile(buf, GFPARM_RMODE_REREAD | GFPARM_RMODE_CREAT);

    // Count the number of human drivers registered in the params
    NbDrivers = -1;
    if (drvInfo) {
        const char *driver;
        do {
            NbDrivers++;
            snprintf(sstring, sizeof(sstring), "Robots/index/%d", NbDrivers+1);
            driver = GfParmGetStr(drvInfo, sstring, "name", "");
        } while (strlen(driver) > 0);

        GfParmReleaseHandle(drvInfo);	// Release in case we got it.
    }//if drvInfo

    return NbDrivers;
}

/*
 * Original function: moduleInitialize
 *
 * Changes from original:
 *
 * Add function pointer parameter for InitFuncPt.
 *
 * Replace hard coded robot name.
 */
int HumanDriver::initialize(tModInfo *modInfo, tfModPrivInit InitFuncPt)
{
    if (NbDrivers <= 0) {
        GfOut("human : No human driver registered, or moduleMaxInterfaces() was not called (NbDrivers=%d)\n", NbDrivers);
        return -1;
    }

    // Reset module interfaces info.
    memset(modInfo, 0, NbDrivers*sizeof(tModInfo));

    // Clear the local driver name vector (in case needed).
    VecNames.clear();

    // Open and load the drivers params file
    snprintf(buf, sizeof(buf), "%sdrivers/%s/%s.xml", GfLocalDir(),
            robotname, robotname);
    void *drvInfo = GfParmReadFile(buf, GFPARM_RMODE_REREAD | GFPARM_RMODE_CREAT);

    if (drvInfo) {
        // Fill the module interfaces info : each driver is associated to 1 interface.
        for (int i = 0; i < NbDrivers; i++) {
            snprintf(sstring, sizeof(sstring), "Robots/index/%d", i+1);
            const char* pszDriverName = GfParmGetStr(drvInfo, sstring, "name", 0);
            if (pszDriverName && strlen(pszDriverName) > 0) {
                // Don't rely on GfParm allocated data : duplicate the name ;
                // and also save the pointer somewhere in order to release it at the end.
                char* pszLocDriverName = strdup(pszDriverName);
                VecNames.push_back(pszLocDriverName);
                modInfo->name = pszLocDriverName;	/* name of the module (short) */
                modInfo->desc = "Joystick controlable driver";	/* description (can be longer) */
                modInfo->fctInit = InitFuncPt;	/* init function */
                modInfo->gfId    = ROB_IDENT;	/* supported framework Id */
                modInfo->index   = i+1;
                modInfo++;
            }//if strlen
        }//for i

        GfParmReleaseHandle(drvInfo);	// Release in case we got it.
    }//if drvInfo

    return 0;
}

/*
 * Original function: moduleTerminate
 *
 * Changes from original:
 *
 * Deallocate cmdControl memory
 */
void HumanDriver::terminate()
{
    if (PrefHdle)
    {
        GfParmReleaseHandle(PrefHdle);
    }
    if (joyInfo)
    {
        GfctrlJoyRelease(joyInfo);
    }
    if (mouseInfo)
    {
        GfctrlMouseRelease(mouseInfo);
    }
    GfuiKeyEventRegisterCurrent(0);

    // Free the human context vector
    std::vector<tHumanContext*>::iterator itDrvCtx = HCtx.begin();
    while (itDrvCtx != HCtx.end())
    {
        if (*itDrvCtx)
        {
            if ((*itDrvCtx)->cmdControl)
            {
                free((*itDrvCtx)->cmdControl);
            }
            free(*itDrvCtx);
        }
        itDrvCtx++;
    }
    HCtx.clear();

    // Free the local driver name vector
    std::vector<char*>::iterator itDrvName = VecNames.begin();
    while (itDrvName != VecNames.end())
    {
        free(*itDrvName);
        itDrvName++;
    }
    VecNames.clear();
}


/*
 * Original function: initTrack
 *
 * Changes from original:
 *
 * Search for the car name in robot specific locations but always load the car
 * setup for human robot. Technically, this is no longer necessary since
 * networkhuman does not call this function for remote drivers.
 *
 */
void HumanDriver::init_track(int index,
        tTrack* track,
        void *carHandle,
        void **carParmHandle,
        tSituation *s)
{
    char trackname[256];

    const int idx = index - 1;

    curTrack = track;
    char *s1 = strrchr(track->filename, '/') + 1;
    char *s2 = strchr(s1, '.');
    strncpy(trackname, s1, s2 - s1);
    trackname[s2 - s1] = 0;
    snprintf(sstring, sizeof(sstring), "Robots/index/%d", index);

    snprintf(buf, sizeof(buf), "%sdrivers/%s/%s.xml", GfLocalDir(),
            robotname, robotname);
    void *drvInfo = GfParmReadFile(buf, GFPARM_RMODE_REREAD | GFPARM_RMODE_CREAT);
    std::string carname = (drvInfo != NULL)
        ? GfParmGetStrNC(drvInfo, sstring, "car name", NULL)
        : "";

    snprintf(sstring, sizeof(sstring), "%sdrivers/curcarnames.xml", GfLocalDir());
    void *curCars = GfParmReadFile(sstring, GFPARM_RMODE_REREAD);
    if (curCars) {
        snprintf(sstring, sizeof(sstring), "drivers/%s/%d",
                robotname, index + NbDrivers + 1);
        carname = GfParmGetStr(curCars, sstring, "car name", carname.c_str());
    }//if curCars

    snprintf(sstring, sizeof(sstring), "%s/drivers/human/car.xml", GfLocalDir());
    *carParmHandle = GfParmReadFile(sstring, GFPARM_RMODE_REREAD);

    snprintf(sstring, sizeof(sstring), "%sdrivers/human/cars/%s/default.xml", GfLocalDir(), carname.c_str());
    void *newhandle = GfParmReadFile(sstring, GFPARM_RMODE_REREAD);
    if (newhandle) {
        *carParmHandle = (*carParmHandle)
            ? GfParmMergeHandles(*carParmHandle, newhandle,
                    (GFPARM_MMODE_SRC|GFPARM_MMODE_DST|GFPARM_MMODE_RELSRC|GFPARM_MMODE_RELDST))
            : newhandle;
    }

    snprintf(sstring, sizeof(sstring), "%sdrivers/human/cars/%s/%s.xml", GfLocalDir(), carname.c_str(), trackname);
    newhandle = GfParmReadFile(sstring, GFPARM_RMODE_REREAD);
    if (newhandle) {
        *carParmHandle = (*carParmHandle)
            ? GfParmMergeHandles(*carParmHandle, newhandle,
                    (GFPARM_MMODE_SRC|GFPARM_MMODE_DST|GFPARM_MMODE_RELSRC|GFPARM_MMODE_RELDST))
            : newhandle;

        if (*carParmHandle) {
            GfOut("Player: %s Loaded\n", sstring);
        }
    } else {
        if (*carParmHandle) {
            GfOut("Player: %s Default Setup Loaded\n", sstring);
        }
    }//if-else newhandle

    if (curTrack->pits.type != TR_PIT_NONE) {
        snprintf(sstring, sizeof(sstring), "%s/%s/%d", HM_SECT_PREF, HM_LIST_DRV, index);
        HCtx[idx]->nbPitStopProg = (int)GfParmGetNum(PrefHdle, sstring, HM_ATT_NBPITS, (char*)NULL, 0);
        GfOut("Player: index %d , Pit stops %d\n", index, HCtx[idx]->nbPitStopProg);
    } else {
        HCtx[idx]->nbPitStopProg = 0;
    }//if-else curTrack->pits

    //Initial fuel fill computation
    SetFuelAtRaceStart(track, carParmHandle, s, idx);

    speedLimit = curTrack->pits.speedLimit;

    // simuV4 ...
    HCtx[idx]->brakeRep = GfParmGetNum(carHandle, (char*) SECT_BRKSYST, PRM_BRKREP, (char*)NULL, 0.5);
    HCtx[idx]->brakeCorr = GfParmGetNum(carHandle, (char*) SECT_BRKSYST, PRM_BRKCOR_FR, (char*)NULL, 0.0f);
    HCtx[idx]->useESP = HCtx[idx]->brakeCorr != 0;
    // ... simuV4

    if(drvInfo) {
        GfParmReleaseHandle(drvInfo);
    }
}

/*
 * Original function: newrace
 *
 * Changes from original: none
 */
void HumanDriver::new_race(int index, tCarElt* car, tSituation *s)
{
    const int idx = index - 1;

    // Have to read engine curve
    char midx[64];
    struct tEdesc
    {
        tdble rpm;
        tdble tq;
        tdble drpm;
        tdble dtq;
    } *Edesc;

    sprintf(midx, "%s/%s", SECT_ENGINE, ARR_DATAPTS);
    int IMax = GfParmGetEltNb(car->_carHandle, midx);
    //GfOut("IMax = %d\n", IMax);

    Edesc = (struct tEdesc*) malloc((IMax + 1) * sizeof(struct tEdesc));

    for (int i = 0; i < IMax; i++) {
        sprintf(midx, "%s/%s/%d", SECT_ENGINE, ARR_DATAPTS, i+1);
        Edesc[i].rpm = GfParmGetNum(car->_carHandle, midx, PRM_RPM, (char*) NULL, car->_enginerpmMax);
        Edesc[i].tq = GfParmGetNum(car->_carHandle, midx, PRM_TQ, (char*) NULL, 0.0f);
        Edesc[i].drpm = 0;
        Edesc[i].dtq = 0;
        if (i > 0) {
            Edesc[i-1].drpm = Edesc[i].rpm - Edesc[i-1].rpm;
            Edesc[i-1].dtq = Edesc[i].tq - Edesc[i-1].tq;
        }
        GfOut("rpm %f = tq %f \n", Edesc[i].rpm * 9.549, Edesc[i].tq);
    }

    // Initialize engine RPM shifting threshold table for automatic shifting mode.
    for (int i = 0; i < MAX_GEARS; i++) {
        HCtx[idx]->shiftThld[i] = 10000.0;
    }

    // only calc for forward gears, excluding top gear
    for (int i = 2; i < car->_gearNb; i++) {
        double rpm;
        double newrpm;
        double curTorque = 0;
        double nextTorque = 0;

        if (car->_gearRatio[i] == 0)
            continue;

        // scan torque to see where changing up gives more power
        for (rpm = car->_enginerpmMaxTq; rpm < car->_enginerpmRedLine; rpm += 10) {
            curTorque = 0;
            nextTorque = 0;

            newrpm = rpm * car->_gearRatio[i+1] / car->_gearRatio[i];

            for (int a = 0; a < IMax - 1; a++) {
                if (rpm >= Edesc[a].rpm && rpm < Edesc[a+1].rpm) {
                    curTorque = (Edesc[a].tq + Edesc[a].dtq * (rpm - Edesc[a].rpm) / Edesc[a].drpm) * car->_gearRatio[i];
                    break;
                }
            }

            for (int a = 0; a < IMax - 1; a++) {
                if (newrpm >= Edesc[a].rpm && newrpm < Edesc[a+1].rpm) {
                    nextTorque = (Edesc[a].tq + Edesc[a].dtq * (newrpm - Edesc[a].rpm) / Edesc[a].drpm) * car->_gearRatio[i+1];
                    break;
                }
            }

            if (nextTorque > curTorque) break;
        }

#if 0
        HCtx[idx]->shiftThld[i] = car->_enginerpmRedLine * car->_wheelRadius(2) * 0.85 / car->_gearRatio[i];
        GfOut("Old - Gear %d: Change Up RPM %f = Speed %f\n", i-1, car->_enginerpmRedLine * 0.85 * 9.549, HCtx[idx]->shiftThld[i] * 3.6);
#else
        rpm = MIN(rpm, car->_enginerpmRedLine * 0.93);

        HCtx[idx]->shiftThld[i] = rpm * car->_wheelRadius(2) / car->_gearRatio[i];
        GfOut("New - Gear %d: Change Up RPM %f = Speed %f\n", i-1, rpm * 9.549, HCtx[idx]->shiftThld[i] * 3.6);
#endif
    }
    free(Edesc);

    // Center the mouse.
    if (HCtx[idx]->mouseControlUsed) {
        GfctrlMouseCenter();
    }

    // Initialize key state table
    memset(keyInfo, 0, sizeof(keyInfo));
    memset(lastReadKeyState, 0, sizeof(lastReadKeyState));

#ifndef WIN32
#ifdef TELEMETRY
    if (s->_raceType == RM_TYPE_PRACTICE) {
        RtTelemInit(-10, 10);
        RtTelemNewChannel("Dist", &HCtx[idx]->distToStart, 0, 0);
        RtTelemNewChannel("Ax", &car->_accel_x, 0, 0);
        RtTelemNewChannel("Ay", &car->_accel_y, 0, 0);
        RtTelemNewChannel("Steer", &car->ctrl->steer, 0, 0);
        RtTelemNewChannel("Throttle", &car->ctrl->accelCmd, 0, 0);
        RtTelemNewChannel("Brake", &car->ctrl->brakeCmd, 0, 0);
        RtTelemNewChannel("Gear", &HCtx[idx]->gear, 0, 0);
        RtTelemNewChannel("Speed", &car->_speed_x, 0, 0);
    }
#endif
#endif

    const std::string traintype = GfParmGetStr(car->_carHandle, SECT_DRIVETRAIN, PRM_TYPE, VAL_TRANS_RWD);
    if (traintype == VAL_TRANS_RWD) {
        HCtx[idx]->driveTrain = eRWD;
    } else if (traintype == VAL_TRANS_FWD) {
        HCtx[idx]->driveTrain = eFWD;
    } else if (traintype == VAL_TRANS_4WD) {
        HCtx[idx]->driveTrain = e4WD;
    }//if traintype

    // Set up the autoclutch
    tControlCmd	*cmd = HCtx[idx]->cmdControl;
    HCtx[idx]->autoClutch = true;

    HCtx[idx]->maxClutchTime = GfParmGetNum(car->_carHandle, SECT_GEARBOX, PRM_SHIFTTIME, (char*)NULL, 0.2f);
    switch (car->_skillLevel) {
    case 0: // Rookie
        HCtx[idx]->maxClutchTime *= 2;
        break;
    case 1: // Amateur
        HCtx[idx]->maxClutchTime *= 1.6;
        break;
    case 2: // Semi-Pro
        HCtx[idx]->maxClutchTime *= 1.2;
        break;
    default:
    case 3: // Pro
        break;
    }

    // Set up glancing
    car->_oldglance = 0;
    car->_glance = 0;

    //GfOut("SteerCmd : Left : sens=%4.1f, spSens=%4.2f, deadZ=%4.2f\n",
    //	  cmd[CMD_LEFTSTEER].sens, cmd[CMD_LEFTSTEER].spdSens, cmd[CMD_LEFTSTEER].deadZone);
    //GfOut("SteerCmd : Right: sens=%4.1f, spSens=%4.2f, deadZ=%4.2f\n",
    //	  cmd[CMD_RIGHTSTEER].sens, cmd[CMD_RIGHTSTEER].spdSens, cmd[CMD_RIGHTSTEER].deadZone);

    // Setup Keyboard map (key code => index of the associated command in keyInfo / lastReadKeyState).
    for (int i = 0; i < NbCmdControl; i++)
    {
        if (cmd[i].type == GFCTRL_TYPE_KEYBOARD)
        {
            if (mapKeys.find(cmd[i].val) == mapKeys.end())
            {
                mapKeys[cmd[i].val] = keyIndex;
                keyIndex++;
            }
        }//KEYBOARD

    }//for i
}

/*
 * Original function: resumerace
 *
 * Changes from original:
 *
 * Simplifies conditions for clearing the key map.
 */
void HumanDriver::resume_race(int index, tCarElt* car, tSituation *s)
{
    const int idx = index - 1;
    tControlCmd	*cmd = HCtx[idx]->cmdControl;

    // re-read the controls as they may have changed
    read_prefs(index);

    if (resume_keybd) {
        GfOut("Clearing Keyboard map (index %d)\n", index);
        keyIndex = 0;
        mapKeys.clear();
        resume_keybd = false;
    }

    // Setup Keyboard map (key code => index of the associated command
    // in keyInfo / lastReadKeyState).
    for (int i = 0; i < NbCmdControl; i++)
    {
        if (cmd[i].type == GFCTRL_TYPE_KEYBOARD)
        {
            if (mapKeys.find(cmd[i].val) == mapKeys.end())
            {
                mapKeys[cmd[i].val] = keyIndex;
                keyIndex++;
            }
        }//KEYBOARD

    }//for i
}

/*
 * Changes from original: none
 */
static int lookUpKeyMap(int key)
{
    const tKeyMap::const_iterator p = mapKeys.find(key);

    if (p != mapKeys.end())
        return p->second;

    return -1;
}

/*
 * Changes from original: none
 */
static void updateKeys(void)
{
    int i;
    int nKeyInd;
    int idx;
    tControlCmd *cmd;

    for (idx = 0; idx < (int)HCtx.size(); idx++) {
        if (HCtx[idx]) {
            cmd = HCtx[idx]->cmdControl;
            for (i = 0; i < NbCmdControl; i++) {
                if (cmd[i].type == GFCTRL_TYPE_KEYBOARD) {
                    nKeyInd = lookUpKeyMap(cmd[i].val);
                    if (lastReadKeyState[nKeyInd] == GFUI_KEY_DOWN) {
                        if (keyInfo[nKeyInd].state == GFUI_KEY_UP) {
                            keyInfo[nKeyInd].edgeDn = 1;
                        } else {
                            keyInfo[nKeyInd].edgeDn = 0;
                        }
                    } else {
                        if (keyInfo[nKeyInd].state == GFUI_KEY_DOWN) {
                            keyInfo[nKeyInd].edgeUp = 1;
                        } else {
                            keyInfo[nKeyInd].edgeUp = 0;
                        }
                    }
                    keyInfo[nKeyInd].state = lastReadKeyState[nKeyInd];
                }
            }
        }
    }
}//updateKeys

/*
 * Changes from original: none
 */
static int onKeyAction(int key, int modifier, int state)
{
    // Update key state only if the key is assigned to a player command.
    const int nKeyInd = lookUpKeyMap(key);
    if (nKeyInd >= 0)
        lastReadKeyState[lookUpKeyMap(key)] = state;

    return 0;
}//onKeyAction

/*
 * Changes from original: none
 */
static void common_drive(const int index, tCarElt* car, tSituation *s)
{
    tdble slip;
    tdble ax0;
    tdble brake;
    tdble clutch;
    tdble throttle;
    tdble leftSteer;
    tdble rightSteer;
    tdble newGlance;;
#if (BINCTRL_STEERING == JEPZ || BINCTRL_STEERING == JPM)
    tdble sensFrac, speedFrac;
#endif
    int scrw, scrh, dummy;

    const int idx = index - 1;
    tControlCmd *cmd = HCtx[idx]->cmdControl;

    if (init_keybd && !GfuiScreenIsActive(0))
    {
        GfuiKeyEventRegisterCurrent(onKeyAction);
        init_keybd = false;
    }

    if (init_mouse && !GfuiScreenIsActive(0) && HCtx[idx]->mouseControlUsed) {
        GfuiMouseShow();
        GfctrlMouseCenter();
        GfctrlMouseInitCenter();
        init_mouse = false;
    }

    HCtx[idx]->distToStart = RtGetDistFromStart(car);
    HCtx[idx]->gear = (tdble)car->_gear;	/* telemetry */

    GfScrGetSize(&scrw, &scrh, &dummy, &dummy);

    int oldgear = car->_gearCmd;
    memset(&(car->ctrl), 0, sizeof(tCarCtrl));
    car->_gearCmd = oldgear;

    car->_lightCmd = HCtx[idx]->lightCmd;

    if (car->_laps != HCtx[idx]->lastPitStopLap) {
        car->_raceCmd = RM_CMD_PIT_ASKED;
    }

    // Update the controls at most once per "robots time slice" (RCM_MAX_DT_ROBOTS s)
    // (i.e. keyboard/joystick/mouse values read for all players simultaneously).
    if (lastKeyUpdate != s->currentTime && index == ControlsUpdaterIndex) {
        updateKeys();

        if (joyPresent) {
            GfctrlJoyGetCurrentStates(joyInfo);
        }

        GfctrlMouseGetCurrentState(mouseInfo);
        lastKeyUpdate = s->currentTime;
    }

    // Perform mapping of axis->buttons functions
    for (dummy=0; dummy <= CMD_END_OF_LIST; dummy++) {
        if (cmd[dummy].type == GFCTRL_TYPE_JOY_ATOB) {
            if (joyInfo->ax[cmd[dummy].val] >= cmd[dummy].min && joyInfo->ax[cmd[dummy].val] <= cmd[dummy].max) {
                // Abuse deadZone to store state; 1=just pressed, 2=held
                if (cmd[dummy].deadZone < 2) cmd[dummy].deadZone++;
            } else cmd[dummy].deadZone = 0;
        }
    }

    // Allow Grid Gearbox to freely change as race starts
    if (s->currentTime <= 0 && HCtx[idx]->transmission == eTransGrid)
    {
        /* default to neutral gear */
        preGear = 0;

        for (int i = CMD_GEAR_R; i <= CMD_GEAR_6; i++) {
            if ((cmd[i].type == GFCTRL_TYPE_JOY_BUT && joyInfo->levelup[cmd[i].val])
                    || (cmd[i].type == GFCTRL_TYPE_MOUSE_BUT && mouseInfo->button[cmd[i].val])
#if 0	//SDW fixme
                    || (cmd[i].type == GFCTRL_TYPE_KEYBOARD && keyInfo[lookUpKeyMap(cmd[i].val)].state)
#endif
               )
            {
                preGear = i - CMD_GEAR_N;
            }
        }

        GfOut("Gridbox Initial Gear %d\n", preGear);
    }



    if ((cmd[CMD_ABS].type == GFCTRL_TYPE_JOY_BUT && joyInfo->edgeup[cmd[CMD_ABS].val])
            || (cmd[CMD_ABS].type == GFCTRL_TYPE_MOUSE_BUT && mouseInfo->edgeup[cmd[CMD_ABS].val])
            || (cmd[CMD_ABS].type == GFCTRL_TYPE_KEYBOARD && keyInfo[lookUpKeyMap(cmd[CMD_ABS].val)].edgeUp)
            || (cmd[CMD_ABS].type == GFCTRL_TYPE_JOY_ATOB && cmd[CMD_ABS].deadZone == 1))
    {
        HCtx[idx]->paramAbs = !HCtx[idx]->paramAbs;
        snprintf(sstring, sizeof(sstring), "%s/%s/%d", HM_SECT_PREF, HM_LIST_DRV, index);
        GfParmSetStr(PrefHdle, sstring, HM_ATT_ABS, Yn[!HCtx[idx]->paramAbs].c_str());
        GfParmWriteFile(NULL, PrefHdle, "Human");
    }

    if ((cmd[CMD_ASR].type == GFCTRL_TYPE_JOY_BUT && joyInfo->edgeup[cmd[CMD_ASR].val])
            || (cmd[CMD_ASR].type == GFCTRL_TYPE_MOUSE_BUT && mouseInfo->edgeup[cmd[CMD_ASR].val])
            || (cmd[CMD_ASR].type == GFCTRL_TYPE_KEYBOARD && keyInfo[lookUpKeyMap(cmd[CMD_ASR].val)].edgeUp)
            || (cmd[CMD_ASR].type == GFCTRL_TYPE_JOY_ATOB && cmd[CMD_ASR].deadZone == 1))
    {
        HCtx[idx]->paramAsr = !HCtx[idx]->paramAsr;
        snprintf(sstring, sizeof(sstring), "%s/%s/%d", HM_SECT_PREF, HM_LIST_DRV, index);
        GfParmSetStr(PrefHdle, sstring, HM_ATT_ASR, Yn[!HCtx[idx]->paramAsr].c_str());
        GfParmWriteFile(NULL, PrefHdle, "Human");
    }

    sprintf(car->_msgCmd[0], "%s %s", (HCtx[idx]->paramAbs ? "ABS" : ""), (HCtx[idx]->paramAsr ? "TCS" : ""));
    memcpy(car->_msgColorCmd, color, sizeof(car->_msgColorCmd));

    if ((cmd[CMD_SPDLIM].type == GFCTRL_TYPE_JOY_BUT && joyInfo->edgeup[cmd[CMD_SPDLIM].val])
            || (cmd[CMD_SPDLIM].type == GFCTRL_TYPE_MOUSE_BUT && mouseInfo->edgeup[cmd[CMD_SPDLIM].val])
            || (cmd[CMD_SPDLIM].type == GFCTRL_TYPE_KEYBOARD && keyInfo[lookUpKeyMap(cmd[CMD_SPDLIM].val)].edgeUp)
            || (cmd[CMD_SPDLIM].type == GFCTRL_TYPE_JOY_ATOB && cmd[CMD_SPDLIM].deadZone == 1))
    {
        speedLimiter = !speedLimiter;
    }

    sprintf(car->_msgCmd[1], "Speed Limiter %s", (speedLimiter ? "On" : "Off"));

    if ((cmd[CMD_LIGHT1].type == GFCTRL_TYPE_JOY_BUT && joyInfo->edgeup[cmd[CMD_LIGHT1].val])
            || (cmd[CMD_LIGHT1].type == GFCTRL_TYPE_MOUSE_BUT && mouseInfo->edgeup[cmd[CMD_LIGHT1].val])
            || (cmd[CMD_LIGHT1].type == GFCTRL_TYPE_KEYBOARD && keyInfo[lookUpKeyMap(cmd[CMD_LIGHT1].val)].edgeUp)
            || (cmd[CMD_LIGHT1].type == GFCTRL_TYPE_JOY_ATOB && cmd[CMD_LIGHT1].deadZone == 1))
    {
        if (HCtx[idx]->lightCmd & RM_LIGHT_HEAD1) {
            HCtx[idx]->lightCmd &= ~(RM_LIGHT_HEAD1 | RM_LIGHT_HEAD2);
        } else {
            HCtx[idx]->lightCmd |= RM_LIGHT_HEAD1 | RM_LIGHT_HEAD2;
        }
    }

    switch (cmd[CMD_LEFTSTEER].type) {
    case GFCTRL_TYPE_JOY_AXIS:
        ax0 = joyInfo->ax[cmd[CMD_LEFTSTEER].val];

        // limit and normalise
        if (ax0 > cmd[CMD_LEFTSTEER].max) {
            ax0 = cmd[CMD_LEFTSTEER].max;
        } else if (ax0 < cmd[CMD_LEFTSTEER].min) {
            ax0 = cmd[CMD_LEFTSTEER].min;
        }
        ax0 = (ax0 - cmd[CMD_LEFTSTEER].min) / (cmd[CMD_LEFTSTEER].max - cmd[CMD_LEFTSTEER].min);

        // pow used to indicate the polarity of 'more turn'
        if (cmd[CMD_LEFTSTEER].pow > 0)
            ax0 = ax0 - cmd[CMD_LEFTSTEER].deadZone;
        else 
            ax0 = 1 - ax0 - cmd[CMD_LEFTSTEER].deadZone;

        if (ax0 < 0) ax0 = 0;

        if (1 - cmd[CMD_LEFTSTEER].deadZone != 0)
            ax0 = ax0 / (1 - cmd[CMD_LEFTSTEER].deadZone);
        else
            ax0 = 0;

        leftSteer = fabs(cmd[CMD_LEFTSTEER].pow) * pow(ax0, 1.0f / cmd[CMD_LEFTSTEER].sens) / (1.0 + cmd[CMD_LEFTSTEER].spdSens * car->_speed_xy / 100.0);
        break;
    case GFCTRL_TYPE_MOUSE_AXIS:
        ax0 = mouseInfo->ax[cmd[CMD_LEFTSTEER].val] - cmd[CMD_LEFTSTEER].deadZone;
        if (ax0 > cmd[CMD_LEFTSTEER].max) {
            ax0 = cmd[CMD_LEFTSTEER].max;
        } else if (ax0 < cmd[CMD_LEFTSTEER].min) {
            ax0 = cmd[CMD_LEFTSTEER].min;
        }
        ax0 = ax0 * cmd[CMD_LEFTSTEER].pow;
        leftSteer = pow(fabs(ax0), 1.0f / cmd[CMD_LEFTSTEER].sens) / (1.0f + cmd[CMD_LEFTSTEER].spdSens * car->_speed_xy / 1000.0);
        break;
    case GFCTRL_TYPE_KEYBOARD:
    case GFCTRL_TYPE_JOY_BUT:
    case GFCTRL_TYPE_MOUSE_BUT:
        if (cmd[CMD_LEFTSTEER].type == GFCTRL_TYPE_KEYBOARD) {
            ax0 = keyInfo[lookUpKeyMap(cmd[CMD_LEFTSTEER].val)].state;
        } else if (cmd[CMD_LEFTSTEER].type == GFCTRL_TYPE_MOUSE_BUT) {
            ax0 = mouseInfo->button[cmd[CMD_LEFTSTEER].val];
        } else {
            ax0 = joyInfo->levelup[cmd[CMD_LEFTSTEER].val];
        }
#if (BINCTRL_STEERING == JEPZ)
        if (ax0 == 0) {
            leftSteer = HCtx[idx]->prevLeftSteer - s->deltaTime * 5.0;
        } else {
            ax0 = 2 * ax0 - 1;
            sensFrac = 10.0 * cmd[CMD_LEFTSTEER].sens;
            speedFrac = fabs(car->_speed_x * cmd[CMD_LEFTSTEER].spdSens);
            if (speedFrac < 1.0) speedFrac = 1.0;
            leftSteer = HCtx[idx]->prevLeftSteer + s->deltaTime * ax0 * sensFrac / speedFrac;
        }
        if (leftSteer > 1.0) leftSteer = 1.0;
        if (leftSteer < 0.0) leftSteer = 0.0;
#elif (BINCTRL_STEERING == JPM)
        // ax should be 0 or 1 here (to be checked) => -1 (zero steer) or +1 (full steer left).
        ax0 = 2 * ax0 - 1;
        sensFrac = 1.0 + (3.5 - 1.5 * ax0) * cmd[CMD_LEFTSTEER].sens;
        speedFrac = 1.0 + car->_speed_x * car->_speed_x * cmd[CMD_LEFTSTEER].spdSens / 300.0;
        leftSteer = HCtx[idx]->prevLeftSteer + s->deltaTime * ax0 * sensFrac / speedFrac;
        //GfOut("Left : ax=%4.1f, ws=%4.2f, ss=%4.2f, prev=%5.2f, new=%5.2f (spd=%6.2f)\n",
        //	  ax0, sensFrac, speedFrac, HCtx[idx]->prevLeftSteer, leftSteer, car->_speed_x);
        if (leftSteer > 1.0)
            leftSteer = 1.0;
        else if (leftSteer < 0.0)
            leftSteer = 0.0;
#else
        if (ax0 == 0) {
            leftSteer = 0;
        } else {
            ax0 = 2 * ax0 - 1;
            leftSteer = HCtx[idx]->prevLeftSteer + ax0 * s->deltaTime / cmd[CMD_LEFTSTEER].sens / (1.0 + cmd[CMD_LEFTSTEER].spdSens * car->_speed_x / 1000.0);
            if (leftSteer > 1.0) leftSteer = 1.0;
            if (leftSteer < 0.0) leftSteer = 0.0;
        }
#endif
        HCtx[idx]->prevLeftSteer = leftSteer;
        break;
    default:
        leftSteer = 0;
        break;
    }

    switch (cmd[CMD_RIGHTSTEER].type) {
    case GFCTRL_TYPE_JOY_AXIS:
        ax0 = joyInfo->ax[cmd[CMD_RIGHTSTEER].val];

        // limit and normalise
        if (ax0 > cmd[CMD_RIGHTSTEER].max) {
            ax0 = cmd[CMD_RIGHTSTEER].max;
        } else if (ax0 < cmd[CMD_RIGHTSTEER].min) {
            ax0 = cmd[CMD_RIGHTSTEER].min;
        }
        ax0 = (ax0 - cmd[CMD_RIGHTSTEER].min) / (cmd[CMD_RIGHTSTEER].max - cmd[CMD_RIGHTSTEER].min);

        // pow used to indicate the polarity of 'more turn'
        if (cmd[CMD_RIGHTSTEER].pow > 0)
            ax0 = ax0 - cmd[CMD_RIGHTSTEER].deadZone;
        else 
            ax0 = 1 - ax0 - cmd[CMD_RIGHTSTEER].deadZone;

        if (ax0 < 0) ax0 = 0;

        if (1 - cmd[CMD_RIGHTSTEER].deadZone != 0)
            ax0 = ax0 / (1 - cmd[CMD_RIGHTSTEER].deadZone);
        else
            ax0 = 0;

        rightSteer = -1 * fabs(cmd[CMD_RIGHTSTEER].pow) * pow(ax0, 1.0f / cmd[CMD_RIGHTSTEER].sens) / (1.0 + cmd[CMD_RIGHTSTEER].spdSens * car->_speed_xy / 100.0);
        break;
    case GFCTRL_TYPE_MOUSE_AXIS:
        ax0 = mouseInfo->ax[cmd[CMD_RIGHTSTEER].val] - cmd[CMD_RIGHTSTEER].deadZone;
        if (ax0 > cmd[CMD_RIGHTSTEER].max) {
            ax0 = cmd[CMD_RIGHTSTEER].max;
        } else if (ax0 < cmd[CMD_RIGHTSTEER].min) {
            ax0 = cmd[CMD_RIGHTSTEER].min;
        }
        ax0 = ax0 * cmd[CMD_RIGHTSTEER].pow;
        rightSteer = - pow(fabs(ax0), 1.0f / cmd[CMD_RIGHTSTEER].sens) / (1.0f + cmd[CMD_RIGHTSTEER].spdSens * car->_speed_xy / 1000.0);
        break;
    case GFCTRL_TYPE_KEYBOARD:
    case GFCTRL_TYPE_JOY_BUT:
    case GFCTRL_TYPE_MOUSE_BUT:
        if (cmd[CMD_RIGHTSTEER].type == GFCTRL_TYPE_KEYBOARD) {
            ax0 = keyInfo[lookUpKeyMap(cmd[CMD_RIGHTSTEER].val)].state;
        } else if (cmd[CMD_RIGHTSTEER].type == GFCTRL_TYPE_MOUSE_BUT) {
            ax0 = mouseInfo->button[cmd[CMD_RIGHTSTEER].val];
        } else {
            ax0 = joyInfo->levelup[cmd[CMD_RIGHTSTEER].val];
        }
#if (BINCTRL_STEERING == JEPZ)
        if (ax0 == 0) {
            rightSteer = HCtx[idx]->prevRightSteer + s->deltaTime * 5.0;
        } else {
            ax0 = 2 * ax0 - 1;
            sensFrac = 10.0 * cmd[CMD_RIGHTSTEER].sens;
            speedFrac = fabs(car->_speed_x * cmd[CMD_RIGHTSTEER].spdSens);
            if (speedFrac < 1.0) speedFrac = 1.0;
            rightSteer = HCtx[idx]->prevRightSteer - s->deltaTime * ax0 * sensFrac / speedFrac;
        }
        if (rightSteer > 0.0) rightSteer = 0.0;
        if (rightSteer < -1.0) rightSteer = -1.0;
#elif (BINCTRL_STEERING == JPM)
        // ax should be 0 or 1 here (to be checked) => -1 (zero steer) or +1 (full steer left).
        ax0 = 2 * ax0 - 1;
        sensFrac = 1.0 + (3.5 - 1.5 * ax0) * cmd[CMD_RIGHTSTEER].sens;
        speedFrac = 1.0 + car->_speed_x * car->_speed_x * cmd[CMD_RIGHTSTEER].spdSens / 300.0;
        rightSteer = HCtx[idx]->prevRightSteer - s->deltaTime * ax0 * sensFrac / speedFrac;
        //GfOut("Right: ax=%4.1f, ws=%4.2f, ss=%4.2f, prev=%5.2f, new=%5.2f (spd=%6.2f)\n",
        //	  ax0, sensFrac, speedFrac, HCtx[idx]->prevRightSteer, rightSteer, car->_speed_x);
        if (rightSteer < -1.0)
            rightSteer = -1.0;
        else if (rightSteer > 0.0)
            rightSteer = 0.0;
#else
        if (ax0 == 0) {
            rightSteer = 0;
        } else {
            ax0 = 2 * ax0 - 1;
            rightSteer = HCtx[idx]->prevRightSteer - ax0 * s->deltaTime / cmd[CMD_RIGHTSTEER].sens / (1.0 + cmd[CMD_RIGHTSTEER].spdSens * car->_speed_x / 1000.0);
            if (rightSteer > 0.0) rightSteer = 0.0;
            if (rightSteer < -1.0) rightSteer = -1.0;
        }
#endif
        HCtx[idx]->prevRightSteer = rightSteer;
        break;
    default:
        rightSteer = 0;
        break;
    }

    car->_steerCmd = leftSteer + rightSteer;

#define GLANCERATE 3 	// speed at which the driver turns his head, ~1/3s to full glance
    newGlance = car->_glance;

    if ((cmd[CMD_LEFTGLANCE].type == GFCTRL_TYPE_JOY_BUT && joyInfo->levelup[cmd[CMD_LEFTGLANCE].val])
            || (cmd[CMD_LEFTGLANCE].type == GFCTRL_TYPE_MOUSE_BUT && mouseInfo->button[cmd[CMD_LEFTGLANCE].val])
            || (cmd[CMD_LEFTGLANCE].type == GFCTRL_TYPE_KEYBOARD && keyInfo[lookUpKeyMap(cmd[CMD_LEFTGLANCE].val)].state)
            || (cmd[CMD_LEFTGLANCE].type == GFCTRL_TYPE_JOY_ATOB && cmd[CMD_LEFTGLANCE].deadZone != 0))
    {
        newGlance = newGlance - GLANCERATE * s->deltaTime;
    	if (newGlance < -0.5) newGlance=-0.5;
    } else if ((cmd[CMD_RIGHTGLANCE].type == GFCTRL_TYPE_JOY_BUT && joyInfo->levelup[cmd[CMD_RIGHTGLANCE].val])
            || (cmd[CMD_RIGHTGLANCE].type == GFCTRL_TYPE_MOUSE_BUT && mouseInfo->button[cmd[CMD_RIGHTGLANCE].val])
            || (cmd[CMD_RIGHTGLANCE].type == GFCTRL_TYPE_KEYBOARD && keyInfo[lookUpKeyMap(cmd[CMD_RIGHTGLANCE].val)].state)
            || (cmd[CMD_RIGHTGLANCE].type == GFCTRL_TYPE_JOY_ATOB && cmd[CMD_RIGHTGLANCE].deadZone != 0))
    { 
        newGlance = newGlance + GLANCERATE * s->deltaTime;
    	if (newGlance > 0.5) newGlance=0.5;
    } else if (cmd[CMD_RIGHTGLANCE].type == GFCTRL_TYPE_JOY_AXIS && joyInfo->ax[cmd[CMD_RIGHTGLANCE].val] > cmd[CMD_RIGHTGLANCE].min)
    {
        newGlance = joyInfo->ax[cmd[CMD_RIGHTGLANCE].val];
    } else if (cmd[CMD_LEFTGLANCE].type == GFCTRL_TYPE_JOY_AXIS && joyInfo->ax[cmd[CMD_LEFTGLANCE].val] < cmd[CMD_LEFTGLANCE].max)
    {
        newGlance = joyInfo->ax[cmd[CMD_LEFTGLANCE].val];
    } else {
        // return view to center
        car->_oldglance = 0;
        if (newGlance > 0) {
            newGlance = newGlance - GLANCERATE * s->deltaTime;
            if (newGlance < 0) newGlance = 0;
        }
        if (newGlance < 0) {
            newGlance = newGlance + GLANCERATE * s->deltaTime;
            if (newGlance > 0) newGlance = 0;
        }
    }

    // limit glance
    if (newGlance > 1) newGlance=1;
    if (newGlance < -1) newGlance=-1;

    // Limit twitching between values
    if (newGlance != 0) {
        if (newGlance != car->_oldglance) {
            car->_oldglance = car->_glance;
            car->_glance = newGlance;
        }
    } else {
        car->_oldglance = 0;
        car->_glance = 0;
    }


    switch (cmd[CMD_BRAKE].type) {
    case GFCTRL_TYPE_JOY_AXIS:
        brake = joyInfo->ax[cmd[CMD_BRAKE].val];
        if (brake > cmd[CMD_BRAKE].max) {
            brake = cmd[CMD_BRAKE].max;
        } else if (brake < cmd[CMD_BRAKE].min) {
            brake = cmd[CMD_BRAKE].min;
        }
        car->_brakeCmd = fabs(cmd[CMD_BRAKE].pow *
                pow(fabs((brake - cmd[CMD_BRAKE].minVal) /
                        (cmd[CMD_BRAKE].max - cmd[CMD_BRAKE].min)),
                    1.0f / cmd[CMD_BRAKE].sens));
        break;
    case GFCTRL_TYPE_MOUSE_AXIS:
        ax0 = mouseInfo->ax[cmd[CMD_BRAKE].val] - cmd[CMD_BRAKE].deadZone;
        if (ax0 > cmd[CMD_BRAKE].max) {
            ax0 = cmd[CMD_BRAKE].max;
        } else if (ax0 < cmd[CMD_BRAKE].min) {
            ax0 = cmd[CMD_BRAKE].min;
        }
        ax0 = ax0 * cmd[CMD_BRAKE].pow;
        car->_brakeCmd =  pow(fabs(ax0), 1.0f / cmd[CMD_BRAKE].sens) / (1.0 + cmd[CMD_BRAKE].spdSens * car->_speed_x / 1000.0);
        break;
    case GFCTRL_TYPE_JOY_BUT:
        car->_brakeCmd = joyInfo->levelup[cmd[CMD_BRAKE].val];
        break;
    case GFCTRL_TYPE_MOUSE_BUT:
        car->_brakeCmd = mouseInfo->button[cmd[CMD_BRAKE].val];
        break;
    case GFCTRL_TYPE_KEYBOARD:
        car->_brakeCmd = keyInfo[lookUpKeyMap(cmd[CMD_BRAKE].val)].state;
        break;
    default:
        car->_brakeCmd = 0;
        break;
    }

    switch (cmd[CMD_CLUTCH].type) {
    case GFCTRL_TYPE_JOY_AXIS:
        clutch = joyInfo->ax[cmd[CMD_CLUTCH].val];
        if (clutch > cmd[CMD_CLUTCH].max) {
            clutch = cmd[CMD_CLUTCH].max;
        } else if (clutch < cmd[CMD_CLUTCH].min) {
            clutch = cmd[CMD_CLUTCH].min;
        }
        car->_clutchCmd = fabs(cmd[CMD_CLUTCH].pow *
                pow(fabs((clutch - cmd[CMD_CLUTCH].minVal) /
                        (cmd[CMD_CLUTCH].max - cmd[CMD_CLUTCH].min)),
                    1.0f / cmd[CMD_CLUTCH].sens));
        break;
    case GFCTRL_TYPE_MOUSE_AXIS:
        ax0 = mouseInfo->ax[cmd[CMD_CLUTCH].val] - cmd[CMD_CLUTCH].deadZone;
        if (ax0 > cmd[CMD_CLUTCH].max) {
            ax0 = cmd[CMD_CLUTCH].max;
        } else if (ax0 < cmd[CMD_CLUTCH].min) {
            ax0 = cmd[CMD_CLUTCH].min;
        }
        ax0 = ax0 * cmd[CMD_CLUTCH].pow;
        car->_clutchCmd =  pow(fabs(ax0), 1.0f / cmd[CMD_CLUTCH].sens) / (1.0 + cmd[CMD_CLUTCH].spdSens * car->_speed_x / 1000.0);
        break;
    case GFCTRL_TYPE_JOY_BUT:
        car->_clutchCmd = joyInfo->levelup[cmd[CMD_CLUTCH].val];
        break;
    case GFCTRL_TYPE_MOUSE_BUT:
        car->_clutchCmd = mouseInfo->button[cmd[CMD_CLUTCH].val];
        break;
    case GFCTRL_TYPE_KEYBOARD:
        car->_clutchCmd = keyInfo[lookUpKeyMap(cmd[CMD_CLUTCH].val)].state;
        break;
    default:
        car->_clutchCmd = 0;
        break;
    }

    // if player's used the clutch manually then we dispense with autoClutch
    if (car->_clutchCmd != 0.0f)
        HCtx[idx]->autoClutch = false;

    // Ebrake here so that it can override the clutch control
    if ((cmd[CMD_EBRAKE].type == GFCTRL_TYPE_JOY_BUT && joyInfo->levelup[cmd[CMD_EBRAKE].val])
            || (cmd[CMD_EBRAKE].type == GFCTRL_TYPE_MOUSE_BUT && mouseInfo->button[cmd[CMD_EBRAKE].val])
            || (cmd[CMD_EBRAKE].type == GFCTRL_TYPE_KEYBOARD && keyInfo[lookUpKeyMap(cmd[CMD_EBRAKE].val)].state == GFUI_KEY_DOWN)
            || (cmd[CMD_EBRAKE].type == GFCTRL_TYPE_JOY_ATOB && cmd[CMD_EBRAKE].deadZone != 0))
    {
        car->_ebrakeCmd = 1;
        if (HCtx[idx]->autoClutch)
            car->_clutchCmd = 1;
    } else {
        car->_ebrakeCmd = 0;
    }

    switch (cmd[CMD_THROTTLE].type) {
    case GFCTRL_TYPE_JOY_AXIS:
        throttle = joyInfo->ax[cmd[CMD_THROTTLE].val];
        if (throttle > cmd[CMD_THROTTLE].max) {
            throttle = cmd[CMD_THROTTLE].max;
        } else if (throttle < cmd[CMD_THROTTLE].min) {
            throttle = cmd[CMD_THROTTLE].min;
        }
        car->_accelCmd = fabs(cmd[CMD_THROTTLE].pow *
                pow(fabs((throttle - cmd[CMD_THROTTLE].minVal) /
                        (cmd[CMD_THROTTLE].max - cmd[CMD_THROTTLE].min)),
                    1.0f / cmd[CMD_THROTTLE].sens));
        break;
    case GFCTRL_TYPE_MOUSE_AXIS:
        ax0 = mouseInfo->ax[cmd[CMD_THROTTLE].val] - cmd[CMD_THROTTLE].deadZone;
        if (ax0 > cmd[CMD_THROTTLE].max) {
            ax0 = cmd[CMD_THROTTLE].max;
        } else if (ax0 < cmd[CMD_THROTTLE].min) {
            ax0 = cmd[CMD_THROTTLE].min;
        }
        ax0 = ax0 * cmd[CMD_THROTTLE].pow;
        car->_accelCmd =  pow(fabs(ax0), 1.0f / cmd[CMD_THROTTLE].sens) / (1.0 + cmd[CMD_THROTTLE].spdSens * car->_speed_x / 1000.0);
        if (isnan (car->_accelCmd)) {
            car->_accelCmd = 0;
        }
        /* printf("  axO:%f  accelCmd:%f\n", ax0, car->_accelCmd); */
        break;
    case GFCTRL_TYPE_JOY_BUT:
        car->_accelCmd = joyInfo->levelup[cmd[CMD_THROTTLE].val];
        break;
    case GFCTRL_TYPE_MOUSE_BUT:
        car->_accelCmd = mouseInfo->button[cmd[CMD_THROTTLE].val];
        break;
    case GFCTRL_TYPE_KEYBOARD:
        car->_accelCmd = keyInfo[lookUpKeyMap(cmd[CMD_THROTTLE].val)].state;
        break;
    default:
        car->_accelCmd = 0;
        break;
    }

    // thanks Christos for the following: gradual accel/brake changes for on/off controls.
    if (cmd[CMD_BRAKE].type == GFCTRL_TYPE_JOY_BUT
            || cmd[CMD_BRAKE].type == GFCTRL_TYPE_MOUSE_BUT
            || cmd[CMD_BRAKE].type == GFCTRL_TYPE_KEYBOARD)
    {
        if (s->currentTime > 1.0)
        {
            static const tdble inc_rate = 0.2f;

            tdble d_brake = car->_brakeCmd - HCtx[idx]->pbrake;
            if (fabs(d_brake) > inc_rate && car->_brakeCmd > HCtx[idx]->pbrake)
                car->_brakeCmd =
                    MIN(car->_brakeCmd, HCtx[idx]->pbrake + inc_rate*d_brake/fabs(d_brake));
        }
        HCtx[idx]->pbrake = car->_brakeCmd;
    }

    if (cmd[CMD_THROTTLE].type == GFCTRL_TYPE_JOY_BUT
            || cmd[CMD_THROTTLE].type == GFCTRL_TYPE_MOUSE_BUT
            || cmd[CMD_THROTTLE].type == GFCTRL_TYPE_KEYBOARD)
    {
        if (s->currentTime > 1.0)
        {
            static const tdble inc_rate = 0.2f;

            tdble d_accel = car->_accelCmd - HCtx[idx]->paccel;
            if (fabs(d_accel) > inc_rate && car->_accelCmd > HCtx[idx]->paccel)
                car->_accelCmd =
                    MIN(car->_accelCmd, HCtx[idx]->paccel + inc_rate*d_accel/fabs(d_accel));
        }

    }

    // Linear delay of autoclutch
    if (HCtx[idx]->clutchTime > 0.0f)
        HCtx[idx]->clutchTime -= s->deltaTime;

    // automatically adjust throttle when auto-shifting
    if (HCtx[idx]->clutchTime > 0.0f && HCtx[idx]->autoClutch && car->_gear > 1 && car->_speed_xy > 10) {
        // Target RPMs slightly above ideal match
        double rpm = 1.1 * car->_speed_xy * car->_gearRatio[car->_gear + car->_gearOffset] / car->_wheelRadius(2);

        car->_accelCmd += (rpm - car->_enginerpm) * 4 / car->_enginerpmRedLine;
        //GfOut("Desired rpms for gear %d = %f\n", car->_gear, rpm * 9.54);

        car->_accelCmd = MIN(car->_accelCmd, 1.0);
        car->_accelCmd = MAX(car->_accelCmd, 0.0);
    }

    HCtx[idx]->paccel = car->_accelCmd;

    if (HCtx[idx]->autoReverseEngaged) {
        /* swap brake and throttle */
        brake = car->_brakeCmd;
        car->_brakeCmd = car->_accelCmd;
        car->_accelCmd = brake;
    }

    if (HCtx[idx]->paramAbs)
    {
        if (fabs(car->_speed_x) > 10.0 && car->_brakeCmd > 0.0)
        {
            tdble brake1 = car->_brakeCmd, brake2 = car->_brakeCmd, brake3 = car->_brakeCmd;
            //tdble rearskid = MAX(0.0, MAX(car->_skid[2], car->_skid[3]) - MAX(car->_skid[0], car->_skid[1]));
            int i;

            // reduce brake if car sliding sideways
            tdble skidAng = atan2(car->_speed_Y, car->_speed_X) - car->_yaw;
            NORM_PI_PI(skidAng);

            if (car->_speed_x > 5 && fabs(skidAng) > 0.2)
                brake1 = MIN(car->_brakeCmd, 0.10 + 0.70 * cos(skidAng));

#if 0
            // reduce brake if car steering sharply
            if (fabs(car->_steerCmd) > 0.1)
            {
                tdble decel = ((fabs(car->_steerCmd)-0.1) * (1.0 + fabs(car->_steerCmd)) * 0.2);
                brake2 = MIN(car->_brakeCmd, MAX(0.35, 1.0 - decel));
            }
#endif

            const tdble abs_slip = 1.0;
            const tdble abs_range = 9.0;

            // reduce brake if wheels are slipping
            slip = 0;
            for (i = 0; i < 4; i++) {
                slip = MAX(slip, car->_speed_x - (car->_wheelSpinVel(i) * car->_wheelRadius(i)));
            }

            if (slip > abs_slip)
                brake3 = MAX(MIN(0.35, car->_brakeCmd), car->_brakeCmd - MIN(car->_brakeCmd*0.8, (slip - abs_slip) / abs_range));

            car->_brakeCmd = MIN(brake1, MIN(brake2, brake3));
        }
    }


    if (HCtx[idx]->paramAsr)
    {
        tdble origaccel = car->_accelCmd;

        tdble drivespeed = 0.0;
        switch (HCtx[idx]->driveTrain)
        {
        case e4WD:
            drivespeed = ((car->_wheelSpinVel(FRNT_RGT) + car->_wheelSpinVel(FRNT_LFT)) *
                    car->_wheelRadius(FRNT_LFT) +
                    (car->_wheelSpinVel(REAR_RGT) + car->_wheelSpinVel(REAR_LFT)) *
                    car->_wheelRadius(REAR_LFT)) / 4.0;
            break;
        case eFWD:
            drivespeed = (car->_wheelSpinVel(FRNT_RGT) + car->_wheelSpinVel(FRNT_LFT)) *
                car->_wheelRadius(FRNT_LFT) / 2.0;
            break;
        case eRWD:
            // ADJUSTMENTS TO RWD Asr:-
            // Originally this purely returned the speed of the wheels, which when the speed of
            // the car is subtracted below provides the degree of slip, which is then used to
            // reduce the amount of accelerator.
            //
            // The new calculation below reduces the degree to which the difference between wheel
            // and car speed affects slip, and instead looks at the SlipAccel and SlipSide values,
            // which are more important as they signify an impending loss of control.  The SlipSide
            // value is reduced the faster the car's travelling, as usually it matters most in the
            // low to mid speed ranges.  We also take into account the difference between the
            // player's steer command and the actual yaw rate of the vehicle - where the player
            // is steering against the yaw rate, we decrease the amount of acceleration to stop
            // tirespin sending the rear wheels into a spinout.

            tdble friction = MIN(car->_wheelSeg(REAR_RGT)->surface->kFriction, car->_wheelSeg(REAR_LFT)->surface->kFriction) - 0.2;
            if (friction < 1.0) friction *= MAX(0.6, friction);

            bool  steer_correct = (fabs(car->_yaw_rate) > fabs(car->_steerCmd * MAX(4.0, car->_speed_x/12.0) * friction) ||
                    (car->_yaw_rate < 0.0 && car->_steerCmd > 0.0) ||
                    (car->_yaw_rate > 0.0 && car->_steerCmd < 0.0));
            tdble steer_diff    = fabs(car->_yaw_rate - car->_steerCmd);

            tdble slipf = (steer_correct ? 8 * friction : 15 * friction);

            drivespeed = (((car->_wheelSpinVel(REAR_RGT) + car->_wheelSpinVel(REAR_LFT)) - (20 * friction)) *
                    car->_wheelRadius(REAR_LFT) +
                    (steer_correct ? (steer_diff * fabs(car->_yaw_rate) * (8 / friction)) : 0.0) +
                    MAX(0.0, (-(car->_wheelSlipAccel(REAR_RGT)) - friction)) +
                    MAX(0.0, (-(car->_wheelSlipAccel(REAR_LFT)) - friction)) +
                    fabs(car->_wheelSlipSide(REAR_RGT) * MAX(4, 80-fabs(car->_speed_x))/slipf) +
                    fabs(car->_wheelSlipSide(REAR_LFT) * MAX(4, 80-fabs(car->_speed_x))/slipf))
                / 2.0;
            break;
        }

        tdble slip = drivespeed - fabs(car->_speed_x);
        if (slip > 2.5)
            car->_accelCmd = MIN(car->_accelCmd, origaccel - MIN(origaccel-0.2, ((slip - 2.5)/20.0)));
    }

    if (speedLimiter) {
        if (speedLimit != 0) {
            tdble dv = speedLimit - car->_speed_x;
            if (dv > 0.0) {
                car->_accelCmd = MIN(car->_accelCmd, fabs(dv/6.0));
            } else {
                car->_brakeCmd = MAX(car->_brakeCmd, fabs(dv/5.0));
                car->_accelCmd = 0;
            }//if-else dv
        }//if speedLimit
    }//if speedLimiter


#ifndef WIN32
#ifdef TELEMETRY
    if ((car->_laps > 1) && (car->_laps < 5)) {
        if (HCtx[idx]->lap == 1) {
            RtTelemStartMonitoring("Player");
        }
        RtTelemUpdate(car->_curLapTime);
    }
    if (car->_laps == 5) {
        if (HCtx[idx]->lap == 4) {
            RtTelemShutdown();
        }
    }
#endif
#endif

    HCtx[idx]->lap = car->_laps;
}//common_drive

// simuV4 code ...
/*
 * Changes from original: none
 */
static void common_brake(const int idx, tCarElt* car, tSituation *s)
{
    if(car->_brakeCmd > 0.0)
    {
        if (HCtx[idx]->useESP)
        {
            float DriftAngle = atan2(car->_speed_Y,car->_speed_X) - car->_yaw;
            FLOAT_NORM_PI_PI(DriftAngle);            

            if (DriftAngle > 4.0/180.0*PI)
            {
                HCtx[idx]->brakeLeft = 1.0f + 0.3f;
                HCtx[idx]->brakeRight = 1.0f - 0.3f;
                HCtx[idx]->brakeFront = 1.0f + HCtx[idx]->brakeCorr;
                HCtx[idx]->brakeRear = 1.0f - HCtx[idx]->brakeCorr;
            }
            else if (DriftAngle > 2.0/180.0*PI)
            {
                HCtx[idx]->brakeLeft = 1.0f + 0.3f;
                HCtx[idx]->brakeRight = 1.0f - 0.3f;
                HCtx[idx]->brakeFront = 1.0f;
                HCtx[idx]->brakeRear = 1.0f;
            }
            else if (DriftAngle < -4.0/180.0*PI)
            {
                HCtx[idx]->brakeRight = 1.0f + 0.3f;
                HCtx[idx]->brakeLeft = 1.0f - 0.3f;
                HCtx[idx]->brakeFront = 1.0f + HCtx[idx]->brakeCorr;
                HCtx[idx]->brakeRear = 1.0f - HCtx[idx]->brakeCorr;
            }
            else if (DriftAngle < -2.0/180.0*PI)
            {
                HCtx[idx]->brakeRight = 1.0f + 0.3f;
                HCtx[idx]->brakeLeft = 1.0f - 0.3f;
                HCtx[idx]->brakeFront = 1.0f;
                HCtx[idx]->brakeRear = 1.0f;
            }
            else
            {
                HCtx[idx]->brakeRight = 1.0f;
                HCtx[idx]->brakeLeft = 1.0f;
                HCtx[idx]->brakeFront = 1.0f;
                HCtx[idx]->brakeRear = 1.0f;
            }

            car->ctrl.singleWheelBrakeMode = 1;
            car->ctrl.brakeFrontRightCmd = (float) (car->_brakeCmd * HCtx[idx]->brakeRep * HCtx[idx]->brakeRight * HCtx[idx]->brakeFront); 
            car->ctrl.brakeFrontLeftCmd = (float) (car->_brakeCmd * HCtx[idx]->brakeRep * HCtx[idx]->brakeLeft * HCtx[idx]->brakeFront); 
            car->ctrl.brakeRearRightCmd = (float) (car->_brakeCmd * (1 - HCtx[idx]->brakeRep) * HCtx[idx]->brakeRight * HCtx[idx]->brakeRear); 
            car->ctrl.brakeRearLeftCmd = (float) (car->_brakeCmd * (1 - HCtx[idx]->brakeRep) * HCtx[idx]->brakeLeft * HCtx[idx]->brakeRear); 
        }
        else
            car->ctrl.singleWheelBrakeMode = 0;
    }
}
// ... simuV4 code

/*
 * Changes from original: none
 */
static tdble getAutoClutch(const int idx, int gear, int newGear, tCarElt *car)
{
    tdble ret = 0.0f;

    if (newGear != 0 && newGear < car->_gearNb) {
        if (newGear != gear)
            HCtx[idx]->clutchTime = HCtx[idx]->maxClutchTime;

        if (gear == 1 && car->_speed_xy < 10 && HCtx[idx]->clutchTime > 0)
            // Hold clutch at 1/2 to allow a faster launch from stationary
            HCtx[idx]->clutchTime = HCtx[idx]->maxClutchTime / 2;

        ret = HCtx[idx]->clutchTime / HCtx[idx]->maxClutchTime;
    }//if newGear

    return ret;
}//getAutoClutch

/*
 * Changes from original: none
 */
void HumanDriver::drive_mt(int index, tCarElt* car, tSituation *s)
{
    const int idx = index - 1;

    tControlCmd *cmd = HCtx[idx]->cmdControl;

    common_drive(index, car, s);

    //Can it be left out? car->_gearCmd = car->_gear;
    /* manual shift sequential */
    if (HCtx[idx]->transmission == eTransSeq)
    {
        /* Up shifting command */
        if ((cmd[CMD_UP_SHFT].type == GFCTRL_TYPE_JOY_BUT && joyInfo->edgeup[cmd[CMD_UP_SHFT].val])
                || (cmd[CMD_UP_SHFT].type == GFCTRL_TYPE_MOUSE_BUT && mouseInfo->edgeup[cmd[CMD_UP_SHFT].val])
                || (cmd[CMD_UP_SHFT].type == GFCTRL_TYPE_KEYBOARD && keyInfo[lookUpKeyMap(cmd[CMD_UP_SHFT].val)].edgeUp)
                || (cmd[CMD_UP_SHFT].type == GFCTRL_TYPE_JOY_ATOB && cmd[CMD_UP_SHFT].deadZone == 1))
        {
            if (car->_gear > -1)
                car->_gearCmd++;
            else if (HCtx[idx]->seqShftAllowNeutral && car->_gear == -1)
                car->_gearCmd = 0;
            /* always allow up shift out of reverse to improve game play */
            else if (car->_gear == -1)
                car->_gearCmd = 1;
        }

        /* Down shifting command */
        if ((cmd[CMD_DN_SHFT].type == GFCTRL_TYPE_JOY_BUT && joyInfo->edgeup[cmd[CMD_DN_SHFT].val])
                || (cmd[CMD_DN_SHFT].type == GFCTRL_TYPE_MOUSE_BUT && mouseInfo->edgeup[cmd[CMD_DN_SHFT].val])
                || (cmd[CMD_DN_SHFT].type == GFCTRL_TYPE_KEYBOARD && keyInfo[lookUpKeyMap(cmd[CMD_DN_SHFT].val)].edgeUp)
                || (cmd[CMD_DN_SHFT].type == GFCTRL_TYPE_JOY_ATOB && cmd[CMD_DN_SHFT].deadZone == 1))
        {
            if (car->_gear > 1)
                car->_gearCmd--;
            else if (HCtx[idx]->seqShftAllowNeutral && car->_gear == 1)
                car->_gearCmd = 0;
            else if (HCtx[idx]->seqShftAllowReverse && car->_gear < 2)
                car->_gearCmd = -1;
        }

        /* Neutral gear command */
        if ((cmd[CMD_GEAR_N].type == GFCTRL_TYPE_JOY_BUT && joyInfo->edgeup[cmd[CMD_GEAR_N].val])
                || (cmd[CMD_GEAR_N].type == GFCTRL_TYPE_MOUSE_BUT && mouseInfo->edgeup[cmd[CMD_GEAR_N].val])
                || (cmd[CMD_GEAR_N].type == GFCTRL_TYPE_KEYBOARD && keyInfo[lookUpKeyMap(cmd[CMD_GEAR_N].val)].edgeUp)
                || (cmd[CMD_GEAR_N].type == GFCTRL_TYPE_JOY_ATOB && cmd[CMD_GEAR_N].deadZone == 1))
        {
            car->_gearCmd = 0;
        }

        /* Reverse gear command */
        if ((cmd[CMD_GEAR_R].type == GFCTRL_TYPE_JOY_BUT && joyInfo->edgeup[cmd[CMD_GEAR_R].val])
                || (cmd[CMD_GEAR_R].type == GFCTRL_TYPE_MOUSE_BUT && mouseInfo->edgeup[cmd[CMD_GEAR_R].val])
                || (cmd[CMD_GEAR_R].type == GFCTRL_TYPE_KEYBOARD && keyInfo[lookUpKeyMap(cmd[CMD_GEAR_R].val)].edgeUp)
                || (cmd[CMD_GEAR_R].type == GFCTRL_TYPE_JOY_ATOB && cmd[CMD_GEAR_R].deadZone == 1))
        {
            /* Only allow Reverse to be selected at low speed (~40kmph) or from neutral */
            if (car->_speed_x < 10 || car->_gear == 0)
                car->_gearCmd = -1;
        }
    }

    /* manual shift direct (button for each gear) */
    else if (HCtx[idx]->transmission == eTransGrid)
    {
        /* Go to neutral gear if any gear command released (edge down) */
        if (HCtx[idx]->relButNeutral) {
            for (int i = CMD_GEAR_R; i <= CMD_GEAR_6; i++) {
                if ((cmd[i].type == GFCTRL_TYPE_JOY_BUT && joyInfo->edgedn[cmd[i].val])
                        || (cmd[i].type == GFCTRL_TYPE_MOUSE_BUT && mouseInfo->edgedn[cmd[i].val])
                        || (cmd[i].type == GFCTRL_TYPE_KEYBOARD && keyInfo[lookUpKeyMap(cmd[i].val)].edgeDn))
                {
                    car->_gearCmd = 0;
                }
            }
        }

        /* Select the right gear if any gear command activated (edge up) */
        for (int i = CMD_GEAR_R; i <= CMD_GEAR_6; i++) {
            if ((cmd[i].type == GFCTRL_TYPE_JOY_BUT && joyInfo->edgeup[cmd[i].val])
                    || (cmd[i].type == GFCTRL_TYPE_MOUSE_BUT && mouseInfo->edgeup[cmd[i].val])
                    || (cmd[i].type == GFCTRL_TYPE_KEYBOARD && keyInfo[lookUpKeyMap(cmd[i].val)].edgeUp))
            {
                car->_gearCmd = i - CMD_GEAR_N;
            }
        }
    }

    /* H-Box selector using XY axis of joy/thumbstick */
    else if (HCtx[idx]->transmission == eTransHbox)
    {
        // Used to test bitfield of allowable changes
        int hboxGearTest = 1 << (car->_gear + 1);
        float ax0, ay0;

        ax0 = joyInfo->ax[cmd[CMD_HBOX_X].val];
        ay0 = joyInfo->ax[cmd[CMD_HBOX_Y].val];

        if (ax0 > 0.33) {
            if (ay0 < -0.66 && hboxChanges[5] & hboxGearTest)
                car->_gearCmd = 5;

            if (car->_speed_x < 10) {
                /* 'R' Only selectable at low speed */
                if (ay0 > 0.66 && hboxChanges[0] & hboxGearTest)
                    car->_gearCmd = -1;
            } else {
                /* '6' Only selectable at high speed */
                if (ay0 > 0.66 && hboxChanges[6] & hboxGearTest)
                    car->_gearCmd = 6;
            }
        } else if (ax0 < -0.33) {
            if (ay0 < -0.66 && hboxChanges[1] & hboxGearTest)
                car->_gearCmd = 1;
            if (ay0 > 0.66 && hboxChanges[2] & hboxGearTest)
                car->_gearCmd = 2;
        } else {
            if (ay0 < -0.66 && hboxChanges[3] & hboxGearTest)
                car->_gearCmd = 3;
            if (ay0 > 0.66 && hboxChanges[4] & hboxGearTest)
                car->_gearCmd = 4;
        }

        /* 'N' selectable from any gear */
        if (ay0 < 0.33 && ay0 > -0.33 && ax0 > -0.5 && ax0 < -0.33)
            car->_gearCmd = 0;
        /* Extended 'N' area when using clutch to allow 'jumping' gears */
        if (ay0 < 0.33 && ay0 > -0.33 && ax0 > -0.5 && ax0 < 0.5 && !HCtx[idx]->autoClutch)
            car->_gearCmd = 0;

        /* Neutral gear command */
        if ((cmd[CMD_GEAR_N].type == GFCTRL_TYPE_JOY_BUT && joyInfo->edgeup[cmd[CMD_GEAR_N].val])
                || (cmd[CMD_GEAR_N].type == GFCTRL_TYPE_MOUSE_BUT && mouseInfo->edgeup[cmd[CMD_GEAR_N].val])
                || (cmd[CMD_GEAR_N].type == GFCTRL_TYPE_KEYBOARD && keyInfo[lookUpKeyMap(cmd[CMD_GEAR_N].val)].edgeUp)
                || (cmd[CMD_GEAR_N].type == GFCTRL_TYPE_JOY_ATOB && cmd[CMD_GEAR_N].deadZone == 1))
        {
            car->_gearCmd = 0;
        }

        /* Reverse gear command */
        if ((cmd[CMD_GEAR_R].type == GFCTRL_TYPE_JOY_BUT && joyInfo->edgeup[cmd[CMD_GEAR_R].val])
                || (cmd[CMD_GEAR_R].type == GFCTRL_TYPE_MOUSE_BUT && mouseInfo->edgeup[cmd[CMD_GEAR_R].val])
                || (cmd[CMD_GEAR_R].type == GFCTRL_TYPE_KEYBOARD && keyInfo[lookUpKeyMap(cmd[CMD_GEAR_R].val)].edgeUp)
                || (cmd[CMD_GEAR_R].type == GFCTRL_TYPE_JOY_ATOB && cmd[CMD_GEAR_R].deadZone == 1))
        {
            /* Only allow Reverse to be selected at low speed (~40kmph) or from neutral */
            if (car->_speed_x < 10 || car->_gear == 0)
                car->_gearCmd = -1;
        }
    }

    // check if Grid gear shift is left in gear
    if (preGear) {
        car->_gearCmd = preGear;
        preGear = 0;
    }

    if (HCtx[idx]->autoClutch && car->_clutchCmd == 0.0f)
        car->_clutchCmd = getAutoClutch(idx, car->_gear, car->_gearCmd, car);

    common_brake(idx, car, s);

}

/*
 * Changes from original: none
 */
void HumanDriver::drive_at(int index, tCarElt* car, tSituation *s)
{
    const int idx = index - 1;

    tControlCmd *cmd = HCtx[idx]->cmdControl;

    common_drive(index, car, s);

    /* shift */
    int gear = car->_gear;
    gear += car->_gearOffset;
    //can it be left out? car->_gearCmd = car->_gear;

    if (!HCtx[idx]->autoReverse) {
        /* manual shift */
        if ((cmd[CMD_UP_SHFT].type == GFCTRL_TYPE_JOY_BUT && joyInfo->edgeup[cmd[CMD_UP_SHFT].val])
                || (cmd[CMD_UP_SHFT].type == GFCTRL_TYPE_MOUSE_BUT && mouseInfo->edgeup[cmd[CMD_UP_SHFT].val])
                || (cmd[CMD_UP_SHFT].type == GFCTRL_TYPE_KEYBOARD && keyInfo[lookUpKeyMap(cmd[CMD_UP_SHFT].val)].edgeUp)
                || (cmd[CMD_UP_SHFT].type == GFCTRL_TYPE_JOY_ATOB && cmd[CMD_UP_SHFT].deadZone == 1))
        {
            if (car->_gear == 0)
                HCtx[idx]->manual = false; // switch back to auto gearbox
            else
                HCtx[idx]->manual = true;

            if (!HCtx[idx]->seqShftAllowNeutral && car->_gear == -1) {
                HCtx[idx]->manual = false; // switch back to auto gearbox
                car->_gearCmd = 1;
            } else
                car->_gearCmd++;
        }//CMD_UP_SHFT

        if ((cmd[CMD_DN_SHFT].type == GFCTRL_TYPE_JOY_BUT && joyInfo->edgeup[cmd[CMD_DN_SHFT].val])
                || (cmd[CMD_DN_SHFT].type == GFCTRL_TYPE_MOUSE_BUT && mouseInfo->edgeup[cmd[CMD_DN_SHFT].val])
                || (cmd[CMD_DN_SHFT].type == GFCTRL_TYPE_KEYBOARD && keyInfo[lookUpKeyMap(cmd[CMD_DN_SHFT].val)].edgeUp)
                || (cmd[CMD_DN_SHFT].type == GFCTRL_TYPE_JOY_ATOB && cmd[CMD_DN_SHFT].deadZone == 1))
        {
            if (car->_gear > 1) {
                car->_gearCmd--;
                HCtx[idx]->manual = true;
            } else if (HCtx[idx]->seqShftAllowNeutral && car->_gear == 1) {
                car->_gearCmd = 0;
                HCtx[idx]->manual = true;
            } else if (HCtx[idx]->seqShftAllowReverse && car->_gear < 2) {
                car->_gearCmd = -1;
                HCtx[idx]->manual = true;
            }
        }//CMD_DN_SHFT

        /* Neutral gear command */
        if ((cmd[CMD_GEAR_N].type == GFCTRL_TYPE_JOY_BUT && joyInfo->edgeup[cmd[CMD_GEAR_N].val])
                || (cmd[CMD_GEAR_N].type == GFCTRL_TYPE_MOUSE_BUT && mouseInfo->edgeup[cmd[CMD_GEAR_N].val])
                || (cmd[CMD_GEAR_N].type == GFCTRL_TYPE_KEYBOARD && keyInfo[lookUpKeyMap(cmd[CMD_GEAR_N].val)].edgeUp)
                || (cmd[CMD_GEAR_N].type == GFCTRL_TYPE_JOY_ATOB && cmd[CMD_GEAR_N].deadZone == 1))
        {
            car->_gearCmd = 0;
            HCtx[idx]->manual = true;
        }

        /* Reverse gear command */
        if ((cmd[CMD_GEAR_R].type == GFCTRL_TYPE_JOY_BUT && joyInfo->edgeup[cmd[CMD_GEAR_R].val])
                || (cmd[CMD_GEAR_R].type == GFCTRL_TYPE_MOUSE_BUT && mouseInfo->edgeup[cmd[CMD_GEAR_R].val])
                || (cmd[CMD_GEAR_R].type == GFCTRL_TYPE_KEYBOARD && keyInfo[lookUpKeyMap(cmd[CMD_GEAR_R].val)].edgeUp)
                || (cmd[CMD_GEAR_R].type == GFCTRL_TYPE_JOY_ATOB && cmd[CMD_GEAR_R].deadZone == 1))
        {
            /* Only allow Reverse to be selected at low speed (~36kmph) or from neutral */
            if (car->_speed_x < 10 || car->_gear == 0) {
                car->_gearCmd = -1;
                HCtx[idx]->manual = true;
            }
        }

        /* manual shift direct ie. Reverse-Park-Drive*/
        if (HCtx[idx]->relButNeutral) {
            for (int i = CMD_GEAR_R; i < CMD_GEAR_2; i++) {
                if ((cmd[i].type == GFCTRL_TYPE_JOY_BUT && joyInfo->edgedn[cmd[i].val])
                        || (cmd[i].type == GFCTRL_TYPE_MOUSE_BUT && mouseInfo->edgedn[cmd[i].val])
                        || (cmd[i].type == GFCTRL_TYPE_KEYBOARD && keyInfo[lookUpKeyMap(cmd[i].val)].edgeDn))
                {
                    car->_gearCmd = 0;
                    HCtx[idx]->manual = false;	//return to auto-shift
                }
            }
        }

        /* Select the right gear if any gear command activated (edge up) */
        for (int i = CMD_GEAR_R; i < CMD_GEAR_2; i++) {
            if ((cmd[i].type == GFCTRL_TYPE_JOY_BUT && joyInfo->edgeup[cmd[i].val])
                    || (cmd[i].type == GFCTRL_TYPE_MOUSE_BUT && mouseInfo->edgeup[cmd[i].val])
                    || (cmd[i].type == GFCTRL_TYPE_KEYBOARD && keyInfo[lookUpKeyMap(cmd[i].val)].edgeUp))
            {
                car->_gearCmd = i - CMD_GEAR_N;
                if (car->_gearCmd > 0)
                    HCtx[idx]->manual = false;	//return to auto-shift
                else
                    HCtx[idx]->manual = true;
            }
        }
    }//if !autoReverse

    /* auto shift */
    if (!HCtx[idx]->manual && !HCtx[idx]->autoReverseEngaged) {
        if (car->_speed_x > HCtx[idx]->shiftThld[gear]) {
            car->_gearCmd++;
        } else if ((car->_gearCmd > 1) && (car->_speed_x < (HCtx[idx]->shiftThld[gear-1] - 4.0))) {
            car->_gearCmd--;
        }
        if (car->_gearCmd <= 0)
            car->_gearCmd++;
    }//if !manual && !autoReverse

    /* Automatic Reverse Gear Mode */
    if (HCtx[idx]->autoReverse) {
        if (!HCtx[idx]->autoReverseEngaged) {	//currently not in autoReverse
            if ((car->_brakeCmd > car->_accelCmd) && (car->_speed_xy < 1.0)) {
                HCtx[idx]->autoReverseEngaged = true;
                car->_gearCmd = CMD_GEAR_R - CMD_GEAR_N;
            }
        } else {	//currently in autoreverse mode
            if ((car->_brakeCmd > car->_accelCmd) && (car->_speed_x > -1.0) && (car->_speed_x < 1.0)) {
                HCtx[idx]->autoReverseEngaged = false;
                car->_gearCmd = CMD_GEAR_1 - CMD_GEAR_N;
            } else {
                car->_gearCmd = CMD_GEAR_R - CMD_GEAR_N;
            }
        }//if-else autoReverseEngaged
    }//if autoReverse

    /* Automatic clutch mode */
    if (HCtx[idx]->autoClutch && car->_clutchCmd == 0.0f)
        car->_clutchCmd = getAutoClutch(idx, car->_gear, car->_gearCmd, car);

    common_brake(idx, car, s);
}

/*
 * Changes from original: none
 */
int HumanDriver::pit_cmd(int index, tCarElt* car, tSituation *s)
{
    const int idx = index - 1;

    HCtx[idx]->nbPitStops++;  //Yet another pitstop
    tdble curr_fuel = car->_tank - car->_fuel;  //Can receive max. this fuel

    tdble planned_stops = 1.0
        + MAX(HCtx[idx]->nbPitStopProg - HCtx[idx]->nbPitStops, 0);  //Planned pitstops still ahead

    //Need this amount of extra fuel to finish the race
    tdble fuel =
        ( MaxFuelPerMeter
          * (curTrack->length * car->_remainingLaps + car->_trkPos.seg->lgfromstart)
          + 2.7f / 60.0f * MAX(s->_totTime, 0) )
        / planned_stops
        - car->_fuel;

    //No need to check for limits as curr_fuel cannot be bigger
    //than the tank capacity
    car->_pitFuel = MAX(MIN(curr_fuel, fuel), 0);

    HCtx[idx]->lastPitStopLap = car->_laps;

    car->_pitRepair = (int)car->_dammage;

    if (HCtx[idx]) {
        const tControlCmd *cmd = HCtx[idx]->cmdControl;
        for (int i = 0; i < NbCmdControl; i++) {
            if (cmd[i].type == GFCTRL_TYPE_KEYBOARD) {
                const int key = lookUpKeyMap(cmd[i].val);
                keyInfo[key].state = GFUI_KEY_UP;
                keyInfo[key].edgeDn = 0;
                keyInfo[key].edgeUp = 0;
                lastReadKeyState[key] = GFUI_KEY_UP;
            }
        }//for i
    }//if HCtx

    return ROB_PIT_MENU; /* The player is able to modify the value by menu */
}


/*
 * Changes from original: none
 */
// Trivial strategy:
// fill in as much fuel as required for the whole race,
// or if the tank is too small, fill the tank completely.
static void SetFuelAtRaceStart(tTrack* track, void **carParmHandle,
        tSituation *s, int idx) {
    tdble fuel_requested;
    const tdble initial_fuel = GfParmGetNum(*carParmHandle, SECT_CAR,
            PRM_FUEL, NULL, 0.0f);

    if (initial_fuel) {
        // If starting fuel is set up explicitely,
        // no use computing anything...
        fuel_requested = initial_fuel;
    } else {
        // We must load and calculate parameters.
        const tdble fuel_cons_factor =
            GfParmGetNum(*carParmHandle, SECT_ENGINE, PRM_FUELCONS, NULL, 1.0f);
        tdble fuel_per_lap = track->length * MaxFuelPerMeter * fuel_cons_factor;
        tdble fuel_for_race = fuel_per_lap * (s->_totLaps + 1.0f);// + FuelReserve;
        // aimed at timed sessions:
        fuel_for_race +=  fuel_per_lap / 60.0 * MAX(s->_totTime, 0);
        // divide qty by planned pitstops:
        fuel_for_race /= (1.0 + ((tdble)HCtx[idx]->nbPitStopProg));
        // add some reserve:
        //fuel_for_race += FuelReserve;

        const tdble tank_capacity =
            GfParmGetNum(*carParmHandle, SECT_CAR, PRM_TANK, NULL, 100.0f);
        fuel_requested = MIN(fuel_for_race, tank_capacity);
    }

    GfLogInfo("Human #%d : Starting race session with %.1f l of fuel (%s)\n",
            idx, fuel_requested, initial_fuel ? "as explicitely stated" : "computed");

    GfParmSetNum(*carParmHandle, SECT_CAR, PRM_FUEL, NULL, fuel_requested);
}  // SetFuelAtRaceStart

/*
 * Original function: HmReadPrefs
 *
 * Changes from original:
 *
 * Add parameter for human player index. For human robot robot_index and
 * player_index are always the same. For networkhuman robot, the indexes may be
 * different.
 *
 * Remove allocation for cmdControl.
 *
 * Notes:
 *
 * Never call this function directly. Always call the read_prefs virtual
 * function which determines the correct player_index for the robot.
 */
void HumanDriver::human_prefs(const int robot_index, int player_index)
{
    const char	*prm;
    const char	*defaultSettings;
    char	sstring[1024];
    tCtrlRef	*ref;
    const int	idx = robot_index - 1;
    tControlCmd	*cmdCtrl;

    cmdCtrl = HCtx[idx]->cmdControl;
    memcpy(cmdCtrl, CmdControlRef, NbCmdControl * sizeof (tControlCmd));

    if (!PrefHdle)
    {
        sprintf(sstring, "%s%s", GfLocalDir(), HM_PREF_FILE);
        PrefHdle = GfParmReadFile(sstring,
                GFPARM_RMODE_REREAD | GFPARM_RMODE_CREAT);
    }

    sprintf(sstring, "%s/%s/%d", HM_SECT_PREF, HM_LIST_DRV, player_index);
    prm = GfParmGetStr(PrefHdle, sstring, HM_ATT_TRANS, HM_VAL_AUTO);
    if (!strcmp(prm, HM_VAL_AUTO))
        HCtx[idx]->transmission = eTransAuto;
    else if (!strcmp(prm, HM_VAL_SEQ))
        HCtx[idx]->transmission = eTransSeq;
    else if (!strcmp(prm, HM_VAL_HBOX))
        HCtx[idx]->transmission = eTransHbox;
    else
        HCtx[idx]->transmission = eTransGrid;

    /* Parameters Settings */
    //ABS on/off
    prm = GfParmGetStr(PrefHdle, sstring, HM_ATT_ABS, Yn[HCtx[idx]->paramAbs].c_str());
    HCtx[idx]->paramAbs = (prm == Yn[0]);

    //ASR on/off
    prm = GfParmGetStr(PrefHdle, sstring, HM_ATT_ASR, Yn[HCtx[idx]->paramAsr].c_str());
    HCtx[idx]->paramAsr = (prm == Yn[0]);

    //Controls
    prm = GfParmGetStr(PrefHdle, HM_SECT_PREF, HM_ATT_CONTROL, controlList[2].parmName);
    prm = GfParmGetStr(PrefHdle, sstring, HM_ATT_CONTROL, prm);
    int i;
    for (i = 0; i < nbControl; i++) {
        if (!strcmp(prm, controlList[i].parmName))
            break;
    }//for i

    if (i == nbControl)
        i = 2;

    if (i == 0 && !joyPresent)
        i = 2;

    defaultSettings = controlList[i].settings;

    /* Command Settings */
    GfOut("Command settings for index %d:\n", player_index);
    for (int cmd = 0; cmd < NbCmdControl; cmd++) {

        prm = GfctrlGetNameByRef(cmdCtrl[cmd].type, cmdCtrl[cmd].val);
        prm = GfParmGetStr(PrefHdle, defaultSettings, cmdCtrl[cmd].name, prm);
        prm = GfParmGetStr(PrefHdle, sstring, cmdCtrl[cmd].name, prm);
        if (!prm || strlen(prm) == 0) {
            cmdCtrl[cmd].type = GFCTRL_TYPE_NOT_AFFECTED;
            GfOut("  %s\t: None (-1)\n", cmdCtrl[cmd].name);
            continue;
        }

        ref = GfctrlGetRefByName(prm);
        cmdCtrl[cmd].type = ref->type; // GFCTRL_TYPE_XX
        cmdCtrl[cmd].val = ref->index; // Index for joy. axis, buttons ; 1-bytes ASCII code for keys.
        GfOut("  %s\t: %s\n", cmdCtrl[cmd].name, prm);

        /* min value < max value */
        if (cmdCtrl[cmd].minName) {
            cmdCtrl[cmd].min = (float)GfParmGetNum(PrefHdle, defaultSettings, cmdCtrl[cmd].minName, (char*)NULL, (tdble)cmdCtrl[cmd].min);
            cmdCtrl[cmd].min = cmdCtrl[cmd].minVal = (float)GfParmGetNum(PrefHdle, sstring, cmdCtrl[cmd].minName, (char*)NULL, (tdble)cmdCtrl[cmd].min);
        }//if minName

        /* max value > min value */
        if (cmdCtrl[cmd].maxName) {
            cmdCtrl[cmd].max = (float)GfParmGetNum(PrefHdle, defaultSettings, cmdCtrl[cmd].maxName, (char*)NULL, (tdble)cmdCtrl[cmd].max);
            cmdCtrl[cmd].max = (float)GfParmGetNum(PrefHdle, sstring,		 cmdCtrl[cmd].maxName, (char*)NULL, (tdble)cmdCtrl[cmd].max);
        }//if maxName

        /* 0 < sensitivity */
        if (cmdCtrl[cmd].sensName) {
            cmdCtrl[cmd].sens = (float)GfParmGetNum(PrefHdle, defaultSettings, cmdCtrl[cmd].sensName, (char*)NULL, (tdble)cmdCtrl[cmd].sens);
            cmdCtrl[cmd].sens = (float)GfParmGetNum(PrefHdle, sstring,		 cmdCtrl[cmd].sensName, (char*)NULL, (tdble)cmdCtrl[cmd].sens);
            if (cmdCtrl[cmd].sens <= 0.0)
                cmdCtrl[cmd].sens = 1.0e-6;
        }//if sensName

        /* 0 < power (1 = linear) */
        if (cmdCtrl[cmd].powName) {
            cmdCtrl[cmd].pow = (float)GfParmGetNum(PrefHdle, defaultSettings, cmdCtrl[cmd].powName, (char*)NULL, (tdble)cmdCtrl[cmd].pow);
            cmdCtrl[cmd].pow = (float)GfParmGetNum(PrefHdle, sstring,		 cmdCtrl[cmd].powName, (char*)NULL, (tdble)cmdCtrl[cmd].pow);
        }//if powName

        /* 0 <= sensitivity to car speed */
        if (cmdCtrl[cmd].spdSensName) {
            cmdCtrl[cmd].spdSens = (float)GfParmGetNum(PrefHdle, defaultSettings, cmdCtrl[cmd].spdSensName, (char*)NULL, (tdble)cmdCtrl[cmd].spdSens);
            cmdCtrl[cmd].spdSens = (float)GfParmGetNum(PrefHdle, sstring,		 cmdCtrl[cmd].spdSensName, (char*)NULL, (tdble)cmdCtrl[cmd].spdSens);
            if (cmdCtrl[cmd].spdSens < 0.0)
                cmdCtrl[cmd].spdSens = 0.0;
        }//if spdSendName

        /* 0 =< dead zone < max value - min value (not used for on/off controls like keyboard / mouse buttons / joystick buttons) */
        if (cmdCtrl[cmd].deadZoneName) {
            cmdCtrl[cmd].deadZone = (float)GfParmGetNum(PrefHdle, defaultSettings, cmdCtrl[cmd].deadZoneName, (char*)NULL, (tdble)cmdCtrl[cmd].deadZone);
            cmdCtrl[cmd].deadZone = (float)GfParmGetNum(PrefHdle, sstring,		 cmdCtrl[cmd].deadZoneName, (char*)NULL, (tdble)cmdCtrl[cmd].deadZone);
            if (cmdCtrl[cmd].deadZone < 0.0)
                cmdCtrl[cmd].deadZone = 0.0;
            else if (cmdCtrl[cmd].deadZone > 1.0)
                cmdCtrl[cmd].deadZone = 1.0;
        }//if deadZoneName

        if (cmdCtrl[cmd].min > cmdCtrl[cmd].max)
            std::swap(cmdCtrl[cmd].min, cmdCtrl[cmd].max);

        //cmdCtrl[cmd].deadZone = (cmdCtrl[cmd].max - cmdCtrl[cmd].min) * cmdCtrl[cmd].deadZone;

        if (cmdCtrl[cmd].type == GFCTRL_TYPE_MOUSE_AXIS)
        {
            HCtx[idx]->mouseControlUsed = 1;
            init_mouse = true;
        }

    }//for cmd

    prm = GfParmGetStr(PrefHdle, defaultSettings, HM_ATT_REL_BUT_NEUTRAL, Yn[HCtx[idx]->relButNeutral].c_str());
    prm = GfParmGetStr(PrefHdle, sstring, HM_ATT_REL_BUT_NEUTRAL, prm);
    HCtx[idx]->relButNeutral = (prm == Yn[0]);

    prm = GfParmGetStr(PrefHdle, defaultSettings, HM_ATT_SEQSHFT_ALLOW_NEUTRAL, Yn[HCtx[idx]->seqShftAllowNeutral].c_str());
    prm = GfParmGetStr(PrefHdle, sstring, HM_ATT_SEQSHFT_ALLOW_NEUTRAL, prm);
    HCtx[idx]->seqShftAllowNeutral = (prm == Yn[0]);

    prm = GfParmGetStr(PrefHdle, defaultSettings, HM_ATT_SEQSHFT_ALLOW_REVERSE, Yn[HCtx[idx]->seqShftAllowReverse].c_str());
    prm = GfParmGetStr(PrefHdle, sstring, HM_ATT_SEQSHFT_ALLOW_REVERSE, prm);
    HCtx[idx]->seqShftAllowReverse = (prm == Yn[0]);

    prm = GfParmGetStr(PrefHdle, sstring, HM_ATT_AUTOREVERSE, Yn[HCtx[idx]->autoReverse].c_str());
    HCtx[idx]->autoReverse = (prm == Yn[0]);
}

HumanDriver::HumanDriver(const char *robotname)
{
    this->robotname = robotname;

    joyPresent = false;
    joyInfo = NULL;
    mouseInfo = NULL;
    ControlsUpdaterIndex = -1;
    speedLimiter = false;
    keyIndex = 0;
    lastKeyUpdate = -10.0;
    PrefHdle = NULL;
    NbDrivers = -1;
    preGear = 0;
    resume_keybd = true;
    init_keybd = true;
    init_mouse = false;
}

bool HumanDriver::uses_at(int index)
{
    return HCtx[index-1]->transmission == eTransAuto;
}

void HumanDriver::read_prefs(int index)
{
    human_prefs(index, index);
}
