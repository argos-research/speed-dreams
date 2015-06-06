# Locate SOLID libraries (collision detection for solid bodies)
# This module defines
# SOLID_SOLID_LIBRARY, SOLID_BROAD_LIBRARY : name of each lib
# SOLID_LIBRARY : list of lib names
# SOLID_FOUND : if false, do not try to link to SOLID
# SOLID_INCLUDE_DIR : where to find the headers
#
# $SOLID_DIR is an environment variable that would
# correspond to the ./configure --prefix=$SOLID_DIR
# used in building SOLID.
#
# Created by Jean-Philippe Meuret (based on Mart Kelder's FindPLIB.cmake).

# No use to do all of this twice.
IF(SOLID_FOUND)
  RETURN()
ENDIF(SOLID_FOUND)

# First, try with PkgConfig if available.
FIND_PACKAGE(PkgConfig)
IF(PKGCONFIG_FOUND)

  PKG_CHECK_MODULES(SOLID FreeSOLID)
  IF(NOT SOLID_FOUND)
    PKG_CHECK_MODULES(SOLID SOLID)
  ENDIF (NOT SOLID_FOUND)

  IF(SOLID_FOUND)
    SET(SOLID_FOUND TRUE)
    SET(SOLID_INCLUDE_DIR ${SOLID_INCLUDE_DIRS} CACHE STRING "SOLID include paths")
    SET(SOLID_SOLID_LIBRARY ${SOLID_LIBRARIES} CACHE STRING "SOLID library")
    SET(SOLID_BROAD_LIBRARY "NOT-FOUND" CACHE STRING "BROAD libraries")
    SET(SOLID_LIBRARY ${SOLID_SOLID_LIBRARY})
    MESSAGE(STATUS "Looking for SOLID --- found using pkg-config (${SOLID_SOLID_LIBRARY})")
    RETURN()
  ENDIF(SOLID_FOUND)
  
ENDIF(PKGCONFIG_FOUND)

# Then try the good old way for include dirs.
IF(NOT APPLE)

  FIND_PATH(SOLID_INCLUDE_DIR SOLID/solid.h
    HINTS ENV SOLID_DIR
    PATH_SUFFIXES 
	  include/FreeSOLID include/freesolid include/SOLID include
    PATHS
	  /usr /usr/local
    DOC "Non-Apple include dir for SOLID")

ELSE(NOT APPLE)

  FIND_PATH(SOLID_INCLUDE_DIR solid.h
    HINTS ENV SOLID_DIR
    PATH_SUFFIXES 
	  Headers include/FreeSOLID include/freesolid include/SOLID include
    PATHS
	  #Additional MacOS Paths
   	  ~/Library/Frameworks/SOLID.framework
  	  /Library/Frameworks/SOLID.framework
 	  /System/Library/Frameworks/SOLID.framework # Tiger

	  /usr /usr/local
    DOC "Apple include dir for SOLID")

ENDIF(NOT APPLE)

# Then try the good old way for libs.
FIND_LIBRARY(SOLID_SOLID_LIBRARY 
  NAMES solid FreeSOLID freesolid
  HINTS ENV SOLID_DIR
  PATH_SUFFIXES lib64 lib libs64 libs libs/Win32 libs/Win64
  PATHS /usr /usr/local)

IF(WIN32)

  FIND_LIBRARY(SOLID_BROAD_LIBRARY
    NAMES broad
    HINTS ENV SOLID_DIR
    PATH_SUFFIXES lib64 lib libs64 libs libs/Win32 libs/Win64
    PATHS /usr /usr/local)

ENDIF(WIN32)

IF(SOLID_INCLUDE_DIR AND SOLID_SOLID_LIBRARY AND (NOT WIN32 OR SOLID_BROAD_LIBRARY))
  SET(SOLID_FOUND TRUE)
ENDIF(SOLID_INCLUDE_DIR AND SOLID_SOLID_LIBRARY AND (NOT WIN32 OR SOLID_BROAD_LIBRARY))

IF(SOLID_FOUND)
  MESSAGE(STATUS "Looking for SOLID - found (${SOLID_SOLID_LIBRARY})")
  SET(SOLID_LIBRARY ${SOLID_SOLID_LIBRARY})
  IF(WIN32)
    SET(SOLID_LIBRARY ${SOLID_LIBRARY} ${SOLID_BROAD_LIBRARY})
  ENDIF(WIN32)
ELSE(SOLID_FOUND)
  MESSAGE(FATAL_ERROR "Could not find SOLID")
ENDIF(SOLID_FOUND)

