# Locate OGG libraries ()
# This module defines
# OGG_LIBRARY : list of lib names
# OGG_FOUND : if false, do not try to link to OGG
# OGG_INCLUDE_DIR : where to find the headers
#
# $OGG_DIR is an environment variable that would
# correspond to the ./configure --prefix=$OGG_DIR
# used in building OGG.
#
# Created by Joe Thompson (based on Jean-Philippe Meuret's FindSOLID.cmake).

# No use to do all of this twice.
IF(OGG_FOUND)
  RETURN()
ENDIF(OGG_FOUND)

# First, try with PkgConfig if available.
FIND_PACKAGE(PkgConfig)
IF(PKGCONFIG_FOUND)

  PKG_CHECK_MODULES(OGG ogg)
  IF(NOT OGG_FOUND)
    PKG_CHECK_MODULES(OGG OGG)
  ENDIF (NOT OGG_FOUND)

  IF(OGG_FOUND)
    SET(OGG_FOUND TRUE)
    SET(OGG_INCLUDE_DIR ${OGG_INCLUDE_DIRS} CACHE STRING "OGG include paths")
    SET(OGG_LIBRARY ${OGG_LIBRARIES} CACHE STRING "OGG library")
    MESSAGE(STATUS "Looking for OGG --- found using pkg-config (${OGG_LIBRARY})")
    RETURN()
  ENDIF(OGG_FOUND)
  
ENDIF(PKGCONFIG_FOUND)

# Then try the good old way for include dirs.
IF(NOT APPLE)

  FIND_PATH(OGG_INCLUDE_DIR ogg/ogg.h
    HINTS ENV OGG_DIR
    PATH_SUFFIXES 
	  include/ogg include/OGG include
    PATHS
	  /usr /usr/local
    DOC "Non-Apple include dir for OGG")

ELSE(NOT APPLE)

  FIND_PATH(OGG_INCLUDE_DIR ogg.h
    HINTS ENV OGG_DIR
    PATH_SUFFIXES 
	  Headers include/ogg
    PATHS
	  #Additional MacOS Paths
   	  ~/Library/Frameworks/OGG.framework
  	  /Library/Frameworks/OGG.framework
 	  /System/Library/Frameworks/OGG.framework # Tiger

	  /usr /usr/local
    DOC "Apple include dir for OGG")

ENDIF(NOT APPLE)

# Then try the good old way for libs.
FIND_LIBRARY(OGG_LIBRARY 
  NAMES ogg
  HINTS ENV OGG_DIR
  PATH_SUFFIXES lib64 lib libs64 libs libs/Win32 libs/Win64
  PATHS /usr /usr/local)

IF(WIN32)

  FIND_LIBRARY(OGG_LIBRARY
    NAMES ogg
    HINTS ENV OGG_DIR
    PATH_SUFFIXES lib64 lib libs64 libs libs/Win32 libs/Win64
    PATHS /usr /usr/local)

ENDIF(WIN32)

IF(OGG_INCLUDE_DIR AND OGG_LIBRARY AND (NOT WIN32 OR OGG_LIBRARY))
  SET(OGG_FOUND TRUE)
ENDIF(OGG_INCLUDE_DIR AND OGG_LIBRARY AND (NOT WIN32 OR OGG_LIBRARY))

IF(OGG_FOUND)
  MESSAGE(STATUS "Looking for OGG - found (${OGG_LIBRARY})")
  SET(OGG_LIBRARY ${OGG_LIBRARY})
  IF(WIN32)
    SET(OGG_LIBRARY ${OGG_LIBRARY})
  ENDIF(WIN32)
ELSE(OGG_FOUND)
  MESSAGE(FATAL_ERROR "Could not find OGG")
ENDIF(OGG_FOUND)

