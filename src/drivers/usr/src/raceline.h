/****************************************************************************

    file                 : raceline.h
    created              : Wed Mai 14 19:53:00 CET 2003
    copyright            : (C) 2003-2004 by Bernhard Wymann
    email                : berniw@bluewin.ch
    version              : $Id: raceline.h 3636 2011-05-31 04:36:35Z andrewsumner $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _RACELINE_H_
#define _RACELINE_H_

#include "linalg.h"
#include "mod.h"
#include "globaldefs.h"

enum { LINE_MID=0, LINE_RL };
enum { mode_normal=1, mode_correcting, mode_avoiding, mode_pitting };

#define MAXSEGMENTS 3000
#define MAXDIVS 10000

typedef struct
{
    double *tRInverse;
    double *tx;
    double *ty;
    double *tz;
    double *tzd;
    double *tLane;
    double *txLeft;
    double *tyLeft;
    double *txRight;
    double *tyRight;
    double *tLaneLMargin;
    double *tLaneRMargin;
    double *tFriction;
    double *tBrakeFriction;
    //double *tSegDist;
    double *tElemLength;
    double *tDistance;
    double *ExtLimit;
    tTrackSeg **tSegment;
    int *tDivSeg;
    int *tSegIndex;
    char trackname[64];
    double Width;
    double Length;
    int Segs;
    int init;
    int offset;
} SRaceLine;

typedef struct
{
    tSituation *s;
    double rInverse;
    double mInverse;
    double aInverse;
    double amInverse;
    double decel;
    double adecel;
    double lane;
    double ksteer;
    double tsteer;
    double collision;
    double speedangle;
    double angle;
    double speed;
    double tspeed;
    double avspeed;
    double slowavspeed;
    double accel_redux;
    double overtakecaution;
    double offset;
    double lookahead;
    double steer;
    double NSsteer;
    double NSasteer;
    double laststeer;
    double braking;
    double rlangle;
    double followdist;
    double speedchange;
    double aligned_time;
    int thisdiv;
    int nextdiv;
    int mode;
    int avoidmode;
    int closing;
    int exiting;
    int alone;
    int outsideline;
    int insideline;
    v2d target;
} LRaceLineData;

typedef struct
{
    int i0;
    int i1;
    int i2;
    int i3;
    double d0;
    double d1;
    double d2;
    double d3;
    double t;
    double a0;
    double a1;
    double a2;
    double a3;
} InterpData;

class LRaceLine
{
public:
    LRaceLine();

    void setMinCornerInverse( double wi ) { MinCornerInverse = wi; }
    void setCornerSpeed( double wi ) { CornerSpeed = wi; }
    void setCornerAccel( double wi ) { CornerAccel = wi; }
    void setBrakeDelay( double wi ) { BrakeDelay = wi; }
    void setIntMargin( double wi ) { IntMargin = wi; }
    void setExtMargin( double wi ) { ExtMargin = wi; }
    void setAvoidSpeedAdjust( double wi ) { AvoidSpeedAdjust = wi; }
    void setCarHandle( void *pCarHandle ) { carhandle = pCarHandle; }
    void setSkill( double tskill) { skill = tskill; }
    void setCW( double cw ) { CW = cw; }
    int getCarefulBrake() { return GetModI( tCarefulBrake, Next ); }
    double getRLAngle(int div);

    double MinCornerInverse;
    double IncCornerInverse;
    double IncCornerFactor;
    double BaseCornerSpeed;
    double BaseCornerSpeedX;
    double DefaultCornerSpeedX;
    double CornerSpeed;
    double CornerSpeedX;
    double CornerAccel;
    double BrakeDelay;
    double BrakeDelayX;
    double BrakeMod;
    double BrakePower;
    double IntMargin;
    double ExtMargin;
    double AvoidSpeedAdjust;
    double AvoidSpeedAdjustX;
    double AvoidBrakeAdjust;
    double AvoidBrakeAdjustX;
    double CurveFactor;
    double SecurityZ;
    double MaxSteerTime;
    double MinSteerTime;
    double TargetSpeed;
    double ATargetSpeed;
    double SteerGain;
    double SteerSkid;
    double SkidAccel;
    double DivLength;
    double AccelCurveDampen;   //
    double BrakeCurveDampen;
    double AccelCurveLimit;
    double BrakeCurveLimit;
    double AccelExit;          //
    double AvoidAccelExit;     //
    double OvertakeCaution;    // default 0.0 - higher increases caution in overtaking
    double SkidCorrection;     // default 1.0.  Higher corrects steer errors faster & reduces wobble
    double SteerRIAcc;
    double SteerRIAccC;
    double BumpCaution;
    double SlopeFactor;
    double ExitBoost;
    double ExitBoostX;
    double AvoidExitBoost;
    double AvoidExitBoostX;
    double AvoidOffset;
    bool RaceLineDebug;

    double CW;
    double wheelbase;
    double wheeltrack;
    double k1999steer;
    double laststeer;
    double lastNksteer;
    double lastNasteer;
    double skill;
    double lastyaw;
    double maxfuel;
    double deltaTime;
    double avgerror;

    int Divs;
    int AccelCurveOffset;
    int Iterations;
    int SteerMod;
    int SRLidx;
    int OfftrackAllowed;

    double roughlimit;

    double **tSpeed;
    double *tLaneShift;
    int *tDivSeg;

    LRLMod *tRLMarginRgt;
    LRLMod *tRLMarginLft;
    LRLMod *tOTCaution;
    LRLMod *tRLSpeed;
    LRLMod *tRLBrake;
    LRLMod *tIntMargin;
    LRLMod *tExtMargin;
    LRLMod *tSecurity;
    LRLMod *tDecel;
    LRLMod *tADecel;
    LRLMod *tSpeedLimit;
    LRLMod *tCornerAccel;
    LRLMod *tAccelCurveDampen;
    LRLMod *tCurveFactor;
    LRLMod *tAvoidSpeed;
    LRLMod *tAvoidSpeedX;
    LRLMod *tAvoidBrake;
    LRLMod *tAvoidBrakeX;
    LRLMod *tAccelCurveOffset;
    LRLMod *tCarefulBrake;
    LRLMod *tSkidAccel;
    LRLMod *tAccelExit;
    LRLMod *tSkidCorrection;
    LRLMod *tBumpCaution;
    LRLMod *tBrakeCurve;
    LRaceLineData *data;

    int fDirt;
    int Next;
    int This;
    int CarDiv;
    tTrack *track;

    void *carhandle;
    tCarElt *car;

    void UpdateTxTy(int i, int rl);
    void SetSegmentInfo(const tTrackSeg *pseg, double d, int i, double l, int rl);
    void AllocTrack(tTrack *ptrack);
    void AllocRaceline(int rl, const char *trackname);
    void FreeRaceline(int rl);
    void FreeTrack(bool freeall);
    void SplitTrack(tTrack *ptrack, int rl);
    double SegCamber(int rl, int div);
    double GetRInverse(int prev, double x, double y, int next, int rl);
    void AdjustRadius(int prev, int i, int next, double TargetRInverse, int rl, double Security = -1);
    void Smooth(int Step, int rl);
    void ComputeSpeed(int rl);
    void StepInterpolate(int iMin, int iMax, int Step, int rl);
    void Interpolate(int Step, int rl);
    void CalcZCurvature(int rl);
    void TrackInit(tSituation *p);
    void InitTrack(tTrack* ptrack, tSituation *p);
    void CalcAvoidSpeed( int next, LRaceLineData *data, double angle );
    int findNextCorner( double *nextCRinverse );
    void NewRace(tCarElt* newcar, tSituation *s);
    void GetRaceLineData(tSituation *s, LRaceLineData *data);
    void GetPoint( double offset, vec2f *rt, double *mInverse );
    void GetSteerPoint( double lookahead, vec2f *rt, double offset = -100.0, double time = -1.0 );
    void GetRLSteerPoint( vec2f *rt, double *offset, double time );
    int isOnLine();
    double correctLimit(double avoidsteer, double racesteer, int insideline);
    double getAvoidSpeedDiff( float distance );
    double getK1999Steer() { return k1999steer; }
    double getRInverse(int div);
    double getRInverse() { return getRInverse(Next); }
    void getOpponentInfo(double distance, int rl, double *aspeed, double *rInv, double offset = -1000.0);
    double getRLMarginRgt(int divadvance) { int div=(Next+divadvance)%Divs; return GetModD( tRLMarginRgt, div ); }
    double getRLMarginLft(int divadvance) { int div=(Next+divadvance)%Divs; return GetModD( tRLMarginLft, div ); }
    double getAvoidSteer(double offset, LRaceLineData *data);
    void NoAvoidSteer() { lastNasteer = lastNksteer; }
    double calcAvoidSpeed( double offset, double rInv, double speed, double rlspeed );


    // interpolation...
    void CI_Update(double dist);
    double CubicInterpolation(const double *pd) const;
    double LinearInterpolation(const double *pd) const;
    InterpData interpdata;
};

#endif 
