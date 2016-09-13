INCLUDE(../../../cmake/macros.cmake)

PROJECT(trackgen)

SET(_TARGET_NAME ${SD_TOOLS_EXECPREFIX}trackgen)

SET(_SOURCES ac3d.cpp easymesh.cpp elevation.cpp main.cpp
             objects.cpp relief.cpp track.cpp util.cpp)
SET(_HEADERS ac3d.h easymesh.h elevation.h objects.h relief.h trackgen.h util.h)
SET(_OTHER_SOURCES trackgen.xml)

ADD_INTERFACE_INCLUDEDIR()
ADD_SDLIB_INCLUDEDIR(robottools tgf tgfclient math portability)

IF(OPTION_SDL2)
	ADD_SDL2_INCLUDEDIR()
ELSE(OPTION_SDL2)
	ADD_SDL_INCLUDEDIR()
ENDIF(OPTION_SDL2)

ADD_PLIB_INCLUDEDIR()

#disable developer warning
IF (COMMAND CMAKE_POLICY)
    CMAKE_POLICY(SET CMP0003 NEW)
ENDIF(COMMAND CMAKE_POLICY)

IF(MSVC)
    # Ignore some run-time libs to avoid link time warnings and sometimes even crashes.
    SET(CMAKE_EXE_LINKER_FLAGS_DEBUG 
        "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /NODEFAULTLIB:msvcrt.lib /NODEFAULTLIB:libcmtd.lib")
    SET(CMAKE_EXE_LINKER_FLAGS_RELEASE 
        "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /NODEFAULTLIB:libcmt.lib")
ENDIF(MSVC)

# Note: Headers and other sources needed for having them available in IDEs.
SD_ADD_EXECUTABLE(${_TARGET_NAME} ${_SOURCES} ${_HEADERS} ${_OTHER_SOURCES})

ADD_SDLIB_LIBRARY(${_TARGET_NAME} portability robottools tgf tgfclient)

IF(OPENGL_FOUND)
    TARGET_LINK_LIBRARIES(${_TARGET_NAME} ${OPENGL_LIBRARY})
ENDIF(OPENGL_FOUND)

ADD_PNG_LIBRARY(${_TARGET_NAME})

IF(OPTION_SDL2)
	ADD_SDL2_LIBRARY(${_TARGET_NAME})
ELSE(OPTION_SDL2)
	ADD_SDL_LIBRARY(${_TARGET_NAME})
ENDIF(OPTION_SDL2)

ADD_PLIB_LIBRARY(${_TARGET_NAME} sg ssg ul)
ADD_DL_LIBRARY(${_TARGET_NAME})

# Needed when using the "MinGW Makefiles" CMake generator (but not with the "MSYS Makefiles" one !?)
IF(MINGW)
	TARGET_LINK_LIBRARIES(${_TARGET_NAME} ${MINGW32_LIBRARY})
ENDIF(MINGW)

IF(UNIX)
    SD_INSTALL_FILES(MAN man6 PREFIX ${SOURCE_DIR}/doc/man FILES ${_TARGET_NAME}.6)
ENDIF(UNIX)

SD_INSTALL_FILES(BIN TARGETS ${_TARGET_NAME})

IF((APPLE) AND ("${CMAKE_INSTALL_PREFIX}" MATCHES "\\.app$"))
   SET(APPS "\${CMAKE_INSTALL_PREFIX}/games/${_TARGET_NAME}")
   SET(DIRS /opt/local)
   SET(DIRS ${DIRS} "\${CMAKE_INSTALL_PREFIX}/lib64/games/${_TARGET_NAME}/lib")
   SET(PlugIns)

   INSTALL(CODE "
   INCLUDE(BundleUtilities)
   SET(BU_CHMOD_BUNDLE_ITEMS ON)
   FIXUP_BUNDLE(\"${APPS}\" \"${PlugIns}\" \"${DIRS}\")")
endif()
