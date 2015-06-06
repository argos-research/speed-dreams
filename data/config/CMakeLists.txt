INCLUDE(../../cmake/macros.cmake)

# User settings.
SD_INSTALL_FILES(DATA config USER config
                 FILES logging.xml raceengine.xml screen.xml graph.xml sound.xml)

# Other config files.
SD_INSTALL_FILES(DATA config FILES raceresults.xsl)

# Race modes.
ADD_SUBDIRECTORY(raceman)
