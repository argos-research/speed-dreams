INCLUDE(../../../cmake/macros.cmake)


ADD_INTERFACE_INCLUDEDIR()
ADD_SDLIB_INCLUDEDIR(math tgf robottools)

SET(SENSORS_HEADERS positionTracker.h randomUtil.h gpsSensor.h obstacleSensors.h)

SET(SENSORS_SOURCES positionTracker.cpp randomUtil.cpp gpsSensor.cpp obstacleSensors.cpp)

SD_ADD_LIBRARY(sensors SHARED ${SENSORS_SOURCES} ${SENSORS_HEADERS})

ADD_SDLIB_LIBRARY(sensors tgf)

IF(WIN32)
	SD_INSTALL_FILES(BIN TARGETS sensors)
ELSE(WIN32)
	SD_INSTALL_FILES(LIB lib TARGETS sensors)
ENDIF(WIN32)

SD_INSTALL_FILES(INCLUDE sensors FILES ${SENSORS_HEADERS})
