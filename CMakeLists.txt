CMAKE_MINIMUM_REQUIRED(VERSION 2.6.0 FATAL_ERROR)

# Speed Dreams project
PROJECT(speed-dreams-2)

INCLUDE(cmake/macros.cmake)
INCLUDE(cmake/checks.cmake)

CHECK_HEADERS_H()
CHECK_FUNCTIONS()
CHECK_LIBRARIES()

# Determine version
SET(NEXT_LESSER_VERSION 0) # The next lesser version
SET(NEXT_MAJOR_MINOR_VERSION "2.2") # Next major.minor version

# Uncomment if you don't want automatic SVN version detection
# (poor performances under Windows, don't work with git-svn, ...)
# SET(OPTION_AUTOVERSION Off)

IF(NOT OPTION_AUTOVERSION)
	
	SET(VERSION "${NEXT_MAJOR_MINOR_VERSION}-dev")
	SET(VERSION_LONG "${VERSION}")
	
	MESSAGE(STATUS "Version '${VERSION_LONG}' (not auto-detected from SVN source tree)")

ELSE(NOT OPTION_AUTOVERSION)

	# Use SVN HeadURL property on this CMakeLists.txt file in order to automatically get the current version
	SET(HEAD_URL "$HeadURL: svn://svn.code.sf.net/p/speed-dreams/code/tags/2.2.1/CMakeLists.txt $")

	# Define regular expressions
	SET(REGEX_TAG        "^\\$HeadURL:.*speed-dreams/code/tags/([^/\\$ ]*).*\\$$")
	SET(REGEX_BUG_BRANCH "^\\$HeadURL:.*speed-dreams/code/branches/([0-9]+\\.[0-9]+)(\\.[xX])?/.*\\$$")
	SET(REGEX_TRUNK      "^\\$HeadURL:.*speed-dreams/code/trunk/.*\\$$")
	SET(REGEX_BRANCH     "^\\$HeadURL:.*speed-dreams/code/branches/([^/\\$ ]*).*\\$$")

	# Check regular expressions
	STRING(REGEX MATCH "${REGEX_TAG}"        VERSION_IS_TAG "${HEAD_URL}")
	STRING(REGEX MATCH "${REGEX_BUG_BRANCH}" VERSION_IS_BUG_BRANCH "${HEAD_URL}")
	STRING(REGEX MATCH "${REGEX_TRUNK}"      VERSION_IS_TRUNK "${HEAD_URL}")
	STRING(REGEX MATCH "${REGEX_BRANCH}"     VERSION_IS_BRANCH "${HEAD_URL}")

	# See if we need to look at the current SVN revision
	SET(NEED_SVN_REV TRUE)
	IF(VERSION_IS_TAG)
		STRING(REGEX REPLACE "${REGEX_TAG}" "\\1" VERSION "${HEAD_URL}")
	ELSEIF(VERSION_IS_BUG_BRANCH)
		STRING(REGEX REPLACE "${REGEX_BUG_BRANCH}" "\\1" TMP_VERSION "${HEAD_URL}")
		SET(VERSION "${TMP_VERSION}.${NEXT_LESSER_VERSION}-dev")
	ELSEIF(VERSION_IS_TRUNK)
		SET(VERSION "${NEXT_MAJOR_MINOR_VERSION}-trunk")
	ELSEIF(VERSION_IS_BRANCH)
		STRING(REGEX REPLACE "${REGEX_BRANCH}" "\\1" VERSION "${HEAD_URL}")
	ELSE(VERSION_IS_TAG)
		MESSAGE("Warning: Could not determine version name from HeadURL ${HEAD_URL}")
		SET(VERSION "${NEXT_MAJOR_MINOR_VERSION}-dev")
		SET(NEED_SVN_REV FALSE)
	ENDIF(VERSION_IS_TAG)
	
	# Try to get current SVN revision
	IF(NEED_SVN_REV)
		SET(SVN_FIND_REV_FAILED FALSE)
		FIND_PROGRAM(SVNVERSION_EXEC svnversion PATHS /usr/local/bin /usr/bin)
		MARK_AS_ADVANCED(SVNVERSION_EXEC)
		IF(NOT SVNVERSION_EXEC)
			SET(SVN_FIND_REV_FAILED TRUE)
		ENDIF(NOT SVNVERSION_EXEC)
		IF(NOT SVN_FIND_REV_FAILED)
			MESSAGE(STATUS "Looking for current subversion revision (may be long) ...")
			EXECUTE_PROCESS(COMMAND ${SVNVERSION_EXEC} ${CMAKE_CURRENT_SOURCE_DIR} --no-newline
							RESULT_VARIABLE SVNVERSION_RESULT
							OUTPUT_VARIABLE SVN_REV_LONG)
			IF(NOT ${SVNVERSION_RESULT} EQUAL 0)
				SET(SVN_FIND_REV_FAILED TRUE)
			ENDIF(NOT ${SVNVERSION_RESULT} EQUAL 0)
		ENDIF(NOT SVN_FIND_REV_FAILED)
		IF(NOT SVN_FIND_REV_FAILED)
			IF("${SVN_REV_LONG}" MATCHES ":")
				STRING(REGEX REPLACE "^[^:]*:(.*)$" "\\1" SVN_REV "${SVN_REV_LONG}")
			ELSE("${SVN_REV_LONG}" MATCHES ":")
				SET(SVN_REV "${SVN_REV_LONG}")
			ENDIF("${SVN_REV_LONG}" MATCHES ":")
			IF(NOT SVN_REV)
				SET(SVN_FIND_REV_FAILED TRUE)
			ENDIF(NOT SVN_REV)
		ENDIF(NOT SVN_FIND_REV_FAILED)
		SET(VERSION_LONG "${VERSION}")
		IF(NOT SVN_FIND_REV_FAILED)
			STRING(TOLOWER ${SVN_REV} SVN_REV)
			SET(VERSION_LONG "${VERSION_LONG}-r${SVN_REV}")
		ENDIF(NOT SVN_FIND_REV_FAILED)
	ELSE(NEED_SVN_REV)
		SET(VERSION_LONG "${VERSION}")
	ENDIF(NEED_SVN_REV)

	MESSAGE(STATUS "Version '${VERSION_LONG}'")

ENDIF(NOT OPTION_AUTOVERSION)

# Generate config.h
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/cmake/config.h.in.cmake
               ${CMAKE_CURRENT_BINARY_DIR}/config.h)

# Generate DOxygen configuration file
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/cmake/doxygenconfig.in.cmake
               ${CMAKE_CURRENT_BINARY_DIR}/doxygen_config @ONLY)


# Fictive = no-output target, for having source files available in IDEs.
SET(_SOURCES CHANGES.txt COPYING.txt INSTALL.txt README.txt)
ADD_CUSTOM_TARGET(rootdoc SOURCES ${_SOURCES} data/data/credits.xml)

# Install top tree data files
SD_INSTALL_FILES(DATA "/"  FILES ${_SOURCES})

# Initialize user settings file management (before any of them is collected).
SD_SETUP_SETTINGS_VERSION()

# Sub-dirs to build ...					   
ADD_SUBDIRECTORY(src)
ADD_SUBDIRECTORY(data)
ADD_SUBDIRECTORY(cmake)

# After the user settings files have been collected,
# Update (and then install) version.xml from their respective version.
SD_UPDATE_SETTINGS_VERSION()

# Usefull for using an installation of SD as a robot development framework without building it.
INSTALL_SD_CMAKE()

# Generate a 'make clobber'-like clobber.sh/.bat script in case of an in-source build.
IF("${PROJECT_SOURCE_DIR}" STREQUAL "${PROJECT_BINARY_DIR}")
	SD_GENERATE_CLOBBER_SCRIPT()
ENDIF("${PROJECT_SOURCE_DIR}" STREQUAL "${PROJECT_BINARY_DIR}")

# Add an unistall target.
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/cmake/uninstall.cmake" 
               "${CMAKE_CURRENT_BINARY_DIR}/uninstall.cmake" IMMEDIATE @ONLY)
IF(MSVC)
	SET(_UNINST_TGT_NAME "UNINSTALL")
ELSE(MSVC)
	SET(_UNINST_TGT_NAME "uninstall")
ENDIF(MSVC)
ADD_CUSTOM_TARGET(${_UNINST_TGT_NAME} "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/uninstall.cmake" )

# Configure CPack.
INCLUDE(cmake/packagemaker.cmake)

