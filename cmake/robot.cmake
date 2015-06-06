############################################################################
#
#   file        : robot.cmake
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

# @file     Robots-related macros
# @author   Mart Kelder
# @version  $Id$

# Robot .def file generation for Windows builds with MSVC compilers
#  ROBOTNAME  : Name of the robot
#  DEF_FILE  : Target .def file  path-name
#  Other args  : Robot DLL Interface description, as a list of names of exported symbols,
#                 but with keywords shortcuts :
#                 - if empty, assumed keyword "LEGACY_MIN"
#                 - if "LEGACY_MIN", means the smallest legacy scheme interface  (TORCS style)
#                                    with only "${NAME}" exported
#                 - if "LEGACY", means the  complete legacy scheme interface,
#                                with "${NAME}" and "${NAME}Shut" exported
#                 - if "WELCOME", means the complete new scheme interface (Speed Dreams style),
#                                 with "moduleWelcome", "moduleInitialize" and "moduleTerminate"
#                                 exported
#                 - may be a list of above keywords for multi-scheme interface
#                 - may also be the raw list of symbols to export
MACRO(GENERATE_ROBOT_DEF_FILE ROBOTNAME DEF_FILE)

  # Build the real list of exported symbols from the given one (that may include shortcuts)
  #MESSAGE(STATUS "Generating ${DEF_FILE} for ${ROBOTNAME} robot")
  SET(SYMBOLS) # Initialize the list
  FOREACH(KEYSYM ${ARGN})
    IF(KEYSYM STREQUAL "LEGACY_MIN")
      LIST(APPEND SYMBOLS ${ROBOTNAME})
    ELSEIF(KEYSYM STREQUAL "LEGACY")
      LIST(APPEND SYMBOLS ${ROBOTNAME})
      LIST(APPEND SYMBOLS "${ROBOTNAME}Shut")
    ELSEIF(KEYSYM STREQUAL "WELCOME")
      LIST(APPEND SYMBOLS moduleWelcome moduleInitialize moduleTerminate)
    ELSE(KEYSYM STREQUAL "LEGACY_MIN")
      LIST(APPEND SYMBOLS ${KEYSYM})
    ENDIF(KEYSYM STREQUAL "LEGACY_MIN")
  ENDFOREACH(KEYSYM ${ARGN})

  # Clean duplicates
  LIST(REMOVE_DUPLICATES SYMBOLS)

  #MESSAGE(STATUS "Symbols: ${SYMBOLS}")

  # Build an acceptable string for the .def file from this symbol list
  SET(ROBOTSYMBOLS "")
  FOREACH(SYMBOL ${SYMBOLS})
    SET(ROBOTSYMBOLS "${ROBOTSYMBOLS}\n\t${SYMBOL}")
  ENDFOREACH(SYMBOL ${SYMBOLS})

  # Generate the .def file
  SET(ROBOT_NAME "${ROBOTNAME}")
  IF(IN_SOURCETREE)
    CONFIGURE_FILE(${SOURCE_DIR}/cmake/robot.def.in.cmake ${DEF_FILE})
  ELSE(IN_SOURCETREE)
    CONFIGURE_FILE(${SD_DATADIR_ABS}/cmake/robot.def.in.cmake ${DEF_FILE})
  ENDIF(IN_SOURCETREE)

ENDMACRO(GENERATE_ROBOT_DEF_FILE ROBOTNAME DEF_FILE)

# Robot project definition (module build and install, without associated data)
# Args:
#  NAME       : Name of the robot
#  INTERFACE  : Robot Windows DLL Interface description (tells about exported symbols)
#                 See GENERATE_ROBOT_DEF_FILE macro.
#                 If not specified, defaults to "LEGACY_MIN" ; not used if MODULE used
#  SOURCES    : List of files to use as build sources if any ; not needed if MODULE used
#  CLONENAMES : The names of the clones to generate
#  VERSION    : The VERSION of the libraries to produce (robot and its clones) (def: $VERSION).
#  SOVERSION  : The SOVERSION of the libraries to produce (in the ldconfig meaning) (def: 0.0.0).
#               WARNING: Not taken into account for the moment : might not work with GCC 4.5 or +.
#
# Example:
#    ROBOT_MODULE(NAME simplix VERSION 3.0.5 SOVERSION 0.0.0
#                 INTERFACE LEGACY WELCOME simplix_trb1 simplix_ls1 simplix_36GP
#                 SOURCES simplix.cpp ...
#                 CLONENAMES simplix_trb1 simplix_ls1 simplix_36GP)
MACRO(ROBOT_MODULE)

  SET(RBM_SYNTAX "NAME,1,1,RBM_HAS_NAME,RBM_NAME")
  SET(RBM_SYNTAX ${RBM_SYNTAX} "VERSION,0,1,RBM_HAS_VERSION,RBM_VERSION")
  SET(RBM_SYNTAX ${RBM_SYNTAX} "SOVERSION,0,1,RBM_HAS_SOVERSION,RBM_SOVERSION")
  SET(RBM_SYNTAX ${RBM_SYNTAX} "INTERFACE,0,-1,RBM_HAS_INTERFACE,RBM_INTERFACE")
  SET(RBM_SYNTAX ${RBM_SYNTAX} "SOURCES,0,-1,RBM_HAS_SOURCES,RBM_SOURCES")
  SET(RBM_SYNTAX ${RBM_SYNTAX} "CLONENAMES,0,-1,RBM_HAS_CLONENAMES,RBM_CLONENAMES")

  SPLIT_ARGN(${RBM_SYNTAX} ARGUMENTS ${ARGN})

  IF(NOT RBM_HAS_NAME OR NOT RBM_NAME)
    MESSAGE(FATAL_ERROR "Cannot build a robot module with no specified name")
  ENDIF()
  IF(NOT RBM_HAS_SOURCES OR NOT RBM_SOURCES)
    MESSAGE(FATAL_ERROR "Cannot build a robot module without sources / module to copy")
  ENDIF()
  IF(NOT RBM_HAS_VERSION OR NOT RBM_VERSION)
    SET(RBM_VERSION ${VERSION})
    MESSAGE(STATUS "No version specified for robot module ${RBM_NAME} ; using ${RBM_VERSION}")
  ENDIF()
  IF(NOT RBM_HAS_SOVERSION OR NOT RBM_SOVERSION)
    IF(UNIX)
      SET(RBM_SOVERSION 0.0.0)
      MESSAGE(STATUS "No so-version specified for robot module ${RBM_NAME} ; using ${RBM_SOVERSION}")
    ENDIF()
  ENDIF()

  PROJECT("robot_${RBM_NAME}")

  ADD_INTERFACE_INCLUDEDIR()
  ADD_SDLIB_INCLUDEDIR(learning math portability robottools tgf)
  ADD_PLIB_INCLUDEDIR()

  # DLL export stuff under Windows (through a .def file or __declspec pragmas)
  IF(WIN32)
    # If an interface is specified, use the old way.
    IF(RBM_HAS_INTERFACE AND RBM_INTERFACE)
      IF(MSVC)
        # For MSVC compilers, generate / add a .def file for legacy / welcome interface.
        SET(ROBOT_DEF_FILE ${CMAKE_CURRENT_BINARY_DIR}/${RBM_NAME}_gen.def)
        GENERATE_ROBOT_DEF_FILE(${RBM_NAME} ${ROBOT_DEF_FILE} ${RBM_INTERFACE})
        SET(RBM_SOURCES ${RBM_SOURCES} ${ROBOT_DEF_FILE})
      ENDIF()
    # If no interface is specified, assume it's the new modern one.
    ELSE()
      # For any Windows compiler, use __declspec pragmas.
      ADD_DEFINITIONS(-DROBOT_DLL)
    ENDIF()
  ENDIF(WIN32)

  # Disable developer warning
  IF (COMMAND cmake_policy)
    CMAKE_POLICY(SET CMP0003 NEW)
  ENDIF(COMMAND cmake_policy)

  # Ignore some run-time libs to avoid MSVC link-time warnings and sometimes even crashes.
  IF(MSVC)
      SET(CMAKE_MODULE_LINKER_FLAGS_DEBUG "${CMAKE_MODULE_LINKER_FLAGS_DEBUG} /NODEFAULTLIB:msvcrt.lib")
  ENDIF(MSVC)

  # The robot module is actually a shared library.
  SD_ADD_LIBRARY(${RBM_NAME} ROBOT ${RBM_SOURCES})

  # Customize shared library versions.
  IF(UNIX) # Use ldconfig version naming scheme + no "lib" prefix under Linux
    # Might not work with GCC 4.5 or + (non-robot modules crash at 1st reload = after 1 dlclose) 
    #SET_TARGET_PROPERTIES(${RBM_NAME} PROPERTIES VERSION ${RBM_VERSION})
    #SET_TARGET_PROPERTIES(${RBM_NAME} PROPERTIES SOVERSION ${RBM_SOVERSION})
  ELSE()
    SET_TARGET_PROPERTIES(${RBM_NAME} PROPERTIES VERSION ${RBM_VERSION})
  ENDIF()

  # Link/Run-time dependencies
  ADD_PLIB_LIBRARY(${RBM_NAME} sg)

  ADD_SDLIB_LIBRARY(${RBM_NAME} portability tgf robottools)

  # Install target robot module shared library
  SD_INSTALL_FILES(LIB drivers/${RBM_NAME} TARGETS ${RBM_NAME})

  # Install clone robot modules shared libraries (use ldconfig version naming scheme under Linux)
  IF(RBM_HAS_CLONENAMES AND RBM_CLONENAMES)
  
    IF(WIN32)
      GET_TARGET_PROPERTY(MODLOC ${RBM_NAME} RUNTIME_OUTPUT_DIRECTORY)
  	  IF(NOT RUNTIME_OUTPUT_DIRECTORY)
        GET_TARGET_PROPERTY(MODLOC ${RBM_NAME} LIBRARY_OUTPUT_DIRECTORY)
  	  ENDIF()
	  SET(MODLOC "${MODLOC}/${RBM_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX}")
    ELSE(WIN32)
      GET_TARGET_PROPERTY(MODLOC ${RBM_NAME} LOCATION)
    ENDIF(WIN32)

    FOREACH(CLONENAME ${RBM_CLONENAMES})
    
      SET(CLONE_MODDIR "${CMAKE_BINARY_DIR}/${SD_LIBDIR}/drivers/${CLONENAME}")
      SET(CLONE_MODLOC "${CLONE_MODDIR}/${CLONENAME}${CMAKE_SHARED_LIBRARY_SUFFIX}")
      IF(FALSE) #IF(UNIX)
        # Might not work with GCC 4.5 or + (see above) 
        ADD_CUSTOM_COMMAND(TARGET ${RBM_NAME} POST_BUILD
                           COMMAND ${CMAKE_COMMAND} -E echo "Cloning ${RBM_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX} into ${CLONE_MODLOC}.${RBM_VERSION}"
                           COMMAND ${CMAKE_COMMAND} -E make_directory "${CLONE_MODDIR}"
                           COMMAND ${CMAKE_COMMAND} -E copy_if_different ${MODLOC} ${CLONE_MODLOC}.${RBM_VERSION})
        ADD_CUSTOM_COMMAND(TARGET ${RBM_NAME} POST_BUILD
                           COMMAND ${CMAKE_COMMAND} -E create_symlink ${CLONE_MODLOC}.${RBM_VERSION} ${CLONE_MODLOC}.${RBM_SOVERSION}
                           COMMAND ${CMAKE_COMMAND} -E create_symlink ${CLONE_MODLOC}.${RBM_SOVERSION} ${CLONE_MODLOC})
        SD_INSTALL_FILES(LIB drivers/${CLONENAME}
                         FILES ${CLONE_MODLOC} ${CLONE_MODLOC}.${RBM_SOVERSION} ${CLONE_MODLOC}.${RBM_VERSION} )
      ELSE()
        ADD_CUSTOM_COMMAND(TARGET ${RBM_NAME} POST_BUILD
                           COMMAND ${CMAKE_COMMAND} -E echo "Creating directory ${CLONE_MODDIR}"
                           COMMAND ${CMAKE_COMMAND} -E make_directory "${CLONE_MODDIR}"
                           COMMAND ${CMAKE_COMMAND} -E echo "Cloning ${RBM_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX}=${MODLOC} into ${CLONE_MODLOC}"
                           COMMAND ${CMAKE_COMMAND} -E copy_if_different ${MODLOC} ${CLONE_MODLOC})
        SD_INSTALL_FILES(LIB drivers/${CLONENAME}
                         FILES ${CLONE_MODLOC})
      ENDIF()
        
    ENDFOREACH(CLONENAME ${RBM_CLONENAMES})
    
  ENDIF()

ENDMACRO(ROBOT_MODULE)

# Robot data installation
# Args:
#  NAME     : Name of the robot (may be a clone)
#  PREFIX   : Prefix to use to get source path for files/subdirs specified in FILES/SUBDIRS
#  FILES    : Files to install in the robot's data dir (see PREFIX)
#  SUBDIRS  : Sub-dirs to recusively install in the robot's data dir (see PREFIX)
#  PATTERNS : Files to install from SUBDIRS in the robot's data dir, 
#             as glob patterns (defaults to *.*)
#  USER     : If this keyword is present, also mark _any_ above specified XML file
#             as a user settings file (installed at run-time is the user settings folders).
#  
MACRO(ROBOT_DATA)

  SET(RBD_SYNTAX "NAME,1,1,RBD_HAS_NAME,RBD_NAME")
  SET(RBD_SYNTAX ${RBD_SYNTAX} "PREFIX,0,1,RBD_HAS_PREFIX,RBD_PREFIX")
  SET(RBD_SYNTAX ${RBD_SYNTAX} "FILES,0,-1,RBD_HAS_FILES,RBD_FILES")
  SET(RBD_SYNTAX ${RBD_SYNTAX} "SUBDIRS,0,-1,RBD_HAS_SUBDIRS,RBD_SUBDIRS")
  SET(RBD_SYNTAX ${RBD_SYNTAX} "PATTERNS,0,-1,RBD_HAS_PATTERNS,RBD_PATTERNS")
  SET(RBD_SYNTAX ${RBD_SYNTAX} "USER,0,0,RBD_IS_USER,_")

  SPLIT_ARGN(${RBD_SYNTAX} ARGUMENTS ${ARGN})

  #MESSAGE(STATUS "ROBOT_DATA(${RBD_NAME}): PREFIX=${RBD_PREFIX} FILES=${RBD_FILES} PATTERNS=${RBD_PATTERNS} SUBDIRS=${RBD_SUBDIRS} USER=${RBD_IS_USER}")

  # Check arguments syntax / values
  IF(NOT RBD_HAS_NAME OR NOT RBD_NAME)
    MESSAGE(FATAL_ERROR "Cannot install data for a robot module with no specified name")
  ENDIF()

  # Install specified files.
  IF(RBD_HAS_FILES AND RBD_FILES)

    IF(RBD_IS_USER)
      SD_INSTALL_FILES(DATA drivers/${RBD_NAME} USER drivers/${RBD_NAME} 
                PREFIX ${RBD_PREFIX} FILES ${RBD_FILES})
    ELSE()
      SD_INSTALL_FILES(DATA drivers/${RBD_NAME} 
                  PREFIX ${RBD_PREFIX} FILES ${RBD_FILES})
    ENDIF()

  ENDIF()

  # Install subdirs if specified.
  IF(RBD_HAS_SUBDIRS AND RBD_SUBDIRS)

    # Install specified files.
    IF(RBD_IS_USER)
      SD_INSTALL_DIRECTORIES(DATA drivers/${RBD_NAME} USER drivers/${RBD_NAME} 
                             PREFIX ${RBD_PREFIX} DIRECTORIES ${RBD_SUBDIRS} 
                             PATTERNS ${RBD_PATTERNS})
    ELSE()
      SD_INSTALL_DIRECTORIES(DATA drivers/${RBD_NAME} 
                             PREFIX ${RBD_PREFIX} DIRECTORIES ${RBD_SUBDIRS} 
                             PATTERNS ${RBD_PATTERNS})
    ENDIF()

  ENDIF(RBD_HAS_SUBDIRS AND RBD_SUBDIRS)

ENDMACRO(ROBOT_DATA)

# Robot project definition (module build and install, with associated data)
# Args:
#  NAME    : Name of the robot
#  INTERFACE  : Robot Windows DLL Interface description (tells about exported symbols)
#                 See GENERATE_ROBOT_DEF_FILE macro.
#                 If not specified, defaults to "LEGACY_MIN" ; not used if MODULE used
#  SOURCES  : List of files to use as build sources if any ; not needed if MODULE used
#  PREFIX  : Dir. prefix for source files/subdirs to install in the robot's data dir
#  FILES  : Extra (non default) files to install in the robot's data dir
#  SUBDIRS  : Data subdirectories to install in the robot's data dir
MACRO(ROBOT)

  SET(RB_SYNTAX "NAME,1,1,RB_HAS_NAME,RB_NAME")
  SET(RB_SYNTAX ${RB_SYNTAX} "INTERFACE,0,-1,RB_HAS_INTERFACE,RB_INTERFACE")
  SET(RB_SYNTAX ${RB_SYNTAX} "SOURCES,0,-1,RB_HAS_SOURCES,RB_SOURCES")
  SET(RB_SYNTAX ${RB_SYNTAX} "PREFIX,0,-1,RB_HAS_PREFIX,RB_PREFIX")
  SET(RB_SYNTAX ${RB_SYNTAX} "FILES,0,-1,RB_HAS_FILES,RB_FILES")
  SET(RB_SYNTAX ${RB_SYNTAX} "SUBDIRS,0,-1,RB_HAS_SUBDIRS,RB_SUBDIRS")

  #SET(__DEBUG__ TRUE)
  SPLIT_ARGN(${RB_SYNTAX} ARGUMENTS ${ARGN})
  #SET(__DEBUG__ FALSE)

  #MESSAGE(STATUS "ROBOT(${RB_NAME}): INTERFACE=${RB_INTERFACE} (${RB_HAS_INTERFACE}) SOURCES=${RB_SOURCES} (${RB_HAS_SOURCES}) PREFIX=${RB_PREFIX} (${RB_HAS_PREFIX}) FILES=${RB_FILES} (${RB_HAS_FILES}) SUBDIRS=${RB_SUBDIRS} (${RB_HAS_SUBDIRS})")

  IF(NOT RB_HAS_NAME OR NOT RB_NAME)
    MESSAGE(FATAL_ERROR "Cannot build a robot with no specified name")
  ENDIF()

  IF(NOT RB_HAS_SOURCES OR NOT RB_SOURCES)
    MESSAGE(FATAL_ERROR "Cannot build a robot without sources")
  ENDIF()

  ROBOT_MODULE(NAME ${RB_NAME} INTERFACE ${RB_INTERFACE} SOURCES ${RB_SOURCES})

  ROBOT_DATA(NAME ${RB_NAME} PREFIX ${RB_PREFIX} FILES ${RB_FILES} SUBDIRS ${RB_SUBDIRS})

ENDMACRO(ROBOT)
