# Locate VORBISFILE libraries ()
# This module defines
# VORBISFILE_LIBRARY : list of lib names
# VORBISFILE_FOUND : if false, do not try to link to VORBISFILE
# VORBISFILE_INCLUDE_DIR : where to find the headers
#
# $VORBISFILE_DIR is an environment variable that would
# correspond to the ./configure --prefix=$VORBISFILE_DIR
# used in building VORBISFILE.
#
# Created by Joe Thompson (based on Jean-Philippe Meuret's FindSOLID.cmake).

# No use to do all of this twice.
IF(VORBISFILE_FOUND)
  RETURN()
ENDIF(VORBISFILE_FOUND)

# First, try with PkgConfig if available.
FIND_PACKAGE(PkgConfig)
IF(PKGCONFIG_FOUND)

  PKG_CHECK_MODULES(VORBISFILE vorbisfile)
  IF(NOT VORBISFILE_FOUND)
    PKG_CHECK_MODULES(VORBISFILE VORBISFILE)
  ENDIF (NOT VORBISFILE_FOUND)

  IF(VORBISFILE_FOUND)
    SET(VORBISFILE_FOUND TRUE)
    SET(VORBISFILE_INCLUDE_DIR ${VORBISFILE_INCLUDE_DIRS} CACHE STRING "VORBISFILE include paths")
    SET(VORBISFILE_LIBRARY ${VORBISFILE_LIBRARIES} CACHE STRING "VORBISFILE library")
    MESSAGE(STATUS "Looking for VORBISFILE --- found using pkg-config (${VORBISFILE_LIBRARY})")
    RETURN()
  ENDIF(VORBISFILE_FOUND)
  
ENDIF(PKGCONFIG_FOUND)

# Then try the good old way for include dirs.
IF(NOT APPLE)

  FIND_PATH(VORBISFILE_INCLUDE_DIR vorbis/vorbisfile.h
    HINTS ENV VORBISFILE_DIR
    PATH_SUFFIXES 
	  include/vorbis include/VORBIS include
    PATHS
	  /usr /usr/local
    DOC "Non-Apple include dir for VORBISFILE")

ELSE(NOT APPLE)

  FIND_PATH(VORBISFILE_INCLUDE_DIR vorbisfile.h
    HINTS ENV VORBISFILE_DIR
    PATH_SUFFIXES 
	  Headers include/vorbis
    PATHS
	  #Additional MacOS Paths
   	  ~/Library/Frameworks/VORBIS.framework
  	  /Library/Frameworks/VORBIS.framework
 	  /System/Library/Frameworks/VORBIS.framework # Tiger

	  /usr /usr/local
    DOC "Apple include dir for VORBISFILE")

ENDIF(NOT APPLE)

# Then try the good old way for libs.
FIND_LIBRARY(VORBISFILE_LIBRARY 
  NAMES vorbisfile
  HINTS ENV VORBISFILE_DIR
  PATH_SUFFIXES lib64 lib libs64 libs libs/Win32 libs/Win64
  PATHS /usr /usr/local)

IF(WIN32)

  FIND_LIBRARY(VORBISFILE_LIBRARY
    NAMES vorbisfile
    HINTS ENV VORBISFILE_DIR
    PATH_SUFFIXES lib64 lib libs64 libs libs/Win32 libs/Win64
    PATHS /usr /usr/local)

ENDIF(WIN32)

IF(VORBISFILE_INCLUDE_DIR AND VORBISFILE_LIBRARY AND (NOT WIN32 OR VORBISFILE_LIBRARY))
  SET(VORBISFILE_FOUND TRUE)
ENDIF(VORBISFILE_INCLUDE_DIR AND VORBISFILE_LIBRARY AND (NOT WIN32 OR VORBISFILE_LIBRARY))

IF(VORBISFILE_FOUND)
  MESSAGE(STATUS "Looking for VORBISFILE - found (${VORBISFILE_LIBRARY})")
  SET(VORBISFILE_LIBRARY ${VORBISFILE_LIBRARY})
  IF(WIN32)
    SET(VORBISFILE_LIBRARY ${VORBISFILE_LIBRARY})
  ENDIF(WIN32)
ELSE(VORBISFILE_FOUND)
  MESSAGE(FATAL_ERROR "Could not find VORBISFILE")
ENDIF(VORBISFILE_FOUND)

