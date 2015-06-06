//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
// unitsysfoo.h
//--------------------------------------------------------------------------*
// TORCS: "The Open Racing Car Simulator"
// A robot for Speed Dreams-Version 1.4.0/2.X
//--------------------------------------------------------------------------*
// (C++-Portierung der Unit UnitSysFoo.pas)
//
// File         : unitsysfoo.h
// Created      : 2007.02.20
// Last changed : 2011.06.02
// Copyright    : © 2007-2011 Wolf-Dieter Beelitz
// eMail        : wdb@wdbee.de
// Version      : 3.01.000
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
#ifndef _UNITSYSFOO_H_
#define _UNITSYSFOO_H_

#include <math.h>

//==========================================================================*
// Forewarding
//--------------------------------------------------------------------------*
class TSysFoo;
typedef TSysFoo* PSysFoo;
//==========================================================================*

//==========================================================================*
// Definition unserer Klasse TSysFoo
//--------------------------------------------------------------------------*
// Der Ringpuffer hat eine Länge von 256 Werten (255 nutzbar, einer wird für
// das automatische Löschen benötigt!)
// Die Faltung nutzt den Byte-Inkrement-Overflow aus um eine explizite
// Abfrage am Pufferende einzusparen, also Inc(255) -> 0!
// Eine Überwachung von Overflow-Events durch den Compiler darf deshalb nicht
// aktiviert sein!
//--------------------------------------------------------------------------*
// Der Aufruf ... (N,K)  erzeugt eine Systemfunktion ...
// TSysFoo.Create 1.0 0.0 0.0 0.0 ...
// ...Create(1,1) 0.0 1.0 0.0 0.0 ...
// ...Create(2)   0.5 0.5 0.0 0.0 ...
// ...Create(2,1) 0.0 0.5 0.5 0.0 ...
// ...Create(4,2) 0.0 0.0 0.25 0.25 0.25 0.25 0.0 ...
// usw. wobei N + K < 255 sein muss!
//
// D.h. mit diesen Aufrufen lassen sich sehr bequem Systemfunkionen z.B. für
// die Berechnung von gleitenden Mittelwerten (über N Werte) mit einer
// zusätzlichen Verzögerung (K) erzeugen.
//
// Andere Systemfunktionen können über die Eigenschaft SysFoo[I] gesetzt
// werden. Die Summe der Werte der Systemfunktion muss 1.0 ergeben.
// Es können auch beliebige Werte definiert und dann mit dem
// Aufruf Normalize auf die Summe 1.0 skaliert werden.
//
// Die Überwachung der Integrität ist auskommentiert, da sie hier nicht
// benötigt wird (Funktion Faltung).
//--------------------------------------------------------------------------*
class TSysFoo {
  private:
    bool oAutoNorm;                              // Automatisch normalisieren
    bool oDirty;                                 // Normalisierung nötig
    int oNSysFoo;                                // Länge der Systemfunktion
    float oSignal[256];                          // Ringpuffer für Signal
    float oSysFoo[256];                          // Systemfunktion
    unsigned char oSigIndex;                     // Ringpufferstartindex

    float Get(int Index);                        //
    void Put(int Index, float Value);

  public:
	  TSysFoo                                    // Standardkonstruktor
	    (unsigned int N = 1, unsigned int K = 0);// Länge und Verzögerung

    float Faltung                                // Faltung d. Signalimpulses
      (float Impuls);                            // mit der Systemfunktion

    void Normalize();                            // SysFoo normieren

    int Length();                                // Länge der Systemfunktion

    bool AutoNorm();                             // Autoamtisch normalisieren
    void Reset();                                // Clear buffers
    float SysFoo(int Index);                     // Systemfunktion

};
//==========================================================================*
#endif // _UNITLINALG_H_
//--------------------------------------------------------------------------*
// end of file unitsysfoo.h
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
