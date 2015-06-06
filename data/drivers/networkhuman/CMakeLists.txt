INCLUDE(../../../cmake/macros.cmake)

SET(ROBOT_NAME "networkhuman")

ROBOT_DATA(NAME ${ROBOT_NAME}
           FILES logo.png)
#           SUBDIRS cars PATTERNS *.xml)

ROBOT_DATA(NAME ${ROBOT_NAME}
           FILES ${ROBOT_NAME}.xml preferences.xml car.xml
           USER)
