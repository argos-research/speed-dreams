INCLUDE(../../../cmake/macros.cmake)

# Official track sets.
SD_ADD_SUBDIRECTORY(aalborg)
SD_ADD_SUBDIRECTORY(brondehach)
SD_ADD_SUBDIRECTORY(chemisay)
SD_ADD_SUBDIRECTORY(corkscrew)
SD_ADD_SUBDIRECTORY(espie)
SD_ADD_SUBDIRECTORY(forza)
SD_ADD_SUBDIRECTORY(goldstone-sand)
SD_ADD_SUBDIRECTORY(karwada)
SD_ADD_SUBDIRECTORY(migrants)
SD_ADD_SUBDIRECTORY(ruudskogen)

# Work-in-progress track sets.
IF(NOT OPTION_OFFICIAL_ONLY)

	SD_ADD_SUBDIRECTORY(bueno)

ENDIF()
