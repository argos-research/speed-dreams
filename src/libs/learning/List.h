/* -*- Mode: C++; -*- */
/* VER: $Id: List.h 2917 2010-10-17 19:03:40Z pouillot $ */
// copyright (c) 2004 by Christos Dimitrakakis <dimitrak@idiap.ch>
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef LIST_H
#define LIST_H

#include <cstdlib>
#include <cstdio>
#include <cassert>

#include "learning.h"


/// A list item
typedef struct ListItem {
	void* obj; ///< data
	void (*free_obj) (void* obj); ///< free hook
	struct ListItem* prev; ///< previous item
	struct ListItem* next; ///< next item
} LISTITEM;


/**
   \brief A very simple list structure
   
   The structure is initialised empty. The search function pointer is
   ListLinearSearchRetrieve.
   \return NULL if nothing could be
   created.
   \see ListLinearSearchRetrieve
*/
typedef struct List {
	LISTITEM* curr; ///< current item
	LISTITEM* head; ///< head item
	LISTITEM* tail; ///< tail item
	int n; ///< number of items
	/// Method by which to search objects
	LISTITEM* (*retrieve) (struct List* list, void* ptr);
} LIST;


/// Create a new list
LEARNING_API LIST* List(void);
/// Get the size of the list
LEARNING_API int ListSize(LIST* list);
/// Append an item to the list
LEARNING_API LISTITEM* ListAppend(LIST* list, void* p);
/// Append an item to the list with free hook
LEARNING_API LISTITEM* ListAppend(LIST* list, void* p, void (*free_obj) (void* obj));
/// Move to the first list item
LEARNING_API LISTITEM* FirstListItem(LIST* list);
/// Move to the last list item
LEARNING_API LISTITEM* LastListItem(LIST* list);
/// Advance one item
LEARNING_API LISTITEM* NextListItem(LIST* list);
/// Remove the topmost item of the list (also frees obj memory)
LEARNING_API int PopItem(LIST* list);
/// Clear the list
LEARNING_API int ClearList(LIST* list);
/// Finds the LISTITEM pointer corresponding to the data
LEARNING_API LISTITEM* FindItem (LIST* list, void* ptr);
/// Get the nth item of the list
LEARNING_API LISTITEM* GetItem (LIST* list, int n);

LEARNING_API LISTITEM* ListItem(void* ptr, void (*free_obj) (void* obj));
LEARNING_API LISTITEM* GetNextItem(LISTITEM* ptr);
LEARNING_API LISTITEM* GetPrevItem(LISTITEM* ptr);
LEARNING_API LISTITEM* LinkNext(LISTITEM* src, void* ptr, void (*free_obj) (void* obj));
LEARNING_API LISTITEM* LinkPrev(LISTITEM* src, void* ptr, void (*free_obj) (void* obj));
LEARNING_API int FreeListItem(LIST* list, LISTITEM* ptr);
LEARNING_API int RemoveListItem(LIST* list, LISTITEM* ptr);

LEARNING_API LISTITEM* ListLinearSearchRetrieve (struct List* list, void* ptr);

#endif
