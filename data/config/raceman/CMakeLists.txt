INCLUDE(../../../cmake/macros.cmake)

SET(_FILES practice.xml quickrace.xml
           singleevent-36gp.xml
           singleevent-challenge.xml singleevent-endurance.xml
           singleevent-ls-gt1.xml singleevent-mp5.xml
           championship.xml championship-supercars.xml championship-trb1.xml
           championship-36gp.xml championship-ls-gt1.xml championship-mp5.xml
           networkrace.xml championship-mpa1.xml career-36GP.xml career-supercars.xml
		   career_36GP.xmls career_MPA1.xmls
           career_supercars.xmls career_ls1.xmls)

SD_INSTALL_FILES(DATA config/raceman USER config/raceman FILES ${_FILES})

SET(_EXTRA_FILES extra/skill.xml)

SD_INSTALL_FILES(DATA config/raceman/extra USER config/raceman/extra 
                 FILES ${_EXTRA_FILES})

# Fictive = no-output target, for having source files available in IDEs.
ADD_CUSTOM_TARGET(raceman SOURCES ${_FILES} ${_EXTRA_FILES})

