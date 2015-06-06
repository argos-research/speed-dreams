INCLUDE(../../../cmake/macros.cmake)

FILE(GLOB VERT_FILES *.vert)
FILE(GLOB FRAG_FILES *.frag)

# Fictive = no-output target, for having source files available in IDEs.
SET(SHADER_SOURCES car.vert car.frag)
ADD_CUSTOM_TARGET(shaders SOURCES ${SHADER_SOURCES})


SD_INSTALL_FILES(DATA data/shaders FILES ${VERT_FILES})
SD_INSTALL_FILES(DATA data/shaders FILES ${FRAG_FILES})

