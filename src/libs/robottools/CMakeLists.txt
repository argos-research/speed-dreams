INCLUDE(../../../cmake/macros.cmake)

#PROJECT(robottools)

ADD_INTERFACE_INCLUDEDIR()
ADD_SDLIB_INCLUDEDIR(portability tgf math tgfclient)
ADD_SDL_INCLUDEDIR()
ADD_PLIB_INCLUDEDIR()

SET(ROBOTTOOLS_SOURCES rtteammanager.cpp rttelem.cpp rttimeanalysis.cpp  rttrack.cpp rtutil.cpp rthumandriver.cpp)
SET(ROBOTTOOLS_HEADERS robottools.h teammanager.h timeanalysis.h humandriver.h)

IF(WIN32)
	# DLL export stuff under Windows (to avoid .def file)
	ADD_DEFINITIONS(-DROBOTTOOLS_DLL)
ENDIF(WIN32)
    
IF(MSVC)
	# Ignore some run-time libs to avoid link time warnings and sometimes even crashes.
    SET(CMAKE_SHARED_LINKER_FLAGS_DEBUG "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} /NODEFAULTLIB:msvcrt.lib")
ENDIF(MSVC)

# Disable developer warning
IF (COMMAND CMAKE_POLICY)
	CMAKE_POLICY(SET CMP0003 NEW)
ENDIF(COMMAND CMAKE_POLICY)

# Note: Headers needed for having them available in IDEs.
SD_ADD_LIBRARY(robottools SHARED ${ROBOTTOOLS_SOURCES} ${ROBOTTOOLS_HEADERS})

# Might not work with GCC 4.5 or + (non-robot modules crash at 1st reload = after 1 dlclose) 
#SET_TARGET_PROPERTIES(robottools PROPERTIES VERSION ${VERSION} SOVERSION 0.0.0)

ADD_PLIB_LIBRARY(robottools sg ul)

ADD_SDLIB_LIBRARY(robottools portability tgf tgfclient)

IF(WIN32)
	SD_INSTALL_FILES(BIN TARGETS robottools)
ELSE(WIN32)
	SD_INSTALL_FILES(LIB lib TARGETS robottools)
ENDIF(WIN32)

SD_INSTALL_FILES(INCLUDE FILES robottools.h teammanager.h)

