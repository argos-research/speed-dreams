INCLUDE(../../cmake/macros.cmake)

# Official track sets.
SD_ADD_SUBDIRECTORY(dirt)
SD_ADD_SUBDIRECTORY(circuit)
SD_ADD_SUBDIRECTORY(speedway)
SD_ADD_SUBDIRECTORY(road)

# Work-in-progress track sets.
IF(NOT OPTION_OFFICIAL_ONLY)

	SD_ADD_SUBDIRECTORY(gprix)
	SD_ADD_SUBDIRECTORY(development)
	SD_ADD_SUBDIRECTORY(karting)

ENDIF()
