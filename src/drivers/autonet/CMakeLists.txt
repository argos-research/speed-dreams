INCLUDE(../../../cmake/macros.cmake)

SET(ROBOT_NAME "autonet")

SET(ROBOT_INTERFACE WELCOME)

SET(ROBOT_SOURCES ${ROBOT_NAME}.cpp serial.c timer.cpp)

ADD_SDLIB_INCLUDEDIR(robottools tgf sensors)

ROBOT_MODULE(NAME ${ROBOT_NAME} VERSION 1.0.0 SOVERSION 1.0.0
             INTERFACE ${ROBOT_INTERFACE}
             SOURCES ${ROBOT_SOURCES})

ADD_SDLIB_LIBRARY(${ROBOT_NAME} robottools tgf sensors)

# For data associated to the robot module, see data/drivers/autonet/CMakeLists.txt
