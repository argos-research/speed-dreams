INCLUDE(../../../cmake/macros.cmake)

#PROJECT(learning)

SET(LEARNING_HEADERS learning.h ANN.h ann_policy.h Distribution.h learn_debug.h List.h 
                     MathFunctions.h policy.h real.h string_utils.h)
SET(LEARNING_SOURCES ANN.cpp Distribution.cpp List.cpp MathFunctions.cpp
                     ann_policy.cpp policy.cpp string_utils.cpp)
SET(LEARNING_OTHER_SOURCES README) 

ADD_SDLIB_INCLUDEDIR(portability)

# Disable developer warning
IF (COMMAND CMAKE_POLICY)
	CMAKE_POLICY(SET CMP0003 NEW)
ENDIF(COMMAND CMAKE_POLICY)

IF(WIN32)
	# DLL export stuff under Windows (to avoid .def file)
	ADD_DEFINITIONS(-DLEARNING_DLL)
ENDIF(WIN32)

# Note: Other sources and headers needed for having them available in IDEs.
SD_ADD_LIBRARY(learning SHARED ${LEARNING_SOURCES} ${LEARNING_HEADERS} ${LEARNING_OTHER_SOURCES})

# Might not work with GCC 4.5 or + (non-robot modules crash at 1st reload = after 1 dlclose) 
#SET_TARGET_PROPERTIES(learning PROPERTIES VERSION ${VERSION} SOVERSION 0.0.0)

ADD_SDLIB_LIBRARY(learning portability)

IF(WIN32)
	SD_INSTALL_FILES(BIN TARGETS learning)
ELSE(WIN32)
	SD_INSTALL_FILES(LIB lib TARGETS learning)
ENDIF(WIN32)

SD_INSTALL_FILES(INCLUDE learning FILES ${LEARNING_HEADERS})

