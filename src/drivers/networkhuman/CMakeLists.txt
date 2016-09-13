INCLUDE(../../../cmake/macros.cmake)

SET(ROBOT_NAME "networkhuman")
SET(ROBOT_INTERFACE WELCOME)
SET(ROBOT_SOURCES ${ROBOT_NAME}.cpp)

ADD_SDLIB_INCLUDEDIR(portability tgfclient networking)
IF(OPTION_SDL2)
	ADD_SDL2_INCLUDEDIR()
ELSE(OPTION_SDL2)
	ADD_SDL_INCLUDEDIR()
ENDIF(OPTION_SDL2)

ROBOT_MODULE(NAME ${ROBOT_NAME} VERSION ${VERSION} SOVERSION 0.0.0
             INTERFACE ${ROBOT_INTERFACE}
             SOURCES ${ROBOT_SOURCES})

ADD_SDLIB_LIBRARY(${ROBOT_NAME} portability tgfclient networking)

# For data associated to the robot module, see data/drivers/networkhuman/CMakeLists.txt
