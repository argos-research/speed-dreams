INCLUDE(../../../../cmake/macros.cmake)

#PROJECT(genparoptv1)

SET(_SOURCES genparoptv1.cpp
             raceupdate.cpp racecars.cpp
             raceinit.cpp racemain.cpp
             racetrack.cpp raceresults.cpp 
             racestate.cpp racesituation.cpp racemessage.cpp
             raceutil.cpp genetic.cpp)
SET(_HEADERS genparoptv1.h 
             raceupdate.h racecars.h
             raceinit.h 
             racetrack.h raceresults.h
             racestate.h racesituation.h racemessage.h
             raceutil.h genetic.h)
SET(_OTHER_SOURCES )

ADD_INTERFACE_INCLUDEDIR()

ADD_SDLIB_INCLUDEDIR(math portability robottools tgf tgfdata)

ADD_PLIB_INCLUDEDIR()
ADD_SDL_INCLUDEDIR()

# Disable developer warning
IF (COMMAND CMAKE_POLICY)
    CMAKE_POLICY(SET CMP0003 NEW)
ENDIF(COMMAND CMAKE_POLICY)

IF(WIN32)
    # DLL export stuff under Windows (to avoid .def file)
    ADD_DEFINITIONS(-DGENPAROPTV1_DLL)
ENDIF(WIN32)
    
IF(MSVC)
    # Ignore some run-time libs to avoid link time warnings and sometimes even crashes.
    SET(CMAKE_MODULE_LINKER_FLAGS_DEBUG "${CMAKE_MODULE_LINKER_FLAGS_DEBUG} /NODEFAULTLIB:msvcrt.lib")
ENDIF(MSVC)

# Note: Other source files  and headers needed for having them available in IDEs.
SD_ADD_LIBRARY(genparoptv1 MODULE ${_SOURCES} ${_HEADERS} ${_OTHER_SOURCES})

# Might not work with GCC 4.5 or + (non-robot modules crash at 1st reload = after 1 dlclose) 
#SET_TARGET_PROPERTIES(genparoptv1 PROPERTIES VERSION ${VERSION} SOVERSION 0.0.0)

ADD_SDLIB_LIBRARY(genparoptv1 portability tgf tgfdata robottools)

SD_INSTALL_FILES(LIB modules/racing TARGETS genparoptv1)
