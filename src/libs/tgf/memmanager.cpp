/***************************************************************************
	memmanager.cpp -- The Memory Manager
                             -------------------                                         
    created              : Wed Nov 12 17:54:00:00 CEST 2014
    copyright            : (C) 2014 by Wolf-Dieter Beelitz
    email                : wdbee@users.sourceforge.net
    version              : $Id:$
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "memmanager.h"
#ifdef __DEBUG_MEMORYMANAGER__

#include <portability.h>

#include "tgf.h"

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
// Configuration ...
//----------------------------------------------------------------------------*
#ifdef WIN32
// Windows ...
	#if defined(__MINGW32__)
	// MinGW ...
		#include <windows.h>
		#include <crtdbg.h>
		#include <assert.h>
		#define GetRetAddrs GCCRetAddrs
	// ... MinGW
	#else
	// VC++ ...
		#include <windows.h>
		#include <crtdbg.h>
		#include <assert.h>
		#include <stdio.h>
		#include <intrin.h>
		#pragma intrinsic(_ReturnAddress)
		#undef ANSI_ISO	// Old VC++ versions returning NULL instead of exception
		#define GetRetAddrs _ReturnAddress
	// ... VC++
	#endif
// ... Windows
#else
// Linux ...
	#include <unistd.h> // getcwd, access
	#define ANSI_ISO	// ANSI/ISO compliant behavior: exeption if allo failed
// ... Linux
#endif
//----------------------------------------------------------------------------*
// ... Configuration
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
// Implementation ...
//
// Private variables
//
//----------------------------------------------------------------------------*
static tMemoryManager* GfMM = NULL;		// The one and only memory manager!
static unsigned int GfMM_Counter = 0;	// Counter of memory blocks
//============================================================================*
#if defined(__MINGW32__)
//============================================================================*
// GCC allows to set the level parameter, we use 0 to get the same as for VC++
//----------------------------------------------------------------------------*
void* GCCRetAddrs(void)
{
	return __builtin_return_address(0);
}
//============================================================================*
#endif

//----------------------------------------------------------------------------*
// ... Implementation
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
// Interface ...
//----------------------------------------------------------------------------*

//============================================================================*
// API: Override the global new operator
//----------------------------------------------------------------------------*
//ExternC void* operator new (size_t size)
void* operator new (size_t size)
{
	void* retAddr = GetRetAddrs();
	return GfMemoryManagerAlloc(size, GF_MM_ALLOCTYPE_NEW, retAddr);
}
//============================================================================*

//============================================================================*
// API: Override the global delete operator
//----------------------------------------------------------------------------*
//ExternC void operator delete (void* b)
void operator delete (void* b)
{
	GfMemoryManagerFree(b, GF_MM_ALLOCTYPE_NEW);
}
//============================================================================*

//============================================================================*
// API: Override malloc
//----------------------------------------------------------------------------*
ExternC void* _tgf_win_malloc(size_t size)
{
	void* retAddr = GetRetAddrs();
	return GfMemoryManagerAlloc(size, GF_MM_ALLOCTYPE_MALLOC, retAddr);
}
//============================================================================*

//============================================================================*
// API: Override free
//----------------------------------------------------------------------------*
ExternC void _tgf_win_free(void* b)
{
	GfMemoryManagerFree(b, GF_MM_ALLOCTYPE_MALLOC);
}
//============================================================================*

//============================================================================*
// API: Override calloc
//----------------------------------------------------------------------------*
ExternC void* _tgf_win_calloc(size_t num, size_t size)
{
	void* retAddr = GetRetAddrs();
	void* p = GfMemoryManagerAlloc(num*size, GF_MM_ALLOCTYPE_MALLOC, retAddr);
	memset(p, 0, num * size);
	return p;
}
//============================================================================*

//============================================================================*
// API: Override recalloc
//----------------------------------------------------------------------------*
ExternC void* _tgf_win_realloc(void* memblock, size_t size)
{
	void* retAddr = GetRetAddrs();

	if (size == 0) 
	{
		_tgf_win_free(memblock);
		return NULL;
	}

	void* p = GfMemoryManagerAlloc(size, GF_MM_ALLOCTYPE_MALLOC, retAddr);
	if (p == NULL) 
	{
		return NULL;
	}

	if (memblock != NULL) 
	{
		if (GfMemoryManagerRunning())
		{
			// Needed additional space
			int bsize = 
				sizeof(tDSMMLinkBlock)	// Data of Memory Manager
				+ sizeof(int)			// Requested size of the block	
				+ sizeof(int)			// Marker to detect corrupted blocks	
				+ GfMM->AddedSpace;		// Security margin for debugging

			size_t s = MIN(*(int*)((char*) memblock - bsize), (int) size);

			memcpy(p, memblock, s);
		}
		else
		{
			size_t s = MIN(*(int*)((char*)memblock 
				- sizeof(int)), (int)size);

			memcpy(p, memblock, s);
		}

		_tgf_win_free(memblock);
	}
	return p;
}
//============================================================================*

//============================================================================*
// API: Override strdup
//----------------------------------------------------------------------------*
ExternC char * _tgf_win_strdup(const char* str)
{
	void* retAddr = GetRetAddrs();
	
	char * s = (char*) GfMemoryManagerAlloc(
		strlen(str)+1,GF_MM_ALLOCTYPE_MALLOC,retAddr);

	strcpy(s,str);

	return s;
}
//============================================================================*

//----------------------------------------------------------------------------*
// ... Interface 
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
// Implementation ...
//----------------------------------------------------------------------------*

//============================================================================*
// Load configuration from configuration file
//----------------------------------------------------------------------------*
void GfMemoryManagerLoadFromFile(tMemoryManager* MM)
{
    char buf[255+1]; // MAX_XML_SECTION_NAME

	char* XmlFileName = "E:\\Root\\SD\\SD_V2.2\\data\\memmanager.xml";

	void* Handle = GfParmReadFile(XmlFileName, GFPARM_RMODE_REREAD, true, false);
	if (Handle != NULL)
	{

		// Write header data
		tdble version =
			GfParmGetNum(Handle, "CONFIGURATION", "version", NULL, (tdble) MM_VERSION);
		int maxBlockSize = (int)
			GfParmGetNum(Handle, "CONFIGURATION", "max block size", NULL, (tdble) MAXBLOCKSIZE);
		MM->AddedSpace = (int)
			GfParmGetNum(Handle, "CONFIGURATION", "additional space", NULL, (tdble) MM->AddedSpace);
		int sizeOfMemoryManager = (int)
			GfParmGetNum(Handle, "CONFIGURATION", "size", NULL, (tdble) MM->Size);

		if (maxBlockSize > MAXBLOCKSIZE)
			maxBlockSize = MAXBLOCKSIZE;

		// Read block data
		for (int I = 0; I < maxBlockSize; I++)
		{
			tMMBlockStack* Stack = &(MM->StackBuffer.Stack[I]);

			// Prepare the section depending on the part number
			snprintf(buf,sizeof(buf),"BLOCK/%d",I);

			Stack->Size = (int)
				GfParmGetNum(Handle, buf, "block size", NULL, (tdble) Stack->Size);
			MM->Capacity[I+1] = (int) 
				(GfParmGetNum(Handle, buf, "capacity", NULL, 0));
		}

		// Release handle
		GfParmReleaseHandle(Handle);
	}
}
//============================================================================*

//============================================================================*
// Save needed number of blocks per block size into configuration file
//----------------------------------------------------------------------------*
void GfMemoryManagerSaveToFile(void)
{
    char buf[255+1]; // MAX_XML_SECTION_NAME

	char* XmlFileName = ".\\memmanager.xml";
	char* FileType = "Memory Manager Configuration File";
	char* AuthorName = "Wolf-Dieter Beelitz";

	void* Handle = GfParmReadFile(XmlFileName, GFPARM_RMODE_REREAD, true, false);
	if (Handle == NULL)
	{
		// Create an empty file
		Handle = GfParmReadFile(XmlFileName, GFPARM_RMODE_REREAD | GFPARM_RMODE_CREAT, false, false);
		// Write header to xml file
		GfParmWriteFileSDHeader (XmlFileName, Handle, FileType, AuthorName, false);	
		// Release handle
		GfParmReleaseHandle(Handle);
		// Open created car type track setup file
		Handle = GfParmReadFile(XmlFileName, GFPARM_RMODE_REREAD, true, false);
	}

	// Write header data
	GfParmSetNum(Handle, "CONFIGURATION", "version", NULL, MM_VERSION);
	GfParmSetNum(Handle, "CONFIGURATION", "max block size", NULL, MAXBLOCKSIZE);
	GfParmSetNum(Handle, "CONFIGURATION", "additional space", NULL, (tdble) GfMM->AddedSpace);
	GfParmSetNum(Handle, "CONFIGURATION", "size", NULL, (tdble) GfMM->Size);

	fprintf(stderr,"Capacity changes:\n");

	// Write block data
	for (int I = 0; I < MAXBLOCKSIZE; I++)
	{
		tMMBlockStack* Stack = &(GfMM->StackBuffer.Stack[I]);

		// Prepare the section depending on the part number
		snprintf(buf,sizeof(buf),"BLOCK/%d",I);

		unsigned int Capacity = (int) MAX(GfMM->HistMax[I+1],GfMM->Capacity[I+1]);

		// Write data
		GfParmSetNum(Handle, buf, "block size", NULL, (tdble) Stack->Size, 0, MAXBLOCKSIZE);
		GfParmSetNum(Handle, buf, "capacity", NULL, (tdble) Capacity, 0, 1024*64);
		GfParmSetNum(Handle, buf, "planned", NULL, (tdble) GfMM->Capacity[I+1], 0, 1024*64);

		if (Capacity != GfMM->Capacity[I+1])
			fprintf(stderr,"block size %d capacity +%d\n",Stack->Size,Capacity - GfMM->Capacity[I+1]);
	}

	// Save data and header to xml file
	GfParmWriteFileSDHeader (XmlFileName, Handle, FileType, AuthorName, false);	

	// Release handle
	GfParmReleaseHandle(Handle);

}
//============================================================================*

//============================================================================*
// Create the one and only global memory manager (allocate memory for data)
//----------------------------------------------------------------------------*
tMemoryManager* GfMemoryManager(void)
{
	// Create the  Memory Manager
    tMemoryManager* MemoryManager = (tMemoryManager*)  
		malloc(sizeof(tMemoryManager));            

	// Set the pointer to the linking block
	MemoryManager->GarbageCollection = (tDSMMLinkBlock*) MemoryManager;

	// Initialize variables of the Memory Manager
	MemoryManager->Allocated = 0;
	MemoryManager->MaxAllocated = 0;
	MemoryManager->Requested = 0;
	MemoryManager->MaxRequested = 0;

	MemoryManager->BigB = 0;
	MemoryManager->BigBMax = 0;
	for (int I = 0; I <= MAXBLOCKSIZE; I++)
	{
		MemoryManager->Hist[I] = 0;
		MemoryManager->HistMax[I] = 0;
	}

	MemoryManager->State = GF_MM_STATE_NULL;     
	MemoryManager->AddedSpace = 0;
	MemoryManager->Group = 0;
	MemoryManager->Size = sizeof(tMemoryManager);	
	MemoryManager->DoNotFree = false;

	// Initialize variables of the Linking Block
	MemoryManager->RootOfList.Mark = MM_MARKER_BEGIN;
	MemoryManager->RootOfList.Type = GF_MM_ALLOCTYPE_MEMMAN;
	MemoryManager->RootOfList.Grup = 0;
	MemoryManager->RootOfList.BLID = GfMM_Counter++;	
	MemoryManager->RootOfList.RAdr = NULL;
	MemoryManager->RootOfList.Prev = NULL;
	MemoryManager->RootOfList.Next = NULL;
	MemoryManager->RootOfList.Size = sizeof(tMemoryManager);	

	for (int I = 0; I < MAXBLOCKSIZE; I++)
	{
		MemoryManager->Capacity[I] = 0;
	}

	GfMemoryManagerLoadFromFile(MemoryManager);

	for (int I = 0; I < MAXBLOCKSIZE; I++)
	{
		tMMBlockStack* Stack = &(MemoryManager->StackBuffer.Stack[I]);
		Stack->Size = I + 1;
		Stack->Count = MemoryManager->Capacity[I];
		Stack->Index = 0;
		Stack->Block = (tDSMMLinkBlock**) malloc (Stack->Count * sizeof(tDSMMLinkBlock*));
		for (int J = 0; J < Stack->Count; J++)
			Stack->Block[J] = NULL;
	}

	return MemoryManager;
}
//============================================================================*

//============================================================================*
// Add block to stack
//----------------------------------------------------------------------------*
void GfMemoryManagerBlockFree (size_t size, tDSMMLinkBlock* c)
{
	if ((size > 0) && (size <= MAXBLOCKSIZE))
	{
		tMMBlockStack* Stack = &(GfMM->StackBuffer.Stack[size-1]);
		if (c->Size != Stack->Size)
			assert(0);
		if (Stack->Index < Stack->Count - 1)
		{
			Stack->Block[(Stack->Index)] = c;
			++Stack->Index;
			return;
		}
	}

	GlobalFree(c);
}
//============================================================================*

//============================================================================*
// Get block from stack
//----------------------------------------------------------------------------*
tDSMMLinkBlock* GfMemoryManagerBlockAllocate (size_t size, size_t bsize)
{
	if ((size > 0) && (size <= MAXBLOCKSIZE))
	{
		tMMBlockStack* Stack = &(GfMM->StackBuffer.Stack[size-1]);
		if (Stack->Index > 0)
		{
			Stack->Index--;
			tDSMMLinkBlock* c =	Stack->Block[(Stack->Index)];
			if (c->Size != size)
				assert(0);
			return c;
		}
	}

	tDSMMLinkBlock* c = (tDSMMLinkBlock*) GlobalAlloc(GMEM_FIXED, bsize);
	return c; 
}
//============================================================================*

//============================================================================*
// Allocate memory
//----------------------------------------------------------------------------*
void* GfMemoryManagerAlloc (size_t size, uint8 type, void* retAddr)
{
	if (GfMemoryManagerRunning())
	{
		// Need additional space for linked list and other data
		int bsize = 
			sizeof(tDSMMLinkBlock)	// Data of Memory Manager
			+ size					// Space allocated for caller	
			+ sizeof(int)			// Marker to detect corrupted blocks	
			+ GfMM->AddedSpace;		// Security margin for debugging

		// Allocate memory block
		//tDSMMLinkBlock* c = (tDSMMLinkBlock*) GlobalAlloc(GMEM_FIXED, bsize); 
		tDSMMLinkBlock* c = GfMemoryManagerBlockAllocate(size, bsize);


		// Check pointer to the block
		if (c == NULL)				
#ifdef ANSI_ISO
			throw std::bad_alloc();	// ANSI/ISO compliant behavior
#else
			return c;
#endif
		GfMM->Allocated += bsize;
		GfMM->MaxAllocated = MAX(GfMM->MaxAllocated,GfMM->Allocated);
		GfMM->Requested += size;
		GfMM->MaxRequested = MAX(GfMM->MaxRequested,GfMM->Requested);

		// Put block into the double linked list
		if (GfMM->RootOfList.Next != NULL)
		{
			tDSMMLinkBlock* n = GfMM->RootOfList.Next;
			n->Prev = c;
			c->Next = n;
		}
		else
			c->Next = NULL;	// Is last block

		GfMM->RootOfList.Next = c;
		c->Prev = &GfMM->RootOfList;

		// Setup block data
		c->Mark = MM_MARKER_BEGIN;
		c->Type = type;
		c->Size = size;
		c->RAdr = retAddr;
		c->Grup = GfMM->Group; // Set "color" of block
		int ID = c->BLID = GfMM_Counter++;
/*
		// Dump the "colored" blocks
		if (c->Grup > 0)
			fprintf(stderr,"+ Color: %d ID: %d Size: %d Allocated at %p\n",
			  c->Grup,c->BLID,c->Size,c->RAdr);
*/
		// Update statistics
		GfMemoryManagerHistAllocate(size);

		// Get address to the marker at the end
		char* e = (char*) c;
		int* m = (int*) (e + bsize - sizeof(int));
		*m = MM_MARKER_END;

		void* b = (void*) (c + 1);  //c is still pointing to the data

		// Hunting memory leaks ...
#define	IDTOSTOP 12144 // ID of block you are looking for
		char buf[10];
		snprintf(buf,sizeof(buf),"%p",c->RAdr);
		buf[0] = 'X';
		buf[1] = 'X';
		buf[2] = 'X';
		buf[3] = 'X';
		if ((ID == IDTOSTOP) || (strncmp(buf,"XXXX511C",8) == 0))
		{
			ID = 0;	// set breakpoint here 
					// to stop at allocation of 
					// block with ID = IDTOSTOP
		}
		// ... Hunting memory leaks

		return b;
	}
	else
	{
		return (void*) GlobalAlloc(GMEM_FIXED, size); 
	}
}
//============================================================================*

//============================================================================*
// Release memory
//----------------------------------------------------------------------------*
void GfMemoryManagerFree (void* b, uint8 type)
{
	if (b == NULL)	// If already done
		return;		//   return without action

	if (GfMemoryManagerRunning())
	{
		// Get start of data block ...
		tDSMMLinkBlock* c = ((tDSMMLinkBlock*) b - 1);
//		--c;
		// ... Get start of data block

		// Get address to the marker at the end
		// Need additional space for linked list and other data
		int bsize = 
			sizeof(tDSMMLinkBlock)	// Data of Memory Manager
			+ c->Size				// Space allocated for caller	
			+ sizeof(int)			// Marker to detect corrupted blocks	
			+ GfMM->AddedSpace;		// Security margin for debugging

		char* e = (char*) c;
		int* m = (int*) (e + bsize - sizeof(int));

		// Hunting corrupted blocks ...
		if (c->BLID == IDTOSTOP)
		{
			c->BLID = 0; // set breakpoint here 
		}
		// ... Hunting corrupted blocks

		// Check block
		if ((c->Mark != MM_MARKER_BEGIN) || (*m != MM_MARKER_END))
		{
			// Block is corrupted
			fprintf(stderr,
				"Called for corrupted block; %d; at; %p;\n",c->BLID,c);
		}
		// Check call type (new/delete or malloc/free)
		else if (c->Type != type) 
		{	// Show error message if new/free or malloc/delete 
			fprintf(stderr,
				"Called by wrong call type for block; %d; at address; %p; (%d)\n",
				c->BLID,c,c->Type);
		}
		else
		{	// Update counter
			GfMM->Allocated -= bsize;
			GfMM->Requested -= c->Size;

			// Update statistics
			GfMemoryManagerHistFree(c->Size);

			// Take the block out of the double linked list
			tDSMMLinkBlock* n = c->Next;
			tDSMMLinkBlock* p = c->Prev;
			p->Next = n;
			if ((n != NULL) && (n->Mark == MM_MARKER_BEGIN))
				n->Prev = p;
/*
			// Dump the "colored" blocks
			if ( (GfMM->Group == 0) && (c->Grup > 0) )
				fprintf(stderr,"- Color: %d ID: %d Size: %d Allocated at %p\n",
				  c->Grup,c->BLID,c->Size,c->RAdr);
*/
			// Check release mode
			if (GfMM->DoNotFree)
				return;	// accept the leak for debugging

			// Release the allocated memory
			// GlobalFree(c);
			GfMemoryManagerBlockFree(c->Size, c);
		}
	}
	else
	{
		// Handle blocks that are allocated without the Memory Manager
		// or stayed in the list while release the Memory Manager
		GlobalFree(b); 
	}
}
//============================================================================*

//============================================================================*
// Setup data of memory manager
//----------------------------------------------------------------------------*
void GfMemoryManagerSetup(int AddedSpace)
{
	if (GfMM != NULL)
	{
		if (GfMM->State != GF_MM_STATE_INIT)
		{
			// Setup data for memory manager
			GfMM->AddedSpace = AddedSpace;
		}
		GfMM->State = GF_MM_STATE_INIT; 
	}
}
//============================================================================*

//============================================================================*
// Initialize the global memory manager
// Returns true if Manager was created, false if it existed before
//----------------------------------------------------------------------------*
bool GfMemoryManagerInitialize(void)
{
	if (GfMM == NULL)
	{
		GfMM = GfMemoryManager();
		return true;
	}
	else
	{
		if (GfMM->State != GF_MM_STATE_NULL)
		{
			GfMemoryManagerRelease();
			GfMM = NULL;     
			return GfMemoryManagerInitialize();
		}
	}
	return false;
}
//============================================================================*

//============================================================================*
// Destroy the one and only global memory manager and it's allocated data
// Dump = true: Dump info to console
//----------------------------------------------------------------------------*
void GfMemoryManagerRelease(bool Dump)
{
	unsigned int LeakSizeTotal = 0;
	unsigned int LeakSizeNewTotal = 0;
	unsigned int LeakSizeMallocTotal = 0;

	unsigned int MaxLeakSizeTotal = 0;
	unsigned int MaxLeakSizeNewTotal = 0;
	unsigned int MaxLeakSizeMallocTotal = 0;

    if (GfMM != NULL)                           
	{
		tDSMMLinkBlock* Block = GfMM->GarbageCollection;
		tMemoryManager* MM = GfMM;                           

		if (Dump)
		{
			fprintf(stderr,"\nCurrent size requested         : %d [Byte]",MM->Requested);
			fprintf(stderr,"\nCurrent size allocated         : %d [Byte]\n",MM->Allocated);
		}

		GfMM = NULL;                           

		tDSMMLinkBlock* CurrentBlock = Block->Next;

		int n = 0;
		while (CurrentBlock)
		{
			tDSMMLinkBlock* ToFree = CurrentBlock;
			CurrentBlock = CurrentBlock->Next;

			if (ToFree->Mark == MM_MARKER_BEGIN)
			{
				LeakSizeTotal += ToFree->Size;
				if (MaxLeakSizeTotal < ToFree->Size)
					MaxLeakSizeTotal = ToFree->Size;

				if (ToFree->Type == 1)
				{
					if (Dump)
					{
						fprintf(stderr,
							"%04.4d; Block; %04.4d; Size; %06.6d; ReturnTo; %p; Address; %p; new/delete;\n",
							++n,ToFree->BLID,ToFree->Size,ToFree->RAdr,ToFree);
					}
					LeakSizeNewTotal += ToFree->Size;
					if (MaxLeakSizeNewTotal < ToFree->Size)
						MaxLeakSizeNewTotal = ToFree->Size;
					delete(ToFree);
				}
				else
				{
					if (Dump)
					{
						fprintf(stderr,"%04.4d; Block; %04.4d; Size; %06.6d; ReturnTo; %p; Address; %p; malloc/free;\n",
							++n,ToFree->BLID,ToFree->Size,ToFree->RAdr,ToFree);
					}
					LeakSizeMallocTotal += ToFree->Size;
					if (MaxLeakSizeMallocTotal < ToFree->Size)
						MaxLeakSizeMallocTotal = ToFree->Size;
					free(ToFree);
				}
			}
			else
			{
				if (Dump)
				{
					fprintf(stderr,"%d Block corrupted\n",++n);
				}
				CurrentBlock = NULL;
			}
		}

		if (Dump)
		{
			fprintf(stderr,"\nMemory manager leak statistics:\n\n");

			fprintf(stderr,"Number of allocated blocks     : %d\n",GfMM_Counter);
			fprintf(stderr,"Number of memory leaks         : %d\n\n",n);
	
			fprintf(stderr,"Total leak size new/delete     : %d [Byte]\n",LeakSizeNewTotal);
			fprintf(stderr,"Total leak size malloc/free    : %d [Byte]\n",LeakSizeMallocTotal);
			fprintf(stderr,"Total leak size total          : %d [Byte]\n\n",LeakSizeTotal);

			fprintf(stderr,"Max leak block size new/delete : %d [Byte]\n",MaxLeakSizeNewTotal);
			fprintf(stderr,"Max leak block size malloc/free: %d [Byte]\n",MaxLeakSizeMallocTotal);
			fprintf(stderr,"Max leak block size total      : %d [Byte]\n\n",MaxLeakSizeTotal);

			fprintf(stderr,"Max size requested at one time : %.3f [MB]\n",MM->MaxRequested/(1024.0*1024));
			fprintf(stderr,"Max size allocated at one time : %.3f [MB]\n",MM->MaxAllocated/(1024.0*1024));
			fprintf(stderr,"Max size requested at one time : %d [B]\n",MM->MaxRequested);
			fprintf(stderr,"Max size allocated at one time : %d [B]\n",MM->MaxAllocated);
			fprintf(stderr,"Overhead for Memory Manager    : %.3f [MB]\n",(MM->MaxAllocated - MM->MaxRequested)/(1024.0*1024));
			fprintf(stderr,"Mean overhead                  : %.6f [%%]\n",(100.0 * (MM->MaxAllocated - MM->MaxRequested))/MM->MaxRequested);

			fprintf(stderr,"Remaining size requested       : %d [Byte]\n",MM->Requested);
			fprintf(stderr,"Remaining size allocated       : %d [Byte]\n",MM->Allocated);

			
			fprintf(stderr,"\nPress [Enter] to show next part of info\n");

			getchar(); // Stop to show leaks first

			unsigned int total = MM->Hist[0];
			for (int I = 1; I <= MAXBLOCKSIZE; I++)
				total += I * MM->Hist[I];

			total /= (1024 * 1024); // Byte -> MB

			fprintf(stderr,"\nTotal size of blocks requested : %d [MB]\n",total);

			fprintf(stderr,"\nNumber of blocks     >= %d [Byte] : %d",MAXBLOCKSIZE,MM->BigBMax);
			fprintf(stderr,"\nTotal size of blocks >= %d [Byte] : %.3f [kB]",MAXBLOCKSIZE,MM->HistMax[0]/1024.0);
			fprintf(stderr,"\nMean size of blocks  >= %d [Byte] : %.3f [kB]\n",MAXBLOCKSIZE,MM->HistMax[0]/1024.0 / MM->BigBMax);

			fprintf(stderr,"\nHistogram of block sizes < %d [Byte]:\n",MAXBLOCKSIZE);
			fprintf(stderr,"\nBlocksize : Number of blocks requested");
			fprintf(stderr,"\n          : at same time :  remaining\n");

			for (int I = 1; I <= MAXBLOCKSIZE; I++)
			{
				if (MM->HistMax[I] > 0)
					fprintf(stderr,"%4d      : %8d     : %d\n",I,MM->HistMax[I],MM->Hist[I]);
			}
		}

		for (int I = 0; I < MAXBLOCKSIZE; I++)
		{
			tMMBlockStack* Stack = &(MM->StackBuffer.Stack[I]);
			free(Stack->Block);
		}
		free(Block); // Delete the memory manager itself
	}

	if (Dump)
	{
		fprintf(stderr,"\nPress [Enter] to close the program\n");
		getchar();
	}
}
//============================================================================*

//============================================================================*
// Check for Memory Manager running
//----------------------------------------------------------------------------*
bool GfMemoryManagerRunning(void)
{
	if (GfMM != NULL)
		return true;
	else
		return false;
}
//============================================================================*

//============================================================================*
// Set DoNotFree flag for debugging
//----------------------------------------------------------------------------*
void GfMemoryManagerDoAccept(void)
{
	GfMM->DoNotFree = true;
}
//============================================================================*

//============================================================================*
// Reset DoNotFree flag for debugging
//----------------------------------------------------------------------------*
void GfMemoryManagerDoFree(void)
{
	GfMM->DoNotFree = false;
}
//============================================================================*

//============================================================================*
// Set Group ID for allocation of blocks
//----------------------------------------------------------------------------*
uint16 GfMemoryManagerSetGroup(uint16 Group)
{
	uint16 LastColor = GfMM->Group;
	GfMM->Group = Group;
	return LastColor;
}
//============================================================================*
	
//============================================================================*
// Update statistics
//----------------------------------------------------------------------------*
void GfMemoryManagerHistAllocate(size_t size)
{
	if (size <= MAXBLOCKSIZE)
	{
		GfMM->Hist[size] += 1;
		GfMM->HistMax[size] = MAX(GfMM->HistMax[size],GfMM->Hist[size]);
	}
	else
	{
		GfMM->BigB += 1;
		GfMM->BigBMax = MAX(GfMM->BigBMax,GfMM->BigB);
		GfMM->Hist[0] += size;
		GfMM->HistMax[0] = MAX(GfMM->HistMax[0],GfMM->Hist[0]);
	}
}
//============================================================================*
	
//============================================================================*
// Update statistics
//----------------------------------------------------------------------------*
void GfMemoryManagerHistFree(size_t size)
{
	if (size <= MAXBLOCKSIZE)
	{
		GfMM->Hist[size] -= 1;
	}
	else
	{
		GfMM->Hist[0] -= size;
	}
}
//============================================================================*

//----------------------------------------------------------------------------*
// ... Implementation
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
#endif

