INCLUDE(../../../cmake/macros.cmake)

FILE(GLOB TXT_FILES *.txt)
FILE(GLOB rgb *.rgb)
FILE(GLOB rgba *.rgba)
FILE(GLOB png *.png)
FILE(GLOB dds *.dds)

SD_INSTALL_FILES(DATA data/sky FILES ${TXT_FILES})
