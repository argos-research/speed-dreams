/***************************************************************************

    file                 : networkhuman.cpp
    created              : Sat Mar 18 23:16:38 CET 2000
    copyright            : (C) 2000 by Eric Espie
    email                : torcs@free.fr
    version              : $Id: networkhuman.cpp 5522 2013-06-17 21:03:25Z torcs-ng $

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
    @version	$Id: networkhuman.cpp 5522 2013-06-17 21:03:25Z torcs-ng $
*/

/* 2013/3/21 Tom Low-Shang
 *
 * Deleted original contents, except for robot interface entry points.
 */
#include <portability.h>
#include <network.h>
#include <humandriver.h>

static void initTrack(int index, tTrack* track, void *carHandle, void **carParmHandle, tSituation *s);
static void drive_mt(int index, tCarElt* car, tSituation *s);
static void drive_at(int index, tCarElt* car, tSituation *s);
static void newrace(int index, tCarElt* car, tSituation *s);
static void resumerace(int index, tCarElt* car, tSituation *s);
static int  pitcmd(int index, tCarElt* car, tSituation *s);

class NetworkHuman: public HumanDriver
{
public:
    NetworkHuman();

    void init_track(int index, tTrack* track, void *carHandle,
        void **carParmHandle, tSituation *s);
    void new_race(int index, tCarElt* car, tSituation *s);
    void resume_race(int index, tCarElt* car, tSituation *s);
    void drive_mt(int index, tCarElt* car, tSituation *s);
    void drive_at(int index, tCarElt* car, tSituation *s);
    int pit_cmd(int index, tCarElt* car, tSituation *s);
    void read_prefs(int index);
    void set_active_index();
    int get_active_index();

private:
    int active_index;  // index of local network driver
    int human_index;   // index of local human driver

    bool is_active_index(int index);
    int get_human_index();
} robot;

#ifdef _WIN32
/* should be present in mswindows */
BOOL WINAPI DllEntryPoint (HINSTANCE hDLL, DWORD dwReason, LPVOID Reserved)
{
    return TRUE;
}
#endif

static void
shutdown(int index)
{
    robot.shutdown(index);
}



/*
 * Function
 *	InitFuncPt
 *
 * Description
 *	Robot functions initialisation
 *
 * Parameters
 *	pt	pointer on functions structure
 *
 * Return
 *	0
 *
 * Remarks
 *
 */
static int
InitFuncPt(int index, void *pt)
{
	tRobotItf *itf = (tRobotItf *)pt;

        robot.init_context(index, robot.get_active_index());

	itf->rbNewTrack = initTrack;	/* give the robot the track view called */
	/* for every track change or new race */
	itf->rbNewRace  = newrace;
	itf->rbResumeRace  = resumerace;

	/* drive during race */
	itf->rbDrive = robot.uses_at(index) ? drive_at : drive_mt;
	itf->rbShutdown = shutdown;
	itf->rbPitCmd   = pitcmd;
	itf->index      = index;

	return 0;
}

/*
 * Function
 *	moduleWelcome
 *
 * Description
 *	First function of the module called at load time :
 *      - the caller gives the module some information about its run-time environment
 *      - the module gives the caller some information about what he needs
 *
 * Parameters
 *	welcomeIn  : Run-time info given by the module loader at load time
 *	welcomeOut : Module run-time information returned to the called
 *
 * Return
 *	0, if no error occured 
 *	non 0, otherwise
 *
 * Remarks
 *	MUST be called before moduleInitialize()
 */
extern "C" int moduleWelcome(const tModWelcomeIn* welcomeIn, tModWelcomeOut* welcomeOut)
{
    welcomeOut->maxNbItf = robot.count_drivers();

    robot.set_active_index();

    return 0;
}

/*
 * Function
 *	moduleInitialize
 *
 * Description
 *	module entry point
 *
 * Parameters
 *	modInfo	administrative info on module
 *
 * Return
 *	0 if no error occured
 *	-1, if any error occured 
 *
 * Remarks
 *
 */

extern "C" int moduleInitialize(tModInfo *modInfo)
{
    return robot.initialize(modInfo, InitFuncPt);
}


/*
 * Function
 *	moduleTerminate
 *
 * Description
 *	Module exit point
 *
 * Parameters
 *	None
 *
 * Return
 *	0
 *
 * Remarks
 *
 */

extern "C" int moduleTerminate()
{
    robot.terminate();

    return 0;
}

/*
 * Function
 *
 *
 * Description
 *	search under drivers/networkhuman/tracks/<trackname>/car-<model>-<index>.xml
 *		     drivers/networkhuman/car-<model>-<index>.xml
 *		     drivers/networkhuman/tracks/<trackname>/car-<model>.xml
 *		     drivers/networkhuman/car-<model>.xml
 *
 * Parameters
 *
 *
 * Return
 *
 *
 * Remarks
 *
 */
static void initTrack(int index, tTrack* track, void *carHandle, void **carParmHandle, tSituation *s)
{
    robot.init_track(index, track, carHandle, carParmHandle, s);
}

/*
 * Function
 *
 *
 * Description
 *
 *
 * Parameters
 *
 *
 * Return
 *
 */

static void newrace(int index, tCarElt* car, tSituation *s)
{
    robot.new_race(index, car, s);
}

static void resumerace(int index, tCarElt* car, tSituation *s)
{
    robot.resume_race(index, car, s);
}

/*
 * Function
 *
 *
 * Description
 *
 *
 * Parameters
 *
 *
 * Return
 *
 *
 * Remarks
 *	
 */
static void drive_mt(int index, tCarElt* car, tSituation *s)
{
    robot.drive_mt(index, car, s);
}
/*
 * Function
 *
 *
 * Description
 *
 *
 * Parameters
 *
 *
 * Return
 *	
 *
 * Remarks
 *	
 */
static void drive_at(int index, tCarElt* car, tSituation *s)
{
    robot.drive_at(index, car, s);
}

static int pitcmd(int index, tCarElt* car, tSituation *s)
{
    return robot.pit_cmd(index, car, s);
}

NetworkHuman::NetworkHuman():
    HumanDriver("networkhuman")
{
    active_index = 0;
    human_index = 0;
}

/*
 * Override to handle local/remote drivers.
 */
void NetworkHuman::init_track(int index, tTrack* track, void *carHandle,
    void **carParmHandle, tSituation *s)
{
    if (is_active_index(index))
    {
        HumanDriver::init_track(index, track, carHandle, carParmHandle, s);
    }
    else
    {
        *carParmHandle = NULL;
    }
}

/*
 * Override to handle local/remote drivers.
 */
void NetworkHuman::new_race(int index, tCarElt* car, tSituation *s)
{
    if (is_active_index(index))
    {
        HumanDriver::new_race(index, car, s);
    }
}

/*
 * Override to handle local/remote drivers.
 */
void NetworkHuman::resume_race(int index, tCarElt* car, tSituation *s)
{
    if (is_active_index(index))
    {
        HumanDriver::resume_race(index, car, s);
    }
}

/*
 * Override to handle local/remote drivers.
 */
void NetworkHuman::drive_mt(int index, tCarElt* car, tSituation *s)
{
    if (is_active_index(index))
    {
        HumanDriver::drive_mt(index, car, s);
    }
}

/*
 * Override to handle local/remote drivers.
 */
void NetworkHuman::drive_at(int index, tCarElt* car, tSituation *s)
{
    if (is_active_index(index))
    {
        HumanDriver::drive_at(index, car, s);
    }
}

/*
 * Override to handle local/remote drivers.
 */
int NetworkHuman::pit_cmd(int index, tCarElt* car, tSituation *s)
{
    if (is_active_index(index))
    {
        return HumanDriver::pit_cmd(index, car, s);
    }
    return ROB_PIT_IM;
}

/*
 * Override to handle local/remote drivers.
 */
void NetworkHuman::read_prefs(int index)
{
    if (is_active_index(index))
    {
        human_prefs(index, get_human_index());
    }
}

void NetworkHuman::set_active_index()
{
    if (NetGetNetwork())
    {
        active_index = NetGetNetwork()->GetNetworkHumanIdx();
    }
}

int NetworkHuman::get_active_index()
{
    return active_index;
}

bool NetworkHuman::is_active_index(int index)
{
    if (NetGetNetwork())
    {
        return active_index == index;
    }
    return false;
}

int NetworkHuman::get_human_index()
{
    if (human_index)
    {
        return human_index;
    }

    if (NetGetNetwork())
    {
        const char *driver_name = NetGetNetwork()->GetDriverName();

        const char *driver_file = "drivers/human/human.xml";
        void *driver_info = GfParmReadFileLocal(driver_file,
                GFPARM_RMODE_REREAD);

        if (driver_info)
        {
            char path2[256];
            int i = 0;
            const char *name;

            do
            {
                i++;
                snprintf(path2, sizeof path2, "Robots/index/%d", i);
                name = GfParmGetStr(driver_info, path2, "name", NULL);
                if (name && strcmp(driver_name, name) == 0)
                {
                    human_index = i;
                    break;
                }	
            }
            while(name);

            return human_index;
        }
    }
    
    return 0;
}

