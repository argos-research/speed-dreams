############################################################################
#
#   file        : options.cmake
#   copyright   : (C) 2008 by Mart Kelder, 2010 by J.-P. Meuret
#   web         : www.speed-dreams.org 
#   version     : $Id$
#
############################################################################

############################################################################
#                                                                          #
#   This program is free software; you can redistribute it and/or modify   #
#   it under the terms of the GNU General Public License as published by   #
#   the Free Software Foundation; either version 2 of the License, or      #
#   (at your option) any later version.                                    #
#                                                                          #
############################################################################

# @file     CMake configuration options
# @author   Mart Kelder, J.-P. Meuret
# @version  $Id$

MACRO(ADD_SD_COMPILE_OPTIONS)

  # This has to be done more than once, because HAVE_CONFIG_H may change.
  IF(HAVE_CONFIG_H)

    ADD_DEFINITIONS(-DHAVE_CONFIG_H)

    IF(IN_SOURCETREE)
      SET(INCLUDE_CANDIDATE ${SOURCE_DIR})
    ELSE(IN_SOURCETREE)
      SET(INCLUDE_CANDIDATE "")
    ENDIF(IN_SOURCETREE)
    FIND_PATH(CONFIGH_INCLUDE_DIR config.h ${INCLUDE_CANDIDATE} /usr/include /usr/local/include NO_DEFAULT_PATH)
    FIND_PATH(CONFIGH_INCLUDE_DIR config.h ${INCLUDE_CANDIDATE} /usr/include /usr/local/include)
    MARK_AS_ADVANCED(CONFIGH_INCLUDE_DIR)
    IF(CONFIGH_INCLUDE_DIR)
      INCLUDE_DIRECTORIES(${CONFIGH_INCLUDE_DIR})
    ELSE(CONFIGH_INCLUDE_DIR)
      MESSAGE(FATAL_ERROR "Cannot find config.h header file")
    ENDIF(CONFIGH_INCLUDE_DIR)

  ENDIF(HAVE_CONFIG_H)

  # Build options (do it only once).
  IF(NOT _ALREADY_DONE)

    # Default build configuration.
    IF(NOT MSVC)
      IF(NOT CMAKE_BUILD_TYPE)
        SET(CMAKE_BUILD_TYPE "Release" CACHE STRING
            "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel."
            FORCE)
      ENDIF()
      MESSAGE(STATUS "Build configuration : ${CMAKE_BUILD_TYPE}")
    ENDIF(NOT MSVC)

    # CMake options.
    SET(OPTION_CHECK_CONTENTS false CACHE BOOL "Set to On if you don't want the build to be stopped by missing optional contents folders")
    MARK_AS_ADVANCED(OPTION_CHECK_CONTENTS)

    SET(OPTION_OFFICIAL_ONLY false CACHE BOOL "Build / install only officially released contents")

    SET(OPTION_FORCE_DEBUG false CACHE BOOL "Force debug symbols even in Release build (Automatic in Debug builds)")

    SET(OPTION_TRACE true CACHE BOOL "Enable traces into the console or log file")

    SET(OPTION_TRACE_LEVEL "5" CACHE STRING "Trace level integer threshold, only if OPTION_TRACE (traces with higher level are not logged ; 0=Fatal, 1=Error, 2=Warning, 3=Info, 4=Trace, 5=Debug, ...)")

    SET(OPTION_PROFILER false CACHE BOOL "Enable profiler")
  
    SET(OPTION_SCHEDULE_SPY false CACHE BOOL "Enable fine grained scheduling spy")
  
    SET(OPTION_3RDPARTY_EXPAT true CACHE BOOL "Use 3rd party Expat library rather than bundled TXML")

    SET(OPTION_3RDPARTY_SQLITE3 false CACHE BOOL "Use SQLite3 as database for record/replay")

    SET(OPTION_OSGGRAPH true CACHE BOOL "Build OpenScenGraph-based WIP osggraph graphics module")

    SET(OPTION_SDL2 true CACHE BOOL "Build with SDL2 instead of SDL 1.2")
    SET(OPTION_SDL_JOYSTICK true CACHE BOOL "Use SDL for Joystick instead of PLIB")

    SET(OPTION_WEBSERVER false CACHE BOOL "Build with WebServer functionality")

    IF(APPLE)
      # Automatically set OPTION_USE_MACPORTS (at least until someone fixes the regular APPLE build)
      MESSAGE(STATUS "Automatically set OPTION_USE_MACPORTS (at least until someone fixes the regular APPLE build)")
      SET(OPTION_USE_MACPORTS true CACHE BOOL "Use the MacPorts dependencies")
    ENDIF(APPLE)

    # Enable building with 3rd party SOLID library under Windows, as we ship the binary package,
    # but not under Linux, where FreeSolid seems not to be available by default on most distros.
    IF(WIN32)
      SET(_OPTION_3RDPARTY_SOLID true)
    ELSE(WIN32)
      SET(_OPTION_3RDPARTY_SOLID false)
    ENDIF(WIN32)
    SET(OPTION_3RDPARTY_SOLID ${_OPTION_3RDPARTY_SOLID} CACHE BOOL "Use 3rd party SOLID library rather than simu-bundled one")

    IF(UNIX)
      SET(OPTION_XRANDR true CACHE BOOL "XrandR")  
      SET(OPTION_GLEXTPROTOTYPES true CACHE BOOL "Enable prototypes in glext.h")
      SET(OPTION_UNLOAD_SSGGRAPH true CACHE BOOL "If false, never unload ssggraph module (useful on some Linuxes to avoid XOrg crashes)")  
    ENDIF(UNIX)

    
    IF(OPTION_USE_MACPORTS)
       SET(CMAKE_MACOSX_RPATH TRUE)
       #SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
       SET(CMAKE_PREFIX_PATH "/opt/local" CACHE PATH "Prepended to search path")
       #SET(CMAKE_FIND_ROOT_PATH "/opt/local" CACHE PATH "Prepended to search path")
       SET(CMAKE_FIND_FRAMEWORK LAST)
       MESSAGE(STATUS "Remove the line below to true when OSG works on MacPorts")
       SET(OPTION_OSGGRAPH false CACHE BOOL "Build OpenScenGraph-based WIP osggraph graphics module")
    ENDIF(OPTION_USE_MACPORTS)
	
    SET(OPTION_AUTOVERSION true CACHE BOOL "Enable automatic computation of the version from SVN source tree")
    
    # Custom 3rdParty location for some Windows builds (standard CMake Find<package> macros
    # can't find it, so we needed another solution : see FindCustom3rdParty.cmake).
    IF(MSVC)
      SET(_OPTION_CUSTOM_3RDPARTY true) # Always needed for MSVC compilers.
    ELSEIF(MINGW)
      IF(CMAKE_GENERATOR STREQUAL "MSYS Makefiles")
        # Not needed with "MSYS Makefiles" generator when using MinGW
        # (3rd party libs assumed to be installed in standard location /usr/local).
        SET(_OPTION_CUSTOM_3RDPARTY false)
      ELSE(CMAKE_GENERATOR STREQUAL "MSYS Makefiles")
        # Just as for MSVC builds : special location.
        SET(_OPTION_CUSTOM_3RDPARTY true)
      ENDIF(CMAKE_GENERATOR STREQUAL "MSYS Makefiles")
    ENDIF(MSVC)

    SET(OPTION_CUSTOM_3RDPARTY ${_OPTION_CUSTOM_3RDPARTY} CACHE BOOL "Set to ON to use 3rdParty prebuilt API located in <PROJECT_SOURCE_DIR>/../3rdparty")
    MARK_AS_ADVANCED(OPTION_CUSTOM_3RDPARTY)

    # Compiler definitions (needs more comments. Is it needed under Windows ?).
    ADD_DEFINITIONS(-D_DEFAULT_SOURCE -DSHM)

    IF(MSVC)
	
      # Inhibit definition of Macros min(a,b) and max(a,b) for Windows MSVC builds,
      # as the names conflict with the template functions from standard template library
      ADD_DEFINITIONS(-DNOMINMAX)
	  
    ENDIF(MSVC)

    IF(OPTION_FORCE_DEBUG)
      ADD_DEFINITIONS(-DSD_DEBUG)
    ENDIF(OPTION_FORCE_DEBUG)

    IF(OPTION_TRACE)
      ADD_DEFINITIONS(-DTRACE_OUT)
    ENDIF(OPTION_TRACE)

    IF(OPTION_TRACE_LEVEL)
      ADD_DEFINITIONS(-DTRACE_LEVEL=${OPTION_TRACE_LEVEL})
    ENDIF(OPTION_TRACE_LEVEL)

    IF(OPTION_XRANDR)
      ADD_DEFINITIONS(-DUSE_RANDR_EXT)
    ENDIF(OPTION_XRANDR)

    IF(OPTION_PROFILER)
      ADD_DEFINITIONS(-DPROFILER)
    ENDIF(OPTION_PROFILER)

    IF(OPTION_SCHEDULE_SPY)
      ADD_DEFINITIONS(-DSCHEDULE_SPY)
    ENDIF(OPTION_SCHEDULE_SPY)
  
    IF(OPTION_3RDPARTY_EXPAT)
      ADD_DEFINITIONS(-DTHIRD_PARTY_EXPAT)
    ENDIF(OPTION_3RDPARTY_EXPAT)

    IF(OPTION_3RDPARTY_SQLITE3)
      ADD_DEFINITIONS(-DTHIRD_PARTY_SQLITE3)
    ENDIF(OPTION_3RDPARTY_SQLITE3)

    IF(OPTION_3RDPARTY_SOLID)
      ADD_DEFINITIONS(-DTHIRD_PARTY_SOLID)
    ENDIF(OPTION_3RDPARTY_SOLID)
  
    IF(OPTION_GLEXTPROTOTYPES)
      ADD_DEFINITIONS(-DGL_GLEXT_PROTOTYPES)
    ENDIF(OPTION_GLEXTPROTOTYPES)

    IF(OPTION_UNLOAD_SSGGRAPH)
      ADD_DEFINITIONS(-DUNLOAD_SSGGRAPH)
    ENDIF(OPTION_UNLOAD_SSGGRAPH)

    IF(OPTION_SDL_JOYSTICK)
          ADD_DEFINITIONS(-DSDL_JOYSTICK)
    ENDIF(OPTION_SDL_JOYSTICK)

    IF(OPTION_WEBSERVER)
          ADD_DEFINITIONS(-DWEBSERVER)
    ENDIF(OPTION_WEBSERVER)

    IF(OPTION_USE_MACPORTS)
          ADD_DEFINITIONS(-DUSE_MACPORTS)
    ENDIF(OPTION_USE_MACPORTS)

    # Define for code that needs Torcs backward compatibility
    ADD_DEFINITIONS(-DSPEED_DREAMS)
  
  ENDIF(NOT _ALREADY_DONE)

  # Compile options
  IF(NOT _ALREADY_DONE)

    # GCC warnings (at least for the 4.x series, there are none by default).
    IF(CMAKE_COMPILER_IS_GNUCXX)

      # Add useful warnings / checks.
      SET(_SD_WOPTS "-Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers")

      SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
      SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${_SD_WOPTS}")
      SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${_SD_WOPTS}")

    # MSVC warnings (default warning level 3 is not exactly what we want).
    ELSEIF(MSVC)

      # Suppress bothering MSVC warnings and add useful warnings / checks.
      SET(_SD_WOPTS "/W4") # Better warning level (default seems to be 3).

      # Level 3 useless/boring warnings.
      SET(_SD_WOPTS "${_SD_WOPTS} /wd4251") # class XXX needs a DLL interface
      SET(_SD_WOPTS "${_SD_WOPTS} /wd4996") # std::_Copy_opt disaproved because unsafe

      # Level 4 useless/boring warnings.
      SET(_SD_WOPTS "${_SD_WOPTS} /wd4100") # unreferenced formal parameter
      SET(_SD_WOPTS "${_SD_WOPTS} /wd4127") # conditional expression is constant
      SET(_SD_WOPTS "${_SD_WOPTS} /wd4201") # nonstandard extension used : nameless struct/union
      SET(_SD_WOPTS "${_SD_WOPTS} /wd4706") # assignment within conditional expression

      # Other useless warnings.
      ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_DEPRECATE 
                      -D_CRT_NO_DEPRECATE -D_SCL_SECURE_NO_WARNINGS)
	  
      # That's all.
      SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${_SD_WOPTS}")
      SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${_SD_WOPTS}")

    ENDIF(CMAKE_COMPILER_IS_GNUCXX)

  ENDIF(NOT _ALREADY_DONE)

ENDMACRO(ADD_SD_COMPILE_OPTIONS)
