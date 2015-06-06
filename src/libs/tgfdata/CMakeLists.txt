INCLUDE(../../../cmake/macros.cmake)

#PROJECT(tgfdata)

ADD_PLIB_INCLUDEDIR()

ADD_INTERFACE_INCLUDEDIR()
ADD_SDLIB_INCLUDEDIR(portability txml math tgf)

SET(TGFDATA_HEADERS tgfdata.h cars.h tracks.h racemanagers.h race.h drivers.h)
SET(TGFDATA_SOURCES tgfdata.cpp cars.cpp tracks.cpp racemanagers.cpp race.cpp drivers.cpp)

IF(WIN32)
    # DLL export stuff under Windows (to avoid .def file)
    ADD_DEFINITIONS(-DTGFDATA_DLL)
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
SD_ADD_LIBRARY(tgfdata SHARED ${TGFDATA_SOURCES} ${TGFDATA_HEADERS})

# Might not work with GCC 4.5 or + (non-robot modules crash at 1st reload = after 1 dlclose) 
#SET_TARGET_PROPERTIES(tgfdata PROPERTIES VERSION ${VERSION} SOVERSION 0.0.0)

IF(CMAKE_SKIP_RPATH OR CMAKE_SKIP_BUILD_RPATH)
    SD_ADD_LIBRARY(tgfdata_static STATIC ${TGFDATA_SOURCES})
ENDIF(CMAKE_SKIP_RPATH OR CMAKE_SKIP_BUILD_RPATH)

ADD_SDLIB_LIBRARY(tgfdata portability txml tgf) # txml ignored if OPTION_3RDPARTY_EXPAT

ADD_PLIB_LIBRARY(tgfdata sg ul)

IF(WIN32)
    SD_INSTALL_FILES(BIN TARGETS tgfdata)
ELSE(WIN32)
    SD_INSTALL_FILES(LIB lib TARGETS tgfdata)
ENDIF(WIN32)

SD_INSTALL_FILES(INCLUDE FILES ${TGFDATA_HEADERS})

