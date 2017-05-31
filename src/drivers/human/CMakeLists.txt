INCLUDE(../../../cmake/macros.cmake)

SET(ROBOT_NAME "human")
SET(ROBOT_INTERFACE WELCOME)
SET(ROBOT_SOURCES ${ROBOT_NAME}.cpp)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

ADD_SDLIB_INCLUDEDIR(robottools tgfclient sensors)

ROBOT_MODULE(NAME ${ROBOT_NAME} VERSION 1.0.0 SOVERSION 1.0.0
             INTERFACE ${ROBOT_INTERFACE}
             SOURCES ${ROBOT_SOURCES})

ADD_SDLIB_LIBRARY(${ROBOT_NAME} robottools tgfclient sensors)

TARGET_LINK_LIBRARIES(${ROBOT_NAME} libpistache.so)

# For data associated to the robot module, see data/drivers/human/CMakeLists.txt
