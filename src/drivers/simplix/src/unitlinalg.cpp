//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
// unitlinalg.cpp
//--------------------------------------------------------------------------*
// TORCS: "The Open Racing Car Simulator"
// A robot for Speed Dreams-Version 1.4.0/2.X
//--------------------------------------------------------------------------*
// Hilfsfunktionen für 2D- und 3D-Vektoren
// (C++-Portierung der Unit UnitLinAlg.pas)
// 
// File         : unitlinalg.cpp
// Created      : 2007.02.20
// Last changed : 2011.06.02
// Copyright    : © 2007-2011 Wolf-Dieter Beelitz
// eMail        : wdb@wdbee.de
// Version      : 3.01.000
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
#include "unitglobal.h"
#include "unitcommon.h"

#include "unitlinalg.h"

//==========================================================================*
// Result := Value * Value
//--------------------------------------------------------------------------*
Tdble Sqr(const double Value)
{
  return (Tdble)(Value * Value);
};
//==========================================================================*

//==========================================================================*
// 2D: Result := LHS + RHS
//--------------------------------------------------------------------------*
TV2D Add(const TV2D &LHS, const TV2D &RHS)
{
  TV2D Result;
  Result.x = LHS.x + RHS.x;
  Result.y = LHS.y + RHS.y;
  return Result;
};
//==========================================================================*

//==========================================================================*
// 2D: Result := Vector
//--------------------------------------------------------------------------*
TV2D Assign(const TV2D &Vector)
{
  TV2D Result;
  Result.x = Vector.x;
  Result.y = Vector.y;
  return Result;
};
//==========================================================================*

//==========================================================================*
// 2D: Result := CosAlpha(LHS,RHS)
//--------------------------------------------------------------------------*
Tdble CosAlpha(const TV2D &LHS, const TV2D &RHS)
{
  return Mult(LHS,RHS) / (Len(LHS) * Len(RHS));
};
//==========================================================================*

//==========================================================================*
// 2D: Result := CosAlpha(LHS-C,RHS-C)
//--------------------------------------------------------------------------*
Tdble CosAlpha(const TV2D &LHS, const TV2D &RHS, const TV2D &C)
{
  TV2D DLHS, DRHS;

  DLHS = Sub(LHS,C);
  DRHS = Sub(RHS,C);
  return Mult(DLHS,DRHS) / (Len(DLHS) * Len(DRHS));
};
//==========================================================================*

//==========================================================================*
// 2D: Result := Dir(LHS,RHS);
//--------------------------------------------------------------------------*
Tdble Dir(const TV2D &LHS, const TV2D &RHS)
{
  return (Tdble) atan2(LHS.y - RHS.y,LHS.x - RHS.x);
};
//==========================================================================*

//==========================================================================*
// 2D: Result := Dist(LHS,RHS)
//--------------------------------------------------------------------------*
Tdble Dist(const TV2D &LHS, const TV2D &RHS)
{
  return (Tdble) sqrt(Sqr(LHS.x - RHS.x) + Sqr(LHS.y - RHS.y));
};
//==========================================================================*

//==========================================================================*
// 2D: Result := Divide(Vector,Value);
//--------------------------------------------------------------------------*
TV2D Divide(const TV2D &Vector, const Tdble &Value)
{
  TV2D Result;
  Result.x = Vector.x / Value;
  Result.y = Vector.y / Value;
  return Result;
};
//==========================================================================*

//==========================================================================*
// 2D: Result := Len(Vector) (Länge des Vectors)
//--------------------------------------------------------------------------*
Tdble Len(const TV2D &Vector)
{
  return (Tdble) sqrt(Vector.x * Vector.x + Vector.y * Vector.y);
};
//==========================================================================*

//==========================================================================*
// 2D: Result := Neg(Vector)
//--------------------------------------------------------------------------*
TV2D Neg(const TV2D &Vector)
{
  TV2D Result;
  Result.x = -Vector.x;
  Result.y = -Vector.y;
  return Result;
};
//==========================================================================*

//==========================================================================*
// 2D: Result := Normalize(Vector) (Vektor mit Länge 1, Richtung wie Vector)
//--------------------------------------------------------------------------*
TV2D Normalize(const TV2D &Vector)
{
  TV2D Result;
  Tdble L;

  L = Len(Vector);
  Result.x = Vector.x / L;
  Result.y = Vector.y / L;
  return Result;
};
//==========================================================================*

//==========================================================================*/
// 2D: Result := Factor * Vector
//--------------------------------------------------------------------------*
TV2D Mult(Tdble &Factor, const TV2D &Vector)
{
  TV2D Result;
  Result.x = Factor * Vector.x;
  Result.y = Factor * Vector.y;
  return Result;
};
//==========================================================================*

//==========================================================================*
// 2D: Result := LHS * RHS
//--------------------------------------------------------------------------*
Tdble Mult(const TV2D &LHS, const TV2D &RHS)
{
  return (Tdble) (LHS.x * RHS.x + LHS.y * RHS.y);
};
//==========================================================================*

//==========================================================================*
// 2D: Result := Rot(Vector,Center,Arc)
//--------------------------------------------------------------------------*
TV2D Rot(const TV2D &Vector, const TV2D &Center, Tdble &Arc)
{
  TV2D Tmp;
  TV2D D;
  Tdble SinA;
  Tdble CosA;

  D = Sub(Vector,Center);
  SinA = sin(Arc);
  CosA = cos(Arc);
  Tmp.x = (D.x * CosA - D.y * SinA);
  Tmp.y = (D.x * SinA + D.y * CosA);
  return Add(Center,Tmp);
};
//==========================================================================*

//==========================================================================*
// Vorzeichen
//--------------------------------------------------------------------------*
Tdble Sign(Tdble &Value)
{
  if (Value < 0.0)
    return +1.0;
  else
    return -1.0;
};
//==========================================================================*

//==========================================================================*
// Vorzeichen
//--------------------------------------------------------------------------*
Int Sign(Int &Value)
{
  if (Value < 0)
    return +1;
  else
    return -1;
};
//==========================================================================*

//==========================================================================*
// 2D: Result := LHS - RHS
//--------------------------------------------------------------------------*
TV2D Sub(const TV2D &LHS, const TV2D &RHS)
{
  TV2D Result;
  Result.x = LHS.x - RHS.x;
  Result.y = LHS.y - RHS.y;
  return Result;
};
//==========================================================================*





//==========================================================================*
// 3D: Result := LHS + RHS
//--------------------------------------------------------------------------*
TV3D Add(const TV3D &LHS, const TV3D &RHS)
{
  TV3D Result;
  Result.x = LHS.x + RHS.x;
  Result.y = LHS.y + RHS.y;
  Result.z = LHS.z + RHS.z;
  return Result;
};
//==========================================================================*

//==========================================================================*
// 3D: Result := Vector
//--------------------------------------------------------------------------*
TV3D Assign(const TV3D &Vector)
{
  TV3D Result;
  Result.x = Vector.x;
  Result.y = Vector.y;
  Result.z = Vector.z;
  return Result;
};
//==========================================================================*

//==========================================================================*
// 3D: Result := LHS x RHS
//--------------------------------------------------------------------------*
TV3D CrossProd(const TV3D &LHS, const TV3D &RHS)
{
  TV3D Result;
  Result.x = LHS.y * RHS.z - LHS.z * RHS.y;
  Result.y = LHS.z * RHS.x - LHS.x * RHS.z;
  Result.z = LHS.x * RHS.y - LHS.y * RHS.x;
  return Result;
};
//==========================================================================*

//==========================================================================*
// 3D: Result := Dist(LHS,RHS)
//--------------------------------------------------------------------------*
Tdble Dist(const TV3D &LHS, const TV3D &RHS)
{
  return sqrt(Sqr(LHS.x - RHS.x)
    + Sqr(LHS.y - RHS.y)
    + Sqr(LHS.z - RHS.z));
};
//==========================================================================*

//==========================================================================*
// 3D: Result := Divide(Vector,Value);
//--------------------------------------------------------------------------*
TV3D Divide(const TV3D &Vector, const Tdble &Value)
{
  TV3D Result;
  Result.x = Vector.x / Value;
  Result.y = Vector.y / Value;
  Result.z = Vector.z / Value;
  return Result;
};
//==========================================================================*

//==========================================================================*
// 3D: Result := Len(Vector) (Länge des Vectors)
//--------------------------------------------------------------------------*
Tdble Len(const TV3D &Vector)
{
  return sqrt(Vector.x * Vector.x
    + Vector.y * Vector.y
    + Vector.z * Vector.z);
};
//==========================================================================*

//==========================================================================*
// 3D: Result := Neg(Vector)
//--------------------------------------------------------------------------*
TV3D Neg(const TV3D &Vector)
{
  TV3D Result;
  Result.x = -Vector.x;
  Result.y = -Vector.y;
  Result.z = -Vector.z;
  return Result;
};
//==========================================================================*

//==========================================================================*
// 3D: Result := Normalize(Vector) (Vektor mit Länge 1, Richtung wie Vector)
//--------------------------------------------------------------------------*
TV3D Normalize(const TV3D &Vector)
{
  TV3D Result;
  Tdble L;

  L = Len(Vector);
  Result.x = Vector.x / L;
  Result.y = Vector.y / L;
  Result.z = Vector.z / L;
  return Result;
};
//==========================================================================*

//==========================================================================*/
// 3D: Result := Factor * Vector
//--------------------------------------------------------------------------*
TV3D Mult(Tdble &Factor, const TV3D &Vector)
{
  TV3D Result;
  Result.x = Factor * Vector.x;
  Result.y = Factor * Vector.y;
  Result.z = Factor * Vector.z;
  return Result;
};
//==========================================================================*

//==========================================================================*
// 3D: Result := LHS * RHS
//--------------------------------------------------------------------------*
Tdble Mult(const TV3D &LHS, const TV3D &RHS)
{
  return LHS.x * RHS.x + LHS.y * RHS.y + LHS.z * RHS.z;
};
//==========================================================================*

//==========================================================================*
// 3D: Result := RotZ(Vector,Center,Arc)
//--------------------------------------------------------------------------*
TV3D RotZ(const TV3D &Vector, const TV3D &Center, Tdble &Arc, Tdble &DZ)
{
  TV3D Tmp;
  TV3D D;
  Tdble SinA;
  Tdble CosA;

  D = Sub(Vector,Center);
  SinA = sin(Arc);
  CosA = cos(Arc);
  Tmp.x = (D.x * CosA - D.y * SinA);
  Tmp.y = (D.x * SinA + D.y * CosA);
  Tmp.z = D.z + DZ;
  return Add(Center,Tmp);
};
//==========================================================================*

//==========================================================================*
// 3D: Result := LHS - RHS
//--------------------------------------------------------------------------*
TV3D Sub(const TV3D &LHS, const TV3D &RHS)
{
  TV3D Result;
  Result.x = LHS.x - RHS.x;
  Result.y = LHS.y - RHS.y;
  Result.z = LHS.z - RHS.z;
  return Result;
};
//==========================================================================*

//==========================================================================*
// Mixed: Result := Dist(LHS,RHS)
//--------------------------------------------------------------------------*
Tdble Dist(const TV3D &LHS, const TV2D &RHS)
{
  return sqrt(Sqr(LHS.x - RHS.x)
    + Sqr(LHS.y - RHS.y));
};
//==========================================================================*

//==========================================================================*
// Mixed: Result := Dist(LHS,RHS)
//--------------------------------------------------------------------------*
Tdble Dist(const TVec3d &LHS, const TVec3d &RHS)
{
  return sqrt(Sqr(LHS.x - RHS.x)
    + Sqr(LHS.y - RHS.y));
};
//==========================================================================*

//==========================================================================*
// Random number generator: Internal constants
// Works if integer overflow is at 2^31 or heigher
//--------------------------------------------------------------------------*
//#define SD_RANDOM_SEED 0xfded
//#define SD_RANDOM_A    9301
//#define SD_RANDOM_C    49297
//#define SD_RANDOM_M    233280
//--------------------------------------------------------------------------*
// Works if integer overflow is at 2^30 or heigher
//--------------------------------------------------------------------------*
#define SD_RANDOM_SEED 0xfded
#define SD_RANDOM_A    8121
#define SD_RANDOM_C    28411
#define SD_RANDOM_M    134456
unsigned int sd_randomSeed;
//--------------------------------------------------------------------------*
// For normally distributed random numbers
//--------------------------------------------------------------------------*
static int sd_flagHaveOne = 0;
static double sd_secondRandomNumber;
//==========================================================================*

//==========================================================================*
// Random number generator: Init. (Use seed = 0 to get always same sequence)
//--------------------------------------------------------------------------*
void sd_srand(unsigned int seed)
{
	sd_randomSeed = seed ? seed : SD_RANDOM_SEED;
	return;
}
//==========================================================================*

//==========================================================================*
// Random number generator: Get a random number [0 .. SD_RANDOM_M - 1]
//--------------------------------------------------------------------------*
unsigned int sd_randInt()
{
	sd_randomSeed = SD_RANDOM_A * sd_randomSeed + SD_RANDOM_C;
	return (sd_randomSeed % SD_RANDOM_C);
}
//==========================================================================*

//==========================================================================*
// Random number generator: Get a random number [0 .. 1)
//--------------------------------------------------------------------------*
float sd_randFloat()
{
	sd_randomSeed = SD_RANDOM_A * sd_randomSeed + SD_RANDOM_C;
	return ((float)(sd_randomSeed % SD_RANDOM_C) / (float) SD_RANDOM_C);
}
//==========================================================================*

//==========================================================================*
// Random number generator: Normally distributed random numbers
//--------------------------------------------------------------------------*
float sd_randNormalFloat()
{
	double fac, radius, val1, val2;

	if (sd_flagHaveOne == 0)
	{
		do
		{
			val1 = 2.0 * sd_randFloat() - 1.0;
			val2 = 2.0 * sd_randFloat() - 1.0;
			radius = val1 * val1 + val2 * val2;
		} while (radius >= 1.0);
		fac = sqrt(-2.0 * log(radius)/radius);
		sd_secondRandomNumber = val1 * fac;
		sd_flagHaveOne = 1;
		return (float)(val2*fac);
	}
	else
	{
		sd_flagHaveOne = 0;
		return (float)(sd_secondRandomNumber);
	}
}
//==========================================================================*

//--------------------------------------------------------------------------*
// end of file unitlinalg.cpp
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
