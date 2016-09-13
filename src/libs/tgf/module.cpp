/***************************************************************************
                      module.cpp -- Dynamic module management                                
                             -------------------                                         
    created              : Mod Mar 14 20:32:14 CEST 2011
    copyright            : (C) 2011 by Jean-Philippe Meuret
    web                  : http://www.speed-dreams.org
    version              : $Id: module.cpp 5835 2014-11-14 17:29:58Z wdbee $
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

/** @file   
    		Dynamic module management.
    @version	$Id: module.cpp 5835 2014-11-14 17:29:58Z wdbee $
    @ingroup	module
*/

#include <sstream>

#include "tgf.hpp"

#ifdef WIN32
# include <windows.h>
# define dlopen(pszShLibFileName) (void*)LoadLibrary(pszShLibFileName)
# define dlsym(pvHandle, pszFuncName) GetProcAddress((HMODULE)pvHandle, pszFuncName)
# define dlclose(pvHandle) !FreeLibrary((HMODULE)pvHandle)
# define soLibHandle(handle) (void*)handle
#else
# include <dlfcn.h>
# define dlopen(soFileName) dlopen(soFileName, RTLD_LAZY|RTLD_GLOBAL)
# define soLibHandle(handle) handle
#endif

// Name and type of the 2 extern "C" module interface functions.
typedef int (*tModOpenFunc)(const char*, void*); // Returns 0 on success, !0 otherwise.
static const char* pszOpenModuleFuncName = "openGfModule";

typedef int (*tModCloseFunc)(); // Returns 0 on success, !0 otherwise.
static const char* pszCloseModuleFuncName = "closeGfModule";

// Error code decoder.
static std::string lastDLErrorString()
{
	std::string strError;
	
#ifdef WIN32

    // Retrieve the system error message for the last-error code
    LPVOID lpMsgBuf;
    DWORD dw = GetLastError(); 
	
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
				  FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
				  NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				  (LPTSTR) &lpMsgBuf, 0, NULL );

	strError = (const char*)lpMsgBuf;
	
    LocalFree(lpMsgBuf);

#else

	strError = dlerror();

#endif

	return strError;
}

// The (static) table of loaded modules and their associated shared library.
std::map<std::string, GfModule*> GfModule::_mapModulesByLibName;

GfModule::GfModule(const std::string& strShLibName, void* hShLibHandle)
: _strShLibName(strShLibName), _hShLibHandle(hShLibHandle)
{
}

GfModule::~GfModule()
{
	_mapModulesByLibName.clear(); // Avoid memory leaks
}

GfModule* GfModule::load(const std::string& strShLibName)
{
	// Don't load shared libraries twice
	// Warning: Only checked through the give nlibrary file path-name ...
	if (_mapModulesByLibName.find(strShLibName) != _mapModulesByLibName.end())
	{
		GfLogDebug("Not re-loading module %s (already done)\n", strShLibName.c_str());
		return _mapModulesByLibName[strShLibName];
	}

	// Try and open the shared library.
	void* hSOLib = dlopen(strShLibName.c_str());
	if (!hSOLib)
	{
		GfLogError("Failed to load library %s (%s)\n",
				   strShLibName.c_str(), lastDLErrorString().c_str());
		return 0;
	}

	// Try and get the module opening function.
	tModOpenFunc modOpenFunc = (tModOpenFunc)dlsym(hSOLib, pszOpenModuleFuncName);
    if (!modOpenFunc)
    {
		GfLogError("Library %s doesn't export any '%s' function' ; module NOT loaded\n",
				   strShLibName.c_str(), pszOpenModuleFuncName);
		(void)dlclose(hSOLib);
		return 0;
	}

	// Call the module opening function (must instanciate the module and register_ it on success).
	if (modOpenFunc(strShLibName.c_str(), hSOLib))
	{
		GfLogError("Library %s '%s' function call failed ; module NOT loaded\n",
				   strShLibName.c_str(), pszOpenModuleFuncName);
		(void)dlclose(hSOLib);
		return 0;
	}

	// Check if the module was successfully register_ed.
	if (_mapModulesByLibName.find(strShLibName) == _mapModulesByLibName.end())
	{
		GfLogError("Library %s '%s' function failed to register the open module ; NOT loaded\n",
				   strShLibName.c_str(), pszOpenModuleFuncName);
		(void)dlclose(hSOLib);
		return 0;
	}

	// Yesssss !
	GfLogTrace("Module %s loaded\n", strShLibName.c_str());

	return _mapModulesByLibName[strShLibName];
}

bool GfModule::isPresent(const std::string& strModCatName, const std::string& strModName)
{
	std::ostringstream ossModLibPathName;
	
	ossModLibPathName << GfLibDir() << "modules/" << strModCatName << "/"
					  <<  strModName << '.' << DLLEXT;

	return GfFileExists(ossModLibPathName.str().c_str());
}

GfModule* GfModule::load(const std::string& strModPathName, const std::string& strModName)
{
	std::ostringstream ossModLibPathName;
	
	ossModLibPathName << GfLibDir() << strModPathName << "/" <<  strModName << '.' << DLLEXT;

	return load(ossModLibPathName.str());
}

// TODO: Tests (not yet used, see #809).
std::vector<GfModule*> GfModule::loadFromDir(const std::string& strDirPath,
											 bool bUseChildDirs)
{
	std::vector<GfModule*> vecModules;

	GfLogDebug("GfModule::loadFromDir(%s)\n", strDirPath.c_str());

	// Get the list of files/sub-dirs in the folder.
	tFList* lstFilesOrDirs = GfDirGetList(strDirPath.c_str());
	if (lstFilesOrDirs)
	{
		// Filter module shared libraries and try and load each of them.
		tFList* pFileOrDir = lstFilesOrDirs;
		do 
		{
			// Ignore "." and ".." folders.
			if (pFileOrDir->name[0] == '.') 
				continue;
			
			GfLogDebug("  Examining %s\n", pFileOrDir->name);
		
			// Build module shared library path-name (consider only foders, not files).
			std::ostringstream ossShLibPath;
			ossShLibPath << strDirPath << '/' << pFileOrDir->name;
			if (bUseChildDirs)
				ossShLibPath << '/' << pFileOrDir->name;
			ossShLibPath << DLLEXT;

			// Check existence.
			if (!GfFileExists(ossShLibPath.str().c_str()))
				continue;

			// Try and load.
			GfModule* pModule = GfModule::load(ossShLibPath.str().c_str());
			if (pModule)
				vecModules.push_back(pModule);
			else
				GfLogWarning("Failed to load module %s\n", ossShLibPath.str().c_str());
			
		}
		while ((pFileOrDir = pFileOrDir->next) != lstFilesOrDirs);
	}
	
	return vecModules;
}

bool GfModule::unload(GfModule*& pModule)
{
	// Try and get the module closing function.
	std::string strShLibName = pModule->getSharedLibName();
	void* hShLibHandle = pModule->getSharedLibHandle();
	tModCloseFunc modCloseFunc = (tModCloseFunc)dlsym(hShLibHandle, pszCloseModuleFuncName);
    if (!modCloseFunc)
    {
		GfLogWarning("Library %s doesn't export any '%s' function' ; not called\n",
					 strShLibName.c_str(), pszCloseModuleFuncName);
	}

	// Call the module closing function (must delete the module instance).
	if (modCloseFunc())
	{
		GfLogWarning("Library %s '%s' function call failed ; going on\n",
					 strShLibName.c_str(), pszCloseModuleFuncName);
	}
	
	// Make it clear that the passed pointer is no more usable (it was deleted).
	pModule = 0; 
	
	// Try and close the shared library.
	if (dlclose(hShLibHandle))
	{
		GfLogWarning("Failed to unload library %s (%s) ; \n",
					 strShLibName.c_str(), lastDLErrorString().c_str());
		return false;
	}
	
	GfLogTrace("Module %s unloaded\n", strShLibName.c_str());

	return true;
}

// TODO: Tests (not yet used, see #809).
bool GfModule::unload(std::vector<GfModule*>& vecModules)
{
	bool bStatus = true;
	std::vector<GfModule*>::iterator itMod;
	for(itMod = vecModules.begin(); itMod != vecModules.end(); itMod++)
		bStatus = bStatus && unload(*itMod);

	return bStatus;
}

bool GfModule::register_(GfModule* pModule) // Can't use 'register' as it is a C++ keyword.
{
	bool bStatus = false;
	
	if (pModule)
	{
		if (_mapModulesByLibName.find(pModule->getSharedLibName()) != _mapModulesByLibName.end())
		{
			GfLogError("Can only register 1 module from %s\n", pModule->getSharedLibName().c_str());
		}
		else
		{
			_mapModulesByLibName[pModule->getSharedLibName()] = pModule;
			bStatus = true;
		}
	}

	return bStatus;
}

bool GfModule::unregister(GfModule* pModule)
{
	bool bStatus = false;
	
	if (pModule)
	{
		if (_mapModulesByLibName.find(pModule->getSharedLibName()) == _mapModulesByLibName.end())
		{
			GfLogError("Can't unregister module in %s (not yet registered)\n", 
					   pModule->getSharedLibName().c_str());
		}
		else
		{
			_mapModulesByLibName.erase(pModule->getSharedLibName());
			bStatus = true;
		}
	}

	return bStatus;
}

const std::string& GfModule::getSharedLibName() const
{
	return _strShLibName;
}

void* GfModule::getSharedLibHandle() const
{
	return _hShLibHandle;
}
