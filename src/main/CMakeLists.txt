INCLUDE(../../cmake/macros.cmake)

PROJECT(speed-dreams-2)

SET(_TARGET_NAME speed-dreams-2)

SET(_SOURCES main.cpp)
IF(MSVC)
	SET(_SOURCES ${_SOURCES} main.rc resource.h ../../data/data/icons/icon.ico)
ENDIF(MSVC)

ADD_INTERFACE_INCLUDEDIR()
ADD_SDLIB_INCLUDEDIR(tgf tgfclient tgfdata math portability)

ADD_SDL_INCLUDEDIR()
ADD_PLIB_INCLUDEDIR()

# Disable developer warning
IF(COMMAND CMAKE_POLICY)
	CMAKE_POLICY(SET CMP0003 NEW)
ENDIF(COMMAND CMAKE_POLICY)

IF(MSVC)
    # Ignore some run-time libs to avoid link time warnings and sometimes even crashes.
    SET(CMAKE_EXE_LINKER_FLAGS_DEBUG 
        "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /NODEFAULTLIB:msvcrt.lib /NODEFAULTLIB:libcmt.lib")
    SET(CMAKE_EXE_LINKER_FLAGS_RELEASE 
        "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /NODEFAULTLIB:libcmt.lib")
ENDIF(MSVC)

SD_ADD_EXECUTABLE(${_TARGET_NAME} ${_SOURCES})

ADD_SDLIB_LIBRARY(${_TARGET_NAME} portability tgf tgfclient tgfdata)

# Needed when using the "MinGW Makefiles" CMake generator (but not with the "MSYS Makefiles" one !)
IF(MINGW)
	TARGET_LINK_LIBRARIES(${_TARGET_NAME} ${MINGW32_LIBRARY})
ENDIF(MINGW)

IF(UNIX)
	SD_INSTALL_FILES(MAN man6 PREFIX ${SOURCE_DIR}/doc/man FILES ${_TARGET_NAME}.6)
ENDIF(UNIX)

SD_INSTALL_FILES(BIN TARGETS ${_TARGET_NAME})

# Under Windows, install needed 3rd party DLLs close to Speed Dreams executable.
IF(WIN32)
	INCLUDE(customthirdparty)
	SD_INSTALL_CUSTOM_3RDPARTY(${_TARGET_NAME})
ENDIF(WIN32)
