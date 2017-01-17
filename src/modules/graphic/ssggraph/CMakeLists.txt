INCLUDE(../../../../cmake/macros.cmake)

SET(SSGGRAPH_HEADERS grMoonpos.h grSky.h
                     grSphere.h grStars.cpp
                     grboard.h grcam.h grcar.h
                     grcarlight.h grmain.h 
                     grsimplestate.h 
                     grmultitexstate.h 
                     grloadac.h grssgext.h
                     grscene.h grbackground.h 
                     grscreen.h 
                     grshadow.h grskidmarks.h
                     grsmoke.h grtexture.h loadsgi.h
                     grtracklight.h 
                     grtrackmap.h grutil.h
                     grvtxtable.h grrain.h
                     ssggraph.h)
SET(SSGGRAPH_SOURCES grMoonpos.cpp
                     grMoon.cpp grSun.cpp grCloudLayer.cpp grSky.cpp
                     grSkyDome.cpp grSphere.cpp grStars.cpp
                     grboard.cpp grcam.cpp grcar.cpp
                     grcarlight.cpp grmain.cpp 
                     grsimplestate.cpp 
                     grmultitexstate.cpp 
                     grloadac.cpp
                     grscene.cpp grbackground.cpp 
                     grscreen.cpp
                     grshadow.cpp grskidmarks.cpp
                     grsmoke.cpp grtexture.cpp
                     grtracklight.cpp 
                     grtrackmap.cpp grutil.cpp
                     grvtxtable.cpp grrain.cpp
                     ssggraph.cpp)

SET(SSGGRAPH_OTHER_SOURCES )

ADD_INTERFACE_INCLUDEDIR()
ADD_SDLIB_INCLUDEDIR(portability math tgf tgfclient robottools)

ADD_PLIB_INCLUDEDIR()

IF(OPTION_SDL2)
	ADD_SDL2_INCLUDEDIR()
ELSE(OPTION_SDL2)
	ADD_SDL_INCLUDEDIR()
ENDIF(OPTION_SDL2)

IF(COMMAND CMAKE_POLICY)
    CMAKE_POLICY(SET CMP0003 NEW)
ENDIF(COMMAND CMAKE_POLICY)

IF(WIN32)
    # DLL export stuff under Windows (to avoid .def file)
    ADD_DEFINITIONS(-DSSGGRAPH_DLL)
ENDIF(WIN32)

IF(MSVC)
    # Ignore some run-time libs to avoid link time warnings and sometimes even crashes.
    SET(CMAKE_MODULE_LINKER_FLAGS_DEBUG 
        "${CMAKE_MODULE_LINKER_FLAGS_DEBUG} /NODEFAULTLIB:msvcrt.lib /NODEFAULTLIB:libcmtd.lib")
    SET(CMAKE_MODULE_LINKER_FLAGS_RELEASE 
        "${CMAKE_MODULE_LINKER_FLAGS_RELEASE} /NODEFAULTLIB:libcmt.lib")
ENDIF(MSVC)


# Note: Other sources and headers needed for having them available in IDEs.
SD_ADD_LIBRARY(ssggraph MODULE ${SSGGRAPH_SOURCES} ${SSGGRAPH_HEADERS} ${SSGGRAPH_OTHER_SOURCES})

# Might not work with GCC 4.5 or + (non-robot modules crash at 1st reload = after 1 dlclose) 
#SET_TARGET_PROPERTIES(ssggraph PROPERTIES VERSION ${VERSION} SOVERSION 0.0.0)

IF(OPTION_USE_MACPORTS)
   ADD_PLIB_LIBRARY(ssggraph ul sg ssg ssgaux)
ELSE(OPTION_USE_MACPORTS)
   ADD_PLIB_LIBRARY(ssggraph ul sg sl ssg ssgaux)
ENDIF(OPTION_USE_MACPORTS)

IF(OPTION_SDL2)
	ADD_SDL2_LIBRARY(ssggraph)
ELSE(OPTION_SDL2)
	ADD_SDL_LIBRARY(ssggraph)
ENDIF(OPTION_SDL2)

ADD_OPENGL_LIBRARY(ssggraph)

ADD_SDLIB_LIBRARY(ssggraph portability tgf tgfclient robottools)

SD_INSTALL_FILES(LIB modules/graphic TARGETS ssggraph)
