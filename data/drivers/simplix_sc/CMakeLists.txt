INCLUDE(../../../cmake/macros.cmake)

# The data for the SC clone of the ubiquitous Simplix module.

SET(ROBOT_SUBDIRS 0 1 2 3 4 5 6 7 8 9 10 11
                  sc-boxer-96 sc-spirit-300 sc-cavallo-360 sc-fmc-gt4 
                  sc-lynx-220 sc-murasama-nsx
                  tracks)

ROBOT_DATA(NAME simplix_sc
           FILES simplix_sc.xml default.xml logo.rgb readme.txt
           SUBDIRS ${ROBOT_SUBDIRS} PATTERNS *.xml logo.rgb *.png *.jpg)
