INCLUDE(../../../cmake/macros.cmake)

FILE(GLOB sounds *.wav)

SD_INSTALL_FILES(DATA data/sound FILES ${sounds})
