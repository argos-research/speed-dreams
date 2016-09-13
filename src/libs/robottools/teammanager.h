/***************************************************************************

    file                 : teammanager.h
    created              : Sun Feb 22 23:43:00 CET 2009
    last changed         : Sun May 29 23:00:00 CET 2011
    copyright            : (C) 2009-2011 by Wolf-Dieter Beelitz
    email                : wdbee@users.sourceforge.net
    version              : 1.1

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
    This is a collection of useful functions for using a teammanager with
	teams build of different robots.
	It can handle teams with more drivers than cars per pit.
	You can see how to use in the simplix robots. 

    @author	<a href=mailto:wdbee@users.sourceforge.net>Wolf-Dieter Beelitz</a>
    @version	
    @ingroup	robottools
*/

#ifndef _TEAMMANAGER_H_
#define _TEAMMANAGER_H_

#include <car.h>
#include <track.h>                               // TR_PIT_MAXCARPERPIT = 4
#include <raceman.h>                             // tSituation

#include "robottools.h"


//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// Implementation:
//

// Teammanager defines

#define RT_TM_CURRENT_MAJOR_VERSION 1            // First version of teammanager
//#define RT_TM_CURRENT_MINOR_VERSION 0          // Initial minor version
#define RT_TM_CURRENT_MINOR_VERSION 1            // 1.1 Extended data for TeamDriver

#define RT_TM_PIT_IS_FREE NULL                   // = *Car if reserved/used

#define RT_TM_STATE_NULL 0                       // Team manager was created
#define RT_TM_STATE_INIT 1                       // Team manager was initialized
#define RT_TM_STATE_USED 2                       // Team manager was used


// Teammanager Utilities

//
// Version header
//
typedef struct tDataStructVersionHeader
{                                                // NEVER CHANGE THIS >>> 
    short int MajorVersion;                      // Changed if struct is extended 
    short int MinorVersion;                      // Changed for changes without extending the struct 
	int Size;                                    // sizeof the struct including this header
	tDataStructVersionHeader* Next;              // Linked list for garbage collection
                                                 // NEVER CHANGE THIS <<< 
} tDataStructVersionHeader;


//
// Data of a teammate
//
typedef struct tTeammate
{                                                // NEVER CHANGE THIS >>> V1.X
    tDataStructVersionHeader Header;             // Version and size of this struct
	CarElt*	Car;		                         // The car of this team member
	tTeammate* Next;	                         // The next team member
	int Count;                                   // Nbr of Teammates in this list
	                                             // <<< NEVER CHANGE THIS V1.X
	                                             // Extend it here if needed but increment VERSION!
} tTeammate;


//
// Data of a teams pit (For later use with multiple pits per team)
//
typedef struct tTeamPit
{                                                // NEVER CHANGE THIS >>> V1.X
    tDataStructVersionHeader Header;             // Version and size of this struct
	tTeamPit* Next;                              // Linked list of pits of this team for later use
	tTeammate* Teammates;						 // Linked list of teammates of this pit
	CarElt*	PitState;                            // Request for shared pit
	tTrackOwnPit* Pit;                           // Game pit
	int Count;                                   // Nbr of TeamPits in this list
	char* Name;                                  // Name of the Teampit
	                                             // <<< NEVER CHANGE THIS V1.X
	                                             // Extend it here if needed but increment VERSION!
} tTeamPit;

//
// Data of a team
//
typedef struct tTeam
{                                                // NEVER CHANGE THIS >>> V1.X
    tDataStructVersionHeader Header;             // Version and size of this struct
	char* TeamName;	                             // Name of team
	tTeam* Next;                                 // Linked list of teams
	tTeamPit* TeamPits;                          // Linked list of pits of this team
	int Count;                                   // Nbr of teammates
	int MinMajorVersion;                         // Min MajorVersion used by an teammates robot
	                                             // <<< NEVER CHANGE THIS V1.X
} tTeam;                                         // and add the additional values to function RtTeamUpdate
												 // and initialization in function RtTeam

//
// Data of a driver beeing in the race
//
typedef struct tTeamDriver 
{                                                // NEVER CHANGE THIS >>> V1.X
    tDataStructVersionHeader Header;             // Version and size of this struct
	tTeamDriver* Next;                           // Linked list of drivers (containig all drivers of the race)
	int Count;                                   // Nbr of drivers
	tCarElt* Car;                                // Drivers car
	tTeam* Team;                                 // Drivers Team
	tTeamPit* TeamPit;                           // Drivers Pit

	float RemainingDistance;                     // Distance still to race
	float Reserve;                               // Reserve in [m] to keep fuel for
	float MinFuel;                               // Min fuel of all other teammates using this pit
	int MinLaps;                                 // All Teammates using this pit have to be able to drive this nbr of laps 
	int FuelForLaps;                             // Driver has still fuel for this nbr of laps
	int LapsRemaining;                           // Nbr of laps still to race
	                                             // <<< NEVER CHANGE THIS V1.X
	/*------------------------*/
	/* V1.1 additional fields */             
                                                 // NEVER CHANGE THIS >>> V1.1
	// For beeing accepted while pitting ...
	float StillToGo;                             // Longitudinal distance still to go
	float MoreOffset;                            // Lateral distance to go deeper into pit
	float TooFastBy;                             // Speed over the limit
												 // <<< NEVER CHANGE THIS V1.1
												 // Extend it here if needed but increment VERSION!
} tTeamDriver; 

//
// Data of the one and only team manager
//
typedef struct 
{                                                // NEVER CHANGE THIS >>> V1.X
    tDataStructVersionHeader Header;             // Version and size of this struct
    tDataStructVersionHeader* GarbageCollection; // Linked List of allocated memory blocks used for destruction
	tTeam* Teams;                                // Linked list of teams
	tTeamDriver* TeamDrivers;                    // Linked list of drivers belonging to a team
	tTrack* Track;                               // Track
	tTeamDriver** Drivers;                       // Array of pointers to TeamDrivers 
	int State;                                   // State of team manager
	int Count;                                   // Nbr of drivers in race
	bool PitSharing;                             // Pit sharing activated? 
	float RaceDistance;							 // Distance to race
	                                             // <<< NEVER CHANGE THIS V1.X
	                                             // Extend it here if needed but increment VERSION!
} tTeamManager;
//
// End of implementation
//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// Teammanager
// Robot developer API:
//

//
// Utility functions
//
ROBOTTOOLS_API bool RtIsTeamMate                   // Check wether Car0 is Teammate of Car1
	(const CarElt* Car0, const CarElt* Car1);

//
// Teammanager related functions for use by robots
//
ROBOTTOOLS_API short int RtTeamManagerGetMajorVersion(); // Get major version of used team manager data blocks
ROBOTTOOLS_API short int RtTeamManagerGetMinorVersion(); // Get minor version of used team manager data blocks

ROBOTTOOLS_API void RtTeamManagerShowInfo();     // Switch on team manager info output 
ROBOTTOOLS_API void RtTeamManagerLaps(int Laps); // Nbr of laps to add for MinLaps 

ROBOTTOOLS_API bool RtTeamManagerInit();         // Initialize team manager (is called by RtTeamManagerIndex
                                                 // and RtTeamManagerDump implicitly)

ROBOTTOOLS_API int RtTeamManagerIndex(           // Add a Teammate to it's team (at NewRace)
	CarElt* const Car,                           // -> teammate's car 
	tTrack* const Track,                         // -> track
	tSituation* Situation);                      // -> situaion
                                                 // <- TeamIndex as handle for the subsequent calls

ROBOTTOOLS_API void RtTeamManagerRelease();              // Release team manager at Shutdown

ROBOTTOOLS_API void RtTeamManagerDump(int DumpMode = 0); // For tests: Dump content to console
                                                         // -> DumpMode = 2, dump allways
                                                         // -> DumpMode = 1, dump only after last driver has been added
                                                         // -> DumpMode = 0, dump only after last driver has been added if more than 1 driver is used

ROBOTTOOLS_API void RtTeamManagerStart();                // Start team manager, needed to start if not all robots use it 

//
// Team related functions for use by robots
//
ROBOTTOOLS_API bool RtTeamAllocatePit(   // Try to allocate the pit for use of this teammate 
	const int TeamIndex);

ROBOTTOOLS_API bool RtTeamIsPitFree(     // Check wether the pit to use is available
	const int TeamIndex);

ROBOTTOOLS_API bool RtTeamNeedPitStop(   // Check wether this teammate should got to pit for refueling 
	const int TeamIndex,                 // (depends from the fuel of all other teammates using the same pit)
	float FuelPerM,                      // Fuel consumption per m
	int RepairWanted);                   // Damage to repair at next pitstop

ROBOTTOOLS_API void RtTeamReleasePit(    // Release the pit
	const int TeamIndex);

ROBOTTOOLS_API int RtTeamUpdate(    // Get nbr of laps all other teammates using the same pit can still race 
	const int TeamIndex,
	const int FuelForLaps);         // -> Nbr of laps the driver has fuel for
                                    // <- Min nbr of laps all other teammates using the same pit have fuel for

// V1.0
// Team driver related functions for use by robots
//
ROBOTTOOLS_API float RtTeamDriverRemainingDistance( // Get the remaining distance to race
	const int TeamIndex);                           // Depends on beeing overlapped or not

// V1.1 
ROBOTTOOLS_API tTeamDriver* RtTeamDriverByCar(      // Get the team driver's data by car*
	CarElt* const Car);
//
// End of robot developer API
//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

#endif /* _TEAMMANAGER_H_ */ 
