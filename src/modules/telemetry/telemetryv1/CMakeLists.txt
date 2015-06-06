INCLUDE(../../../../cmake/macros.cmake)

SET(TELEMETRY_SOURCES telemetryitf.cpp telemetry.cpp)
IF(MSVC)
  SET(TELEMETRY_SOURCES ${TELEMETRY_SOURCES} telemetry.def)
ENDIF()
SET(TELEMETRY_HEADERS tlm.h)
SET(TELEMETRY_OTHER_SOURCES telemetry.sh)

ADD_INTERFACE_INCLUDEDIR()
ADD_SDLIB_INCLUDEDIR(tgf portability)

# Note: Headers and other source files needed for having them available in IDEs.
SD_ADD_LIBRARY(telemetryv1 MODULE ${TELEMETRY_SOURCES} ${TELEMETRY_HEADERS} ${TELEMETRY_OTHER_SOURCES})

# Might not work with GCC 4.5 or + (non-robot modules crash at 1st reload = after 1 dlclose) 
#SET_TARGET_PROPERTIES(telemetryv1 PROPERTIES VERSION ${VERSION} SOVERSION 0.0.0)

ADD_SDLIB_LIBRARY(telemetryv1 portability tgf)

SD_INSTALL_FILES(LIB modules/telemetry TARGETS telemetryv1)
