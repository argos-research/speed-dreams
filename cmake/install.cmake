############################################################################
#
#   file        : install.cmake
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

# @file     CMake install macros
# @author   Mart Kelder, J.-P. Meuret
# @version  $Id$


# Get the real pathname of a target (taking care of the MSVC configuration-driven variables).
# Args :
#  TGT_NAME Name of the target
#  Name of the target variable for the output path-name
MACRO(_GET_TARGET_REAL_PATHNAME TGT_NAME VAR_PATHNAME)

    GET_TARGET_PROPERTY(${VAR_PATHNAME} ${TGT_NAME} LOCATION)
    MESSAGE(STATUS "GET_TARGET_REAL_PATHNAME(${TGT_NAME})=${${VAR_PATHNAME}}")
    IF(MSVC)
      STRING(REPLACE "$(OutDir)" "${CMAKE_INSTALL_CONFIG_NAME}" ${VAR_PATHNAME} ${${VAR_PATHNAME}})
      STRING(REPLACE "$(ConfigurationName)" "${CMAKE_INSTALL_CONFIG_NAME}" ${VAR_PATHNAME} ${${VAR_PATHNAME}})
      STRING(REPLACE "$(Configuration)" "${CMAKE_INSTALL_CONFIG_NAME}" ${VAR_PATHNAME} ${${VAR_PATHNAME}})
    ENDIF(MSVC)

ENDMACRO(_GET_TARGET_REAL_PATHNAME TGT_NAME VAR_PATHNAME)


# Update and install the version.xml file (for user settings files management).
MACRO(SD_SETUP_SETTINGS_VERSION)

  # Create a custom target for updating the version.xml file (through xmlversion.exe)
  # which registers the versions of the (XML) user settings files.
  # This target will depend on all other ones, in order it is built
  # after them, in cas they install such (XML) user settings files
  # (see SD_INSTALL_FILES and SD_INSTALL_DIRECTORIES, with USER flag).
  # Note: We need a specific target for doing this, rather than adding custom
  #       xmlversion.exe commands to each corresponding target,
  #       otherwise parallel builds would likely conflict while updating
  #       the unique version.xml file.
  ADD_CUSTOM_TARGET(settings_versions ALL)

  # Initialize the list of xmlversion args.
  DEFINE_PROPERTY(TARGET PROPERTY XMLVERSION_ARGS
                  BRIEF_DOCS "List of xmlversion args"
                  FULL_DOCS "List of xmlversion args ({src file, target user dir} couples)")

ENDMACRO(SD_SETUP_SETTINGS_VERSION)

# Update and install the version.xml file (for user settings files management).
# Note: This macro must be called in the same folder (CMakeLists.txt) as SD_SETUP_SETTINGS_VERSION
#       because of ADD_CUSTOM_COMMAND(TARGET limitations (will simply be ignored if not same dir
#       as when the ADD_CUSTOM_TARGET was called).
MACRO(SD_UPDATE_SETTINGS_VERSION)

	# Determine the full path-name of xmlversion.exe
    SET(_XMLVER_EXE "${CMAKE_BINARY_DIR}/${SD_BINDIR}/${CMAKE_EXECUTABLE_PREFIX}xmlversion${CMAKE_EXECUTABLE_SUFFIX}")

    #MESSAGE(STATUS "_XMLVER_EXE=${_XMLVER_EXE}")

    # In order to run xmlversion.exe in the build tree (see below), under Windows,
    # we nearly always have to copy 3rd party dependency and compiler run-time DLLs next to it.
    # Note: Not really needed as speed-dreams-2 is built before settings_versions,
    #       and so already did this job. But in case someone changes the build order ...
    IF(WIN32)

	  SET(_DLLS_TO_INSTALL)

	  # Internal dependencies : not needed, as we already generate them in SD_BINDIR.

	  # 3rd party dependencies
	  # (not needed for MinGW builds through the "MSYS Makefiles" generator,
	  #  as in this case, these DLLs are installed in the standard Linux folder /usr/local/bin,
	  #  which is in the PATH under MSYS).
	  # TODO: Use Custom3rdParty macros for this (and thus avoid duplicate code) ?
	  IF(NOT CMAKE_GENERATOR STREQUAL "MSYS Makefiles")

        IF(OPTION_3RDPARTY_EXPAT)
          FIND_PACKAGE(EXPAT)
          GET_FILENAME_COMPONENT(_LIB_PATH "${EXPAT_LIBRARY}" PATH)
          GET_FILENAME_COMPONENT(_LIB_NAME "${EXPAT_LIBRARY}" NAME_WE)
          SET(_DLL_PATHNAME ${_LIB_PATH}/../bin/${_LIB_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX})
	      LIST(APPEND _DLLS_TO_INSTALL ${_DLL_PATHNAME})
        ENDIF()

        FIND_PACKAGE(SDL)
        SET(_LIB_PATHNAME "${SDL_LIBRARY}")
        IF(MINGW)
          # Multiple lib specs fuss ... find the one
          STRING(REGEX REPLACE ".*;([^;]+dll[^;]*);.*" "\\1" _LIB_PATHNAME "${_LIB_PATHNAME}")
        ENDIF(MINGW)
        GET_FILENAME_COMPONENT(_LIB_PATH "${_LIB_PATHNAME}" PATH)
        GET_FILENAME_COMPONENT(_LIB_NAME "${_LIB_PATHNAME}" NAME_WE)
        IF(MINGW)
          STRING(REGEX REPLACE "lib(.*)" "\\1" _LIB_NAME "${_LIB_NAME}")
        ENDIF(MINGW)
        SET(_DLL_PATHNAME ${_LIB_PATH}/../bin/${_LIB_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX})
	    LIST(APPEND _DLLS_TO_INSTALL ${_DLL_PATHNAME})

      ENDIF(NOT CMAKE_GENERATOR STREQUAL "MSYS Makefiles")

	  # Compiler run-time dependencies
	  # (neither needed for MinGW builds through the "MSYS Makefiles" generator,
	  #  - same reason than for the 3rd party dependencies -
	  #  nor for MSVC builds, where Visual Studio takes care itself of it).
	  # TODO: Use Custom3rdParty macros for this (and thus avoid duplicate code) ?
	  IF(MINGW AND NOT CMAKE_GENERATOR STREQUAL "MSYS Makefiles")

		GET_FILENAME_COMPONENT(_MINGW_BINDIR "${CMAKE_CXX_COMPILER}" PATH)
	    LIST(APPEND _DLLS_TO_INSTALL "${_MINGW_BINDIR}/libstdc++-6.dll")
		LIST(APPEND _DLLS_TO_INSTALL "${_MINGW_BINDIR}/libgcc_s_dw2-1.dll")

      ENDIF(MINGW AND NOT CMAKE_GENERATOR STREQUAL "MSYS Makefiles")

      # Copy the dependency DLLs found above.
      #MESSAGE(STATUS "xmlversion : DLLs to install=${_DLLS_TO_INSTALL}")
      IF(_DLLS_TO_INSTALL)
        SET(_TGT_DIR "${CMAKE_BINARY_DIR}/${SD_BINDIR}")
        ADD_CUSTOM_COMMAND(TARGET settings_versions PRE_BUILD
                             COMMAND ${CMAKE_COMMAND} -E echo Creating directory "${_TGT_DIR}"
                             COMMAND ${CMAKE_COMMAND} -E make_directory "${_TGT_DIR}"
                             VERBATIM)
        FOREACH(_DLL ${_DLLS_TO_INSTALL})
          ADD_CUSTOM_COMMAND(TARGET settings_versions PRE_BUILD
                             COMMAND ${CMAKE_COMMAND} -E echo Copying "${_DLL}" to "${_TGT_DIR}"
                             COMMAND ${CMAKE_COMMAND} -E copy "${_DLL}" "${_TGT_DIR}"
                             VERBATIM)
        ENDFOREACH()
      ENDIF()

    ENDIF(WIN32)

    # Update version.xml from the collected user settings files (see SD_INSTALL_FILES).
    GET_PROPERTY(_XMLVER_ARGS TARGET settings_versions PROPERTY XMLVERSION_ARGS)
    #MESSAGE(STATUS "SD_UPDATE_SETTINGS_VERSION : XMLVERSION_ARGS=${_XMLVER_ARGS}")
    GET_FILENAME_COMPONENT(_XMLVER_DIR ${_XMLVER_EXE} PATH)
    GET_FILENAME_COMPONENT(_XMLVER_NAME ${_XMLVER_EXE} NAME)

    SET(_SRC_FILE)
    FOREACH(_ARG ${_XMLVER_ARGS})

      #MESSAGE(STATUS "${_ARG}")
      IF(NOT _SRC_FILE)

        SET(_SRC_FILE ${_ARG})

      ELSE()

        SET(_USER_DIR ${_ARG})

        # Register file for run-time install/update at game startup, thanks to xmlversion
        # (filesetup.cpp services will do the job at game startup).
        ADD_CUSTOM_COMMAND(TARGET settings_versions POST_BUILD
                           WORKING_DIRECTORY "${_XMLVER_DIR}"
                           COMMAND ${CMAKE_COMMAND} -E echo "${_XMLVER_EXE}" "${CMAKE_BINARY_DIR}/${SD_BINDIR}/version.xml" "${_SRC_FILE}" "${_USER_DIR}" "${PROJECT_SOURCE_DIR}/data"
                           COMMAND "${_XMLVER_NAME}" "${CMAKE_BINARY_DIR}/${SD_BINDIR}/version.xml" "${_SRC_FILE}" "${_USER_DIR}" "${PROJECT_SOURCE_DIR}/data"
                           VERBATIM)

        # Done for this {file, folder} couple, ready for next one.
        SET(_SRC_FILE)

      ENDIF()

    ENDFOREACH()

    # Install version.xml
    IF(MSVC)
      INSTALL(FILES "${CMAKE_BINARY_DIR}/${SD_BINDIR}/version.xml" DESTINATION "${SD_DATADIR}")
    ELSE()
      INSTALL(FILES "${CMAKE_BINARY_DIR}/${SD_BINDIR}/version.xml" DESTINATION "${SD_DATADIR_ABS}")
    ENDIF(MSVC)

ENDMACRO(SD_UPDATE_SETTINGS_VERSION)

# Data/Lib/Bin/Include files installation (with user settings registration for data files)
# Note: Missing files will be skipped if not there and OPTION_CHECK_CONTENTS is Off.
# Args:
#  DATA     : Data subdirectory where to install specified data files
#  LIB      : Lib subdirectory where to install specified files/targets
#  BIN      : If present, instructs to install specified files/targets in the bin dir
#  INCLUDE  : Include subdirectory where to install specified files
#  USER     : User settings subdirectory where to install/update specified data files at run-time
#  PREFIX   : Prefix to use to get source path for files specified in FILES
#  FILES    : Files to install (see PREFIX)
#  TARGETS  : Targets to install
# Examples:
#  SD_INSTALL_FILES(DATA drivers/bt FILES bt.xml logo.rgb)
#     Installs bt.xml and logo.rgb in ${prefix}/${SD_DATADIR}/drivers/bt
#  SD_INSTALL_FILES(DATA config/raceman USER config/raceman FILES quickrace.xml endrace.xml)
#     Installs quickrace.xml and endrace.xml in ${prefix}/${SD_DATADIR}/drivers/bt
#     and copies the file to the users settings folder ${SD_LOCALDIR}/config/raceman at startup.
#  SD_INSTALL_FILES(LIB drivers/bt TARGETS bt.so)
#     Installs bt.so in ${prefix}/${SD_LIBDIR}/drivers/bt
#  SD_INSTALL_FILES(BIN TARGETS speed-dreams)
#     Installs the speed-dreams target in ${prefix}/${SD_BINDIR}
#  SD_INSTALL_FILES(MAN man6 PREFIX ${SOURCE_DIR}/doc/man FILES sd2-menuview.6)
#     Installs ${SOURCE_DIR}/doc/man/sd2-menuview.6 in ${prefix}/${SD_MANDIR}/man6
MACRO(SD_INSTALL_FILES)

  SET(SD_INSTALL_FILES_SYNTAX "DATA,1,1,IS_DATA,DATA_PATH")
  SET(SD_INSTALL_FILES_SYNTAX ${SD_INSTALL_FILES_SYNTAX} "LIB,1,1,IS_LIB,LIB_PATH")
  SET(SD_INSTALL_FILES_SYNTAX ${SD_INSTALL_FILES_SYNTAX} "BIN,0,0,IS_BIN,_")
  SET(SD_INSTALL_FILES_SYNTAX ${SD_INSTALL_FILES_SYNTAX} "INCLUDE,0,1,IS_INCLUDE,INCLUDE_PATH")
  SET(SD_INSTALL_FILES_SYNTAX ${SD_INSTALL_FILES_SYNTAX} "MAN,1,1,IS_MAN,MAN_PATH")
  SET(SD_INSTALL_FILES_SYNTAX ${SD_INSTALL_FILES_SYNTAX} "USER,1,1,IS_USER,USER_PATH")
  SET(SD_INSTALL_FILES_SYNTAX ${SD_INSTALL_FILES_SYNTAX} "PREFIX,0,1,HAS_PREFIX,PREFIX")
  SET(SD_INSTALL_FILES_SYNTAX ${SD_INSTALL_FILES_SYNTAX} "FILES,0,-1,HAS_FILES,FILES")
  SET(SD_INSTALL_FILES_SYNTAX ${SD_INSTALL_FILES_SYNTAX} "TARGETS,0,-1,HAS_TARGETS,TARGETS")

  SPLIT_ARGN(${SD_INSTALL_FILES_SYNTAX} ARGUMENTS ${ARGN})

  #MESSAGE(STATUS "  SD_INSTALL_FILES: LIB=${IS_LIB}:${LIB_PATH} BIN=${IS_BIN} INCLUDE=${IS_INCLUDE}:${INCLUDE_PATH} DATA=${IS_DATA}:${DATA_PATH} MAN=${IS_MAN}:${MAN_PATH} USER=${IS_USER}:${USER_PATH} TARGETS=${HAS_TARGETS}:${TARGETS} FILES=${HAS_FILES}:${FILES}")

  # Fix/Check argument syntax / values
  IF(NOT DATA_PATH)
    SET(IS_DATA FALSE)
  ENDIF()
  IF(NOT USER_PATH)
    SET(IS_USER FALSE)
  ENDIF()
  IF(NOT LIB_PATH)
    SET(IS_LIB FALSE)
  ENDIF()
  IF(NOT MAN_PATH)
    SET(IS_MAN FALSE)
  ENDIF()
  IF(NOT PREFIX)
    SET(HAS_PREFIX FALSE)
  ENDIF()
  IF(NOT FILES)
    SET(HAS_FILES FALSE)
  ENDIF()
  IF(NOT TARGETS)
    SET(HAS_TARGETS FALSE)
  ENDIF()

  IF(IS_DATA OR IS_LIB OR IS_BIN OR IS_INCLUDE OR IS_MAN)
    IF(HAS_PREFIX)
      IF(NOT HAS_FILES)
        MESSAGE(FATAL_ERROR "SD_INSTALL_FILES: Expected FILES when PREFIX keyword is present")
      ENDIF()
    ENDIF()
  ELSE()
    MESSAGE(FATAL_ERROR "SD_INSTALL_FILES: Expected 1 and only 1 LIB, DATA, BIN, INCLUDE or MAN keyword")
  ENDIF()

  IF(IS_USER)
    IF(NOT IS_DATA)
      MESSAGE(FATAL_ERROR "SD_INSTALL_FILES: Expected DATA when USER keyword is present")
    ENDIF()
  ENDIF()

  # Compute destination sub-dir
  IF(IS_LIB)
    SET(DEST1 ${SD_LIBDIR})
    SET(DEST2 ${LIB_PATH})
  ELSEIF(IS_DATA)
    SET(DEST1 ${SD_DATADIR})
    SET(DEST2 ${DATA_PATH})
  ELSEIF(IS_BIN)
    SET(DEST1 ${SD_BINDIR})
    SET(DEST2 "")
  ELSEIF(IS_INCLUDE)
    SET(DEST1 ${SD_INCLUDEDIR})
    SET(DEST2 ${INCLUDE_PATH})
  ELSEIF(IS_MAN)
    SET(DEST1 ${SD_MANDIR})
    SET(DEST2 ${MAN_PATH})
  ENDIF()

  IF(DEST2 STREQUAL "" OR DEST2 STREQUAL "/")
    SET(DEST2 "")
    SET(DEST_ALL "${DEST1}")
  ELSE()
    SET(DEST_ALL "${DEST1}/${DEST2}")
  ENDIF()

  # Prepend prefix to files if specified.
  SET(REAL_FILES) # Reset the list (remember, it's a CMakeLists.txt global variable :-()
  IF(HAS_FILES)
    SET(_FILES) # Same.
    FOREACH(FILE ${FILES})
      #MESSAGE(STATUS "SD_INSTALL_FILES: ${FILE}")
      IF(HAS_PREFIX)
        SET(_FILE ${PREFIX}/${FILE})
      ELSE()
        SET(_FILE ${FILE})
      ENDIF()
      # Contents check for non-generated files if specified.
      IF(NOT IS_ABSOLUTE ${_FILE})
        SET(_FILE "${CMAKE_CURRENT_SOURCE_DIR}/${_FILE}")
      ENDIF()
      IF(IS_LIB OR IS_BIN OR EXISTS ${_FILE} OR OPTION_CHECK_CONTENTS)
        LIST(APPEND REAL_FILES ${_FILE})
        LIST(APPEND _FILES ${FILE})
      ELSE()
        IF(IS_ABSOLUTE ${_FILE}) # Make message less long : remove useless source dir path.
          STRING(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/" "" _FILE "${_FILE}")
        ENDIF()
        MESSAGE(STATUS "Note : Won't install missing file ${_FILE}")
      ENDIF()
    ENDFOREACH()
    SET(FILES ${_FILES})
  ENDIF()

  # Install files
  #MESSAGE(STATUS "REAL_FILES '${REAL_FILES}' DESTINATION '${DEST_ALL}'")
  IF(REAL_FILES)
    INSTALL(FILES ${REAL_FILES} DESTINATION ${DEST_ALL})
  ENDIF()

  # Install targets
  IF(HAS_TARGETS)

    INSTALL(TARGETS ${TARGETS} DESTINATION ${DEST_ALL})

  ENDIF()

  # Collect user settings files and target folder for later (see SD_XX_SETTINGS_VERSION above).
  IF(IS_USER)

    FOREACH(FILE ${FILES})
      IF("${FILE}" MATCHES "\\.xml")
        GET_FILENAME_COMPONENT(FILE_NAME ${FILE} NAME)
        SET_PROPERTY(TARGET settings_versions APPEND PROPERTY XMLVERSION_ARGS
                     "${DATA_PATH}/${FILE_NAME};${USER_PATH}/${FILE_NAME}")
      ENDIF()
    ENDFOREACH()

    # Debug traces.
    #GET_PROPERTY(_XMLVER_ARGS TARGET settings_versions PROPERTY XMLVERSION_ARGS)
    #MESSAGE(STATUS "SD_INSTALL_FILES(USER) : XMLVERSION_ARGS=${_XMLVER_ARGS}")

  ENDIF(IS_USER)

ENDMACRO(SD_INSTALL_FILES)

# Directory installation with pattern matching on files and user settings registration
# Note: Missing dirs will be skipped if not there and OPTION_CHECK_CONTENTS is Off.
# Args:
#  DATA        : Data subdirectory where to install specified sub-dirs
#  USER        : User settings subdirectory where to install/update specified sub-dirs at run-time
#  PREFIX      : Prefix to use to get source path for dirs specified in DIRECTORIES
#  DIRECTORIES : Sub-dirs to recursively install (see PREFIX)
#  PATTERNS    : Glob patterns to use for seelecting files to install (defaults to *.*)
# Example:
#  SD_INSTALL_DIRECTORIES(DATA drivers/human USER drivers/human 
#                         PREFIX pfx DIRECTORIES cars tracks PATTERNS *.xml)
#  will recursively install any .xml file from drivers/human/pfx/cars and drivers/human/pfx/tracks
#  into drivers/human/cars and  drivers/human/pfx/tracks data dirs ; 
#  these files / sub-dirs will also be scheduled for run-time update/install in user settings dir.
MACRO(SD_INSTALL_DIRECTORIES)

  SET(SDID_SYNTAX "DATA,1,1,IS_DATA,DATA_PATH")
  SET(SDID_SYNTAX ${SDID_SYNTAX} "USER,1,1,IS_USER,USER_PATH")
  SET(SDID_SYNTAX ${SDID_SYNTAX} "PREFIX,0,1,HAS_PREFIX,PREFIX")
  SET(SDID_SYNTAX ${SDID_SYNTAX} "DIRECTORIES,0,-1,HAS_DIRECTORIES,DIRECTORIES")
  SET(SDID_SYNTAX ${SDID_SYNTAX} "PATTERNS,0,-1,HAS_PATTERNS,PATTERNS")

  SPLIT_ARGN(${SDID_SYNTAX} ARGUMENTS ${ARGN})

  #MESSAGE(STATUS "  SD_INSTALL_DIRECTORIES: DATA=${IS_DATA}:${DATA_PATH} USER=${IS_USER}:${USER_PATH} DIRS=${HAS_DIRECTORIES}:${DIRECTORIES} PATTERNS=${HAS_PATTERNS}:${PATTERNS}")

  # Fix/Check argument syntax / values
  IF(NOT DATA_PATH)
    SET(IS_DATA FALSE)
  ENDIF()
  IF(NOT USER_PATH)
    SET(IS_USER FALSE)
  ENDIF()
  IF(NOT PREFIX)
    SET(HAS_PREFIX FALSE)
  ENDIF()
  IF(NOT DIRECTORIES)
    SET(HAS_DIRECTORIES FALSE)
  ENDIF()
  IF(NOT PATTERNS)
    SET(HAS_PATTERNS TRUE)
    SET(PATTERNS "*.*")
  ENDIF()

  IF(IS_DATA AND HAS_DIRECTORIES)
    IF(HAS_PREFIX)
      SET(PREFIX "${PREFIX}/")
      SET(POSTFIX "/${PREFIX}")
    ELSE()
      SET(PREFIX "")
      SET(POSTFIX "")
    ENDIF()
  ELSE()
    MESSAGE(ERROR "SD_INSTALL_DIRECTORIES: Expected mandatory DATA and DIRECTORIES keywords")
  ENDIF()

  # Compute destination sub-dir
  IF(DATA_PATH STREQUAL "/")
    SET(DEST_ALL "${SD_DATADIR}")
  ELSE()
    SET(DEST_ALL "${SD_DATADIR}/${DATA_PATH}")
  ENDIF()

  # Check / filter contents if specified
  SET(_DIRECTORIES)
  FOREACH(DIRECTORY ${DIRECTORIES})
    #MESSAGE(STATUS "SD_INSTALL_DIRS: ${DIRECTORY}")
    IF(NOT IS_ABSOLUTE ${DIRECTORY})
      SET(_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${PREFIX}${DIRECTORY}")
    ELSE()
      SET(_DIRECTORY "${DIRECTORY}")
    ENDIF()
    IF(EXISTS ${_DIRECTORY} OR OPTION_CHECK_CONTENTS)
      LIST(APPEND _DIRECTORIES ${DIRECTORY})
    ELSE()
      MESSAGE(STATUS "Note : Won't install missing dir. ${PREFIX}${DIRECTORY}")
    ENDIF()
  ENDFOREACH()
  SET(DIRECTORIES ${_DIRECTORIES})
  
  # Install selected files into the data dir.
  # And collect user settings files (if any) and target folder for later
  # (see SD_XX_SETTINGS_VERSION above).
  FOREACH(DIRECTORY ${DIRECTORIES})
    SET(FILES)
    FOREACH(PATTERN ${PATTERNS})
      FILE(GLOB_RECURSE _FILES RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}${POSTFIX}" "${PREFIX}${DIRECTORY}/${PATTERN}")
      LIST(APPEND FILES ${_FILES})
    ENDFOREACH()
    #MESSAGE(STATUS "SD_INSTALL_DIRS: FILES=${FILES}")
    FOREACH(FILE ${FILES})
      IF(NOT "${FILE}" MATCHES "\\.svn")
        GET_FILENAME_COMPONENT(SUBDIR ${FILE} PATH)
        INSTALL(FILES ${PREFIX}${FILE} DESTINATION ${DEST_ALL}/${SUBDIR})
        IF(IS_USER AND "${FILE}" MATCHES "\\.xml")
          SET_PROPERTY(TARGET settings_versions APPEND PROPERTY XMLVERSION_ARGS
                       "${DATA_PATH}/${FILE};${USER_PATH}/${FILE}")
        ENDIF()
      ENDIF()
    ENDFOREACH()
  ENDFOREACH()

ENDMACRO(SD_INSTALL_DIRECTORIES)

# Macro to install CMake config files for SD if in-source build.
IF(IN_SOURCETREE)
  MACRO(INSTALL_SD_CMAKE)
    INSTALL(CODE 
        "SET(CUR_DESTDIR \"\$ENV{DESTDIR}\")
         IF(CUR_DESTDIR MATCHES \"[^/]\")
           STRING(REGEX REPLACE \"^(.*[^/])/*$\" \"\\\\1\" CUR_DESTDIR_CORR \"\${CUR_DESTDIR}\")
         ELSE(CUR_DESTDIR MATCHES \"[^/]\")
           SET(CUR_DESTDIR_CORR \"\")
           ENDIF(CUR_DESTDIR MATCHES \"[^/]\")
         FILE(MAKE_DIRECTORY \"\${CUR_DESTDIR_CORR}${SD_DATADIR_ABS}/cmake\")
         FILE(WRITE \"\${CUR_DESTDIR_CORR}${SD_DATADIR_ABS}/cmake/speed-dreams.cmake\"
            \"SET(SD_DATADIR_ABS \\\"${SD_DATADIR_ABS}\\\")
              SET(SD_LOCALDIR \\\"${SD_LOCALDIR}\\\")
            SET(SD_LIBDIR_ABS \\\"${SD_LIBDIR_ABS}\\\")
            SET(SD_BINDIR_ABS \\\"${SD_BINDIR_ABS}\\\")
            SET(SD_INCLUDEDIR_ABS \\\"${SD_INCLUDEDIR_ABS}\\\")
               SET(IN_SOURCETREE FALSE)\\n\\n\")
         FILE(READ \"${SOURCE_DIR}/cmake/macros.cmake\" SD_MACRO_CONTENT)
         FILE(APPEND \"\${CUR_DESTDIR_CORR}${SD_DATADIR_ABS}/cmake/speed-dreams.cmake\" \${SD_MACRO_CONTENT})")
  ENDMACRO(INSTALL_SD_CMAKE)
ENDIF(IN_SOURCETREE)

MACRO(SD_INSTALL_CAR CARNAME)

  SET(SDIC_FILES ${CARNAME}.xml ${ARGN})

  FILE(GLOB AC3D_FILES *.acc)
  FILE(GLOB SRCAC_FILES *-src.ac)
  FILE(GLOB AC_FILES *.ac)
  FOREACH(AC_FILE ${AC_FILES})
    LIST(FIND SRCAC_FILES "${AC_FILE}" IS_SRC)
    LIST(FIND ACC_FILES "${AC_FILE}c" HAS_ACC)
    IF(IS_SRC EQUAL -1 AND HAS_ACC EQUAL -1)
      LIST(APPEND AC3D_FILES ${AC_FILE})
    ENDIF()
  ENDFOREACH()
  SET(AC3D_FILES ${AC3D_FILES} ${ACC_FILES})

  FILE(GLOB OSGT_FILES *.osg)
  FILE(GLOB PNG_FILES *.png)
  FILE(GLOB DDS_FILES *.dds)
  FILE(GLOB JPG_FILES *.jpg)
  FILE(GLOB RGB_FILES *.rgb)
  FILE(GLOB WAV_FILES *.wav)

  SET(SDIC_FILES ${SDIC_FILES} ${AC3D_FILES} ${OSGT_FILE} 
  	  ${RGB_FILES} ${PNG_FILES} ${DDS_FILES} ${JPG_FILES} ${WAV_FILES})

  SD_INSTALL_FILES(DATA cars/models/${CARNAME} FILES ${SDIC_FILES})

ENDMACRO(SD_INSTALL_CAR CARNAME)

MACRO(SD_INSTALL_TRACK TRACKNAME CATEGORY)

  SET(SDIT_FILES ${TRACKNAME}.xml ${ARGN})

  # Among AC3D files, keep any .acc one, but exclude *-src.ac
  # and *.ac when a .acc with same name exists.
  FILE(GLOB ACC_FILES *.acc)
  FILE(GLOB OSGT_FILES *.osg)
  FILE(GLOB SRCAC_FILES *-src.ac)
  FILE(GLOB AC_FILES *.ac)
  FOREACH(AC_FILE ${AC_FILES})
    LIST(FIND SRCAC_FILES "${AC_FILE}" IS_SRC)
    LIST(FIND ACC_FILES "${AC_FILE}c" HAS_ACC)
    IF(IS_SRC EQUAL -1 AND HAS_ACC EQUAL -1)
      LIST(APPEND AC3D_FILES ${AC_FILE})
    ENDIF()
  ENDFOREACH()
  SET(AC3D_FILES ${AC3D_FILES} ${ACC_FILES})

  FILE(GLOB PNG_FILES *.png)
  FILE(GLOB DDS_FILES *.dds)
  FILE(GLOB JPG_FILES *.jpg)
  FILE(GLOB RGB_FILES *.rgb)

  SET(SDIT_FILES ${SDIT_FILES} ${AC3D_FILES} ${OSGT_FILES} 
  	${RGB_FILES} ${PNG_FILES} ${JPG_FILES} ${DDS_FILES})

  SD_INSTALL_FILES(DATA tracks/${CATEGORY}/${TRACKNAME} FILES ${SDIT_FILES})

ENDMACRO(SD_INSTALL_TRACK TRACKNAME CATEGORY)
