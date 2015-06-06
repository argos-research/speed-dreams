# - Find ENET library
# Find the native ENET includes and library
# This module defines
#   ENET_INCLUDE_DIR, where to find enet/enet.h
#   ENET_LIBRARY libraries to link against to use ENET
#   ENET_FOUND If false, do not try to use ENET
# $ENETDIR is an environment variable that would correspond
# to the prefix used to configure ENET.
#
# Created by Jean-Philippe Meuret, based on the FindOpenAL.cmake module 
# modified by  Bryan Donlan, from original version by Eric Wang.
FIND_PATH(ENET_INCLUDE_DIR enet/enet.h
  HINTS ENV $ENETDIR
  PATH_SUFFIXES 
	Headers include
  PATHS
    ~/Library/Frameworks/GLFW.framework
    /Library/Frameworks/GLFW.framework
    /System/Library/Frameworks/GLFW.framework # Tiger
    /usr/local
    /usr
    /sw # Fink
    /opt/local # DarwinPorts
    /opt/csw # Blastwave
    /opt)

# I'm not sure if I should do a special casing for Apple. It is
# unlikely that other Unix systems will find the framework path.
# But if they do ([Next|Open|GNU]Step?),
# do they want the -framework option also?
IF(${ENET_INCLUDE_DIR} MATCHES ".framework")
  STRING(REGEX REPLACE "(.*)/.*\\.framework/.*" "\\1" ENET_FRAMEWORK_PATH_TMP ${ENET_INCLUDE_DIR})
  IF("${ENET_FRAMEWORK_PATH_TMP}" STREQUAL "/Library/Frameworks"
      OR "${ENET_FRAMEWORK_PATH_TMP}" STREQUAL "/System/Library/Frameworks"
      )
    # String is in default search path, don't need to use -F
    SET (ENET_LIBRARY "-framework GLFW" CACHE STRING "GLFW framework for OSX")
  ELSE("${ENET_FRAMEWORK_PATH_TMP}" STREQUAL "/Library/Frameworks"
      OR "${ENET_FRAMEWORK_PATH_TMP}" STREQUAL "/System/Library/Frameworks"
      )
    # String is not /Library/Frameworks, need to use -F
    SET(ENET_LIBRARY "-F${ENET_FRAMEWORK_PATH_TMP} -framework GLFW" CACHE STRING "GLFW framework for OSX")
  ENDIF("${ENET_FRAMEWORK_PATH_TMP}" STREQUAL "/Library/Frameworks"
    OR "${ENET_FRAMEWORK_PATH_TMP}" STREQUAL "/System/Library/Frameworks"
    )
  # Clear the temp variable so nobody can see it
  SET(ENET_FRAMEWORK_PATH_TMP "" CACHE INTERNAL "")
 
ELSE(${ENET_INCLUDE_DIR} MATCHES ".framework")
  FIND_LIBRARY(ENET_LIBRARY
    NAMES enet
    HINTS ENV ENETDIR
    PATH_SUFFIXES lib64 lib libs64 libs libs/Win32 libs/Win64
    PATHS
      /usr/local
      /usr
      /sw
      /opt/local
      /opt/csw
      /opt)
ENDIF(${ENET_INCLUDE_DIR} MATCHES ".framework")

SET(ENET_FOUND "NO")
IF (ENET_LIBRARY)
    #If ENET_INCLUDE_DIR points to /usr/include/enet while enet.h is in /usr/include/enet/enet.h,
    #then #include <time.h> will cause /usr/include/enet/time.h be loaded instead of /usr/include/time.h, which causes compiler errors.
    #We warn the user if the wrong include is possibly used
    IF(NOT EXISTS "${ENET_INCLUDE_DIR}/enet/enet.h")
        MESSAGE(WARNING " The file ${ENET_INCLUDE_DIR}/enet/enet.h does not exists. Make sure ENET_INCLUDE_DIR points to the directory containing \"enet/enet.h\" and not to the directory containing \"enet.h\". ENET_INCLUDE_DIR currently has value \"${ENET_INCLUDE_DIR}\"")
    ENDIF(NOT EXISTS "${ENET_INCLUDE_DIR}/enet/enet.h")
    SET(ENET_FOUND "YES")
    #MESSAGE(STATUS "Found ENET: ${ENET_LIBRARY}")
ENDIF(ENET_LIBRARY)


#MARK_AS_ADVANCED(ENET_INCLUDE_DIR ENET_LIBRARY)
