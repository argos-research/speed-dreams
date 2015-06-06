INCLUDE(../../../cmake/macros.cmake)

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR})

SET(ROBOT_NAME "kilo2008")

SET(ROBOT_INTERFACE LEGACY WELCOME)

SET(ROBOT_SOURCES ${ROBOT_NAME}.cpp opponent.cpp pit.cpp strategy.cpp
    linalg.h opponent.h strategy.h pit.h cardata.h cardata.cpp spline.cpp
    spline.h raceline.cpp raceline.h kdriver.cpp kdriver.h util.h util.cpp)

ROBOT_MODULE(NAME ${ROBOT_NAME} VERSION 2.0.0 SOVERSION 1.0.0
             INTERFACE ${ROBOT_INTERFACE}
             SOURCES ${ROBOT_SOURCES})

# For data associated to the robot module, see data/drivers/kilo2008/CMakeLists.txt
