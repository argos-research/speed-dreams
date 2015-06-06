INCLUDE(../../../../cmake/macros.cmake)

SET(OSGGRAPH_HEADERS OsgUtil/OsgVectorArrayAdapter.h
	             OsgUtil/OsgColor.h
		     OsgUtil/OsgAtomic.h
                     OsgUtil/OsgReferenced.h 
		     #OsgUtil/OsgSharedPtr.h
		     #OsgUtil/OsgStateAttributeFactory.h 
		     OsgUtil/OsgMath.h

                     OsgSky/OsgSphere.h
		     OsgSky/OsgSun.h 
                     OsgSky/OsgMoon.h 
                     OsgSky/OsgStars.h 
                     OsgSky/OsgDome.h 
                     OsgSky/OsgCloud.h
		     OsgSky/OsgSky.h
                     
		     OsgLoader/AccGeode.h
		     OsgLoader/AccException.h
		     OsgLoader/ReaderWriterACC.h
                     OsgLoader/OsgLoader.h

		     OsgWorld/OsgScenery.h

		     OsgFX/OsgRender.h
		     OsgFX/OsgReflectionMapping.h
                     OsgFX/OsgShader.h
                       
                     OsgView/OsgView.h  	
                     OsgView/OsgScreens.h 
                     OsgView/OsgCamera.h
		     OsgView/OsgDebugHUD.h

		     OsgCar/OsgCar.h 
		     OsgCar/OsgWheel.h 
		     OsgCar/OsgBrake.h

		     OsgGraph.h
		     OsgMain.h
                     )
			
SET(OSGGRAPH_SOURCES #OsgUtil/OsgStateAttributeFactory.cpp 
		     OsgUtil/OsgMath.cpp

		     OsgSky/OsgSphere.cpp
		     OsgSky/OsgSun.cpp 
		     OsgSky/OsgMoon.cpp 
		     OsgSky/OsgStars.cpp 
                     OsgSky/OsgDome.cpp  
                     OsgSky/OsgCloud.cpp 
                     OsgSky/OsgSky.cpp 

                     OsgLoader/AccGeode.cpp 
                     OsgLoader/AccException.cpp
                     OsgLoader/ReaderWriterACC.cpp 
                     OsgLoader/OsgLoader.cpp 

                     OsgWorld/OsgBackground.cpp
                     OsgWorld/OsgScenery.cpp 

		     OsgFX/OsgRender.cpp
		     OsgFX/OsgReflectionMapping.cpp
                     OsgFX/OsgShader.cpp

                     OsgView/OsgView.cpp 
                     OsgView/OsgScreens.cpp
		     OsgView/OsgCamera.cpp
		     OsgView/OsgDebugHUD.cpp

		     OsgCar/OsgCar.cpp 
		     OsgCar/OsgWheel.cpp
                     OsgCar/OsgBrake.cpp

		     OsgGraph.cpp
		     OsgMain.cpp
		     )

INCLUDE_DIRECTORIES(OsgUtil OsgSky OsgLoader OsgWorld OsgFX OsgView OsgCar .)

ADD_INTERFACE_INCLUDEDIR()
ADD_SDLIB_INCLUDEDIR(portability math tgf tgfclient robottools)

ADD_SDL_INCLUDEDIR()
ADD_OSG_INCLUDEDIR()

# Temporary, but should not remain !
ADD_PLIB_INCLUDEDIR()

IF(COMMAND CMAKE_POLICY)
    CMAKE_POLICY(SET CMP0003 NEW)
ENDIF(COMMAND CMAKE_POLICY)

IF(WIN32)
    # DLL export stuff under Windows (to avoid .def file)
    ADD_DEFINITIONS(-DOSGGRAPH_DLL)
ENDIF(WIN32)

IF(MSVC)
   # Ignore some run-time libs to avoid link time warnings and sometimes even crashes.
    SET(CMAKE_MODULE_LINKER_FLAGS_DEBUG "${CMAKE_MODULE_LINKER_FLAGS_DEBUG} /NODEFAULTLIB:msvcrt.lib")
ENDIF(MSVC)

# Note: Other sources and headers needed for having them available in IDEs.
SD_ADD_LIBRARY(osggraph MODULE ${OSGGRAPH_SOURCES} ${OSGGRAPH_HEADERS} ${OSGGRAPH_OTHER_SOURCES})

# Might not work with GCC 4.5 or + (non-robot modules crash at 1st reload = after 1 dlclose) 
#SET_TARGET_PROPERTIES(osggraph PROPERTIES VERSION ${VERSION} SOVERSION 0.0.0)

ADD_OSG_LIBRARY(osggraph)
ADD_SDL_LIBRARY(osggraph)
ADD_OPENGL_LIBRARY(osggraph)

# Temporary, but should not remain !
ADD_PLIB_LIBRARY(osggraph ul sg)

ADD_SDLIB_LIBRARY(osggraph tgf tgfclient robottools)

SD_INSTALL_FILES(LIB modules/graphic TARGETS osggraph)
