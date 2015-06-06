INCLUDE(../../../cmake/macros.cmake)

# Official car sets.
SD_INSTALL_FILES(DATA cars/categories
                 FILES Supercars.xml 36GP.xml TRB1.xml LS-GT1.xml LS-GT2.xml MPA1.xml MP5.xml RS.xml Preview.xml)

# Internal / non-official / work-in-progress car sets
# (we deliver all of them in case some advanced users / developers
#  manually install cars from these categories).
SD_INSTALL_FILES(DATA cars/categories
                 FILES References.xml LS-P1.xml LS-P2.xml MP1.xml MP10.xml Raid-T1.xml Nascar.xml)

