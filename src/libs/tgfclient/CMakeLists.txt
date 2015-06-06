INCLUDE(../../../cmake/macros.cmake)

#PROJECT(tgfclient)

ADD_INTERFACE_INCLUDEDIR()
ADD_SDLIB_INCLUDEDIR(tgf math portability)
ADD_SDL_INCLUDEDIR()
ADD_JPEG_INCLUDEDIR()
ADD_PLIB_INCLUDEDIR()
ADD_OPENAL_INCLUDEDIR()
ADD_VORBIS_INCLUDEDIR()


SET(_SOURCES control.cpp glfeatures.cpp guibutton.cpp guifont.cpp
             guiimage.cpp guimenu.cpp guiscrollbar.cpp guitexture.cpp
             tgfclient.cpp gui.cpp guiedit.cpp guihelp.cpp
             guilabel.cpp guiobject.cpp guiscrollist.cpp 
             guicombobox.cpp guicheckbox.cpp guiprogresbar.cpp 
             guiscreen.cpp guieventloop.cpp guiapplication.cpp 
             musicplayer.cpp musicplayer.h 
             glfeatures.h gui.h guiscreen.h guimenu.h tgfclient.h guifont.h )
SET(_OTHER_SOURCES guimenutest.xml)

SET(_SOURCES ${_SOURCES} 
            soundstream.cpp soundstream.h 
            oggsoundstream.cpp oggsoundstream.h 
            openalmusicplayer.cpp openalmusicplayer.h )

#disable developer warning
IF (COMMAND CMAKE_POLICY)
    CMAKE_POLICY(SET CMP0003 NEW)
ENDIF(COMMAND CMAKE_POLICY)

IF(WIN32)
    # DLL export stuff under Windows (to avoid .def file)
    ADD_DEFINITIONS(-DTGFCLIENT_DLL)
ENDIF(WIN32)
    
IF(MSVC)
    # Ignore some run-time libs to avoid link time warnings and sometimes even crashes.
    SET(CMAKE_SHARED_LINKER_FLAGS_DEBUG 
        "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} /NODEFAULTLIB:msvcrt.lib /NODEFAULTLIB:libcmt.lib")
    SET(CMAKE_SHARED_LINKER_FLAGS_RELEASE 
        "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /NODEFAULTLIB:libcmt.lib")
ENDIF(MSVC)

# Note: Other sources needed for having them available in IDEs.
SD_ADD_LIBRARY(tgfclient SHARED ${_SOURCES} ${_OTHER_SOURCES})

# Might not work with GCC 4.5 or + (non-robot modules crash at 1st reload = after 1 dlclose) 
#SET_TARGET_PROPERTIES(tgfclient PROPERTIES VERSION ${VERSION} SOVERSION 0.0.0)

ADD_PLIB_LIBRARY(tgfclient ul sg sl)

ADD_SDLIB_LIBRARY(tgfclient portability tgf networking)

ADD_PNG_LIBRARY(tgfclient)
ADD_JPEG_LIBRARY(tgfclient)
IF(OPENGL_FOUND)
    TARGET_LINK_LIBRARIES(tgfclient ${OPENGL_LIBRARY})
ENDIF(OPENGL_FOUND)

IF(OPENAL_FOUND)
    TARGET_LINK_LIBRARIES(tgfclient ${OPENAL_LIBRARY})
ENDIF(OPENAL_FOUND)

IF(VORBIS_FOUND)
      TARGET_LINK_LIBRARIES(tgfclient ${VORBIS_LIBRARY})
ENDIF(VORBIS_FOUND)
IF(VORBISFILE_FOUND)
      TARGET_LINK_LIBRARIES(tgfclient ${VORBISFILE_LIBRARY})
ENDIF(VORBISFILE_FOUND)
IF(OGG_FOUND)
      TARGET_LINK_LIBRARIES(tgfclient ${OGG_LIBRARY})
ENDIF(OGG_FOUND)

ADD_PLIB_LIBRARY(tgfclient ul sg js)
ADD_SDL_LIBRARY(tgfclient)

IF(WIN32)
#joystick library
    TARGET_LINK_LIBRARIES(tgfclient winmm)
ENDIF(WIN32)

IF(UNIX)
    ADD_X11_LIBRARY(tgfclient)
    IF(OPTION_XRANDR)
        ADD_XRANDR_LIBRARY(tgfclient)
    ENDIF(OPTION_XRANDR)
ENDIF(UNIX)

IF(UNIX)
    SD_INSTALL_FILES(LIB lib TARGETS tgfclient)
ELSE(UNIX)
    SD_INSTALL_FILES(BIN TARGETS tgfclient)
ENDIF(UNIX)

SD_INSTALL_FILES(INCLUDE FILES glfeatures.h guiscreen.h tgfclient.h)

