/***************************************************************************

    file                 : driver.cpp
    created              : Thu Dec 20 01:21:49 CET 2002
    copyright            : (C) 2002-2004 Bernhard Wymann
    email                : berniw@bluewin.ch
    version              : $Id: driver.cpp 6184 2015-10-24 23:50:04Z torcs-ng $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "driver.h"

#define CONTROL_SKILL


//==========================================================================*
// Statics
//--------------------------------------------------------------------------*
int Driver::NBBOTS = MAX_NBBOTS;					// Nbr of drivers/robots
const char* Driver::MyBotName = "usr";				// Name of this bot
const char* Driver::ROBOT_DIR = "drivers/usr";		// Sub path to dll
const char* Driver::SECT_PRIV = "private";			// Private section
const char* Driver::DEFAULTCARTYPE  = "car1-trb1";	// Default car type
int   Driver::RobotType = 0;

/*bool  Driver::AdvancedParameters = false;			// Advanced parameters
bool  Driver::UseOldSkilling = false;				// Use old skilling
bool  Driver::UseSCSkilling = false;				// Use supercar skilling
bool  Driver::UseMPA1Skilling = false;				// Use mpa1 car skilling
float Driver::SkillingFactor = 0.1f;				// Skilling factor for career-mode
bool  Driver::UseBrakeLimit = false;				// Use brake limit
bool  Driver::UseGPBrakeLimit = false;				// Use brake limit GP36
bool  Driver::UseRacinglineParameters = false;		// Use racingline parameters
bool  Driver::UseWingControl = false;				// Use wing control parameters
float Driver::BrakeLimit = -6;						// Brake limit
float Driver::BrakeLimitBase = 0.025f;				// Brake limit base
float Driver::BrakeLimitScale = 25;					// Brake limit scale
float Driver::SpeedLimitBase = 0.025f;				// Speed limit base
float Driver::SpeedLimitScale = 25;					// Speed limit scale
bool  Driver::FirstPropagation = true;				// Initialize
bool  Driver::Learning = false;						// Initialize*/
bool  Driver::UseWingControl = false;				// Use wing control parameters

double Driver::LengthMargin;						// safety margin long.
bool Driver::Qualification;							// Global flag
static const char *WheelSect[4] = {SECT_FRNTRGTWHEEL, SECT_FRNTLFTWHEEL, SECT_REARRGTWHEEL, SECT_REARLFTWHEEL};
static const char *WingSect[2]  = {SECT_FRNTWING, SECT_REARWING};
//const float Driver::MAX_UNSTUCK_ANGLE = (float)(15.0f/180.0f*PI); // [radians] If the angle of the car on the track is smaller, we assume we are not stuck.
const float Driver::MAX_UNSTUCK_ANGLE = 1.3f;
const float Driver::UNSTUCK_TIME_LIMIT = 2.5f;			// [s] We try to get unstuck after this time.
const float Driver::MAX_UNSTUCK_SPEED = 5.0f;			// [m/s] Below this speed we consider being stuck.
const float Driver::MIN_UNSTUCK_DIST = -1.0f;			// [m] If we are closer to the middle we assume to be not stuck.
const float Driver::G = 9.81f;							// [m/(s*s)] Welcome on Earth.
const float Driver::FULL_ACCEL_MARGIN = 1.0f;			// [m/s] Margin reduce oscillation of brake/acceleration.
const float Driver::SHIFT = 0.9f;						// [-] (% of rpmredline) When do we like to shift gears.
const float Driver::SHIFT_MARGIN = 4.0f;				// [m/s] Avoid oscillating gear changes.
const float Driver::ABS_MINSPEED = 3.0f;				// [m/s] Below this speed the ABS is disabled (numeric, division by small numbers).
const float Driver::LOOKAHEAD_CONST = 18.0f;			// [m]
const float Driver::LOOKAHEAD_FACTOR = 0.33f;			// [-]
const float Driver::WIDTHDIV = 3.0f;					// [-] Defines the percentage of the track to use (2/WIDTHDIV).
const float Driver::SIDECOLL_MARGIN = 3.0f;				// [m] Distance between car centers to avoid side collisions.
const float Driver::BORDER_OVERTAKE_MARGIN = 1.0f;      // [m]
const float Driver::OVERTAKE_OFFSET_SPEED = 5.0f;		// [m/s] Offset change speed.
const float Driver::PIT_LOOKAHEAD = 6.0f;				// [m] Lookahead to stop in the pit.
const float Driver::PIT_BRAKE_AHEAD = 200.0f;			// [m] Workaround for "broken" pitentries.
const float Driver::PIT_MU = 0.4f;						// [-] Friction of pit concrete.
const float Driver::MAX_SPEED = 84.0f;					// [m/s] Speed to compute the percentage of brake to apply.
const float Driver::MAX_FUEL_PER_METER = 0.0008f;		// [liter/m] fuel consumtion.
const float Driver::CLUTCH_SPEED = 5.0f;				// [m/s]
const float Driver::CENTERDIV = 0.1f;					// [-] (factor) [0.01..0.6].
const float Driver::DISTCUTOFF = 200.0f;				// [m] How far to look, terminate while loops.
const float Driver::MAX_INC_FACTOR = 8.0f;				// [m] Increment faster if speed is slow [1.0..10.0].
const float Driver::CATCH_FACTOR = 8.0f;				// [-] select MIN(catchdist, dist*CATCH_FACTOR) to overtake.
const float Driver::CLUTCH_FULL_MAX_TIME = 2.0f;		// [s] Time to apply full clutch.
const float Driver::USE_LEARNED_OFFSET_RANGE = 0.2f;    // [m] if offset < this use the learned stuff

const float Driver::TEAM_REAR_DIST = 50.0f;				//
const int Driver::TEAM_DAMAGE_CHANGE_LEAD = 700;		// When to change position in the team?

#define SKIPLIMIT 4

enum { FLYING_FRONT = 1, FLYING_BACK = 2, FLYING_SIDE = 4 };
enum { STUCK_REVERSE = 1, STUCK_FORWARD = 2 };

// Static variables.
Cardata *Driver::cardata = NULL;
static int current_light = RM_LIGHT_HEAD1 | RM_LIGHT_HEAD2;

#define BUFLEN 256
//static char PathFilenameBuffer[BUFLEN];					// for path and filename

#define RANDOM_SEED 0xfded
#define RANDOM_A    1664525
#define RANDOM_C    1013904223

//==========================================================================*
// Set name of robot (and other appendant features)
//--------------------------------------------------------------------------*
void Driver::SetBotName(void* RobotSettings, char* Value)
{
    // At this point TORCS gives us no information
    // about the name of the driver, the team and
    // our own car type!
    // Because we want it to set the name as defined
    // in the teams xml file and to load depending
    // setup files we have to find it out:

    // Needed for Career mode?
    /*if (CarType)
        free (CarType);*/
    CarType = NULL;

    char SectionBuffer[256];                     // Buffer
    char indexstr[32];

    snprintf(SectionBuffer, BUFLEN, "%s/%s/%d", ROB_SECT_ROBOTS, ROB_LIST_INDEX, m_Index);
    char* Section = SectionBuffer;

    // Modified to avoid memory leaks
    // Speed dreams has a trick to find out the oCarType
    RtGetCarindexString(m_Index, "usr", (char) m_Extended, indexstr, 32);
    if( m_Extended )
      CarType = strdup( indexstr );
    else // avoid empty car type
      CarType = strdup(GfParmGetStr(RobotSettings, Section, ROB_ATTR_CAR, DEFAULTCARTYPE));                             // section, default car type

    m_BotName = Value;                            // Get pointer to drv. name

    m_TeamName = GfParmGetStr(RobotSettings, Section, ROB_ATTR_TEAM, (char *) CarType);                                 // section, default car type

    m_RaceNumber = (int) GfParmGetNum(RobotSettings, Section, ROB_ATTR_RACENUM, (char *) NULL, (tdble) m_Index + 1);      // section, index as default

    LogUSR.debug("#Bot name    : %s\n", m_BotName);
    LogUSR.debug("#Team name   : %s\n", m_TeamName);
    LogUSR.debug("#Car type    : %s\n", CarType);
    LogUSR.debug("#Race number : %d\n", m_RaceNumber);
};

void Driver::SetRandomSeed(unsigned int seed)
{
   random_seed = seed ? seed : RANDOM_SEED;

   return;
}

unsigned int Driver::getRandom()
{
    random_seed = RANDOM_A * random_seed + RANDOM_C;
    return (random_seed >> 16);
}

Driver::Driver(int index) :
    HasABS(false),
    HasESP(false),
    HasTCL(false),
    HasTYC(false),

    NoTeamWaiting(0),
    TeamWaitTime(0.0f),
    truespeed(0.0f),
    deltaTime(0.0f),
    FuelSpeedUp(0.0f),
    TclSlip(2.0f),
    TclRange(10.0f),
    AbsSlip(1.0f),
    AbsRange(9.0f),
    OversteerASR(0.7f),
    BrakeMu(1.0f),
    YawRateAccel(0.0f),
    AccelMod(0),
    random_seed(0),
    DebugMsg(0),
    racetype(0),
    mode(0),
    avoidmode(0),
    lastmode(0),
    allow_stuck(1),
    stuck(0),
    stuckcheck(0),
    stuck_timer(0.0f),
    last_stuck_time(-100.0f),
    prefer_side(0),
    allowcorrecting(0),
    pitpos(0),

    prevspeedangle(0.0f),
    speedangle(0.0f),
    angle(0.0f),

    mass(0.0f),
    maxfuel(0.0f),
    myoffset(0.0f),
    pitoffset(0.0f),
    laststeer(0.0f),
    lastbrake(0.0f),
    lastaccel(0.0f),
    lastNSasteer(0.0f),
    lastNSksteer(0.0f),
    avgaccel_x(0.0f),
    car(NULL),
    raceline(NULL),
    opponents(NULL),
    opponent(NULL),
    pit(NULL),
    strategy(NULL),
    mycardata(NULL),
    tLftMargin(NULL),
    tRgtMargin(NULL),
    simtime(0.0),
    avoidtime(0.0),
    frontavoidtime(0.0),
    correcttimer(0.0),
    correctlimit(1000.0),
    aligned_timer(0.0),
    stopped_timer(0.0),
    brakedelay(0.0),
    brakeratio(1.0),
    deltamult(0.0),
    nextCRinverse(0.0),
    sideratio(100.0),
    laststeer_direction(0.0),
    steerLock(0.4),
    currentspeedsqr(0.0f),
    currentspeed(0.0f),
    clutchtime(0.0f),
    oldlookahead(0.0f),
    oldtime_mod(0.0f),
    racesteer(0.0f),
    stucksteer(0.0f),
    prevleft(0.0f),
    rldata(NULL),
    avoidlftoffset(0.0f),
    avoidrgtoffset(0.0f),
    accelcmd(0.0f),
    brakecmd(0.0f),
    faccelcmd(0.0f),
    fbrakecmd(0.0f),
    TurnDecel(0.0f),
    PitOffset(0.0f),
    PitExitSpeed(100.0f),
    RevsChangeDown(0.0f),
    RevsChangeUp(0.0f),
    RevsChangeDownMax(0.0f),
    MaxSteerTime(1.5f),
    MinSteerTime(1.0f),
    SteerCutoff(55.0f),
    SmoothSteer(1.0f),
    LookAhead(1.0f),
    IncFactor(1.0f),
    SideMargin(0.0f),
    OutSteerFactor(0.0f),
    StuckAccel(0.8f),
    StuckAngle(1.6f),
    FollowMargin(0.0f),
    SteerLookahead(0.0f),
    CorrectDelay(0.0f),
    SteerMaxRI(0.008),
    SkidSteer(0.7),
    MinAccel(0.2),
    lookahead(10.0f),
    brakemargin(0.0f),
    MaxGear(0),
    NoPit(0),
    radius(NULL),
    alone(0),
    carindex(0),
    collision(0.0f),
    global_skill(0.0f),
    driver_aggression(0.0f),
    skill(0.0f),
    skill_adjust_limit(0.0),
    skill_adjust_timer(-1.0),
    decel_adjust_targ(1.0),
    decel_adjust_perc(1.0),
    brake_adjust_targ(1.0),
    brake_adjust_perc(1.0),
    fuelperlap(5.0f),
    #ifdef SPEED_DREAMS
    teamIndex(0),
    pitStopChecked(false),
    #endif
    MAX_UNSTUCK_COUNT(0),
    INDEX(0),
    CARMASS(0.0f),
    CA(0.0f),
    CW(0.0f),
    TIREMU(0.0f),
    OVERTAKE_OFFSET_INC(0.0f),
    MU_FACTOR(0.0f),
    track(NULL),
    skipcount(0),
    cmd_accel(0.0f),
    cmd_brake(0.0f),
    cmd_steer(0.0f),
    cmd_gear(0),
    cmd_clutch(0.0f),
    cmd_light(0.0f),
    mRain(0)
{
    LogUSR.debug("\n#TDriver::TDriver() >>>\n\n");
    //int I;
    m_Index = index;										// Save own index
    m_Extended = ( index < 0 || index >= NBBOTS ) ? 1 : 0;  //   is extended or not

    //Driver::LengthMargin = LENGTH_MARGIN;					// Initialize safty margin
    //enableCarNeedsSinLong = false;

    LogUSR.debug("\n#<<< Driver::Driver()\n\n");
}


Driver::~Driver()
{
    if (raceline)
    {
        raceline->FreeTrack(true);
        delete raceline;
    }
    delete opponents;
    delete pit;
    delete [] radius;
    delete strategy;
    delete rldata;
    if (cardata != NULL) {
        delete cardata;
        cardata = NULL;
    }

    free(tLftMargin);
    free(tRgtMargin);
    free(tYawRateAccel);

    if (CarType != NULL)
      free(CarType);
}

// Called for every track change or new race.
void Driver::initTrack(tTrack* t, void *carHandle, void **carParmHandle, tSituation *s)
{
    track = t;

    const int BUFSIZE = 255;
    int i;
    char buffer[BUFSIZE+1];

    decel_adjust_perc = global_skill = skill = driver_aggression = 0.0;
#ifdef CONTROL_SKILL
    // load the global skill level, range 0 - 10
    snprintf(buffer, BUFSIZE, "%sconfig/raceman/extra/skill.xml", GetLocalDir());
    void *skillHandle = GfParmReadFile(buffer, GFPARM_RMODE_REREAD);
    if(!skillHandle)
    {
        snprintf(buffer, BUFSIZE, "%sconfig/raceman/extra/skill.xml", GetDataDir());
        skillHandle = GfParmReadFile(buffer, GFPARM_RMODE_REREAD);
    }//if !skillHandle

    if (skillHandle)
    {
        global_skill = GfParmGetNum(skillHandle, (char *)SECT_SKILL, (char *)PRV_SKILL_LEVEL, (char *) NULL, 30.0f);
    }

    global_skill = MAX(0.0f, MIN(30.0f, global_skill));

    // Initialize the base param path
    const char* BaseParamPath = Driver::ROBOT_DIR;
    //const char* PathFilename = PathFilenameBuffer;

    //load the driver skill level, range 0 - 1
    float driver_skill = 0.0f;
    snprintf(buffer, BUFSIZE, "%s/%d/skill.xml", BaseParamPath, m_Index);
    skillHandle = GfParmReadFile(buffer, GFPARM_RMODE_STD);
    if (skillHandle)
    {
        driver_skill = GfParmGetNum(skillHandle, SECT_SKILL, PRV_SKILL_LEVEL, (char *) NULL, 0.0);
        driver_aggression = GfParmGetNum(skillHandle, SECT_SKILL, PRV_SKILL_AGGRO, (char *)NULL, 0.0);
        driver_skill = (float)MIN(1.0, MAX(0.0, driver_skill));
    }

    skill = (float)((global_skill + driver_skill * 2) * (1.0 + driver_skill));
#endif

    // Load a custom setup if one is available.
    // Get a pointer to the first char of the track filename.
    char *ptrackname = strrchr(track->filename, '/') + 1;
    char *p = strrchr(ptrackname, '.');
    char trackname[256] = {0};

    if (p)
        strncpy(trackname, ptrackname, p - ptrackname);
    else
        strcpy(trackname, ptrackname);

    mRain = getWeather();

    //if (mRain == 0)
    snprintf(buffer, BUFSIZE, "%s/%s/default.xml", BaseParamPath, CarType);
    /*else
      snprintf(buffer, BUFSIZE, "drivers/%s/%s/default-%d.xml",robot_name, carName, mRain);*/

    *carParmHandle = GfParmReadFile(buffer, GFPARM_RMODE_STD);
    void *newhandle;

    if (mRain == 0)
        snprintf(buffer, BUFSIZE, "%s/%s/%s.xml", BaseParamPath, CarType, trackname);
    else
        snprintf(buffer, BUFSIZE, "%s/%s/%s-%d.xml", BaseParamPath, CarType, trackname, mRain);

    newhandle = GfParmReadFile(buffer, GFPARM_RMODE_STD);
    if (newhandle)
    {
        if (*carParmHandle)
            *carParmHandle = GfParmMergeHandles(*carParmHandle, newhandle, (GFPARM_MMODE_SRC|GFPARM_MMODE_DST|GFPARM_MMODE_RELSRC|GFPARM_MMODE_RELDST));
        else
            *carParmHandle = newhandle;
    }
    else
    {
        if (mRain == 0)
            snprintf(buffer, BUFSIZE, "%s/%s/%s.xml", BaseParamPath, CarType, trackname);
        else
            snprintf(buffer, BUFSIZE, "%s/%s/%s-%d.xml", BaseParamPath, CarType, trackname, mRain);

        newhandle = GfParmReadFile(buffer, GFPARM_RMODE_STD);
        if (newhandle)
        {
            if (*carParmHandle)
                *carParmHandle = GfParmMergeHandles(*carParmHandle, newhandle, (GFPARM_MMODE_SRC|GFPARM_MMODE_DST| GFPARM_MMODE_RELSRC|GFPARM_MMODE_RELDST));
            else
                *carParmHandle = newhandle;
        }
    }

    // Create a pit stop strategy object.
    strategy = new SimpleStrategy2();
    strategy->setTrack(track);

    // Init fuel.
    strategy->setFuelAtRaceStart(t, carParmHandle, s, INDEX);

    const char *enabling;

    HasTYC = false;
    enabling = GfParmGetStr(carParmHandle, SECT_FEATURES, PRM_TIRETEMPDEG, VAL_NO);
    if (strcmp(enabling, VAL_YES) == 0)
    {
        HasTYC = true;
        LogUSR.info("#Car has TYC yes\n");
    }
    else
        LogUSR.info("#Car has TYC no\n");

    HasABS = false;
    enabling = GfParmGetStr(carParmHandle, SECT_FEATURES, PRM_ABSINSIMU, VAL_NO);
    if (strcmp(enabling, VAL_YES) == 0)
    {
        HasABS = true;
        LogUSR.info("#Car has ABS yes\n");
    }
    else
        LogUSR.info("#Car has ABS no\n");

    HasESP = false;
    enabling = GfParmGetStr(carParmHandle, SECT_FEATURES, PRM_ESPINSIMU, VAL_NO);
    if (strcmp(enabling, VAL_YES) == 0)
    {
        HasESP = true;
        LogUSR.info("#Car has ESP yes\n");
    }
    else
        LogUSR.info("#Car has ESP no\n");

    HasTCL = false;
    enabling = GfParmGetStr(carParmHandle, SECT_FEATURES, PRM_TCLINSIMU, VAL_NO);
    if (strcmp(enabling, VAL_YES) == 0)
    {
        HasTCL = true;
        LogUSR.info("#Car has TCL yes\n");
    }
    else
        LogUSR.info("#Car has TCL no\n");

    // For test of simu options override switches here
    /*
  oCarHasABS = true;
  oCarHasTCL = true;
  oCarHasESP = true;
  */

    // Load and set parameters.
    MU_FACTOR = GfParmGetNum(*carParmHandle, SECT_PRIVATE, BT_ATT_MUFACTOR, (char*)NULL, 0.69f);

    PitOffset = GfParmGetNum( *carParmHandle, SECT_PRIVATE, PRV_PIT_OFFSET, (char *)NULL, 10.0f );
    PitExitSpeed = GfParmGetNum( *carParmHandle, SECT_PRIVATE, PRV_PIT_EXIT_SPEED, (char *)NULL, 100.0f );
    TurnDecel = GfParmGetNum( *carParmHandle, SECT_PRIVATE, PRV_TURN_DECEL, (char *)NULL, 1.0f );
    RevsChangeUp = GfParmGetNum( *carParmHandle, SECT_PRIVATE, PRV_REVS_UP, (char *)NULL, 0.96f );
    RevsChangeDown = GfParmGetNum( *carParmHandle, SECT_PRIVATE, PRV_REVS_DOWN, (char *)NULL, 0.75f );
    RevsChangeDownMax = GfParmGetNum( *carParmHandle, SECT_PRIVATE, PRV_REVS_DOWN_MAX, (char *)NULL, 0.85f );
    MaxSteerTime = GfParmGetNum( *carParmHandle, SECT_PRIVATE, PRV_MAX_STEER_TIME, (char *)NULL, 1.5f );
    MinSteerTime = GfParmGetNum( *carParmHandle, SECT_PRIVATE, PRV_MIN_STEER_TIME, (char *)NULL, 1.0f );
    SteerCutoff = GfParmGetNum( *carParmHandle, SECT_PRIVATE, PRV_STEER_CUTOFF, (char *)NULL, 55.0f );
    SmoothSteer = GfParmGetNum( *carParmHandle, SECT_PRIVATE, PRV_SMOOTH_STEER, (char *)NULL, 1.0f );
    LookAhead = GfParmGetNum( *carParmHandle, SECT_PRIVATE, PRV_LOOKAHEAD, (char *)NULL, 1.0f );
    IncFactor = GfParmGetNum( *carParmHandle, SECT_PRIVATE, PRV_INC_FACTOR, (char *)NULL, 1.0f );
    SideMargin = GfParmGetNum( *carParmHandle, SECT_PRIVATE, PRV_SIDE_MARGIN, (char *)NULL, 0.0f );
    OutSteerFactor = GfParmGetNum( *carParmHandle, SECT_PRIVATE, PRV_OUT_STEER_X, (char *)NULL, 1.0f );
    StuckAccel = GfParmGetNum( *carParmHandle, SECT_PRIVATE, PRV_STUCK_ACCEL, (char *)NULL, 0.8f );
    StuckAngle = GfParmGetNum( *carParmHandle, SECT_PRIVATE, PRV_STUCK_ANGLE, (char *)NULL, 1.6f );
    FollowMargin = GfParmGetNum( *carParmHandle, SECT_PRIVATE, PRV_FOLLOW_MARGIN, (char *)NULL, 0.0f );
    SteerLookahead = GfParmGetNum( *carParmHandle, SECT_PRIVATE, PRV_STEER_LOOKAHEAD, (char *)NULL, 1.0f );
    CorrectDelay = GfParmGetNum( *carParmHandle, SECT_PRIVATE, PRV_CORRECT_DELAY, (char *)NULL, 0.0f );
    MinAccel = GfParmGetNum( *carParmHandle, SECT_PRIVATE, PRV_MIN_ACCEL, (char *)NULL, 0.2f );
    MaxGear = (int) GfParmGetNum( *carParmHandle, SECT_PRIVATE, PRV_MAX_GEAR, (char *)NULL, 6.0f );
    NoPit = (int) GfParmGetNum( *carParmHandle, SECT_PRIVATE, PRV_NO_PIT, (char *)NULL, 0.0f );
    NoTeamWaiting = (int) GfParmGetNum( *carParmHandle, SECT_PRIVATE, PRV_NO_TEAM_WAITING, (char *)NULL, 1.0f );
    TeamWaitTime = GfParmGetNum( *carParmHandle, SECT_PRIVATE, PRV_TEAM_WAIT_TIME, (char *)NULL, 0.0f);
    YawRateAccel = GfParmGetNum( *carParmHandle, SECT_PRIVATE, PRV_YAW_RATE_ACCEL, (char *)NULL, 0.0f);
    BrakeScale = GfParmGetNum( *carParmHandle, SECT_PRIVATE, PRV_BRAKE_SCALE, (char *)NULL, 1.0f);

    double brkpressure = (GfParmGetNum( *carParmHandle, SECT_BRKSYST, PRM_BRKPRESS, (char *) NULL, 0.0f ) / 1000) * BrakeScale;
    brakeratio -= MIN(0.5, MAX(0.0, brkpressure - 20000.0) / 100000);

    for (i=0; i<6; i++)
    {
        char szTmp[32];

        sprintf(szTmp, "%s %d", PRV_REVS_UP, i+1);
        GearRevsChangeUp[i] = GfParmGetNum( *carParmHandle, SECT_PRIVATE, szTmp, (char *)NULL, RevsChangeUp );

        sprintf(szTmp, "%s %d", PRV_REVS_DOWN, i+1);
        GearRevsChangeDown[i] = GfParmGetNum( *carParmHandle, SECT_PRIVATE, szTmp, (char *)NULL, RevsChangeDown );

        sprintf(szTmp, "%s %d", PRV_REVS_DOWN_MAX, i+1);
        GearRevsChangeDownMax[i] = GfParmGetNum( *carParmHandle, SECT_PRIVATE, szTmp, (char *)NULL, RevsChangeDownMax );
    }

    tLftMargin = (LRLMod *) malloc( sizeof(LRLMod) );
    tRgtMargin = (LRLMod *) malloc( sizeof(LRLMod) );
    tYawRateAccel = (LRLMod *) malloc( sizeof(LRLMod) );
    memset( tLftMargin, 0, sizeof(LRLMod) );
    memset( tRgtMargin, 0, sizeof(LRLMod) );
    memset( tYawRateAccel, 0, sizeof(LRLMod) );

    for (i=0; i<LMOD_DATA; i++)
    {
        char str[32];

        sprintf(str, "%d %s", i, PRV_BEGIN);
        int div = (int) GfParmGetNum(*carParmHandle, SECT_PRIVATE, str, (char *) NULL, 0.0);
        sprintf(str, "%d %s", i, PRV_END);
        int enddiv = (int) GfParmGetNum(*carParmHandle, SECT_PRIVATE, str, (char *) NULL, 0.0);
        enddiv = MAX(div, enddiv);

        if (enddiv == 0 && div == 0)
            break;

        sprintf(str, "%d %s", i, PRV_AV_RIGHT_MARGIN);
        double rmargin = GfParmGetNum(*carParmHandle, SECT_PRIVATE, str, (char *) NULL, 0.0);
        AddMod( tRgtMargin, div, enddiv, rmargin, 0 );

        sprintf(str, "%d %s", i, PRV_AV_LEFT_MARGIN);
        double lmargin = GfParmGetNum(*carParmHandle, SECT_PRIVATE, str, (char *) NULL, 0.0);
        AddMod( tLftMargin, div, enddiv, lmargin, 0 );

        sprintf(str, "%d %s", i, PRV_YAW_RATE_ACCEL);
        double yra = GfParmGetNum(*carParmHandle, SECT_PRIVATE, str, (char *) NULL, 0.0);
        AddMod( tYawRateAccel, div, enddiv, yra, 0 );
    }
}

void Driver::LoadDAT( tSituation *s, char *carname, char *trackname )
{
    return;
#if 0
    FILE *fp;
    char buffer[1024+1];

    switch (s->_raceType)
    {
    case RM_TYPE_PRACTICE:
        snprintf(buffer, 1024, "drivers/%s/%s/practice/%s.dat", robot_name, carname, trackname);
        break;
    case RM_TYPE_QUALIF:
        snprintf(buffer, 1024, "drivers/%s/%s/qualifying/%s.dat", robot_name, carname, trackname);
        break;
    case RM_TYPE_RACE:
        snprintf(buffer, 1024, "drivers/%s/%s/race/%s.dat", robot_name, carname, trackname);
        break;
    }

    if (!(fp = fopen(buffer, "r")))
    {
        fprintf(stderr, "Failed to load %s\n", buffer);
        snprintf(buffer, 1024, "drivers/%s/%s/%s.dat", robot_name, carname, trackname);

        if (!(fp = fopen(buffer, "r")))
        {
            fprintf(stderr, "Failed to load %s\n", buffer);
            return;
        }
    }

    fprintf(stderr, "Loading %s\n", buffer);
    while (fgets(buffer, 1024, fp))
    {
        int len = strlen(buffer);
        if (len <= 1)
            continue;

        char *p = buffer + (len-1);
        while (p >= buffer && (*p == 10 || *p == 13))
        {
            *p = 0;
            p--;
        }
        if (p <= buffer)
            continue;

        p = strtok( buffer, " " );
        int spd = strcmp(p, "SPD");
        int brk = strcmp(p, "BRK");
        int gan = strcmp(p, "GAN");

        if (!spd || !brk || !gan)
        {
            p = strtok( NULL, " " );
            int bgn = atoi(p);
            p = strtok( NULL, " " );
            int end = atoi(p);
            p = strtok( NULL, " " );
            double val = atof(p);
            p = strtok( NULL, " " );
            double valX = atof(p);

            if (!spd)
            {
                AddMod( raceline->tRLSpeed0, bgn, end, val, 0 );
                AddMod( raceline->tRLSpeed1, bgn, end, valX, 0 );
            }
            else if (!gan)
            {
                AddMod( raceline->tSteerGain0, bgn, end, val, 0 );
                AddMod( raceline->tSteerGain1, bgn, end, valX, 0 );
            }
            else
            {
                AddMod( raceline->tRLBrake0, bgn, end, val, 0 );
                AddMod( raceline->tRLBrake1, bgn, end, valX, 0 );
            }
        }
    }

    fclose(fp);
#endif
}



// Start a new race.
void Driver::newRace(tCarElt* car, tSituation *s)
{
    deltaTime = (float) RCM_MAX_DT_ROBOTS;
    MAX_UNSTUCK_COUNT = int(UNSTUCK_TIME_LIMIT/deltaTime);
    OVERTAKE_OFFSET_INC = OVERTAKE_OFFSET_SPEED*deltaTime;
    random_seed = 0;
    alone = allow_stuck = 1;
    stuckcheck = 0;
    clutchtime = stuck_timer = 0.0f;
    last_stuck_time = -100.0f;
    oldtime_mod = oldlookahead = laststeer = lastbrake = lastaccel = avgaccel_x = lastNSasteer = lastNSksteer = 0.0f;
    brake_adjust_targ = decel_adjust_targ = 1.0f;
    brake_adjust_perc = decel_adjust_perc = 1.0f;
    prevleft = car->_trkPos.toLeft;
    this->car = car;
    int stdebug = (int) GfParmGetNum(car->_carHandle, SECT_PRIVATE, PRV_STEER_DEBUG, NULL, 0);
    int otdebug = (int) GfParmGetNum(car->_carHandle, SECT_PRIVATE, PRV_OVERTAKE_DEBUG, NULL, 0);
    int brdebug = (int) GfParmGetNum(car->_carHandle, SECT_PRIVATE, PRV_BRAKE_DEBUG, NULL, 0);
    if ((RM_TYPE_PRACTICE == s->_raceType && stdebug >= 0) || stdebug > 0) DebugMsg |= debug_steer;
    if (otdebug) DebugMsg |= debug_overtake;
    if (brdebug) DebugMsg |= debug_brake;
    FuelSpeedUp = GfParmGetNum(car->_carHandle, SECT_PRIVATE, PRV_FUEL_SPEEDUP, NULL, 0.0f);
    TclSlip = GfParmGetNum(car->_carHandle, SECT_PRIVATE, PRV_TCL_SLIP, NULL, 2.0f);
    TclRange = GfParmGetNum(car->_carHandle, SECT_PRIVATE, PRV_TCL_RANGE, NULL, 10.0f);
    AbsSlip = GfParmGetNum(car->_carHandle, SECT_PRIVATE, PRV_ABS_SLIP, NULL, 2.5f);
    AbsRange = GfParmGetNum(car->_carHandle, SECT_PRIVATE, PRV_ABS_RANGE, NULL, 5.0f);
    OversteerASR = GfParmGetNum(car->_carHandle, SECT_PRIVATE, PRV_OVERSTEER_ASR, NULL, 0.4f);
    BrakeMu = GfParmGetNum(car->_carHandle, SECT_PRIVATE, PRV_BRAKE_MU, NULL, 1.0f);
    YawRateAccel = GfParmGetNum(car->_carHandle, SECT_PRIVATE, PRV_YAW_RATE_ACCEL, NULL, 0.0f);
    AccelMod = (int) GfParmGetNum(car->_carHandle, SECT_PRIVATE, PRV_ACCEL_MOD, NULL, 0.0f);
    fuelperlap = GfParmGetNum(car->_carHandle, SECT_PRIVATE, PRV_FUEL_PER_LAP, NULL, 5.0f);
    CARMASS = GfParmGetNum(car->_carHandle, SECT_CAR, PRM_MASS, NULL, 1000.0f);
    maxfuel = GfParmGetNum(car->_carHandle, SECT_CAR, PRM_TANK, NULL, 100.0f);
    steerLock = GfParmGetNum(car->_carHandle, SECT_STEER, PRM_STEERLOCK, (char *)NULL, 4.0f);
    brakemargin = GfParmGetNum(car->_carHandle, SECT_PRIVATE, PRV_BRAKE_MARGIN, (char *)NULL, 0.0f);
    myoffset = 0.0f;
    cmd_accel = cmd_brake = cmd_clutch = cmd_steer = cmd_light = 0.0f;
    cmd_gear = 1;
    skipcount = 0;
    simtime = correcttimer = skill_adjust_limit = aligned_timer = stopped_timer = 0.0;
    avoidtime = frontavoidtime = 0.0;
    correctlimit = 1000.0;
    deltamult = 1.0 / s->deltaTime;
    racetype = s->_raceType;
    initWheelPos();
    initCa();
    initCw();
    initTireMu();
    initTCLfilter();

    raceline = new LRaceLine();
    raceline->NewRace( car, s );
    raceline->setSkill( skill );
    raceline->AllocTrack( track );
    {
        char *ptrackname = strrchr(track->filename, '/') + 1;
        char *p = strrchr(ptrackname, '.');
        char trackname[256] = {0};

        if (p)
            strncpy(trackname, ptrackname, p - ptrackname);
        else
            strcpy(trackname, ptrackname);

        char carName[256];
        {
            char const *path = SECT_GROBJECTS "/" LST_RANGES "/" "1";
            char const *key = PRM_CAR;
            strncpy( carName, GfParmGetStr(car->_carHandle, path, key, ""), sizeof(carName) );
            char *p = strrchr(carName, '.');
            if (p) *p = '\0';
        }

        LoadDAT( s, carName, trackname );
    }
    raceline->InitTrack(track, s);
    raceline->setCW( CW );

    rldata = new LRaceLineData();
    memset(rldata, 0, sizeof(LRaceLineData));

    // Create just one instance of cardata shared by all drivers.
    if (cardata == NULL) {
        cardata = new Cardata(s);
    }
    mycardata = cardata->findCar(car);
    simtime = s->currentTime;
    skill_adjust_timer = -1;

    // initialize the list of opponents.
    opponents = new Opponents(s, this, cardata);
    opponent = opponents->getOpponentPtr();

    // Set team mate.
    char *teammate = (char *) GfParmGetStr(car->_carHandle, SECT_PRIVATE, BT_ATT_TEAMMATE, NULL);
    if (teammate != NULL) {
        opponents->setTeamMate(teammate);
    }

    // Initialize radius of segments.
    radius = new float[track->nseg];
    computeRadius(radius);

    // create the pit object.
    pit = new Pit(s, this, PitOffset);
    setMode( mode_correcting );
    lastmode = mode_correcting;

    carindex = 0;

    for (int i = 0; i < s->_ncars; i++)
    {
        if (s->cars[i] == car)
        {
            carindex = i;
            break;
        }
    }

    strategy->Init(this);                         // Init strategy

#ifdef SPEED_DREAMS
    teamIndex = RtTeamManagerIndex( car, track, s );
    strategy->setTeamIndex( teamIndex );
#endif
}

void Driver::calcSpeed()
{
    accelcmd = brakecmd = 0.0f;
    faccelcmd = fbrakecmd = 0.0f;
    double speed = rldata->speed;
    double avspeed = MAX((currentspeed+0.4)-(MAX(0.0, 1.6-fabs(MAX(0.0, angle-speedangle))*5)), rldata->avspeed);
    double slowavspeed = rldata->slowavspeed;

    double speed_redux = 1.0;

    if (mode != mode_normal)
    {
        if (fabs(speedangle) > 0.05 &&
                (fabs(speedangle) > fabs(rldata->rlangle) || fabs(speedangle - rldata->rlangle) > 0.05))
            speed_redux -= MIN(0.6, MIN(fabs(speedangle), fabs(speedangle-rldata->rlangle))/2);
        avspeed *= speed_redux;
        slowavspeed *= speed_redux;
    }

    if (mode == mode_avoiding && !allowcorrecting)
    {
        speed = avspeed;
        if ((avoidmode & avoidside) && !rldata->insideline && sideratio < 1.0 &&
                ((rldata->rInverse > 0.0 && (avoidmode & avoidright) && speedangle < -(sideratio/10)) ||
                 (rldata->rInverse < 0.0 && (avoidmode & avoidleft) && speedangle > (sideratio/10))))
        {
            speed = slowavspeed;
        }
    }
    else if ((mode == mode_correcting || (simtime - aligned_timer < 2.0)) && rldata->insideline && rldata->closing)
    {
        speed = slowavspeed;
    }
    else if ((mode == mode_correcting || (simtime - aligned_timer < 5.0)))
    {
        //avspeed = MIN(rldata->speed - MIN(3.0, fabs(rldata->rInverse)*400), avspeed);
        avspeed = MIN(rldata->speed, (slowavspeed+avspeed)/2);
        double rlspeed = rldata->speed;//MAX(avspeed, rldata->speed-1.0);
        //speed = avspeed + (rlspeed-avspeed) * MAX(0.0, MIN(1.0, 1.0 - MAX(fabs(correctlimit*2), fabs(fabs(laststeer)-fabs(rldata->rInverse*80))/2)));
        speed = avspeed + (rlspeed-avspeed) * MAX(0.0, MIN(1.0, 1.0 - (fabs(correctlimit*2) + fabs(angle - rldata->rlangle)*5)));
    }

    if (pit->getInPit() && !pit->getPitstop())
    {
        float s = pit->toSplineCoord( car->_distFromStartLine );
        if (s > pit->getNPitEnd())
            speed = MIN(speed, PitExitSpeed);
    }

    //if (mode == mode_normal && fabs(rldata->rInverse) < 0.001 && fabs(car->_steerCmd) < 0.01 && fabs(angle) < 0.01 && fabs(car->_yaw_rate) < 0.01)
    //  aligned_timer = 0.0;

    double x = (10 + car->_speed_x) * (speed - car->_speed_x) / 200;
    double lane2left = car->_trkPos.toLeft / track->width;
    double lane2right = car->_trkPos.toRight / track->width;
    int sidedanger = ((rldata->rInverse > 0.0 && speedangle < -(rldata->rInverse) * lane2left*2) || (rldata->rInverse < 0.0 && speedangle > rldata->rInverse*lane2right*2));
    double skid = MAX(0.0, (car->_skid[2] + car->_skid[3] + car->_skid[0] + car->_skid[1])) * 3;

    brakecmd = 0.0f;
    accelcmd = 100.0f;

    {
        double skidangle = angle;
        if (mode != mode_normal)
        {
            if ((angle > 0.0 && speedangle < angle) ||
                    (angle < 0.0 && speedangle > angle))
                skidangle += speedangle/2;
        }

#if 0
        // if over/understeering, decrease speed
        if (rldata->rInverse > 0.0)
        {
            double mod = ((laststeer-rldata->rInverse) * fabs(rldata->rInverse*15));
            if (mod < 0.0) // oversteer, will increase x but by less if skidding outwards
                mod /= 4 - MIN(0.0, skidangle * 10);
            x = MIN(x, 1.0) - mod;
        }
        else
        {
            double mod = ((laststeer-rldata->rInverse) * fabs(rldata->rInverse*15));
            if (mod > 0.0) // oversteer
                mod /= 4 + MAX(0.0, skidangle * 10);
            x = MIN(x, 1.0) + mod;
        }

#else
        if (//(mode == mode_normal || !sidedanger) &&
                (skidangle < 0.0 && laststeer > 0.0 && rldata->rInverse < -0.001) ||
                (skidangle > 0.0 && laststeer < 0.0 && rldata->rInverse > 0.001))
        {
            // increase acceleration if correcting a skid
            double diff = MAX(0.0, MIN(fabs(laststeer), MAX(fabs(skidangle/7)/1000, fabs(rldata->rInverse * 50)))) * MAX(0.0, MIN(2.0, (7-skid)));
            if (collision)
                diff *= MIN(1.0, collision / 3.0f)*0.8;
            x += diff * OversteerASR;
        }
        else if (mode != mode_normal &&
                 (car->_accel_x < 1.0 || sidedanger) &&
                 ((angle > 0.0 && laststeer > 0.0 && rldata->rInverse < -0.001) ||
                  (angle < 0.0 && laststeer < 0.0 && rldata->rInverse > 0.001)))
        {
            // understeering, decrease speed
            double diff = MIN(fabs(laststeer), MAX(fabs(angle)/50, fabs(rldata->rInverse * 50))) * 4;
            x -= diff;
        }
#endif

#if 0
        // check if we're really close behind someone on a bend
        // if so (and we're not overtaking) make sure we don't under-brake or
        // over-accelerate and thus lose control.
        fbrakecmd = faccelcmd = 0.0f;

        if (mode == mode_normal)
        {
            for (int i=0; i<opponents->getNOpponents(); i++)
            {
                tCarElt *ocar = opponent[i].getCarPtr();

                if (ocar == car)
                    continue;

                if (opponent[i].getTeam() != TEAM_FRIEND)
                    continue;

                if (!(opponent[i].getState() & OPP_FRONT) || opponent[i].getDistance() > 3.0)
                    continue;

                if (fabs(car->_trkPos.toLeft - ocar->_trkPos.toLeft) > car->_dimension_x*0.8)
                    continue;

                if (mode == mode_normal && opponent[i].isTeamMate() && opponent[i].getDistance() < 1.5)
                {
                    accelcmd = ocar->_accelCmd;
                    if (accelcmd > 0.0f)
                        faccelcmd = accelcmd;
                }

                /*
        if ((rldata->rInverse > 0.001 && car->_trkPos.toLeft > rldata->lane * car->_trkPos.seg->width) ||
            (rldata->rInverse < -0.001 && car->_trkPos.toLeft < rldata->lane * car->_trkPos.seg->width))
        {
          double change = MAX(currentspeed-rldata->speed, fabs(car->_trkPos.toLeft - (rldata->lane * car->_trkPos.seg->width)));

          x -= MAX(0.0, MIN(1.0, change));
          break;
        }
        */

                if (opponent[i].getDistance() < 1.5 && ocar->_brakeCmd > 0.0 && (opponent[i].getState() & OPP_COLL))
                {
                    brakecmd = ocar->_brakeCmd*MAX(1.0, currentspeed / opponent[i].getSpeed());
                    if (brakecmd > 0.0)
                    {
                        x = MIN(x, -0.001);
                        fbrakecmd = brakecmd;
                    }
                }

                break;
            }
        }
#endif
    }

    if (x > 0)
    {
        if (AccelMod == 0 || !rldata->exiting)
        {
            if (accelcmd < 100.0f)
                accelcmd = MIN(accelcmd, (float) x * 1.5f);
            else
                accelcmd = (float) x;
        }
        else
            accelcmd = 1.0f;
        accelcmd = (float)MAX(accelcmd, MinAccel);
    }
    else
        brakecmd = MIN(1.0f, MAX(brakecmd, (float) (-(MAX(10.0, brakedelay*0.7))*(x*1.5))));
}

int Driver::rearOffTrack()
{
    int right_bad = (car->_wheelSeg(REAR_RGT) != car->_trkPos.seg &&
            (car->_wheelSeg(REAR_RGT)->surface->kFriction < car->_trkPos.seg->surface->kFriction*0.8 ||
             car->_wheelSeg(REAR_RGT)->surface->kRoughness > MAX(0.02, car->_trkPos.seg->surface->kRoughness*1.2) ||
             car->_wheelSeg(REAR_RGT)->surface->kRollRes > MAX(0.005, car->_trkPos.seg->surface->kRollRes*1.2)));

    int left_bad =  (car->_wheelSeg(REAR_LFT) != car->_trkPos.seg &&
            (car->_wheelSeg(REAR_LFT)->surface->kFriction < car->_trkPos.seg->surface->kFriction*0.8 ||
             car->_wheelSeg(REAR_LFT)->surface->kRoughness > MAX(0.02, car->_trkPos.seg->surface->kRoughness*1.2) ||
             car->_wheelSeg(REAR_LFT)->surface->kRollRes > MAX(0.005, car->_trkPos.seg->surface->kRollRes*1.2)));

    if (left_bad && right_bad) return 1;

    if (car->_speed_x < 10.0 && (left_bad || right_bad)) return 1;

    return 0;
}

// Drive during race.
void Driver::drive(tSituation *s)
{
    laststeer = car->_steerCmd;
    memset(&car->ctrl, 0, sizeof(tCarCtrl));

    /* USR stores pit positions in car->_lightCmd, so we shift
   * that information 2 pos left and flip on the real
   * light commands. (Situation-aware) */
    car->_lightCmd = ((int)cmd_light << 2) | current_light;

    skipcount++;

    if (skipcount > SKIPLIMIT)
        skipcount = 0;

    if (skipcount > 1)
    {
        // potentially can skip (we never do if skipcount == 0)

        if (mode == mode_normal)
        {
            // driving on the raceline
            if (fabs(car->_yaw_rate) < 0.15 &&
                    fabs(car->_accel_x) > -2 &&
                    fabs(speedangle - angle) < 0.1)
            {
                // car not under stress, we can skip
                car->_accelCmd = cmd_accel;
                car->_brakeCmd = cmd_brake;
                car->_steerCmd = cmd_steer;
                car->_gearCmd = cmd_gear;
                car->_clutchCmd = cmd_clutch;
                return;
            }
        }
        else
        {
            // we're avoiding someone, don't skip (for the most part)
            if (skipcount > 2)
                skipcount = 0;
        }
    }


    update(s);

    //pit->setPitstop(true);

#ifdef CONTROL_SKILL
    calcSkill();
#endif

    car->_steerCmd = getSteer(s);

    if (!isStuck())
    {
        car->_gearCmd = getGear();
        calcSpeed();

        if (!HasABS && !HasESP)
            car->_brakeCmd = filterABS(filterBrakeSpeed(filterBColl(filterBPit(getBrake()))));
        else
            car->_brakeCmd = filterBrakeSpeed(filterBColl(filterBPit(getBrake())));

        if (car->_brakeCmd <= 0.001f)
        {
            if(!HasTCL)
                car->_accelCmd = filterTCL(filterTrk(filterTeam(filterOverlap(getAccel()))));
            else
                car->_accelCmd = filterTrk(filterTeam(filterOverlap(getAccel())));
        } else {
            car->_accelCmd = 0.0f;
        }

        if (!collision && fabs(car->_speed_x) < 1.0)
        {
            car->_accelCmd = MAX(car->_accelCmd, 0.4f);
            car->_brakeCmd = 0.0f;
        }

        if (car->_speed_x < -1.0f)
        {
            car->_accelCmd = 0.0f;
            car->_brakeCmd = 0.7f;
        }

        car->_clutchCmd = getClutch();
        if (DebugMsg & debug_steer)
            LogUSR.debug("%s %d/%d: ", car->_name, rldata->thisdiv, rldata->nextdiv);

    }

    if (DebugMsg & debug_steer)
    {
        double skid = (car->_skid[0]+car->_skid[1]+car->_skid[2]+car->_skid[3])/2;
        LogUSR.debug("%d%c%c%c s%.2f k%.2f ss%.2f cl%.3f g%d->%d brk%.3f acc%.2f dec%.2f coll%.1f %c",mode,((mode==mode_avoiding)?'A':' '),(avoidmode==avoidleft?'L':(avoidmode==avoidright?'R':' ')),(mode==mode_correcting?'c':' '),car->_steerCmd,rldata->ksteer,stucksteer,correctlimit,car->_gear,car->_gearCmd,car->_brakeCmd,car->_accelCmd,rldata->decel,collision,(rldata->closing?'c':'e'));
        LogUSR.debug(" spd%.1f|k%.1f|a%.1f|t%.1f angle=%.2f/%.2f/%.2f yr=%.2f skid=%.2f acxy=%.2f/%.2f inv%.3f/%.3f slip=%.3f/%.3f %.3f/%.3f\n",(double)currentspeed,(double)rldata->speed,(double)rldata->avspeed,(double)getTrueSpeed(),angle,speedangle,rldata->rlangle,car->_yaw_rate,skid,car->_accel_x,car->_accel_y,nextCRinverse,rldata->mInverse,car->_wheelSpinVel(FRNT_RGT)*car->_wheelRadius(FRNT_RGT)-car->_speed_x,car->_wheelSpinVel(FRNT_LFT)*car->_wheelRadius(FRNT_LFT)-car->_speed_x,car->_wheelSpinVel(REAR_RGT)*car->_wheelRadius(REAR_RGT)-car->_speed_x,car->_wheelSpinVel(REAR_LFT)*car->_wheelRadius(REAR_LFT)-car->_speed_x);
    }

    laststeer = car->_steerCmd;
    lastbrake = car->_brakeCmd;
    lastaccel = car->_accelCmd;
    lastmode = mode;
    prevleft = car->_trkPos.toLeft;

    cmd_accel = car->_accelCmd;
    cmd_brake = car->_brakeCmd;
    cmd_steer = car->_steerCmd;
    cmd_clutch = car->_clutchCmd;
    cmd_gear = car->_gearCmd;
    /* USR pit positions are now in car->_lightCmd, on positions 2 and 3
   * from the right, so we get that information shifting 2 pos right */
    cmd_light = (float)(car->_lightCmd >> 2);
}


// Set pitstop commands.
int Driver::pitCommand(tSituation *s)
{
    car->_pitRepair = strategy->pitRepair(car, s);
    car->_pitFuel = strategy->pitRefuel(car, s);
    // This should be the only place where the pit stop is set to false!
    pit->setPitstop(false);
    return ROB_PIT_IM; // return immediately.
}


// End of the current race for this driver, maybe it was eliminated from a
// continuing race, so release the pit.
void Driver::endRace(tSituation *s)
{
#ifdef SPEED_DREAMS
    RtTeamReleasePit(teamIndex);
#endif
    car->ctrl.raceCmd = 0;
}

// cleanup
void Driver::shutdown()
{
#ifdef SPEED_DREAMS
    RtTeamManagerRelease();
#endif
}

/***************************************************************************
 *
 * utility functions
 *
***************************************************************************/

void Driver::computeRadius(float *radius)
{
    float lastturnarc = 0.0f;
    int lastsegtype = TR_STR;

    tTrackSeg *currentseg, *startseg = track->seg;
    currentseg = startseg;

    do {
        if (currentseg->type == TR_STR)
        {
            lastsegtype = TR_STR;
            radius[currentseg->id] = FLT_MAX;
        } else
        {
            if (currentseg->type != lastsegtype)
            {
                float arc = 0.0f;
                tTrackSeg *s = currentseg;
                lastsegtype = currentseg->type;

                while (s->type == lastsegtype && arc < PI/2.0f)
                {
                    arc += s->arc;
                    s = s->next;
                }

                lastturnarc = (float) (arc/(PI/2.0f));
            }

            radius[currentseg->id] = (float) (currentseg->radius + currentseg->width/2.0)/lastturnarc;
        }

        currentseg = currentseg->next;
    } while (currentseg != startseg);

}

// Compute the length to the end of the segment.
float Driver::getDistToSegEnd()
{
    if (car->_trkPos.seg->type == TR_STR)
    {
        return car->_trkPos.seg->length - car->_trkPos.toStart;
    } else
    {
        return (car->_trkPos.seg->arc - car->_trkPos.toStart)*car->_trkPos.seg->radius;
    }
}

// Compute fitting acceleration.
float Driver::getAccel()
{
    if (car->_gear > 0)
    {
        accelcmd = MIN(1.0f, accelcmd);

        if (pit->getInPit() && car->_brakeCmd == 0.0f)
        {
            //float s = pit->toSplineCoord( car->_distFromStartLine );

#if 0
            if (pit->needPitstop())
            {
                if (currentspeedsqr > pit->getSpeedlimitSqr()*0.9)
                {
                    accelcmd = MIN(accelcmd, 0.4f);
                }
            }
#endif

            accelcmd = MIN(accelcmd, 0.6f);
        }
        else if (fabs(angle) > 0.8 && currentspeed > 10.0f)
            accelcmd = MAX(0.0f, MIN(accelcmd, 1.0f - currentspeed/100.0f * fabs(angle)));

        return accelcmd;
    } else
    {
        return 1.0;
    }
}

// If we get lapped reduce accelerator.
float Driver::filterOverlap(float accel)
{
    int i;

    if (!(avoidmode & avoidback))
    {
        return accel;
    }

    for (i = 0; i < opponents->getNOpponents(); i++)
    {
        if (opponent[i].getState() & OPP_LETPASS)
        {
            return accel*0.4f;
        }
    }
    return accel;
}

// slows us down to allow a team-member to catch up
float Driver::filterTeam(float accel)
{
    if (mode != mode_normal) return accel;

    double minaccel = accel;
    double closest = -10000.0;
    int i;

    // first filter for following closely
#if 0
    for (i = 0; i < opponents->getNOpponents(); i++)
    {
        if (opponent[i].getCarPtr() == car) continue;
        if ((opponent[i].getTeam() != TEAM_FRIEND) || (opponent[i].getCarPtr()->_msgColorCmd[0] == 1.0))
            continue;

        if (opponent[i].getDistance() > 3.0 || opponent[i].getDistance() < 0.0)
            break;

        minaccel = accel = MIN(accel, opponent[i].getCarPtr()->_accelCmd*0.9);
        car->_brakeCmd = MAX(car->_brakeCmd, opponent[i].getCarPtr()->_brakeCmd*0.7);
    }
#endif

    if (NoTeamWaiting) return accel;

    // now filter to wait for catching up
    for (i = 0; i < opponents->getNOpponents(); i++)
    {
        if (opponent[i].getCarPtr() == car) continue;
        if (opponent[i].getTeam() & TEAM_FRIEND) continue;

        if (opponent[i].getDistance() < 0.0 && opponent[i].getDistance() > closest)
            closest = opponent[i].getDistance();

        if (opponent[i].getCarPtr()->_pos < car->_pos)
        {
            if (opponent[i].getDistance() < -150.0)
            {
                return accel;
            }
        }

        if (opponent[i].getCarPtr()->_pos >= car->_pos + 2 &&
                opponent[i].getCarPtr()->_laps == car->_laps &&
                opponent[i].getDistance() > -(car->_speed_x*2) &&
                opponent[i].getDistance() < 0.0)
        {
            return accel;
        }
    }

    for (i = 0; i < opponents->getNOpponents(); i++)
    {
        if (opponent[i].getCarPtr()->_state == RM_CAR_STATE_PIT
                || opponent[i].getCarPtr()->_state == RM_CAR_STATE_PULLUP
                || opponent[i].getCarPtr()->_state == RM_CAR_STATE_PULLDN
                || opponent[i].getCarPtr()->_state == RM_CAR_STATE_OUT)
            continue;

        if (opponent[i].getCarPtr() == car) continue;
        if (!(opponent[i].getTeam() & TEAM_FRIEND))
            continue;
        if (opponent[i].getDistance() > -25.0)
            continue;

        double time_behind = fabs(opponent[i].getDistance()) / opponent[i].getCarPtr()->_speed_x;

        if ((opponent[i].getTeam() & TEAM_FRIEND) &&
                opponent[i].getCarPtr()->_laps >= car->_laps &&
                opponent[i].getCarPtr()->_dammage < car->_dammage + 2000 &&
                ((time_behind <= TeamWaitTime && time_behind > 0.4) ||
                 (opponent[i].getDistance() < 0.0 && opponent[i].getDistance() > -(car->_speed_x * TeamWaitTime))) &&
                opponent[i].getDistance() > closest &&
                opponent[i].getDistance() < -25.0)
        {
            return MIN(accel, 0.9f);
        }
    }


    return (float)minaccel;
}


// Compute initial brake value.
float Driver::getBrake()
{
    // Car drives backward?
    if (car->_speed_x < -MAX_UNSTUCK_SPEED) {
        // Yes, brake.
        return 1.0;
    } else {
        // We drive forward, normal braking.
#ifdef CONTROL_SKILL
        brakecmd *= (float)brake_adjust_perc;
#endif
        return brakecmd;
    }
}


// Compute gear.
int Driver::getGear()
{
    car->_gearCmd = car->_gear;
    if (car->_gear <= 0) {
        return 1;
    }
    if (1 || car->_gear > 2)
    {
        // Hymie gear changing
        float speed = currentspeed;
        float *tRatio = car->_gearRatio + car->_gearOffset;
        float rpm = (float) ((speed + 0.5) * tRatio[car->_gear] / car->_wheelRadius(2));
        float down_rpm = (float) (car->_gear > 1 ? (speed + 0.5) * tRatio[car->_gear-1] / car->_wheelRadius(2) : rpm);

        float rcu = (car->_gear < 6 && car->_gear >= 0 ? GearRevsChangeUp[car->_gear] : RevsChangeUp);
        float rcd = (car->_gear < 6 && car->_gear >= 0 ? GearRevsChangeDown[car->_gear] : RevsChangeDown);
        float rcm = (car->_gear < 6 && car->_gear >= 0 ? GearRevsChangeDownMax[car->_gear] : RevsChangeDownMax);

        if (rpm + MAX(0.0, (double) (car->_gear-3) * (car->_gear-3)*10) > car->_enginerpmMax * rcu && car->_gear < MaxGear)
            car->_gearCmd = car->_gear + 1;

        if (car->_gear > 1 &&
                rpm < car->_enginerpmMax * rcd &&
                down_rpm < car->_enginerpmMax * rcm)
            car->_gearCmd = car->_gear - 1;
    }
    else
    {
        // BT gear changing
        float gr_up = car->_gearRatio[car->_gear + car->_gearOffset];
        float omega = car->_enginerpmRedLine/gr_up;
        float wr = car->_wheelRadius(2);

        if (omega*wr*SHIFT < car->_speed_x) {
            car->_gearCmd = car->_gear + 1;
        } else {
            float gr_down = car->_gearRatio[car->_gear + car->_gearOffset - 1];
            omega = car->_enginerpmRedLine/gr_down;
            if (car->_gear > 1 && omega*wr*SHIFT > car->_speed_x + SHIFT_MARGIN) {
                car->_gearCmd = car->_gear - 1;
            }
        }
    }

    return car->_gearCmd;
}


void Driver::setMode( int newmode )
{
    if (mode == newmode)
        return;

    if (mode == mode_normal || mode == mode_pitting)
    {
        correcttimer = simtime + 7.0;
        //correctlimit = 1000.0;
    }

    if (newmode == mode_avoiding && mode != mode_avoiding)
        avoidtime = simtime;

    mode = newmode;

    switch (newmode) {
    case mode_normal:
        current_light = RM_LIGHT_HEAD1 | RM_LIGHT_HEAD2;
        break;
    case mode_pitting:
        current_light = RM_LIGHT_HEAD2;
        break;
    case mode_avoiding:
        current_light = RM_LIGHT_HEAD1;
        break;
    }
}

void Driver::calcSkill()
{
#ifdef CONTROL_SKILL
    //if (RM_TYPE_PRACTICE != racetype)
    {
        if (skill_adjust_timer == -1.0 || simtime - skill_adjust_timer > skill_adjust_limit)
        {
            double rand1 = (double) getRandom() / 65536.0;  // how long we'll change speed for
            double rand2 = (double) getRandom() / 65536.0;  // the actual speed change
            double rand3 = (double) getRandom() / 65536.0;  // whether change is positive or negative

            // acceleration to use in current time limit
            decel_adjust_targ = (skill/4 * rand1);

            // brake to use - usually 1.0, sometimes less (more rarely on higher skill)
            brake_adjust_targ = MAX(0.85, 1.0 - MAX(0.0, skill/15 * (rand2-0.85)));

            // how long this skill mode to last for
            skill_adjust_limit = 5.0 + rand3 * 50.0;
            skill_adjust_timer = simtime;
        }

        if (decel_adjust_perc < decel_adjust_targ)
            decel_adjust_perc += MIN(deltaTime*4, decel_adjust_targ - decel_adjust_perc);
        else
            decel_adjust_perc -= MIN(deltaTime*4, decel_adjust_perc - decel_adjust_targ);

        if (brake_adjust_perc < brake_adjust_targ)
            brake_adjust_perc += MIN(deltaTime*2, brake_adjust_targ - brake_adjust_perc);
        else
            brake_adjust_perc -= MIN(deltaTime*2, brake_adjust_perc - brake_adjust_targ);
        LogUSR.debug("skill: decel %.3f - %.3f, brake %.3f - %.3f\n", decel_adjust_perc, decel_adjust_targ, brake_adjust_perc, brake_adjust_targ);
    }
#endif
}

double Driver::getFollowDistance()
{
    double mindist = 1000.0;

    if (mode != mode_normal)
        return mindist;

    for (int i = 0; i < opponents->getNOpponents(); i++)
    {
        if (opponent[i].getCarPtr() == car) continue;
        //if (opponent[i].getTeam() != TEAM_FRIEND) continue;
        if (!(opponent[i].getState() & OPP_FRONT))
            continue;

        if (opponent[i].getDistance() > 5.0)
            continue;

        mindist = MIN(mindist, opponent[i].getDistance()) - FollowMargin;
    }
    return mindist;
}

// Compute steer value.
float Driver::getSteer(tSituation *s)
{
    double targetAngle;
    memset(rldata, 0, sizeof(LRaceLineData));
    rldata->angle = angle;
    rldata->speedangle = speedangle;
    rldata->mode = mode;
    rldata->avoidmode = avoidmode;
    rldata->collision = collision;
    rldata->steer = rldata->laststeer = laststeer;
    rldata->alone = alone;
    rldata->followdist = getFollowDistance();
    rldata->s = s;
    rldata->aligned_time = simtime - aligned_timer;
    raceline->GetRaceLineData( s, rldata );
    if (FuelSpeedUp)
    {
        double fuel = (car->_fuel/maxfuel);
        fuel = MIN(1.0, fuel * (fuel+0.15));
        rldata->speed += FuelSpeedUp * (1.0 - fuel);
    }
    double steer = 0.0, tmpsteer = 0.0;
    double avoidsteer = 0.0;
    double racesteer = (rldata->ksteer);
    vec2f target;

#if 0
    if (SteerLookahead > 6.0f)
    {
        raceline->GetSteerPoint( (double) SteerLookahead, &target );
        targetAngle = atan2(target.y - car->_pos_Y, target.x - car->_pos_X);
        racesteer = calcSteer( targetAngle, 0 );
        double rIf = MIN(1.0, fabs(rldata->rInverse) / SteerMaxRI);
        racesteer = racesteer + (rldata->ksteer - racesteer) * rIf;
    }
#endif

    target = getTargetPoint(false);
    steer = avoidsteer = racesteer;
    lastNSksteer = (float)rldata->NSsteer;

    if (mode != mode_normal || SteerLookahead < 6.0f)
    {
        targetAngle = atan2(target.y - car->_pos_Y, target.x - car->_pos_X);
        tmpsteer = calcSteer( targetAngle, 0 );
    }


    if (mode != mode_normal)
    {
        avoidsteer = tmpsteer;

        if (mode == mode_pitting)
        {
            correctlimit = (avoidsteer - racesteer);
            return (float)avoidsteer;
        }

        //targetAngle = atan2(rldata->target.y - car->_pos_Y, rldata->target.x - car->_pos_X);

        allowcorrecting = 0;
        if (mode == mode_avoiding &&
                (!avoidmode ||
                 (avoidmode == avoidright && racesteer > avoidsteer) ||
                 (avoidmode == avoidleft && racesteer < avoidsteer)))
        {
            // we're avoiding, but trying to steer somewhere the raceline takes us.
            // hence we'll just correct towards the raceline instead.
            allowcorrecting = 1;
        }

        bool yr_ok = (fabs(car->_yaw_rate) < 0.1 || (car->_yaw_rate > rldata->rInverse*100-0.1 && car->_yaw_rate < rldata->rInverse*100+0.1));
        bool angle_ok = (angle > rldata->rlangle-0.06 && angle < rldata->rlangle+0.06);
        //double rlsteer = (fabs(rldata->rInverse) >= 0.004 ? rldata->rInverse * 12 : 0.0);
        bool steer_ok = (racesteer<laststeer+0.05 && racesteer>laststeer-0.05);

        double skid = (car->_skid[0] + car->_skid[1] + car->_skid[2] + car->_skid[3]) / 2;
        if (mode == mode_correcting)
        {
            if (lastmode == mode_normal ||
                    (angle_ok &&
                     (simtime > 15.0 || car->_speed_x > 20) &&
                     yr_ok &&
                     skid < 0.1 &&
                     steer_ok &&
                     ((fabs(car->_trkPos.toMiddle) < car->_trkPos.seg->width/2 - 1.0) || car->_speed_x < 10.0) &&
                     (raceline->isOnLine())))
            {
                // we're correcting & are now close enough to the raceline to
                // switch back to 'normal' mode...
                setMode( mode_normal );
                aligned_timer = simtime;
                steer = racesteer;
                if (DebugMsg & debug_steer)
                    LogUSR.debug("ALIGNED steer_ok=%d avsteer=%.3f racest=%.3f\n", steer_ok, avoidsteer, racesteer);
            }
            else if (DebugMsg & debug_steer)
                LogUSR.debug("NOT ALIGNED %d %d %d %d %.2f %.2f %.2f\n", angle_ok, yr_ok, (skid<0.1), steer_ok, avoidsteer,racesteer,laststeer);
        }

        if (mode != mode_normal)
        {
            if (mode != mode_correcting && !allowcorrecting)
            {
                int flying = checkFlying();
                if (flying & FLYING_FRONT)
                {
                    steer = 0.0;
                }
                else if (flying & FLYING_BACK)
                {
                    steer /= 3.0;
                }
                else
                {
                    //correctlimit = 1000.0;
                    correcttimer = simtime + 7.0;
                    steer = ( avoidsteer );
                }
                double climit = (steer - racesteer);
                if (fabs(climit) > fabs(correctlimit))
                    correctlimit = climit;
            }
            else
            {
                steer = (float)(correctSteering( (float)avoidsteer, (float)racesteer ));
                correctlimit = (steer - racesteer);
            }

            if (fabs(angle) >= 1.6)
            {
                if (steer > 0.0)
                    steer = 1.0;
                else
                    steer = -1.0;
            }
        }
        else
            correctlimit = (steer - racesteer);
    }
    else
    {
        raceline->NoAvoidSteer();
        lastNSasteer = (float)rldata->NSsteer;
        correctlimit = (steer - racesteer);
    }


    if (mode == mode_avoiding &&
            (lastmode == mode_normal || lastmode == mode_correcting) &&
            !((avoidmode & avoidright) && (avoidmode & avoidleft)))
    {
        // if we're only avoiding on one side, and racesteer avoids more than avoidsteer, and just prior we
        // weren't avoiding, return to prior mode.
        if ((racesteer >= steer && avoidmode == avoidright) ||
                (racesteer <= steer && avoidmode == avoidleft))
        {
            if (lastmode == mode_normal)
                steer = racesteer;
            setMode(lastmode);
        }
    }

    return (float)steer;
}


double Driver::calcSteer( double targetAngle, int rl )
{
#if 1
    if (mode != mode_pitting)
    {
        float kksteer = (float)raceline->getAvoidSteer(myoffset, rldata);
        return kksteer;
    }
#endif

    double rearskid = MAX(0.0, MAX(car->_skid[2], car->_skid[3]) - MAX(car->_skid[0], car->_skid[1])) + MAX(car->_skid[2], car->_skid[3]) * fabs(angle)*0.9;
    double steer = 0.0;

#if 0  // olethros steering
    double steer_direction = targetAngle - car->_yaw - 0.1 * car->_yaw_rate;

    double avoidance = 0.0;
    if (!(pit->getInPit()))
    {
        if (car->_trkPos.toRight < car->_dimension_y)
            avoidance = tanh(0.2 * (car->_dimension_y - car->_trkPos.toRight));
        else if (car->_trkPos.toLeft < car->_dimension_y)
            avoidance = tanh(0.2 * (car->_trkPos.toLeft - car->_dimension_y));
    }

    tTrackSeg *seg = car->_trkPos.seg;
    double correct_drift = -0.01 * atan2(car->_speed_Y, car->_speed_X);
    NORM_PI_PI(steer_direction);

    steer = avoidance + correct_drift + steer_direction/car->_steerLock;


#else  // old ki steering
    double steer_direction = targetAngle - car->_yaw - car->_speed_x/300 * car->_yaw_rate;

    NORM_PI_PI(steer_direction);
    if (DebugMsg & debug_steer)
        LogUSR.debug("STEER tm%.2f off%.2f sd%.3f", car->_trkPos.toMiddle, myoffset, steer_direction);

    if (car->_speed_x > 10.0 && mode != mode_normal && mode != mode_pitting)
    {
        double limit = MAX(20.0, 90.0 - car->_speed_x) * (((avoidmode & avoidside) ? 0.0074 : 0.0045) * SmoothSteer);
        double rgtlimit = limit, lftlimit = limit;
        if (laststeer_direction > 0.0) rgtlimit = MIN(laststeer_direction, rgtlimit*2);
        if (laststeer_direction < 0.0) lftlimit = MIN(-laststeer_direction, lftlimit*2);

        steer_direction = MAX(laststeer_direction - rgtlimit, MIN(laststeer_direction + lftlimit, steer_direction));

#if 1
        double speedsteer = (80.0 - MIN(70.0, MAX(40.0, currentspeed))) /
                ((185.0 * MIN(1.0, car->_steerLock / 0.785)) +
                 (185.0 * (MIN(1.3, MAX(1.0, 1.0 + rearskid))) - 185.0));
        if (fabs(steer_direction) > speedsteer)
        {
            steer_direction = MAX(-speedsteer, MIN(speedsteer, steer_direction));
        }
#endif
    }
    laststeer_direction = steer_direction;

    steer = (steer_direction/car->_steerLock);
    if (DebugMsg & debug_steer)
        LogUSR.debug("/sd%.3f a%.3f", steer_direction, steer);

    if (DebugMsg & debug_steer)
        LogUSR.debug(" b%.3f", steer);

    lastNSasteer = (float)steer;

    double nextangle = angle + car->_yaw_rate/3;
    if (fabs(nextangle) > fabs(speedangle))
    {
        // steer into the skid
        //double sa = MAX(-0.3, MIN(0.3, speedangle/3));
        //double anglediff = (sa - angle) * (0.3 + fabs(angle)/6);
        double anglediff = (speedangle - nextangle) * (0.1 + fabs(nextangle)/6);
        steer += (float) (anglediff*SkidSteer);
    }

    if (fabs(angle) > 1.2)
    {
        if (steer > 0.0f)
            steer = 1.0f;
        else
            steer = -1.0f;
    }
    else if (fabs(car->_trkPos.toMiddle) - car->_trkPos.seg->width/2 > 2.0)
    {
        steer = (float) MIN(1.0f, MAX(-1.0f, steer * (1.0f + (fabs(car->_trkPos.toMiddle) - car->_trkPos.seg->width/2)/14 + fabs(angle)/2)));
    }
#endif  // old ki steering

    if (DebugMsg & debug_steer)
        LogUSR.debug(" d%.3f", steer);

    if (mode != mode_pitting)
    {
        // limit how far we can steer against last steer...
        double limit = 0.0;
        limit = ((90.0 - MAX(40.0, MIN(60.0, car->_speed_x))) / (120)) * SmoothSteer; // (130);
        if (fabs(laststeer) > fabs(steer))
            limit = MAX(limit, fabs(laststeer)/2);
        steer = MAX(laststeer-limit, MIN(laststeer+limit, steer));
        //steer = MAX(rldata->steer-limit, MIN(rldata->steer+limit, steer));

#if 1
        if (simtime > 3.0)
        {
            // and against raceline...
            double climit = (correctlimit);
#if 0
            if (climit > 0.0 && angle - rldata->rlangle > 0.0)
                climit += (angle - rldata->rlangle)/2;
            else if (climit < 0.0 && angle - rldata->rlangle < 0.0)
                climit -= fabs(angle - rldata->rlangle)/2;
#endif

            double limitchange = ((90.0 - MAX(40.0, MIN(60.0, car->_speed_x))) / ((avoidmode & avoidside) ? 130 : 200)) * 3;
            climit += limitchange;
            steer = MAX(rldata->ksteer-climit, MIN(rldata->ksteer+climit, steer));
        }
#endif
        steer = smoothSteering((float)steer);
    }
#if 0
    else if (currentspeed > MAX(25.0, pit->getSpeedlimit()))
    {
        // stop sudden steer changes while pitting
        double limit = MAX(0.1, (40 - fabs(currentspeed - pit->getSpeedlimit())) * 0.025);
        steer = MIN(limit, MAX(-limit, steer));
    }
#endif

    if (DebugMsg & debug_steer)
        LogUSR.debug(" e%.3f\n", steer);

    return steer;
}

int Driver::checkFlying()
{
    int i = 0;
    if (car->_speed_x < 20)
        return 0;

    if (car->priv.wheel[0].relPos.z < wheelz[0] &&
            car->priv.wheel[1].relPos.z < wheelz[1])
    {
        i += FLYING_FRONT;
    }
    if (car->priv.wheel[2].relPos.z < wheelz[2]-0.05 &&
            car->priv.wheel[3].relPos.z < wheelz[3]-0.05)
    {
        i += FLYING_BACK;
    }
    if (!i)
    {
        if ((car->priv.wheel[0].relPos.z < wheelz[0] &&
             car->priv.wheel[2].relPos.z < wheelz[2] - 0.05) ||
                (car->priv.wheel[1].relPos.z < wheelz[1] &&
                 car->priv.wheel[3].relPos.z < wheelz[3] - 0.05))
        {
            i = FLYING_SIDE;
        }
    }

    return i;
}

float Driver::correctSteering( float avoidsteer, float racesteer )
{
    if (simtime < 15.0 && car->_speed_x < 20.0)
        return avoidsteer;
    if (simtime < CorrectDelay)
        return avoidsteer;

    float steer = avoidsteer;
    //float accel = MIN(0.0f, car->_accel_x);
    double speed = 50.0; //MAX(50.0, currentspeed);
    double changelimit = raceline->correctLimit(avoidsteer, racesteer, rldata->insideline) / 5;
    //double changelimit = MIN(raceline->correctLimit(avoidsteer, racesteer), (((120.0-currentspeed)/100) * (0.1 + fabs(rldata->mInverse/4)))) * SmoothSteer;

    double climit = fabs(correctlimit * changelimit);

    if (DebugMsg & debug_steer)
        LogUSR.debug("CORRECT: cl=%.3f/%.3f=%.3f as=%.3f rs=%.3f NS=%.3f",correctlimit,changelimit,climit,avoidsteer,racesteer,lastNSasteer);
    if (/*mode == mode_correcting &&*/ simtime > 2.0f)
    {
        // move steering towards racesteer...
        if (fabs(correctlimit) < 900.0)
        {
            if (steer < racesteer)
            {
                if (fabs(steer-racesteer) <= car->_speed_x / 2000)
                {
                    //steer = (float) MIN(racesteer, steer + correctlimit);
                    if (DebugMsg & debug_steer) LogUSR.debug(" RA%.3f", racesteer);
                    steer = racesteer;
                    lastNSasteer = (float)rldata->NSsteer;
                }
                else
                {
                    steer = (float) MIN(racesteer, MAX(steer + climit, racesteer - fabs(correctlimit) + climit));
                    lastNSasteer = (float) MIN(rldata->NSsteer, MAX(lastNSasteer, rldata->NSsteer + climit));
                    if (DebugMsg & debug_steer) LogUSR.debug(" MA%.3f", steer);
                }
            }
            else
            {
                if (fabs(steer-racesteer) <= car->_speed_x / 2000)
                {
                    //steer = (float) MAX(racesteer, steer-climit);
                    steer = racesteer;
                    lastNSasteer = (float)rldata->NSsteer;
                    if (DebugMsg & debug_steer) LogUSR.debug(" RB%.3f", racesteer);
                }
                else
                {
                    steer = (float) MAX(racesteer, MIN(steer - fabs(climit), racesteer + fabs(correctlimit) + climit));
                    lastNSasteer = (float) MAX(rldata->NSsteer, MIN(lastNSasteer, rldata->NSsteer + climit));
                    if (DebugMsg & debug_steer) LogUSR.debug(" MB%.3f", steer);
                }
            }
        }
        //else
        {
            speed -= avgaccel_x/10;
            speed = MAX(55.0, MIN(150.0, speed + (speed*speed/55)));
            double rInverse = rldata->mInverse * (avgaccel_x<0.0 ? 1.0 + fabs(avgaccel_x)/10.0 : 1.0);
            double correctspeed = 0.5;
            if ((rInverse > 0.0 && racesteer > steer) || (rInverse < 0.0 && racesteer < steer))
                correctspeed += rInverse*110;

            if (racesteer > steer)
                //steer = (float) MIN(racesteer, steer + (((155.0-speed)/10000) * correctspeed));
                steer = (float) MIN(racesteer, steer + changelimit);
            else
                //steer = (float) MAX(racesteer, steer - (((155.0-speed)/10000) * correctspeed));
                steer = (float) MAX(racesteer, steer - changelimit);
            if (fabs(racesteer) < fabs(steer))
            {
                if (racesteer > steer)
                    steer += (fabs(steer) - fabs(racesteer)) / 2;
                else
                    steer -= (fabs(steer) - fabs(racesteer)) / 2;
            }

            if (lastNSksteer > lastNSasteer)
                //lastNSasteer = (float) MIN(rldata->NSsteer, lastNSasteer + (((155.0-speed)/10000) * correctspeed));
                lastNSasteer = (float) MIN(rldata->NSsteer, lastNSasteer + changelimit);
            else
                //lastNSasteer = (float) MAX(rldata->NSsteer, lastNSasteer - (((155.0-speed)/10000) * correctspeed));
                lastNSasteer = (float) MAX(rldata->NSsteer, lastNSasteer - changelimit);
            if (DebugMsg & debug_steer) LogUSR.debug(" I%.3f", steer);
        }

    }

    if (DebugMsg & debug_steer) LogUSR.debug(" %.3f NS=%.3f\n", steer, lastNSasteer);

    return steer;
}


float Driver::smoothSteering( float steercmd )
{
    // don't smooth steering unless going into pits
    if (pitoffset != -100.0f)
        return steercmd;

#if 1
    // experimental smoothing code, beware!!!
    double steer = steercmd;
    double stdelta = steer - laststeer;//car->_steerCmd;
    double maxSpeed = MAX(200.0, 300.0 - car->_speed_x*2) * (PI/180.0);

    //if (mode == mode_normal)
    //  maxSpeed = 200.0 * (PI / 180.0);

    if ((fabs(stdelta) / deltaTime) > maxSpeed)
        steer = SIGN(stdelta) * maxSpeed * deltaTime + laststeer;//car->_steerCmd;

    steercmd = (float)steer;

    // limit amount of steer according to speed & raceline
    double a_error_factor = ((rldata->exiting && rldata->outsideline) ? 0.9 : 0.8);
    double smangle = angle * (0.5 + fabs(angle*2));
    double angle_error = (smangle - rldata->rlangle/2) * a_error_factor;
    double lstlimit = MAX(40.0, 80.0 - car->_speed_x) * 0.004 - MIN(0.0, MAX(-0.5, angle_error));
    double rstlimit = -(MAX(40.0, 80.0 - car->_speed_x) * 0.004 + MAX(0.0, MIN(0.5, angle_error)));
    double strate = 61.0 + lastaccel*10;

    if (rldata->rInverse*strate > lstlimit)
        lstlimit = rldata->rInverse*strate;
    if (rldata->rInverse*strate < rstlimit)
        rstlimit = rldata->rInverse*strate;
    steercmd = (float)MAX(rstlimit, MIN(lstlimit, steercmd));
#endif

    return steercmd;
#if 0
    // try to limit sudden changes in steering to avoid loss of control through oversteer.
    double lftspeedfactor = ((((60.0 - (MAX(40.0, MIN(70.0, currentspeed + MAX(0.0, car->_accel_x*5))) - 25)) / 300) * 2.5) / 0.585) * SmoothSteer;
    double rgtspeedfactor = lftspeedfactor;

    if (fabs(steercmd) < fabs(laststeer) && fabs(steercmd) <= fabs(laststeer - steercmd))
    {
        lftspeedfactor *= 2;
        rgtspeedfactor *= 2;
    }

    lftspeedfactor -= MIN(0.0f, car->_yaw_rate/10);
    rgtspeedfactor += MAX(0.0f, car->_yaw_rate/10);

    steercmd = (float) MAX(laststeer - rgtspeedfactor, MIN(laststeer + lftspeedfactor, steercmd));
    return steercmd;
#endif
}

// Compute the clutch value.
float Driver::getClutch()
{
    if (1 || car->_gearCmd > 1)
    {
        float maxtime = MAX(0.06f, 0.32f - ((float) car->_gearCmd / 65.0f));
        if (car->_gear != car->_gearCmd && car->_gearCmd < MaxGear)
            clutchtime = maxtime;
        if (clutchtime > 0.0f)
            clutchtime -= (float) (RCM_MAX_DT_ROBOTS * (0.02f + ((float) car->_gearCmd / 8.0f)));
        return 2.0f * clutchtime;
    } else {
        float drpm = car->_enginerpm - car->_enginerpmRedLine/2.0f;
        float ctlimit = 0.9f;
        if (car->_gearCmd > 1)
            ctlimit -= 0.15f + (float) car->_gearCmd/13.0f;
        clutchtime = MIN(ctlimit, clutchtime);
        if (car->_gear != car->_gearCmd)
            clutchtime = 0.0f;
        float clutcht = (ctlimit - clutchtime) / ctlimit;
        if (car->_gear == 1 && car->_accelCmd > 0.0f) {
            clutchtime += (float) RCM_MAX_DT_ROBOTS;
        }

        if (car->_gearCmd == 1 || drpm > 0) {
            float speedr;
            if (car->_gearCmd == 1) {
                // Compute corresponding speed to engine rpm.
                float omega = car->_enginerpmRedLine/car->_gearRatio[car->_gear + car->_gearOffset];
                float wr = car->_wheelRadius(2);
                speedr = (CLUTCH_SPEED + MAX(0.0f, car->_speed_x))/fabs(wr*omega);
                float clutchr = MAX(0.0f, (1.0f - speedr*2.0f*drpm/car->_enginerpmRedLine)) * (car->_gearCmd == 1 ? 0.95f : (0.7f - (float)(car->_gearCmd)/30.0f));
                return MIN(clutcht, clutchr);
            } else {
                // For the reverse gear.
                clutchtime = 0.0f;
                return 0.0f;
            }
        } else {
            return clutcht;
        }
    }
}

// move offset to allow for bends (if any)
float Driver::adjustOffset(float offset)
{
    return offset;
#if 0
    float adjustment = (float) (rldata->rInverse * 10);
    float width = (float) (car->_trkPos.seg->width * 0.75);

    //if (mode==mode_avoiding)
    {
        // we want to adjust outwards a bit if in close to the corner (more if avoiding
        // a car on the inside, way less otherwise).  If the car is on the outside third
        // of the track we don't want to adjust at all.  Inbetween the inside third and
        // the outside third we want a gradual decline in the adjust amount...
        if (adjustment < 0.0)
        {
            if (car->_trkPos.toRight > width*0.7)
                adjustment = 0.0;
            else if (car->_trkPos.toRight > width*0.3)
                adjustment *= (float)MAX(0.0, 1.0 - ((car->_trkPos.toRight-width*0.3)/width*0.4));
        }
        else if (adjustment > 0.0)
        {
            if (car->_trkPos.toLeft > width*0.7)
                adjustment = 0.0;
            else if (car->_trkPos.toLeft > width*0.3)
                adjustment *= (float)MAX(0.0, 1.0 - ((car->_trkPos.toLeft-width*0.3)/width*0.4));
        }
#if 0
        if (adjustment < 0.0 && car->_trkPos.toLeft < width)
            adjustment *= MAX(0.1, MIN(1.0, car->_trkPos.toLeft*1.6 / width));
        else if (adjustment > 0.0 && car->_trkPos.toRight < width)
            adjustment *= MAX(0.1, MIN(1.0, car->_trkPos.toRight*1.6 / width));
#endif

        //adjustment *= 1.0 + MAX(0.0, currentspeed / rldata->avspeed)*2;
        if ((avoidmode == avoidright && adjustment < 0.0) ||
                (avoidmode == avoidleft && adjustment > 0.0))
            adjustment *= 1;
        else
            adjustment /= 2;
    }

    double speed = currentspeed;
    double xspeed = MIN(rldata->speed, rldata->avspeed);
    if (speed < xspeed)
        adjustment *= (float)MIN(2.0, xspeed / speed);

    offset -= adjustment;

    return offset;
#endif
}

// Compute target point for steering.
vec2f Driver::getTargetPoint(bool use_lookahead, double targetoffset)
{
    tTrackSeg *seg = car->_trkPos.seg;
    float length = getDistToSegEnd();
    float offset = (targetoffset > -99 ? (float)targetoffset
                                       : (skipcount < 2 ? getOffset() : myoffset));
    double time_mod = 1.0;
    pitoffset = -100.0f;

    if (pit->getInPit()) {
        // To stop in the pit we need special lookahead values.
        if (currentspeedsqr > pit->getSpeedlimitSqr()) {
            lookahead = PIT_LOOKAHEAD + car->_speed_x*LOOKAHEAD_FACTOR;
        } else {
            lookahead = PIT_LOOKAHEAD;
        }
    } else {
        // Usual lookahead.
        lookahead = (float) rldata->lookahead;

        double speed = MAX(20.0, MIN(45.0, currentspeed));// + MAX(0.0, car->_accel_x));
        lookahead = (float) (LOOKAHEAD_CONST * 1.5 + speed * 0.45);
        lookahead = MIN(lookahead, (float) (LOOKAHEAD_CONST + ((speed*(speed/10)) * 0.15)));
        lookahead *= SteerLookahead;
        double ri = (fabs(rldata->mInverse) < fabs(rldata->rInverse) ? rldata->mInverse : rldata->rInverse);
        //double amI = MIN(0.05, MAX(-0.05, (rldata->amInverse)));
        double amI = MIN(0.05, MAX(-0.05, (ri)));
        double famI = fabs(amI);

        if (famI > 0.0)
        {
            double cornerfx = 1.0;
            double toMid = car->_trkPos.toMiddle + speedangle * 20;
            double modfactor = (currentspeed / rldata->avspeed);
            modfactor *= modfactor;
            if (amI > 0.0)
            {
                if (toMid < 0.0)
                {
                    cornerfx += famI * (MIN(track->width/2, fabs(toMid)) / track->width) * 40;
                    time_mod += famI * (MIN(track->width/2, fabs(toMid)) / track->width) * 40;
                }
                else
                {
                    cornerfx -= MIN(0.7, famI * (MIN(track->width/2, fabs(toMid)) / track->width) * 40);
                    time_mod -= MIN(0.7, famI * (MIN(track->width/2, fabs(toMid)) / track->width) * 40 * modfactor);
                }
            }
            else
            {
                if (toMid > 0.0)
                {
                    cornerfx += famI * (MIN(track->width/2, fabs(toMid)) / track->width) * 40;
                    time_mod += famI * (MIN(track->width/2, fabs(toMid)) / track->width) * 40;
                }
                else
                {
                    cornerfx -= MIN(0.7, famI * (MIN(track->width/2, fabs(toMid)) / track->width) * 40);
                    time_mod -= MIN(0.7, famI * (MIN(track->width/2, fabs(toMid)) / track->width) * 40 * modfactor);
                }
            }
            lookahead *= (float)cornerfx;
        }

        if (time_mod < oldtime_mod)
            time_mod = MAX(time_mod, oldtime_mod - deltaTime*2);
        oldtime_mod = (float)time_mod;


#if 0
        if (fabs(rldata->rInverse) > 0.001)
        {
            // increase lookahead if on the outside of a corner
            double cornerfx = 1.0;
            double cornerlmt = MIN(track->width/2, 1.0 + fabs(rldata->rInverse)*500);
            if (rldata->rInverse > 0.0 && car->_trkPos.toRight < cornerlmt)
                cornerfx = 1.0 + MAX(0.0, (cornerlmt-car->_trkPos.toRight)/cornerlmt)*1.5;
            else if (rldata->rInverse < 0.0 && car->_trkPos.toLeft < cornerlmt)
                cornerfx = (1.0 - MAX(0.0, (cornerlmt-car->_trkPos.toLeft)/cornerlmt))*1.5;
            lookahead *= cornerfx;
        }
#endif
#if 0
        lookahead = LOOKAHEAD_CONST + car->_speed_x*LOOKAHEAD_FACTOR;
        lookahead = MAX(lookahead, LOOKAHEAD_CONST + ((car->_speed_x*(car->_speed_x/2)) / 60.0));
#endif

#if 1
        lookahead *= LookAhead;

        // Prevent "snap back" of lookahead on harsh braking.
        float cmplookahead = (float)(oldlookahead - (car->_speed_x*RCM_MAX_DT_ROBOTS)*0.65);//0.55f;
        if (lookahead < cmplookahead) {
            lookahead = cmplookahead;
        }
#endif
    }

    oldlookahead = lookahead;

    // Search for the segment containing the target point.
    while (length < lookahead) {
        seg = seg->next;
        length += seg->length;
    }

    length = lookahead - length + seg->length;
    float fromstart = seg->lgfromstart;
    fromstart += length;

    // Compute the target point.
    pitoffset = pit->getPitOffset(pitoffset, fromstart, pitpos);
    if ((pit->getPitstop() || pit->getInPit()) && pitoffset != -100.0f)
    {
        setMode(mode_pitting);
        offset = myoffset = pitoffset;
    }
    else if (mode == mode_pitting)
    {
        setMode(mode_correcting);
    }

    //if (mode == mode_correcting || mode == mode_avoiding)
    //  offset = adjustOffset( offset );

    vec2f s;
    //if (mode != mode_pitting)
    {
        //double steertime = SteerTime * time_mod * MAX(1.0, (1.0 - (car->_accel_x/70 * MAX(0.1, MIN(1.0, collision)))));
        double steertime = MIN(MaxSteerTime, MinSteerTime + MAX(0.0, currentspeed-20.0)/30.0);
        if (car->_speed_x > SteerCutoff)
            time_mod *= SteerCutoff / currentspeed;
        raceline->GetSteerPoint( lookahead, &s, offset, (use_lookahead ? -100.0 : steertime * time_mod) );
        return s;
    }

#if 0
    // all the BT code below is for steering into pits only.
    s.x = (seg->vertex[TR_SL].x + seg->vertex[TR_SR].x)/2;
    s.y = (seg->vertex[TR_SL].y + seg->vertex[TR_SR].y)/2;
    //double dx, dy;
    vec2f t, rt;

    if ( seg->type == TR_STR) {
        vec2f d, n;
        n.x = (seg->vertex[TR_EL].x - seg->vertex[TR_ER].x)/seg->length;
        n.y = (seg->vertex[TR_EL].y - seg->vertex[TR_ER].y)/seg->length;
        n.normalize();
        d.x = (seg->vertex[TR_EL].x - seg->vertex[TR_SL].x)/seg->length;
        d.y = (seg->vertex[TR_EL].y - seg->vertex[TR_SL].y)/seg->length;
        t = s + d*length + offset*n;

        return t;
    } else {
        vec2f c, n;
        c.x = seg->center.x;
        c.y = seg->center.y;
        float arc = length/seg->radius;
        float arcsign = (seg->type == TR_RGT) ? -1.0f : 1.0f;
        arc = arc*arcsign;
        s = s.rotate(c, arc);

        n = c - s;
        n.normalize();
        t = s + arcsign*offset*n;

        return t;
    }
#endif
}


bool Driver::canOvertake2( Opponent *o, int avoidingside )
{
    tCarElt *ocar = o->getCarPtr();
    double oAspeed, oRInv;

    double distance = o->getDistance() * MAX(0.5, 1.0 - (ocar->_pos > car->_pos ? MIN(o->getDistance()/2, 3.0) : 0.0));

    if (avoidingside == TR_RGT)
    {
        double offset = MIN(car->_trkPos.toMiddle, ocar->_trkPos.toMiddle - (car->_dimension_y/2 + ocar->_dimension_y/2 + 2));
        raceline->getOpponentInfo( o->getDistance(), LINE_MID, &oAspeed, &oRInv, offset );
    }
    else
    {
        double offset = MAX(car->_trkPos.toMiddle, ocar->_trkPos.toMiddle + (car->_dimension_y/2 + ocar->_dimension_y/2 + 2));
        raceline->getOpponentInfo( o->getDistance(), LINE_MID, &oAspeed, &oRInv, offset );
    }

    oAspeed = MIN(oAspeed, o->getSpeed()+2.0);
    oAspeed = MAX(oAspeed, car->_speed_x - MIN(distance, o->getTimeImpact())/2);

    if (oAspeed >= o->getSpeed())  // speed on avoidside of opponent is fast enough
    {
        if (DebugMsg & debug_overtake)
            LogUSR.debug("-> %s: OVERTAKE2 ospd=%.1f oAspd=%.1f\n", ocar->_name, o->getSpeed(), oAspeed);
        return true;
    }

    if (DebugMsg & debug_overtake)
        LogUSR.debug("-> %s: FAIL2!!!! ospd=%.1f oAspd=%.1f\n", ocar->_name, o->getSpeed(), oAspeed);

    return false;
}

bool Driver::canOvertake( Opponent *o, double *mincatchdist, bool outside, bool lenient )
{
    if (!o) return false;

    //int segid = car->_trkPos.seg->id;
    tCarElt *ocar = o->getCarPtr();
    //int osegid = ocar->_trkPos.seg->id;
    double otry_factor = (lenient ? (0.2 + MAX(0.0, 1.0 - ((simtime-frontavoidtime)/7.0)) * 0.8) : 1.0);
    double overtakecaution = MAX(0.0, rldata->overtakecaution + (outside ? MIN(0.0, car->_accel_x/8) : 0.0)) - driver_aggression/2;
    double orInv=0.0, oAspeed=0.0;
    raceline->getOpponentInfo(o->getDistance(), LINE_RL, &oAspeed, &orInv);
    double rInv = MAX(fabs(rldata->rInverse), fabs(orInv));
    double distance = o->getDistance() * otry_factor * MAX(0.5, 1.0 - (ocar->_pos > car->_pos ? MIN(o->getDistance()/2, 3.0) : 0.0));
    double speed = currentspeed + MAX(0.0, (10.0 - distance)/2);
    double avspeed = MIN(rldata->avspeed, speed + 2.0);
    speed = MIN(avspeed, (speed + MAX(0.0, (30.0 - distance) * MAX(0.1, 1.0 - MAX(0.0, rInv-0.001)*80))));
    double ospeed = o->getSpeed();
    oAspeed = MIN(oAspeed, ospeed+2.0);
    oAspeed = MAX(oAspeed, car->_speed_x - MIN(distance, o->getTimeImpact())/2);
    double timeLimit = 3.0 - MIN(2.4, rInv * 1000);

    if (*mincatchdist > speed - ospeed)
    {
        // already overtaking a slower opponent

        if (DebugMsg & debug_overtake)
            LogUSR.debug("%.1f %s: IGNORE!!! spddiff=%.1f minspeed=%.1f\n", otry_factor, ocar->_name, speed-(ospeed+2*overtakecaution),*mincatchdist);

        return false;
    }

    if ((speed > ospeed + 2*overtakecaution + fabs(rInv) * 300 ||  // our speed quicker than opponent
         distance < 4.0 - (fabs(rInv) * 40)) &&                    // really really close
            oAspeed > ospeed &&                                        // avoid speed quicker than opponent
            (o->getTimeImpact() * (1.0+overtakecaution) < timeLimit || // approaching opponent quickly
             distance < MAX(3.0, speed/5)))                            // close behind opponent
    {
        // faster than opponent, overtake
        *mincatchdist = speed - ospeed;

        if (DebugMsg & debug_overtake)
            LogUSR.debug("%.1f %s: OVERTAKE! spd=%.1f ospd=%.1f oAspd=%.1f ti=%.1f\n", otry_factor, ocar->_name,speed,ospeed+2*overtakecaution,oAspeed,o->getTimeImpact());

        return true;
    }


    // not worth the risk, ignore opponent

    if (DebugMsg & debug_overtake)
        LogUSR.debug("%.1f %s: FAIL!!!!! spd=%.1f ospd=%.1f oAspd=%.1f ti=%.1f\n",otry_factor,ocar->_name,speed,ospeed+2*overtakecaution,oAspeed,o->getTimeImpact());

    return false;
}

// Compute offset to normal target point for overtaking or let pass an opponent.
float Driver::getOffset()
{
    int i, avoidmovt = 0;
    double /*catchdist,*/ mincatchdist = -1000.0, mindist = -1000.0;
    Opponent *o = NULL;
    double lane2left = rldata->lane * car->_trkPos.seg->width;
    double lane2right = car->_trkPos.seg->width-lane2left;
    avoidmode = 0;
    sideratio = 100.0;

    avoidlftoffset = car->_trkPos.seg->width / 2;
    avoidrgtoffset = -car->_trkPos.seg->width / 2;

    // Increment speed dependent.
    //double incspeed = MIN(40.0, MAX(30.0, currentspeed));
    //double incfactor = (MAX_INC_FACTOR*0.5 - MIN(incspeed/10, MAX_INC_FACTOR*0.5-0.5)) * 60 * IncFactor;
    //double incspeed = MIN(60.0, MAX(40.0, currentspeed)) - 10.0;
    //double incfactor = (MAX_INC_FACTOR - MIN(fabs(incspeed)/MAX_INC_FACTOR, (MAX_INC_FACTOR - 1.0f))) * (10.0f + MAX(0.0, (CA-1.9)*10)) * IncFactor;
    double incfactor = (MAX_INC_FACTOR - MIN(40.0/MAX_INC_FACTOR, (MAX_INC_FACTOR - 1.0f))) * (10.0f + MAX(0.0, (CA-1.9)*10)) * (IncFactor*2) * MAX(0.2, 1.0 - fabs(rldata->rInverse*90));

    //double rgtinc = incfactor * MIN(3.0, MAX(0.6, 1.0 + rldata->mInverse * (rldata->mInverse<0.0?-5:100)));
    //double lftinc = incfactor * MIN(3.0, MAX(0.6, 1.0 - rldata->mInverse * (rldata->mInverse>0.0?-5:100)));
    double ri = rldata->aInverse;
    double rgtinc = incfactor * MIN(3.0, MAX(0.6, 1.0 + (ri < 0.0 ? ri*4 : ri*MAX(5.0, car->_speed_x-25)*OutSteerFactor)));
    double lftinc = incfactor * MIN(3.0, MAX(0.6, 1.0 - (ri > 0.0 ? ri*4 : ri*MAX(5.0,car->_speed_x-25)*OutSteerFactor)));

    //double reduce_movt = MAX(0.01, 1.0 - (MIN(1.0, fabs(laststeer))*2 * fabs(angle-speedangle)*3));
#if 1
    double reduce_movt = MAX(0.1, MIN(1.0, 1.0 - fabs(angle*2 - laststeer)) * MAX(fabs(angle-speedangle), fabs(speedangle-angle))*1);
    reduce_movt = MIN(reduce_movt, MAX(0.1, 1.0 + fabs(car->_accel_x)/30));
    lftinc *= reduce_movt;
    rgtinc *= reduce_movt;
#else
    if (ri > 0.0)
        rgtinc *= MAX(0.0, MIN(1.0, 1.0 - (angle*2-car->_yaw_rate/2) * MIN(1.0, rldata->rInverse*40)));
    else if (ri < 0.0)
        lftinc *= MAX(0.0, MIN(1.0, 1.0 + (angle*2-car->_yaw_rate/2) * MIN(1.0, (-rldata->rInverse)*40)));
#endif

    double moffset = car->_trkPos.toMiddle;

    //double origoffset = moffset;

    double Width = car->_trkPos.seg->width;
#if 1
    double lftmargin = GetModD( tLftMargin, rldata->nextdiv );
    double rgtmargin = GetModD( tRgtMargin, rldata->nextdiv );
    double maxoffset = MIN(moffset+OVERTAKE_OFFSET_INC*lftinc*4, Width/2 - (car->_dimension_y+SideMargin+rgtmargin));
    double minoffset = MAX(moffset-OVERTAKE_OFFSET_INC*rgtinc*4, -(Width/2 - (car->_dimension_y+SideMargin+lftmargin)));

    double oldmax = maxoffset, oldmin = minoffset;

    {
        ri = (fabs(rldata->aInverse) < fabs(rldata->mInverse) ? rldata->aInverse : rldata->mInverse);
        if (rldata->mInverse > 0.0)
            maxoffset = MIN(maxoffset, MAX(Width/8, Width/2 - (1.5 + ri*1000)));
        else
            minoffset = MAX(minoffset, MIN(-Width/8, -Width/2 + (1.5 - ri*1000)));
    }
#endif


    //myoffset = car->_trkPos.toMiddle;
    if (mode == mode_normal)
    {
        moffset = rldata->offset;
        myoffset = (float) moffset;
    }
    else
    {
        // reduce amount we deviate from the raceline according to how long we've been avoiding, and also
        // how fast we're going.
        //double dspeed = MAX(0.0, rldata->speed - currentspeed) * 4;
        //double pspeed = MAX(1.0, 60.0 - (currentspeed - (30.0 + MAX(0.0, car->_accel_x) + dspeed))) / 10;

        // instead of toMiddle just aiming at where the car currently is, we move it in the direction
        // the car's travelling ... but less so if the speedangle is different from the car's angle.
        double sa = speedangle;
        //double anglechange = ((sa*0.8) * MAX(0.0, 1.0 - fabs(sa-angle)*(0.6+fabs(sa-angle)))) * 0.7;
        double anglechange = ((sa*1.0) * MAX(0.0, 1.0 - fabs(sa-angle))) * (fabs(currentspeed)/50);
        double toMiddle = car->_trkPos.toMiddle + anglechange*0;
        moffset = toMiddle;
        myoffset = (float) moffset;

        if (mode == mode_correcting && avoidtime < simtime)
            avoidtime += deltaTime*0.8;
        if (simtime - avoidtime > 3.0)
            avoidtime = simtime - 3.0;

        if (0)
        {
            if (toMiddle > rldata->offset)
                moffset = MIN(toMiddle, rldata->offset + (simtime-avoidtime) * 45.0 * incfactor/8);
            else
                moffset = MAX(toMiddle, rldata->offset - (simtime-avoidtime) * 45.0 * incfactor/8);
        }

#if 0
        if (fabs(car->_trkPos.toMiddle) < MAX(fabs(minoffset), fabs(maxoffset)))
        {
            if (anglechange > 0.0)
                minoffset = MIN(maxoffset, MAX(minoffset, car->_trkPos.toMiddle - MAX(0.1, 1.0 - anglechange*2)*2));
            else
                maxoffset = MAX(minoffset, MIN(maxoffset, car->_trkPos.toMiddle + MAX(0.1, 1.0 - fabs(anglechange*2))*2));
        }
#endif
    }

    {
        minoffset = MAX(minoffset, car->_trkPos.toMiddle - OVERTAKE_OFFSET_INC*rgtinc*100);
        maxoffset = MIN(maxoffset, car->_trkPos.toMiddle + OVERTAKE_OFFSET_INC*lftinc*100);
    }
    //myoffset = car->_trkPos.toMiddle;

    // Side Collision.
    for (i = 0; i < opponents->getNOpponents(); i++)
    {
        tCarElt *ocar = opponent[i].getCarPtr();

        if (ocar->_state & (RM_CAR_STATE_NO_SIMU & ~RM_CAR_STATE_PIT))
            continue;

        if (fabs(ocar->_trkPos.toMiddle) > Width/2 + 3.0 &&
                fabs(car->_trkPos.toMiddle-ocar->_trkPos.toMiddle) >= 5.0)
            continue;

        if ((opponent[i].getState() & OPP_SIDE))
        {
            o = &opponent[i];
            if (DebugMsg & debug_overtake)
                LogUSR.debug("%s SIDE %s\n", car->_name, ocar->_name);

            double sidedist = fabs(ocar->_trkPos.toLeft - car->_trkPos.toLeft);
            double sidemargin = opponent[i].getWidth()/2 + getWidth()/2 + 5.0f + MAX(fabs(rldata->rInverse), fabs(rldata->mInverse))*100;
            double side = (car->_trkPos.toMiddle-angle) - (ocar->_trkPos.toMiddle-opponent[i].getAngle());
            double sidedist2 = sidedist;
            if (side > 0.0)
            {
                // I'm on his left
                sidedist2 -= (o->getSpeedAngle() - speedangle) * 40;
                sidemargin -= MIN(0.0, rldata->rInverse*100);
            }
            else
            {
                // I'm on his right
                sidedist2 -= (speedangle - o->getSpeedAngle()) * 40;
                sidemargin += MAX(0.0, rldata->rInverse*100);
            }
            int closing = (sidedist2 < sidedist);

            if (sidedist < sidemargin || sidedist2 < sidemargin || opponent[i].getState() & OPP_COLL)
            {
                //double w = Width/WIDTHDIV-BORDER_OVERTAKE_MARGIN;
                //double sdiff = 2.0 - MAX(sidemargin-sidedist, sidemargin-sidedist2)/sidemargin;
                double sdiff = MAX(sidemargin-sidedist, sidemargin-sidedist2) + MAX(0.0, sidedist - sidedist2);

                if (side > 0.0) {
                    double linc = OVERTAKE_OFFSET_INC * (lftinc * MAX(0.2, MIN(1.1, sdiff/4)) * 1.5);
                    if (rldata->rInverse < 0.0)
                        linc -= rldata->rInverse * 200 * IncFactor;
                    myoffset += (float) linc;
                    //if (rldata->rInverse < 0.0)
                    //  myoffset -= rldata->rInverse * 500 * IncFactor;
                    avoidmovt = 1;
                    if (DebugMsg & debug_overtake)
                        LogUSR.debug("%s SIDE to Rgt %s, MOVING LEFT by %.3f, sm=%.3f sd=%.3f/%.3f sm-sd=%.3f mInv=%.3f\n",car->_name,ocar->_name,(float) (OVERTAKE_OFFSET_INC*lftinc*MAX(0.2f, MIN(1.5f, sdiff))),sidemargin,sidedist,sidedist2,sdiff,rldata->mInverse);
                } else if (side <= 0.0) {
                    double rinc = OVERTAKE_OFFSET_INC * (rgtinc * MAX(0.2, MIN(1.1, sdiff/4)) * 1.5);
                    if (rldata->rInverse > 0.0)
                        rinc += rldata->rInverse * 200 * IncFactor;
                    myoffset -= (float) rinc;
                    //if (rldata->rInverse > 0.0)
                    //  myoffset -= rldata->rInverse * 500 * IncFactor;
                    avoidmovt = 1;
                    if (DebugMsg & debug_overtake)
                        LogUSR.debug("%s SIDE to Lft %s, MOVING RIGHT by %.3f sm-sd=%.2f mo=%.3f\n",car->_name,ocar->_name,(OVERTAKE_OFFSET_INC*lftinc*MAX(1.0f, MIN(2.0f, sdiff))),sdiff,myoffset);
                }

                if (avoidmovt)
                    sideratio = MIN(sidedist,sidedist2)/sidemargin;
                else if (DebugMsg & debug_overtake)
                    LogUSR.debug("%s SIDE %s, NO MOVE %.1f\n", car->_name, ocar->_name, myoffset);
            }
            else if (sidedist > sidemargin+3.0)
            {
                if ((car->_trkPos.toLeft > ocar->_trkPos.toLeft && rldata->rInverse > 0.0) ||
                        (car->_trkPos.toLeft < ocar->_trkPos.toLeft && rldata->rInverse < 0.0))
                    avoidtime = MIN(simtime, avoidtime + MIN(deltaTime*0.9, fabs(rldata->rInverse*1.2)));

                if (ocar->_trkPos.toLeft > car->_trkPos.toLeft &&
                        car->_trkPos.toLeft < MIN(lane2left, 4.0 + fabs(nextCRinverse)*1000))
                {
                    myoffset -= (float) (OVERTAKE_OFFSET_INC*lftinc/4);
                    if (DebugMsg & debug_overtake)
                        LogUSR.debug("%s SIDE to Rgt %s, MOVING BACK TO RIGHT\n", car->_name, ocar->_name);
                    if (!avoidmode)
                        avoidtime = MIN(simtime, avoidtime+deltaTime*0.5);
                }
                else if (ocar->_trkPos.toLeft < car->_trkPos.toLeft &&
                         car->_trkPos.toRight < MIN(lane2right, 4.0 + fabs(nextCRinverse)*1000))
                {
                    myoffset += (float) (OVERTAKE_OFFSET_INC*rgtinc/4);
                    if (DebugMsg & debug_overtake)
                        LogUSR.debug("%s SIDE to Lft %s, MOVING BACK TO LEFT\n", car->_name, ocar->_name);
                    if (!avoidmode)
                        avoidtime = MIN(simtime, avoidtime+deltaTime*0.5);
                }
                else if (DebugMsg & debug_overtake)
                    LogUSR.debug("%s SIDE %s, NO MOVE %.1f\n", car->_name, ocar->_name, myoffset);
            }
            else if (DebugMsg & debug_overtake)
                LogUSR.debug("%s SIDE %s, NO MOVE AT ALL! %.1f\n", car->_name, ocar->_name, myoffset);

            if (ocar->_trkPos.toLeft > car->_trkPos.toLeft)
            {
                avoidrgtoffset = (float) MAX(avoidrgtoffset, ocar->_trkPos.toMiddle + (o->getWidth()+1.0f));
                avoidmode |= avoidright;
                //if (avoidmovt)
                avoidmode |= avoidside;
                if (closing)
                    avoidmode |= avoidsideclosing;
            }
            else
            {
                avoidlftoffset = (float) MIN(avoidlftoffset, ocar->_trkPos.toMiddle - (o->getWidth()+1.0f));
                avoidmode |= avoidleft;
                //if (avoidmovt)
                avoidmode |= avoidside;
                if (closing)
                    avoidmode |= avoidsideclosing;
            }

        }
    }

    if (avoidmode)
    {
        if (!avoidmovt)
            avoidtime = MIN(simtime, avoidtime+deltaTime*1.0);
        goto end_getoffset;
    }

#if 0
    // don't try and front-avoid if we're struggling for control!
    if (avgaccel_x - fabs(laststeer) * fabs(angle-speedangle) < -10.0)
        goto end_getoffset;

    if (rldata->lftmargin > 0.0 && minoffset < rldata->offset - rldata->lftmargin)
        minoffset = rldata->offset - rldata->lftmargin;
    if (rldata->rgtmargin > 0.0 && maxoffset > rldata->offset + rldata->rgtmargin)
        maxoffset = rldata->offset + rldata->rgtmargin;
#endif

    if (car->_speed_x > 20.0 || simtime > 10.0)
    {
        double caution = rldata->overtakecaution;

        int avoidingside = TR_STR;
        bool mustmove = false;

        {
            tTrackSeg *wseg = (rldata->rInverse > 0.0 ? car->_wheelSeg(FRNT_LFT) : car->_wheelSeg(FRNT_RGT));
            if (wseg->surface->kFriction > car->_trkPos.seg->surface->kFriction)
                caution += (wseg->surface->kFriction - car->_trkPos.seg->surface->kFriction) * 4;
        }

        //caution += fabs(speedangle - angle)*10;
        int otry_success = 0;
        double rInverse = rldata->rInverse;
        if (fabs(nextCRinverse) > fabs(rInverse))
            rInverse = nextCRinverse;

        for (int otry=0; otry<=1; otry++)
        {
            // Overtake.
            for (i = 0; i < opponents->getNOpponents(); i++)
            {
                tCarElt *ocar = opponent[i].getCarPtr();

                // strategy telling us to follow this car?
                if ((opponent[i].getState() & OPP_FRONT_FOLLOW))
                    continue;

                // off track or a long way wide of us?
                if (!(opponent[i].getState() & OPP_COLL) &&
                        fabs(ocar->_trkPos.toMiddle) > Width/2 + 3.0 &&
                        fabs(car->_trkPos.toMiddle-ocar->_trkPos.toMiddle) >= 8.0)
                    continue;

                if (ocar->_state & (RM_CAR_STATE_NO_SIMU & ~RM_CAR_STATE_PIT))
                    continue;

                if ((opponent[i].getState() & OPP_FRONT) &&
                        !(0 && opponent[i].isTeamMate() && car->race.laps <= opponent[i].getCarPtr()->race.laps))
                {
                    double mcd = mincatchdist;
                    if (canOvertake(&opponent[i], &mcd, false, (otry == 1)))
                    {
                        bool thismustmove = false;

                        o = &opponent[i];

                        tCarElt *ocar = o->getCarPtr();
                        avoidingside = (car->_trkPos.toLeft > ocar->_trkPos.toLeft ? TR_LFT : TR_RGT);

                        if (avoidingside != TR_STR)
                        {
                            int newside = checkSwitch(avoidingside, o, ocar);
                            if (DebugMsg & debug_overtake)
                                LogUSR.debug(" AVOIDING A %c\n", (avoidingside==TR_LFT?'L':'R'));
                            if (newside != avoidingside)
                            {
                                if (DebugMsg & debug_overtake)
                                    LogUSR.debug(" SWITCH 1 from %c to %c\n", (avoidingside==TR_LFT?'L':'R'),(newside==TR_LFT?'L':'R'));
                                avoidingside = newside;
                                thismustmove = true;
                            }
                        }

                        if (!canOvertake2(o, avoidingside))
                        {
                            o = NULL;
                        }
                        else
                        {
                            if (!thismustmove && (o->getState() & OPP_COLL))
                                thismustmove = true;
                            mincatchdist = mcd;
                            mustmove = thismustmove;
                        }
                    }
                }
            }

            if (o || mode != mode_avoiding) break;
        }

        if (o != NULL)
        {
            tCarElt *ocar = o->getCarPtr();
            double sidedist = fabs(ocar->_trkPos.toLeft - car->_trkPos.toLeft);

            // work out what offset to steer for
            {
                if (avoidingside == TR_LFT)
                {
                    sidedist -= (speedangle - o->getSpeedAngle()) * 20;
                    if (mustmove ||
                            sidedist < car->_dimension_y + ocar->_dimension_y + 2.0 ||
                            (o->getState() & OPP_COLL) ||
                            (prefer_side == TR_RGT && car->_trkPos.toRight > MIN(lane2right, 3.0 - nextCRinverse*1000)))
                    {
                        double rinc = OVERTAKE_OFFSET_INC * rgtinc;
                        if (rInverse > 0.0)
                            rinc += rInverse * 200 * IncFactor;
                        myoffset -= (float) rinc;
                        if (DebugMsg & debug_overtake)
                            LogUSR.debug("%s LFT %s, MOVING RIGHT %.3f/%.3f -> %.3f (rgtinc=%.2f (%.2f) rinc=%.2f)\n", car->_name, ocar->_name, (float) (OVERTAKE_OFFSET_INC*rgtinc),rinc,myoffset,rgtinc,lftinc,rinc);
                        avoidmovt = 1;
                    }
                    else if (sidedist > car->_dimension_y + ocar->_dimension_y + 4.0 &&
                             car->_trkPos.toRight < MIN(lane2right-1.0, 4.0 + fabs(nextCRinverse)*1000))
                    {
                        if (DebugMsg & debug_overtake)
                            LogUSR.debug("%s LFT %s, MOVING BACK TO LEFT %.3f\n",car->_name,ocar->_name,(float) (OVERTAKE_OFFSET_INC*lftinc/2));
                        myoffset += (float) (OVERTAKE_OFFSET_INC*lftinc)/2;
                        if (!avoidmode)
                            avoidtime = MIN(simtime, avoidtime+deltaTime);
                    }
                    else if (DebugMsg & debug_overtake)
                        LogUSR.debug("%s LFT %s, HOLDING LINE\n",car->_name,ocar->_name);
                }
                else // if (avoidingside == TR_RGT)
                {
                    sidedist -= (o->getSpeedAngle() - speedangle) * 20;
                    if (mustmove ||
                            sidedist < car->_dimension_y + ocar->_dimension_y + 2.0 ||
                            (o->getState() & OPP_COLL) ||
                            (prefer_side == TR_LFT && car->_trkPos.toLeft > MIN(lane2left, 3.0 + nextCRinverse*1000)))
                    {
                        if (DebugMsg & debug_overtake)
                            LogUSR.debug("%s RGT %s, MOVING LEFT %.3f\n",car->_name,ocar->_name,(float) (OVERTAKE_OFFSET_INC*lftinc));
                        double linc = OVERTAKE_OFFSET_INC * lftinc;
                        if (rInverse < 0.0)
                            linc -= rInverse * 200 * IncFactor;
                        myoffset += (float) linc;
                        avoidmovt = 1;
                    }
                    else if (sidedist > car->_dimension_y + ocar->_dimension_y + 4.0 &&
                             car->_trkPos.toLeft < MIN(lane2left-1.0, 4.0 + fabs(nextCRinverse)*1000))
                    {
                        if (DebugMsg & debug_overtake)
                            LogUSR.debug("%s RGT %s, MOVING BACK TO RIGHT %.3f\n", car->_name, ocar->_name,(float) (OVERTAKE_OFFSET_INC*rgtinc/2));
                        myoffset -= (float) (OVERTAKE_OFFSET_INC*rgtinc)/2;
                        if (!avoidmode)
                            avoidtime = MIN(simtime, avoidtime+deltaTime);
                    }
                    else if (DebugMsg & debug_overtake)
                        LogUSR.debug("%s RGT %s, HOLDING LINE\n", car->_name, ocar->_name);
                }

                {
                    //if (ocar->_trkPos.toLeft > car->_trkPos.toLeft)
                    if (avoidingside == TR_RGT)
                    {
                        avoidrgtoffset = (float) MAX(avoidrgtoffset, ocar->_trkPos.toMiddle + (o->getWidth()+1.0f));
                        avoidmode |= avoidright;
                    }
                    else
                    {
                        avoidlftoffset = (float) MIN(avoidlftoffset, ocar->_trkPos.toMiddle - (o->getWidth()+1.0f));
                        avoidmode |= avoidleft;
                    }

                    if ((avoidingside == TR_LFT && rInverse > 0.0) ||
                            (avoidingside == TR_RGT && rInverse < 0.0))
                        avoidtime = MIN(simtime, avoidtime + MIN(deltaTime*0.9, fabs(rInverse*1.2)));

                    myoffset = (float) (MAX(minoffset, MIN(maxoffset, myoffset)));

                    if (!otry_success)
                        frontavoidtime = simtime;
                }
            }
        }

        if (!avoidmode)
        {
            o = NULL;

            // Let overlap or let less damaged team mate pass.
            for (i = 0; i < opponents->getNOpponents(); i++)
            {
                // Let the teammate with less damage overtake to use slipstreaming.
                // The position change happens when the damage difference is greater than
                // TEAM_DAMAGE_CHANGE_LEAD.
                if ((opponent[i].getState() & OPP_LETPASS))
                {
                    // Behind, larger distances are smaller ("more negative").
                    if (opponent[i].getDistance() > mindist) {
                        mindist = opponent[i].getDistance();
                        o = &opponent[i];
                    }
                }
            }

            if (o != NULL)
            {
                tCarElt *ocar = o->getCarPtr();
                float side = car->_trkPos.toMiddle - ocar->_trkPos.toMiddle;
                float w = car->_trkPos.seg->width/WIDTHDIV-BORDER_OVERTAKE_MARGIN;
                if (DebugMsg & debug_overtake)
                    LogUSR.debug("%s BEHIND %s (%d %d %d %d)\n", car->_name, ocar->_name,((o->getState() & OPP_LETPASS) && !o->isTeamMate()),(o->isTeamMate() && (car->_dammage - o->getDamage() > TEAM_DAMAGE_CHANGE_LEAD)),((o->getDistance() > -TEAM_REAR_DIST) && (o->getDistance() < -car->_dimension_x)),(car->race.laps == o->getCarPtr()->race.laps));
                if (side > 0.0f) {
                    if (myoffset < w) {
                        myoffset += (float) (OVERTAKE_OFFSET_INC*lftinc);
                        avoidmovt = 1;
                    }
                } else {
                    if (myoffset > -w) {
                        myoffset -= (float) (OVERTAKE_OFFSET_INC*rgtinc);
                        avoidmovt = 1;
                    }
                }

                avoidmode |= avoidback;

                if (ocar->_trkPos.toLeft > car->_trkPos.toLeft)
                {
                    avoidrgtoffset = (float) MAX(avoidrgtoffset, ocar->_trkPos.toMiddle + (o->getWidth()+1.0f));
                    avoidmode |= avoidright;
                }
                else
                {
                    avoidlftoffset = (float) MIN(avoidlftoffset, ocar->_trkPos.toMiddle - (o->getWidth()+1.0f));
                    avoidmode |= avoidleft;
                }

                myoffset = (float) (MAX(minoffset, MIN(maxoffset, myoffset)));
                if (DebugMsg & debug_overtake)
                    LogUSR.debug("%s BEHIND %s (%g)\n", car->_name, ocar->_name, myoffset);

            }
        }
    }

    if (mode == mode_avoiding && avoidmode == 0)
        setMode(mode_correcting);
    if (mode == mode_normal)
        myoffset = (float) moffset;

#if 1
    // no-one to avoid, work back towards raceline by correcting steering
    if (mode == mode_correcting && (simtime > 15.0 || car->_speed_x > 20) && simtime > CorrectDelay)
    {
        double factor = 0.25;//(fabs(car->_trkPos.toMiddle) < car->_trkPos.seg->width/2 + 2.0 ? 0.25 : 1.0);
        if (fabs(myoffset) > fabs(rldata->offset))
        {
            //double inc = OVERTAKE_OFFSET_INC * MIN(lftinc, rgtinc) * factor;
            if (myoffset < rldata->offset && myoffset < 2.0)
                myoffset += (float) (MIN(rldata->offset-myoffset, OVERTAKE_OFFSET_INC * rgtinc/5 * factor));
            else if (myoffset > rldata->offset && myoffset > 2.0)
                myoffset -= (float) (MIN(myoffset-rldata->offset, OVERTAKE_OFFSET_INC * lftinc/5 * factor));
        }

        maxoffset = MIN(moffset+OVERTAKE_OFFSET_INC*lftinc/2, Width/2 - (car->_dimension_y+SideMargin));
        minoffset = MAX(moffset-OVERTAKE_OFFSET_INC*rgtinc/2, -(Width/2 - (car->_dimension_y+SideMargin)));
    }
#endif

end_getoffset:
    if (avoidmode)
        setMode(mode_avoiding);

    if (mode == mode_avoiding && !avoidmovt)
        avoidtime = MIN(simtime, avoidtime+deltaTime*1.5);

    //  minoffset = MAX(minoffset, MIN(1.5, rldata->offset));
    //  maxoffset = MIN(maxoffset, MAX(track->width - 1.5, rldata->offset));
    {
        double mo = myoffset;
        myoffset = (float) (MAX(minoffset, MIN(maxoffset, myoffset)));
        if (DebugMsg & debug_overtake)
            if (mode != mode_normal)
            {
                LogUSR.debug("mode=%d max=%.1f(%.1f) min=%.1f(%.1f) myoff=%.1f->%.1f->%.1f\n",mode,maxoffset,oldmax,minoffset,oldmin,car->_trkPos.toMiddle,mo,myoffset);
            }
    }
    return myoffset;
}

int Driver::checkSwitch( int side, Opponent *o, tCarElt *ocar )
{
    double xdist = o->getDistance();
    double t_impact = MAX(0.0, MIN(10.0, o->getTimeImpact()));
    if (car->_speed_x - ocar->_speed_x < MIN(5.0, xdist*3))
        t_impact *= (1.0 + (5.0 - (car->_speed_x - ocar->_speed_x)));
    t_impact = MIN(3.0, MIN(t_impact, (5.0-(xdist-fabs(rldata->mInverse*1000)))/10));

    double mcatchleft = MAX(1.0, MIN(track->width-1.0, car->_trkPos.toLeft - speedangle * (t_impact * 10)));
    double ocatchleft = MAX(1.0, MIN(track->width-1.0, ocar->_trkPos.toLeft - o->getSpeedAngle() * (t_impact * 10)));
    double ydist = mcatchleft-ocatchleft;
    double sdiff = MAX(0.0, currentspeed - o->getSpeed());
    double radius = MIN(car->_dimension_y*3, fabs(nextCRinverse) * 200);
    double speedchange = 0.0;

    if (prefer_side == side && rldata->speedchange < 0.0 && ocar->_pos > car->_pos)
        speedchange = fabs(rldata->speedchange)*3;

#if 0
    if (fabs(mcatchleft - ocatchleft) < car->_dimension_y + 1.5 + radius &&
            (fabs(mcatchleft - ocatchleft) < fabs(car->_trkPos.toLeft - ocar->_trkPos.toLeft) ||
             (side == TR_LFT && ocatchleft > car->_trkPos.seg->width - car->_dimension_y + 1.5 + radius) ||
             (side == TR_RGT && ocatchleft < car->_dimension_y + 1.5 + radius)))
#endif
    {
        double switchrad = 1.0 + (side == prefer_side ? radius * 4 : 0);

        switch (side)
        {
        case TR_RGT:
            if (DebugMsg & debug_overtake)
                LogUSR.debug("CHECKSWITCH: Rgt - ti=%.2f dm=%.1f o=%.2f->%.2f m=%.2f->%.2f\n",t_impact,deltamult,ocar->_trkPos.toLeft,ocatchleft,car->_trkPos.toLeft,mcatchleft);
            if (nextCRinverse > 0.0)
                radius = 0.0;
            if ((side == prefer_side ||
                 ocatchleft < (car->_dimension_y + 3.0 + radius + speedchange) * switchrad ||
                 ocatchleft < (mcatchleft - 1.5) * switchrad) &&
                    xdist > sdiff + ydist + MAX(0.0, angle*10) &&
                    track->width - ocatchleft > (car->_dimension_y + 3.0 + radius + speedchange))
            {
                if (DebugMsg & debug_overtake)
                    LogUSR.debug("            Switch to his right (side=lft) - %d %d %d %d\n",(side==prefer_side),(ocatchleft<mcatchleft-1.5),(xdist>sdiff+ydist+MAX(0.0, angle*10)),(track->width-ocatchleft>(car->_dimension_y+3+radius+speedchange)));
                side = TR_LFT;
            }
            break;

        case TR_LFT:
        default:
            if (DebugMsg & debug_overtake)
                LogUSR.debug("CHECKSWITCH: Lft - ti=%.2f dm=%.1f o=%.2f->%.2f m=%.2f->%.2f\n",t_impact,deltamult,ocar->_trkPos.toLeft,ocatchleft,car->_trkPos.toLeft,mcatchleft);
            if (nextCRinverse < 0.0)
                radius = 0.0;
            if ((side == prefer_side ||
                 track->width-ocatchleft < (car->_dimension_y + 3.0 + radius + speedchange) * switchrad ||
                 ocatchleft > (mcatchleft + 1.5) * switchrad) &&
                    xdist > sdiff + (-ydist) + MAX(0.0, -angle*10) &&
                    ocatchleft > (car->_dimension_y + 3.0 + radius + speedchange))
            {
                side = TR_RGT;
            }
            break;
        }
    }

    return side;
}

// Update my private data every timestep.
void Driver::update(tSituation *s)
{
    // Update global car data (shared by all instances) just once per timestep.
    if (simtime != s->currentTime) {
        simtime = s->currentTime;
        cardata->update();
        mycardata->updateWalls();
    }
    else
    {
        // always update my car
        mycardata->update();
        mycardata->updateWalls();

        int nCars = cardata->getNCars();

        for (int i = 0; i < nCars; i++)
        {
            // update cars that are close to ours
            SingleCardata *cdata = cardata->getCarData(i);
            double mdist = car->_distFromStartLine;
            double odist = (double) cdata->getDistFromStart();

            if (odist > track->length - 30 && mdist < 30)
                mdist += track->length;
            else if (mdist > track->length - 30 && odist < 30)
                odist += track->length;

            double dist = fabs(mdist - odist);
            if (dist < 60.0)
                cdata->update();
        }
    }

    evalTrueSpeed();

    prefer_side = raceline->findNextCorner( &nextCRinverse );

    // Update the local data rest.
    avgaccel_x += (car->_accel_x - avgaccel_x)/2;
    prevspeedangle = speedangle;
    speedangle = (float) -(mycardata->getTrackangle() - atan2(car->_speed_Y, car->_speed_X));
    FLOAT_NORM_PI_PI(speedangle);
    mass = CARMASS + car->_fuel;
    currentspeedsqr = car->_speed_x*car->_speed_x;
    currentspeed = getSpeed();
    opponents->update(s, this, DebugMsg);
    strategy->update(car, s);

    if (car->_state <= RM_CAR_STATE_PIT && !NoPit)
    {
#ifdef SPEED_DREAMS
        float DLong, DLat;                             // Dist to Pit
        RtDistToPit(car,track,&DLong,&DLat);

        if (DLong > 500)
            pitStopChecked = false;

        if (!pit->getPitstop()
                && (!pitStopChecked)
                && (DLong < 500)
                && (car->_distFromStartLine < pit->getNPitEntry() || car->_distFromStartLine > pit->getNPitEnd()))
#else
        if (!pit->getPitstop() && (car->_distFromStartLine < pit->getNPitEntry() || car->_distFromStartLine > pit->getNPitEnd()))
#endif
        {
            bool pitstop = strategy->needPitstop(car, s, opponents);
            if (pitstop)
            {
                pit->setPitstop(pitstop);
                pit->needPitstop(pitstop);
            }
        }

        if (pit->getPitstop() && car->_pit)
        {
            pitpos = PIT_MID;

            for (int i=0; i<opponents->getNOpponents(); i++)
            {
                //int idx = opponent[i].getIndex();
                if (opponent[i].getTeam() != TEAM_FRIEND) continue;
                if (opponent[i].getCarPtr() == car) continue;
                if (opponent[i].getCarPtr()->_state > RM_CAR_STATE_PIT)
                    continue;

                int opitpos = (int) opponent[i].getCarPtr()->_lightCmd;

                if (opitpos != PIT_NONE && car->_fuel > fuelperlap*1.5 && car->_trkPos.toLeft >= 0.0 && car->_trkPos.toLeft <= track->width)
                {
                    // pit occupied & we've got enough fuel to go around again
                    pit->setPitstop( 0 );
                    pitpos = PIT_NONE;
                    break;
                }

                if (!opponent[i].getCarPtr()->_pit) // This sometimes happens (Ticket #90) !
                    break;

                if (opponent[i].getCarPtr()->_pit->pos.seg == car->_pit->pos.seg)
                {
                    // sharing a pit
                    if (opitpos == PIT_FRONT)
                    {
                        double pitloc = pit->getNPitLoc( PIT_MID );
                        double myfrompit = pitloc - car->_distFromStartLine;
                        double opfrompit = pitloc - opponent[i].getCarPtr()->_distFromStartLine;
                        if (myfrompit < 0.0) myfrompit += track->length;
                        if (opfrompit < 0.0) opfrompit += track->length;

                        // work out who's closest to the pit & therefore should go in front
                        if (opfrompit > myfrompit)
                        {
                            pitpos = PIT_FRONT;
                        }
                        else
                        {
                            pitpos = PIT_BACK; // go in behind other car
                        }
                    }
                    else
                    {
                        pitpos = PIT_FRONT; // stop at end of pit space to leave room
                    }
                }

                break;
            }
        }
        else if (!pit->getInPit())
            pitpos = PIT_NONE;
    }
    else
    {
        pitpos = PIT_NONE;
    }

#ifdef SPEED_DREAMS
    if (pitpos == PIT_NONE)
        RtTeamReleasePit(teamIndex);
#else
    car->_lightCmd = (char) pitpos;
#endif

    pit->update();
    alone = isAlone();
    simtime = s->currentTime;

    float trackangle = RtTrackSideTgAngleL(&(car->_trkPos));
    angle = trackangle - car->_yaw;
    FLOAT_NORM_PI_PI(angle);
    angle = -angle;
}


int Driver::isAlone()
{
    int i;

    for (i = 0; i < opponents->getNOpponents(); i++) {
#if 0
        if (opponent[i].getTeam() == TEAM_FRIEND)
            continue;
#endif

        // not a friend - if we're avoiding then we're obviously not alone
        if (mode == mode_avoiding)
            return 0;

        if ((opponent[i].getState() & (OPP_COLL | OPP_LETPASS)) ||
                ((opponent[i].getState() & (OPP_FRONT)) && opponent[i].getDistance() < MAX(50.0, car->_speed_x*1.5)) ||
                (fabs(opponent[i].getDistance()) < 50.0))
        {
            return 0; // Not alone.
        }
    }
    return 1; // Alone.
}


float Driver::stuckSteering( float steercmd )
{
    if (stucksteer > 0.0f)
        steercmd = (fabs(steercmd) + stucksteer) / 2;
    else
        steercmd = -(fabs(steercmd) + fabs(stucksteer)) / 2;
    return steercmd;
}

float Driver::GetSafeStuckAccel()
{
    // see if wheels are on a bumpy surface
    tTrackSeg *rgt_seg = car->_trkPos.seg;
    tTrackSeg *lft_seg = car->_trkPos.seg;
    double max_rough = 0.0f;
    float accel = MAX(0.5f, 1.0f - fabs(angle)/3);
    int rgt_off = 0, lft_off = 0;

    if ((car->priv.wheel[FRNT_RGT].seg != car->_trkPos.seg && car->priv.wheel[FRNT_RGT].seg->style == TR_PLAN &&
         car->priv.wheel[REAR_RGT].seg != car->_trkPos.seg && car->priv.wheel[FRNT_RGT].seg->style == TR_PLAN))
    {
        rgt_seg = car->priv.wheel[REAR_RGT].seg;
        if (rgt_seg->style == TR_PLAN &&
                (rgt_seg->surface->kFriction < car->_trkPos.seg->surface->kFriction*0.7 ||
                 rgt_seg->surface->kRoughness > MAX(0.03, car->_trkPos.seg->surface->kRoughness*1.3) ||
                 rgt_seg->surface->kRollRes > MAX(0.005, car->_trkPos.seg->surface->kRollRes*1.5)))
        {
            rgt_off = 1;
            if (car->_trkPos.toRight < car->_dimension_y - 1.5)
                rgt_off = 2;
            max_rough = MAX(max_rough, rgt_seg->surface->kRoughness);
        }
    }

    if ((car->priv.wheel[FRNT_LFT].seg != car->_trkPos.seg && car->priv.wheel[FRNT_LFT].seg->style == TR_PLAN &&
         car->priv.wheel[REAR_LFT].seg != car->_trkPos.seg && car->priv.wheel[REAR_LFT].seg->style == TR_PLAN))
    {
        lft_seg = car->priv.wheel[REAR_LFT].seg;
        if (lft_seg->style == TR_PLAN &&
                (lft_seg->surface->kFriction < car->_trkPos.seg->surface->kFriction*0.7 ||
                 lft_seg->surface->kRoughness > MAX(0.03, car->_trkPos.seg->surface->kRoughness*1.3) ||
                 lft_seg->surface->kRollRes > MAX(0.005, car->_trkPos.seg->surface->kRollRes*1.5)))
        {
            lft_off = 1;
            if (car->_trkPos.toRight < car->_dimension_y - 1.5)
                lft_off = 2;
            max_rough = MAX(max_rough, lft_seg->surface->kRoughness);
        }
    }

    if (lft_off + rgt_off > 0 && (car->_speed_x + fabs(car->_yaw_rate*5) > 3.0))
    {
        // at least one wheel on the bumpy stuff and we're moving
        accel = (float)MAX(0.2f, MIN(accel, (0.8f - (1.0+fabs(car->_yaw_rate)) * (max_rough*20))));
    }
    else if (car->_speed_x > 5.0 && fabs(car->_steerCmd) > fabs(car->_yaw_rate))
    {
        // understeering, reduce accel
        accel = MAX(0.3f, accel - (fabs(car->_steerCmd) - fabs(car->_yaw_rate)));
    }

    return accel;
}

// Check if I'm stuck.
bool Driver::isStuck()
{
    double toSide = MIN(car->_trkPos.toLeft, car->_trkPos.toRight);
    double stangle = angle;
    double fangle = fabs(angle);

    vec2f target = getTargetPoint(false, 0.0);
    double targetAngle = atan2(target.y - car->_pos_Y, target.x - car->_pos_X);
    double stuck_steer = calcSteer( targetAngle, 0 );

    if (stangle < 1.0 && stangle > -0.3 && car->_trkPos.toLeft < 1.0)
        stangle += MIN(0.5, car->_trkPos.toLeft / 5);
    else if (stangle > -1.0 && stangle < 0.3 && car->_trkPos.toRight < 1.0)
        stangle -= MIN(0.5, car->_trkPos.toRight / 5);

    bool returning = ((car->_trkPos.toMiddle > 0.0 && speedangle < -0.2) ||
                      (car->_trkPos.toMiddle < 0.0 && speedangle > 0.2));
    bool departing = ((car->_trkPos.toMiddle > 0.0 && speedangle > 0.2) ||
                      (car->_trkPos.toMiddle < 0.0 && speedangle < -0.2));
    double toWall = toSide;

    // how far to the nearest wall?
    if (car->_trkPos.toMiddle < 0.0 && car->_trkPos.seg->side[TR_SIDE_RGT] != NULL)
    {
        tTrackSeg *seg = car->_trkPos.seg->side[TR_SIDE_RGT];
        if (seg->style == TR_PLAN)
        {
            toWall += seg->width;
            if (seg->side[TR_SIDE_RGT])
            {
                seg = seg->side[TR_SIDE_RGT];
                if (seg->style == TR_PLAN)
                    toWall += seg->width;
            }
        }
    }
    else if (car->_trkPos.toMiddle > 0.0 && car->_trkPos.seg->side[TR_SIDE_LFT] != NULL)
    {
        tTrackSeg *seg = car->_trkPos.seg->side[TR_SIDE_LFT];
        if (seg->style == TR_PLAN)
        {
            toWall += seg->width;
            if (seg->side[TR_SIDE_LFT])
            {
                seg = seg->side[TR_SIDE_LFT];
                if (seg->style == TR_PLAN)
                    toWall += seg->width;
            }
        }
    }

    // update stopped timer
    if (fabs(car->_speed_x) > 5.0)
        stopped_timer = simtime;

    if (pit->getInPit())
    {
        stuck = 0;
        stuck_timer = (float)simtime;
        stucksteer = -100.0f;
    }

    if (stuck)
    {
        // stuck - ok to be "unstuck"?
        if (fangle < 0.7 &&
                toSide > 2.0 &&
                simtime - stuck_timer > 2.0)
        {
            stuck = 0;
            stuck_timer = (float)simtime;
            stucksteer = -100.0f;
            return false;
        }

        setMode( mode_correcting );

        // still stuck - see if we should change gear.
        if (stuck == STUCK_REVERSE)
        {
            // should we give up and go forwards?
            if ((simtime - stuck_timer > (1.0 + fangle) && (fabs(car->_speed_x) < 2.0 || !returning)) ||
                    (car->_trkPos.toMiddle > 0.0 && angle < 0.4 && angle > -2.4) ||
                    (car->_trkPos.toMiddle < 0.0 && angle > -0.4 && angle < 2.4))
            {
                stuck = STUCK_FORWARD;
                stuck_timer = (float)simtime;
            }
        }
        else if (stuck == STUCK_FORWARD)
        {
            // should we try reverse?
            if ((simtime - stuck_timer > MAX(4.0, car->_speed_x/2) &&
                 (fabs(car->_speed_x) < 4.0 || (!returning && fabs(car->_yaw_rate) < 0.4))) ||
                    (car->_trkPos.toRight < 0.0 + fangle && angle < -0.6 && angle > -2.4) ||
                    (car->_trkPos.toLeft < 0.0 + fangle && angle > 0.6 && angle < 2.4))
            {
                stuck = STUCK_REVERSE;
                stuck_timer = (float)simtime;
            }
        }

        last_stuck_time = (float)simtime;
    }
    else if (getSpeed() < 10.0 || toSide < 2.0)
    {
        // not stuck - but are we?
        if (simtime - last_stuck_time > 3.0 &&   // only get stuck if 3 seconds since we were last stuck
                (fangle > MAX(1.0, 1.0 + toSide/5) ||
                 simtime - stopped_timer > 4.0 ||
                 (car->_trkPos.toLeft < 1.0 && car->_trkPos.toMiddle > rldata->offset + 2 && angle > 0.7) ||
                 (car->_trkPos.toRight < 1.0 && car->_trkPos.toMiddle < rldata->offset - 2 && angle < -0.7)))
        {
            // yes, we're stuck
            stuck_timer = last_stuck_time = (float)simtime;
            setMode( mode_correcting );

            stuck = STUCK_REVERSE;

            if ((fangle < 2.0 && currentspeed > 10.0) ||
                    (angle < 0.0 && car->_trkPos.toMiddle > 0.0) ||
                    (angle > 0.0 && car->_trkPos.toMiddle < 0.0) ||
                    (fangle < 1.8 && toWall > 4.0 + fangle*3) ||
                    (fabs(angle - speedangle) * 1.2 && car->_speed_x > 2.0))
            {
                stuck = STUCK_FORWARD;
            }
        }
        else
        {
            // free to keep driving
            stucksteer = -100.0f;
            stuck = 0;
            return false;
        }
    }
    else
    {
        // free to keep driving
        stucksteer = -100.0f;
        stuck = 0;
        return false;
    }

    // seeing we're stuck, determine steering, braking, accel
    if (fangle > 1.7)
    {
        stuck_steer = -stuck_steer;
        if (stuck_steer > 0.0)
            stuck_steer = 1.0;
        else
            stuck_steer = -1.0;
    }
    else if (car->_speed_x > 5.0 && fangle < 0.6 && stuck == STUCK_FORWARD &&
             ((car->_trkPos.toLeft < 2.0 && racesteer < stuck_steer) ||
              (car->_trkPos.toRight < 2.0 && racesteer > stuck_steer)))
    {
        stuck_steer += MAX(-0.15, MIN(0.15, racesteer - stuck_steer));
    }

    if (stucksteer < -99.0f)
    {
        stucksteer = (float)stuck_steer;
    }
    else
    {
        if (stuck == STUCK_FORWARD &&
                ((stucksteer > 0.0 && stuck_steer < 0.0) || (stucksteer < 0.0 && stuck_steer > 0.0)) &&
                fabs(angle) < 1.6)
            stucksteer = (float)stuck_steer;
        else if (stucksteer > 0.0)
            stucksteer = (float)fabs(stuck_steer);
        else
            stucksteer = (float)-fabs(stuck_steer);
    }

    if (stuck == STUCK_REVERSE)
    {
        car->_steerCmd = (float)(-stucksteer*1.4);

        if (car->_speed_x > 3.0 || (toSide < 0.0 && departing))
        {
            car->_accelCmd = 0.0f;
            car->_brakeCmd = 0.4f;
            car->_clutchCmd = 1.0f;
        }
        else
        {
            car->_accelCmd = MAX(0.3f, 0.7f + MIN(0.0f, car->_speed_x/40));
            car->_brakeCmd = 0.0f;
            car->_clutchCmd = 0.0f;
        }

        car->_gearCmd = -1;
    }
    else
    {
        car->_steerCmd = stucksteer;

        if (car->_speed_x < -3.0)
        {
            car->_accelCmd = 0.0f;
            car->_brakeCmd = 0.4f;
            car->_clutchCmd = 1.0f;
        }
        else
        {
            car->_brakeCmd = 0.0f;
            car->_accelCmd = GetSafeStuckAccel();
            car->_accelCmd = MAX(car->_accelCmd/3, car->_accelCmd - fabs(stucksteer/2));
            if (car->_speed_x < 2.0 || fabs(car->_yaw_rate) < 0.5)
                car->_accelCmd = (float)MAX(0.3, car->_accelCmd);
            car->_clutchCmd = 0.0f;
        }

        car->_gearCmd = 1;
    }

    return true;
}


// Compute aerodynamic downforce coefficient CA.
void Driver::initWheelPos()
{
    for (int i=0; i<4; i++)
    {
        char const *WheelSect[4] = {SECT_FRNTRGTWHEEL, SECT_FRNTLFTWHEEL, SECT_REARRGTWHEEL, SECT_REARLFTWHEEL};
        float rh = 0.0;
        rh = GfParmGetNum(car->_carHandle,WheelSect[i],PRM_RIDEHEIGHT,(char *)NULL, 0.10f);
        wheelz[i] = (-rh / 1.0 + car->info.wheel[i].wheelRadius) - 0.01;
    }
}

void Driver::initCa()
{
    char const *WheelSect[4] = {SECT_FRNTRGTWHEEL, SECT_FRNTLFTWHEEL, SECT_REARRGTWHEEL, SECT_REARLFTWHEEL};
    float rearwingarea = GfParmGetNum(car->_carHandle, SECT_REARWING, PRM_WINGAREA, (char*) NULL, 0.0f);
    float rearwingangle = GfParmGetNum(car->_carHandle, SECT_REARWING, PRM_WINGANGLE, (char*) NULL, 0.0f);
    float wingca = 1.23f*rearwingarea*sin(rearwingangle);

    float cl = GfParmGetNum(car->_carHandle, SECT_AERODYNAMICS, PRM_FCL, (char*) NULL, 0.0f) +
            GfParmGetNum(car->_carHandle, SECT_AERODYNAMICS, PRM_RCL, (char*) NULL, 0.0f);
    float h = 0.0f;
    int i;
    for (i = 0; i < 4; i++)
        h += GfParmGetNum(car->_carHandle, WheelSect[i], PRM_RIDEHEIGHT, (char*) NULL, 0.20f);
    h*= 1.5f; h = h*h; h = h*h; h = 2.0f * exp(-3.0f*h);
    CA = h*cl + 4.0f*wingca;
}


// Compute aerodynamic drag coefficient CW.
void Driver::initCw()
{
    float cx = GfParmGetNum(car->_carHandle, SECT_AERODYNAMICS, PRM_CX, (char*) NULL, 0.0f);
    float frontarea = GfParmGetNum(car->_carHandle, SECT_AERODYNAMICS, PRM_FRNTAREA, (char*) NULL, 0.0f);
    CW = 0.645f*cx*frontarea;
}


// Init the friction coefficient of the the tires.
void Driver::initTireMu()
{
    char const *WheelSect[4] = {SECT_FRNTRGTWHEEL, SECT_FRNTLFTWHEEL, SECT_REARRGTWHEEL, SECT_REARLFTWHEEL};
    float tm = FLT_MAX;
    int i;

    for (i = 0; i < 4; i++) {
        tm = MIN(tm, GfParmGetNum(car->_carHandle, WheelSect[i], PRM_MU, (char*) NULL, 1.0f));
    }
    TIREMU = tm;
}

void Driver::GetSteerPoint( double lookahead, vec2f *rt, double offset, double time )
{
    if (offset < -90.0 && mode != mode_normal)
        offset = myoffset;

    raceline->GetSteerPoint( lookahead, rt, offset, time );
}

// Reduces the brake value such that it fits the speed (more downforce -> more braking).
float Driver::filterBrakeSpeed(float brake)
{
    if (CA < 0.01)
        return brake;

    float weight = (CARMASS + car->_fuel)*G;
    float maxForce = weight + CA*MAX_SPEED*MAX_SPEED;
    float force = weight + CA*currentspeedsqr;
    return brake*force/maxForce;
}


// Brake filter for pit stop.
float Driver::filterBPit(float brake)
{
    if (pit->getPitstop() && !pit->getInPit()) {
        float dl, dw;
        RtDistToPit(car, track, &dl, &dw);
        if (dl < PIT_BRAKE_AHEAD)
        {
            float mu = car->_trkPos.seg->surface->kFriction*TIREMU*PIT_MU;
            if (brakedist(0.0f, mu) > dl) {
                return 1.0f;
            }
        }
    }

    if (pit->getInPit()) {
        float s = pit->toSplineCoord(car->_distFromStartLine);
        // Pit entry.
        if (pit->getPitstop()) {
            float mu = car->_trkPos.seg->surface->kFriction*TIREMU*PIT_MU;
            if (s < pit->getNPitStart()) {
                // Brake to pit speed limit.
                float dist = pit->getNPitStart() - s;
                if (brakedist(pit->getSpeedlimit(), mu) > dist) {
                    return 1.0f;
                }
            } else {
                // Hold speed limit.
                if (currentspeedsqr > pit->getSpeedlimitSqr()) {
                    return pit->getSpeedLimitBrake(currentspeedsqr);
                }
            }
            // Brake into pit (speed limit 0.0 to stop)
            float dist = pit->getNPitLoc(pitpos) - s;
            if (pitpos != PIT_BACK && pit->isTimeout(dist)) {
                pit->setPitstop(false);
                return 0.0f;
            } else {
                //        if (brakedist(0.0f, mu) > dist) {
                if (brakedist(0.0f, 0.5f*mu) > dist) {
                    return 2.0f;
                } else if (s > pit->getNPitLoc(pitpos)) {
                    // Stop in the pit.
                    return 2.0f;
                }
            }
        } else {
            // Pit exit.
            if (s < pit->getNPitEnd()) {
                // Pit speed limit.
                if (currentspeedsqr > pit->getSpeedlimitSqr()) {
                    return pit->getSpeedLimitBrake(currentspeedsqr);
                }
            }
        }
    }

    return brake;
}


// Brake filter for collision avoidance.
float Driver::filterBColl(float brake)
{
    collision = 0.0f;

    if (simtime < 1.5)
        return brake;

    float mu = car->_trkPos.seg->surface->kFriction * BrakeMu;
    int i;
    float thisbrake = 0.0f;
    for (i = 0; i < opponents->getNOpponents(); i++)
    {
        if ((opponent[i].getState() & OPP_COLL))
        {
            float accel = 0.0f;//opponent[i].getCarPtr()->_accel_x / MAX(1.0, opponent[i].getTimeImpact()*2);
            float ospeed = opponent[i].getSpeed() + accel;
            float margin = MIN(0.3f, MAX(0.0f, 0.3f - opponent[i].getDistance()));
            if ((opponent[i].getState() & OPP_SIDE_COLL) ||
                    brakedist(ospeed, mu) + MIN(1.0, margin + MAX(0.0, (getSpeed()-ospeed)/9)) > opponent[i].getDistance() + accel)
            {
                accelcmd = 0.0f;
                float thiscollision = MAX(0.01f, MIN(5.0f, opponent[i].getTimeImpact()));
                //thiscollision = MAX(0.01f, MIN(thiscollision, opponent[i].getDistance()/2));
                if (collision)
                    collision = MIN(collision, thiscollision);
                else
                    collision = thiscollision;
                thisbrake = (float)MAX(thisbrake, (0.3f + (5.0 - collision)/4) * brakeratio);
                if (DebugMsg & debug_brake)
                    fprintf(stderr,"%s - %s BRAKE: ti=%.3f\n",car->_name,opponent[i].getCarPtr()->_name,opponent[i].getTimeImpact());
            }
        }
    }
    return MAX(thisbrake, brake);
}


// Antilocking filter for brakes.
float Driver::filterABS(float brake)
{
    if (car->_speed_x < ABS_MINSPEED) return brake;

    float absrange = (collision > 0.0 ? AbsRange * 0.7f : AbsRange);
    float brake1 = brake, brake2 = brake;

    double skidAng = atan2(car->_speed_Y, car->_speed_X) - car->_yaw;
    NORM_PI_PI(skidAng);

    if (fabs(skidAng) > 0.2)
        brake1 = (float) MIN(brake1, 0.1f + 0.7f * cos(skidAng));

    float slip = 0.0f;
    for (int i=0; i<4; i++)
        slip = MAX(slip, car->_speed_x - (car->_wheelSpinVel(i) * car->_wheelRadius(i)));

    if (slip > AbsSlip)
        brake2 = (float) MAX(MIN(0.35f, brake), brake - MIN(brake*0.8f, (slip - AbsSlip) / absrange));

    brake = MIN(brake, MIN(brake1, brake2));
#if 0
    float origbrake = brake;
    //float rearskid = MAX(0.0f, MAX(car->_skid[2], car->_skid[3]) - MAX(car->_skid[0], car->_skid[1]));
    int i;
    float slip = 0.0f;
    for (i = 0; i < 4; i++) {
        slip += car->_wheelSpinVel(i) * car->_wheelRadius(i);
    }
    //slip *= 1.0f + MAX(rearskid, MAX(fabs(car->_yaw_rate)/10, fabs(angle)/8));
    slip = car->_speed_x - slip/4.0f;
    //if (collision)
    //  slip *= 0.25f;
    if (origbrake == 2.0f)
        slip *= 0.1f;

    float absslip = (car->_speed_x < 20.0f ? MIN(AbsSlip, AbsRange/2) : AbsSlip);
    if (slip > absslip) {
        brake = brake - MIN(brake, (slip - absslip)/AbsRange);
    }
    brake = MAX(brake, MIN(origbrake, 0.1f));

    //brake = MAX(MIN(origbrake, collision ? 0.15f :0.05f), brake - MAX(fabs(angle), fabs(car->_yaw_rate) / 2));
    brake = (float) (MAX(MIN(origbrake, (collision ? MAX(0.05f, (5.0-collision)/30) : 0.05f)), brake - fabs(angle-speedangle)*0.3));

    if (fbrakecmd)
        brake = MAX(brake, fbrakecmd);
#endif

    return brake;
}


// TCL filter for accelerator pedal.
float Driver::filterTCL(float accel)
{
    if (simtime < 0.7)
        return accel;

    accel = MIN(1.0f, accel);

    float slip = (this->*GET_DRIVEN_WHEEL_SPEED)() - fabs(car->_speed_x);
    if (slip > TclSlip)
        accel = accel - (float) MIN(accel*0.9f, (slip - TclSlip)/TclRange);
#if 0
    float accel1 = accel, accel2 = accel, accel3 = accel, accel4 = accel, accel5 = accel;

    if (car->_speed_x > 10.0f && !pit->getInPit())
    {
        tTrackSeg *seg = car->_trkPos.seg;
        tTrackSeg *wseg0 = car->_wheelSeg(REAR_RGT);
        tTrackSeg *wseg1 = car->_wheelSeg(REAR_LFT);
        int count = 0;

        if (wseg0->surface->kRoughness > MAX(0.02, seg->surface->kRoughness*1.2))
        {
            accel1 = (float) MAX(0.1f, accel1 - (wseg0->surface->kRoughness-(seg->surface->kRoughness*2.2)) * (wseg0->style == TR_CURB ? 2 : 10));
            if (fabs(car->_steerCmd) > 0.3f)
            {
                if (car->_steerCmd < 0.0)
                    car->_steerCmd = (float)MIN(car->_steerCmd*0.3, car->_steerCmd + (wseg0->surface->kRoughness-(seg->surface->kRoughness*2.2)) * (wseg0->style == TR_CURB ? 2 : 10));
                else
                    car->_steerCmd = (float)MAX(car->_steerCmd*0.3, car->_steerCmd + (wseg0->surface->kRoughness-(seg->surface->kRoughness*2.2)) * (wseg0->style == TR_CURB ? 2 : 10));
            }
        }
        if (wseg1->surface->kRoughness > MAX(0.02, seg->surface->kRoughness*1.2))
        {
            accel1 = (float) MAX(0.1f, accel1 - (wseg1->surface->kRoughness-(seg->surface->kRoughness*2.2)) * (wseg1->style == TR_CURB ? 2 : 10));
            if (fabs(car->_steerCmd) > 0.3f)
            {
                if (car->_steerCmd < 0.0)
                    car->_steerCmd = (float)MIN(car->_steerCmd*0.3, car->_steerCmd - (wseg0->surface->kRoughness-(seg->surface->kRoughness*2.2)) * (wseg0->style == TR_CURB ? 2 : 10));
                else
                    car->_steerCmd = (float)MAX(car->_steerCmd*0.3, car->_steerCmd - (wseg0->surface->kRoughness-(seg->surface->kRoughness*2.2)) * (wseg0->style == TR_CURB ? 2 : 10));
            }
        }

#if 0
        if (wseg0->surface->kFriction < seg->surface->kFriction)
            accel1 = (float) MAX(0.0f, accel1 - (seg->surface->kFriction - wseg0->surface->kFriction));
        if (wseg1->surface->kFriction < seg->surface->kFriction)
            accel1 = (float) MAX(0.0f, accel1 - (seg->surface->kFriction - wseg1->surface->kFriction));
#endif

        if (wseg0->surface->kRollRes > MAX(0.01, seg->surface->kRollRes*1.2))
            accel1 = (float) MAX(0.0f, accel1 - (wseg0->surface->kRollRes - seg->surface->kRollRes*1.2)*4);
        if (wseg1->surface->kRollRes > MAX(0.01, seg->surface->kRollRes*1.2))
            accel1 = (float) MAX(0.0f, accel1 - (wseg1->surface->kRollRes - seg->surface->kRollRes*1.2)*4);

        if (count)
        {
            if (mode != mode_normal &&
                    ((seg->type == TR_RGT && seg->radius <= 200.0f && car->_trkPos.toLeft < 3.0f) ||
                     (seg->type == TR_LFT && seg->radius <= 200.0f && car->_trkPos.toRight < 3.0f)))
                count++;
            accel1 = (float) MAX(0.0f, MIN(accel1, (1.0f-(0.25f*count)) - MAX(0.0f, (getSpeed()-car->_speed_x)/10.0f)));
        }

        if (fabs(angle) > 1.0)
            accel1 = (float) MIN(accel1, 1.0f - (fabs(angle)-1.0)*1.3);
    }

    double turndecel = TurnDecel + rldata->decel;

#ifdef CONTROL_SKILL
    turndecel += decel_adjust_perc;
#endif

    turndecel *= (mode == mode_normal ? 1 : 2);
    if (fabs(car->_steerCmd) > 0.02 && turndecel > 0.0)
    {
        float decel = (float) ((fabs(car->_steerCmd)-0.02f) * (1.0f+fabs(car->_steerCmd)) * 0.7f);
        decel *= (float) (turndecel);
        if (mode != mode_normal)
        {
            decel *= (float)(1.0 + rldata->adecel);
        }
        accel2 = (float) MIN(accel2, MAX(accel2*0.3, 1.0f-decel));
    }

    float slip = (this->*GET_DRIVEN_WHEEL_SPEED)() - fabs(car->_speed_x);
    if (slip > TclSlip) {
        accel3 = accel3 - MIN(accel3, (slip - TclSlip)/TclRange);
    }

    double yra = GetModD( tYawRateAccel, rldata->nextdiv );
    if (yra <= 0.0)
        yra = YawRateAccel;

    accel4 = (float)MAX(0.0, accel4 - fabs(car->_yaw_rate - car->_steerCmd) * yra);

    accel = MAX(accel/4, MIN(accel1, MIN(accel2, MIN(accel4, accel3))));

    if (mode == mode_normal)
    {
        accel5 = (float)MAX(0.0, accel5 - rldata->accel_redux);
        accel = MIN(accel, accel5);
    }

    if (accel > 0.9f)
        accel = 1.0f;

    if (faccelcmd > 0.0f)
        accel = MIN(accel, faccelcmd * 1.2f);
#endif

    return accel;
}


// Traction Control (TCL) setup.
void Driver::initTCLfilter()
{
    char *traintype = (char *) GfParmGetStr(car->_carHandle, SECT_DRIVETRAIN, PRM_TYPE, VAL_TRANS_RWD);
    if (strcmp(traintype, VAL_TRANS_RWD) == 0)
    {
        GET_DRIVEN_WHEEL_SPEED = &Driver::filterTCL_RWD;
    }
    else if (strcmp(traintype, VAL_TRANS_FWD) == 0)
    {
        GET_DRIVEN_WHEEL_SPEED = &Driver::filterTCL_FWD;
    }
    else if (strcmp(traintype, VAL_TRANS_4WD) == 0)
    {
        GET_DRIVEN_WHEEL_SPEED = &Driver::filterTCL_4WD;
    }
}


// TCL filter plugin for rear wheel driven cars.
float Driver::filterTCL_RWD()
{
    float friction = MIN(car->_wheelSeg(REAR_RGT)->surface->kFriction, car->_wheelSeg(REAR_LFT)->surface->kFriction) - 0.2f;
    if (friction < 1.0) friction *= MAX(0.6f, friction);
    bool  steer_correct = (fabs(car->_yaw_rate) > fabs(car->_steerCmd) ||
                           (car->_yaw_rate < 0.0 && car->_steerCmd > 0.0) ||
                           (car->_yaw_rate > 0.0 && car->_steerCmd < 0.0));
    float steer_diff    = fabs(car->_yaw_rate - car->_steerCmd);

    return (float) (((car->_wheelSpinVel(REAR_RGT) + car->_wheelSpinVel(REAR_LFT)) - (20 * friction)) *
                    car->_wheelRadius(REAR_LFT) +
                    (steer_correct ? (steer_diff * fabs(car->_yaw_rate) * (8 / friction)) : 0.0) +
                    MAX(0.0, (-(car->_wheelSlipAccel(REAR_RGT)) - friction)) +
                    MAX(0.0, (-(car->_wheelSlipAccel(REAR_LFT)) - friction)) +
                    fabs(car->_wheelSlipSide(REAR_RGT) * MAX(4, 80-fabs(car->_speed_x))/(8 * friction)) +
                    fabs(car->_wheelSlipSide(REAR_LFT) * MAX(4, 80-fabs(car->_speed_x))/(8 * friction)))
            / 2.0f;
}


// TCL filter plugin for front wheel driven cars.
float Driver::filterTCL_FWD()
{
    return (car->_wheelSpinVel(FRNT_RGT) + car->_wheelSpinVel(FRNT_LFT)) *
            car->_wheelRadius(FRNT_LFT) / 2.0f;
}


// TCL filter plugin for all wheel driven cars.
float Driver::filterTCL_4WD()
{
    return ((car->_wheelSpinVel(FRNT_RGT) + car->_wheelSpinVel(FRNT_LFT)) *
            car->_wheelRadius(FRNT_LFT) +
            (car->_wheelSpinVel(REAR_RGT) + car->_wheelSpinVel(REAR_LFT)) *
            car->_wheelRadius(REAR_LFT)) / 4.0f;
}

// Hold car on the track.
float Driver::filterTrk(float accel)
{
    return accel;

#if 0
    tTrackSeg* seg = car->_trkPos.seg;

    if (car->_speed_x < MAX_UNSTUCK_SPEED ||    // Too slow.
            pit->getInPit() ||              // Pit stop.
            car->_trkPos.toMiddle*speedangle > 0.0f)  // Speedvector points to the inside of the turn.
    {
        return accel;
    }

    if (seg->type == TR_STR) {
        float tm = fabs(car->_trkPos.toMiddle);
        float w = (seg->width - car->_dimension_y)/2.0f ;
        if (tm > w) {
            return 0.0f;
        } else {
            return accel;
        }
    } else {
        float sign = (seg->type == TR_RGT) ? -1.0f : 1.0f;
        if (car->_trkPos.toMiddle*sign > 0.0f) {
            return accel;
        } else {
            float tm = fabs(car->_trkPos.toMiddle);
            float w = seg->width/WIDTHDIV;
            if (tm > w) {
                return 0.0f;
            } else {
                return accel;
            }
        }
    }
#endif
}


// Compute the needed distance to brake.
float Driver::brakedist(float allowedspeed, float mu)
{
    float c = mu*G;
    float d = (CA*mu + CW)/mass;
    float v1sqr = currentspeedsqr;
    float v2sqr = allowedspeed*allowedspeed;
    return (float)((-log((c + v2sqr*d)/(c + v1sqr*d))/(2.0f*d)) + 1.0);
}

//==========================================================================*
// Estimate weather
//--------------------------------------------------------------------------*
int Driver::getWeather()
{
    return (track->local.rain << 4) + track->local.water;
}

void Driver::Meteorology()
{
    // Detect Weather condition track
    tTrackSeg *Seg;
    tTrackSurface *Surf;
    float mRainIntensity = 0.0f;
    mRain = getWeather();

    Seg = track->seg;

    for ( int I = 0; I < track->nseg; I++)
    {
        Surf = Seg->surface;
        mRainIntensity = MAX(mRainIntensity, Surf->kFrictionDry / Surf->kFriction);
        Seg = Seg->next;
    }

    mRainIntensity -= 1;
    GfOut("#mRainIntensity USR: %g\n", mRainIntensity);

    if (mRainIntensity > 0)
    {
        //mRain = true;
        TclSlip = (float) MIN(TclSlip, 2.0);
    }
    else
        mRain = 0;

    GfOut("#Rain BIPBIP: %d\n", mRain);
}

double Driver::TyreConditionFront()
{
    return MIN(car->_tyreCondition(0), car->_tyreCondition(1));
}

double Driver::TyreConditionRear()
{
    return MIN(car->_tyreCondition(2), car->_tyreCondition(3));
}

double Driver::TyreTreadDepthFront()
{
    double Right = (car->_tyreTreadDepth(0) - car->_tyreCritTreadDepth(0));
    double Left = (car->_tyreTreadDepth(1) - car->_tyreCritTreadDepth(1));
    return 100 * MIN(Right, Left);
}

double Driver::TyreTreadDepthRear()
{
    double Right = (car->_tyreTreadDepth(2) - car->_tyreCritTreadDepth(2));
    double Left = (car->_tyreTreadDepth(3) - car->_tyreCritTreadDepth(3));
    return 100 * MIN(Right, Left);
}
