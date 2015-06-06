INCLUDE(../../../cmake/macros.cmake)

FILE(GLOB png *.png)
FILE(GLOB bmp *.bmp)
FILE(GLOB ico *.ico)
FILE(GLOB svg *.svg)

SD_INSTALL_FILES(DATA data/icons FILES ${png} ${bmp} ${ico} ${svg})
