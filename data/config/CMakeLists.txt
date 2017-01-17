INCLUDE(../../cmake/macros.cmake)

# User settings.
IF(OPTION_WEBSERVER)
	SD_INSTALL_FILES(DATA config USER config
                 FILES logging.xml raceengine.xml screen.xml graph.xml sound.xml webserver.xml)
ELSE(OPTION_WEBSERVER)
	SD_INSTALL_FILES(DATA config USER config
                 FILES logging.xml raceengine.xml screen.xml graph.xml sound.xml)
ENDIF(OPTION_WEBSERVER)


# Other config files.
SD_INSTALL_FILES(DATA config FILES raceresults.xsl)

# Race modes.
ADD_SUBDIRECTORY(raceman)
