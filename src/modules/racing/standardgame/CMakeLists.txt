INCLUDE(../../../../cmake/macros.cmake)

#PROJECT(standardgame)

SET(_SOURCES standardgame.cpp
             racecareer.cpp raceupdate.cpp racenetwork.cpp racecars.cpp
             raceinit.cpp racemain.cpp
             racetrack.cpp raceresults.cpp racesimusimu.cpp 
             racestate.cpp racesituation.cpp racemessage.cpp
             raceutil.cpp)
SET(_HEADERS standardgame.h 
             racecareer.h raceupdate.h racenetwork.h racecars.h
             raceinit.h 
             racetrack.h raceresults.h racesimusimu.h 
             racestate.h racesituation.h racemessage.h
             raceutil.h)
SET(_OTHER_SOURCES )

ADD_INTERFACE_INCLUDEDIR()

ADD_SDLIB_INCLUDEDIR(math portability robottools tgf tgfdata networking)

ADD_PLIB_INCLUDEDIR()
ADD_SDL_INCLUDEDIR()

# Disable developer warning
IF (COMMAND CMAKE_POLICY)
    CMAKE_POLICY(SET CMP0003 NEW)
ENDIF(COMMAND CMAKE_POLICY)

IF(WIN32)
    # DLL export stuff under Windows (to avoid .def file)
    ADD_DEFINITIONS(-DSTANDARDGAME_DLL)
ENDIF(WIN32)
    
IF(MSVC)
    # Ignore some run-time libs to avoid link time warnings and sometimes even crashes.
    SET(CMAKE_MODULE_LINKER_FLAGS_DEBUG "${CMAKE_MODULE_LINKER_FLAGS_DEBUG} /NODEFAULTLIB:msvcrt.lib")
ENDIF(MSVC)

# Note: Other source files  and headers needed for having them available in IDEs.
SD_ADD_LIBRARY(standardgame MODULE ${_SOURCES} ${_HEADERS} ${_OTHER_SOURCES})

# Might not work with GCC 4.5 or + (non-robot modules crash at 1st reload = after 1 dlclose) 
#SET_TARGET_PROPERTIES(standardgame PROPERTIES VERSION ${VERSION} SOVERSION 0.0.0)

ADD_SDLIB_LIBRARY(standardgame portability tgf tgfdata robottools networking)

SD_INSTALL_FILES(LIB modules/racing TARGETS standardgame)
