#This file can be distributed with packages such as cars, robots, tracks
#It tries to locate a installed SD version and includes the distributed macro's

SET(SD_PREFIX CACHE PATH "Prefix where Speed Dreams is installed")
SET(SD_DATADIR CACHE PATH "Place where the data is installed")
FIND_FILE(SD_CMAKE_MACROS speed-dreams.cmake PATHS ${SD_PREFIX} ${SD_PREFIX}/${SD_DATADIR} ${SD_DATADIR} /usr /usr/local PATH_SUFFIXES cmake share/cmake share/games/speed-dreams/cmake DOC "Place where Speed Dreams is installed")
IF(NOT SD_CMAKE_MACROS)
	MESSAGE(FATAL_ERROR "Didn't find Speed Dreams. Please specify the location with the SD_PREFIX or SD_DATADIR path.")
ENDIF(NOT SD_CMAKE_MACROS)

INCLUDE(${SD_CMAKE_MACROS})

