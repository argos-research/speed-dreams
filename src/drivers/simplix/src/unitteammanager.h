//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
// unitteammanager.h
//--------------------------------------------------------------------------*
// TORCS: "The Open Racing Car Simulator"
// A robot for Speed Dreams-Version 1.4.0/2.X
//--------------------------------------------------------------------------*
// Teammanager
//
// File         : unitteammanager.h
// Created      : 25.11.2007
// Last changed : 2013.02.16
// Copyright    : © 2007-2013 Wolf-Dieter Beelitz
// eMail        : wdb@wdbee.de
// Version      : 3.06.000
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
#ifndef _UNITTEAMMANAGER_H_
#define _UNITTEAMMANAGER_H_

#include <car.h>
#include "unitglobal.h"

//==========================================================================*
// Deklaration der Klasse TTeamManager
//--------------------------------------------------------------------------*
class TTeamManager  
{
  public:
	int oNbrCars;

	struct TTeammate
	{
		int	Index;		                         // Index of car in race.
		TTeammate* Next;	                     // The next team member.
		CarElt*	Car;		                     // The car of this team member.
	};

	class TTeam
	{
	  public:
		const char*	TeamName;	                 // Name of team.
		int	PitState;	                         // Request for shared pit.
		TTeammate* Member;                       // The next team member.
//		int FuelForLaps[MAX_NBBOTS];             // Fuel for laps 
//		CarElt* Cars[MAX_NBBOTS];                // Cars
		int* FuelForLaps;                        // Fuel for laps 
		CarElt** Cars;                           // Cars
		int Count;                               // Nbr of Teammates
		int oNbrCars;                            // Nbr of cars in race

	  TTeam():                                   // Default constructor
		PitState(PIT_IS_FREE),                   // Pit is free
		Member(NULL),                            // No members yet
		Count(0)                                 // Nbr of members
	  {
		TeamName = "Empty";	                     // Name of team
	  }

	  ~TTeam()                                   // Destructor
	  {
		delete [] FuelForLaps;
		delete [] Cars;
	  }

	  // Instead copy constructor overhead:
	  void Clear()                               // Clear pointers
	  {                                          // to not delete the 
		FuelForLaps = NULL;                      // memory allocated
		Cars = NULL;                             // by destructor
	  }                                          

      void Init(int NbrCars)
	  {
		oNbrCars = NbrCars;
		FuelForLaps = new int[NbrCars];
		Cars = new CarElt*[NbrCars];
		for (int I = 0; I < oNbrCars; I++)       // Loop over all
		{                                        //   possible members 
		  FuelForLaps[I] = 99;                   //   Fuel for laps 
		  Cars[I] = NULL;                        //   No Cars
		}
	  }

	  int GetMinLaps(CarElt* oCar)               // Get Nbr of laps, all
	  {                                          //  teammates has fuel for 
		int MinLaps = 99;                        // Assume much
		for (int I = 0; I < oNbrCars; I++)       // Loop over all possible
		  if (Cars[I] != oCar)                   // entries!
			MinLaps = MIN(MinLaps,FuelForLaps[I]); // If not self, calculate 

		return MinLaps;
	  }
	};

  public:
	TTeamManager();                              // Default constructor
	~TTeamManager();                             // Destructor

	void Clear();                                // Clear all data
	TTeam* Add                                   // Add a car to its team 
	  (CarElt* oCar, PSituation Situation);      
	TTeam* Team(int Index);                      // Get a team

	bool IsTeamMate                              // Check to be a teammate
	  (const CarElt* Car0, const CarElt* Car1) const;

  private:
	int	oCount;                                  // Nbr of Teams  
	TTeam** oTeams;                              // Pointer to Team array
};
//==========================================================================*
#endif // _UNITTEAMMANAGER_H_
//--------------------------------------------------------------------------*
// end of file unitteammanager.h
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
