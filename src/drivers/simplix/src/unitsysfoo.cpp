//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
// unitsysfoo.cpp
//--------------------------------------------------------------------------*
// A robot for Speed Dreams-Version 2.X simuV4
//--------------------------------------------------------------------------*
// Systemfunktion
// (C++-Portierung der Unit UnitSysFoo.pas)
// 
// File         : unitsysfoo.cpp
// Created      : 2007.02.20
// Last changed : 2014.11.29
// Copyright    : © 2007-2014 Wolf-Dieter Beelitz
// eMail        : wdbee@users.sourceforge.net
// Version      : 4.05.000
//--------------------------------------------------------------------------*
// Realisierung einer speziellen "Systemfunktion" zur einfachen und schnellen
// Berechnung des Faltungsintegrals eines lineraren Systems.
//
// Mit dieser Systemfunktion können u.a. auch gleitende Mittelwerte sehr
// schnell berechnet oder Ringpuffer für die verzögerte Auswertung
// von Signalen bereitgestellt werden.
//
// Hier wird z.B. die Bewegungserkennung damit realisiert. Wenn ein Fahrzeug
// durch ein Hindernis blockiert ist (Mauer, andere Wagen usw.), dann ändern
// sich die Koordinaten der Position nicht bzw. nur sehr gering.
// Durch den Vergleich von alter Position mit aktueller Position bei frei
// wählbarer Länge der Verzögrung im Ringpuffer kann diese Situation
// zuverlässig erkannt werden.
// Die in anderen Quellen veröffentlichten Ansätze zur Erkennung von
// Blockaden beruhen auf einer Verknüpfung von verschiedenen aktuellen
// Zustandswerten wie der Richtung, was in ausgefallenen Fällen nicht
// immer funktioniert.
//
// Die erforderliche Rechenzeit ist bei diesem Ansatz von der Länge der
// Verzögerung unabhängig und es werden keine Winkelfunktionen benötigt!
// Da die Bewegungserkennung ständig mitlaufen muss, ist das eine
// entscheidende Verbesserung.
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
#include "unitsysfoo.h"

//==========================================================================*
// Konstruktor
//--------------------------------------------------------------------------*
TSysFoo::TSysFoo(unsigned int N, unsigned int K) :
  oAutoNorm(false),
  oDirty(false),
  oNSysFoo(0),
  oSigIndex(0)
{
  unsigned int I;

  if (N < 1)                                     // Keine Division durch 0!
    N = 1;
  if (N + K > 255)                               // Gesamtlänge begrenzen,
    N = 255 - K;                                 //   Verzögerung erhalten!

  for (I = 0; I < 256; I++)
  {
    oSignal[I] = 0.0;
    oSysFoo[I] = 0.0;
  };

  for (I = K; I < K + N; I++)                    // Systemfunktion generieren
    oSysFoo[I] = 1.0f / N;

  oNSysFoo = N + K;                              // Länge der Systemfunktion
};
//==========================================================================*

//==========================================================================*
// SysFoo auslesen
//--------------------------------------------------------------------------*
float TSysFoo::Get(int Index)
{
  return oSysFoo[Index];
};
//==========================================================================*

//==========================================================================*
// SysFoo setzen
//--------------------------------------------------------------------------*
void TSysFoo::Put(int Index, float Value)
{
  oDirty = true;                                 // Änderungen erfordern
  oSysFoo[Index] = Value;                        // ggf. eine Normalisierung
  if (oAutoNorm)                                 // Wenn aktiviert,
    Normalize();                                   //   automatisch normieren
};
//==========================================================================*

//==========================================================================*
// SysFoo normieren
//--------------------------------------------------------------------------*
void TSysFoo::Normalize()
{

  int I;
  float Sum;

  if (oDirty)                                    // Falls eforderlich
  {                                              // auf Summe = 1.0
    Sum = 0.0;                                   // normieren
    for (I = 0; I < 256; I++)
      Sum += oSysFoo[I];
    for (I = 0; I < 256; I++)
      oSysFoo[I] /= Sum;

    oDirty = false;                              // Normierung Erledigt
  }
};
//==========================================================================*

//==========================================================================*
// Faltung der Systemfunktion mit dem Eingangsimpuls
//--------------------------------------------------------------------------*
float TSysFoo::Faltung(float Impuls)
{
  int I;
  unsigned char J; 

  oSignal[oSigIndex] = 0.0;                      // Alte Werte löschen
  oSigIndex++;                                   // Start im Ringpuffer
  J = oSigIndex;                                 // Faltungsindex
  for (I = 0; I < oNSysFoo; I++)                 // Über die Länge der
  {                                              //   Faltung
    oSignal[J] += oSysFoo[I] * Impuls;
    J++;
//	if (J > 255)
//	  J = 0;
  };
  return oSignal[oSigIndex];                     // Aktuelles Ausgangssignal
};
//==========================================================================*

//==========================================================================*
// Reset buffer
//--------------------------------------------------------------------------*
void TSysFoo::Reset()
{
  for (int I = 0; I < oNSysFoo; I++) 
    oSignal[I] = 0.0;
};
//==========================================================================*

//--------------------------------------------------------------------------*
// end of file unitsysfoo.cpp
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
