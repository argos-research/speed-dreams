INCLUDE(../../../cmake/macros.cmake)

ADD_SUBDIRECTORY(standardgame)

# Unofficial race engines.
IF(NOT OPTION_OFFICIAL_ONLY)

	SD_ADD_SUBDIRECTORY(genparoptv1)

ENDIF()

