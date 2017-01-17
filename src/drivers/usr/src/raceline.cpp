/////////////////////////////////////////////////////////////////////////////
//
// raceline.cpp
//
// car driver for TORCS
// (c) Remi Coulom, March 2000
// (c) Andrew Sumner, 2005 through 2009
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
////////////////////////////////////////////////////////////////////////////

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tgf.h" 
#include "track.h" 
#include "car.h"
#include "raceman.h" 
#include "robot.h" 
#include "robottools.h"

#include "raceline.h"
#include "spline.h"

////////////////////////////////////////////////////////////////////////////
// Structures and Statics
////////////////////////////////////////////////////////////////////////////

static SRaceLine SRL[5] = { { 0 } };
static int SRLinit = 0;

////////////////////////////////////////////////////////////////////////////
// Parameters
////////////////////////////////////////////////////////////////////////////

//
// These parameters are for the computation of the path
//
//static const int Iterations = 100;     // Number of smoothing operations

static const double SecurityR = 100.0; // Security radius
//static double SideDistExt = 2.0; // Security distance wrt outside
//static double SideDistInt = 1.0; // Security distance wrt inside          

/////////////////////////////////////////////////////////////////////////////
// Some utility macros and functions
/////////////////////////////////////////////////////////////////////////////

static double Mag(double x, double y)
{
    return sqrt(x * x + y * y);
}

static double Min(double x1, double x2)
{
    if (x1 < x2)
        return x1;
    else
        return x2;
}

static double Max(double x1, double x2)
{
    if (x1 < x2)
        return x2;
    else
        return x1;
}

/////////////////////////////////////////////////////////////////////////////
// Initialization
/////////////////////////////////////////////////////////////////////////////
LRaceLine::LRaceLine() :
    MinCornerInverse(0.001),
    IncCornerInverse(1.000),
    IncCornerFactor(1.000),
    BaseCornerSpeed(0.0),
    BaseCornerSpeedX(1.0),
    DefaultCornerSpeedX(1.0),
    CornerSpeed(15.0),
    CornerSpeedX(0.0),
    CornerAccel(0.0),
    BrakeDelay(20.0),
    BrakeDelayX(1.0),
    BrakeMod(1.0),
    BrakePower(0.5),
    IntMargin(1.5),
    ExtMargin(2.0),
    AvoidSpeedAdjust(0.0),
    AvoidSpeedAdjustX(1.0),
    AvoidBrakeAdjust(0.0),
    AvoidBrakeAdjustX(1.0),
    CurveFactor(0.14),
    SecurityZ(0.0),
    MaxSteerTime(1.5),
    MinSteerTime(1.0),
    TargetSpeed(0.0),
    ATargetSpeed(0.0),
    SteerGain(1.0),
    SteerSkid(0.06),
    SkidAccel(0.00),
    DivLength(3.0),
    AccelCurveDampen(1.0),
    BrakeCurveDampen(1.0),
    AccelCurveLimit(1.0),
    BrakeCurveLimit(1.5),
    AccelExit(0.0),
    AvoidAccelExit(0.0),
    OvertakeCaution(0.0),
    SkidCorrection(1.0),
    SteerRIAcc(0.0),
    SteerRIAccC(2.0),
    BumpCaution(0.0),
    SlopeFactor(1.0),
    ExitBoost(0.0),
    ExitBoostX(1.0),
    AvoidExitBoost(0.0),
    AvoidExitBoostX(1.0),
    AvoidOffset(1.3),
    RaceLineDebug(false),
    CW(0.0),
    wheelbase(0.0),
    wheeltrack(0.0),
    k1999steer(0.0),
    laststeer(0.0),
    lastNksteer(0.0),
    lastNasteer(0.0),
    skill(0.0),
    lastyaw(0.0),
    maxfuel(0.0),
    deltaTime(0.02),
    avgerror(0.00),
    Divs(0),
    AccelCurveOffset(0),
    Iterations(100),
    SteerMod(0),
    SRLidx(0),
    OfftrackAllowed(1),
    tSpeed(NULL),
    tLaneShift(NULL),
    tRLMarginRgt(NULL),
    tRLMarginLft(NULL),
    tOTCaution(NULL),
    tRLSpeed(NULL),
    tRLBrake(NULL),
    tIntMargin(NULL),
    tExtMargin(NULL),
    tSecurity(NULL),
    tDecel(NULL),
    tADecel(NULL),
    tSpeedLimit(NULL),
    tCornerAccel(NULL),
    tAccelCurveDampen(NULL),
    tCurveFactor(NULL),
    tAvoidSpeed(NULL),
    tAvoidSpeedX(NULL),
    tAvoidBrake(NULL),
    tAvoidBrakeX(NULL),
    tAccelCurveOffset(NULL),
    tCarefulBrake(NULL),
    tSkidAccel(NULL),
    tAccelExit(NULL),
    tSkidCorrection(NULL),
    tBumpCaution(NULL),
    tBrakeCurve(NULL),
    fDirt(0),
    Next(0),
    This(0),
    CarDiv(0),
    track(NULL),
    carhandle(NULL),
    car(NULL)
{
    if (!SRLinit)
        memset(SRL, 0, sizeof(SRL));
    SRLinit = 1;
}

/////////////////////////////////////////////////////////////////////////////
// Update tx and ty arrays
/////////////////////////////////////////////////////////////////////////////
void LRaceLine::UpdateTxTy(int i, int rl)
{
    SRL[rl].tx[i] = SRL[rl].tLane[i] * SRL[rl].txRight[i] + (1 - SRL[rl].tLane[i]) * SRL[rl].txLeft[i];
    SRL[rl].ty[i] = SRL[rl].tLane[i] * SRL[rl].tyRight[i] + (1 - SRL[rl].tLane[i]) * SRL[rl].tyLeft[i];
}                                                                               

/////////////////////////////////////////////////////////////////////////////
// Set segment info
/////////////////////////////////////////////////////////////////////////////
void LRaceLine::SetSegmentInfo(const tTrackSeg *pseg, double d, int i, double l, int rl)
{
    if (pseg)
    {
        //SRL[rl].tSegDist[pseg->id] = d;
        SRL[rl].tSegIndex[pseg->id] = i;
        SRL[rl].tElemLength[pseg->id] = l;
        if (pseg->id >= SRL[rl].Segs)
            SRL[rl].Segs = pseg->id + 1;
    }
}

double LRaceLine::getRInverse( int div )
{
    return SRL[SRLidx].tRInverse[((div + Divs) % Divs)];
}

void LRaceLine::AllocTrack( tTrack *ptrack )
{
    const tTrackSeg *psegCurrent = ptrack->seg;
    int i = 0, nseg = 0;

    FreeTrack(false);

    DivLength = GfParmGetNum( carhandle, "private", "DivLength", (char *)NULL, 3.0 );

    do
    {
        int Divisions = 1 + int(psegCurrent->length / DivLength);
        i += Divisions;
        nseg++;
        psegCurrent = psegCurrent->next;
    }
    while (psegCurrent != ptrack->seg);

    Divs = i - 1;
    nseg++;
    nseg = MAX(Divs, nseg);

    tSpeed = (double **) malloc( 2 * sizeof(double *) );

    SRLidx = (skill < 2.5 ? LINE_RL :
                            (skill < 5.0 ? LINE_RL+1 :
                                           (skill < 8.0 ? LINE_RL+2 : LINE_RL+3)));

    if (SRL[LINE_MID].init)
    {
        if (strcmp(SRL[LINE_MID].trackname, ptrack->name))
            FreeRaceline( LINE_MID );
    }
    if (SRL[SRLidx].init)
    {
        if (strcmp(SRL[SRLidx].trackname, ptrack->name))
            FreeRaceline( SRLidx );
    }

    if (!SRL[LINE_MID].init)
        AllocRaceline( LINE_MID, ptrack->name );

    if (!SRL[SRLidx].init)
        AllocRaceline( SRLidx, ptrack->name );

    tSpeed[0] = (double *) malloc( (Divs+1) * sizeof(double) );
    tSpeed[1] = (double *) malloc( (Divs+1) * sizeof(double) );
    tLaneShift = (double *) malloc( (Divs+1) * sizeof(double) );

    tRLMarginRgt = (LRLMod *) malloc( sizeof(LRLMod) );
    tRLMarginLft = (LRLMod *) malloc( sizeof(LRLMod) );
    tOTCaution = (LRLMod *) malloc( sizeof(LRLMod) );
    tRLSpeed = (LRLMod *) malloc( sizeof(LRLMod) );
    tRLBrake = (LRLMod *) malloc( sizeof(LRLMod) );
    tIntMargin = (LRLMod *) malloc( sizeof(LRLMod) );
    tExtMargin = (LRLMod *) malloc( sizeof(LRLMod) );
    tSecurity = (LRLMod *) malloc( sizeof(LRLMod) );
    tDecel = (LRLMod *) malloc( sizeof(LRLMod) );
    tADecel = (LRLMod *) malloc( sizeof(LRLMod) );
    tSpeedLimit = (LRLMod *) malloc( sizeof(LRLMod) );
    tCornerAccel = (LRLMod *) malloc( sizeof(LRLMod) );
    tAccelCurveDampen = (LRLMod *) malloc( sizeof(LRLMod) );
    tAccelCurveOffset = (LRLMod *) malloc( sizeof(LRLMod) );
    tCurveFactor = (LRLMod *) malloc( sizeof(LRLMod) );
    tAvoidSpeed = (LRLMod *) malloc( sizeof(LRLMod) );
    tAvoidSpeedX = (LRLMod *) malloc( sizeof(LRLMod) );
    tAvoidBrake = (LRLMod *) malloc( sizeof(LRLMod) );
    tAvoidBrakeX = (LRLMod *) malloc( sizeof(LRLMod) );
    tCarefulBrake = (LRLMod *) malloc( sizeof(LRLMod) );
    tSkidAccel = (LRLMod *) malloc( sizeof(LRLMod) );
    tAccelExit = (LRLMod *) malloc( sizeof(LRLMod) );
    tSkidCorrection = (LRLMod *) malloc( sizeof(LRLMod) );
    tBumpCaution = (LRLMod *) malloc( sizeof(LRLMod) );
    tBrakeCurve = (LRLMod *) malloc( sizeof(LRLMod) );

    memset(tSpeed[0], 0, (Divs+1) * sizeof(double));
    memset(tSpeed[1], 0, (Divs+1) * sizeof(double));
    memset(tLaneShift, 0, (Divs+1) * sizeof(double));

    memset(tDecel, 0, sizeof(LRLMod));
    memset(tADecel, 0, sizeof(LRLMod));
    memset(tOTCaution, 0, sizeof(LRLMod));
    memset(tRLMarginRgt, 0, sizeof(LRLMod));
    memset(tRLMarginLft, 0, sizeof(LRLMod));
    memset(tRLSpeed, 0, sizeof(LRLMod));
    memset(tRLBrake, 0, sizeof(LRLMod));
    memset(tIntMargin, 0, sizeof(LRLMod));
    memset(tExtMargin, 0, sizeof(LRLMod));
    memset(tSecurity, 0, sizeof(LRLMod));
    memset(tSpeedLimit, 0, sizeof(LRLMod));
    memset(tCornerAccel, 0, sizeof(LRLMod));
    memset(tAccelCurveDampen, 0, sizeof(LRLMod));
    memset(tAccelCurveOffset, 0, sizeof(LRLMod));
    memset(tCurveFactor, 0, sizeof(LRLMod));
    memset(tAvoidSpeed, 0, sizeof(LRLMod));
    memset(tAvoidSpeedX, 0, sizeof(LRLMod));
    memset(tAvoidBrake, 0, sizeof(LRLMod));
    memset(tAvoidBrakeX, 0, sizeof(LRLMod));
    memset(tCarefulBrake, 0, sizeof(LRLMod));
    memset(tSkidAccel, 0, sizeof(LRLMod));
    memset(tAccelExit, 0, sizeof(LRLMod));
    memset(tSkidCorrection, 0, sizeof(LRLMod));
    memset(tBumpCaution, 0, sizeof(LRLMod));
    memset(tBrakeCurve, 0, sizeof(LRLMod));

    CurveFactor = GfParmGetNum( carhandle, SECT_PRIVATE, PRV_CURVE_FACTOR, (char *)NULL, 0.12f );
    SecurityZ = GfParmGetNum( carhandle, SECT_PRIVATE, PRV_SECURITY, (char *)NULL, 0.00f );
    SteerGain = GfParmGetNum( carhandle, SECT_PRIVATE, PRV_STEER_GAIN, (char *)NULL, 1.30f );
    SteerSkid = GfParmGetNum( carhandle, SECT_PRIVATE, PRV_STEER_SKID, (char *)NULL, 0.06f );
    SkidAccel = GfParmGetNum( carhandle, SECT_PRIVATE, PRV_SKID_ACCEL, (char *)NULL, 0.0f );
    OvertakeCaution = GfParmGetNum( carhandle, SECT_PRIVATE, PRV_OVERTAKE_CAUTION, (char *)NULL, 0.0f );
    SkidCorrection = GfParmGetNum( carhandle, SECT_PRIVATE, PRV_SKID_CORRECTION, (char *)NULL, 1.0f );
    MinCornerInverse = GfParmGetNum( carhandle, SECT_PRIVATE, PRV_MIN_CORNER_INV, (char *)NULL, 0.002f );
    IncCornerInverse = GfParmGetNum( carhandle, SECT_PRIVATE, PRV_INC_CORNER_INV, (char *)NULL, 0.400f );
    IncCornerFactor = GfParmGetNum( carhandle, SECT_PRIVATE, PRV_INC_CORNER_FACTOR, (char *)NULL, 1.0f );
    CornerSpeed = GfParmGetNum( carhandle, SECT_PRIVATE, PRV_BASE_SPEED, (char *)NULL, 15.0f );
    BaseCornerSpeedX = GfParmGetNum( carhandle, SECT_PRIVATE, PRV_BASE_SPEED_X, (char *)NULL, 1.0f ) * (0.6 + MIN(0.45, ((12.0-skill)/12) / 2));
    AvoidSpeedAdjust = GfParmGetNum( carhandle, SECT_PRIVATE, PRV_AVOID_SPEED, (char *)NULL, 0.0f );
    AvoidSpeedAdjustX = GfParmGetNum( carhandle, SECT_PRIVATE, PRV_AVOID_SPEED_X, (char *)NULL, 1.0f );
    AvoidBrakeAdjust = GfParmGetNum( carhandle, SECT_PRIVATE, PRV_AVOID_BRAKE, (char *)NULL, 0.0f );
    AvoidBrakeAdjustX = GfParmGetNum( carhandle, SECT_PRIVATE, PRV_AVOID_BRAKE_X, (char *)NULL, 1.0f );
    IntMargin = GfParmGetNum( carhandle, SECT_PRIVATE, PRV_INT_MARGIN, (char *)NULL, 1.1f ) + (double) (SRLidx-1)/4; //skill/12;
    ExtMargin = GfParmGetNum( carhandle, SECT_PRIVATE, PRV_EXT_MARGIN, (char *)NULL, 1.7f ) + (double) (SRLidx-1)/2;   //skill/5;
    BrakeDelay = GfParmGetNum( carhandle, SECT_PRIVATE, PRV_BASE_BRAKE, (char *)NULL, 35.0f );
    BrakeDelayX = GfParmGetNum( carhandle, SECT_PRIVATE, PRV_BASE_BRAKE_X, (char *)NULL, 1.0f );
    BrakeMod = GfParmGetNum( carhandle, SECT_PRIVATE, PRV_BRAKE_MOD, (char *)NULL, 1.0f );
    BrakePower = GfParmGetNum( carhandle, SECT_PRIVATE, PRV_BRAKE_POWER, (char *)NULL, 0.5f );
    SteerMod = (int) GfParmGetNum( carhandle, SECT_PRIVATE, PRV_STEER_MOD, (char *)NULL, 0.0f );
    OfftrackAllowed = (int) GfParmGetNum( carhandle, SECT_PRIVATE, PRV_OFFTRACK_ALLOWED, (char *)NULL, 1.0f );
    roughlimit = GfParmGetNum( carhandle, SECT_PRIVATE, PRV_OFFTRACK_RLIMIT, (char *)NULL, 0.80f );
    MaxSteerTime = GfParmGetNum( carhandle, SECT_PRIVATE, PRV_MAX_STEER_TIME, (char *)NULL, 1.5f );
    MinSteerTime = GfParmGetNum( carhandle, SECT_PRIVATE, PRV_MIN_STEER_TIME, (char *)NULL, 1.0f );
    AvoidOffset = GfParmGetNum( carhandle, SECT_PRIVATE, PRV_AVOID_OFFSET, (char *)NULL, 1.3f);
    RaceLineDebug = GfParmGetNum( carhandle, SECT_PRIVATE, PRV_RACELINE_DEBUG, (char *)NULL, 0.0f ) ? true : false;
    AccelCurveDampen =  GfParmGetNum( carhandle, SECT_PRIVATE, PRV_ACCEL_CURVE, (char *)NULL, 1.0f );
    BrakeCurveDampen =  GfParmGetNum( carhandle, SECT_PRIVATE, PRV_BRAKE_CURVE, (char *)NULL, 1.0f );
    AccelCurveLimit =  GfParmGetNum( carhandle, SECT_PRIVATE, PRV_ACCEL_CURVE_LIMIT, (char *)NULL, 1.0f );
    BrakeCurveLimit =  GfParmGetNum( carhandle, SECT_PRIVATE, PRV_BRAKE_CURVE_LIMIT, (char *)NULL, 1.5f );
    BumpCaution =  GfParmGetNum( carhandle, SECT_PRIVATE, PRV_BUMP_CAUTION, (char *)NULL, 0.0f );
    SlopeFactor =  GfParmGetNum( carhandle, SECT_PRIVATE, PRV_SLOPE_FACTOR, (char *)NULL, 1.0f );
    ExitBoost =  GfParmGetNum( carhandle, SECT_PRIVATE, PRV_EXIT_BOOST, (char *)NULL, 0.0f );
    ExitBoostX =  GfParmGetNum( carhandle, SECT_PRIVATE, PRV_EXIT_BOOST_X, (char *)NULL, 1.0f );
    AvoidExitBoost =  GfParmGetNum( carhandle, SECT_PRIVATE, PRV_AV_EXIT_BOOST, (char *)NULL, 0.0f );
    AvoidExitBoostX =  GfParmGetNum( carhandle, SECT_PRIVATE, PRV_AV_EXIT_BOOST_X, (char *)NULL, 1.0f );
    maxfuel = GfParmGetNum( carhandle, SECT_CAR, PRM_TANK, (char *)NULL, 100.0f );

    // read custom values...
    for (i=0; i<LMOD_DATA; i++)
    {
        char str[32];
        sprintf(str, "%d %s", i, PRV_BEGIN);
        int div = (int) GfParmGetNum(carhandle, SECT_PRIVATE, str, (char *) NULL, 0.0);
        div = MAX(0, div);
        sprintf(str, "%d %s", i, PRV_END);
        int enddiv = (int) GfParmGetNum(carhandle, SECT_PRIVATE, str, (char *) NULL, 0.0);
        enddiv = MAX(div, MIN(Divs, enddiv));

        if (div == 0 && enddiv == 0)
            break;

        sprintf(str, "%d %s", i, PRV_BASE_SPEED);
        double rlspeed = GfParmGetNum(carhandle, SECT_PRIVATE, str, (char *) NULL, 0.0);
        if (rlspeed > 0.0)
            AddMod( tRLSpeed, div, enddiv, rlspeed, 0 );

        sprintf(str, "%d %s", i, PRV_BASE_BRAKE);
        double rlbrake = GfParmGetNum(carhandle, SECT_PRIVATE, str, (char *) NULL, 0.0);
        if (rlbrake > 0.0)
            AddMod( tRLBrake, div, enddiv, rlbrake, 0 );

        sprintf(str, "%d %s", i, PRV_CURVE_FACTOR);
        double curvefactor = GfParmGetNum(carhandle, SECT_PRIVATE, str, (char *) NULL, -100.0);
        if (curvefactor > -1.0)
            AddMod( tCurveFactor, div, enddiv, curvefactor, 0 );

        sprintf(str, "%d %s", i, PRV_SPEED_LIMIT);
        double speedlimit = GfParmGetNum(carhandle, SECT_PRIVATE, str, (char *) NULL, -100.0);
        if (speedlimit > 0.0)
            AddMod( tSpeedLimit, div, enddiv, speedlimit, 0 );

        sprintf(str, "%d %s", i, PRV_OVERTAKE_CAUTION);
        double otcaution = GfParmGetNum(carhandle, SECT_PRIVATE, str, (char *) NULL, -100.0);
        if (otcaution > -10.0)
            AddMod( tOTCaution, div, enddiv, otcaution, 0 );

        sprintf(str, "%d %s", i, PRV_INT_MARGIN);
        double rlintmargin = GfParmGetNum(carhandle, SECT_PRIVATE, str, (char *) NULL, -100.0);
        if (rlintmargin > -10.0)
            AddMod( tIntMargin, div, enddiv, rlintmargin, 0 );

        sprintf(str, "%d %s", i, PRV_EXT_MARGIN);
        double rlextmargin = GfParmGetNum(carhandle, SECT_PRIVATE, str, (char *) NULL, -100.0);
        if (rlextmargin > -10.0)
            AddMod( tExtMargin, div, enddiv, rlextmargin, 0 );

        sprintf(str, "%d %s", i, PRV_RL_RIGHT_MARGIN);
        double rlrgtmargin = GfParmGetNum(carhandle, SECT_PRIVATE, str, (char *) NULL, -100.0);
        if (rlrgtmargin > -10.0)
            AddMod( tRLMarginRgt, div, enddiv, rlrgtmargin, 0 );

        sprintf(str, "%d %s", i, PRV_RL_LEFT_MARGIN);
        double rllftmargin = GfParmGetNum(carhandle, SECT_PRIVATE, str, (char *) NULL, -100.0);
        if (rllftmargin > -10.0)
            AddMod( tRLMarginLft, div, enddiv, rllftmargin, 0 );

        sprintf(str, "%d %s", i, PRV_AV_RIGHT_MARGIN);
        double avrgtmargin = GfParmGetNum(carhandle, SECT_PRIVATE, str, (char *) NULL, -100.0);
        if (avrgtmargin > -10.0)
            AddMod( tRLMarginRgt, div, enddiv, avrgtmargin, 0 );

        sprintf(str, "%d %s", i, PRV_AV_LEFT_MARGIN);
        double avlftmargin = GfParmGetNum(carhandle, SECT_PRIVATE, str, (char *) NULL, -100.0);
        if (avlftmargin > -10.0)
            AddMod( tRLMarginLft, div, enddiv, avlftmargin, 0 );

        sprintf(str, "%d %s", i, PRV_AVOID_SPEED);
        double avoidspeed = GfParmGetNum(carhandle, SECT_PRIVATE, str, (char *) NULL, -100.0);
        if (avoidspeed > -10.0)
            AddMod( tAvoidSpeed, div, enddiv, avoidspeed, 0 );

        sprintf(str, "%d %s", i, PRV_AVOID_BRAKE);
        double avoidbrake = GfParmGetNum(carhandle, SECT_PRIVATE, str, (char *) NULL, -100.0);
        if (avoidbrake > -10.0)
            AddMod( tAvoidBrake, div, enddiv, avoidbrake, 0 );

        sprintf(str, "%d %s", i, PRV_SKID_CORRECTION);
        double skidcorrect = GfParmGetNum(carhandle, SECT_PRIVATE, str, (char *) NULL, -100.0);
        if (skidcorrect > -10.0)
            AddMod( tSkidCorrection, div, enddiv, skidcorrect, 0 );

        sprintf(str, "%d %s", i, PRV_BRAKE_CURVE);
        double brake_curve = GfParmGetNum(carhandle, SECT_PRIVATE, str, (char *) NULL, 0.0);
        if (brake_curve > 0.01 || brake_curve < -0.01)
            AddMod( tBrakeCurve, div, enddiv, brake_curve, 0 );

        sprintf(str, "%d %s", i, PRV_BUMP_CAUTION);
        double bumpcaution = GfParmGetNum(carhandle, SECT_PRIVATE, str, (char *) NULL, 0.0);
        if (bumpcaution > 0.01 || bumpcaution < -0.01)
            AddMod( tBumpCaution, div, enddiv, bumpcaution, 0 );
    }
}

/////////////////////////////////////////////////////////////////////////////
// Split the track into small elements
// ??? constant width supposed
/////////////////////////////////////////////////////////////////////////////
void LRaceLine::SplitTrack(tTrack *ptrack, int rl)
{
    int rf = (rl > LINE_RL ? LINE_RL : rl);

    SRL[rl].Segs = 0;
    tTrackSeg *psegCurrent = ptrack->seg;

    memset(tSpeed[rf], 0, (Divs+1) * sizeof(double));
    memset(SRL[rl].ExtLimit, 0, (Divs+1) * sizeof(double));

    double Distance = 0;
    double Angle = psegCurrent->angle[TR_ZS];
    double xPos = (psegCurrent->vertex[TR_SL].x +
                   psegCurrent->vertex[TR_SR].x) / 2;
    double yPos = (psegCurrent->vertex[TR_SL].y +
                   psegCurrent->vertex[TR_SR].y) / 2;

    int i = 0;

    do
    {
        int Divisions = 1 + int(psegCurrent->length / DivLength);
        double Step = psegCurrent->length / Divisions;
        double lmargin = 0.0, rmargin = 0.0;

        //if (rl == LINE_MID)
        SetSegmentInfo(psegCurrent, Distance + Step, i, Step, rl);

        if (rl >= LINE_RL && OfftrackAllowed)
        {
            for (int side=0; side<2; side++)
            {
                tTrackSeg *psegside = psegCurrent->side[side];
                double margin = 0.0;

                while (psegside != NULL)
                {
                    if (psegside->style == TR_WALL || psegside->style == TR_FENCE)
                    {
                        if (psegCurrent->type == TR_STR)
                        {
                            margin = MAX(0.0, margin - 0.5);
                        }
                        else
                        {
                            if ((psegCurrent->type == TR_LFT && side == TR_SIDE_LFT) ||
                                    (psegCurrent->type == TR_RGT && side == TR_SIDE_RGT))
                                margin = MAX(0.0, margin - 1.5);
                            else
                                margin = MAX(0.0, margin - 0.5);
                        }
                    }

                    if (psegside->style == TR_WALL || psegside->style == TR_FENCE)
                        break;

                    if (psegside->style == TR_PLAN &&
                            (psegside->surface->kFriction < psegCurrent->surface->kFriction*0.8 ||
                             (psegside->surface->kRoughness > MIN(roughlimit, MAX(0.02, psegCurrent->surface->kRoughness+0.05))) ||
                             (psegside->surface->kRollRes > MAX(0.005, psegCurrent->surface->kRollRes+0.03))))
                        break;

                    if (psegside->style == TR_CURB &&
                            (psegside->surface->kFriction >= psegCurrent->surface->kFriction * 0.9 &&
                             psegside->surface->kRoughness <= psegCurrent->surface->kRoughness + 0.05 &&
                             psegside->surface->kRollRes <= psegCurrent->surface->kRollRes * 0.03 &&
                             psegside->height <= psegside->width/10))
                        break;

                    if (ptrack->pits.type != TR_PIT_NONE)
                    {
                        if (((side == TR_SIDE_LFT && ptrack->pits.side == TR_LFT) ||
                             (side == TR_SIDE_RGT && ptrack->pits.side == TR_RGT)))
                        {
                            double pitstart = ptrack->pits.pitEntry->lgfromstart - 50.0;
                            double pitend = ptrack->pits.pitExit->lgfromstart + ptrack->pits.pitExit->length + 50.0;
                            if (pitstart > pitend)
                            {
                                if (psegCurrent->lgfromstart >= pitstart)
                                    pitend += ptrack->length;
                                else
                                    pitstart -= ptrack->length;
                            }
                            if (psegCurrent->lgfromstart >= pitstart && psegCurrent->lgfromstart <= pitend)
                                break;
                        }
                    }

                    double thiswidth = MIN(psegside->startWidth, psegside->endWidth) * 1.0;
                    if (psegCurrent->type == TR_STR)
                    {
                        if ((side == TR_SIDE_LFT && psegCurrent->type == TR_LFT && psegside->style == TR_CURB) ||
                                (side == TR_SIDE_RGT && psegCurrent->type == TR_RGT && psegside->style == TR_CURB))
                            thiswidth *= 0.5;
                        else if ((side == TR_SIDE_LFT && (psegCurrent->type == TR_RGT || psegCurrent->next->type != TR_LFT)) ||
                                 (side == TR_SIDE_RGT && (psegCurrent->type == TR_LFT || psegCurrent->next->type != TR_RGT)))
                            thiswidth *= 0.8;
                    }
                    margin += thiswidth;
                    psegside = psegside->side[side];
                }

                margin = MAX(0.0, margin - 0.0);

                if (margin > 0.0)
                {
                    margin /= psegCurrent->width;
                    if (side == TR_SIDE_LFT)
                        lmargin += margin;
                    else
                        rmargin += margin;
                }
            }
        }

        for (int j = Divisions; --j >= 0;)
        {
            double cosine = cos(Angle);
            double sine = sin(Angle);

            if (psegCurrent->type == TR_STR)
            {
                xPos += cosine * Step;
                yPos += sine * Step;
            }
            else
            {
                double r = psegCurrent->radius;
                double Theta = psegCurrent->arc / Divisions;
                double L = 2 * r * sin(Theta / 2);
                double x = L * cos(Theta / 2);
                double y;
                if (psegCurrent->type == TR_LFT)
                {
                    Angle += Theta;
                    y = L * sin(Theta / 2);
                }
                else
                {
                    Angle -= Theta;
                    y = -L * sin(Theta / 2);
                }
                xPos += x * cosine - y * sine;
                yPos += x * sine + y * cosine;
            }

            double dx = -psegCurrent->width * sin(Angle) / 2;
            double dy = psegCurrent->width * cos(Angle) / 2;
            SRL[rl].txLeft[i] = xPos + dx;
            SRL[rl].tyLeft[i] = yPos + dy;
            SRL[rl].txRight[i] = xPos - dx;
            SRL[rl].tyRight[i] = yPos - dy;
            SRL[rl].tLane[i] = 0.5;
            SRL[rl].tLaneLMargin[i] = lmargin;
            SRL[rl].tLaneRMargin[i] = rmargin;
            SRL[rl].tFriction[i] = psegCurrent->surface->kFriction;
            if (SRL[rl].tFriction[i] < 1) // ??? ugly trick for dirt
            {
                fDirt = 1;
                //SideDistInt = -1.5;
                //SideDistExt = 0.0;
            }
            UpdateTxTy(i, rl);

            Distance += Step;
            SRL[rl].tDistance[i] = Distance;
            SRL[rl].tDivSeg[i] = psegCurrent->id;
            SRL[rl].tSegment[psegCurrent->id] = psegCurrent;

            i++;
        }

        psegCurrent = psegCurrent->next;
    }
    while (psegCurrent != ptrack->seg);

    SRL[rl].Width = psegCurrent->width;
    SRL[rl].Length = Distance;
}

/////////////////////////////////////////////////////////////////////////////
// Compute the inverse of the radius
/////////////////////////////////////////////////////////////////////////////
double LRaceLine::GetRInverse(int prev, double x, double y, int next, int rl)
{
    double x1 = SRL[rl].tx[next] - x;
    double y1 = SRL[rl].ty[next] - y;
    double x2 = SRL[rl].tx[prev] - x;
    double y2 = SRL[rl].ty[prev] - y;
    double x3 = SRL[rl].tx[next] - SRL[rl].tx[prev];
    double y3 = SRL[rl].ty[next] - SRL[rl].ty[prev];

    double det = x1 * y2 - x2 * y1;
    double n1 = x1 * x1 + y1 * y1;
    double n2 = x2 * x2 + y2 * y2;
    double n3 = x3 * x3 + y3 * y3;
    double nnn = sqrt(n1 * n2 * n3);

    return 2 * det / nnn;
}

/////////////////////////////////////////////////////////////////////////////
// Change lane value to reach a given radius
/////////////////////////////////////////////////////////////////////////////
void LRaceLine::AdjustRadius(int prev, int i, int next, double TargetRInverse, int rl, double Security)
{
    double OldLane = SRL[rl].tLane[i];

    //
    // Start by aligning points for a reasonable initial lane
    //
    SRL[rl].tLane[i] = (-(SRL[rl].ty[next] - SRL[rl].ty[prev]) * (SRL[rl].txLeft[i] - SRL[rl].tx[prev]) +
                        (SRL[rl].tx[next] - SRL[rl].tx[prev]) * (SRL[rl].tyLeft[i] - SRL[rl].ty[prev])) /
            ( (SRL[rl].ty[next] - SRL[rl].ty[prev]) * (SRL[rl].txRight[i] - SRL[rl].txLeft[i]) -
              (SRL[rl].tx[next] - SRL[rl].tx[prev]) * (SRL[rl].tyRight[i] - SRL[rl].tyLeft[i]));

    double bumpExtMargin = 0.0;

#if 0
    if (rl != LINE_MID)
    {
        int range = 5;
        double prevzd=0.0, nextzd=0.0;
        double rI = TargetRInverse;

        for (int n=1; n<range; n++)
        {
            int x = ((i-n) + Divs) % Divs;
            prevzd += SRL[LINE_MID].tzd[x];
        }

        for (int n=0; n<range; n++)
        {
            int x = ((i+n) + Divs) % Divs;
            nextzd += SRL[LINE_MID].tzd[x];
        }

        double diff = prevzd - nextzd;

        if (diff > 0.10)
        {
            diff -= 0.10;
            double safespeed = MAX(15.0, 100.0 - (diff*diff) * 400);
            if (safespeed < 70.0)
            {
                bumpExtMargin = fabs((diff*diff) * (rI*200));
            }
        }
    }
#endif

    if (rl >= LINE_RL)
    {
        if (SRL[rl].tLane[i] < -0.2-SRL[rl].tLaneLMargin[i])
            SRL[rl].tLane[i] = -0.2-SRL[rl].tLaneLMargin[i];
        else if (SRL[rl].tLane[i] > 1.2+SRL[rl].tLaneRMargin[i])
            SRL[rl].tLane[i] = 1.2+SRL[rl].tLaneRMargin[i];

        /*
  if (fabs(tLaneShift[i]) >= 0.01)
  {
   SRL[rl].tLane[i] += tLaneShift[i] / SRL[rl].Width;
   tLaneShift[i] /= 2;
  }
  */
        if (Security == -1)
            SRL[rl].tLane[i] += tLaneShift[i] / SRL[rl].Width;
    }

    if (Security == -1)
        Security = (SecurityZ + GetModD( tSecurity, i ));
    UpdateTxTy(i, rl);

    //
    // Newton-like resolution method
    //
    const double dLane = 0.0001;

    double dx = dLane * (SRL[rl].txRight[i] - SRL[rl].txLeft[i]);
    double dy = dLane * (SRL[rl].tyRight[i] - SRL[rl].tyLeft[i]);

    double dRInverse = GetRInverse(prev, SRL[rl].tx[i] + dx, SRL[rl].ty[i] + dy, next, rl);
    double tcf = GetModD( tCurveFactor, i );
    double cf = (tcf != 0.0 ? tcf : CurveFactor);
    double intmargin = ((IntMargin) + GetModD( tIntMargin, i )) - cf * 5;
    double extmargin = ExtMargin + GetModD( tExtMargin, i );
    double rgtmargin = GetModD( tRLMarginRgt, i );
    double lftmargin = GetModD( tRLMarginLft, i );

    extmargin = MAX(extmargin, SRL[rl].ExtLimit[i]);
    extmargin = MAX(extmargin, bumpExtMargin);

    if (dRInverse > 0.000000001)
    {
        SRL[rl].tLane[i] += (dLane / dRInverse) * TargetRInverse;

        double ExtLane = (extmargin + Security) / SRL[rl].Width;
        double IntLane = ((intmargin) + Security) / SRL[rl].Width;
        if (ExtLane > 0.5)
            ExtLane = 0.5;
        if (IntLane > 0.5)
            IntLane = 0.5;

        if (rl >= LINE_RL)
        {
            if (TargetRInverse >= 0.0)
            {
                IntLane -= SRL[rl].tLaneLMargin[i];
                ExtLane -= SRL[rl].tLaneRMargin[i];
            }
            else
            {
                ExtLane -= SRL[rl].tLaneLMargin[i];
                IntLane -= SRL[rl].tLaneRMargin[i];
            }
        }

        if (TargetRInverse >= 0.0)
        {
            if (SRL[rl].tLane[i] < IntLane)
                SRL[rl].tLane[i] = IntLane;
            if (1 - SRL[rl].tLane[i] < ExtLane)
            {
                if (1 - OldLane < ExtLane)
                    SRL[rl].tLane[i] = Min(OldLane, SRL[rl].tLane[i]);
                else
                    SRL[rl].tLane[i] = 1 - ExtLane;
            }
        }
        else
        {
            if (SRL[rl].tLane[i] < ExtLane)
            {
                if (OldLane < ExtLane)
                    SRL[rl].tLane[i] = Max(OldLane, SRL[rl].tLane[i]);
                else
                    SRL[rl].tLane[i] = ExtLane;
            }
            if (1 - SRL[rl].tLane[i] < IntLane)
                SRL[rl].tLane[i] = 1 - IntLane;
        }

        if (rl == LINE_RL)
        {
            double lftlane = lftmargin / track->width;
            double rgtlane = 1.0 - rgtmargin / track->width;

            if (lftlane > 0.0)
                SRL[rl].tLane[i] = MAX(SRL[rl].tLane[i], lftlane);
            if (rgtlane < 1.0)
                SRL[rl].tLane[i] = MIN(SRL[rl].tLane[i], rgtlane);
        }
    }

    UpdateTxTy(i, rl);
}

/////////////////////////////////////////////////////////////////////////////
// Smooth path
/////////////////////////////////////////////////////////////////////////////
void LRaceLine::Smooth(int Step, int rl)
{
    int prev = ((Divs - Step) / Step) * Step;
    int prevprev = prev - Step;
    int next = Step;
    int nextnext = next + Step;

    for (int i = 0; i <= Divs - Step; i += Step)
    {
        double ri0 = GetRInverse(prevprev, SRL[rl].tx[prev], SRL[rl].ty[prev], i, rl);
        double ri1 = GetRInverse(i, SRL[rl].tx[next], SRL[rl].ty[next], nextnext, rl);
        double lPrev = Mag(SRL[rl].tx[i] - SRL[rl].tx[prev], SRL[rl].ty[i] - SRL[rl].ty[prev]);
        double lNext = Mag(SRL[rl].tx[i] - SRL[rl].tx[next], SRL[rl].ty[i] - SRL[rl].ty[next]);

        double TargetRInverse = (lNext * ri0 + lPrev * ri1) / (lNext + lPrev);

        double Security = lPrev * lNext / (8 * SecurityR);

        if (rl >= LINE_RL)
        {
            // Unused code? (kilo)
            /*int movingout = 0;
   if ((TargetRInverse > 0.0 && SRL[rl].tLane[next] > SRL[rl].tLane[prev]) ||
       (TargetRInverse < 0.0 && SRL[rl].tLane[next] < SRL[rl].tLane[prev]))
     movingout = 1;
   */

            if (ri0 * ri1 > 0)
            {
                double ac1 = fabs(ri0);
                double ac2 = fabs(ri1);
                {
                    double tcf = GetModD( tCurveFactor, next );
                    double curvefactor = (tcf != 0.0 ? tcf : CurveFactor);
                    //double tcd = GetModD( tAccelCurveDampen, next );
                    double ca = AccelCurveDampen;//(tcd > 0.0 ? tcd : AccelCurveDampen);
                    double cb = GetModD( tBrakeCurve, next );
                    if (cb < 0.1) cb = BrakeCurveDampen;

                    if (ac1 < ac2) // curve is increasing
                    {
                        ri0 += curvefactor * (ri1 - ri0 * cb);

                        tTrackSeg *seg = SRL[rl].tSegment[SRL[rl].tDivSeg[i]];
                        if (BrakeCurveLimit > 0.0 && seg->type != TR_STR && seg->radius < 400.0)
                            SRL[rl].ExtLimit[i] = MIN(BrakeCurveLimit, (400.0 - seg->radius) / 35.0);
                    }
                    else if (ac2 < ac1) // curve is decreasing
                    {
                        ri1 += curvefactor * (ri0 - ri1 * ca);

                        tTrackSeg *seg = SRL[rl].tSegment[SRL[rl].tDivSeg[i]];
                        if (AccelCurveLimit > 0.0 && seg->type != TR_STR && seg->radius < 400.0)
                            SRL[rl].ExtLimit[i] = MIN(AccelCurveLimit, (400.0 - seg->radius) / 50.0);
                    }
                }

                TargetRInverse = (lNext * ri0 + lPrev * ri1) / (lNext + lPrev);
            }
        }

        AdjustRadius(prev, i, next, TargetRInverse, rl, Security);

        prevprev = prev;
        prev = i;
        next = nextnext;
        nextnext = next + Step;
        if (nextnext > Divs - Step)
            nextnext = 0;
    }
}

/////////////////////////////////////////////////////////////////////////////
// Interpolate between two control points
/////////////////////////////////////////////////////////////////////////////
void LRaceLine::StepInterpolate(int iMin, int iMax, int Step, int rl)
{
    int next = (iMax + Step) % Divs;
    if (next > Divs - Step)
        next = 0;

    int prev = (((Divs + iMin - Step) % Divs) / Step) * Step;
    if (prev > Divs - Step)
        prev -= Step;

    double ir0 = GetRInverse(prev, SRL[rl].tx[iMin], SRL[rl].ty[iMin], iMax % Divs, rl);
    double ir1 = GetRInverse(iMin, SRL[rl].tx[iMax % Divs], SRL[rl].ty[iMax % Divs], next, rl);

    for (int k = iMax; --k > iMin;)
    {
        double x = double(k - iMin) / double(iMax - iMin);
        double TargetRInverse = x * ir1 + (1 - x) * ir0;
        AdjustRadius(iMin, k, iMax % Divs, TargetRInverse, rl);
    }
}

/*static void ClosestPointOnLineFromPoint(
  vec2f *lp1, vec2f *lp2,
  vec2f *p,   vec2f *r )
{
  double vx = lp2->x - lp1->x;
  double vy = lp2->y - lp1->y;
  double wx = p->x - lp1->x;
  double wy = p->y - lp1->y;

  double c1 = vx * wx + vy * wy;
  double c2 = vx * vx + vy * vy;
  double ratio = c1 / c2;

  r->x = lp1->x + ratio * vx;
  r->y = lp1->y + ratio * vy;
}*/

static double PointDist(vec2f *p1, vec2f *p2)
{
    double dx = (p1->x - p2->x);
    double dy = (p1->y - p2->y);
    return sqrt( dx*dx + dy*dy );
}

/////////////////////////////////////////////////////////////////////////////
// Find changes in line height
/////////////////////////////////////////////////////////////////////////////
void LRaceLine::CalcZCurvature(int rl)
{
    int i;

    for( i = 0; i < Divs; i++ )
    {
        tTrackSeg *seg = SRL[rl].tSegment[SRL[rl].tDivSeg[i]];

        SRL[rl].tz[i] = RtTrackHeightG(seg, (tdble)SRL[rl].tx[i], (tdble)SRL[rl].ty[i]);

        int next = (i + 1) % Divs;
        int prev = (i - 1 + Divs) % Divs;
        double rInverse = GetRInverse(prev, SRL[rl].tx[i], SRL[rl].ty[i], next, rl);
        SRL[rl].tRInverse[i] = rInverse;
    }

    for ( i = 0; i < Divs; i++ )
    {
        int j = ((i-1)+Divs) % Divs;

        vec2f pi, pj;
        pi.x = (tdble)SRL[rl].tx[i]; pi.y = (tdble)SRL[rl].ty[i];
        pj.x = (tdble)SRL[rl].tx[j]; pj.y = (tdble)SRL[rl].ty[j];

        SRL[rl].tzd[i] = (SRL[rl].tz[i] - SRL[rl].tz[j]) / PointDist(&pi, &pj);
    }

    for ( i = 0; i < Divs; i++ )
    {
        double zd = 0.0;
        for (int nx=0; nx<4; nx++)
        {
            int nex = (i + nx) % Divs;
            if (SRL[rl].tzd[nex] < 0.0)
                zd += SRL[rl].tzd[nex] * 2;
            else
                zd += SRL[rl].tzd[nex] * 0.2;
        }

        double camber = SegCamber(rl, i) - 0.002;
        if (camber < 0.0)
        {
            camber *= 3;
            if (rl == LINE_MID)
                camber *= 2;
        }
        double slope = camber + zd/3 * SlopeFactor;
        if (rl != LINE_RL)
        {
            if (slope < 0.0)
                slope *= 1.4;
            else
                slope *= 0.7;
        }

        SRL[rl].tFriction[i] *= 1.0 + MAX(-0.4, slope);

        if (slope < 0.0)
            SRL[rl].tBrakeFriction[i] = 1.0 + MAX(-0.4, slope/10);
        else
            SRL[rl].tBrakeFriction[i] = 1.0 + slope/40;
    }
}

/////////////////////////////////////////////////////////////////////////////
// Calls to StepInterpolate for the full path
/////////////////////////////////////////////////////////////////////////////
void LRaceLine::Interpolate(int Step, int rl)
{
    if (Step > 1)
    {
        int i;
        for (i = Step; i <= Divs - Step; i += Step)
            StepInterpolate(i - Step, i, Step, rl);
        StepInterpolate(i - Step, Divs, Step, rl);
    }
}

double LRaceLine::SegCamber(int rl, int div)
{
    tTrackSeg *seg = SRL[rl].tSegment[SRL[rl].tDivSeg[div]];
    double camber  = (((seg->vertex[TR_SR].z-seg->vertex[TR_SL].z) / 2) + ((seg->vertex[TR_ER].z-seg->vertex[TR_EL].z) / 2)) / seg->width;
    double camber1 = ((seg->vertex[TR_SR].z-seg->vertex[TR_SL].z)) / seg->width;
    double camber2 = ((seg->vertex[TR_ER].z-seg->vertex[TR_EL].z)) / seg->width;

    if (SRL[SRLidx].tRInverse[div] < 0.0)
    {
        camber = -camber;
        camber2 = -camber2;
        camber1 = -camber1;
    }
    if (camber2 < camber1)
        camber = camber2;

    return camber;
}

void LRaceLine::InitTrack(tTrack* ptrack, tSituation *p)
{
    track = ptrack;
    TrackInit(p);
}

void LRaceLine::TrackInit(tSituation *p)
{
    //
    // split track
    //

    for (int rl=LINE_MID; rl<=LINE_RL; rl++)
    {
        int idx = (rl == LINE_MID ? LINE_MID : SRLidx);

        if (SRL[idx].init <= 1)
        {
            GfLogInfo("USR initializing raceline %d (%s) for %s...\n", idx, SRL[idx].trackname, car->_name);

            SRL[idx].init = 2;
            SplitTrack(track, idx);

            //
            // Smoothing loop
            //
            int Iter = 4, i;
            if (idx >= LINE_RL)
                Iter = Iterations;
            int MaxStep = 132;
            for (int Step = MaxStep; (Step /= 2) > 0;)
            {
                for (i = Iter * int(sqrt((float) Step)); --i >= 0;)
                    Smooth(Step, idx);
                Interpolate(Step, idx);
            }

            CalcZCurvature(idx);
        }
        else
        {
            GfLogInfo("USR re-using raceline %d for %s...\n", idx, car->_name);
        }

        ComputeSpeed(idx);
    }
}

void LRaceLine::ComputeSpeed(int rl)
{
    //
    // Compute curvature and speed along the path
    //
    int rf = (rl > LINE_RL ? LINE_RL : rl);

    for (int i = Divs; --i >= 0;)
        tSpeed[rf][i] = -1.0;

    for (int i = Divs; --i >= 0;)
    {
        double trlspeed = GetModD( tRLSpeed, i );
        double avspeed = GetModD( tAvoidSpeed, i );
        double avspeedx = GetModD( tAvoidSpeedX, i );
        //int previ = ((i - 1) + Divs) % Divs;
        double cornerspeed = ((trlspeed > 0 ? trlspeed : CornerSpeed) + BaseCornerSpeed) * BaseCornerSpeedX * DefaultCornerSpeedX;

        if (rl == LINE_MID)
        {
            if (avspeed != 0.0)
                cornerspeed += avspeed;
            else if (avspeedx > 0.0)
                cornerspeed *= avspeedx;
            else
            {
                cornerspeed += AvoidSpeedAdjust;
                cornerspeed *= AvoidSpeedAdjustX;
            }
        }

        /*
  if (((SRL[rl].tRInverse[i] < -0.001 && SRL[rl].tLane[i] < SRL[rl].tLane[previ]) ||
       (SRL[rl].tRInverse[i] > 0.001 && SRL[rl].tLane[i] > SRL[rl].tLane[previ])))
  {
   if (rl == LINE_MID)
   {
    cornerspeed += AvoidExitBoost;
    cornerspeed *= AvoidExitBoostX;
   }
   else
   {
    cornerspeed += ExitBoost;
    cornerspeed *= ExitBoostX;
   }
  }
  */

        int nnext = (i + 5) % Divs;
        //int next = (i + 1) % Divs;
        //int prev = (i - 1 + Divs) % Divs;

        double rInverse = SRL[rl].tRInverse[i];
        double rI = fabs(rInverse);
        double MaxSpeed;

        double TireAccel = cornerspeed * SRL[rl].tFriction[i];

        if (rI > MinCornerInverse)
        {
            if (rI > IncCornerInverse)
                rI *= (1.0 + (rI - IncCornerInverse) * 50 * IncCornerFactor);
            if (CornerSpeedX)
                rI *= MIN(1.0, (1.0 - (rI*20*CornerSpeedX) + (3.0+CW)/30));
            double tca = GetModD( tCornerAccel, i );
            double ca = (tca >= 0.0 ? tca : CornerAccel);
            int tao = GetModI( tAccelCurveOffset, i );
            int ao = (tao != 0 ? tao : AccelCurveOffset);

            MaxSpeed = sqrt(TireAccel / (rI - MinCornerInverse));

            if (rl >= LINE_RL && ca > 0.0)
            {
                int canext = nnext;
                if (ao > 0)
                    canext = (canext + ao) % Divs;
                if ((rInverse > 0.0 && SRL[rl].tLane[canext] > SRL[rl].tLane[i]) ||
                        (rInverse < 0.0 && SRL[rl].tLane[canext] < SRL[rl].tLane[i]))
                {
                    MaxSpeed *= (rInverse > 0.0 ? 1.0 + ((ca/3+SRL[rl].tLane[canext]*0.7)/7)*ca : 1.0+((ca/3+(1.0-SRL[rl].tLane[canext])*0.7)/7)*ca);
                }
            }
        }
        else
            MaxSpeed = 10000;

        double bc = GetModD( tBumpCaution, i );
        if (bc < 0.01 && bc > -0.01)
            bc = BumpCaution;

        if (bc > 0.0)
        {
            // look for a sudden downturn in approaching track segments
            double prevzd = 0.0, nextzd = 0.0;
            int range = int(MAX(40.0, MIN(100.0, MaxSpeed)) / 10.0);

            bc += rI * 80;

            for (int n=1; n<range; n++)
            {
                int x = ((i-n) + Divs) % Divs;
                prevzd += SRL[rl].tzd[x] / MAX(1.0, double(n)/2);
            }

            for (int n=0; n<range; n++)
            {
                int x = ((i+n) + Divs) % Divs;
                nextzd += SRL[rl].tzd[x] / MAX(1.0, double(n+1)/2);
            }

            double diff = (prevzd - nextzd) * 2.2;

            if (diff > 0.10)
            {
                diff -= 0.10;
                double safespeed = MAX(15.0, 100.0 - (diff*diff) * 400 * bc);
                if (safespeed < MIN(70.0, MaxSpeed))
                {
                    // do a couple of divs before this one to be sure we actually reduce speed
                    for (int n=0; n<4; n++)
                    {
                        int f = ((i-n)+Divs) % Divs;
                        tSpeed[rf][f] = safespeed;
                    }

                    for (int n=0; n<4; n++)
                    {
                        int f = (i+n) % Divs;
                        tSpeed[rf][f] = safespeed;
                    }

                    MaxSpeed = MIN(MaxSpeed, safespeed);
                }
            }
        }

        double tlimit = GetModD( tSpeedLimit, i );
        if (tlimit > 10.0)
            MaxSpeed = MIN(MaxSpeed, tlimit);

        if (tSpeed[rf][i] < 0.0)
            tSpeed[rf][i] = MaxSpeed;
        else
            tSpeed[rf][i] = MIN(tSpeed[rf][i], MaxSpeed);
    }

    //
    // Anticipate braking
    //
    double brakedelay = (BrakeDelay + (rl == LINE_MID ? AvoidBrakeAdjust : 0.0)) * BrakeDelayX;
    if (rl == LINE_MID)
        brakedelay *= AvoidBrakeAdjustX;

    for (int i = (Divs*2)-1; --i >= 0;)
    {
        int next = (i+1) % Divs, index = i % Divs;
        //int nnext = (i+2) % Divs, prev = ((i-1)+Divs) % Divs;
        double bd = brakedelay + GetModD( tRLBrake, index );
        if (rl == LINE_MID)
        {
            bd += GetModD( tAvoidBrake, index );
            double bax = GetModD( tAvoidBrakeX, index );
            if (bax > 0.1)
                bd *= bax;
        }

        bd *= SRL[rl].tFriction[index];

        if (tSpeed[rf][index] > tSpeed[rf][next])
        {
            double nspeed = tSpeed[rf][next];

            if (BrakeMod > 1.0)
            {
                //double friction = SRL[rl].tBrakeFriction[index];

                double dist = sqrt((SRL[rl].tx[next] - SRL[rl].tx[index]) * (SRL[rl].tx[next] - SRL[rl].tx[index]) +
                                   (SRL[rl].ty[next] - SRL[rl].ty[index]) * (SRL[rl].ty[next] - SRL[rl].ty[index]));

                //double slowrate = 15;//(15.0 - MIN(150.0, tSpeed[rf][next])/70.0)/2;
                tSpeed[rf][index] = MIN(tSpeed[rf][index], tSpeed[rf][next] + (/*slowrate * friction * */ (dist) * bd)/1000);
            }
            else if (BrakeMod > 0.5)
            {
                tSpeed[rf][index] = MIN(tSpeed[rf][index],
                                        tSpeed[rf][next] + (BrakePower * MAX(0.2, (100.0 - tSpeed[rf][next])/50)) *
                                        (fabs(SRL[rl].tRInverse[next]) > 0.001 ? MAX(0.1, 1.0 - fabs(SRL[rl].tRInverse[next]*80)) : 1.0) *
                                        SRL[rl].tBrakeFriction[index]);
            }
            else if (BrakeMod > 0.1)
            {
                double dist = sqrt((SRL[rl].tx[next] - SRL[rl].tx[index]) * (SRL[rl].tx[next] - SRL[rl].tx[index]) +
                                   (SRL[rl].ty[next] - SRL[rl].ty[index]) * (SRL[rl].ty[next] - SRL[rl].ty[index])) * 10;
                double bspd = MAX(0.0, (MIN(140.0, tSpeed[rf][next]) - 30.0)) / 80 + fabs(SRL[rl].tRInverse[next])*40;
                tSpeed[rf][index] = MIN(tSpeed[rf][index],
                                        nspeed + MAX(0.1,
                                                     ((0.1 - MIN(0.085, fabs(SRL[rl].tRInverse[next])*7))
                                                      * SRL[rl].tBrakeFriction[index]
                                                      * MAX(bd/4.0, bd / ((nspeed*(nspeed/10))/20)))
                                                     * (MAX(0.05, 1.0 - (nspeed > 30.0 ? bspd*(bspd+0.2)  : 0.0)) * BrakeMod)
                                                     * dist*0.75)/10);
            }
            else
            {
                tSpeed[rf][index] = MIN(tSpeed[rf][index], nspeed + MAX(0.1,
                                                                        ((0.1 - MIN(0.085, fabs(SRL[rl].tRInverse[next])*8))
                                                                         * SRL[rl].tBrakeFriction[index]
                                                                         * MAX(bd/10.0, bd / ((nspeed*(nspeed/4))/20)))));
            }
        }
    }

    //
    // Exaggerate Acceleration
    //
    if (AccelExit > 0.0)
    {
        double *tmpSpeed = new double[Divs];

        for (int i = Divs-1; --i >= 0;)
        {
            int next = (i+1) % Divs;
            if (tSpeed[rf][i] < tSpeed[rf][next])
            {
                double sdiff = tSpeed[rf][next] - tSpeed[rf][i];
                sdiff *= AccelExit * MAX(0.2, 1.0 - fabs(SRL[rl].tRInverse[i]) * 80);
                tmpSpeed[i] = tSpeed[rf][i] + sdiff;
            }
        }

        for (int i = Divs-1; --i >= 0;)
            tSpeed[rf][i] = tmpSpeed[i];

        delete[] tmpSpeed;
    }
} 

void LRaceLine::FreeRaceline(int rl)
{
    if (SRL[rl].init)
    {
        GfLogInfo("USR freeing raceline structure %d\n", rl);
        SRL[rl].init = 0;
        if (SRL[rl].tx) free(SRL[rl].tx);
        if (SRL[rl].ty) free(SRL[rl].ty);
        if (SRL[rl].tz) free(SRL[rl].tz);
        if (SRL[rl].tzd) free(SRL[rl].tzd);
        if (SRL[rl].tRInverse) free(SRL[rl].tRInverse);
        if (SRL[rl].tLane) free(SRL[rl].tLane);
        if (SRL[rl].tDivSeg) free(SRL[rl].tDivSeg);
        if (SRL[rl].txLeft) free(SRL[rl].txLeft);
        if (SRL[rl].txRight) free(SRL[rl].txRight);
        if (SRL[rl].tyLeft) free(SRL[rl].tyLeft);
        if (SRL[rl].tyRight) free(SRL[rl].tyRight);
        if (SRL[rl].tLaneLMargin) free(SRL[rl].tLaneLMargin);
        if (SRL[rl].tLaneRMargin) free(SRL[rl].tLaneRMargin);
        if (SRL[rl].tDistance) free(SRL[rl].tDistance);
        if (SRL[rl].tBrakeFriction) free(SRL[rl].tBrakeFriction);
        if (SRL[rl].tFriction) free(SRL[rl].tFriction);
        if (SRL[rl].tSegment) free(SRL[rl].tSegment);
        if (SRL[rl].tSegIndex) free(SRL[rl].tSegIndex);
        if (SRL[rl].tElemLength) free(SRL[rl].tElemLength);
        if (SRL[rl].ExtLimit) free(SRL[rl].ExtLimit);
    }

    memset( &SRL[rl], 0, sizeof(SRaceLine) );
}

void LRaceLine::AllocRaceline(int rl, const char *trackname)
{
    if (!SRL[rl].init)
    {
        GfLogInfo("USR allocating raceline structure %d\n", rl);
        SRL[rl].init = 1;
        strncpy( SRL[rl].trackname, trackname, 63 );
        SRL[rl].tx = (double *) malloc( (Divs+1) * sizeof(double) );
        SRL[rl].ty = (double *) malloc( (Divs+1) * sizeof(double) );
        SRL[rl].tz = (double *) malloc( (Divs+1) * sizeof(double) );
        SRL[rl].tzd = (double *) malloc( (Divs+1) * sizeof(double) );
        SRL[rl].tRInverse = (double *) malloc( (Divs+1) * sizeof(double) );
        SRL[rl].tLane = (double *) malloc( (Divs+1) * sizeof(double) );
        SRL[rl].tDivSeg = (int *) malloc( (Divs+1) * sizeof(int) );
        SRL[rl].txLeft = (double *) malloc( (Divs+1) * sizeof(double) );
        SRL[rl].tyLeft = (double *) malloc( (Divs+1) * sizeof(double) );
        SRL[rl].txRight = (double *) malloc( (Divs+1) * sizeof(double) );
        SRL[rl].tyRight = (double *) malloc( (Divs+1) * sizeof(double) );
        SRL[rl].tLaneLMargin = (double *) malloc( (Divs+1) * sizeof(double) );
        SRL[rl].tLaneRMargin = (double *) malloc( (Divs+1) * sizeof(double) );
        SRL[rl].tDistance = (double *) malloc( (Divs+1) * sizeof(double) );
        SRL[rl].ExtLimit = (double *) malloc( (Divs+1) * sizeof(double) );
        SRL[rl].tElemLength = (double *) malloc( (Divs+1) * sizeof(double) );
        SRL[rl].tFriction = (double *) malloc( (Divs+1) * sizeof(double) );
        SRL[rl].tBrakeFriction = (double *) malloc( (Divs+1) * sizeof(double) );
        SRL[rl].tSegIndex = (int *) malloc( (Divs+1) * sizeof(int) );
        SRL[rl].tSegment = (tTrackSeg **) malloc( (Divs+1) * sizeof(tTrackSeg *) );
        memset(SRL[rl].tx, 0, (Divs+1) * sizeof(double));
        memset(SRL[rl].ty, 0, (Divs+1) * sizeof(double));
        memset(SRL[rl].tz, 0, (Divs+1) * sizeof(double));
        memset(SRL[rl].tzd, 0, (Divs+1) * sizeof(double));
        memset(SRL[rl].tRInverse, 0, (Divs+1) * sizeof(double));
        memset(SRL[rl].tLane, 0, (Divs+1) * sizeof(double));
        memset(SRL[rl].tDivSeg, 0, (Divs+1) * sizeof(int));
        memset(SRL[rl].tSegIndex, 0, (Divs+1) * sizeof(int));
        memset(SRL[rl].tSegment, 0, (Divs+1) * sizeof(tTrackSeg *));
        memset(SRL[rl].txLeft, 0, (Divs+1) * sizeof(double));
        memset(SRL[rl].tyLeft, 0, (Divs+1) * sizeof(double));
        memset(SRL[rl].txRight, 0, (Divs+1) * sizeof(double));
        memset(SRL[rl].tyRight, 0, (Divs+1) * sizeof(double));
        memset(SRL[rl].tDistance, 0, (Divs+1) * sizeof(double));
        memset(SRL[rl].ExtLimit, 0, (Divs+1) * sizeof(double));
        memset(SRL[rl].tElemLength, 0, (Divs+1) * sizeof(double));
        memset(SRL[rl].tFriction, 0, (Divs+1) * sizeof(double));
        memset(SRL[rl].tBrakeFriction, 0, (Divs+1) * sizeof(double));
        memset(SRL[rl].tLaneLMargin, 0, (Divs+1) * sizeof(double));
        memset(SRL[rl].tLaneRMargin, 0, (Divs+1) * sizeof(double));
    }
}

void LRaceLine::FreeTrack(bool freeall)
{
    if (freeall)
    {
        FreeRaceline(LINE_MID);
        FreeRaceline(SRLidx);
    }

    if (tSpeed)
    {
        if (tSpeed[0]) free(tSpeed[0]);
        if (tSpeed[1]) free(tSpeed[1]);
        free(tSpeed);
    }
    if (tLaneShift) free(tLaneShift);

    if (tRLMarginRgt) free(tRLMarginRgt);
    if (tRLMarginLft) free(tRLMarginLft);
    if (tOTCaution) free(tOTCaution);
    if (tRLSpeed) free(tRLSpeed);
    if (tRLBrake) free(tRLBrake);
    if (tIntMargin) free(tIntMargin);
    if (tExtMargin) free(tExtMargin);
    if (tSecurity) free(tSecurity);
    if (tDecel) free(tDecel);
    if (tADecel) free(tADecel);
    if (tSpeedLimit) free(tSpeedLimit);
    if (tCornerAccel) free(tCornerAccel);
    if (tAccelCurveDampen) free(tAccelCurveDampen);
    if (tAccelCurveOffset) free(tAccelCurveOffset);
    if (tCurveFactor) free(tCurveFactor);
    if (tAvoidSpeed) free(tAvoidSpeed);
    if (tAvoidSpeedX) free(tAvoidSpeedX);
    if (tAvoidBrake) free(tAvoidBrake);
    if (tAvoidBrakeX) free(tAvoidBrakeX);
    if (tCarefulBrake) free(tCarefulBrake);
    if (tSkidAccel) free(tSkidAccel);
    if (tAccelExit) free(tAccelExit);
    if (tSkidCorrection) free(tSkidCorrection);
    if (tBumpCaution) free(tBumpCaution);
    if (tBrakeCurve) free(tBrakeCurve);

    tSpeed = NULL;
    tLaneShift = NULL;

    tRLMarginRgt = tRLMarginLft = tOTCaution = tRLSpeed = tRLBrake = tIntMargin = NULL;
    tExtMargin = tSecurity = tDecel = tADecel = tSpeedLimit =tCornerAccel = tAccelCurveDampen = NULL;
    tCurveFactor = tAvoidSpeed = tAvoidSpeedX = tAvoidBrake = tAvoidBrakeX = tAccelCurveOffset = tCarefulBrake = NULL;
    tSkidAccel = tAccelExit = tSkidCorrection = tBrakeCurve = NULL;
}


////////////////////////////////////////////////////////////////////////////
// New race
////////////////////////////////////////////////////////////////////////////
void LRaceLine::NewRace(tCarElt* newcar, tSituation *s)
{
    car = newcar;
    carhandle = car->_carHandle;
    wheelbase = (car->priv.wheel[FRNT_RGT].relPos.x +
                 car->priv.wheel[FRNT_LFT].relPos.x -
                 car->priv.wheel[REAR_RGT].relPos.x -
                 car->priv.wheel[REAR_LFT].relPos.x) / 2;
    wheeltrack = (car->priv.wheel[FRNT_LFT].relPos.y +
                  car->priv.wheel[REAR_LFT].relPos.y -
                  car->priv.wheel[FRNT_RGT].relPos.y -
                  car->priv.wheel[REAR_RGT].relPos.y) / 2;
    deltaTime = s->deltaTime;
    avgerror = 0.0;
} 

double LRaceLine::calcAvoidSpeed(double offset, double rInv, double speed, double rlspeed)
{
    if (fabs(rInv) < 0.0001)
    {
        // treat as a straight
        return speed;
    }
    else if (rInv < 0.0)
    {
        // right hand bend, less speed on right side
        if (offset < 0.0)
            speed = MAX(speed * 0.8, speed - fabs(rInv) * fabs(offset*1.5) * 25);
        //else
        // speed = MIN(speed * 1.2, speed + fabs(rInv) * fabs(offset*1.5) * 25);
    }
    else
    {
        // left hand bend, less speed on left side
        if (offset > 0.0)
            speed = MAX(speed * 0.8, speed - fabs(rInv) * offset*1.5 * 25);
        //else
        // speed = MIN(speed * 1.25, speed + fabs(rInv) * fabs(offset*1.5) * 25);
    }

    speed = MIN(speed, rlspeed);

    return speed;
}

void LRaceLine::getOpponentInfo(double distance, int rl, double *aSpeed, double *rInv, double offset)
{
    int dist = int(distance / DivLength);
    double rinv = SRL[SRLidx].tRInverse[Next];
    double aspd = 1000.0;

    for (int i=1; i<dist; i++)
    {
        int div = (Next + i) % Divs;
        if (fabs(SRL[SRLidx].tRInverse[div]) > fabs(rinv))
            rinv = SRL[SRLidx].tRInverse[div];

        if (offset < -999.0)
        {
            aspd = MIN(aspd, calcAvoidSpeed( offset, rinv, tSpeed[rl][div], tSpeed[rl][div] ));
        }
        else
        {
            if (tSpeed[rl][div] < aspd)
                aspd = tSpeed[rl][div];
        }
    }

    *aSpeed = aspd;
    *rInv = rinv;
}

double LRaceLine::getRLAngle(int div)
{
    int prev = (div - 2 + Divs) % Divs;
    double dx = SRL[SRLidx].tx[div] - SRL[SRLidx].tx[prev];
    double dy = SRL[SRLidx].ty[div] - SRL[SRLidx].ty[prev];

    double angle = -(RtTrackSideTgAngleL(&(car->_trkPos)) - atan2(dy, dx));
    NORM_PI_PI(angle);
    return angle*0.8;
}

////////////////////////////////////////////////////////////////////////////
// Car control
////////////////////////////////////////////////////////////////////////////
void LRaceLine::GetRLSteerPoint( vec2f *rt, double *offset, double time )
{
    int count = 0;
    tTrackSeg *seg = car->_trkPos.seg;
    int SegId = car->_trkPos.seg->id;
    double dist = car->_trkPos.toStart;
    if (dist < 0)
        dist = 0;
    if (car->_trkPos.seg->type != TR_STR)
        dist *= car->_trkPos.seg->radius;
    int next = SRL[SRLidx].tSegIndex[SegId] + int(dist / SRL[SRLidx].tElemLength[SegId]);
    dist = 0.0;
    //double txLast = car->_pos_X;
    //double tyLast = car->_pos_Y;
    double Time = deltaTime*3 + MAX(0.0, time);

    {
        double X = car->_pos_X + car->_speed_X * Time;
        double Y = car->_pos_Y + car->_speed_Y * Time;
        int Index = SRL[SRLidx].tSegIndex[SegId] + int(dist / SRL[SRLidx].tElemLength[SegId]);
        Index = (Index + Divs - 5) % Divs;

        int maxcount = MAX(100, int(car->_speed_x*2));

        while (count < maxcount)
        {
            next = (Index + 1) % Divs;

            if (
                    ((SRL[SRLidx].tx[next] - SRL[SRLidx].tx[Index]) * (X - SRL[SRLidx].tx[next]) +
                     (SRL[SRLidx].ty[next] - SRL[SRLidx].ty[Index]) * (Y - SRL[SRLidx].ty[next]))
                    < -0.1)
            {
                break;
            }

            Index = next;
            count++;
        }

        rt->x = (tdble)SRL[SRLidx].tx[next];
        rt->y = (tdble)SRL[SRLidx].ty[next];
        *offset = -(SRL[SRLidx].tLane[next] * seg->width - seg->width/2);
    }
}

void LRaceLine::GetSteerPoint( double lookahead, vec2f *rt, double offset, double time )
{
    int maxcount = int(lookahead / DivLength + 1);
    int count = 0;
    //tTrackSeg *seg = car->_trkPos.seg;
    int SegId = car->_trkPos.seg->id;
    double dist = 0.0;//car->_trkPos.toStart;
#if 0
    if (dist < 0)
        dist = 0;
    if (car->_trkPos.seg->type != TR_STR)
        dist *= car->_trkPos.seg->radius;
#endif
    int next = SRL[SRLidx].tSegIndex[SegId] + int(dist / SRL[SRLidx].tElemLength[SegId]);
    dist = 0.0;
    //double txLast = car->_pos_X;
    //double tyLast = car->_pos_Y;
    double Time = deltaTime*3 + MAX(0.0, time/2);
    double carspeed = Mag(car->_speed_X, car->_speed_Y);
    double offlane = (offset > -90 ? ((track->width/2) - offset) / track->width : SRL[SRLidx].tLane[next]);

    if (time > 0.0 && carspeed > 10.0)
    {
        double X = car->_pos_X + car->_speed_X * Time;
        double Y = car->_pos_Y + car->_speed_Y * Time;
        int Index = SRL[SRLidx].tSegIndex[SegId] + int(dist / SRL[SRLidx].tElemLength[SegId]);
        Index = (Index + Divs - 5) % Divs;

        maxcount = MAX(100, int(car->_speed_x*2));

        while(count < maxcount)
        {
            next = (Index + 1) % Divs;

            if (
                    ((SRL[SRLidx].tx[next] - SRL[SRLidx].tx[Index]) * (X - SRL[SRLidx].tx[next]) +
                     (SRL[SRLidx].ty[next] - SRL[SRLidx].ty[Index]) * (Y - SRL[SRLidx].ty[next]))
                    < -0.1)
            {
                break;
            }

            Index = next;
            count++;
        }

        rt->x = (tdble)(offlane * SRL[SRLidx].txRight[next] + (1 - offlane) * SRL[SRLidx].txLeft[next]);
        rt->y = (tdble)(offlane * SRL[SRLidx].tyRight[next] + (1 - offlane) * SRL[SRLidx].tyLeft[next]);
    }
    else
    {
        next = Next;
        double txLast = offlane * SRL[SRLidx].txRight[This] + (1 - offlane) * SRL[SRLidx].txLeft[This];
        double tyLast = offlane * SRL[SRLidx].tyRight[This] + (1 - offlane) * SRL[SRLidx].tyLeft[This];

        while (count < maxcount)
        {
            double txNext = offlane * SRL[SRLidx].txRight[next] + (1 - offlane) * SRL[SRLidx].txLeft[next];
            double tyNext = offlane * SRL[SRLidx].tyRight[next] + (1 - offlane) * SRL[SRLidx].tyLeft[next];
            double distx = txNext - txLast;
            double disty = tyNext - tyLast;
            double thisdist = sqrt( distx*distx + disty*disty );
            if ((offset < 0.0 && SRL[SRLidx].tRInverse[next] > 0.0) ||
                    (offset > 0.0 && SRL[SRLidx].tRInverse[next] < 0.0))
            {
                thisdist *= 1.0 - MIN(0.7, (fabs(offset) / (track->width/2)) * fabs(SRL[SRLidx].tRInverse[next]) * car->_speed_x * car->_speed_x/10);
            }
            dist += thisdist;

            rt->x = (tdble)txNext;
            rt->y = (tdble)tyNext;

            if (dist >= lookahead)
            {
                break;
            }

            next = (next + 1) % Divs;
            txLast = txNext;
            tyLast = tyNext;
            count++;
        }
    }
}

void LRaceLine::GetRaceLineData(tSituation *s, LRaceLineData *pdata)
{
    //
    // Find index in data arrays
    //
    data = pdata;
    //tTrackSeg *seg = car->_trkPos.seg;
    int SegId = car->_trkPos.seg->id;
    double dist = car->_trkPos.toStart;
    if (dist < 0)
        dist = 0;
    if (car->_trkPos.seg->type != TR_STR)
        dist *= car->_trkPos.seg->radius;
    int Index = SRL[SRLidx].tSegIndex[SegId] + int(dist / SRL[SRLidx].tElemLength[SegId]);
    This = data->thisdiv = Index;
    //double d = SRL[SRLidx].tSegDist[SegId] + dist;
    laststeer = data->steer;

    double timefactor = s->deltaTime*(3.6);// + MAX(0.0, ((car->_speed_x-45.0) / 10) * TimeFactor));
    if (data->followdist < car->_speed_x*0.7)
        timefactor *= 1.0 + (car->_speed_X*0.7 - data->followdist)/car->_speed_x*0.5;

    {
        double amI = MIN(0.05, MAX(-0.05, SRL[SRLidx].tRInverse[Next]));
        double famI = fabs(amI);
        double time_mod = 1.0;

        if (famI > 0.0)
        {
            double toMid = car->_trkPos.toMiddle + data->speedangle * 20;
            //double toLeft = car->_trkPos.toLeft - data->speedangle * 20;
            //double toRight = car->_trkPos.toRight + data->speedangle * 20;
            double modfactor = (car->_speed_x / data->avspeed);
            modfactor *= modfactor;

            if (amI > 0.0)
            {
                if (toMid < 0.0)
                    time_mod += famI * (MIN(track->width/2, fabs(toMid)) / track->width) * 50;
            }
            else
            {
                if (toMid > 0.0)
                    time_mod += famI * (MIN(track->width/2, fabs(toMid)) / track->width) * 50;
            }
        }

        timefactor *= time_mod;
    }

    if (car->_accel_x < 0.0)
        timefactor *= 1.0 + MIN(2.0, -(car->_accel_x/10));
    //if (data->collision)
    //  timefactor *= 1.0 + MAX(0.0, (5.0-data->collision) / 5.0);
    double Time = timefactor; // + CornerAccel/80;
    //double X4 = car->_pos_X + car->_speed_X * 0.5 / 2;
    //double Y4 = car->_pos_Y + car->_speed_Y * 0.5 / 2;
    double X = car->_pos_X + car->_speed_X * Time / 2;
    double Y = car->_pos_Y + car->_speed_Y * Time / 2;
    //double Xk = car->_pos_X + car->_speed_X * 0.1 / 2;
    //double Yk = car->_pos_Y + car->_speed_Y * 0.1 / 2;
    data->lookahead = 0.0f;
    data->aInverse = SRL[LINE_MID].tRInverse[Index];
    double divcount = 1.0;

    This = Index = (Index + Divs - 5) % Divs;
    int SNext = Index, SNext2 = Index;
    int limit = MAX(30, int(car->_speed_x*2));
    int count = 0;

    while(count < limit)
    {
        Next = SNext = SNext2 = (Index + 1) % Divs;
        double dx = SRL[SRLidx].tx[Next] - car->_pos_X;
        double dy = SRL[SRLidx].ty[Next] - car->_pos_Y;
        data->lookahead = sqrt(dx*dx + dy*dy);
        if (//data->lookahead > 10.0f &&
                (SRL[SRLidx].tx[Next] - SRL[SRLidx].tx[Index]) * (X - SRL[SRLidx].tx[Next]) +
                (SRL[SRLidx].ty[Next] - SRL[SRLidx].ty[Index]) * (Y - SRL[SRLidx].ty[Next]) < MIN(-0.1, -((car->_speed_x/400) + (car->_accel_x/20))))
        {
            break;
        }
        Index = Next;
        data->aInverse += SRL[SRLidx].tRInverse[Index] * MAX(0.0, 1.0 - divcount/15);
        divcount++;
        count++;
    }

    if (data->followdist <= 5.0)
    {
        //int snext = SNext;
        //Next = (Next + int((car->_dimension_x + (5.0 - data->followdist)*1.2) / DivLength)) % Divs;
        SNext = (Next + int((car->_dimension_x + (5.0 - data->followdist)*6) / DivLength)) % Divs;
        if (car->_accel_x > 0.0 && data->followdist <= 2.0)
            SNext = (Next + Divs - int(car->_accel_x * (2.0/MAX(0.4, data->followdist)))) % Divs;

        //if (data->followdist <= 2.0)
        // SNext2 = (Next + int((car->_dimension_x + (2.0 - data->followdist)*15) / DivLength)) % Divs;
        //Index = ((Next + Divs) - 1) % Divs;
    }

    GetPoint( car->_trkPos.toMiddle, NULL, &(data->aInverse) );

    This = data->thisdiv = Index;
    data->nextdiv = Next;
    data->lookahead *= 0.8f;
    data->rInverse = SRL[SRLidx].tRInverse[Index];
    data->mInverse = SRL[LINE_MID].tRInverse[Index];
    data->decel = GetModD( tDecel, Index );
    data->adecel = GetModD( tADecel, Index );
    data->overtakecaution = MAX(OvertakeCaution, GetModD( tOTCaution, Index ));
    if (data->overtakecaution < 0.0)
        data->overtakecaution = MAX(-0.5, data->overtakecaution/3);
    data->braking = tSpeed[LINE_RL][Index] - tSpeed[LINE_RL][Next];
    data->rlangle = getRLAngle(Next);

#if 1
    if ((SRL[SRLidx].tRInverse[Next] < 0.0 && car->_trkPos.toMiddle > 0.0) ||
            (SRL[SRLidx].tRInverse[Next] > 0.0 && car->_trkPos.toMiddle < 0.0))
    {
        //data->lookahead *= MAX(1.0, MIN(3.6, 1.0 + (MIN(2.6, fabs(car->_trkPos.toMiddle) / (seg->width/2)) / 2) * (1.0 + fabs(SRL[SRLidx].tRInverse[Next]) * 65.0 + car->_speed_x/120.0)));
    }
    else if ((SRL[SRLidx].tRInverse[Next] < 0.0 && car->_trkPos.toRight < 5.0) ||
             (SRL[SRLidx].tRInverse[Next] > 0.0 && car->_trkPos.toLeft < 5.0))
    {
        data->lookahead *= MAX(0.8, MIN(1.0, 1.0 - fabs(SRL[SRLidx].tRInverse[Next])*200.0 * ((5.0-MIN(car->_trkPos.toRight, car->_trkPos.toLeft))/5.0)));
    }
#endif

    data->target.x = SRL[SRLidx].tx[Next];
    data->target.y = SRL[SRLidx].ty[Next];
    data->lane = SRL[SRLidx].tLane[Index];

    //
    // Find target speed
    //
    int avnext = (Next-3+Divs) % Divs;
    int avindex = (Index-3+Divs) % Divs;
    double c0 = (SRL[SRLidx].tx[avnext] - SRL[SRLidx].tx[avindex]) * (SRL[SRLidx].tx[avnext] - X)*1.5 +
            (SRL[SRLidx].ty[avnext] - SRL[SRLidx].ty[avindex]) * (SRL[SRLidx].ty[avnext] - Y)*1.5;
    double c1 = (SRL[SRLidx].tx[avnext] - SRL[SRLidx].tx[avindex]) * (X - SRL[SRLidx].tx[avindex])*3 +
            (SRL[SRLidx].ty[avnext] - SRL[SRLidx].ty[avindex]) * (Y - SRL[SRLidx].ty[avindex])*3;
    {
        double sum = c0 + c1;
        c0 /= sum;
        c1 /= sum;
    }

    //double ospeed = TargetSpeed, aspeed = ATargetSpeed;
    ATargetSpeed = tSpeed[LINE_MID][avnext];//(1 - c0) * tSpeed[LINE_MID][avnext] + c0 * tSpeed[LINE_MID][avindex];
    data->avspeed = MAX(10.0, ATargetSpeed);
    if (!data->overtakecaution)
        data->avspeed = MAX(ATargetSpeed, tSpeed[LINE_MID][avnext]);
    else
        data->avspeed = MAX(ATargetSpeed, tSpeed[LINE_MID][avnext] - data->overtakecaution/8);

    c0 = (SRL[SRLidx].tx[Next] - SRL[SRLidx].tx[Index]) * (SRL[SRLidx].tx[Next] - X) +
            (SRL[SRLidx].ty[Next] - SRL[SRLidx].ty[Index]) * (SRL[SRLidx].ty[Next] - Y);
    c1 = (SRL[SRLidx].tx[Next] - SRL[SRLidx].tx[Index]) * (X - SRL[SRLidx].tx[Index]) +
            (SRL[SRLidx].ty[Next] - SRL[SRLidx].ty[Index]) * (Y - SRL[SRLidx].ty[Index]);
    {
        double sum = c0 + c1;
        c0 /= sum;
        c1 /= sum;
    }

    {
        int snext = SNext;
        double spdnxt = tSpeed[LINE_RL][snext];
        double spdidx = tSpeed[LINE_RL][Index];

        TargetSpeed = (1 - c0) * spdnxt + c0 * spdidx;
    }

    data->speed = TargetSpeed;
    //if (TargetSpeed > ospeed && ATargetSpeed < aspeed)
    // data->avspeed = ATargetSpeed = MAX(data->avspeed, aspeed * (TargetSpeed / ospeed));
    data->avspeed = MAX(data->speed*0.6, MIN(data->speed+2.0, data->avspeed));

    double laneoffset = SRL[SRLidx].Width/2 - (SRL[SRLidx].tLane[Next] * SRL[SRLidx].Width);
    data->offset = laneoffset;
    //fprintf(stderr,"GetRLData: offset=%.2f Next=%d lane=%.4f Width=%.2f\n",laneoffset,Next,tLane[Next],Width);

    //double sa = (data->angle > 0.0 ? MIN(data->angle, data->angle+data->speedangle/2) : MAX(data->angle, data->angle+data->speedangle/2));
    //CalcAvoidSpeed( Next, data, sa );

#if 1
    int KNext = Next, KIndex = Index;
    int nnext = Next;

    {
        int nextinc = int(car->_speed_x/10);
        nnext = (Next + nextinc) % Divs;
        data->amInverse = 0.0;

        for (int i=0; i<nextinc; i++)
            data->amInverse += fabs(SRL[LINE_MID].tRInverse[((Next+i)%Divs)]);
        data->amInverse /= nextinc;
    }

    nnext = (Next + int(car->_speed_x/10)) % Divs;

    //
    // Find target curvature (for the inside wheel)
    //
    double TargetCurvature = (1 - c0) * SRL[SRLidx].tRInverse[KNext] + c0 * SRL[SRLidx].tRInverse[Index];
    if (fabs(TargetCurvature) > 0.01)
    {
        double r = 1 / TargetCurvature;
        if (r > 0)
            r -= wheeltrack / 2;
        else
            r += wheeltrack / 2;
        TargetCurvature = 1 / r;
    }


    data->insideline = data->outsideline = 0;

    data->closing = data->exiting = 0;
    if ((SRL[SRLidx].tRInverse[Next] < -0.001 && SRL[SRLidx].tLane[Next] > SRL[SRLidx].tLane[Index]) ||
            (SRL[SRLidx].tRInverse[Next] > 0.001 && SRL[SRLidx].tLane[Next] < SRL[SRLidx].tLane[Index]))
        data->closing = 1;
    else
    {
        data->slowavspeed = MAX(data->avspeed-(data->insideline?1.0:2.5), data->avspeed*0.9);
        if (((SRL[SRLidx].tRInverse[Next] < -0.001 && SRL[SRLidx].tLane[Next] < SRL[SRLidx].tLane[Index]) ||
             (SRL[SRLidx].tRInverse[Next] > 0.001 && SRL[SRLidx].tLane[Next] > SRL[SRLidx].tLane[Index])) &&
                tSpeed[LINE_RL][nnext] >= tSpeed[LINE_RL][This])
        {
            data->exiting = 1;

            TargetSpeed = data->speed = data->speed * ExitBoostX;

            if ((SRL[SRLidx].tRInverse[Next] > 0.0 && car->_trkPos.toLeft <= SRL[SRLidx].tLane[Next] * SRL[SRLidx].Width + 1.0) ||
                    (SRL[SRLidx].tRInverse[Next] < 0.0 && car->_trkPos.toLeft >= SRL[SRLidx].tLane[Next] * SRL[SRLidx].Width - 1.0))
            {
                // inside raceline
                data->insideline = 1;
                data->avspeed = MAX(data->avspeed, data->speed);
            }
            else if (tSpeed[LINE_RL][Next] >= tSpeed[LINE_RL][This] &&
                     ((SRL[SRLidx].tRInverse[Next] > 0.0 && SRL[SRLidx].tLane[Next] > SRL[SRLidx].tLane[This] && car->_trkPos.toLeft > SRL[SRLidx].tLane[Next]*SRL[SRLidx].Width+1.0) ||
                      (SRL[SRLidx].tRInverse[Next] < 0.0 && SRL[SRLidx].tLane[Next] < SRL[SRLidx].tLane[This] && car->_trkPos.toLeft < SRL[SRLidx].tLane[Next]*SRL[SRLidx].Width-1.0)))
            {
                data->outsideline = 1;
            }
#if 0
            double ae = GetModD( tAccelExit, This );
            if (ae > 0.0)
            {
                data->speed *= ae;
                data->avspeed *= MIN(ae, AvoidAccelExit);
            }
            else
            {
                data->speed += AccelExit;
                data->avspeed += AvoidAccelExit;
            }
#endif
        }
    }

    data->speedchange = tSpeed[LINE_RL][Next] - tSpeed[LINE_RL][This];
    data->accel_redux = 0.0;
    data->slowavspeed = calcAvoidSpeed( car->_trkPos.toMiddle, SRL[SRLidx].tRInverse[Next], data->avspeed, data->speed );
    data->avspeed = MAX(data->avspeed, data->slowavspeed);

    if (!SteerMod)
    {
        // new, simpler steering method
        vec2f target(0,0);
        double carspeed = Mag(car->_speed_X, car->_speed_Y);
        double steertime = MIN(MaxSteerTime, MinSteerTime + MAX(0.0, carspeed-20.0)/30.0);
        //double lane2left = track->width * SRL[SRLidx].tLane[Next];

        // an idea to implement soon...
        //if (car->_accel_x < 0.0)
        // steertime *= MIN(1.4, MAX(1.0, -car->_accel_x/10));

        GetSteerPoint(5.0 + car->_speed_x/10, &target, -100.0, steertime);
        double targetAngle = atan2(target.y - car->_pos_Y, target.x - car->_pos_X);
        double offline = MIN(2.0, MAX(-2.0, (SRL[SRLidx].tLane[Next] * track->width - car->_trkPos.toLeft)));
        targetAngle -= offline/15;

        double steer_direction = targetAngle - (car->_yaw + car->_yaw_rate/15);
        NORM_PI_PI(steer_direction);

        k1999steer = steer_direction / car->_steerLock;

        double nextangle = data->angle + car->_yaw_rate/3;

        if (fabs(nextangle) > fabs(data->speedangle))
        {
            //double sa = MAX(-0.3, MIN(0.3, data->speedangle/3));
            double anglediff = (data->speedangle - nextangle) * (0.1 + fabs(nextangle)/6);
            k1999steer += anglediff * (1.0 + MAX(1.0, 1.0 - car->_accel_x/5));
        }

        int Prev = (KIndex + Divs - 1) % Divs;
        int NextNext = (KNext + 1) % Divs;
        double dx = SRL[SRLidx].tx[NextNext] - SRL[SRLidx].tx[Prev];
        double dy = SRL[SRLidx].ty[NextNext] - SRL[SRLidx].ty[Prev];

        //double vx = car->_speed_X;
        //double vy = car->_speed_Y;
        double dirx = cos(car->_yaw);
        double diry = sin(car->_yaw);
        double SinAngleError = dx * diry - dy * dirx;

        if (fabs(SRL[SRLidx].tRInverse[Next]) > MinCornerInverse &&
                fabs(k1999steer) > 0.2 &&
                ((SinAngleError > 0.0 && SRL[SRLidx].tRInverse[Next] > 0.0 && data->speedangle < 0.0) ||
                 (SinAngleError < 0.0 && SRL[SRLidx].tRInverse[Next] < 0.0 && data->speedangle > 0.0)))
        {
            data->accel_redux = fabs(SinAngleError) * ((fabs(k1999steer)-0.2)*0.7) +
                    (fabs(SRL[SRLidx].tRInverse[Next]) - MinCornerInverse) * 1500 * fabs(data->speedangle);
        }
    }
    else
    {
        //
        // Steering control
        //
        double Error = 0;
        double VnError = 0;
        double Skid = 0;
        //unused code? (kilo)
        //double CosAngleError = 1;
        double SinAngleError = 0;
        double carspeed = Mag(car->_speed_X, car->_speed_Y);
        int KNext = (Next+1) % Divs;
        {
            //
            // Ideal value
            //
            {
                k1999steer = atan(wheelbase * TargetCurvature) / car->_steerLock;

                //fprintf(stderr,"a %.3f",k1999steer);
#if 0
                double steergain0 = GetModD( tSteerGain0, CarDiv );
                double steergain1 = GetModD( tSteerGain1, CarDiv );

                double steergain = steergain0 + (steergain1 - steergain0) * (1.0 - (car->_fuel-5.0) / (maxfuel-5.0));
                if (!steergain) steergain = SteerGain;
#else
                double steergain = SteerGain;
#endif

                double cspeed = carspeed;
                if (data->collision)
                    cspeed -= data->collision*3;

                double curlane = car->_trkPos.toLeft / track->width;
                double inside = (!data->closing ? 0.0 : MAX(0.0,
                                                            ((TargetCurvature < 0.0 ? curlane-SRL[SRLidx].tLane[Index] : SRL[SRLidx].tLane[Index]-curlane))));
                steergain = MIN(steergain, MAX(1.0, steergain - inside*2));

                if (carspeed < TargetSpeed-1.0 && (data->mode != mode_normal || data->aligned_time < 10))
                {
                    // decrease steergain as speed is slower than targetspeed and we're either avoiding
                    // or have just recovered from avoiding.
                    steergain = MAX(0.7, steergain - MAX(((TargetSpeed-1.0)-carspeed)/5.0, (1.0 - (carspeed/(TargetSpeed-1.0))) * 12) * (1.0 + fabs(SRL[SRLidx].tRInverse[Next]*10)));
                }

                // decrease steergain at high speed to stop bouncing over curbs
                if (carspeed > 60)
                    steergain = MAX(MIN(1.0, SteerGain), steergain - (carspeed-60.0)/40);

                // what does this do???
                {
                    if (SRL[SRLidx].tRInverse[Next] < -0.002)
                    {
                        if (car->_trkPos.toRight < (-SRL[SRLidx].tRInverse[Next] * 200))
                            steergain = MAX(1.0, steergain - ((-SRL[SRLidx].tRInverse[Next]*200) - car->_trkPos.toRight)/20);
                    }
                    else if (SRL[SRLidx].tRInverse[Next] > 0.002)
                    {
                        if (car->_trkPos.toLeft < (SRL[SRLidx].tRInverse[Next] * 200))
                            steergain = MAX(1.0, steergain - ((SRL[SRLidx].tRInverse[Next]*200) - car->_trkPos.toLeft)/20);
                    }
                }

                k1999steer = atan(wheelbase * steergain * TargetCurvature) / car->_steerLock;
                //fprintf(stderr," b %.3f",k1999steer);

                if (car->_accel_x > 0.0)
                {
                    double ri = (SRL[SRLidx].tRInverse[Next] + SRL[SRLidx].tRInverse[nnext]) / 2;
                    if (data->mode != mode_normal || data->aligned_time < 5)
                        k1999steer += ri * SteerRIAccC;
                    else
                        k1999steer += ri * SteerRIAcc;
                }
                //fprintf(stderr," c %.3f",k1999steer);
            }

            data->NSsteer = lastNksteer = k1999steer;
            int Prev = (KIndex + Divs - 1) % Divs;
            //int PrevPrev = (KIndex + Divs - 5) % Divs;
            int NextNext = (KNext + 1) % Divs;

            //
            // Servo system to stay on the pre-computed path
            //
            if (data->mode == mode_normal && data->aligned_time > 10)
            {
                double dx = SRL[SRLidx].tx[NextNext] - SRL[SRLidx].tx[Prev];
                double dy = SRL[SRLidx].ty[NextNext] - SRL[SRLidx].ty[Prev];
                Error = (dx * (Y - SRL[SRLidx].ty[Prev]) - dy * (X - SRL[SRLidx].tx[Prev])) / Mag(dx, dy);
            }
            else
            {
                double dx = SRL[SRLidx].tx[KNext] - SRL[SRLidx].tx[KIndex];
                double dy = SRL[SRLidx].ty[KNext] - SRL[SRLidx].ty[KIndex];
                Error = (dx * (Y - SRL[SRLidx].ty[Index]) - dy * (X - SRL[SRLidx].tx[Index])) / Mag(dx, dy);
            }

            double Prevdx = SRL[SRLidx].tx[KNext] - SRL[SRLidx].tx[Prev];
            double Prevdy = SRL[SRLidx].ty[KNext] - SRL[SRLidx].ty[Prev];
            double Nextdx = SRL[SRLidx].tx[NextNext] - SRL[SRLidx].tx[KIndex];
            double Nextdy = SRL[SRLidx].ty[NextNext] - SRL[SRLidx].ty[KIndex];
            double dx = c0 * Prevdx + (1 - c0) * Nextdx;
            double dy = c0 * Prevdy + (1 - c0) * Nextdy;
            double n = Mag(dx, dy);
            dx /= n;
            dy /= n;
            double sError = (dx * car->_speed_Y - dy * car->_speed_X) / (carspeed + 0.01);
            double cError = (dx * car->_speed_X + dy * car->_speed_Y) / (carspeed + 0.01);
            VnError = asin(sError);
            if (cError < 0)
                VnError = PI - VnError;
            NORM_PI_PI(VnError);

            k1999steer -= (atan(Error * (300 / (carspeed + 300)) / 15) + VnError) / car->_steerLock;
            //fprintf(stderr," d %.3f",k1999steer);

            //
            // Steer into the skid
            //
            double vx = car->_speed_X;
            double vy = car->_speed_Y;
            double dirx = cos(car->_yaw);
            double diry = sin(car->_yaw);
            //Unused code? (kilo)
            //CosAngleError = dx * dirx + dy * diry;
            SinAngleError = dx * diry - dy * dirx;
            Skid = (dirx * vy - vx * diry) / (carspeed + 1.0);
            if (Skid > 0.9)
                Skid = 0.9;
            if (Skid < -0.9)
                Skid = -0.9;
            double skidfactor = 1.0 - MIN(0.6, carspeed/120);

            double sc = GetModD( tSkidCorrection, This );
            if (sc <= 0.0)
            {
                sc = SkidCorrection + 1.0;

                // if sc not explicitly stated, we reduce it for braking areas where car is turning
                if (car->_accel_x < 0.0)
                    sc = MAX(1.0, sc - fabs(SRL[SRLidx].tRInverse[Next]) * 40);
            }
            else
                sc += 1.0;

            //if (data->collision)
            // sc += (5.0 - data->collision) / 3;

            //if (data->followdist < 10.0 && data->followdist > 0.0)
            // sc += (10.0 - data->followdist) / 8;

            k1999steer += (asin(Skid*sc) / car->_steerLock) * skidfactor;
            //fprintf(stderr," e %.3f",k1999steer);

            if (SteerSkid > 0.0)
            {
                double yr = carspeed * TargetCurvature;
                double diff = (car->_yaw_rate*(data->mode == mode_normal ? 1.0 : 1.2)) - yr;
                double skidaccel = GetModD( tSkidAccel, This );
                if (!skidaccel) skidaccel = SkidAccel;
                double steerskid = (car->_accel_x <= 0.0 ? SteerSkid : SteerSkid * ((1.0 - MIN(0.3, car->_accel_x/15)) * skidaccel));
                if (data->mode != mode_normal || data->aligned_time < 15)
                    steerskid = MAX(steerskid, 0.25 - (data->mode != mode_normal ? 0.0 : data->aligned_time/70));

                steerskid = ((steerskid * (1 + fDirt) * (100 / (carspeed + 100)) * diff) / car->_steerLock);
                k1999steer -= steerskid;//MAX(-speedskid, MIN(speedskid, steerskid));
            }
            //fprintf(stderr," f %.3f",k1999steer);

            if (data->alone)
                if (fabs(k1999steer) < 0.35 && fabs(SRL[SRLidx].tRInverse[Next]) > 0.002)
                    k1999steer *= (1.0 + (0.35 - fabs(k1999steer)) * 1.3);
            //fprintf(stderr," g %.3f\n",k1999steer);  fflush(stderr);

            if (fabs(car->_yaw_rate) >= 4.0 && carspeed > 10.0)
            {
                data->speed = data->avspeed = TargetSpeed = 0.0;
            }

#if 0
            {
                if (fabs(data->angle) > 1.0)
                {
                    if ((data->angle > 0.0 && k1999steer > 0.0) || (data->angle < 0.0 && k1999steer < 0.0))
                        k1999steer = -k1999steer;
                }
                if (fabs(data->angle) > 1.6)
                {
                    //k1999steer = -k1999steer;

                    // we're facing the wrong way.  Set it to full steer in whatever direction for now ...
                    if (k1999steer > 0.0)
                        k1999steer = 1.0;
                    else
                        k1999steer = -1.0;
                }
            }
#endif
        }

        if (fabs(SRL[SRLidx].tRInverse[Next]) > MinCornerInverse &&
                fabs(k1999steer) > 0.2 &&
                ((SinAngleError > 0.0 && SRL[SRLidx].tRInverse[Next] > 0.0 && data->speedangle < 0.0) ||
                 (SinAngleError < 0.0 && SRL[SRLidx].tRInverse[Next] < 0.0 && data->speedangle > 0.0)))
        {
            data->accel_redux = fabs(SinAngleError) * ((fabs(k1999steer)-0.2)*0.7) +
                    (fabs(SRL[SRLidx].tRInverse[Next]) - MinCornerInverse) * 1500 * fabs(data->speedangle);
        }
    }


    if (RaceLineDebug)
    {
        GfLogDebug("USR %s: %d/%d RI=%.3f/%.3f z=%.3f (%.3f/%.3f) Fr=%.3f Speed=%.1f (%.1f/%.1f) Steer=%.3f (redux=%.3f)\n", car->_name, This, Next, SRL[SRLidx].tRInverse[Next], SRL[SRLidx].tRInverse[Next], SRL[SRLidx].tz[Next], SRL[SRLidx].tzd[Next], SegCamber(SRLidx, Next), SRL[SRLidx].tFriction[Next], TargetSpeed, tSpeed[LINE_RL][Next], tSpeed[LINE_MID][Next], k1999steer, data->accel_redux);
    }

    data->ksteer = k1999steer;
#endif

} 

double LRaceLine::getAvoidSteer(double offset, LRaceLineData *data)
{
    //GfLogDebug("USR Offset = %f\n", offset);
    double speedoffset = AvoidOffset *((car->_speed_x *3.6)/ 100);
    if (speedoffset < 1.0f)
        speedoffset = 1.0f;
    else if (speedoffset > AvoidOffset)
        speedoffset = AvoidOffset ;

    offset *= speedoffset;
    //offset *=AvoidOffset;
    double steer = 0.0;

    vec2f target;
    double carspeed = Mag(car->_speed_X, car->_speed_Y);
    //GfLogDebug("USR Car Speed = %f\n", car->_speed_x * 3.6);
    double steertime = MIN(MaxSteerTime, MinSteerTime + MAX(0.0, carspeed-20.0)/30.0);
    if (data->followdist < 5.0)
        steertime = MIN(MaxSteerTime*1.1, steertime * 1.0 + (5.0 - data->followdist)/20);
    //double lane2left = track->width * SRL[SRLidx].tLane[Next];

    double amI = MIN(0.05, MAX(-0.05, SRL[SRLidx].tRInverse[Next]));
    double famI = fabs(amI);
    double time_mod = 1.0;

    if (famI > 0.0)
    {
        double toMid = car->_trkPos.toMiddle + data->speedangle * 20;
        double toLeft = car->_trkPos.toLeft - data->speedangle * 20;
        double toRight = car->_trkPos.toRight + data->speedangle * 20;
        double modfactor = (car->_speed_x / data->avspeed);
        modfactor *= modfactor;

        if (amI > 0.0)
        {
            if (toMid < 0.0)
                time_mod += famI * (MIN(track->width/2, fabs(toMid)) / track->width) * 50;
            else if (toLeft < track->width/3)
                time_mod -= MIN(0.7, famI * (MIN(track->width/3, MAX(0.0, track->width/3-toLeft)) / track->width) * 40 * modfactor);
        }
        else
        {
            if (toMid > 0.0)
                time_mod += famI * (MIN(track->width/2, fabs(toMid)) / track->width) * 50;
            else if (toRight < track->width/3)
                time_mod -= MIN(0.7, famI * (MIN(track->width/3, MAX(0.0, track->width/3-toRight)) / track->width) * 40 * modfactor);
        }
    }

    // an idea to implement soon...
    //if (car->_accel_x < 0.0)
    // steertime *= MIN(1.4, MAX(1.0, -car->_accel_x/10));

    GetSteerPoint(5.0 + car->_speed_x/10, &target, offset, steertime * time_mod);
    double targetAngle = atan2(target.y - car->_pos_Y, target.x - car->_pos_X);
    //double offline = MIN(2.0, MAX(-2.0, (offset - car->_trkPos.toMiddle)));
    //targetAngle -= offline/15;

    double steer_direction = targetAngle - (car->_yaw + car->_yaw_rate/15); //(15-MIN(8, car->_speed_x/10)));
    NORM_PI_PI(steer_direction);

    steer = steer_direction / car->_steerLock;

    double nextangle = data->angle + car->_yaw_rate/4;

    if (fabs(nextangle) > fabs(data->speedangle))
    {
        //double sa = MAX(-0.3, MIN(0.3, data->speedangle/3));
        double anglediff = (data->speedangle - nextangle) * (0.1 + fabs(nextangle)/6);
        steer += anglediff * (1.0 + MAX(1.0, 1.0 - car->_accel_x/5));
    }

    return steer;
} 

int LRaceLine::isOnLine()
{
    double lane2left = SRL[SRLidx].tLane[This] * SRL[SRLidx].Width;

    if (fabs(car->_trkPos.toLeft - lane2left) < MAX(0.06, 1.0 - (car->_speed_x * (car->_speed_x/10))/600))
        return 1;

    return 0;
}

void LRaceLine::GetPoint( double offset, vec2f *rt, double *mInverse )
{
    // TODO - merge point with where car's headed?
    double offlane = ((track->width/2) - offset) / track->width;
    double off2lft = track->width/2 - offset;
    double off2rgt = track->width - off2lft;
    //tTrackSeg *seg = car->_trkPos.seg;
    int SegId = car->_trkPos.seg->id;
    double dist = car->_trkPos.toStart;
    if (dist < 0)
        dist = 0;
    if (car->_trkPos.seg->type != TR_STR)
        dist *= car->_trkPos.seg->radius;
    int Index = SRL[SRLidx].tSegIndex[SegId] + int(dist / SRL[SRLidx].tElemLength[SegId]);
    //double laneoffset = SRL[SRLidx].Width/2 - (SRL[SRLidx].tLane[Index] * SRL[SRLidx].Width);
    double rInv = SRL[LINE_MID].tRInverse[Index];
    Index = This;
#if 1
    if (fabs(SRL[SRLidx].tRInverse[Next]) > fabs(rInv) &&
            ((SRL[SRLidx].tRInverse[Next] < 0 && rInv <= 0.0005) ||
             (SRL[SRLidx].tRInverse[Next] > 0 && rInv >= -0.0005)))
        rInv = SRL[SRLidx].tRInverse[Next];
#endif

    //double time = 0.63;
    double time = 0.02 * 10 * (1.0 + MIN(15.0*(1.0+fabs(rInv*240)), MAX(0.0, (car->_speed_x-(40*(1.0-MIN(0.8, fabs(rInv*70)))))))/18);

    if (rInv > 0.0 && off2lft > 0.0)
        time *= (1.0 + ((off2lft/track->width) * (off2lft/(track->width-3.0)) * fabs(rInv*60)));
    else if (rInv < 0.0 && off2rgt > 0.0)
        time *= (1.0 + ((off2rgt/track->width) * (off2rgt/(track->width-3.0)) * fabs(rInv*60)));
    double X = car->_pos_X + car->_speed_X * time;
    double Y = car->_pos_Y + car->_speed_Y * time;
    int next = Index;
    double avgmInverse = 0.0;
    int divcount = 0;

    double txIndex = offlane * SRL[SRLidx].txRight[Index] + (1 - offlane) * SRL[SRLidx].txLeft[Index];
    double tyIndex = offlane * SRL[SRLidx].tyRight[Index] + (1 - offlane) * SRL[SRLidx].tyLeft[Index];
    double txNext = offlane * SRL[SRLidx].txRight[next] + (1 - offlane) * SRL[SRLidx].txLeft[next];
    double tyNext = offlane * SRL[SRLidx].tyRight[next] + (1 - offlane) * SRL[SRLidx].tyLeft[next];
    int count = 0, limit = MAX(30, int(car->_speed_x*2));
    while(count < limit)
    {
        next = (Index + 1) % Divs;
        txNext = offlane * SRL[SRLidx].txRight[next] + (1 - offlane) * SRL[SRLidx].txLeft[next];
        tyNext = offlane * SRL[SRLidx].tyRight[next] + (1 - offlane) * SRL[SRLidx].tyLeft[next];
        //double dx = txNext - car->_pos_X;
        //double dy = tyNext - car->_pos_Y;
        if ((txNext - txIndex) * (X - txNext) +
                (tyNext - tyIndex) * (Y - tyNext) < -0.1)
        {
            //txNext += (X - txNext)*0.4;
            //tyNext += (Y - tyNext)*0.4;
            break;
        }
        Index = next;
        txIndex = offlane * SRL[SRLidx].txRight[Index] + (1 - offlane) * SRL[SRLidx].txLeft[Index];
        tyIndex = offlane * SRL[SRLidx].tyRight[Index] + (1 - offlane) * SRL[SRLidx].tyLeft[Index];
        if (next >= Next)
        {
            avgmInverse += SRL[SRLidx].tRInverse[Index] * MAX(0.0, 1.0 - (double)divcount/15);
            divcount++;
        }
        count++;
    }

    if (rt)
    {
        rt->x = (tdble)txNext;
        rt->y = (tdble)tyNext;
    }

    if (mInverse)
        *mInverse = avgmInverse;
}

int LRaceLine::findNextCorner( double *nextCRinverse )
{
    //tTrackSeg *seg = car->_trkPos.seg;;
    int prefer_side = ((SRL[SRLidx].tRInverse[Next] > 0.001) ? TR_LFT :
                                                               ((SRL[SRLidx].tRInverse[Next]) < -0.001 ? TR_RGT : TR_STR));
    //double curlane = car->_trkPos.toLeft / track->width;
    int /*next = (Next+5) % Divs,*/ i = 1, div;
    //double distance = 0.0;
    double CR = SRL[SRLidx].tRInverse[Next];

    if (car->_speed_x < 5.0)
    {
        prefer_side = TR_STR;
    }

    if (fabs(CR) < 0.01)
    {
        int iend = MIN(250, (int) car->_speed_x*3);
        for (i=1; i<iend; i++)
        {
            div = (Next + i) % Divs;
            if (SRL[SRLidx].tRInverse[div] > 0.001)
                prefer_side = TR_LFT;
            else if (SRL[SRLidx].tRInverse[div] < -0.001)
                prefer_side = TR_RGT;
            if (prefer_side != TR_STR)
            {
                double thisCR = SRL[SRLidx].tRInverse[div];
                double distance = SRL[SRLidx].tDistance[div] - SRL[SRLidx].tDistance[This];
                if (distance < 0.0)
                    distance = (SRL[SRLidx].tDistance[div]+SRL[SRLidx].Length) - SRL[SRLidx].tDistance[This];
                double time2reach = distance / car->_speed_x;
                thisCR /= MAX(1.0, time2reach*2);
                if (fabs(thisCR) > fabs(CR))
                    CR = thisCR;
                if (fabs(CR) >= 0.01)
                    break;
            }
        }
    }

    *nextCRinverse = CR;

    if (prefer_side == TR_STR)
        *nextCRinverse = 0.0;

    return prefer_side;
}

double LRaceLine::correctLimit(double avoidsteer, double racesteer, int insideline)
{
    //double nlane2left = SRL[SRLidx].tLane[Next] * SRL[SRLidx].Width;
    double tbump = BumpCaution;//GetModD( tBump, This ) * 4;
    //double factor = (insideline ? 50 : 200);

    double limit = 0.04 - MIN(0.039, MAX(20.0, 100.0 - car->_speed_x) / 3000);

    // correct would take us in the opposite direction to a corner - correct less!
    if ((SRL[SRLidx].tRInverse[Next] > 0.001 && avoidsteer > racesteer) ||
            (SRL[SRLidx].tRInverse[Next] < -0.001 && avoidsteer < racesteer))
    {
        limit = MAX(0.001, MIN(limit, limit - (fabs(SRL[SRLidx].tRInverse[Next]) * 200.0 + tbump)));
    }
    else
    {
        // correct would take us in the opposite direction to a corner - correct less (but not as much as above)
        int nnext = (Next + (int) (car->_speed_x/3)) % Divs;
        //double nnlane2left = SRL[SRLidx].tLane[nnext] * SRL[SRLidx].Width;
        if ((SRL[SRLidx].tRInverse[nnext] > 0.001 && avoidsteer > racesteer) ||
                (SRL[SRLidx].tRInverse[nnext] < -0.001 && avoidsteer < racesteer))
            limit = MAX(0.001, MIN(limit, limit - (fabs(SRL[SRLidx].tRInverse[nnext]) * 140.0 + tbump)));
    }

    if ((avoidsteer > racesteer && car->_yaw_rate < 0.0) || (avoidsteer < racesteer && car->_yaw_rate > 0.0))
    {
        // avoid oversteering back towards raceline
        limit = MAX(0.001, limit - (fabs(car->_yaw_rate) * car->_speed_x) / 100.0);
    }

    // ok, we're not inside the racing line.  Check and see if we're outside it and turning
    // into a corner, in which case we want to correct more to try and get closer to the
    // apex.
    //if (tSpeed[LINE_RL][Next] < tSpeed[LINE_RL][This] &&
    //    ((tRInverse[LINE_RL][Next] > 0.001 && tLane[Next] < tLane[This] && car->_trkPos.toLeft > nlane2left + 2.0) ||
    //     (tRInverse[LINE_RL][Next] < -0.001 && tLane[Next] > tLane[This] && car->_trkPos.toLeft < nlane2left - 2.0)))
    // return MAX(1.0, MIN(2.5, (1.0 + fabs(tRInverse[LINE_RL][Next])*2) - tbump));
    //else

    // return MAX(0.5, 1.0 - MAX(fabs(data->rlangle), fabs(data->rInverse*70)));

    return limit;
}

double LRaceLine::getAvoidSpeedDiff( float distance )
{
    int i;
    double speed_diff = 5.0;

    int count = 0;
    int maxcount = (int) (distance/DivLength);
    for (i=Next; count < maxcount; i++)
    {
        LRaceLineData data;
        data.speed = 0.0;
        data.avspeed = 0.0;
        int index = ((i-1)+Divs) % Divs;

        data.speed = (tSpeed[LINE_RL][index] + tSpeed[LINE_RL][i]) / 2;
        CalcAvoidSpeed( i, &data, 0.0 );

        if (data.speed < 9999.0)
            speed_diff = MAX(speed_diff, MIN(data.speed*0.2, (data.speed - data.avspeed) * MAX(0.0, 1.0 - ((double) count / MIN(40.0, (double)maxcount)))));

        count++;
        i = (i % Divs);
    }

    return speed_diff;
} 

void LRaceLine::CalcAvoidSpeed( int next, LRaceLineData *data, double angle )
{
#if 0
    int index = This;
    int nnext = ((next+int(MAX(0.0, car->_speed_x/3)))) % Divs;
    int movingout = (tSpeed[LINE_RL][next] > tSpeed[LINE_RL][index] || (tRInverse[LINE_RL][next] > 0.0 ? (tLane[next] > tLane[index]) : (tLane[index] > tLane[next])));
    int onapex = (tSpeed[LINE_RL][next] >= tSpeed[LINE_RL][index] || (tRInverse[LINE_RL][next] > 0.0 ? (tLane[next] >= tLane[index]) : (tLane[index] >= tLane[next])));
    double clane = (car->_trkPos.toLeft / SRL[SRLidx].Width);
    double rgtrldiff = ((tLane[next] - clane)) / 6;
    double lftrldiff = ((clane - tLane[next])) / 6;
    double nLeft = car->_trkPos.toLeft - (angle*10 + (tRInverse[LINE_MID][nnext]*100 - car->_yaw_rate) * fabs(tRInverse[LINE_MID][next]*300));
    double nRight = car->_trkPos.toRight + (angle*10 - (tRInverse[LINE_MID][nnext]*100 - car->_yaw_rate) * fabs(tRInverse[LINE_MID][next]*300));
    double nMiddle = car->_trkPos.toMiddle + (angle*10 - (tRInverse[LINE_MID][nnext]*100 - car->_yaw_rate) * fabs(tRInverse[LINE_MID][next]*300));

    data->slowavspeed = (tSpeed[LINE_MID][index] + tSpeed[LINE_MID][next]) / 2;
    if (!data->avspeed)
        data->avspeed = data->slowavspeed;
    else
        data->slowavspeed = data->avspeed;
    if (!data->speed)
        data->speed = (tSpeed[LINE_RL][index] + tSpeed[LINE_RL][next]) / 2;
    return;

#if 0
    if (tRInverse[LINE_MID][nnext] > 0.001)
    {
        if (nMiddle > 0.0)
            // slow speed according to distance from middle & rInv of corner
            data->slowavspeed *= MAX(0.6, 1.0 - fabs((tRInverse[LINE_MID][nnext]*5)*(1.0-MAX(0.0, nMiddle/(track->width/2)))));
        else
            // increase speed
            data->slowavspeed *= MIN(1.4, 1.0 + fabs((tRInverse[LINE_MID][nnext]*2)*(1.0-MIN(0.0, (fabs(nMiddle))/(track->width/2)))) * (data->closing ? 1.0 : 1.0));
    }
    else if (tRInverse[LINE_MID][nnext] < -0.001)
    {
        if (nMiddle < 0.0)
            // slow speed according to distance from middle & rInv of corner
            data->slowavspeed *= MAX(0.6, 1.0 - fabs(fabs(tRInverse[LINE_MID][nnext]*5)*(1.0-(fabs(nMiddle)/(track->width/2)))));
        else
            // increase speed
            data->slowavspeed *= MIN(1.4, 1.0 + fabs(fabs(tRInverse[LINE_MID][nnext]*2)*(1.0-((nMiddle)/(track->width/2)))) * (data->closing ? 1.0 : 1.0));
    }
    {
        double laneoffset = SRL[SRLidx].Width/2 - (tLane[nnext] * SRL[SRLidx].Width);

        if ((tRInverse[LINE_RL][nnext] > 0.001 && nLeft >= tLane[nnext] * SRL[SRLidx].Width + 2.0) ||
                (tRInverse[LINE_RL][nnext] < -0.001 && nLeft <= tLane[nnext] * SRL[SRLidx].Width - 2.0))
        {
            // outside the raceline
            if ((tRInverse[LINE_RL][nnext] > 0.001 && /*angle > -lftrldiff &&*/ nMiddle < MIN(-1.0, laneoffset)) ||
                    (tRInverse[LINE_RL][nnext] < -0.001 && /*angle < rgtrldiff &&*/ nMiddle > MAX(1.0, laneoffset)))
            {
                // poor speedangle, so slow the car down
                double cfactor = 7.0;
                data->slowavspeed *= 1.0 - MAX(0.0, MIN(0.3, ((fabs(nMiddle)-1.0)/(SRL[SRLidx].Width/2)) * 6 * fabs(tRInverse[LINE_RL][nnext]) * cfactor));
            }
        }
        else if (tSpeed[LINE_RL][nnext] < tSpeed[LINE_RL][index])
        {
            // inside raceline and slowing down
            data->slowavspeed *= 1.0 - MIN(0.2, fabs(tRInverse[LINE_RL][nnext])*130);
        }

#if 0
        if (data->slowavspeed < data->speed &&
                data->slowavspeed > 0.0 &&
                nnext == Next &&
                fabs(laststeer-k1999steer) < 0.3 && fabs(nMiddle-laneoffset) < 5.0)
        {
            // the closer we are to the raceline, both steering and distancewise, the more we increase
            // speed back to raceline levels.
            double factor = MAX(fabs(laststeer-k1999steer)/0.3, fabs(nMiddle-laneoffset)/5.0);
            data->slowavspeed = data->speed - (data->speed-data->slowavspeed) * factor;
        }

        data->slowavspeed = MAX(data->slowavspeed, data->speed*0.8);
#endif
    }
#endif
#if 1
    data->insideline = 0;


    if ((tRInverse[LINE_RL][next] > 0.0 && movingout && nLeft <= tLane[next] * SRL[SRLidx].Width + 1.0) ||
            (tRInverse[LINE_RL][next] < 0.0 && movingout && nLeft >= tLane[next] * Width - 1.0))
    {
        // raceline speeding up and we're inside it, so speed car up too.
        data->avspeed = MAX(data->speed + fabs(nLeft-(tLane[next]*Width))/5, data->avspeed);
#if 1
        if ((tRInverse[LINE_RL][next] > 0.0 && angle > -nRight/40) ||
                (tRInverse[LINE_RL][next] < -0.0 && angle < nLeft/40))
        {
            //avspeed *= 1.0 + MIN(1.3, (fabs(nMiddle)-2.0) * fabs(tRInverse[LINE_RL][next]) * 8);
            data->avspeed *= 1.0 + MIN(1.3, fabs(angle)*2.5);
            data->insideline = 1;
        }
#endif
    }
    else if (movingout && data->avspeed > 0.0 &&
             ((tRInverse[LINE_MID][next] > 0.0 && data->speedangle > -(nRight/track->width)/6) ||
              (tRInverse[LINE_MID][next] < 0.0 && data->speedangle < (nLeft/track->width)/6)))
    {
        // past the apex and our speedangle's acceptable - full speed ahead!
        if (tRInverse[LINE_MID][next] > 0.0)
            data->avspeed = MAX(data->avspeed, MIN(data->speed, data->avspeed + fabs(-(nRight/track->width)/4 - angle)*3));
        else
            data->avspeed = MAX(data->avspeed, MIN(data->speed, data->avspeed + fabs((nRight/track->width)/4 - angle)*3));
        //data->avspeed = data->speed;
    }
#if 0
    else if (onapex && data->avspeed > 0.0 &&
             ((tRInverse[LINE_RL][next] > 0.0 && angle > -(nRight/track->width)/7) ||
              (tRInverse[LINE_RL][next] < 0.0 && angle < (nLeft/track->width)/7)))
    {
        // on the apex and our speedangle's acceptable - full speed ahead!
        data->avspeed = data->speed;
    }
#endif
    else
        data->avspeed = data->slowavspeed;
#endif

    data->slowavspeed = MIN(data->slowavspeed, data->avspeed);

    return;
#endif
}

/////////////////////////////////////////////////////////////////////////////
// Find discrete position for interpolation context
/////////////////////////////////////////////////////////////////////////////
void LRaceLine::CI_Update(double Dist)
{
#if 0
    interpdata.i1 = This;
    interpdata.i0 = (interpdata.i1 + Divs - 1) % Divs;
    interpdata.i2 = (interpdata.i1 + 1) % Divs;
    interpdata.i3 = (interpdata.i1 + 2) % Divs;

    interpdata.d0 = tDistance[interpdata.i0];
    interpdata.d1 = tDistance[interpdata.i1];
    interpdata.d2 = tDistance[interpdata.i2];
    interpdata.d3 = tDistance[interpdata.i3];

    if (interpdata.d0 > interpdata.d1)
        interpdata.d0 -= track->length;
    if (interpdata.d1 > Dist)
    {
        interpdata.d0 -= track->length;
        interpdata.d1 -= track->length;
    }
    if (Dist > interpdata.d2)
    {
        interpdata.d2 += track->length;
        interpdata.d3 += track->length;
    }
    if (interpdata.d2 > interpdata.d3)
        interpdata.d3 += track->length;

    interpdata.t = (Dist - interpdata.d1) / (interpdata.d2 - interpdata.d1);
    interpdata.a0 = (1 - interpdata.t) * (1 - interpdata.t) * (1 - interpdata.t);
    interpdata.a1 = 3 * (1 - interpdata.t) * (1 - interpdata.t) * interpdata.t;
    interpdata.a2 = 3 * (1 - interpdata.t) * interpdata.t * interpdata.t;
    interpdata.a3 = interpdata.t * interpdata.t * interpdata.t;
#endif
}

/////////////////////////////////////////////////////////////////////////////
// Interpolate between points with a line
/////////////////////////////////////////////////////////////////////////////
double LRaceLine::LinearInterpolation(const double *pd) const
{
    return pd[interpdata.i1] * (1 - interpdata.t) + pd[interpdata.i2] * interpdata.t;
}

/////////////////////////////////////////////////////////////////////////////
// Interpolate between points with a cubic spline
/////////////////////////////////////////////////////////////////////////////
double LRaceLine::CubicInterpolation(const double *pd) const
{
    double x0 = pd[interpdata.i1];
    double x3 = pd[interpdata.i2];
    double der1 = (pd[interpdata.i2] - pd[interpdata.i0]) * (interpdata.d2 - interpdata.d1) / (interpdata.d2 - interpdata.d0);
    double der2 = (pd[interpdata.i3] - pd[interpdata.i1]) * (interpdata.d2 - interpdata.d1) / (interpdata.d3 - interpdata.d1);
    double x1 = x0 + der1 / 3;
    double x2 = x3 - der2 / 3;

    return x0 * interpdata.a0 + x1 * interpdata.a1 + x2 * interpdata.a2 + x3 * interpdata.a3;
}
