//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
// unitcharacteristic.h
//--------------------------------------------------------------------------*
// A robot for Speed Dreams-Version 2.X simuV4
//--------------------------------------------------------------------------*
// Angepasste (angelernte) Kennlinie
//
// File         : unitcharacteristic.h
// Created      : 2007.11.17
// Last changed : 2014.11.29
// Copyright    : © 2007-2014 Wolf-Dieter Beelitz
// eMail        : wdbee@users.sourceforge.net
// Version      : 4.05.000
//--------------------------------------------------------------------------*
// Das Programm wurde unter Windows XP entwickelt und getestet.
// Fehler sind nicht bekannt, dennoch gilt:
// Wer die Dateien verwendet erkennt an, dass für Fehler, Schäden,
// Folgefehler oder Folgeschäden keine Haftung übernommen wird.
//
// Im übrigen gilt für die Nutzung und/oder Weitergabe die
// GNU GPL (General Public License)
// Version 2 oder nach eigener Wahl eine spätere Version.
//--------------------------------------------------------------------------*
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//--------------------------------------------------------------------------*
#ifndef _UNITCHARACTERISTIC_H_
#define _UNITCHARACTERISTIC_H_

#include "unitglobal.h"

//==========================================================================*
// Class TCharacteristic  
//--------------------------------------------------------------------------*
class TCharacteristic  
{
  public:
	TCharacteristic();                           // Default constructor

	TCharacteristic                              // Constructor
	  (double Offset,                            //   Minimum
	  double Max,                                //   Maximum
	  int Count,                                 //   Nbr of increments 
	  double Estimate);                          //   Initial estimate

	~TCharacteristic();                          // Destructor 

	int	Count() const;                           // Nbr of increments

	double Estimate(const int Index) const;      // Estimae at index
	double Estimate(const double Pos) const;     // Estimate at pos

	void Measurement(int Index, double Value);   // Measurement value at index
	void Measurement(double Pos, double Value);  // Measurement value at pos
/*
	bool LoadFromFile(const char* Filename);     // Load characteristic from file
	bool SaveToFile(const char* Filename);       // Save characteristic to file
*/
	void SetWeight(double Weight);               // Set Weight
	double Weight() const;                       // Get Weight

  private:
	int MakeIndex(const double Coord) const;     // Get index from position

  private:
	double*	oData;                               // Measurements
	double oOffset;                              // Minimum offset
	double oRange;                               // Maximum - minimum
	int	oCount;                                  // Nbr of increments
	double oWeight;                              // Weight

};
//==========================================================================*
#endif // _UNITCHARACTERISTIC_H_
//--------------------------------------------------------------------------*
// end of file unitcharacteristic.h
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
