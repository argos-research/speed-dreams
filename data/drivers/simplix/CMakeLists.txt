INCLUDE(../../../cmake/macros.cmake)

# The data for the ubiquitous Simplix module.

SET(ROBOT_SUBDIRS trb1-cavallo-360rb
                  ls1-archer-r9 ls1-cavallo-570s1 ls1-newcastle-fury ls1-taipan-ltsr
                  ls1-toro-rgt ls1-vulture-v6r ls1-zentek-z7r
                  sc-boxer-96 sc-cavallo-360 sc-fmc-gt4 sc-lynx-220 
                  sc-murasama-nsx sc-spirit-300
                  tracks
                  0)

ROBOT_DATA(NAME simplix
           FILES simplix.xml default.xml logo.rgb readme.txt
           SUBDIRS ${ROBOT_SUBDIRS} PATTERNS *.xml *.rgb)
