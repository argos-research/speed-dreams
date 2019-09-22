INCLUDE(../../../cmake/macros.cmake)

SET(ROBOT_NAME "human")
SET(ROBOT_INTERFACE WELCOME)
SET(ROBOT_SOURCES ${ROBOT_NAME}.cpp)

ADD_SDLIB_INCLUDEDIR(robottools tgfclient sensors)
INCLUDE_DIRECTORIES(/home/bernhard/repositories/speed-dreams/src/drivers/RobotGamePause)

ROBOT_MODULE(NAME ${ROBOT_NAME} VERSION 1.0.0 SOVERSION 1.0.0
             INTERFACE ${ROBOT_INTERFACE}
             SOURCES ${ROBOT_SOURCES})

ADD_SDLIB_LIBRARY(${ROBOT_NAME} robottools tgfclient sensors)


INCLUDE_DIRECTORIES(/home/bernhard/repositories/speed-dreams/src/modules/userinterface/legacymenu)
IF(OPTION_SDL2)
	ADD_SDL2_INCLUDEDIR()
ELSE(OPTION_SDL2)
	ADD_SDL_INCLUDEDIR()
ENDIF(OPTION_SDL2)

# For data associated to the robot module, see data/drivers/human/CMakeLists.txt
