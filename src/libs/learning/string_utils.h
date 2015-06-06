// Copyright (C) 2004 Johnny Mariethoz (Johnny.Mariethoz@idiap.ch)
//                and Samy Bengio (bengio@idiap.ch)
//                and Ronan Collobert (collober@iro.umontreal.ca)
//                and Christos Dimitrakakis (dimitrak@idiap.ch)
//
// This file is based on code from Torch. Release II.
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef STRING_UTILS_INC
#define STRING_UTILS_INC

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "learning.h"
#include "learn_debug.h"

/** 
	\file string_utils.h

	Some simple functions for string operations.

    \author Samy Bengio (bengio@idiap.ch)
    \author Johnny Mariethoz (Johnny.Mariethoz@idiap.ch)
    \author Ronan Collobert (collober@iro.umontreal.ca)
    \author Christos Dimitrakakis (dimitrak@idiap.ch)
*/
//@{
/** \brief The StringBuffer structure stores buffers of strings */
typedef struct StringBuffer_ {
     char* c; ///<This is the buffer.
     char* string; ///<This is the string
     unsigned int length; ///<This is the buffer length
} StringBuffer;

/// Make a new stringbuffer
LEARNING_API StringBuffer* NewStringBuffer (int length);

/// Given a pointer to a stringbuffer pointer, free it and clear it
LEARNING_API void FreeStringBuffer (StringBuffer** s);

LEARNING_API StringBuffer* SetStringBufferLength (StringBuffer* s, unsigned int l);

/** Returns the name of a file without leading pathname.
    (It's not a new string, but a pointer in the given string)
 */
LEARNING_API char *strBaseName(char *filename);

/** Returns a fresh copy of the name of a file without suffix.
    (Trailing chars after c) You have to free the memory!
*/
LEARNING_API char *strRemoveSuffix(char *filename, char c='.');

/** Returns the concatenation #n# strings.
    The strings are the parameters given after #n#;
    You have to free the memory!
*/
LEARNING_API char *strConcat(int n, ...);

/** Prints a message */
LEARNING_API void message(const char* msg, ...);
/** Reads a string dynamically */
LEARNING_API StringBuffer* read_string (FILE* f, StringBuffer* s);
/** copies a string */
LEARNING_API char* string_copy (char* c);
//@}



#endif
