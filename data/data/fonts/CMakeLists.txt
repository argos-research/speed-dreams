INCLUDE(../../../cmake/macros.cmake)

FILE(GLOB glf *.glf)
FILE(GLOB ttf *.ttf)

SD_INSTALL_FILES(DATA data/fonts FILES ${glf} ${ttf})
