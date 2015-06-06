INCLUDE(../../../cmake/macros.cmake)

FILE(GLOB AC_FILES *.ac)
FILE(GLOB ACC_FILES *.acc)

SD_INSTALL_FILES(DATA data/objects FILES ${AC_FILES} ${ACC_FILES})
