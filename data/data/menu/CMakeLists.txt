INCLUDE(../../../cmake/macros.cmake)

FILE(GLOB _MENU_FILES *.xml)


SD_INSTALL_FILES(DATA data/menu FILES ${_MENU_FILES})

# Fictive = no-output target, for having source files available in IDEs.
ADD_CUSTOM_TARGET(legacymenu_xml SOURCES ${_MENU_FILES})
