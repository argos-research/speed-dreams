INCLUDE(../../../cmake/macros.cmake)

SET(ROBOT_SUBDIRS sc-lynx-220)

ROBOT_DATA(NAME autonet
           FILES autonet.xml readme.txt
           SUBDIRS ${ROBOT_SUBDIRS} PATTERNS *.xml)
