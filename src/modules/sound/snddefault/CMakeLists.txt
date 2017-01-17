INCLUDE(../../../../cmake/macros.cmake)

SET(_SOURCES snddefault.cpp 
                       CarSoundData.cpp
                       Sound.cpp OpenalSound.cpp
                       SoundInterface.cpp
                       OpenalSoundInterface.cpp 
                       grsound.cpp)
SET(_HEADERS snddefault.h 
                       CarSoundData.h 
                       Sound.h OpenalSound.h
                       SoundInterface.h
                       OpenalSoundInterface.h 
                       grsound.h)
IF(NOT OPTION_USE_MACPORTS)
   SET(_SOURCES ${_SOURCES} PlibSound.cpp PlibSoundInterface.cpp)
   SET(_HEADERS ${_HEADERS} PlibSound.h PlibSoundInterface.h)
ENDIF(NOT OPTION_USE_MACPORTS)

SET(_OTHER_SOURCES ) 

ADD_INTERFACE_INCLUDEDIR()
ADD_SDLIB_INCLUDEDIR(portability math tgf tgfclient robottools)
ADD_PLIB_INCLUDEDIR()

IF(OPTION_SDL2)
	ADD_SDL2_INCLUDEDIR()
ELSE(OPTION_SDL2)
	ADD_SDL_INCLUDEDIR()
ENDIF(OPTION_SDL2)

ADD_OPENAL_INCLUDEDIR()

IF(COMMAND CMAKE_POLICY)
    CMAKE_POLICY(SET CMP0003 NEW)
ENDIF(COMMAND CMAKE_POLICY)

IF(WIN32)
    # DLL export stuff under Windows (to avoid .def file)
    ADD_DEFINITIONS(-DSNDDEFAULT_DLL)
ENDIF(WIN32)

IF(MSVC)
    # Ignore some run-time libs to avoid link time warnings and sometimes even crashes.
    SET(CMAKE_MODULE_LINKER_FLAGS_DEBUG 
        "${CMAKE_MODULE_LINKER_FLAGS_DEBUG} /NODEFAULTLIB:msvcrt.lib /NODEFAULTLIB:libcmtd.lib")
    SET(CMAKE_MODULE_LINKER_FLAGS_RELEASE 
        "${CMAKE_MODULE_LINKER_FLAGS_RELEASE} /NODEFAULTLIB:libcmt.lib")
ENDIF(MSVC)

# Note: Headers and other source files needed for having them available in IDEs.
SD_ADD_LIBRARY(snddefault MODULE ${_SOURCES} ${_HEADERS} ${_OTHER_SOURCES})

# Might not work with GCC 4.5 or + (non-robot modules crash at 1st reload = after 1 dlclose) 
#SET_TARGET_PROPERTIES(snddefault PROPERTIES VERSION ${VERSION} SOVERSION 0.0.0)

IF(OPTION_USE_MACPORTS)
   ADD_PLIB_LIBRARY(snddefault ul sg ssg ssgaux)
ELSE(OPTION_USE_MACPORTS)
   ADD_PLIB_LIBRARY(snddefault ul sg sl ssg ssgaux)
ENDIF(OPTION_USE_MACPORTS)

IF(OPTION_SDL2)
	ADD_SDL2_LIBRARY(snddefault)
ELSE(OPTION_SDL2)
	ADD_SDL_LIBRARY(snddefault)
ENDIF(OPTION_SDL2)

ADD_SDLIB_LIBRARY(snddefault portability tgf tgfclient robottools)

IF(OPENAL_FOUND)
    TARGET_LINK_LIBRARIES(snddefault ${OPENAL_LIBRARY})
ENDIF(OPENAL_FOUND)

IF(OPENGL_FOUND)
    TARGET_LINK_LIBRARIES(snddefault ${OPENGL_LIBRARY})
ENDIF(OPENGL_FOUND)

SD_INSTALL_FILES(LIB modules/sound TARGETS snddefault)
