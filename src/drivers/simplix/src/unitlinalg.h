//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
// unitlinalg.h
//--------------------------------------------------------------------------*
// A robot for Speed Dreams-Version 2.X simuV4
//--------------------------------------------------------------------------*
// Hilfsfunktionen für 2D- und 3D-Vektoren
// (C++-Portierung der Unit UnitLinAlg.pas)
//
// File         : unitlinalg.h
// Created      : 2007.11.20
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
#ifndef _UNITLINALG_H_
#define _UNITLINALG_H_

#include <math.h>
#include <tgf.h>
#include <track.h>

#include "car.h"         // TORCS
#include "unitvec3d.h"   // TVec3d

//==========================================================================*
// Typdefinition für die Darstellung der Koordinaten
//--------------------------------------------------------------------------*
typedef double TFloat;                             // 64 Bit
typedef double Double;                             // 64 Bit
typedef int Int;                                   // 32 signed integer
typedef unsigned int UInt;                         // 32 unsigned integer
typedef unsigned int Cardinal;                     // 32 unsigned integer
typedef bool Boolean;                              // bool
typedef float Tdble;                               // 32 Bit
typedef float* PTdble;                             // 32 Bit array
typedef void* Pointer;                             // Zeiger
typedef char* PChar;                               // String

typedef tCarElt* PCarElt;                          // TORCS
typedef tTrack* PTrack;

typedef tTrackSeg* PTrackSeg;

//==========================================================================*

//==========================================================================*
// Typdefinition 2D Vektor und 3D Vektor
//--------------------------------------------------------------------------*
typedef struct 
{
  TFloat x, y;
} 
TV2D;
typedef TV2D* PV2D;

typedef t3Dd T3Dd;
typedef t3Dd* P3Dd;
typedef t3Dd TV3D;
typedef t3Dd* PV3D;
//==========================================================================*


//==========================================================================*
// Prototypen
//--------------------------------------------------------------------------*
Tdble Sqr(const double Value);
// 2D:
TV2D Add(const TV2D &LHS, const TV2D &RHS);
TV2D Assign(const TV2D &Vector);
Tdble CosAlpha(const TV2D &LHS, const TV2D &RHS);
Tdble CosAlpha(const TV2D &LHS, const TV2D &RHS, const TV2D &C);
Tdble Dir(const TV2D &LHS, const TV2D &RHS);
Tdble Dist(const TV2D &LHS, const TV2D &RHS);
TV2D Divide(const TV2D &Vector, const Tdble &Value);
Tdble Len(const TV2D &Vector);
TV2D Neg(const TV2D &Vector);
TV2D Normalize(const TV2D &Vector);
TV2D Mult(Tdble &Factor, const TV2D &Vector);
Tdble Mult(const TV2D &LHS, const TV2D &RHS);
TV2D Rot(const TV2D &Vector, const TV2D &Center, Tdble &Arc);
Int Sign(Int &Value);
Tdble Sign(Tdble &Value);
TV2D Sub(const TV2D &LHS, const TV2D &RHS);

// 3D:
TV3D Add(const TV3D &LHS, const TV3D &RHS);
TV3D Assign(const TV3D &Vector);
Tdble Dist(const TV3D &LHS, const TV3D &RHS);
TV3D Divide(const TV3D &Vector, const Tdble &Value);
Tdble Len(const TV3D &Vector);
TV3D Neg(const TV3D &Vector);
TV3D Normalize(const TV3D &Vector);
TV3D Mult(Tdble &Factor, const TV3D &Vector);
Tdble Mult(const TV3D &LHS, const TV3D &RHS);
TV3D CrossProd(const TV3D &LHS, const TV3D &RHS);
TV3D RotZ(const TV3D &Vector, const TV3D &Center, Tdble &Arc, Tdble &DZ);
TV3D Sub(const TV3D &LHS, const TV3D &RHS);

// Mixed
Tdble Dist(const TV2D &LHS, const TV2D &RHS);

// New
Tdble Dist(const TVec3d &LHS, const TVec3d &RHS);

//==========================================================================*
#endif // _UNITLINALG_H_
//--------------------------------------------------------------------------*
// end of file unitlinalg.h
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
