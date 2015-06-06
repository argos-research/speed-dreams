INCLUDE(../../../cmake/macros.cmake)

SET(ROBOT_NAME "human")

ROBOT_DATA(NAME ${ROBOT_NAME}
           FILES logo.png logo.rgb
           SUBDIRS cars PATTERNS *.xml)

ROBOT_DATA(NAME ${ROBOT_NAME}
           FILES ${ROBOT_NAME}.xml preferences.xml car.xml
           USER)
