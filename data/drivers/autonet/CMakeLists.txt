INCLUDE(../../../cmake/macros.cmake)

# The data for the SC clone of the ubiquitous USR robot module.

SET(ROBOT_SUBDIRS sc-lynx-220)

ROBOT_DATA(NAME autonet
           FILES autonet.xml readme.txt
           SUBDIRS ${ROBOT_SUBDIRS} PATTERNS *.xml)
