############################################################################
#
#   file        : macros.cmake
#   copyright   : (C) 2008 by Mart Kelder
#   web         : www.speed-dreams.org 
#   version     : $Id: macros.cmake 5926 2015-03-24 12:38:37Z torcs-ng $
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

# @file     Main CMake configuration file (to be included in every CMakeLists.txt)
# @author   Mart Kelder
# @version  $Id: macros.cmake 5926 2015-03-24 12:38:37Z torcs-ng $

#MESSAGE(STATUS "Processing ${CMAKE_CURRENT_SOURCE_DIR} ...")

# By default, we assume an "in-source" build.
IF(NOT DEFINED IN_SOURCETREE)
  SET(IN_SOURCETREE TRUE)
ENDIF(NOT DEFINED IN_SOURCETREE)

# Setup the install prefix.
IF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  IF(WIN32)
    SET(CMAKE_INSTALL_PREFIX "/speed-dreams-2-build" CACHE PATH "Prefix prepended to install directories" FORCE)
  ELSE(WIN32)
    SET(CMAKE_INSTALL_PREFIX "/usr/local" CACHE PATH "Prefix prepended to install directories" FORCE)
  ENDIF(WIN32)
ENDIF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

# Determine the source folder if ???
IF(NOT SOURCE_DIR AND IN_SOURCETREE)
  FIND_PATH(SOURCE_CMAKE_PATH cmake/macros.cmake PATHS . .. ../.. ../../.. ../../../.. ../../../../.. ../../../../../.. ../../../../../../.. ../../../../../../../.. ${CMAKE_SOURCE_DIR} NO_DEFAULT_PATH)
  FIND_PATH(SOURCE_CMAKE_PATH cmake/macros.cmake PATHS . .. ../.. ../../.. ../../../.. ../../../../.. ../../../../../.. ../../../../../../.. ../../../../../../../.. ${CMAKE_SOURCE_DIR})
  GET_FILENAME_COMPONENT(SOURCE_DIR ${SOURCE_CMAKE_PATH} ABSOLUTE CACHE)
  MARK_AS_ADVANCED(SOURCE_DIR)
  SET(SOURCE_CMAKE_PATH "")
  MARK_AS_ADVANCED(SOURCE_CMAKE_PATH)
ENDIF(NOT SOURCE_DIR AND IN_SOURCETREE)

# The path of the folder of the current CMakeLists.txt
GET_FILENAME_COMPONENT(CURRENT_LIST_FILE_PATH ${CMAKE_CURRENT_LIST_FILE} PATH)

# Macros arg list parsing tools.
IF(NOT _ALREADY_DONE)
  INCLUDE(${CURRENT_LIST_FILE_PATH}/splitargn.cmake)
ENDIF(NOT _ALREADY_DONE)

# Include dir for config.h
IF(IN_SOURCETREE)
  FIND_PATH(CONFIGH_INCLUDE_DIR config.h PATHS ${SOURCE_DIR} ${CMAKE_BINARY_DIR} NO_DEFAULT_PATH)
  FIND_PATH(CONFIGH_INCLUDE_DIR config.h PATHS ${SOURCE_DIR} ${CMAKE_BINARY_DIR})
  MARK_AS_ADVANCED(CONFIGH_INCLUDE_DIR)
  IF(CONFIGH_INCLUDE_DIR)
    SET(HAVE_CONFIG_H TRUE)
  ELSE(CONFIGH_INCLUDE_DIR)
    SET(HAVE_CONFIG_H FALSE)
  ENDIF(CONFIGH_INCLUDE_DIR)
ELSE(IN_SOURCETREE)
  SET(HAVE_CONFIG_H FALSE)
ENDIF(IN_SOURCETREE)

# Determine the default value of the user settings folder.
IF(WIN32)
  SET(SD_LOCALDIR "~/speed-dreams-2.settings" CACHE DOC "Where the user settings files should go")
ELSE(WIN32) #UNIX
  SET(SD_LOCALDIR "~/.speed-dreams-2" CACHE DOC "Where the user settings files should go")
ENDIF(WIN32)

# Determine the default value of the tools executable file prefix.
SET(SD_TOOLS_EXECPREFIX "sd2-" CACHE DOC "Prefix for the tools executable names")
MARK_AS_ADVANCED(SD_TOOLS_EXECPREFIX)

# Determine the default value of the data, bin and lib (and man) folders.
IF(IN_SOURCETREE)
  IF(CMAKE_SIZEOF_VOID_P MATCHES 4)
    SET(_DEFLIBDIR "lib")
  ELSE()
    SET(_DEFLIBDIR "lib64")
  ENDIF()
  IF(WIN32)
    SET(SD_BINDIR bin CACHE PATH "Place where the executables should go")
    SET(SD_DATADIR data CACHE PATH "Place where all the static data files should go")
    SET(SD_LIBDIR ${_DEFLIBDIR} CACHE PATH "Place where the libraries should go")
    SET(SD_INCLUDEDIR include CACHE PATH "Place where the include files should go")
  ELSE(WIN32) #UNIX
    SET(SD_BINDIR games CACHE PATH "Place where the executables should go")
    SET(SD_DATADIR share/games/speed-dreams-2 CACHE PATH "Place where all the static data files should go")
    SET(SD_LIBDIR ${_DEFLIBDIR}/games/speed-dreams-2 CACHE PATH "Place where the libraries should go")
    SET(SD_INCLUDEDIR include/speed-dreams-2 CACHE PATH "Place where the include files should go")
    SET(SD_MANDIR share/man CACHE PATH "Place where the manual pages should go")
  ENDIF(WIN32)
  MARK_AS_ADVANCED(SD_BINDIR)
  MARK_AS_ADVANCED(SD_DATADIR)
  MARK_AS_ADVANCED(SD_LIBDIR)
  MARK_AS_ADVANCED(SD_INCLUDEDIR)
  IF(UNIX)
    MARK_AS_ADVANCED(SD_MANDIR)
  ENDIF(UNIX)
ELSE(IN_SOURCETREE)
  SET(SD_DATADIR ${SD_DATADIR_ABS})
  SET(SD_LIBDIR ${SD_LIBDIR_ABS})
  SET(SD_BINDIR ${SD_BINDIR_ABS})
  SET(SD_INCLUDEDIR ${SD_INCLUDEDIR_ABS})
  IF(UNIX)
    SET(SD_MANDIR ${SD_MANDIR_ABS})
  ENDIF(UNIX)
ENDIF(IN_SOURCETREE)

# Determine the aboslute paths of the data, bin and lib (and man) folders.
IF(IS_ABSOLUTE ${SD_DATADIR})
  GET_FILENAME_COMPONENT(SD_DATADIR_ABS ${SD_DATADIR} ABSOLUTE)
ELSE(IS_ABSOLUTE ${SD_DATADIR})
  GET_FILENAME_COMPONENT(SD_DATADIR_ABS ${CMAKE_INSTALL_PREFIX}/${SD_DATADIR} ABSOLUTE)
ENDIF(IS_ABSOLUTE ${SD_DATADIR})

IF(IS_ABSOLUTE ${SD_LIBDIR})
  GET_FILENAME_COMPONENT(SD_LIBDIR_ABS ${SD_LIBDIR} ABSOLUTE)
ELSE(IS_ABSOLUTE ${SD_LIBDIR})
  GET_FILENAME_COMPONENT(SD_LIBDIR_ABS ${CMAKE_INSTALL_PREFIX}/${SD_LIBDIR} ABSOLUTE)
ENDIF(IS_ABSOLUTE ${SD_LIBDIR})

IF(IS_ABSOLUTE ${SD_BINDIR})
  GET_FILENAME_COMPONENT(SD_BINDIR_ABS ${SD_BINDIR} ABSOLUTE)
ELSE(IS_ABSOLUTE ${SD_BINDIR})
  GET_FILENAME_COMPONENT(SD_BINDIR_ABS ${CMAKE_INSTALL_PREFIX}/${SD_BINDIR} ABSOLUTE)
ENDIF(IS_ABSOLUTE ${SD_BINDIR})

IF(IS_ABSOLUTE ${SD_INCLUDEDIR})
  GET_FILENAME_COMPONENT(SD_INCLUDEDIR_ABS ${SD_INCLUDEDIR} ABSOLUTE)
ELSE(IS_ABSOLUTE ${SD_INCLUDEDIR})
  GET_FILENAME_COMPONENT(SD_INCLUDEDIR_ABS ${CMAKE_INSTALL_PREFIX}/${SD_INCLUDEDIR} ABSOLUTE)
ENDIF(IS_ABSOLUTE ${SD_INCLUDEDIR})

IF(UNIX)
  IF(IS_ABSOLUTE ${SD_MANDIR})
    GET_FILENAME_COMPONENT(SD_MANDIR_ABS ${SD_MANDIR} ABSOLUTE)
  ELSE(IS_ABSOLUTE ${SD_MANDIR})
    GET_FILENAME_COMPONENT(SD_MANDIR_ABS ${CMAKE_INSTALL_PREFIX}/${SD_MANDIR} ABSOLUTE)
  ENDIF(IS_ABSOLUTE ${SD_MANDIR})
ENDIF(UNIX)

STRING(REGEX REPLACE "^(.*[^/])/*$" "\\1" SD_LOCALDIR_TMP ${SD_LOCALDIR})
SET(SD_LOCALDIR ${SD_LOCALDIR_TMP})

SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CURRENT_LIST_FILE_PATH})
SET(CMAKE_INSTALL_RPATH "${SD_LIBDIR_ABS}/lib")

# Configuration options macros.
INCLUDE(${CURRENT_LIST_FILE_PATH}/options.cmake)

# Robots-related macros.
INCLUDE(${CURRENT_LIST_FILE_PATH}/robot.cmake)

# Robots-related macros.
INCLUDE(${CURRENT_LIST_FILE_PATH}/install.cmake)

# Internal dependencies macros (includes and libs).
INCLUDE(${CURRENT_LIST_FILE_PATH}/internaldeps.cmake)

# 3rd party dependencies macros (includes and libs).
INCLUDE(${CURRENT_LIST_FILE_PATH}/thirdpartydeps.cmake)

# Use as a replacement of native ADD_DIRECTORY if the target folder may be optional
# (if it is actually not there, and OPTION_CHECK_CONTENTS is Off,
#  then the build will continue with a simple status message).
MACRO(SD_ADD_SUBDIRECTORY DIR_PATH)

  IF(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${DIR_PATH} OR OPTION_CHECK_CONTENTS)
    ADD_SUBDIRECTORY(${DIR_PATH})
  ELSE()
    MESSAGE(STATUS "Note : Won't build missing dir. ${DIR_PATH}")
  ENDIF()

ENDMACRO(SD_ADD_SUBDIRECTORY PATH)

# Replacement of standard ADD_EXECUTABLE command (same interface).
MACRO(SD_ADD_EXECUTABLE TARGET_NAME)

  # Standard ADD_EXECUTABLE command.
  ADD_EXECUTABLE(${TARGET_NAME} ${ARGN})

  # Change target location (for running in build-tree without installing).
  SET(_TGT_DIR "${CMAKE_BINARY_DIR}/${SD_BINDIR}")

  SET_TARGET_PROPERTIES(${TARGET_NAME} PROPERTIES 
                        RUNTIME_OUTPUT_DIRECTORY ${_TGT_DIR})

  IF(MSVC)

    FOREACH(_CFG ${CMAKE_CONFIGURATION_TYPES})
      STRING(TOUPPER ${_CFG} _CFG)
      SET_TARGET_PROPERTIES(${TARGET_NAME} PROPERTIES 
                            RUNTIME_OUTPUT_DIRECTORY_${_CFG} "${_TGT_DIR}")
    ENDFOREACH()

  ENDIF(MSVC)

  # Make the "settings_versions" target depend on this target,
  # in order settings_versions is built after them.
  ADD_DEPENDENCIES(settings_versions ${TARGET_NAME})

ENDMACRO(SD_ADD_EXECUTABLE TARGET_NAME)


# Replacement of standard ADD_LIBRARY command,
# in order to take care of :
# * changing target location, for running in build-tree without installing,
# * changing target name for modules and robot DLLs (no "lib" prefix).
# Nearly same behaviour as standard ADD_LIBRARY, but :
# * more library types (possible values for TARGET_TYPE arg) :
#   - STATIC, SHARED, MODULE : no change,
#   - ROBOT : same as MODULE for standard ADD_LIBRARY.
# * TARGET_TYPE type arg is mandatory (no default).
MACRO(SD_ADD_LIBRARY TARGET_NAME TARGET_TYPE)

  # Standard ADD_EXECUTABLE command.
  IF(${TARGET_TYPE} STREQUAL "ROBOT")
    ADD_LIBRARY(${TARGET_NAME} MODULE ${ARGN})
  ELSE()
    ADD_LIBRARY(${TARGET_NAME} ${TARGET_TYPE} ${ARGN})
  ENDIF()

  # Determine target location (for running in build-tree without installing).
  IF(${TARGET_TYPE} STREQUAL "SHARED")

    IF(WIN32)
      SET(_TGT_DIR "${CMAKE_BINARY_DIR}/${SD_BINDIR}")
    ELSE()
      SET(_TGT_DIR "${CMAKE_BINARY_DIR}/${SD_LIBDIR}/lib")
    ENDIF()

  ELSEIF(${TARGET_TYPE} STREQUAL "MODULE")
    IF(CMAKE_MAJOR_VERSION GREATER 2)
		cmake_policy(SET CMP0026 OLD)
		cmake_policy(SET CMP0045 OLD)
	ENDIF()
	
    GET_TARGET_PROPERTY(_TGT_LOC ${TARGET_NAME} LOCATION)
    GET_FILENAME_COMPONENT(_TGT_TYPE ${_TGT_LOC} PATH)
    GET_FILENAME_COMPONENT(_TGT_TYPE ${_TGT_TYPE} PATH)
    IF(MSVC)
      # Take care of the build config-specific Debug/Release/... folder.
      GET_FILENAME_COMPONENT(_TGT_TYPE ${_TGT_TYPE} PATH)
    ENDIF()
    GET_FILENAME_COMPONENT(_TGT_TYPE ${_TGT_TYPE} NAME)

    SET(_TGT_DIR "${CMAKE_BINARY_DIR}/${SD_LIBDIR}/modules/${_TGT_TYPE}")

  ELSEIF(${TARGET_TYPE} STREQUAL "ROBOT")

    SET(_TGT_DIR "${CMAKE_BINARY_DIR}/${SD_LIBDIR}/drivers/${TARGET_NAME}")

  ELSEIF(NOT ${TARGET_TYPE} STREQUAL "STATIC")

    MESSAGE(FATAL "Unsupported library type ${TARGET_TYPE} for ${TARGET_NAME}")

  ENDIF()

  # Change target location (for running in build-tree without installing).
  SET_TARGET_PROPERTIES(${TARGET_NAME} PROPERTIES 
                        RUNTIME_OUTPUT_DIRECTORY "${_TGT_DIR}" 
                        LIBRARY_OUTPUT_DIRECTORY "${_TGT_DIR}")

  IF(MSVC)

    FOREACH(_CFG ${CMAKE_CONFIGURATION_TYPES})
      STRING(TOUPPER ${_CFG} _CFG)
      SET_TARGET_PROPERTIES(${TARGET_NAME} PROPERTIES 
                            RUNTIME_OUTPUT_DIRECTORY_${_CFG} "${_TGT_DIR}" 
                            LIBRARY_OUTPUT_DIRECTORY_${_CFG} "${_TGT_DIR}")
    ENDFOREACH()

  ENDIF(MSVC)

  # No prefix for module and robot DLLs.
  IF(${TARGET_TYPE} STREQUAL "MODULE" OR ${TARGET_TYPE} STREQUAL "ROBOT")

    IF(UNIX OR MINGW)
      SET_TARGET_PROPERTIES(${TARGET_NAME} PROPERTIES PREFIX "")
    ENDIF(UNIX OR MINGW)

  ENDIF()

  # Make the "settings_versions" target depend on this target,
  # in order settings_versions is built after them.
  ADD_DEPENDENCIES(settings_versions ${TARGET_NAME})

ENDMACRO(SD_ADD_LIBRARY TARGET_NAME TARGET_TYPE)



# Generate clobber.sh/bat shell script (remove _any_ build system generated file)
MACRO(SD_GENERATE_CLOBBER_SCRIPT)

    IF(MSVC)
  
      SET(TGT_SCRIPT "${SOURCE_DIR}/clobber.bat")
      FILE(WRITE  "${TGT_SCRIPT}" "@echo off\n")
      FILE(APPEND "${TGT_SCRIPT}" "rem CMake-generated script for in-source build tree total cleanup\n")
      FILE(APPEND "${TGT_SCRIPT}" "rem (remove any build-system-generated file (+ .bak, *~, ... etc), \n")
      FILE(APPEND "${TGT_SCRIPT}" "rem  in case you want to get back to something like\n")
      FILE(APPEND "${TGT_SCRIPT}" "rem  right after a 'svn checkout' command).\n")
      FILE(APPEND "${TGT_SCRIPT}" "\n")
      FILE(APPEND "${TGT_SCRIPT}" "rem Check if we are on top of a CMake-enabled SD source tree\n")
      FILE(APPEND "${TGT_SCRIPT}" "if not exist CMakeLists.txt goto ERROR\n")
      FILE(APPEND "${TGT_SCRIPT}" "if not exist cmake goto ERROR\n")
      FILE(APPEND "${TGT_SCRIPT}" "if not exist data goto ERROR\n")
      FILE(APPEND "${TGT_SCRIPT}" "if not exist src goto ERROR\n")
      FILE(APPEND "${TGT_SCRIPT}" "\n")
      FILE(APPEND "${TGT_SCRIPT}" "echo Cleaning up in-source build tree ...\n")
      FILE(APPEND "${TGT_SCRIPT}" "\n")
      FILE(APPEND "${TGT_SCRIPT}" "rem CMake/compiler generated files and dirs\n")
      FILE(APPEND "${TGT_SCRIPT}" "\n")
      FILE(APPEND "${TGT_SCRIPT}" "del CMakeCache.txt\n")
      FILE(APPEND "${TGT_SCRIPT}" "del CMakeLists.txt.user\n")
      FILE(APPEND "${TGT_SCRIPT}" "del config.h\n")
      FILE(APPEND "${TGT_SCRIPT}" "del CPackConfig.cmake\n")
      FILE(APPEND "${TGT_SCRIPT}" "del CPackSourceConfig.cmake\n")
      FILE(APPEND "${TGT_SCRIPT}" "del uninstall.cmake\n")
      FILE(APPEND "${TGT_SCRIPT}" "del doxygen_config\n")
      FILE(APPEND "${TGT_SCRIPT}" "del install_manifest.txt\n")
      FILE(APPEND "${TGT_SCRIPT}" "del speed-dreams-2.ncb\n")
      FILE(APPEND "${TGT_SCRIPT}" "del /ah speed-dreams-2.suo\n")
      FILE(APPEND "${TGT_SCRIPT}" "del speed-dreams-2.sln\n")
      FILE(APPEND "${TGT_SCRIPT}" "del version.h\n")
      FILE(APPEND "${TGT_SCRIPT}" "del xmlversion_loc.txt\n")
      FILE(APPEND "${TGT_SCRIPT}" "del /s /q *.vcproj*\n")
      FILE(APPEND "${TGT_SCRIPT}" "del /s /q cmake_install.cmake\n")
      FILE(APPEND "${TGT_SCRIPT}" "for /f \"tokens=*\" %%G in ('dir /b /ad /s CMakeFiles*') do rmdir /s /q %%G\n")
      FILE(APPEND "${TGT_SCRIPT}" "for /f \"tokens=*\" %%G in ('dir /b /ad /s *.dir') do rmdir /s /q %%G\n")
      FILE(APPEND "${TGT_SCRIPT}" "for /f \"tokens=*\" %%G in ('dir /b /ad /s Debug*') do rmdir /s /q %%G\n")
      FILE(APPEND "${TGT_SCRIPT}" "for /f \"tokens=*\" %%G in ('dir /b /ad /s Release*') do rmdir /s /q %%G\n")
      FILE(APPEND "${TGT_SCRIPT}" "rmdir /s /q _CPack_Packages\n")
      FILE(APPEND "${TGT_SCRIPT}" "\n")
      FILE(APPEND "${TGT_SCRIPT}" "rem Other useless files\n")
      FILE(APPEND "${TGT_SCRIPT}" "del /s /q *.*~\n")
      FILE(APPEND "${TGT_SCRIPT}" "del /s /q *.~*\n")
      FILE(APPEND "${TGT_SCRIPT}" "del /s /q *.bak\n")
      FILE(APPEND "${TGT_SCRIPT}" "del /s /q *.flc\n")
      FILE(APPEND "${TGT_SCRIPT}" "del /s /q *.orig\n")
      FILE(APPEND "${TGT_SCRIPT}" "del /s /q *.cbp\n")
      FILE(APPEND "${TGT_SCRIPT}" "\n")
      FILE(APPEND "${TGT_SCRIPT}" "echo Done.\n")
      FILE(APPEND "${TGT_SCRIPT}" "\n")
      FILE(APPEND "${TGT_SCRIPT}" "goto END\n")
      FILE(APPEND "${TGT_SCRIPT}" "\n")
      FILE(APPEND "${TGT_SCRIPT}" ":ERROR\n")
      FILE(APPEND "${TGT_SCRIPT}" "\n")
      FILE(APPEND "${TGT_SCRIPT}" "echo Bad current dir for that ; please run from the root folder of a CMake-enabled SD source tree.\n")
      FILE(APPEND "${TGT_SCRIPT}" "\n")
      FILE(APPEND "${TGT_SCRIPT}" ":END\n")
  
    ELSE(MSVC)
  
      SET(TGT_SCRIPT "${SOURCE_DIR}/clobber.sh")
      FILE(WRITE  "${TGT_SCRIPT}" "#!/bin/sh\n")
      FILE(APPEND "${TGT_SCRIPT}" "# CMake-generated script for in-source build tree total cleanup\n")
      FILE(APPEND "${TGT_SCRIPT}" "# (remove any build-system-generated file (+ .bak, *~, ... etc), \n")
      FILE(APPEND "${TGT_SCRIPT}" "#  in case you want to get back to something like\n")
      FILE(APPEND "${TGT_SCRIPT}" "#  right after a 'svn checkout' command).\n")
      FILE(APPEND "${TGT_SCRIPT}" "\n")
      FILE(APPEND "${TGT_SCRIPT}" "# Check if we are on top of a CMake-enabled SD source tree\n")
      FILE(APPEND "${TGT_SCRIPT}" "if [ -f CMakeLists.txt -a -d cmake -a -d data -a -d src ] ; then\n")
      FILE(APPEND "${TGT_SCRIPT}" "\n")
      FILE(APPEND "${TGT_SCRIPT}" "  echo \"Cleaning up in-source build tree ...\"\n")
      FILE(APPEND "${TGT_SCRIPT}" "  \n")
      FILE(APPEND "${TGT_SCRIPT}" "  # CMake/compiler generated files and dirs\n")
      FILE(APPEND "${TGT_SCRIPT}" "  rm -f CMakeCache.txt CMakeLists.txt.user config.h doxygen_config\n")
      FILE(APPEND "${TGT_SCRIPT}" "  rm -f CPackConfig.cmake CPackSourceConfig.cmake\n")
      FILE(APPEND "${TGT_SCRIPT}" "  rm -f uninstall.cmake\n")
      FILE(APPEND "${TGT_SCRIPT}" "  rm -f xmlversion_loc.txt install_manifest.txt version.h.txt\n")
      FILE(APPEND "${TGT_SCRIPT}" "  rm -fr _CPack_Packages\n")
      FILE(APPEND "${TGT_SCRIPT}" "  find . -depth -type d -name \"CMakeFiles\" -exec rm -fr {} \\;\n")
      FILE(APPEND "${TGT_SCRIPT}" "  find . -type f -name \"cmake_install.cmake\" -delete\n")
      FILE(APPEND "${TGT_SCRIPT}" "  find . -type f -name \"Makefile\" -delete\n")
      FILE(APPEND "${TGT_SCRIPT}" "  find . -type f -name \"*.so\" -delete\n")
      FILE(APPEND "${TGT_SCRIPT}" "  find src/tools -type f -name \"sd2-*\" -delete\n")
      FILE(APPEND "${TGT_SCRIPT}" "  find src/tools/xmlversion -type f -name \"xmlversion\" -delete\n")
      FILE(APPEND "${TGT_SCRIPT}" "  find src/main -type f -name \"speed-dreams-2\" -delete\n")
      FILE(APPEND "${TGT_SCRIPT}" "\n")
      FILE(APPEND "${TGT_SCRIPT}" "  # Other useless files\n")
      FILE(APPEND "${TGT_SCRIPT}" "  find . -type f -name \"*.rej\" -delete\n")
      FILE(APPEND "${TGT_SCRIPT}" "  find . -type f -name \"*.orig\" -delete\n")
      FILE(APPEND "${TGT_SCRIPT}" "  find . -type f -name \"*.flc\" -delete\n")
      FILE(APPEND "${TGT_SCRIPT}" "  find . -type f -name \"*.bak\" -delete\n")
      FILE(APPEND "${TGT_SCRIPT}" "  find . -type f -name \"*.cbp\" -delete\n")
      FILE(APPEND "${TGT_SCRIPT}" "  find . -type f -name \"*.a\" -delete\n")
      FILE(APPEND "${TGT_SCRIPT}" "  find . -type f -name \"*~\" -delete\n")
      FILE(APPEND "${TGT_SCRIPT}" "  find . -type f -name \"*.~*\" -delete\n")
      FILE(APPEND "${TGT_SCRIPT}" "  \n")
      FILE(APPEND "${TGT_SCRIPT}" "  echo \"Done.\"\n")
      FILE(APPEND "${TGT_SCRIPT}" "  \n")
      FILE(APPEND "${TGT_SCRIPT}" "else\n")
      FILE(APPEND "${TGT_SCRIPT}" "  echo \"Bad current dir for that ; please run from the root folder of a CMake-enabled SD source tree.\"\n")
      FILE(APPEND "${TGT_SCRIPT}" "fi\n")
      EXECUTE_PROCESS(COMMAND chmod ugo+x ${TGT_SCRIPT})
    ENDIF(MSVC)
  
ENDMACRO(SD_GENERATE_CLOBBER_SCRIPT)

# Add non-default compile options.
ADD_SD_COMPILE_OPTIONS()

# A useful variable for things that only need to be done once
# (macros.cmake is actually included by every CMakeLists.txt,
#  in order one can run 'cmake .' everywhere in the source tree,
#  but the bad side effect if that it is thus often included
#  _multiple_ times by every CMakeLists.txt).
IF(NOT _ALREADY_DONE)
  SET(_ALREADY_DONE TRUE)
ENDIF(NOT _ALREADY_DONE)
