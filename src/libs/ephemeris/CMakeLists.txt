INCLUDE(../../../cmake/macros.cmake)

#PROJECT(ephemeris)

SET(EPHEMERIS_SOURCES   celestialbody.cpp ephemeris.cpp jupiter.cpp  mars.cpp 
			mercury.cpp  moonpos.cpp neptune.cpp  saturn.cpp 
			star.cpp  stardata.cpp uranus.cpp venus.cpp)
SET(EPHEMERIS_HEADERS	celestialbody.h ephemeris.h jupiter.h mars.h mercury.h 
			moonpos.h neptune.h pluto.h saturn.h star.h stardata.h 
			uranus.h venus.h)
                       
ADD_SDLIB_INCLUDEDIR(portability math)

ADD_PLIB_INCLUDEDIR()
ADD_SDL_INCLUDEDIR()

IF(WIN32)
    # DLL export stuff under Windows (to avoid .def file)
    ADD_DEFINITIONS(-DEPHEMERIS_DLL)
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
SD_ADD_LIBRARY(ephemeris SHARED ${EPHEMERIS_SOURCES} ${EPHEMERIS_HEADERS})

# Might not work with GCC 4.5 or + (non-robot modules crash at 1st reload = after 1 dlclose) 
#SET_TARGET_PROPERTIES(ephemeris PROPERTIES VERSION ${VERSION} SOVERSION 0.0.0)

ADD_PLIB_LIBRARY(ephemeris ul sg)

ADD_SDLIB_LIBRARY(ephemeris portability)

IF(WIN32)
    SD_INSTALL_FILES(BIN TARGETS ephemeris)
ELSE(WIN32)
    SD_INSTALL_FILES(LIB lib TARGETS ephemeris)
ENDIF(WIN32)

SD_INSTALL_FILES(INCLUDE FILES ${EPHEMERIS_HEADERS})

