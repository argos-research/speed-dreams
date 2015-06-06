INCLUDE(../../../cmake/macros.cmake)

FILE(GLOB _FILES *.xml)

SD_INSTALL_FILES(DATA data/tracks FILES ${_FILES})


# Fictive = no-output target, for having source files available in IDEs.
ADD_CUSTOM_TARGET(tracks_xml SOURCES ${_FILES})
