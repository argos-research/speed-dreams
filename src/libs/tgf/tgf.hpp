/***************************************************************************
                    tgf.hpp -- C++ Interface file for The Gaming Framework
                             -------------------
    created              : Mod Mar 14 20:32:14 CEST 2011
    copyright            : (C) 2011 by Jean-Philippe Meuret
    web                  : http://www.speed-dreams.org
    version              : $Id: tgf.hpp 5839 2014-11-15 15:05:25Z wdbee $
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
    	The Gaming Framework API, with C++-specific extensions.
    @version	$Id: tgf.hpp 5839 2014-11-15 15:05:25Z wdbee $
*/

#ifndef __TGF__HPP__
#define __TGF__HPP__

#ifdef _MSC_VER
// Disable useless MSVC warnings
#  pragma warning (disable:4251) // class XXX needs a DLL interface ...
#endif

#include <string>
#include <list>
#include <vector>
#include <map>
#include <ostream>

#include "tgf.h"


//****************************************
// New dynamically loadable modules system
// - 1 and only 1 module per shared library,
// - a module is a GfModule-derived class instance (and even a singleton),
// - the shared library exports 2 extern 'C' functions :
//   - int openGfModule(const char* pszShLibName, void* hShLibHandle) :
//     it must instanciate the module class (new), register the module instance (register_),
//     initialize / allocate any needed internal resource and finally return 0
//     if everything is OK, non-0 otherwise;
//   - int closeGfModule() :
//     it must release any allocated resource, unregister the module instance (unregister),
//     and finally return 0, non-0 otherwise.

class TGF_API GfModule
{
 public: // Services for the code that is client of the module.

	//! Load a module from the given library file path and name (relative to GfLibDir()).
	static GfModule* load(const std::string& strShLibName);

	//! Load a module from the given path (relative to GfLibDir()) and shared library name.
	static GfModule* load(const std::string& strModPath, const std::string& strModName);

	//! Load the modules whose shared libraries are in a given folder (relative to GfLibDir()).
	//! If bUseChildDirs, assume each library is in an own sub-folder and has same name,
	//! otherwise load them from the given folder itself (and no other).
	static std::vector<GfModule*> loadFromDir(const std::string& strDirPath, bool bUseChildDirs = true);

	//! Check if a module the given path (relative to GfLibDir()) and shared library name
	//! is really installed.
	static bool isPresent(const std::string& strModPath, const std::string& strModName);

	//! Delete a module and unload the associated library (supposed to contain no other module).
	static bool unload(GfModule*& pModule);

	//! Delete each module of the given list and unload the associated libraries (supposed to contain each only one module).
	static bool unload(std::vector<GfModule*>& vecModules);

	//! Get the module as a pointer to the given interface (aka "facet").
	template <class Interface>
	Interface* getInterface()
	{
		return dynamic_cast<Interface*>(this);
	}
	
 public: // Services for the module implementation.

	//! Register a new module instance (aimed at being called by the GfModuleOpen function).
	static bool register_(GfModule* pModule);
	
	//! Constructor.
	GfModule(const std::string& strShLibName, void* hShLibHandle);

	//! Destructor.
	virtual ~GfModule();

	//! Get the asssociated shared library path-name.
	const std::string& getSharedLibName() const;
		
	//! Get the asssociated shared library handle.
	void* getSharedLibHandle() const;
		
 protected:

	//! Unregister a module instance.
	static bool unregister(GfModule* pModule);
	
 protected:

	//! The table of loaded modules and their associated shared library (key = file name).
	static std::map<std::string, GfModule*> _mapModulesByLibName;

	//! The associated shared library file path-name.
	std::string _strShLibName;

	//! The OS-level handle of the associated shared library.
	void* _hShLibHandle;
};

//****************************************
// The event loop class

class TGF_API GfEventLoop
{
  public: // Member functions.

	//! Constructor
	GfEventLoop();

	//! Destructor
	virtual ~GfEventLoop();

	//! The real event loop function : 1) process keyboard events, 2) do the 'computing' job.
	virtual void operator()(void);

	//! Set the "key pressed" callback function.
	void setKeyboardDownCB(void (*func)(int key, int modifiers, int x, int y));

	//! Set the "key released" callback function.
	void setKeyboardUpCB(void (*func)(int key, int modifiers, int x, int y));

	//! Set the "recompute" callback function (run at the end of _every_ event loop).
	void setRecomputeCB(void (*func)(void));

	//! Set a one-shot timer callback function with given delay.
	void setTimerCB(unsigned int millis, void (*func)(int value));

	//! Request the event loop to terminate on next loop. 
	void postQuit();

  protected:

	//! Process a keyboard event.
	void injectKeyboardEvent(int code, int modifier, int state,
							 int unicode, int x = 0, int y = 0);

	//! Is a quit request pending ?
	bool quitRequested() const;

	//! Do the 'computing' job of the loop (call the recompute CB or sleep).
	void recompute();
	
  private: // Member data.

	//! Private data (pimp pattern).
	class Private;
	Private* _pPrivate;
};

//****************************************
// Application base class

class TGF_API GfApplication
{
 public:

	//! Accessor to the singleton.
	static GfApplication& self();
	
	//! Constructor.
    GfApplication(const char* pszName, const char* pszVersion, const char* pszDesc);

	//! Destructor.
	virtual ~GfApplication();

	//! Name accessor.
	const std::string& name() const;
	
	//! Version accessor.
	const std::string& version() const;
	
	//! Description accessor.
	const std::string& description() const;

	//! Initialization (when specializing, you must call base implementation first).
    virtual void initialize(bool bLoggingEnabled, int argc = 0, char **argv = 0);

	//! Add the given option to the automatically processed ones when parseOptions is called.
	void registerOption(const std::string& strShortName,
						const std::string& strLongName,
						bool bHasValue);

	//! Add a text line to the list of those which show the cmd line syntax when help is invoked.
	void addOptionsHelpSyntaxLine(const std::string& strTextLine);

	//! Add a text line to the list of those which explain the cmd line options when help is invoked.
	void addOptionsHelpExplainLine(const std::string& strTextLine);
	
	//! Parse the command line for registered options.
	bool parseOptions();

	//! Check if we have the specified regsitered option in the command line.
	bool hasOption(const std::string& strLongName) const; // No leading '--'

	//! Check if we have the specified regsitered option in the command line, and get its value.
	bool hasOption(const std::string& strLongName, // No leading '--'
				   std::string& strValue) const;

	//! Get the args remaining unprocessed after parseOptions.
	const std::vector<std::string>& remainingArgs() const;

	//! Check if we have the specified unregistered option in the command line.
	// bool hasUnregisteredOption(const std::string& strShortName, // No leading '-'
	// 						   const std::string& strLongName) const; // No leading '--'

	//! Check if we have the specified unregistered option in the command line, and get its value.
	// bool hasUnregisteredOption(const std::string& strShortName, // No leading '-'
	// 						   const std::string& strLongName, // No leading '--'
	// 						   std::string& strValue) const;

	//! Update user settings files if obsolete.
	void updateUserSettings();

	//! Application event loop.
	void setEventLoop(GfEventLoop* pEventLoop);
	GfEventLoop& eventLoop();

	//! Restart the app.
	virtual void restart();

	// Allow clean restart
	void (*ReleaseData)();

 protected:

	//! Print a short help about using the command line.
	void printUsage(const char* pszErrMsg = 0) const;
	
 protected:

	//! The app. name.
	std::string _strName;
	
	//! The app. description.
	std::string _strDesc;
	
	//! The app. version.
	std::string _strVersion;
	
	//! The event loop.
	GfEventLoop* _pEventLoop;
	
	//! The list of original command line args (setup in constructor).
	std::list<std::string> _lstArgs;

	//! The vector of left command line args (ignored by by parseOptions).
	std::vector<std::string> _vecRemArgs;

	//! The registered options : to be parsed / parsed by parseOptions.
	class Option
	{
	public:
		std::string strShortName;
		std::string strLongName;
		bool bHasValue;
		bool bFound;
		std::string strValue;
	public:
		Option()
		{
		}
		Option(const std::string& strShortName_, const std::string& strLongName_,
			   bool bHasValue_ = false)
			: strShortName(strShortName_), strLongName(strLongName_), bHasValue(bHasValue_),
			  bFound(false)
		{
		}
	};
	std::list<Option> _lstOptions;
	
	//! The help syntax/explaination about the options (setup in constructor).
	class OptionsHelp
	{
	public:
		std::list<std::string> lstSyntaxLines;
		std::list<std::string> lstExplainLines;
	};
	OptionsHelp _optionsHelp;

 protected:

	//! The singleton.
	static GfApplication* _pSelf;
};

//! Shortcut to the application singleton.
inline GfApplication& GfApp()
{
	return GfApplication::self();
}
				  
#endif // __TGF__HPP__


