INCLUDE(../../../cmake/macros.cmake)

#PROJECT(tgf)

ADD_SDL_INCLUDEDIR()

ADD_EXPAT_INCLUDEDIR()

ADD_SDLIB_INCLUDEDIR(txml portability)

SET(TGF_HEADERS os.h tgf.h tgf.hpp modinfo.h osspec.h)
SET(TGF_SOURCES application.cpp eventloop.cpp directory.cpp file.cpp filesetup.cpp 
                formula.cpp hash.cpp module.cpp legacymodule.cpp modinfo.cpp os.cpp
                params.cpp profiler.cpp schedulespy.cpp tgf.cpp trace.cpp)
SET(TGF_OTHER_SOURCES params.dtd)

IF(WIN32)
    SET(TGF_HEADERS ${TGF_HEADERS} windowsspec.h)
    SET(TGF_SOURCES ${TGF_SOURCES} windowsspec.cpp)
ELSE(WIN32)
    SET(TGF_HEADERS ${TGF_HEADERS} linuxspec.h)
    SET(TGF_SOURCES ${TGF_SOURCES} linuxspec.cpp)
ENDIF(WIN32)

# Disable developer warning
IF(COMMAND CMAKE_POLICY)
    CMAKE_POLICY(SET CMP0003 NEW)
ENDIF(COMMAND CMAKE_POLICY)

IF(WIN32)
    # DLL export stuff under Windows (to avoid .def file)
    ADD_DEFINITIONS(-DTGF_DLL)
ENDIF(WIN32)

# Note: Other sources needed for having them available in IDEs.
SD_ADD_LIBRARY(tgf SHARED ${TGF_HEADERS} ${TGF_SOURCES} ${TGF_OTHER_SOURCES})

# Might not work with GCC 4.5 or + (non-robot modules crash at 1st reload = after 1 dlclose) 
#SET_TARGET_PROPERTIES(tgf PROPERTIES VERSION ${VERSION} SOVERSION 0.0.0)

IF(CMAKE_SKIP_RPATH OR CMAKE_SKIP_BUILD_RPATH)
    SD_ADD_LIBRARY(tgf_static STATIC ${TGF_HEADERS} ${TGF_SOURCES})
ENDIF(CMAKE_SKIP_RPATH OR CMAKE_SKIP_BUILD_RPATH)

ADD_SDLIB_LIBRARY(tgf portability txml) # txml ignored if OPTION_3RDPARTY_EXPAT

ADD_DL_LIBRARY(tgf)
ADD_SDL_LIBRARY(tgf)
ADD_EXPAT_LIBRARY(tgf) # Ignored if not OPTION_3RDPARTY_EXPAT

IF(WIN32)
    SD_INSTALL_FILES(BIN TARGETS tgf)
ELSE(WIN32)
    SD_INSTALL_FILES(LIB lib TARGETS tgf)
ENDIF(WIN32)

SD_INSTALL_FILES(INCLUDE FILES ${TGF_HEADERS})

SD_INSTALL_FILES(DATA config FILES params.dtd)
