/***************************************************************************

    file                 : grboard.cpp
    created              : Thu Aug 17 23:52:20 CEST 2000
    copyright            : (C) 2000 by Eric Espie
    email                : torcs@free.fr
    version              : $Id: grboard.cpp 5320 2013-03-15 00:24:04Z kmetykog $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "grboard.h"

#include <plib/ssg.h>
#include <portability.h>    // snprintf
#include <robottools.h>     // RELAXATION

#include <algorithm>        // remove
#include <sstream>

#include "grmain.h"       // grWinX, grHandle, grMaxDamage
#include "grtrackmap.h"   // cGrTrackMap
#include "grcar.h"        // grCarInfo
#include "grutil.h"       // grWriteTime
#include "grloadac.h"     // grssgSetCurrentOptions
#include "grscreen.h"

using std::string;

static const string rgba[4] =
    { GFSCR_ATTR_RED, GFSCR_ATTR_GREEN, GFSCR_ATTR_BLUE, GFSCR_ATTR_ALPHA };

static const int NB_BOARDS = 3;
static const int NB_LBOARDS = 5;    // # of leaderboard states
static const int NB_DEBUG = 4;

// Boards work on a OrthoCam with fixed height of 600, width flows
// with split screen(s) and can be limited to 'board width' % of screen
static const int TOP_ANCHOR = 600;
static const int BOTTOM_ANCHOR = 0;
static const int DEFAULT_WIDTH = 800;

static const int BUFSIZE = 256;


cGrBoard::cGrBoard(int myid) :
    normal_color_(NULL), danger_color_(NULL), ok_color_(NULL),
    error_color_(NULL), inactive_color_(NULL), emphasized_color_(NULL),
    ahead_color_(NULL), behind_color_(NULL), arcade_color_(NULL),
    background_color_(NULL)
{
  id = myid;
  trackMap = NULL;

  // Scrolling leaderboard variables
  iStart = 0;
  iTimer = 0.0;
  iStringStart = 0;
}


cGrBoard::~cGrBoard()
{
  delete trackMap;
  trackMap = 0;

  delete [] normal_color_;
  delete [] danger_color_;
  delete [] ok_color_;
  delete [] error_color_;
  delete [] inactive_color_;
  delete [] emphasized_color_;
  delete [] ahead_color_;
  delete [] behind_color_;
  delete [] arcade_color_;
  delete [] background_color_;
}


void
cGrBoard::loadDefaults(const tCarElt *curCar)
{
  //Load dash colours from graph.xml
  char path[1024];
  snprintf(path, sizeof(path), "%s%s", GfLocalDir(), GR_PARAM_FILE);
  void *hdle = GfParmReadFile(path, GFPARM_RMODE_STD | GFPARM_RMODE_CREAT);
  const char *pszSpanSplit;

  ReadDashColor(hdle, GFSCR_ELT_NORMALCLR,      &normal_color_);
  ReadDashColor(hdle, GFSCR_ELT_DANGERCLR,      &danger_color_);
  ReadDashColor(hdle, GFSCR_ELT_OKCLR,          &ok_color_);
  ReadDashColor(hdle, GFSCR_ELT_ERRORCLR,       &error_color_);
  ReadDashColor(hdle, GFSCR_ELT_INACTIVECLR,    &inactive_color_);
  ReadDashColor(hdle, GFSCR_ELT_EMPHASIZEDCLR,  &emphasized_color_);
  ReadDashColor(hdle, GFSCR_ELT_AHEADCLR,       &ahead_color_);
  ReadDashColor(hdle, GFSCR_ELT_BEHINDCLR,      &behind_color_);
  ReadDashColor(hdle, GFSCR_ELT_ARCADECLR,      &arcade_color_);
  ReadDashColor(hdle, GFSCR_ELT_BACKGROUNDCLR,  &background_color_);

  GfParmReleaseHandle(hdle);

  //Load other dash properties
  snprintf(path, sizeof(path), "%s/%d", GR_SCT_DISPMODE, id);

  debugFlag = (int)GfParmGetNum(grHandle, path, GR_ATT_DEBUG, NULL, 1);
  boardFlag = (int)GfParmGetNum(grHandle, path, GR_ATT_BOARD, NULL, 2);
  leaderFlag  = (int)GfParmGetNum(grHandle, path, GR_ATT_LEADER, NULL, 1);
  leaderNb  = (int)GfParmGetNum(grHandle, path, GR_ATT_NBLEADER, NULL, 10);
  counterFlag = (int)GfParmGetNum(grHandle, path, GR_ATT_COUNTER, NULL, 1);
  GFlag = (int)GfParmGetNum(grHandle, path, GR_ATT_GGRAPH, NULL, 1);
  arcadeFlag  = (int)GfParmGetNum(grHandle, path, GR_ATT_ARCADE, NULL, 0);
  boardWidth  = (int)GfParmGetNum(grHandle, path, GR_ATT_BOARDWIDTH, NULL, 100);
  speedoRise  = (int)GfParmGetNum(grHandle, path, GR_ATT_SPEEDORISE, NULL, 0);

  trackMap->setViewMode((int)GfParmGetNum(grHandle, path, GR_ATT_MAP,
                                NULL, trackMap->getDefaultViewMode()));

  // Only apply driver preferences when not spanning split screens
  pszSpanSplit = GfParmGetStr(grHandle, GR_SCT_GRAPHIC, GR_ATT_SPANSPLIT, GR_VAL_NO);
  if (strcmp(pszSpanSplit, GR_VAL_YES) && curCar->_driverType == RM_DRV_HUMAN) {
    snprintf(path, sizeof(path), "%s/%s", GR_SCT_DISPMODE, curCar->_name);
    debugFlag = (int)GfParmGetNum(grHandle, path, GR_ATT_DEBUG, NULL, debugFlag);
    boardFlag = (int)GfParmGetNum(grHandle, path, GR_ATT_BOARD, NULL, boardFlag);
    leaderFlag  = (int)GfParmGetNum(grHandle, path, GR_ATT_LEADER, NULL, leaderFlag);
    leaderNb  = (int)GfParmGetNum(grHandle, path, GR_ATT_NBLEADER, NULL, leaderNb);
    counterFlag   = (int)GfParmGetNum(grHandle, path, GR_ATT_COUNTER, NULL, counterFlag);
    GFlag   = (int)GfParmGetNum(grHandle, path, GR_ATT_GGRAPH, NULL, GFlag);
    arcadeFlag  = (int)GfParmGetNum(grHandle, path, GR_ATT_ARCADE, NULL, arcadeFlag);
    boardWidth  = (int)GfParmGetNum(grHandle, path, GR_ATT_BOARDWIDTH, NULL, boardWidth);
    speedoRise  = (int)GfParmGetNum(grHandle, path, GR_ATT_SPEEDORISE, NULL, speedoRise);
    trackMap->setViewMode((int)GfParmGetNum(grHandle, path, GR_ATT_MAP,
                                NULL, trackMap->getViewMode()));
  }

  if (boardWidth < 0 || boardWidth > 100)
    boardWidth = 100;
  this->setWidth(DEFAULT_WIDTH);

  if (speedoRise < 0 || speedoRise > 100)
    speedoRise = 0;
}


void
cGrBoard::setWidth(int val)
{
  // Setup the margins according to percentage of screen width
  centerAnchor = (val / 2);
  leftAnchor = (val / 2) - val * boardWidth / 200;
  rightAnchor = (val / 2) + val * boardWidth / 200;
}

void
cGrBoard::selectBoard(int val)
{
  char path[1024];
  snprintf(path, sizeof(path), "%s/%d", GR_SCT_DISPMODE, id);

  switch (val) {
    case 0:
      boardFlag = (boardFlag + 1) % NB_BOARDS;
      GfParmSetNum(grHandle, path, GR_ATT_BOARD, (char*)NULL, (tdble)boardFlag);
      break;
    case 1:
      counterFlag = (counterFlag + 1) % NB_BOARDS;
      GfParmSetNum(grHandle, path, GR_ATT_COUNTER, (char*)NULL, (tdble)counterFlag);
      break;
    case 2:
      leaderFlag = (leaderFlag + 1) % NB_LBOARDS;
      GfParmSetNum(grHandle, path, GR_ATT_LEADER, (char*)NULL, (tdble)leaderFlag);
      break;
    case 3:
      debugFlag = (debugFlag + 1) % NB_DEBUG;
      GfParmSetNum(grHandle, path, GR_ATT_DEBUG, (char*)NULL, (tdble)debugFlag);
      break;
    case 4:
      GFlag = 1 - GFlag;
      GfParmSetNum(grHandle, path, GR_ATT_GGRAPH, (char*)NULL, (tdble)GFlag);
      break;
    case 5:
      arcadeFlag = 1 - arcadeFlag;
      GfParmSetNum(grHandle, path, GR_ATT_ARCADE, (char*)NULL, (tdble)arcadeFlag);
      break;
  }
  GfParmWriteFile(NULL, grHandle, "graph");
}


/**
 * grDispDebug
 *
 * Displays debug information in the top right corner.
 * It is a 3-state display, states as follows:
 * 0 - Don't display any info
 * 1 - Display the mean and instant FPS
 * 2 - Like 2 + the absolute frame counter
 * 3 - Like 2 + the segment the car is on, car's distance from startline, current camera
 *
 * @param s[in]     The current situation
 * @param frame[in] Frame info to display
 */
void
cGrBoard::grDispDebug(const tSituation *s, const cGrFrameInfo* frame)
{
  char buf[BUFSIZE];
  snprintf(buf, sizeof(buf), "FPS: %.1f(%.1f)  ",
            frame->fInstFps, frame->fAvgFps);
  int dx = GfuiFontWidth(GFUI_FONT_SMALL_C, buf);

  int x2 = rightAnchor - dx;    // 2nd column
  int y = TOP_ANCHOR - 15;
  int dy = GfuiFontHeight(GFUI_FONT_SMALL_C);
  int x = (debugFlag > 1) ? x2 - dx : x2;   // 1st column

  // Display frame rates (instant and average)
  snprintf(buf, sizeof(buf), "FPS: %.1f(%.1f)",
            frame->fInstFps, frame->fAvgFps);
  GfuiDrawString(buf, normal_color_, GFUI_FONT_SMALL_C, x, y);

  if (debugFlag == 2) {
    //Only display detailed information in Debug Mode > 1
    // Display frame counter in 2nd column
    snprintf(buf, sizeof(buf),  "Frm: %u", frame->nTotalFrames);
    GfuiDrawString(buf, normal_color_, GFUI_FONT_SMALL_C, x2, y);

    // Display current fovy in 2nd row, 1st column
    y -= dy;
    cGrScreen *curScreen = grGetCurrentScreen();
    if (curScreen) {
      cGrCamera *curCam = curScreen->getCurCamera();
      if (curCam) {
        snprintf(buf, sizeof(buf), "FovY: %2.1f", curCam->getFovY());
        GfuiDrawString(buf, normal_color_, GFUI_FONT_SMALL_C, x, y);
      }
    }

    // Display simulation time in 2nd row, 2nd column
    snprintf(buf, sizeof(buf),  "Time: %.f", s->currentTime);
    GfuiDrawString(buf, normal_color_, GFUI_FONT_SMALL_C, x2, y);
  } else if (debugFlag == 3) {
    // Only display detailed information in Debug Mode > 1
    // Display segment name in 2nd column
    snprintf(buf, sizeof(buf),  "Seg: %s", car_->_trkPos.seg->name);
    GfuiDrawString(buf, normal_color_, GFUI_FONT_SMALL_C, x2, y);

    // Display distance from start in 2nd row, 1st column
    y -= dy;
    snprintf(buf, sizeof(buf), "DfS: %5.0f", car_->_distFromStartLine);
    GfuiDrawString(buf, normal_color_, GFUI_FONT_SMALL_C, x, y);

    // Display current camera in 2nd row, 2nd column
    tRoadCam *curCam = car_->_trkPos.seg->cam;
    if (curCam) {
      snprintf(buf, sizeof(buf), "Cam: %s", curCam->name);
      GfuiDrawString(buf, normal_color_, GFUI_FONT_SMALL_C, x2, y);
    }
  }
}  // grDispDebug


void
cGrBoard::grDispGGraph()
{
  // Position the graph
  const tdble X1 = (tdble)(rightAnchor - 100);
  const tdble Y1 = (tdble)(BOTTOM_ANCHOR + 70);
  const tdble XC = (tdble)(rightAnchor - 30);
  const tdble YC = (tdble)(Y1 - 50);

  // Draw the static blue thin cross and vertical segment.
  glBegin(GL_LINES);
  glColor4fv(ahead_color_);
  glVertex2f(X1 - 50, Y1);
  glVertex2f(X1 + 50, Y1);
  glVertex2f(X1, Y1 - 50);
  glVertex2f(X1, Y1 + 50);
  glVertex2f(XC, YC);
  glVertex2f(XC, YC + 100);
  glEnd();

  // Draw the throttle gauge (vertical thick segment, starting in X1,Y1,
  // going upwards, length proportional to the throttle command).
  static const tdble THNSS = 2.0f;

  glBegin(GL_QUADS);
  glColor4fv(behind_color_);

  // red if at least one wheel is slipping, blue otherwise).
  // a) Detect wheel slip, and change current color to red if so.
  for (int xx = 0; xx < 4; ++xx) {
    if (fabs(car_->_speed_x)
        - fabs(car_->_wheelSpinVel(xx) * car_->_wheelRadius(xx)) < -5.0f) {
      glColor4fv(danger_color_);
      break;
    }
  }
  glVertex2f(X1 - THNSS, Y1);
  glVertex2f(X1 + THNSS, Y1);
  glVertex2f(X1 + THNSS, Y1 + car_->ctrl.accelCmd * 50.0f);
  glVertex2f(X1 - THNSS, Y1 + car_->ctrl.accelCmd * 50.0f);

  // Back to normal blue color.
  glColor4fv(behind_color_);

  // Draw the brake gauge (vertical thick segment, starting in X1,Y1,
  // going downward, length proportional to the brake command,
  // red if at least one wheel is blocking/blocked, blue otherwise).
  // a) Detect wheel blocking, and change current color to red if so.
  for (int xx = 0; xx < 4; ++xx) {
    if (fabs(car_->_speed_x)
        - fabs(car_->_wheelSpinVel(xx) * car_->_wheelRadius(xx)) > 5.0f) {
      glColor4fv(danger_color_);
      break;
    }
  }
  // b) Draw the gauge.
  glVertex2f(X1 - THNSS, Y1);
  glVertex2f(X1 + THNSS, Y1);
  glVertex2f(X1 + THNSS, Y1 - car_->ctrl.brakeCmd * 50.0f);
  glVertex2f(X1 - THNSS, Y1 - car_->ctrl.brakeCmd * 50.0f);

  // Back to normal blue color.
  glColor4fv(behind_color_);

  // Draw the steer gauge (horizontal thick segment, starting in X1,Y1,
  // going left or right according to the direction,
  // length proportional to the steer command).

  for (int xx = 0; xx < 2; ++xx) {
    if (fabs(car_->_wheelSlipSide(xx)) > 5.0) {
      glColor4fv(danger_color_);
      break;
    }
  }

  glVertex2f(X1, Y1 - THNSS);
  glVertex2f(X1, Y1 + THNSS);
  glVertex2f(X1 - car_->ctrl.steer * 50.0f, Y1 + THNSS);
  glVertex2f(X1 - car_->ctrl.steer * 50.0f, Y1 - THNSS);

  // Back to normal blue color.
  glColor4fv(behind_color_);


  // Draw the clutch gauge (vertical thick segment, starting in XC, YC,
  // going upwards, length proportional to the clutch command).
  glVertex2f(XC - THNSS, YC);
  glVertex2f(XC + THNSS, YC);
  glVertex2f(XC + THNSS, YC + car_->ctrl.clutchCmd * 100.0f);
  glVertex2f(XC - THNSS, YC + car_->ctrl.clutchCmd * 100.0f);

  glEnd();

  // Draw the acceleration gauge (thin segment, starting in X1, Y1,
  // going in the direction of the acceleration vector).
  const tdble X2 = -car_->_DynGC.acc.y / 9.81f * 25.0f + X1;
  const tdble Y2 = car_->_DynGC.acc.x / 9.81f * 25.0f + Y1;

  glBegin(GL_LINES);
  glColor4fv(emphasized_color_);
  glVertex2f(X1, Y1);
  glVertex2f(X2, Y2);
  glEnd();
}


void
cGrBoard::grDrawGauge(tdble X1, tdble Y1, tdble H,
                        float *color1, float *color2,
                        tdble val, const char *title)
{
  tdble curH = MAX(MIN(val, 1.0), 0.0);
  curH *= H;

  static const tdble THNSSBG = 2.0;
  static const tdble THNSSFG = 2.0;

  glBegin(GL_QUADS);
  // set F and D column to: 404040
  glColor4f(0.25, 0.25, 0.25, 0.8);
  glVertex2f(X1 - (THNSSBG + THNSSFG), Y1 - THNSSBG);
  glVertex2f(X1 + (THNSSBG + THNSSFG), Y1 - THNSSBG);
  glVertex2f(X1 + (THNSSBG + THNSSFG), Y1 + H + THNSSBG);
  glVertex2f(X1 - (THNSSBG + THNSSFG), Y1 + H + THNSSBG);

  glColor4fv(color2);
  glVertex2f(X1 - THNSSFG, Y1 + curH);
  glVertex2f(X1 + THNSSFG, Y1 + curH);
  glVertex2f(X1 + THNSSFG, Y1 + H);
  glVertex2f(X1 - THNSSFG, Y1 + H);

  glColor4fv(color1);
  glVertex2f(X1 - THNSSFG, Y1);
  glVertex2f(X1 + THNSSFG, Y1);
  glVertex2f(X1 + THNSSFG, Y1 + curH);
  glVertex2f(X1 - THNSSFG, Y1 + curH);
  glEnd();
  GfuiDrawString((char *)title, emphasized_color_, GFUI_FONT_MEDIUM, //yellow
                    (int)(X1 - (THNSSBG + THNSSFG)),
                    (int)(Y1 - THNSSBG - GfuiFontHeight(GFUI_FONT_MEDIUM)),
                    2*(THNSSBG + THNSSFG), GFUI_ALIGN_HC);
}

void
cGrBoard::grDispMisc(bool bCurrentScreen)
{
  if (bCurrentScreen)
  {
    // Draw the current screen indicator (only in split screen mode)
    // (a green-filled square on the bottom right corner of the screen).
    static const float h = 10.0f;
    const float w = h;

    const float x = rightAnchor - w - 5;
    const float y = BOTTOM_ANCHOR + 5;

    glBegin(GL_QUADS);
    glColor4f(0.0, 1.0, 0.0, 1.0);
    glVertex2f(x,     y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x,     y + h);
    glEnd();
  }
}


//
// grDispCarBoard1
//
// Displays driver-specific info (version 1)
//
// @param s[in] A pointer to the current situation
// @param car[in] A pointer to the current driver's car
// @return void
//
void
cGrBoard::grDispCarBoard1(const tSituation *s)
{
  char buf[BUFSIZE];

  // Populate buf to get the width of the drawing area
  snprintf(buf, sizeof(buf), "%s: %d/%d", car_->_name, car_->_pos, s->_ncars);

  int dy = GfuiFontHeight(GFUI_FONT_MEDIUM_C);
  static const int dy2 = GfuiFontHeight(GFUI_FONT_SMALL_C);
  int dx = GfuiFontWidth(GFUI_FONT_MEDIUM_C, buf);
  static const int dxc = 60;

  int x = leftAnchor + 15;                      // constant text left pos.
  int y = BOTTOM_ANCHOR + dy2 * 8 + dy + 5;     // first row top pos.
  // (counting from the bottom, we have 8+1 rows to display)

  const int x2 = x + 40;    // volatile text left pos.
  const int xr = x2 + dxc;
  dx = MAX(dx, (xr - x));

  // Display board
  // We have 8 rows with small font and 1 with middle
  grSetupDrawingArea(x - 5, y + dy + 5, x + dx + 5, y - dy2 * 8 - dy + 5);

  // Display driver name and race position (in medium font size)
  GfuiDrawString(buf, emphasized_color_, GFUI_FONT_MEDIUM_C, x, y);  // yellow
  y -= dy;

  // From now on we use small font
  dy = GfuiFontHeight(GFUI_FONT_SMALL_C);

  // Display fuel
  GfuiDrawString("Fuel:", normal_color_, GFUI_FONT_SMALL_C, x, y);
  float *color = (car_->_fuel < 5.0) ? danger_color_ : normal_color_;  // Display low fuel in red
  snprintf(buf, sizeof(buf), "%.1f l", car_->_fuel);
  GfuiDrawString(buf, color, GFUI_FONT_SMALL_C, x2, y, dxc, GFUI_ALIGN_HR);
  y -= dy;

  // Display damage
  color = (car_->_state & RM_CAR_STATE_BROKEN) ? danger_color_: normal_color_;    //red/white
  GfuiDrawString("Damage:", color, GFUI_FONT_SMALL_C, x, y);
  snprintf(buf, sizeof(buf),  "%d", car_->_dammage);
  GfuiDrawString(buf, color, GFUI_FONT_SMALL_C, x2, y, dxc, GFUI_ALIGN_HR);
  y -= dy;

  // Display lap counter
  char const* lapsTimeLabel;
  grGetLapsTime(s, buf, &lapsTimeLabel);
  GfuiDrawString(lapsTimeLabel, normal_color_, GFUI_FONT_SMALL_C, x, y);
  GfuiDrawString(buf, color, GFUI_FONT_SMALL_C, x2, y, dxc, GFUI_ALIGN_HR);
  y -= dy;

  // Display total race time
  GfuiDrawString("Total:", normal_color_, GFUI_FONT_SMALL_C, x, y);
  grWriteTime(color, GFUI_FONT_SMALL_C, x2, y, dxc, s->currentTime, 0);
  y -= dy;

  // Display current lap time
  GfuiDrawString("Curr:", normal_color_, GFUI_FONT_SMALL_C, x, y);
  grWriteTime(color, GFUI_FONT_SMALL_C, x2, y, dxc, car_->_curLapTime, 0);
  y -= dy;

  // Display last lap time
  GfuiDrawString("Last:", normal_color_, GFUI_FONT_SMALL_C, x, y);
  grWriteTime(color, GFUI_FONT_SMALL_C, x2, y, dxc, car_->_lastLapTime, 0);
  y -= dy;

  // Display best lap time
  GfuiDrawString("Best:", normal_color_, GFUI_FONT_SMALL_C, x, y);
  grWriteTime(color, GFUI_FONT_SMALL_C, x2, y, dxc, car_->_bestLapTime, 0);
  y -= dy;

  // Display top speed
  GfuiDrawString("Top Speed:", normal_color_, GFUI_FONT_SMALL_C, x, y);
  snprintf(buf, sizeof(buf), "%d", (int)(car_->_topSpeed * 3.6));
  GfuiDrawString(buf, normal_color_, GFUI_FONT_SMALL_C, x2, y, dxc, GFUI_ALIGN_HR);
  y -= dy;
}  // grDispCarBoard1


//
// grDispCarBoard2
//
// Displays driver-specific info (version 2)
//
// @param s[in] A pointer to the current situation
// @param car[in] A pointer to the current driver's car
// @return void
//
void
cGrBoard::grDispCarBoard2(const tSituation *s)
{
  // Font sizes
  int dy = GfuiFontHeight(GFUI_FONT_MEDIUM_C);
  static const int dy2 = GfuiFontHeight(GFUI_FONT_SMALL_C);
  static const int dxc = 60;

  const int x = leftAnchor + 15;    // constant text 1 left pos.
  const int x2 = x + 40;            // volatile text 1 left pos.
  const int x3 = x2 + dxc;          // constant text 2 left pos.
  const int xr = x3 + dxc;          // volatile text 2 left pos.
  int y = BOTTOM_ANCHOR + 8 * dy2 + dy + 5;      // first row top pos.
  // (counting from the bottom, we have 6+2 rows to display)

  // Populate buf to get the width of the drawing area
  char buf[BUFSIZE];
  snprintf(buf, sizeof(buf), "%s: %d/%d", car_->_name, car_->_pos, s->_ncars);
  const int dx = MAX(GfuiFontWidth(GFUI_FONT_MEDIUM_C, buf), (xr - x));

  // Display board
  // We have 8 rows with small font and 1 with middle
  grSetupDrawingArea(x - 5, y + dy + 5, x + dx + 5, y - dy2 * 8 - dy + 5);

  // Display driver name and race position (in medium font size)
  GfuiDrawString(buf, emphasized_color_, GFUI_FONT_MEDIUM_C, x, y);  //yellow
  y -= dy;

  // From now on we use small font
  dy = GfuiFontHeight(GFUI_FONT_SMALL_C);

  // Display fuel
  GfuiDrawString("Fuel:", normal_color_, GFUI_FONT_SMALL_C, x, y);
  float *color = (car_->_fuel < 5.0) ? danger_color_ : normal_color_;    //red/white
  snprintf(buf, sizeof(buf),  "%.1f l", car_->_fuel);
  GfuiDrawString(buf, color, GFUI_FONT_SMALL_C, x2, y, dxc, GFUI_ALIGN_HR);
  y -= dy;

  // Display lap counter
  char const *lapsTimeLabel;
  grGetLapsTime(s, buf, &lapsTimeLabel);
  GfuiDrawString(lapsTimeLabel, normal_color_, GFUI_FONT_SMALL_C, x, y);
  GfuiDrawString(buf, normal_color_, GFUI_FONT_SMALL_C, x2, y, dxc, GFUI_ALIGN_HR);
  y -= dy;

  // Display best lap time and diff of last and best lap
  GfuiDrawString("Best:", normal_color_, GFUI_FONT_SMALL_C, x, y);
  grWriteTime(normal_color_, GFUI_FONT_SMALL_C, x2, y, dxc, car_->_bestLapTime, 0);
  grWriteTime(normal_color_, GFUI_FONT_SMALL_C, x3, y, dxc, car_->_deltaBestLapTime, 1);
  y -= dy;

  // Display current lap time and split times
  GfuiDrawString("Time:", normal_color_, GFUI_FONT_SMALL_C, x, y);
  grWriteTime(normal_color_, GFUI_FONT_SMALL_C, x2, y, dxc, car_->_curLapTime, 0);
  double time;
  if (grGetSplitTime(s, false, time, NULL, &color))
    grWriteTime(color, GFUI_FONT_SMALL_C, x3, y, dxc, time, 1);
  y -= dy;
  y -= dy;

  // Display car ahead and diff
  color = ahead_color_;
  if (car_->_pos != 1) {
    snprintf(buf, sizeof(buf), "%s", s->cars[car_->_pos - 2]->_sname);
    GfuiDrawString(buf, color, GFUI_FONT_SMALL_C, x, y);
    if (s->_raceType == RM_TYPE_RACE) {
      if (s->cars[car_->_pos - 2]->_laps == car_->_laps) {
        grWriteTime(color, GFUI_FONT_SMALL_C, x3, y, dxc,
                        s->cars[car_->_pos - 2]->_curTime-car_->_curTime, 1);
      } else {
        GfuiDrawString("--:---", color, GFUI_FONT_SMALL_C, x3, y, dxc,
                        GFUI_ALIGN_HR);
      }
    } else {
      if (car_->_bestLapTime > 0.0f) {
        grWriteTime(color, GFUI_FONT_SMALL_C, x3, y, dxc,
                        car_->_bestLapTime - s->cars[car_->_pos - 2]->_bestLapTime, 1);
      } else {
        GfuiDrawString("--:---", color, GFUI_FONT_SMALL_C, x3, y, dxc,
                        GFUI_ALIGN_HR);
      }
    }
  } else {
    GfuiDrawString(" ", color, GFUI_FONT_SMALL_C, x, y);
    GfuiDrawString("--:---", color, GFUI_FONT_SMALL_C, x3, y, dxc, GFUI_ALIGN_HR);
  }
  y -= dy;

  // Display car behind and diff
  color = normal_color_;  //white
  if (car_->_pos != s->_ncars) {
    snprintf(buf, sizeof(buf), "%s", s->cars[car_->_pos]->_sname);
    GfuiDrawString(buf, color, GFUI_FONT_SMALL_C, x, y);
    if (s->_raceType == RM_TYPE_RACE) {
      if (s->cars[car_->_pos]->_laps == car_->_laps) {
        grWriteTime(color, GFUI_FONT_SMALL_C, x3, y, dxc,
                        s->cars[car_->_pos]->_curTime - car_->_curTime, 1);
      } else {
        GfuiDrawString("--:---", color, GFUI_FONT_SMALL_C, x3, y, dxc,
                        GFUI_ALIGN_HR);
      }
    } else {
      if (s->cars[car_->_pos]->_bestLapTime > 0.0f) {
        grWriteTime(color, GFUI_FONT_SMALL_C, x3, y, dxc,
                        s->cars[car_->_pos]->_bestLapTime - car_->_bestLapTime, 1);
      } else {
        GfuiDrawString("--:---", color, GFUI_FONT_SMALL_C, x3, y, dxc,
                        GFUI_ALIGN_HR);
      }
    }
  } else {
    GfuiDrawString(" ", color, GFUI_FONT_SMALL_C, x, y);
    GfuiDrawString("--:---", color, GFUI_FONT_SMALL_C, x3, y, dxc, GFUI_ALIGN_HR);
  }
  y -= dy;

  // Display control messages
  // (lines 3 and 4 only, ABS TCS SPD goes to separate indicator)
  for (int i = 2; i < 4; ++i) {
    if (car_->ctrl.msg[i]) {
      GfuiDrawString(car_->ctrl.msg[i], (float*)(car_->ctrl.msgColor),
                        GFUI_FONT_SMALL_C, x, y);
      y -= dy;
    }
  }
}  // grDispCarBoard2


void
cGrBoard::grDispCarBoard(const tSituation *s)
{
  switch (boardFlag) {
    case 0:
      break;

    case 1:
      grDispCarBoard1(s);
      if (true)
        grDispIndicators(false);
      break;

    case 2:
      grDispCarBoard2(s);
      if (true)
        grDispIndicators(false);
      break;

    default:
      break;
  }
}


#define ALIGN_CENTER  0
#define ALIGN_LEFT  1
#define ALIGN_RIGHT 2

void
cGrBoard::grDispEngineLeds(int X, int Y, int align, bool bg)
{
  // Green LED
  GLfloat ledcolg[2][3] = {
    {0.0, 0.2, 0.0},
    {0.0, 1.0, 0.0}
  };

  // Red LED
  GLfloat ledcolr[2][3] = {
    {0.2, 0.0, 0.0},
    {1.0, 0.0, 0.0}
  };

  int ledNb     = 20;
  int ledHeight = 10;
  int ledWidth  = 5;
  int ledSpace  = 2;
  int ledRed    = (int)((car_->_enginerpmRedLine * 0.9 / car_->_enginerpmMax) * (tdble)ledNb);
  int ledLit    = (int)((car_->_enginerpm / car_->_enginerpmMax) * (tdble)ledNb);

  int x;
  switch (align) {
    case ALIGN_CENTER:
      x = X - ((ledNb * ledWidth) + (ledNb - 1) * ledSpace) / 2;
      break;
    case ALIGN_LEFT:
      x = X;
      break;
    case ALIGN_RIGHT:
      x = X - ((ledNb * ledWidth) + (ledNb - 1) * ledSpace);
      break;
    default:
      x = X - ((ledNb * ledWidth) + (ledNb - 1) * ledSpace) / 2;
      break;
  }

  int y = Y;
  glBegin(GL_QUADS);

  // Draw background?
  if (bg) {
    glColor3f(0.1, 0.1, 0.1);
    glVertex2f(x - ledSpace, y + ledHeight + ledSpace);
    glVertex2f(x + ledNb * (ledWidth+ ledSpace), y + ledHeight + ledSpace);
    glVertex2f(x + ledNb * (ledWidth+ ledSpace), BOTTOM_ANCHOR);
    glVertex2f(x - ledSpace, BOTTOM_ANCHOR);
  }

  const int xref = x;
  glColor3fv(ledcolg[0]);
  for (int i = 0; i < ledRed; ++i) {
    glVertex2f(x, y);
    glVertex2f(x + ledWidth, y);
    glVertex2f(x + ledWidth, y + ledHeight);
    glVertex2f(x, y + ledHeight);
    x += ledWidth + ledSpace;
  }

  glColor3fv(ledcolr[0]);
  for (int i = ledRed; i < ledNb; ++i) {
    glVertex2f(x, y);
    glVertex2f(x + ledWidth, y);
    glVertex2f(x + ledWidth, y + ledHeight);
    glVertex2f(x, y + ledHeight);
    x += ledWidth + ledSpace;
  }
  x = xref;

#define DD  1
  glColor3fv(ledcolg[1]);
  for (int i = 0; i < ledNb; ++i) {
    if (i == ledRed) {
      glColor3fv(ledcolr[1]);
    }
    if (i <= ledLit) {
      glVertex2f(x + DD, y + DD);
      glVertex2f(x + ledWidth - DD, y + DD);
      glVertex2f(x + ledWidth - DD, y + ledHeight - DD);
      glVertex2f(x + DD, y + ledHeight - DD);
      x += ledWidth + ledSpace;
    } else {
      break;
    }
  }
  glEnd();
}  // grDispEngineLeds


/**
 * grDispLeaderBoard
 *
 * Displays the leader board (in the lower left corner of the screen)
 * If [drawLaps] is on, writes the lap counter on the top of the list.
 * @param s[in] situation provided by the sim
 */
void
cGrBoard::grDispLeaderBoard(const tSituation *s)
{
  double time_left;
  if (leaderFlag == 4) {
    // Scrolling line in the top of the screen
    grDispLeaderBoardScrollLine(s);
  } else if (leaderFlag == 3 && leaderNb < s->_ncars) {
    // This mode is only reasonable if there are more drivers
    // than the leaderboard can display at once.
    // Scrolling results in the top left corner
    grDispLeaderBoardScroll(s);
  } else {
    // Static leaderboard
    char buf[BUFSIZE];

    int current = 0;  // Position of the currently displayed car
    for (int i = 0; i < s->_ncars; ++i) {
      if (car_ == s->cars[i]) {
        current = i;
        break;
      }
    }   // for i

    // Coords, limits
    const int x = leftAnchor + 10;
    const int x2 = x + 100;
    const int dxc = 60;
    const int xr = x2 + dxc;

    const int dy = GfuiFontHeight(GFUI_FONT_SMALL_C);
    //Max # of lines to display (# of cars in race or max 10 by default)
    const int maxLines = MIN(leaderNb, s->_ncars);
    // Display # of laps on top of the list?
    const int drawLaps = MIN(1, leaderFlag - 1);

    int y = TOP_ANCHOR - 10;
    int y2 = y - 5 - dy * (maxLines + drawLaps);

    grSetupDrawingArea(x, y, xr + 5, y2);
    y = y2;

    // Display current car in last line (ie is its position >= 10)?
    bool drawCurrent = (current >= maxLines) ? true : false;

    // The board is built from bottom up:
    // driver position #10/current is drawn first,
    // then from #9 onto #1, then the text 'Laps' if drawLaps requires.
    for (int j = maxLines; j > 0; --j) {
      int i;  // index of driver to be displayed
      if (drawCurrent) {
        i = current;
        drawCurrent = 0;
      } else {
        i = j - 1;
      }  // if drawCurrent

      // Set colour of the drivers to that
      // defined in the drivers' XML file.
      // Current driver is yellow.
      float *color = (i == current)
        ? emphasized_color_ : grCarInfo[s->cars[i]->index].iconColor;    //yellow?

      if (i == current)
        color = emphasized_color_;
      else if (i < current)
        color = ahead_color_;
      else
        color = normal_color_;

      // Driver position + short name
      snprintf(buf, sizeof(buf), "%3d: %s", i + 1, s->cars[i]->_sname);
      GfuiDrawString(buf, color, GFUI_FONT_SMALL_C, x, y);

      // Display driver time / time behind leader / laps behind leader
      string sEntry = grGenerateLeaderBoardEntry(s->cars[i], s, (i == 0));
      if (s->cars[i]->_state & RM_CAR_STATE_DNF
        || s->cars[i]->_state & RM_CAR_STATE_PIT) {
        // driver DNF or in pit, show 'out' in red
        color = danger_color_;
      }
      GfuiDrawString(sEntry.c_str(), color, GFUI_FONT_SMALL_C, x2, y, dxc,
                        GFUI_ALIGN_HR);

      y += dy;  // 'Line feed'
    }   // for j

    // Write 'Lap X/Y' on top of the leader board
    if (drawLaps) {
      if (s->_raceType == RM_TYPE_RACE) {
        if (s->_totTime > s->currentTime) {
          GfuiDrawString(" Laps:", emphasized_color_, GFUI_FONT_SMALL_C, x, y);
          snprintf(buf, sizeof(buf), "%d", MAX(s->cars[0]->_laps-1, 0));
        } else {
          GfuiDrawString(" Lap:", emphasized_color_, GFUI_FONT_SMALL_C, x, y);  //yellow
          snprintf(buf, sizeof(buf), "%d / %d", s->cars[0]->_laps, s->_totLaps);
        }
        GfuiDrawString(buf, emphasized_color_, GFUI_FONT_SMALL_C, x2, y, dxc,
                        GFUI_ALIGN_HR);
      } else {
        if (s->_totTime > 0.0f) {
          time_left = MAX(MIN(s->_totTime, s->_totTime - s->currentTime), 0);
          GfuiDrawString(" Time left:", emphasized_color_, GFUI_FONT_SMALL_C, x, y);
          snprintf(buf, sizeof(buf), "%d:%02d:%02d",
                    (int)floor(time_left / 3600.0f),
                    (int)floor(time_left/60.0f) % 60,
                    (int)floor(time_left) % 60);
        } else {
          GfuiDrawString(" Lap:", emphasized_color_, GFUI_FONT_SMALL_C, x, y);
          snprintf(buf, sizeof(buf), "%d / %d", s->cars[0]->_laps, s->_totLaps);
        }
        GfuiDrawString(buf, emphasized_color_, GFUI_FONT_SMALL_C, x2, y, dxc,
                        GFUI_ALIGN_HR);
      }
    }   // if drawLaps
  }   // else
}   // grDispLeaderBoard


void
cGrBoard::grDispCounterBoard2()
{
  // RPM
  tgrCarInstrument *curInst = &(grCarInfo[car_->index].instrument[0]);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_TEXTURE_2D);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
  glTranslatef(centerAnchor,
                BOTTOM_ANCHOR + (speedoRise * TOP_ANCHOR / 100), 0);
  if (curInst->texture) {
    glBindTexture(GL_TEXTURE_2D, curInst->texture->getTextureHandle());
  }
  glCallList(curInst->CounterList);
  glBindTexture(GL_TEXTURE_2D, 0);

  tdble val = (*(curInst->monitored) - curInst->minValue) / curInst->maxValue;
  val = MIN(1.0, MAX(0.0, val));    // val between 0.0 and 1.0
  val = curInst->minAngle + val * curInst->maxAngle;

  RELAXATION(val, curInst->prevVal, 30);

  glPushMatrix();
  glTranslatef(curInst->needleXCenter, curInst->needleYCenter, 0);
  glRotatef(val, 0, 0, 1);
  glCallList(curInst->needleList);
  glPopMatrix();

  // Show gear
  char buf[32];
  if (car_->_gear <= 0)
    snprintf(buf, sizeof(buf), "%s", car_->_gear == 0 ? "N" : "R");
  else
    snprintf(buf, sizeof(buf), "%d", car_->_gear);
  GfuiDrawString(buf, curInst->needleColor, GFUI_FONT_LARGE_C,
                    (int)curInst->digitXCenter - 30,
                    (int)curInst->digitYCenter,
                    60, GFUI_ALIGN_HC);

  glTranslatef(-centerAnchor, -BOTTOM_ANCHOR, 0);

  // Speedo
  curInst = &(grCarInfo[car_->index].instrument[1]);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_TEXTURE_2D);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
  glTranslatef(centerAnchor, BOTTOM_ANCHOR, 0);
  if (curInst->texture) {
    glBindTexture(GL_TEXTURE_2D, curInst->texture->getTextureHandle());
  }
  glCallList(curInst->CounterList);
  glBindTexture(GL_TEXTURE_2D, 0);

  // Reverse speed should show on needle, too
  val = (*(curInst->monitored) - curInst->minValue) / curInst->maxValue;
  if (val < 0.0)
    val *= -1.0;
  val = MIN(1.0, val);
  val = curInst->minAngle + val * curInst->maxAngle;

  RELAXATION(val, curInst->prevVal, 30);

  glPushMatrix();
  glTranslatef(curInst->needleXCenter, curInst->needleYCenter, 0);
  glRotatef(val, 0, 0, 1);
  glCallList(curInst->needleList);
  glPopMatrix();

  // Digital speedo
  if (curInst->digital) {
    // Do not add "%3d" or something, because the digital font
    // DOES NOT SUPPORT BLANKS!!!!
    snprintf(buf, sizeof(buf), "%d", abs((int)(car_->_speed_x * 3.6)));
    GfuiDrawString(buf, curInst->needleColor, GFUI_FONT_LARGE_C,
                    (int)curInst->digitXCenter - 30,
                    (int)curInst->digitYCenter,
                    60, GFUI_ALIGN_HC);
  }

  glTranslatef(-centerAnchor, -BOTTOM_ANCHOR, 0);

  // Fuel and damage meter
  if (counterFlag == 1) {
    float *color;
    if (car_->_fuel < 5.0f) {
      color = danger_color_;    //red
    } else {
      color = emphasized_color_;    //yellow
    }

    grDrawGauge(centerAnchor + 140, BOTTOM_ANCHOR + 25, 100, color,
                background_color_, car_->_fuel / car_->_tank, "F");
    grDrawGauge(centerAnchor + 155, BOTTOM_ANCHOR + 25, 100, danger_color_, //red
                background_color_, (tdble)(car_->_dammage) / grMaxDammage, "D");
  }

  glTranslatef(0, -(speedoRise * TOP_ANCHOR / 100), 0);
}  // grDispCounterBoard2


void
cGrBoard::initBoard(void)
{
  if (!trackMap) {
    trackMap = new cGrTrackMap();
  }
}


void
cGrBoard::shutdown(void)
{
  delete trackMap;  // 'delete 0' is safe.
  trackMap = NULL;
}


void
cGrBoard::grDispArcade(const tSituation *s)
{
#define XM  15  // X margin
#define YM  10  // Y margin

  // We are ARCADE, we draw BIIIIG
  int dy = GfuiFontHeight(GFUI_FONT_BIG_C);
  const int dxc = 100;

  const int x = leftAnchor + XM;
  const int x2 = x + 50;
  const int width = rightAnchor - leftAnchor - 2 * XM;
  int y = TOP_ANCHOR - YM - dy;

  // Display driver name and race position
  char buf[BUFSIZE];
  snprintf(buf, sizeof(buf), "%d/%d", car_->_pos, s->_ncars);
  GfuiDrawString(buf, arcade_color_, GFUI_FONT_BIG_C, x, y);

  dy = GfuiFontHeight(GFUI_FONT_LARGE_C);
  y -= dy;

  // Display current lap time
  GfuiDrawString("Time:", arcade_color_, GFUI_FONT_LARGE_C, x, y);
  grWriteTime(arcade_color_, GFUI_FONT_LARGE_C, x2, y, dxc,
                car_->_curLapTime, 0);
  y -= dy;

  // Display best lap time
  GfuiDrawString("Best:", arcade_color_, GFUI_FONT_LARGE_C, x, y);
  grWriteTime(arcade_color_, GFUI_FONT_LARGE_C, x2, y, dxc,
                car_->_bestLapTime, 0);

  y = TOP_ANCHOR - YM - dy;
  grGetLapsTime (s, buf, NULL);
  GfuiDrawString(buf, arcade_color_, GFUI_FONT_LARGE_C, x, y,
                width, GFUI_ALIGN_HR);

  // Display driver name
  snprintf(buf, sizeof(buf), "%s", car_->_sname);
  GfuiDrawString(buf, arcade_color_, GFUI_FONT_LARGE_C, x, y,
                width, GFUI_ALIGN_HC);

  // Draw fuel/damage gauges
  float *color = (car_->_fuel < 5.0) ? danger_color_ : emphasized_color_;   //red/yellow
  grDrawGauge(leftAnchor + XM, BOTTOM_ANCHOR + 25, 100, color,
                background_color_, car_->_fuel / car_->_tank, "F");
  grDrawGauge(leftAnchor + XM + 15, BOTTOM_ANCHOR + 25, 100, danger_color_, //red
                background_color_, (tdble)(car_->_dammage) / grMaxDammage, "D");

  // Display ABS/TCS/SPD indicators
  grDispIndicators(true);

  // Display speed and gear
  dy = GfuiFontHeight(GFUI_FONT_LARGE_C);
  y = YM + dy;
  snprintf(buf, sizeof(buf), "%3d km/h", abs((int)(car_->_speed_x * 3.6)));
  GfuiDrawString(buf, arcade_color_, GFUI_FONT_BIG_C, x, y,
                width, GFUI_ALIGN_HR);
  y = YM;
  if (car_->_gear <= 0)
    snprintf(buf, sizeof(buf), "%s", car_->_gear == 0 ? "N" : "R");
  else
    snprintf(buf, sizeof(buf), "%d", car_->_gear);
  GfuiDrawString(buf, arcade_color_, GFUI_FONT_LARGE_C, x, y,
                width, GFUI_ALIGN_HR);

  // Display engine LED scale
  grDispEngineLeds(rightAnchor - XM, YM + dy + GfuiFontHeight (GFUI_FONT_BIG_C),
                ALIGN_RIGHT, false);
}  // grDispArcade


/**
 * This function calculates if the split time must be displayed, and if so what the
 * split time is.
 *
 * @param s[in] A pointer to the current situation
 * @param gap_inrace[in] True if it must display the gap during races, false if compares the current lap with the personal best lap
 * @param time[out] The split difference time
 * @param laps_different[out] Contains the number of laps behind / for at the split point
 * @param color[out] The colour which can be used to display the split time
 * @return true if there is a split time to be displayed, false otherwise
 */
bool cGrBoard::grGetSplitTime(const tSituation *s, bool gap_inrace, double &time,
                                int *laps_different, float **color)
{
  tdble curSplit;
  tdble bestSplit;
  tdble bestSessionSplit;
  const tCarElt *ocar = car_;
  const tCarElt *fcar = car_;
  int sign = 1;
  int laps;

  if (laps_different)
    *laps_different = 0;

  if (s->_raceType != RM_TYPE_RACE || s->_ncars == 1) {
    if (car_->_currentSector == 0)
      return false;

    curSplit = car_->_curSplitTime[car_->_currentSector - 1];
    bestSplit = car_->_bestSplitTime[car_->_currentSector - 1];

    if (car_->_curLapTime - curSplit > 5.0f)
      return false; /* Only display split for five seconds */

    if (s->_ncars > 1) {
      bestSessionSplit = s->cars[0]->_bestSplitTime[car_->_currentSector - 1];

      if (bestSessionSplit <= 0.0f)
        return false;

      time = curSplit - bestSessionSplit;
      if (time < 0.0f)
        *color = error_color_;  //pink
      else if (curSplit < bestSplit)
        *color = ok_color_;     //green
      else
        *color = normal_color_; //white
    } else {
      if (bestSplit < 0.0f)
        return false;

      time = curSplit - bestSplit;

      if (time < 0.0f)
        *color = ok_color_; //green
      else
        *color = normal_color_; //white
    }
  } else if (gap_inrace) {
    if (car_->_pos == 1) {
      fcar = s->cars[1];
      sign = -1;
    }

    ocar = s->cars[fcar->_pos-2];

    if (fcar->_currentSector == 0)
      return false;

    curSplit = fcar->_curSplitTime[fcar->_currentSector - 1];
    bestSplit = ocar->_curSplitTime[fcar->_currentSector - 1];

    if (fcar->_curLapTime - curSplit > 5.0f)
      return false;

    laps = ocar->_laps - fcar->_laps;
    if (ocar->_currentSector < fcar->_currentSector
        || (ocar->_currentSector == fcar->_currentSector
            && fcar->_curTime + curSplit < ocar->_curTime + bestSplit))
      --laps;

    if (!laps_different && laps != 0)
      return false;

    if (laps_different)
      *laps_different = sign * laps;

    time = ocar->_curTime + bestSplit - (fcar->_curTime + curSplit);
    if (sign < 0)
      time *= -1.0f;

    *color = normal_color_; //white
  } else {
    if (car_->_currentSector == 0)
      return false;

    curSplit = car_->_curSplitTime[car_->_currentSector - 1];
    bestSplit = car_->_bestSplitTime[car_->_currentSector - 1];

    if (bestSplit < 0.0f)
      return false;

    if (car_->_curLapTime - curSplit > 5.0f)
      return false;

    time = curSplit - bestSplit;
    if (time < 0.0f)
      *color = ok_color_;    //green
    else
      *color = normal_color_;   //white
  }

  return true;
}

/**
 * This function gives back the information about the remaining laps / time
 *
 * @param s[in] The current situation
 * @param car[in] The current car
 * @param result[out] An already existing string of len BUFSIZE which will contain the text
 * @param label[out] The label (Lap: or Time: ) If zero, then the label is added to @p result.
 */
void cGrBoard::grGetLapsTime(const tSituation *s, char* result,
                                char const **label) const
{
  bool time = true;
  double cur_left;
  char const *loc_label;

  // Don't show time data if race haven't started yet or is already finished
  if (s->_totTime < 0.0f
        || (s->_totTime < s->currentTime && s->_extraLaps > 0))
    time = false;

  if (label) {
    *label = time ? "Time: " : "Lap: ";
    loc_label = "";
  } else {
    loc_label = time ? "Time: " : "Lap: ";
  }

  // Show only lap counts before start or after race
  if (!time) {
    snprintf(result, BUFSIZE, "%s%d/%d", loc_label, car_->_laps, s->_totLaps);
  } else {
    cur_left = s->_totTime - s->currentTime;
    if (s->currentTime < 0.0f)
      cur_left = s->_totTime;
    if (cur_left < 0.0f)
      cur_left = 0.0f;

    snprintf(result, BUFSIZE, "%s%d:%02d:%02d", loc_label,
                (int)floor(cur_left / 3600.0f),
                (int)floor(cur_left / 60.0f) % 60,
                (int)floor(cur_left) % 60);
  }
}


void cGrBoard::refreshBoard(tSituation *s, const cGrFrameInfo* frameInfo,
                            const tCarElt *currCar, bool isCurrScreen)
{
  car_ = currCar;
  grDispMisc(isCurrScreen);

  if (arcadeFlag) {
    grDispArcade(s);
  } else {
    if (debugFlag)
      grDispDebug(s, frameInfo);
    if (GFlag)
      grDispGGraph();
    if (boardFlag)
      grDispCarBoard(s);
    if (leaderFlag)
      grDispLeaderBoard(s);
    if (counterFlag)
      grDispCounterBoard2();
  }

  trackMap->display(currCar, s, 0, 0, rightAnchor, TOP_ANCHOR);
}


// TODO(?): clean solution for cleanup.
static ssgSimpleState* cleanup[1024];
static int nstate = 0;


void grInitBoardCar(tCarElt *car)
{
  static const int nMaxTexPathSize = 4096;
  const bool bMasterModel = strlen(car->_masterModel) != 0;

  grssgLoaderOptions options;
  grssgSetCurrentOptions(&options);

  int index = car->index; /* current car's index */
  tgrCarInfo *carInfo = &grCarInfo[index];
  void *handle = car->_carHandle;

  /* Set tachometer/speedometer textures search path :
   1) driver level specified, in the user settings,
   2) driver level specified,
   3) car level specified,
   4) common textures */
  grFilePath = (char*)malloc(nMaxTexPathSize);
  int lg = 0;
  lg += snprintf(grFilePath + lg, nMaxTexPathSize - lg, "%sdrivers/%s/%d/%s;",
                    GfLocalDir(), car->_modName, car->_driverIndex, car->_carName);
  if (bMasterModel)
    lg += snprintf(grFilePath + lg, nMaxTexPathSize - lg, "%sdrivers/%s/%d/%s;",
                    GfLocalDir(), car->_modName, car->_driverIndex, car->_masterModel);

  lg += snprintf(grFilePath + lg, nMaxTexPathSize - lg, "%sdrivers/%s/%d;",
                    GfLocalDir(), car->_modName, car->_driverIndex);

  lg += snprintf(grFilePath + lg, nMaxTexPathSize - lg, "%sdrivers/%s/%s;",
                    GfLocalDir(), car->_modName, car->_carName);
  if (bMasterModel)
    lg += snprintf(grFilePath + lg, nMaxTexPathSize - lg, "%sdrivers/%s/%s;",
                    GfLocalDir(), car->_modName, car->_masterModel);

  lg += snprintf(grFilePath + lg, nMaxTexPathSize - lg, "%sdrivers/%s;",
                    GfLocalDir(), car->_modName);

  lg += snprintf(grFilePath + lg, nMaxTexPathSize - lg, "drivers/%s/%d/%s;",
                    car->_modName, car->_driverIndex, car->_carName);
  if (bMasterModel)
    lg += snprintf(grFilePath + lg, nMaxTexPathSize - lg, "drivers/%s/%d/%s;",
                    car->_modName, car->_driverIndex, car->_masterModel);

  lg += snprintf(grFilePath + lg, nMaxTexPathSize - lg, "drivers/%s/%d;",
                    car->_modName, car->_driverIndex);

  lg += snprintf(grFilePath + lg, nMaxTexPathSize - lg, "drivers/%s/%s;",
                    car->_modName, car->_carName);
  if (bMasterModel)
    lg += snprintf(grFilePath + lg, nMaxTexPathSize - lg, "drivers/%s/%s;",
                    car->_modName, car->_masterModel);

  lg += snprintf(grFilePath + lg, nMaxTexPathSize - lg, "drivers/%s;", car->_modName);

  lg += snprintf(grFilePath + lg, nMaxTexPathSize - lg, "cars/models/%s;", car->_carName);
  if (bMasterModel)
    lg += snprintf(grFilePath + lg, nMaxTexPathSize - lg, "cars/models/%s;", car->_masterModel);

  lg += snprintf(grFilePath + lg, nMaxTexPathSize - lg, "data/textures");

  /* Tachometer --------------------------------------------------------- */
  tgrCarInstrument *curInst = &(carInfo->instrument[0]);

  /* Load the Tachometer texture */
  const char *param = GfParmGetStr(handle, SECT_GROBJECTS,
                                    PRM_TACHO_TEX, "rpm8000.png");

  curInst->texture = (ssgSimpleState*)grSsgLoadTexState(param);
  if (curInst->texture == 0)
    curInst->texture = (ssgSimpleState*)grSsgLoadTexState("rpm8000.rgb");

  cleanup[nstate] = curInst->texture;
  nstate++;

  /* Load the instrument placement */
  tdble xSz = GfParmGetNum(handle, SECT_GROBJECTS, PRM_TACHO_XSZ, (char*)NULL, 128);
  tdble ySz = GfParmGetNum(handle, SECT_GROBJECTS, PRM_TACHO_YSZ, (char*)NULL, 128);

  // position are delta from center of screen
  tdble xpos = GfParmGetNum(handle, SECT_GROBJECTS, PRM_TACHO_XPOS, (char*)NULL, 0 - xSz);
  tdble ypos = GfParmGetNum(handle, SECT_GROBJECTS, PRM_TACHO_YPOS, (char*)NULL, 0);
  tdble needlexSz = GfParmGetNum(handle, SECT_GROBJECTS, PRM_TACHO_NDLXSZ, (char*)NULL, 50);
  tdble needleySz = GfParmGetNum(handle, SECT_GROBJECTS, PRM_TACHO_NDLYSZ, (char*)NULL, 2);
  curInst->needleXCenter = GfParmGetNum(handle, SECT_GROBJECTS, PRM_TACHO_XCENTER, (char*)NULL, xSz / 2.0) + xpos;
  curInst->needleYCenter = GfParmGetNum(handle, SECT_GROBJECTS, PRM_TACHO_YCENTER, (char*)NULL, ySz / 2.0) + ypos;
  curInst->digitXCenter = GfParmGetNum(handle, SECT_GROBJECTS, PRM_TACHO_XDIGITCENTER, (char*)NULL, xSz / 2.0) + xpos;
  curInst->digitYCenter = GfParmGetNum(handle, SECT_GROBJECTS, PRM_TACHO_YDIGITCENTER, (char*)NULL, 10) + ypos;
  curInst->minValue = GfParmGetNum(handle, SECT_GROBJECTS, PRM_TACHO_MINVAL, (char*)NULL, 0);
  curInst->maxValue = GfParmGetNum(handle, SECT_GROBJECTS, PRM_TACHO_MAXVAL, (char*)NULL, RPM2RADS(10000)) - curInst->minValue;
  curInst->minAngle = GfParmGetNum(handle, SECT_GROBJECTS, PRM_TACHO_MINANG, "deg", 225);
  curInst->maxAngle = GfParmGetNum(handle, SECT_GROBJECTS, PRM_TACHO_MAXANG, "deg", -45) - curInst->minAngle;
  curInst->monitored = &(car->_enginerpm);
  curInst->prevVal = curInst->minAngle;

  /* Get colour to use for needle, default is Red */
  curInst->needleColor[0] = GfParmGetNum(handle, SECT_GROBJECTS,
                                PRM_NEEDLE_RED, (char*)NULL, 1.0);
  curInst->needleColor[1] = GfParmGetNum(handle, SECT_GROBJECTS,
                                PRM_NEEDLE_GREEN, (char*)NULL, 0.0);
  curInst->needleColor[2] = GfParmGetNum(handle, SECT_GROBJECTS,
                                PRM_NEEDLE_BLUE, (char*)NULL, 0.0);
  curInst->needleColor[3] = GfParmGetNum(handle, SECT_GROBJECTS,
                                PRM_NEEDLE_ALPHA, (char*)NULL, 1.0);

  curInst->CounterList = glGenLists(1);
  glNewList(curInst->CounterList, GL_COMPILE);
  glBegin(GL_TRIANGLE_STRIP);
  {
    glColor4f(1.0, 1.0, 1.0, 0.0);
    glTexCoord2f(0.0, 0.0);
    glVertex2f(xpos, ypos);
    glTexCoord2f(0.0, 1.0);
    glVertex2f(xpos, ypos + ySz);
    glTexCoord2f(1.0, 0.0);
    glVertex2f(xpos + xSz, ypos);
    glTexCoord2f(1.0, 1.0);
    glVertex2f(xpos + xSz, ypos + ySz);
  }
  glEnd();
  glEndList();

  curInst->needleList = glGenLists(1);
  glNewList(curInst->needleList, GL_COMPILE);
  glBegin(GL_TRIANGLE_STRIP);
  {
    glColor4f(curInst->needleColor[0], curInst->needleColor[1],
                curInst->needleColor[2], curInst->needleColor[3]);
    glVertex2f(0, -needleySz);
    glVertex2f(0, needleySz);
    glVertex2f(needlexSz, -needleySz / 2.0);
    glVertex2f(needlexSz, needleySz / 2.0);
  }
  glEnd();
  glEndList();


  /* Speedometer ----------------------------------------------------------- */
  curInst = &(carInfo->instrument[1]);

  /* Load the Speedometer texture */
  param = GfParmGetStr(handle, SECT_GROBJECTS, PRM_SPEEDO_TEX, "speed360.png");

  curInst->texture = (ssgSimpleState*)grSsgLoadTexState(param);
  if (curInst->texture == 0)
    curInst->texture = (ssgSimpleState*)grSsgLoadTexState("speed360.rgb");

  free(grFilePath);

  cleanup[nstate] = curInst->texture;
  nstate++;

  /* Load the instrument placement */
  xSz = GfParmGetNum(handle, SECT_GROBJECTS, PRM_SPEEDO_XSZ, (char*)NULL, 128);
  ySz = GfParmGetNum(handle, SECT_GROBJECTS, PRM_SPEEDO_YSZ, (char*)NULL, 128);

  // position are delta from center of screen
  xpos = GfParmGetNum(handle, SECT_GROBJECTS, PRM_SPEEDO_XPOS, (char*)NULL, 0);
  ypos = GfParmGetNum(handle, SECT_GROBJECTS, PRM_SPEEDO_YPOS, (char*)NULL, 0);
  needlexSz = GfParmGetNum(handle, SECT_GROBJECTS, PRM_SPEEDO_NDLXSZ, (char*)NULL, 50);
  needleySz = GfParmGetNum(handle, SECT_GROBJECTS, PRM_SPEEDO_NDLYSZ, (char*)NULL, 2);
  curInst->needleXCenter = GfParmGetNum(handle, SECT_GROBJECTS, PRM_SPEEDO_XCENTER, (char*)NULL, xSz / 2.0) + xpos;
  curInst->needleYCenter = GfParmGetNum(handle, SECT_GROBJECTS, PRM_SPEEDO_YCENTER, (char*)NULL, ySz / 2.0) + ypos;
  curInst->digitXCenter = GfParmGetNum(handle, SECT_GROBJECTS, PRM_SPEEDO_XDIGITCENTER, (char*)NULL, xSz / 2.0) + xpos;
  curInst->digitYCenter = GfParmGetNum(handle, SECT_GROBJECTS, PRM_SPEEDO_YDIGITCENTER, (char*)NULL, 10) + ypos;
  curInst->minValue = GfParmGetNum(handle, SECT_GROBJECTS, PRM_SPEEDO_MINVAL, (char*)NULL, 0);
  curInst->maxValue = GfParmGetNum(handle, SECT_GROBJECTS, PRM_SPEEDO_MAXVAL, (char*)NULL, 100) - curInst->minValue;
  curInst->minAngle = GfParmGetNum(handle, SECT_GROBJECTS, PRM_SPEEDO_MINANG, "deg", 225);
  curInst->maxAngle = GfParmGetNum(handle, SECT_GROBJECTS, PRM_SPEEDO_MAXANG, "deg", -45) - curInst->minAngle;
  curInst->monitored = &(car->_speed_x);
  curInst->prevVal = curInst->minAngle;
  if (strcmp(GfParmGetStr(handle, SECT_GROBJECTS, PRM_SPEEDO_DIGITAL, "yes"), "yes") == 0) {
    curInst->digital = 1;
  }

  /* Get colour to use for needle, default is Red */
  curInst->needleColor[0] = GfParmGetNum(handle, SECT_GROBJECTS,
                                PRM_NEEDLE_RED, (char*)NULL, 1.0);
  curInst->needleColor[1] = GfParmGetNum(handle, SECT_GROBJECTS,
                                PRM_NEEDLE_GREEN, (char*)NULL, 0.0);
  curInst->needleColor[2] = GfParmGetNum(handle, SECT_GROBJECTS,
                                PRM_NEEDLE_BLUE, (char*)NULL, 0.0);
  curInst->needleColor[3] = GfParmGetNum(handle, SECT_GROBJECTS,
                                PRM_NEEDLE_ALPHA, (char*)NULL, 1.0);

  curInst->CounterList = glGenLists(1);
  glNewList(curInst->CounterList, GL_COMPILE);
  glBegin(GL_TRIANGLE_STRIP);
  {
    glColor4f(1.0, 1.0, 1.0, 0.0);
    glTexCoord2f(0.0, 0.0);
    glVertex2f(xpos, ypos);
    glTexCoord2f(0.0, 1.0);
    glVertex2f(xpos, ypos + ySz);
    glTexCoord2f(1.0, 0.0);
    glVertex2f(xpos + xSz, ypos);
    glTexCoord2f(1.0, 1.0);
    glVertex2f(xpos + xSz, ypos + ySz);
  }
  glEnd();
  glEndList();

  curInst->needleList = glGenLists(1);
  glNewList(curInst->needleList, GL_COMPILE);
  glBegin(GL_TRIANGLE_STRIP);
  {
    glColor4f(curInst->needleColor[0], curInst->needleColor[1],
                curInst->needleColor[2], curInst->needleColor[3]);
    glVertex2f(0, -needleySz);
    glVertex2f(0, needleySz);
    glVertex2f(needlexSz, -needleySz / 2.0);
    glVertex2f(needlexSz, needleySz / 2.0);
  }
  glEnd();
  glEndList();
}   // grInitBoardCar


void grShutdownBoardCar(void)
{
  /*int i;
  for (i = 0; i < nstate; i++) {
    printf("%d\n", i);
    if (cleanup[i]->getRef() > 0) {
      ssgDeRefDelete(cleanup[i]);
    } else {
      delete cleanup[i];
    }
  }
  nstate = 0;*/
}

#define LEADERBOARD_SCROLL_TIME 2.0
/**
 * grDispLeaderBoardScroll
 *
 * Displays the leaderboard in a vertical scrolled fashion,
 * if there are more than 10 names to display.
 *
 * @param s[in] current situation, provided by the sim
*/
void
cGrBoard::grDispLeaderBoardScroll(const tSituation *s)
{
  // Scrolling
  if (iTimer == 0 || s->currentTime < iTimer)
    iTimer = s->currentTime;
  if (s->currentTime >= iTimer + LEADERBOARD_SCROLL_TIME) {
    iTimer = s->currentTime;
    ++iStart;
    // Limit: number of cars + one separator line
    iStart = iStart % (s->_ncars + 1);
  }

  int current = 0;  // Position of the currently displayed car
  for (int i = 0; i < s->_ncars; ++i) {
    if (car_ == s->cars[i]) {
      current = i;
      break;
    }
  }  // for i

  // Coords, limits
  const int x = leftAnchor + 10;
  const int x2 = x + 100;
  static const int dxc = 60;

  const int dy = GfuiFontHeight(GFUI_FONT_SMALL_C);
  // Max # of lines to display (# of cars in race or max 10 by default)
  const int maxLines = MIN(leaderNb, s->_ncars);
  int y = TOP_ANCHOR - 10;
  int y2 = y - 5 - dy * (maxLines + 1);

  grSetupDrawingArea(x, y, x2 + dxc + 5, y2);
  y = y2;

  // The board is built from bottom up:
  // driver position #10/current is drawn first,
  // then from #9 onto #1, then the text 'Laps' if drawLaps requires.
  char buf[BUFSIZE];
  float *color;
  for (int j = maxLines - 1; j >= 0; j--) {
    int i = j + iStart;  // index of driver to be displayed

    if (i == s->_ncars) {
      // print empty line after last car
    } else {
      i = i % (s->_ncars + 1);

      // Set colour of the drivers to that
      // defined in the drivers' XML file.
      // Current driver is 'emphasized' colour (yellow...)
      color = (i == current)
        ? emphasized_color_ : grCarInfo[s->cars[i]->index].iconColor;

      // Driver position + short name
      snprintf(buf, sizeof(buf), "%3d: %s", i + 1, s->cars[i]->_sname);
      GfuiDrawString(buf, color, GFUI_FONT_SMALL_C, x, y);

      // Display driver time / time behind leader / laps behind leader
      string sEntry = grGenerateLeaderBoardEntry(s->cars[i], s, (i == 0));
      if (s->cars[i]->_state & RM_CAR_STATE_DNF
        || s->cars[i]->_state & RM_CAR_STATE_PIT) {
        // driver DNF or in pit, show 'out' in red
        color = danger_color_;  // red
      }
      GfuiDrawString(sEntry.c_str(), color, GFUI_FONT_SMALL_C, x2, y, dxc,
                        GFUI_ALIGN_HR);
    }   // else i
    y += dy;  // 'Line feed'
  }  // for j

  // Write 'Lap X/Y' on top of the leader board
  color = normal_color_;    //white
  if (s->currentTime < s->_totTime) {
    GfuiDrawString(" Laps:", color, GFUI_FONT_SMALL_C, x, y); //white
    snprintf(buf, sizeof(buf), "%d", s->cars[0]->_laps);
  } else {
    GfuiDrawString(" Lap:", color, GFUI_FONT_SMALL_C, x, y);  //white
    snprintf(buf, sizeof(buf), "%d / %d", s->cars[0]->_laps, s->_totLaps);
  }
  GfuiDrawString(buf, color, GFUI_FONT_SMALL_C, x2, y, dxc, GFUI_ALIGN_HR);   //white
}   // grDispLeaderBoardScroll


#define LEADERBOARD_LINE_SCROLL_RATE 80     // pixels per second
#define LEADERBOARD_LINE_SCROLL_DELAY 5     // seconds
/**
 * grDispLeaderBoardScrollLine
 *
 * Scrolls the leaderboard on the bottom line, as seen on TV broadcasts.
 *
 * @param s[in] current situation, provided by the sim
 */
void
cGrBoard::grDispLeaderBoardScrollLine(const tSituation *s)
{
  // At first, get the current time and rebuild the ScrollLine text
  if (iTimer == 0 || s->currentTime < iTimer) {
    iTimer = s->currentTime;
    st.clear();
    /*!The roster holds the driver's position, name and difference
     * *at the time* the leader starts a new lap.
     * So it can happen it is somewhat mixed up, it will settle down
     * in the next lap.
    */

    std::ostringstream osRoster;
    // Add the track name as separator, embedded with 3 spaces each side.
    osRoster << "   " << grTrack->name << "   ";
    // Add # of laps
    osRoster << "Lap " << s->cars[0]->race.laps << " | ";
    for (int i = 0; i < s->_ncars; i++) {
      // Driver position + name
      osRoster.width(3);
      osRoster << (i + 1);
      osRoster << ": " << s->cars[i]->_cname;

      // Display driver time / time behind leader / laps behind leader
      string sEntry = grGenerateLeaderBoardEntry(s->cars[i], s, (i == 0));

      // get rid of leading spaces
      size_t iCut = sEntry.find_first_not_of(' ');
      if (iCut != string::npos && iCut != 0) {
        sEntry = sEntry.substr(iCut - 1);
      }  // if iCut
      // Add to roster, then separate it from next one
      osRoster << sEntry << "   ";
    }   // for i

    st.assign(osRoster.str());
  }

  int offset = (s->currentTime - iTimer - LEADERBOARD_LINE_SCROLL_DELAY)
                * LEADERBOARD_LINE_SCROLL_RATE;
  if (offset < 0)
    offset = 0;

  int dy = GfuiFontHeight(GFUI_FONT_MEDIUM_C);
  int dx = GfuiFontWidth(GFUI_FONT_SMALL_C, "W") * st.size();

  // Set up drawing area
  grSetupDrawingArea(leftAnchor, TOP_ANCHOR, rightAnchor, TOP_ANCHOR - dy);

  // Check if scrolling is completed
  if (offset > dx + 5)
    iTimer = 0;
  else
    // Display the line
    GfuiDrawString(st.c_str(), normal_color_, GFUI_FONT_MEDIUM_C, //white
                    leftAnchor + 5 - offset, TOP_ANCHOR - dy);
}   // grDispLeaderBoardScrollLine


/**
 * name: grGenerateLeaderBoardEntry
 *
 * Generates one leaderboard entry,
 * this time only the time behind/laps behind part.
 * Optimally it would make up the whole string that can be shown
 * in the line, but colour handling and positioning
 * is not available then.
 *
 * @param isLeader
 * @return string
 */
string
cGrBoard::grGenerateLeaderBoardEntry(const tCarElt *car, const tSituation* s,
                                        const bool isLeader) const
{
  char buf[BUFSIZE];

  // Display driver time / time behind leader / laps behind leader
  if (car->_state & RM_CAR_STATE_DNF)  {
    snprintf(buf, sizeof(buf), "       out");
    return buf;
  }

  if (car->_state & RM_CAR_STATE_PIT) {
    snprintf(buf, sizeof(buf), "       PIT");
    return buf;
  }

  // This is the leader, put out his time
  if (isLeader) {
    if (car->_bestLapTime == 0) {
      snprintf(buf, sizeof(buf), "       --:---");
    } else {
      if (s->_raceType == RM_TYPE_RACE || s->_ncars <= 1)
        grWriteTimeBuf(buf, car->_curTime, 0);
      else
        grWriteTimeBuf(buf, car->_bestLapTime, 0);
    }
    return buf;
  }

  // This is not the leader
  int lapsBehindLeader = car->_lapsBehindLeader;

  if (car->_laps < s->cars[0]->_laps - 1) {
    // need to do a little math as
    // car->_lapsBehindLeader is only updated at finish line
    lapsBehindLeader = s->cars[0]->_laps - car->_laps;

    if (s->cars[0]->_distFromStartLine < car->_distFromStartLine)
      lapsBehindLeader--;
  }

  switch (lapsBehindLeader) {
    case 0:  // Driver in same lap as leader or on first lap
      if (car->_bestLapTime == 0 || car->_laps < s->cars[0]->_laps) {
        snprintf(buf, sizeof(buf), "       --:---");
      } else {
        grWriteTimeBuf(buf, car->_timeBehindLeader, 1);
      }
      break;

    case 1:  // 1 lap behind leader
      snprintf(buf, sizeof(buf), "+%3d Lap", lapsBehindLeader);
      break;

    default:  // N laps behind leader
      snprintf(buf, sizeof(buf), "+%3d Laps", lapsBehindLeader);
      break;
  }

  return buf;
}


/**
 * Set up a drawing area to put textual info there.
 *
 * Draws a dark quadrangle on the given coords.
 *
 * @param xl X left
 * @param yb Y bottom
 * @param xr X right
 * @param yt Y top
 */
void
cGrBoard::grSetupDrawingArea(int xl, int yb, int xr, int yt) const
{
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glBegin(GL_QUADS);
  // darkblue to fit the menu style: 0a162f
  glColor4f(0.039, 0.086, 0.184, 0.8);
  glVertex2f(xl, yb);
  glVertex2f(xr, yb);
  glVertex2f(xr, yt);
  glVertex2f(xl, yt);
  glEnd();
  glDisable(GL_BLEND);
}


void
cGrBoard::grDispIndicators(const bool arcade)
{
  // Only useful for humans - maybe robots should show that, too?
  if (car_->_driverType == RM_DRV_HUMAN) {
    bool abs = false;   // Show ABS indicator?
    bool tcs = false;   // Show TCS indicator?
    bool spd = false;   // Show speed limiter indicator?

    // Parse control messages if they include ABS / TCS / SPD
    for (int i = 0; i < 4; i++) {
      if (car_->ctrl.msg[i]) {
        abs = abs || strstr(car_->ctrl.msg[i], "ABS");
        tcs = tcs || strstr(car_->ctrl.msg[i], "TCS");
        spd = spd || strstr(car_->ctrl.msg[i], "Speed Limiter On");
      }
    }

    // Setup drawing area
    int dy = GfuiFontHeight(GFUI_FONT_MEDIUM_C);
    int dy2 = GfuiFontHeight(GFUI_FONT_SMALL_C);
    int dx = GfuiFontWidth(GFUI_FONT_MEDIUM_C, "SPD");

    int x, y;
    if (arcade) {
      x = leftAnchor + 15 + 30;                 // constant text left pos.
      y = BOTTOM_ANCHOR + dy2 * 8 + dy - 2;   // first row top pos.
    } else {
      x = centerAnchor - 200;                 // constant text left pos.
      y = BOTTOM_ANCHOR + dy2 * 8 + dy + 5;   // first row top pos.
    }

    // Display board
    grSetupDrawingArea(x - 5, y + dy + 5, x + dx + 5, y - dy2 * 8 - dy + 5);

    // Display strings (until we are more advanced graphically)
    if (abs)
      GfuiDrawString("ABS", emphasized_color_, GFUI_FONT_MEDIUM_C, x, y);    //yellow
    else
      GfuiDrawString("ABS", inactive_color_, GFUI_FONT_MEDIUM_C, x, y);  //grey
    y -= dy;

    if (tcs)
      GfuiDrawString("TCS", emphasized_color_, GFUI_FONT_MEDIUM_C, x, y);    //yellow
    else
      GfuiDrawString("TCS", inactive_color_, GFUI_FONT_MEDIUM_C, x, y);  //grey
    y -= dy;

    if (spd)
      GfuiDrawString("SPD", emphasized_color_, GFUI_FONT_MEDIUM_C, x, y);    //yellow
    else
      GfuiDrawString("SPD", inactive_color_, GFUI_FONT_MEDIUM_C, x, y);  //grey
  }  // if human
}  // grDispIndicators


void cGrBoard::ReadDashColor(void *hdle, const string color_name, float **color) {
  char buf[1024];
  snprintf(buf, sizeof(buf), "%s/%s/%s",
        GFSCR_SECT_DASHSETTINGS, GFSCR_DASH_COLORS, color_name.c_str());
  delete [] *color;
  *color = new float[4];
  for (int i = 0; i < 4; ++i) {
    (*color)[i] = GfParmGetNum(hdle, buf, rgba[i].c_str(), NULL, 1.0);
  }
}

