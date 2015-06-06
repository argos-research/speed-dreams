INCLUDE(../../../cmake/macros.cmake)

FILE(GLOB rgb *.rgb)
FILE(GLOB rgba *.rgba)
FILE(GLOB png *.png)

SD_INSTALL_FILES(DATA data/textures FILES ${rgb} ${rgba} ${png})
