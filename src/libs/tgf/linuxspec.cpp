/***************************************************************************
  
	file                 : linuxspec.cpp
	created              : Sat Mar 18 23:54:05 CET 2000
	copyright            : (C) 2000 by Eric Espie
	email                : torcs@free.fr
	version              : $Id: linuxspec.cpp 6099 2015-08-31 00:03:46Z beaglejoe $
	
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

// Note: This code is intended to be compiled ONLY under Linuxes / BSDs / Mac OS X.

#include <cstdlib>
#include <cstddef>
#include <cstdarg>
#include <climits>
#include <cstring>
#include <cerrno>
#include <string>
#include <sstream>

#include <sys/types.h>
#include <dirent.h>
#include <dlfcn.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/utsname.h> // uname

#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
#include <sys/param.h>
#include <sys/sysctl.h>
#if defined(__APPLE__) && !defined(USE_MACPORTS)
//#include <Carbon/Carbon.h> /* Carbon APIs for Multiprocessing (TODO) */
#endif
#else
// Define _GNU_SOURCE in order to have access to pthread_set/getaffinity_np
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <pthread.h>
#endif

#include "tgf.h"

#include "os.h"

static const size_t SOFileExtLen = strlen("."DLLEXT);

/*
 * Function
 *	linuxModLoad
 *
 * Description
 *	Load the module of given shared library file
 *	(Load the shared library, then retrieve info about the module (tModInfo struct) ;
 *	 the library is NOT unloaded).
 *
 * Parameters
 *	sopath  (in)     path of the shared library file to load
 *	modlist	(in/out) list of module interfaces description structure (may begin empty)
 *
 * Return
 *	0	Ok
 *	-1	error
 *
 * Remarks
 *	* Nothing done if a module with equal shared library file path-name
 *	  already exists in modlist (WARNING: if same shared library file, but with different 
 *	  path-names, like with an absolute and a relative one, the module is loaded again !)
 *	* The loaded module info structure is added at the HEAD of the list (**modlist)
 *	  (not added, but only moved to HEAD, if a module with equal shared library file path-name
 *	   already exists in modlist).
 *	
 */
static int
linuxModLoad(unsigned int /* gfid */, const char *sopath, tModList **modlist)
{
	tSOHandle handle;
	tModList* curMod;
	
	/* Try and avoid loading the same module twice (WARNING: Only checks sopath equality !) */
	if ((curMod = GfModIsInList(sopath, *modlist)) != 0)
	{
		GfLogInfo("Module %s already loaded\n", sopath);
		GfModMoveToListHead(curMod, modlist); // Force module to be the first in the list.
		return 0;
	}
	
	GfLogInfo("Loading module %s\n", sopath);
	
	/* Load the shared library */
	handle = dlopen(sopath, RTLD_LAZY);
	if (handle)
	{
		/* Initialize the module */
		if (GfModInitialize(handle, sopath, GfIdAny, &curMod) == 0)
		{
			if (curMod) /* Retained against GfIdAny */
				// Add the loaded module at the head of the list (no sort by priority).
				GfModAddInList(curMod, modlist, /* priosort */ 0);
		}
		else 
		{
			dlclose(handle);
			GfLogError("linuxModLoad: Module init function failed %s\n", sopath);
			return -1;
		}
	}
	else
	{
		GfLogError("linuxModLoad: ...  %s\n", dlerror());
		return -1;
	}
	
	return 0;
}

/*
 * Function
 *	linuxModInfo
 *
 * Description
 *	Retrieve info about the module of given shared library file,
 *	(Load the shared library, then retrieve info about the module (tModInfo struct),
 *	 and finally unload the library).
 *
 * Parameters
 *	sopath  (in)     path of the shared library file to load
 *	modlist	(in/out) list of module interfaces description structure (may begin empty)
 *
 * Return
 *	0	Ok
 *	-1	error
 *
 * Remarks
 *	* Nothing done if a module with equal shared library file path-name
 *	  already exists in modlist (WARNING: if same shared library file, but with different 
 *	  path-names, like with an absolute and a relative one, the module is loaded again !)
 *	* The loaded module info structure is added at the HEAD of the list (**modlist)
 *	  (not added, but only moved to HEAD, if a module with equal shared library file path-name
 *	   already exists in modlist).
 *	
 */
static int
linuxModInfo(unsigned int /* gfid */, const char *sopath, tModList **modlist)
{
	tSOHandle handle;
	tModList *curMod;
	int       infoSts = 0;
	
	/* Try and avoid loading the same module twice (WARNING: Only checks sopath equality !) */
	if ((curMod = GfModIsInList(sopath, *modlist)) != 0)
	{
		GfLogInfo("Module %s already requested\n", sopath);
		GfModMoveToListHead(curMod, modlist); // Force module to be the first in the list.
		return infoSts;
	}
	
	GfLogTrace("Querying module %s\n", sopath);
	
	/* Load the shared library */
	handle = dlopen(sopath, RTLD_LAZY);
	if (handle) 
	{
		/* Initialize the module */
		if (GfModInitialize(handle, sopath, GfIdAny, &curMod) == 0)
		{
			if (curMod) /* Retained against GfIdAny */
			{
				// Add the loaded module at the head of the list (no sort by priority).
				GfModAddInList(curMod, modlist, /* priosort */ 0);
			}
			
			/* Terminate the module */
			infoSts = GfModTerminate(handle, sopath);
		} 
		else 
		{
			GfLogError("linuxModInfo: Module init function failed %s\n", sopath);
			infoSts = -1;
		}
		
		/* Close the DLL whatever happened */
		dlclose(handle);
	} 
	else 
	{
		GfLogError("linuxModInfo: ...  %s\n", dlerror());
		infoSts = -1;
	}
	
	return infoSts;
}

/*
 * Function
 *	linuxModLoadDir
 *
 * Description
 *	Load the modules whose shared library files are contained in a given directory
 *	(for each shared library, load it and retrieve info about the module (tModInfo struct) ;
 *	 the shared library is NOT unloaded)
 *
 * Parameters
 *	gfid    (in)		id of the gaming framework of the modules to load,
 *	dir     (in)		directory to search (relative)
 *	modlist (in/out)	list of module description structure (may begin empty)
 *
 * Return
 *	>=0	number of modules loaded
 *	-1	error
 *
 * Remarks
 *	The loaded module info structures are added in the list according to each module's priority
 *	(NOT at the head of the list).
 *	
 */
static int
linuxModLoadDir(unsigned int gfid, const char *dir, tModList **modlist)
{
	char		sopath[256];	/* path of the lib[x].so */
	tSOHandle 		handle;
	DIR			*dp;
	struct dirent	*ep;
	int			modnb;		/* number on loaded modules */
	tModList		*curMod;
	
	modnb = 0;
	
	/* open the current directory */
	dp = opendir(dir);
	if (dp) 
	{
		/* some files in it */
		while ((ep = readdir (dp)) != 0) 
		{
			if ((strlen(ep->d_name) > SOFileExtLen + 1) &&
				(strcmp("."DLLEXT, ep->d_name+strlen(ep->d_name)-SOFileExtLen) == 0)) /* xxxx.so */
			{
				sprintf(sopath, "%s/%s", dir, ep->d_name);
				/* Try and avoid loading the same module twice (WARNING: Only checks sopath equality !) */
				if (!GfModIsInList(sopath, *modlist))
				{
					/* Load the shared library */
					GfLogInfo("Loading module %s\n", sopath);
					handle = dlopen(sopath, RTLD_LAZY);
					if (handle)
					{
						/* Initialize the module */
						if (GfModInitialize(handle, sopath, gfid, &curMod) == 0)
						{
							if (curMod) /* Retained against gfid */
							{
								modnb++;
								GfModAddInList(curMod, modlist, /* priosort */ 1);
							}
						} 
						else
						{
							dlclose(handle);
							modnb = -1;
							break;
						}
					}
					else
					{
						GfLogError("linuxModLoadDir: ...  %s\n", dlerror());
						modnb = -1;
						break;
					}
				}
			}
		}
		(void)closedir(dp);
	}
	else 
	{
		GfLogError("linuxModLoadDir: ... Couldn't open the directory %s\n", dir);
		modnb = -1;
	}
	
	return modnb;
}

/*
 * Function
 *	linuxModInfoDir
 *
 * Description
 *	Retrieve info about the modules whose shared library files are contained in a given directory
 *	(for each shared library, load it, retrieve info about the module (tModInfo struct),
 *	 and finally unload the library).
 *
 * Parameters
 *	dir	(in)     directory to search (relative)
 *	level   (in)     if 1, load any shared library contained in the subdirs of dir
 *	                 and whose name is the same as the containing subdir (ex: bt/bt.so)
 *	                 if 0, load any shared library contained in dir (ignore subdirs)
 *	modlist	(in/out) list of module description structure (may begin empty)
 *
 * Return
 *	>=0	number of modules loaded
 *	-1	error
 *
 * Remarks
 *	The loaded module info structures are added in the list according to each module's priority
 *	(NOT at the head of the list).
 *	
 */
static int
linuxModInfoDir(unsigned int /* gfid */, const char *dir, int level, tModList **modlist)
{
	char		 sopath[256];	/* path of the lib[x].so */
	tSOHandle		 handle;
	DIR			*dp;
	struct dirent	*ep;
	int			 modnb;		/* number on loaded modules */
	tModList		*curMod;
	
	modnb = 0;
	
	/* open the current directory */
	dp = opendir(dir);
	if (dp)
	{
		/* some files in it */
		while ((ep = readdir (dp)) != 0) 
		{
			if (((strlen(ep->d_name) >  SOFileExtLen + 1) && 
				 (strcmp("."DLLEXT, ep->d_name+strlen(ep->d_name)-SOFileExtLen) == 0)) /* xxxx.so */
				|| ((level == 1) && (ep->d_name[0] != '.')))
			{
				if (level == 1)
					sprintf(sopath, "%s/%s/%s.%s", dir, ep->d_name, ep->d_name,DLLEXT);
				else
					sprintf(sopath, "%s/%s", dir, ep->d_name);
				
				/* Try and avoid loading the same module twice (WARNING: Only checks sopath equality !) */
				if (!GfModIsInList(sopath, *modlist))
				{
					/* Load the shared library */
					GfLogTrace("Querying module %s\n", sopath);
					handle = dlopen(sopath, RTLD_LAZY);
					if (handle)
					{
						/* Initialize the module */
						if (GfModInitialize(handle, sopath, GfIdAny, &curMod) == 0)
						{
							if (curMod) /* Retained against gfid */
							{
								/* Get associated info */
								modnb++;
								GfModAddInList(curMod, modlist, /* priosort */ 1);
							}
							
							/* Terminate the module */
							GfModTerminate(handle, sopath);
						}
						
						/* Close the shared library */
						dlclose(handle);
					} 
					else 
					{
						GfLogError("linuxModInfoDir: ...  %s\n", dlerror());
					}
				}
			}
		}
		(void)closedir(dp);
	} 
	else 
	{
		GfLogError("linuxModInfoDir: ... Couldn't open the directory %s.\n", dir);
		return -1;
	}
	
	return modnb;
}

/*
 * Function
 *	linuxModUnloadList
 *
 * Description
 *	Unload the modules of a list
 *
 * Parameters
 *	modlist	(in/out) list of modules to unload
 *
 * Return
 *	0	Ok
 *	-1	Error
 *
 * Remarks
 *	
 */
static int
linuxModUnloadList(tModList **modlist)
{
	tModList		*curMod;
	tModList		*nextMod;
	int                 termSts;
	int                 unloadSts = 0;
	
	curMod = *modlist;
	if (curMod == 0)
		return 0;
	
	do 
	{
		nextMod = curMod->next;
		
		/* Terminate the module */
		termSts = GfModTerminate(curMod->handle, curMod->sopath);
		if (termSts)
			unloadSts = termSts;
		
		// Comment out for valgrind runs, be aware that the driving with the keyboard does
		// just work to first time this way.
		dlclose(curMod->handle);
		GfLogInfo("Unloaded module %s\n", curMod->sopath);
		
		GfModInfoFreeNC(curMod->modInfo, curMod->modInfoSize);
		free(curMod->sopath);
		free(curMod);
		
		curMod = nextMod;
	}
	while (curMod != *modlist);
	
	*modlist = (tModList *)NULL;
	
	return unloadSts;
}

/*
 * Function
 *	linuxDirGetList
 *
 * Description
 *	Get a list of entries in a directory
 *
 * Parameters
 *	directory name
 *
 * Return
 *	list of directory entries
 */
static tFList *
linuxDirGetList(const char *dir)
{
	DIR *dp;
	struct dirent *ep;
	tFList *flist = (tFList*)NULL;
	tFList *curf;
	
	/* open the current directory */
	dp = opendir(dir);
	if (dp != NULL) {
		/* some files in it */
		while ((ep = readdir(dp)) != 0) {
			if ((strcmp(ep->d_name, ".") != 0) && (strcmp(ep->d_name, "..") != 0)) {
				curf = (tFList*)calloc(1, sizeof(tFList));
				curf->name = strdup(ep->d_name);
				if (flist == (tFList*)NULL) {
					curf->next = curf;
					curf->prev = curf;
					flist = curf;
				} else {
					/* sort entries... */
					if (strcasecmp(curf->name, flist->name) > 0) {
						do {
							flist = flist->next;
						} while ((strcasecmp(curf->name, flist->name) > 0) && (strcasecmp(flist->name, flist->prev->name) > 0));
						flist = flist->prev;
					} else {
						do {
							flist = flist->prev;
						} while ((strcasecmp(curf->name, flist->name) < 0) && (strcasecmp(flist->name, flist->next->name) < 0));
					}
					curf->next = flist->next;
					flist->next = curf;
					curf->prev = flist;
					curf->next->prev = curf;
					flist = curf;
				}
			}
		}
		closedir(dp);
	}
	return flist;
}

/*
 * Function
 *	linuxDirGetListFiltered
 *
 * Description
 *	Get a list of entries in a directory
 *
 * Parameters
 *	directory name
 *
 * Return
 *	list of directory entries
 */
static tFList *
linuxDirGetListFiltered(const char *dir, const char *prefix, const char *suffix)
{
	DIR	*dp;
	struct dirent *ep;
	tFList *flist = (tFList*)NULL;
	tFList *curf;
	int	prefixLg, suffixLg;
	int	fnameLg;
	
	if ((!prefix || strlen(prefix) == 0) && (!suffix || strlen(suffix) == 0))
		return linuxDirGetList(dir);
	
	suffixLg = suffix ? strlen(suffix) : 0;
	prefixLg = prefix ? strlen(prefix) : 0;
	
	/* open the current directory */
	dp = opendir(dir);
	if (dp != NULL) {
		/* some files in it */
		while ((ep = readdir(dp)) != 0) {
			fnameLg = strlen(ep->d_name);
			if ((!prefix || (fnameLg > prefixLg
							 && strncmp(ep->d_name, prefix, prefixLg) == 0))
				&& (!suffix || (fnameLg > suffixLg
								&& strncmp(ep->d_name + fnameLg - suffixLg, suffix, suffixLg) == 0))) {
				curf = (tFList*)calloc(1, sizeof(tFList));
				curf->name = strdup(ep->d_name);
				curf->dispName = 0;
				curf->userData = 0;
				if (flist == (tFList*)NULL) {
					curf->next = curf;
					curf->prev = curf;
					flist = curf;
				} else {
					/* sort entries... */
					if (strcasecmp(curf->name, flist->name) > 0) {
						do {
							flist = flist->next;
						} while ((strcasecmp(curf->name, flist->name) > 0) && (strcasecmp(flist->name, flist->prev->name) > 0));
						flist = flist->prev;
					} else {
						do {
							flist = flist->prev;
						} while ((strcasecmp(curf->name, flist->name) < 0) && (strcasecmp(flist->name, flist->next->name) < 0));
					}
					curf->next = flist->next;
					flist->next = curf;
					curf->prev = flist;
					curf->next->prev = curf;
					flist = curf;
				}
			}
		}
		closedir(dp);
	}
	return flist;
}

// Initial "time" (actually the number of seconds since the system has been up).
static double InitTime = -1.0;

/*
* Function
*	linuxTimeClock
*
* Description
*	Return the number of seconds since the game is running (resolution 1 micro-second).
*
* Parameters
*	None
*
* Return
*	The number of seconds since the game is running
*
*/
static double
linuxTimeClock(void)
{
	struct timeval tv;
	
	gettimeofday(&tv, 0);
	
	if (InitTime < 0)
		InitTime = (double)(tv.tv_sec + tv.tv_usec * 1e-6);
	
	return (double)(tv.tv_sec + tv.tv_usec * 1e-6) - InitTime;
}


/*
* Function
*	linuxGetNumberOfCPUs
*
* Description
*	Retrieve the actual number of CPUs in the system
*       Note that a core is considered here as a "CPU", and an Intel hyper-threaded processor
*       will report twice as many "CPUs" as actual cores ...
*
* Parameters
*	None
*
* Return
*	The number of CPUs in the system
*
* Remarks
*       WARNING: Not tested under platforms other than Linux : Mac OS X, BSD, Solaris, AIX.
*	
*/
unsigned linuxGetNumberOfCPUs()
{
	static unsigned nCPUs = 0;
	
	if (nCPUs == 0)
	{
		
		// MacOS X, FreeBSD, OpenBSD, NetBSD, etc ...
#if (defined(__APPLE__) && !defined(USE_MACPORTS)) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
		
		nt mib[4];
		size_t len; 
		
		// Set the mib for hw.ncpu
		
		// Get the number of CPUs from the system
		// 1) Try HW_AVAILCPU first.
		mib[0] = CTL_HW;
		mib[1] = HW_AVAILCPU;  // alternatively, try HW_NCPU;
		sysctl(mib, 2, &nCPUs, &len, NULL, 0);
		
		if (nCPUs < 1) 
		{
			// 2) Try alternatively HW_NCPU.
			mib[1] = HW_NCPU;
			sysctl(mib, 2, &nCPUs, &len, NULL, 0);
		}
		
		// Linux, Solaris, AIX
#elif defined(linux) || defined(__linux__) || defined(USE_MACPORTS)
		
		nCPUs = (unsigned)sysconf(_SC_NPROCESSORS_ONLN);
		
		// Anything else ... not supported.
#else
		
#warning "Unsupported Linux OS"
		
#endif
		
		if (nCPUs < 1)
		{
			GfLogWarning("Could not get the number of CPUs here ; assuming only 1\n");
			nCPUs = 1;
		}
		else
			GfLogInfo("Detected %d CPUs\n", nCPUs);
	}
	
	return nCPUs;
}

/*
* Function
*    linuxSetThreadAffinity
*
* Description
*    Force the current thread to run on the specified CPU.
*
* Parameters
*    nCPUId : the index in [0, # of actual CPUs on the system [ (any other value will actually reset the thread affinity to the "system" affinity mask, meaning no special CPU assignement)
*
* Return
*    true if any error occured, false otherwise
*
* Remarks
*    
*/
#if !defined(USE_MACPORTS)
std::string cpuSet2String(const cpu_set_t* pCPUSet)
{
	std::ostringstream ossCPUSet;
	for (int nCPUIndex = 0; nCPUIndex < CPU_SETSIZE; nCPUIndex++)
		if (CPU_ISSET(nCPUIndex, pCPUSet))
		{
			if (ossCPUSet.tellp() > 0)
				ossCPUSet << ',';
			ossCPUSet << nCPUIndex;
		}
	
	return ossCPUSet.str();
}
#endif
bool
linuxSetThreadAffinity(int nCPUId)
{
	// MacOS X, FreeBSD, OpenBSD, NetBSD, etc ...
#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
	
	GfLogWarning("Thread affinity not yet implemented on Mac OS X or BSD.\n");
	// TODO.
	
	// Linux, Solaris, AIX ... with NPTL (Native POSIX Threads Library)
#elif defined(linux) || defined(__linux__)
	
	// Get the handle for the current thread.
	pthread_t hCurrThread = pthread_self();
	
	// Determine the affinity mask to set for the current thread.
	cpu_set_t nThreadAffinityMask;
	CPU_ZERO(&nThreadAffinityMask);
	if (nCPUId == GfAffinityAnyCPU)
	{
		// No special affinity on any CPU => set "system" affinity mask
		// (1 bit for each installed CPU).
		for (int nCPUIndex = 0; (unsigned)nCPUIndex < linuxGetNumberOfCPUs(); nCPUIndex++)
		{
			CPU_SET(nCPUIndex, &nThreadAffinityMask);
		}
	}
	else
	{	
		// Affinity on a specified CPU => compute its mask.
		CPU_SET(nCPUId, &nThreadAffinityMask);
	}
	
	// Set the affinity mask for the current thread ("stick" it to the target core).
	if (pthread_setaffinity_np(hCurrThread, sizeof(nThreadAffinityMask), &nThreadAffinityMask))
	{
		GfLogError("Failed to set current pthread (handle=0x%X) affinity on CPU(s) %s (%s)\n",
				   hCurrThread, cpuSet2String(&nThreadAffinityMask).c_str(), strerror(errno));
		return false;
	}
	else
		GfLogInfo("Affinity set on CPU(s) %s for current pthread (handle=0x%X)\n",
				  cpuSet2String(&nThreadAffinityMask).c_str(), hCurrThread);
	
	return true;
	
	// Anything else ... not supported.
#else
	
#warning "linuxspec.cpp::linuxSetThreadAffinity : Unsupported Linux OS"
	GfLogWarning("Thread affinity not yet implemented on this unknown Unix.\n");
	
#endif
	
	return false;
}

/*
* Function
*	linuxGetOSInfo
*
* Description
*	Get some info about the running OS (name, x.y.z release, and nb of bits).
*
* Parameters
*	strName : target string  for the OS name ("" if any error occurred)
*	nMajor  : target OS major version integer (-1 if could not be obtained)
*	nMinor  : target OS minor version integer (-1 if could not be obtained)
*	nPatch  : target OS patch version integer (-1 if could not be obtained)
*	nBits   : target OS number of bits (32 or 64) integer (or -1 if could not be obtained)
*
* Return
*	True if OK, False otherwise.
*/
static bool
linuxGetOSInfo(std::string& strName, int& nMajor, int& nMinor, int& nPatch, int& nBits)
{
	struct utsname utsName;
	if(uname(&utsName) < 0)
	{
		const int errnum = errno; // Get errno before it is overwritten by some system call.
		GfLogWarning("Could not get OS info through uname (%s).\n", strerror(errnum));
		return false;
	}
	
	//GfLogDebug("linuxGetOSInfo : name='%s', version='%s', release='%s'\n",
	//		   utsName.sysname, utsName.version, utsName.release);
	strName = utsName.sysname;
	strName += " ";
	strName += utsName.release;
	strName += " ";
	strName += utsName.version;
	const int nNums = sscanf(utsName.release, "%d.%d.%d", &nMajor, &nMinor, &nPatch);
	if (nNums < 1)
		nMajor = -1;
	if (nNums < 2)
		nMinor = -1;
	if (nNums < 3)
		nPatch = -1;
	nBits = strstr(utsName.release, "64") ? 64 : 32;
	
	return true;
}

/*
 * Function
 *    LinuxSpecInit
 *
 * Description
 *    Initialize the specific linux functions
 *
 * Parameters
 *    none
 *
 * Return
 *    none
 *
 * Remarks
 *    
 */
void
LinuxSpecInit(void)
{
	memset(&GfOs, 0, sizeof(GfOs));
	
	GfOs.modLoad = linuxModLoad;
	GfOs.modLoadDir = linuxModLoadDir;
	GfOs.modUnloadList = linuxModUnloadList;
	GfOs.modInfo = linuxModInfo;
	GfOs.modInfoDir = linuxModInfoDir;
	GfOs.dirGetList = linuxDirGetList;
	GfOs.dirGetListFiltered = linuxDirGetListFiltered;
	GfOs.timeClock = linuxTimeClock;
	GfOs.sysGetNumberOfCPUs = linuxGetNumberOfCPUs;
	GfOs.sysSetThreadAffinity = linuxSetThreadAffinity;
	GfOs.sysGetOSInfo = linuxGetOSInfo;
}
