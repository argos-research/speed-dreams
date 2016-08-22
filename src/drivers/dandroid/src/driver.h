/***************************************************************************

    file                 : driver.h
    created              : 2006-08-31 01:21:49 UTC
    copyright            : (C) Daniel Schellhammer

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _DRIVER_H_
#define _DRIVER_H_

#include "torcs_or_sd.h"
#include "globaldefinitions.h"

#include <fstream>
#include "opponent.h"
#include "pit.h"
#include "danpath.h"
#include "pidcontroller.h"


class PathInfo {
  public:
  DanPoint carpos;
  DanPoint tarpos;
  double maxspeed;
  double offset;
};


class TDriver {
  public:
  TDriver(int index);
  ~TDriver();

  const char* MyBotName;                      // Name of this bot 

  void InitTrack(PTrack Track, PCarHandle CarHandle, PCarSettings *CarParmHandle, PSituation Situation);
  void NewRace(PtCarElt Car, PSituation Situation);
  void Drive();
  int PitCmd();
  void EndRace();
  void Shutdown();

  private:
  // Utility functions
  void updateTime();
  void updateTimer();
  void updateBasics();
  void updateOpponents();
  void updatePath();
  void updateUtils();
  void setControls();
  void printChangedVars();
  void setPrevVars();
  void initVars();
  double getPitSpeed();
  double getMaxSpeed(DanPoint danpoint);
  double curveSpeed(double radius);
  double bumpSpeed(double curv_z);
  double brakeSpeed(double nextdist, double nextspeed);
  double brakeDist(double speed, double allowedspeed);
  double getBrake(double maxspeed);
  double getAccel(double maxspeed);
  double getSteer();
  int getGear();
  double getClutch();
  bool stateStuck();
  bool stateOfftrack();
  bool statePitstop();
  bool statePitlane();
  void setDrvState(int state);
  double pathOffs(int path);
  void setDrvPath(int path);
  void calcDrvState();
  void calcTargetToMiddle();
  bool overtakeOpponent();
  int overtakeStrategy();
  void updateStuck();
  bool onCollision();
  double brakeDistToOpp(Opponent* opp);
  bool oppInCollisionZone(Opponent* opp);
  bool oppOnMyLine(Opponent* opp);
  double diffSpeedMargin(Opponent* opp);
  bool oppNoDanger(Opponent* opp);
  double fromStart(double fromstart);
  void updateSector();
  void learnSpeedFactors();
  bool offtrack();
  bool equalSpeedFactors();
  bool allSectorsFaster();
  int nextLearnSector(int sect);
  void increaseSpeedFactor(int sect, double inc);
  void getSpeedFactors();
  void updatePathCar(int line);
  void updatePathTarget(int line);
  void updatePathOffset(int line);
  void updatePathSpeed(int line);
  void updateCurveAhead();
  void updateDrivingFast();
  void updateCatchedRaceLine();
  void updateFrontCollFactor();
  void updateLetPass();
  void calcMaxspeed();
  void calcTarget();
  void limitSteerAngle(double& targetangle);
  void calcGlobalTarget();
  void calcTargetAngle();
  double filterABS(double brake);
  double filterTCL(double accel);
  double filterTCL_FWD();
  double filterTCL_RWD();
  double filterTCLSideSlip(double accel);
  void initCa();
  void readSpecs();
  void readPrivateSection();
  void printSetup();
  void controlSpeed(double& accelerator, double speed);
  void updateAttackAngle();
  bool controlAttackAngle(double& targetangle);
  void controlOffset(double& targetangle);
  void controlYawRate(double& targetangle);
  bool hysteresis(bool lastout, double in, double hyst);
  double getFuel(double dist);
  void saveFile();
  void saveSectorSpeeds();
  bool readSectorSpeeds();
  void driverMsg(std::string desc);
  void driverMsgValue(int priority, std::string desc, double value);

  // Per robot global data
  int mDrvPath;
  int prev_mDrvPath;
  enum {PATH_O, PATH_L, PATH_R}; // States for mDrvPath
  int mDrvState;
  int prev_mDrvState;
  enum {STATE_RACE, STATE_STUCK, STATE_OFFTRACK, STATE_PITLANE, STATE_PITSTOP}; // States for mDrvState

  tSituation* oSituation;
  tCarElt* oCar;        // pointer to tCarElt struct
  double oCurrSimTime;
  
  PTrack mTrack;
  int mCarIndex;
  std::string mCarType;
  DanPath mDanPath;
  Opponents mOpponents;  // the container for opponents
  Opponent* mOpp;         // relevant opponent for calculations
  Opponent* mOppNear;
  Opponent* mOppNear2;
  Opponent* mOppBack;
  Opponent* mOppLetPass;
  double mOppDist;
  double mOppSidedist;
  bool mOppAside;
  bool mOppLeft;
  bool mOppLeftHyst;
  bool mOppLeftOfMe;
  bool mOppLeftOfMeHyst;
  bool mOppInFrontspace;
  bool mBackmarkerInFrontOfTeammate;
  bool mTwoOppsAside;
  bool mOppComingFastBehind;
  bool prev_mOppComingFastBehind;
  bool mLearning;
  bool mTestpitstop;
  int mTestLine;
  int mDriverMsgLevel;
  int mDriverMsgCarIndex;
  Pit mPit;
  double mTankvol;
  double mFuelPerMeter;
  double mMu;    // friction coefficient
  double mMass;  // mass of car + fuel
  double mSpeed;
  double mClutchtime;
  int mPrevgear;
  bool mControlAttackAngle;
  bool prev_mControlAttackAngle;
  double mAttackAngle;
  bool mControlYawRate;
  bool prev_mControlYawRate;
  bool mBumpSpeed;
  bool prev_mBumpSpeed;
  double mSectorTime;
  double mOldTimer;
  bool mTenthTimer;
  int mShiftTimer;
  int mGear;
  bool mStuck;
  int mStuckcount;
  bool mStateChange;
  bool mPathChange;
  bool mOvertake;
  bool prev_mOvertake;
  int mOvertakeTimer;
  bool mLetPass;
  bool prev_mLetPass;
  bool mLeavePit;
  double mFriction;
  double mCentrifugal;
  double mBrakeFriction;
  double mBrakeforce;
  double mBorderdist;
  bool mOnLeftSide;
  int mTrackType;
  double mTrackRadius;
  bool mOnCurveInside;
  double mAngleToTrack;
  bool mAngleToLeft;
  bool mPointingToWall;
  double mWallToMiddleAbs;
  double mWalldist;
  int mLastDamage;
  int mDamageDiff;
  int mPrevRacePos;
  int mRacePosChange;
  double mAccel;
  double mAccelAvg;
  double mAccelAvgSum;
  int mAccelAvgCount;
  double mMaxspeed;
  std::vector <DanSector> mSect;
  int mSector;
  int prev_mSector;
  double mSectSpeedfactor;
  PathInfo mPath[3];
  bool mCurveAhead;
  bool prev_mCurveAhead;
  double mCurveAheadFromStart;
  bool mDrivingFast;
  bool prev_mDrivingFast;
  int mDrivingFastCount;
  bool mCatchingOpp;
  bool mLearnSectTime;
  bool mGetLapTime;
  double mLastLapTime;
  double mBestLapTime;
  bool mLearnLap;
  bool mAllSectorsFaster;
  bool mLearnSingleSector;
  int mLearnSector;
  bool mLearnedAll;
  bool mOfftrackInSector;
  bool mFinalLearnLap;
  double mFromStart;
  double mToMiddle;
  double mTargetFromstart;
  double mTargetToMiddle;
  double mNormalTargetToMiddle;
  double mPrevTargetdiff;
  double mTargetAngle;
  bool mMaxSteerAngle;
  bool prev_mMaxSteerAngle;
  Vec2d mGlobalCarPos;
  Vec2d mGlobalTarget;
  bool mCatchedRaceLine;
  bool prev_mCatchedRaceLine;
  double mCatchedRaceLineTime;
  double mAbsFactor;
  double mTclFactor;
  double mFrontCollFactor;
  bool mColl;
  bool mWait;
  double mFuelStart;
  double mPathOffs;
  double mAccelX;
  double mAccelXSum;
  int mAccelXCount;
  double mSkillGlobal;
  double mSkillDriver;
  PidController mSpeedController;
  PidController mAttackAngleController;
  PidController mOffsetController;
  int mWatchdogCount;
  // Data that should stay constant after first initialization
  double mFRONTWINGANGLE;
  double mREARWINGANGLE;
  double mWHEELBASE;  // wheelbase of the car
  double mCARMASS;    // mass of the car only
  double mCA;         // aerodynamic downforce coefficient
  double mBRAKEPRESS;
  double mBRAKEDECEL;
  double mBRAKEFORCEFACTOR;
  double mBUMPSPEEDFACTOR;
  double mFUELWEIGHTFACTOR;
  int mPITDAMAGE;
  double mPITENTRYMARGIN;
  double mPITENTRYSPEED;
  double mPITEXITSPEED;
  double mSPEEDFACTOR;
  double mTARGETFACTOR;
  double mTARGETWALLDIST;
  bool mTRACTIONCONTROL;
  double mMAXLEFT;
  double mMAXRIGHT;
  double mMARGIN;
  double mCLOTHFACTOR;
  double mSEGLEN;
  // Class constants
  double mLOOKAHEAD_CONST;
  double mFRONTCOLL_MARGIN;
  double mOVT_FRONTSPACE;
  double mOVT_FRONTMARGIN;
};

#endif // _DRIVER_H_
