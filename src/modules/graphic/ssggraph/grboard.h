/***************************************************************************

    file                 : grboard.h
    created              : Thu Aug 17 23:55:47 CEST 2000
    copyright            : (C) 2000 by Eric Espie
    email                : torcs@free.fr
    version              : $Id: grboard.h 6071 2015-08-14 16:27:16Z kakukri $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _GRBOARD_H_
#define _GRBOARD_H_

#include <car.h>        // tCarElt
#include <raceman.h>    // tSituation

class cGrTrackMap;
class cGrFrameInfo;

#include <string>
#include <vector>

class cGrBoard
{
 protected:
    int id;     // Board Id
    const tCarElt* car_;

    int boardFlag;
    int leaderFlag;
    int debugFlag;
    int leaderNb;
    int counterFlag;
    int GFlag;
    int dashboardFlag;
    int arcadeFlag;
    int boardWidth;
    int leftAnchor;
    int centerAnchor;
    int rightAnchor;
    int speedoRise;
    // Scrolling leaderboard variables
    int iStart;
    double iTimer;
    int iStringStart;
    std::string st;     // This is the line we will display in the top

 private:
    void grDispDebug(const tSituation *s, const cGrFrameInfo* frame);
    void grDispGGraph();
    void grDispMisc(bool bCurrentScreen);
    void grDrawGauge(tdble X1, tdble Y1, tdble H, float *clr1,
                        float *clr2, tdble val, const char *title);
    void grDispEngineLeds(int X, int Y, int align, bool bg);

    void grDispCarBoard(const tSituation *s);
    void grDispCarBoard1(const tSituation *s);
    void grDispCarBoard2(const tSituation *s);

    void grDispIndicators(const bool arcade);

    void grDispLeaderBoard(const tSituation *s);
    void grDispCounterBoard2();
    void grDispLeaderBoardScroll(const tSituation *s);
    void grDispLeaderBoardScrollLine(const tSituation *s);
    
    void grDispDashboard();

    void grDispArcade(const tSituation *s);
    std::string grGenerateLeaderBoardEntry(const tCarElt *car, const tSituation *s,
                                            const bool isLeader) const;
    // Track overview object
    cGrTrackMap *trackMap;

    bool grGetSplitTime(const tSituation *s, bool gap_inrace,
                        double &time, int *laps_different, float **color);
    void grGetLapsTime(const tSituation *s, char* result,
                        char const** label) const;
    void grSetupDrawingArea(int xl, int yb, int xr, int yt) const;

private:
    //Dash colour handling
    float *normal_color_;
    float *danger_color_;
    float *ok_color_;
    float *error_color_;
    float *inactive_color_;
    float *emphasized_color_;
    float *ahead_color_;
    float *behind_color_;
    float *arcade_color_;
    float *background_color_;

    void ReadDashColor(void *hdle, const std::string color_name, float **color);

 public:
    explicit cGrBoard(int myid);
    ~cGrBoard();

    void initBoard(void);
    void shutdown(void);
    void selectBoard(int brd);
    void setWidth(int width);
    inline cGrTrackMap *getTrackMap() { return trackMap; }

    void refreshBoard(tSituation *s, const cGrFrameInfo* frameInfo,
                        const tCarElt *currCar, bool isCurrScreen);
    void loadDefaults(const tCarElt *curCar);
};

extern void grInitBoardCar(tCarElt *car);
extern void grShutdownBoardCar(void);

#endif /* _GRBOARD_H_ */
