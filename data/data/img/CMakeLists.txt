INCLUDE(../../../cmake/macros.cmake)

FILE(GLOB png *.png)
FILE(GLOB jpg *.jpg)


SD_INSTALL_FILES(DATA data/img FILES ${png} ${jpg})
