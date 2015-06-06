INCLUDE(../../../../cmake/macros.cmake)

SET(GENNMTAB_SOURCES gennmtab.c)
SET(GENNMTAB_OTHER_SOURCES gennmtab)

# Note: Other sources needed for having them available in IDEs.
SD_ADD_EXECUTABLE(gennmtab ${GENNMTAB_SOURCES} ${GENNMTAB_OTHER_SOURCES})

