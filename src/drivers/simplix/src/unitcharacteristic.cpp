//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
// unitcharacteristic.cpp
//--------------------------------------------------------------------------*
// TORCS: "The Open Racing Car Simulator"
// A robot for Speed Dreams-Version 1.4.0/2.X
//--------------------------------------------------------------------------*
// Angepasste (angelernte) Kennlinie
//
// File         : unitcharacteristic.cpp
// Created      : 2007.11.17
// Last changed : 2011.06.02
// Copyright    : © 2007-2011 Wolf-Dieter Beelitz
// eMail        : wdb@wdbee.de
// Version      : 3.01.000
//--------------------------------------------------------------------------*
// This program was developed and tested on windows XP
// There are no known Bugs, but:
// Who uses the files accepts, that no responsibility is adopted
// for bugs, dammages, aftereffects or consequential losses.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
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

#include <math.h>
#include <stdio.h>
#include <memory.h>

#include "unitglobal.h"
#include "unitcharacteristic.h"

//==========================================================================*
// Default constructor
//--------------------------------------------------------------------------*
TCharacteristic::TCharacteristic():
  oData(NULL),
  oOffset(0.0),
  oRange(0.0),
  oCount(0),
  oWeight(0.5)
{
}
//==========================================================================*

 //==========================================================================*
// Constructor
//--------------------------------------------------------------------------*
TCharacteristic::TCharacteristic
  (double Offset, double Max, int Count, double Estimate)
{
  memset(this, 0, sizeof(*this));                // Make Linux delphi like
  oWeight = 0.5;

  oOffset = Offset;
  oRange = Max - Offset;
  oCount = Count;

  oData = new double[Count];
  for (int I = 0; I < Count; I++)
	oData[I] = Estimate;
}
//==========================================================================*

//==========================================================================*
// Destructor
//--------------------------------------------------------------------------*
TCharacteristic::~TCharacteristic()
{
  delete [] oData;
}
//==========================================================================*

//==========================================================================*
// Nbr of increments
//--------------------------------------------------------------------------*
int TCharacteristic::Count() const
{
  return oCount;
}
//==========================================================================*

//==========================================================================*
// Estimate of value at Index
//--------------------------------------------------------------------------*
double TCharacteristic::Estimate(const int index) const
{
  return oData[index];
}
//==========================================================================*

//==========================================================================*
// Estimate of value at position
//--------------------------------------------------------------------------*
double TCharacteristic::Estimate(const double Pos) const
{
  int Index = MakeIndex(Pos);
  return oData[Index];
}
//==========================================================================*

//==========================================================================*
// Measurement of value at index
//--------------------------------------------------------------------------*
void TCharacteristic::Measurement(int Index, double Value)
{
  double Delta = oWeight * (Value - oData[Index]);
  oData[Index] += Delta;
}
//==========================================================================*

//==========================================================================*
// Measurement of value at position
//--------------------------------------------------------------------------*
void TCharacteristic::Measurement(double Pos, double Value)
{
  int Index = MakeIndex(Pos);
  double Delta = oWeight * (Value - oData[Index]);
  oData[Index] += Delta;
}
//==========================================================================*
/*
//==========================================================================*
// Load characteristic from file
//--------------------------------------------------------------------------*
bool TCharacteristic::LoadFromFile(const char* Filename)
{
#ifdef mysecure
  FILE* F;
  int err = myfopen(&F, Filename, "r");
#else
  FILE* F = fopen(Filename, "w");
#endif
  if (F == 0)
    return false;

  fscanf(F, "%d %lf %lf %lf\n",
	&oCount, &oOffset, &oRange, &oWeight);

  delete [] oData;
  oData = new double[oCount];

  for (int I = 0; I < oCount; I++)
  {
    fscanf(F, "%lf\n", &oData[I]);
  }

  fclose(F);

  return true;
}
//==========================================================================*

//==========================================================================*
// Save characteristic to file
//--------------------------------------------------------------------------*
bool TCharacteristic::SaveToFile(const char* Filename)
{
#ifdef mysecure
  FILE* F;
  int err = myfopen(&F, Filename, "w");
#else
  FILE* F = fopen(Filename, "w");
#endif
  if (F == 0)
    return false;

  fprintf(F, "%d %-15.12g %-15.12g %-15.12g\n",
 	oCount, oOffset, oRange, oWeight);

  for (int I = 0; I < oCount; I++)
  {
	fprintf(F, "%-15.12g\n", oData[I]);
  }

  fclose(F);

  return true;
}
//==========================================================================*
*/
//==========================================================================*
// Set weight
//--------------------------------------------------------------------------*
void TCharacteristic::SetWeight(double Weight)
{
  oWeight = Weight;
}
//==========================================================================*

//==========================================================================*
// Get weight
//--------------------------------------------------------------------------*
double TCharacteristic::Weight() const
{
  return oWeight;
}
//==========================================================================*

//==========================================================================*
// Get Index from position
//--------------------------------------------------------------------------*
int TCharacteristic::MakeIndex(const double Pos) const
{
  double T = (oCount - 1) * (Pos - oOffset) / oRange;
  T = MAX(0.0,MIN(T,oCount - 1));
  return (int) floor(T);
}
//==========================================================================*
// end of file unitcharacteristic.cpp
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
