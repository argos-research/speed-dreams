# Locate VORBIS libraries ()
# This module defines
# VORBIS_LIBRARY : list of lib names
# VORBIS_FOUND : if false, do not try to link to VORBIS
# VORBIS_INCLUDE_DIR : where to find the headers
#
# $VORBIS_DIR is an environment variable that would
# correspond to the ./configure --prefix=$VORBIS_DIR
# used in building VORBIS.
#
# Created by Joe Thompson (based on Jean-Philippe Meuret's FindSOLID.cmake).

# No use to do all of this twice.
IF(VORBIS_FOUND)
  RETURN()
ENDIF(VORBIS_FOUND)

# First, try with PkgConfig if available.
FIND_PACKAGE(PkgConfig)
IF(PKGCONFIG_FOUND)

  PKG_CHECK_MODULES(VORBIS vorbis)
  IF(NOT VORBIS_FOUND)
    PKG_CHECK_MODULES(VORBIS VORBIS)
  ENDIF (NOT VORBIS_FOUND)

  IF(VORBIS_FOUND)
    SET(VORBIS_FOUND TRUE)
    SET(VORBIS_INCLUDE_DIR ${VORBIS_INCLUDE_DIRS} CACHE STRING "VORBIS include paths")
    SET(VORBIS_LIBRARY ${VORBIS_LIBRARIES} CACHE STRING "VORBIS library")
    MESSAGE(STATUS "Looking for VORBIS --- found using pkg-config (${VORBIS_LIBRARY})")
    RETURN()
  ENDIF(VORBIS_FOUND)
  
ENDIF(PKGCONFIG_FOUND)

# Then try the good old way for include dirs.
IF(NOT APPLE)

  FIND_PATH(VORBIS_INCLUDE_DIR vorbis/vorbisfile.h
    HINTS ENV VORBIS_DIR
    PATH_SUFFIXES 
	  include/vorbis include/VORBIS include
    PATHS
	  /usr /usr/local
    DOC "Non-Apple include dir for VORBIS")

ELSE(NOT APPLE)

  FIND_PATH(VORBIS_INCLUDE_DIR vorbisfile.h
    HINTS ENV VORBIS_DIR
    PATH_SUFFIXES 
	  Headers include/vorbis
    PATHS
	  #Additional MacOS Paths
   	  ~/Library/Frameworks/VORBIS.framework
  	  /Library/Frameworks/VORBIS.framework
 	  /System/Library/Frameworks/VORBIS.framework # Tiger

	  /usr /usr/local
    DOC "Apple include dir for VORBIS")

ENDIF(NOT APPLE)

# Then try the good old way for libs.
FIND_LIBRARY(VORBIS_LIBRARY 
  NAMES vorbis
  HINTS ENV VORBIS_DIR
  PATH_SUFFIXES lib64 lib libs64 libs libs/Win32 libs/Win64
  PATHS /usr /usr/local)

IF(WIN32)

  FIND_LIBRARY(VORBIS_LIBRARY
    NAMES vorbis
    HINTS ENV VORBIS_DIR
    PATH_SUFFIXES lib64 lib libs64 libs libs/Win32 libs/Win64
    PATHS /usr /usr/local)

ENDIF(WIN32)

IF(VORBIS_INCLUDE_DIR AND VORBIS_LIBRARY AND (NOT WIN32 OR VORBIS_LIBRARY))
  SET(VORBIS_FOUND TRUE)
ENDIF(VORBIS_INCLUDE_DIR AND VORBIS_LIBRARY AND (NOT WIN32 OR VORBIS_LIBRARY))

IF(VORBIS_FOUND)
  MESSAGE(STATUS "Looking for VORBIS - found (${VORBIS_LIBRARY})")
  SET(VORBIS_LIBRARY ${VORBIS_LIBRARY})
  IF(WIN32)
    SET(VORBIS_LIBRARY ${VORBIS_LIBRARY})
  ENDIF(WIN32)
ELSE(VORBIS_FOUND)
  MESSAGE(FATAL_ERROR "Could not find VORBIS")
ENDIF(VORBIS_FOUND)

