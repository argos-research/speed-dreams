INCLUDE(../../../cmake/macros.cmake)

#PROJECT(networking)

SET(SD_NETWORK_SOURCES network.cpp server.cpp client.cpp robotxml.cpp pack.cpp)
SET(SD_NETWORK_HEADERS network.h robotxml.h pack.h)

#disable developer warning
if (COMMAND cmake_policy)
	cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

# Note: Headers needed for having them available in IDEs.
#TODO: Maje this a real (dynamically loadable) module, or move it to standard src/libs !
SD_ADD_LIBRARY(networking SHARED ${SD_NETWORK_SOURCES} ${SD_NETWORK_HEADERS})

# Might not work with GCC 4.5 or + (non-robot modules crash at 1st reload = after 1 dlclose) 
#SET_TARGET_PROPERTIES(networking PROPERTIES VERSION ${VERSION} SOVERSION 0.0.0)

ADD_INTERFACE_INCLUDEDIR()
ADD_SDL_INCLUDEDIR()
ADD_SDLIB_INCLUDEDIR(networking portability tgf math)

ADD_PLIB_INCLUDEDIR()
ADD_ENET_INCLUDEDIR()
ADD_PLIB_LIBRARY(networking ul sg)
ADD_ENET_LIBRARY(networking)
ADD_SDL_LIBRARY(networking)

IF(WIN32)
	TARGET_LINK_LIBRARIES(networking Winmm ws2_32)
ENDIF(WIN32)

IF(MSVC)
	# Ignore some run-time libs to avoid link time warnings and sometimes even crashes.
	SET(CMAKE_SHARED_LINKER_FLAGS_DEBUG 
		"${CMAKE_SHARED_LINKER_FLAGS_DEBUG} /NODEFAULTLIB:msvcrt.lib /NODEFAULTLIB:libcmt.lib")
	SET(CMAKE_SHARED_LINKER_FLAGS_RELEASE 
		"${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /NODEFAULTLIB:libcmt.lib")
ENDIF(MSVC)


ADD_SDLIB_LIBRARY(networking portability tgf)

IF(WIN32)
	SD_INSTALL_FILES(BIN TARGETS networking)
ELSE(WIN32)
	SD_INSTALL_FILES(LIB lib TARGETS networking)
ENDIF(WIN32)
