/*
 * Author: Tom Low-Shang
 * Date  : 2013/3/21
 *
 * Interface to the humandriver robot library.
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef HUMANDRIVER_H
#define HUMANDRIVER_H

#include <robot.h>
#include "robottools.h"

class ROBOTTOOLS_API HumanDriver
{
public:
    HumanDriver(const char *robotname);

    void shutdown(const int index);
    void init_context(int index, int updater_index=0);
    int count_drivers();
    int initialize(tModInfo *modInfo, tfModPrivInit InitFuncPt);
    void terminate();
    void init_track(int index, tTrack* track, void *carHandle,
                    void **carParmHandle, tSituation *s);
    void new_race(int index, tCarElt* car, tSituation *s);
    void resume_race(int index, tCarElt* car, tSituation *s);
    void drive_mt(int index, tCarElt* car, tSituation *s);
    void drive_at(int index, tCarElt* car, tSituation *s);
    int pit_cmd(int index, tCarElt* car, tSituation *s);
    virtual void read_prefs(int index);
    bool uses_at(int index);

protected:
    void human_prefs(const int robot_index, int player_index);

private:
    const char *robotname;
};

#endif
