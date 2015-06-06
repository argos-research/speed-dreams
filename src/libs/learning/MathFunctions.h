/* -*- Mode: c++ -*- */
/* VER: $Id: MathFunctions.h 2917 2010-10-17 19:03:40Z pouillot $ */
// copyright (c) 2004 by Christos Dimitrakakis <dimitrak@idiap.ch>
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef MATH_FUNCTIONS_H
#define MATH_FUNCTIONS_H

#include "learning.h"
#include "real.h"


LEARNING_API int ArgMin (int n, real* x);
LEARNING_API int ArgMax (int n, real* x);
LEARNING_API real SmoothMaxGamma (real f1, real f2, real lambda, real c);
LEARNING_API real SmoothMaxPNorm (real f1, real f2, real p);
LEARNING_API void SoftMax (int n, real* Q, real* p, real beta);
LEARNING_API void SoftMin (int n, real* Q, real* p, real beta);
LEARNING_API void Normalise (real* src, real* dst, int n_elements);
LEARNING_API real EuclideanNorm (real* a, real* b, int n);
LEARNING_API real SquareNorm (real* a, real* b, int n);
LEARNING_API real LNorm (real* a, real* b, int n, real p);
LEARNING_API real Sum (real* a, int n);

template<class T>
inline const T sign(const T& x)
{
	if (x>0) {
		return 1;
	} else if (x<0) {
		return -1;
	} else {
		return 0;
	}
} 

#endif
