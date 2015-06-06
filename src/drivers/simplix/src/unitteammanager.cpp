//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
// unitteammanager.cpp
//--------------------------------------------------------------------------*
// TORCS: "The Open Racing Car Simulator"
// A robot for Speed Dreams-Version 1.4.0/2.X
//--------------------------------------------------------------------------*
// Teammanager
// 
// File         : unitteammanager.cpp
// Created      : 25.11.2007
// Last changed : 2009.07.11
// Copyright    : © 2007-2009 Wolf-Dieter Beelitz
// eMail        : wdb@wdbee.de
// Version      : 3.01.000
//--------------------------------------------------------------------------*
// This program was developed and tested on windows XP
// There are no known Bugs, but:
// Who uses the files accepts, that no responsibility is adopted
// for bugs, dammages, aftereffects or consequential losses.
//
// Das Programm wurde unter Windows XP entwickelt und getestet.
// Fehler sind nicht bekannt, dennoch gilt:
// Wer die Dateien verwendet erkennt an, dass für Fehler, Schäden,
// Folgefehler oder Folgeschäden keine Haftung übernommen wird.
//--------------------------------------------------------------------------*
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// Im übrigen gilt für die Nutzung und/oder Weitergabe die
// GNU GPL (General Public License)
// Version 2 oder nach eigener Wahl eine spätere Version.
//--------------------------------------------------------------------------*
#include "unitglobal.h"
#include "unitcommon.h"

#include "unitteammanager.h"

//==========================================================================*
// Default constructor
//--------------------------------------------------------------------------*
TTeamManager::TTeamManager():
  oCount(0),
  oTeams(NULL)
{
}
//==========================================================================*

//==========================================================================*
// Destructor
//--------------------------------------------------------------------------*
TTeamManager::~TTeamManager()
{
  Clear();
}
//==========================================================================*

//==========================================================================*
// Delete all data
//--------------------------------------------------------------------------*
void TTeamManager::Clear()
{
  for (int I = 0; I < oCount; I++)               // Loop over all Teams
  {
    TTeam* Team = oTeams[I];                     // Get team
	TTeammate* Teammate = Team->Member;          // Get first teammate
	while (Teammate)                             // while teammate != NIL
	{
      TTeammate* ToFree = Teammate;              // Save a pointer
      Teammate = Teammate->Next;                 // get next in list
      delete ToFree;                             // free last teammate
	}
    delete Team;                                 // free team
  }
  delete [] oTeams;                              // Free array
  oTeams = NULL;                                 // Mark as empty 
  oCount = 0;                                    // Adjust counter 
}
//==========================================================================*

//==========================================================================*
// Add a car to his team
//--------------------------------------------------------------------------*
TTeamManager::TTeam* TTeamManager::Add
  (CarElt* oCar, PSituation Situation)
{
  int I;                                         // Loop counter
  oNbrCars = Situation->_ncars;

  TTeammate* NewTeammate = new TTeammate;        // Add car: new teammate 
  NewTeammate->Car = oCar;                       // Set car pointer
  NewTeammate->Index = CarIndex;                 // Set its index
  NewTeammate->Next = NULL;                      // Set next to nil

  for (I = 0; I < oCount; I++)                   // Loop over all teams
  {
    TTeam* Team = oTeams[I];                     // Get Team;
	if (strcmp(CarTeamname,Team->TeamName) == 0) // If Team is cars team
	{                                            //   If Team has allready
      if (Team->Member)                          //   a teammate 
	  {                                          //   Search a teammate
		TTeammate* Teammate = Team->Member;      //   with Next to be
        while (Teammate->Next)                   //   NIL
	      Teammate = Teammate->Next;

	    Teammate->Next = NewTeammate;            // Add new teammate as next 
		Team->Cars[CarDriverIndex] = oCar;
		return Team;                                 
	  }
	  else
	  {
	    Team->Member = NewTeammate;              // This is the first teammate
		return Team;
	  }
	}
  }

  // If the team doesn't exists yet
  TTeam* NewTeam = new TTeam;                    // Create a new team
  NewTeam->Init(oNbrCars);
  NewTeam->TeamName = CarTeamname;               // Set its teamname
  NewTeam->PitState = PIT_IS_FREE;               // Set its pit state
  NewTeam->Member = NewTeammate;                 // set the first teammate  

  for (I = 0; I < oNbrCars; I++) 
  {
    NewTeam->FuelForLaps[I] = 99;                // set nbr of laps
    NewTeam->Cars[I] = NULL;                     // set car as empty
  }
  NewTeam->Cars[CarDriverIndex] = oCar;          // set car
  NewTeam->Count = 1;                            // set counter

  // Expand array of teams
  TTeam** NewTeams = new TTeam*[oCount + 1];     // Create a new array of teams
  if (oTeams)                                    // If old teams exits
    for (I = 0; I < oCount; I++)                 //  loop over all old teams
	{
      NewTeams[I] = oTeams[I];                   //   copy it to the new array 
	  oTeams[I]->Clear();                        //   Clear old pointers
	}
  NewTeams[oCount] = NewTeam;                    // add new team

  delete [] oTeams;                              // Free old team array
  oTeams = NewTeams;                             // Save pointer 
  oCount = oCount + 1;                           // Increment counter

  return NewTeam;
}
//==========================================================================*

//==========================================================================*
// Get a team
//--------------------------------------------------------------------------*
TTeamManager::TTeam* TTeamManager::Team(int Index)
{
  return oTeams[Index];
}
//==========================================================================*

//==========================================================================*
// Check if is teammate
//--------------------------------------------------------------------------*
bool TTeamManager::IsTeamMate(const CarElt* Car0, const CarElt* Car1) const
{
  return strcmp(Car0->_teamname, Car1->_teamname) == 0;
}
//==========================================================================*

//--------------------------------------------------------------------------*
// end of file unitteammanager.cpp
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*

