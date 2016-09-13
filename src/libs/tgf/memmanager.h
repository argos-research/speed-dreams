/***************************************************************************
	memmanager.h -- Interface file for The Memory Manager
                             -------------------                                         
    created              : Wed Nov 12 17:54:00:00 CEST 2014
    copyright            : (C) 2014 by Wolf-Dieter Beelitz
    email                : wdbee@users.sourceforge.net
    version              : $Id$
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef __MEMORYMANAGER__H__
#define __MEMORYMANAGER__H__

#include <cstdio>
#include "tgf.h"

// Use new Memory Manager ...
#ifdef __DEBUG_MEMORYMANAGER__

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
// Interface ...
//----------------------------------------------------------------------------*

//============================================================================*
// Data type WORD
//----------------------------------------------------------------------------*
#ifndef uint16
#define uint16 unsigned short
#endif
//============================================================================*

//============================================================================*
// Data type BYTE
//----------------------------------------------------------------------------*
#ifndef uint8
#define uint8 unsigned char
#endif
//============================================================================*

//============================================================================*
// Prototypes
//----------------------------------------------------------------------------*
TGF_API bool GfMemoryManagerInitialize(void);			
// Release memory manager at Shutdown
TGF_API void GfMemoryManagerRelease(bool Dump = true);	
// Is the memory manager running?
TGF_API bool GfMemoryManagerRunning(void);				
// Setup parameters
TGF_API void GfMemoryManagerSetup(int AddedSpace, uint16 Group = 0);	
// Switch to debug mode:  keep the allocated blocks
TGF_API void GfMemoryManagerDoAccept(void);
// Switch to normal mode: free the blocks
TGF_API void GfMemoryManagerDoFree(void);
// Set the Group ID used while allocation of blocks (old and new blocks)
TGF_API uint16 GfMemoryManagerSetGroup(uint16 Group);
// Save configuration
TGF_API void GfMemoryManagerSaveToFile(void);
//============================================================================*

//----------------------------------------------------------------------------*
// ... Interface
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
// Implementation ...
//----------------------------------------------------------------------------*

//============================================================================*
// Memory manager software version
//----------------------------------------------------------------------------*
#define MM_VERSION 1.0
//============================================================================*

//============================================================================*
// Memory manager states
//----------------------------------------------------------------------------*
#define GF_MM_STATE_NULL 0	// memory manager was created
#define GF_MM_STATE_INIT 1	// memory manager was initialized
#define GF_MM_STATE_USED 2	// memory manager was used
//============================================================================*

//============================================================================*
// Memory manager allocation types
//----------------------------------------------------------------------------*
#define GF_MM_ALLOCTYPE_MEMMAN 0	// allocation for memory manager
#define GF_MM_ALLOCTYPE_NEW 1		// allocation by new
#define GF_MM_ALLOCTYPE_MALLOC 2	// allocation by calloc
//============================================================================*

//============================================================================*
// Memory manager check markers
//----------------------------------------------------------------------------*
#define MM_MARKER_BEGIN 170			// Marker: Start of the block (b 10101010)
#define MM_MARKER_END 1431655765	// (b 01010101 01010101 01010101 01010101)
//============================================================================*

//============================================================================*
// Memory manager histogram
//----------------------------------------------------------------------------*
//#define MAXBLOCKSIZE 4096	// Definition of the max block size for histogram
//#define MAXBLOCKSIZE 1024	// Definition of the max block size for histogram
//#define MAXBLOCKSIZE 64	// Definition of the max block size for histogram
#define MAXBLOCKSIZE 512	// Definition of the max block size for histogram
//============================================================================*

//============================================================================*
// Prototypes of the Memory Manager worker functions
//----------------------------------------------------------------------------*
void* GfMemoryManagerAlloc(size_t size, uint8 type, void* retAddr);
void GfMemoryManagerFree(void* b, uint8 type);
void GfMemoryManagerHistAllocate(size_t size);
void GfMemoryManagerHistFree(size_t size);
//============================================================================*

//============================================================================*
// Block to link allocated memory blocks in a 
// double linked list and some additional flags to check
// integrity of block at call of free
//----------------------------------------------------------------------------*
typedef struct tDSMMLinkBlock
{	
	uint8 Mark;				// Marker to identify it as start of tDSMMLinkBlock
	uint8 Type;				// Type of allocation
	uint16 Grup;  			// Allocation group
	unsigned int BLID;		// ID of allocated memory block
	void* RAdr;				// Return address of new/malloc
	tDSMMLinkBlock* Prev;	// Previous memory block
	tDSMMLinkBlock* Next;	// Next memory block
	unsigned int Size;		// Size of allocated block
} tDSMMLinkBlock;
//============================================================================*

//============================================================================*
// Stack to handle blocks of a defined size
//----------------------------------------------------------------------------*
typedef struct tMMBlockStack
{	
	size_t Size;			// Blocksize handled here
	int Count;				// Capacity of the stack
	int Index;				// Number of available blocks contained - 1
	tDSMMLinkBlock** Block;	// Pointer to an array of pointers of available b.
} tMMBlockStack;
//============================================================================*

//============================================================================*
// Array of stacks
//----------------------------------------------------------------------------*
typedef struct tMMStackBuffer
{	
	unsigned int MaxSize;			// Maximum blocksize that is handled here
	tMMBlockStack Stack[MAXBLOCKSIZE]; 
} tMMStackBuffer;
//============================================================================*

//============================================================================*
// Memory Manager
//----------------------------------------------------------------------------*
typedef struct
{
	tDSMMLinkBlock RootOfList;			// Root of the double linked list
	tDSMMLinkBlock* GarbageCollection;	// Double linked list of allo. blocks
	unsigned int Allocated;				// Current total of alloc. mem. [Bytes]
	unsigned int MaxAllocated;			// Maximum size of alloc. mem. [Bytes]
	unsigned int Requested;				// Current total of requested mem.
	unsigned int MaxRequested;			// Maximum size of requested mem.

	unsigned int BigB;						// Number of big blocks requested
	unsigned int BigBMax;					// Max at same time
	unsigned int Capacity[MAXBLOCKSIZE+1];	// Histogram of the buffer sizes
	unsigned int Hist[MAXBLOCKSIZE+1];		// Histogram of the buffer sizes
	unsigned int HistMax[MAXBLOCKSIZE+1];	// Histogram of max at the same time

	int State;							// State of memory manager
	int AddedSpace;						// Number of bytes added to each block
	uint16 Group;			  			// Allocation group
	size_t Size;						// Size of memory manager
	bool DoNotFree;						// Do not free blocks if flag is set

	tMMStackBuffer StackBuffer;			// Buffer for stacks of blocks

} tMemoryManager;
//============================================================================*
#endif // #ifdef __DEBUG_MEMORYMANAGER__
// ...  Use new Memory Manager
#endif /* __MEMORYMANAGER__H__ */
