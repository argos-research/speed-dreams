INCLUDE(../../../cmake/macros.cmake)

SET(ROBOT_NAME "argos_AI1")

ROBOT_DATA(NAME ${ROBOT_NAME}
           FILES ${ROBOT_NAME}.xml
           SUBDIRS ${ROBOT_SUBDIRS} PATTERNS *.xml)
