INCLUDE(../../../cmake/macros.cmake)

# The ubiquitous robot module and its clones.
SET(ROBOT_NAME "simplix")

# Official-only simplix interface.
SET(ROBOT_INTERFACE LEGACY WELCOME)

# Work-in-progress / unofficial simplix interface.
#IF(NOT OPTION_OFFICIAL_ONLY)
#
#    SET(ROBOT_INTERFACE ${ROBOT_INTERFACE} simplix_mpa1)
#
#ENDIF()

SET(ROBOT_SOURCES 
    src/unitcarparam.cpp
    src/unitcarparam.h
    src/unitcharacteristic.cpp
    src/unitcharacteristic.h
    src/unitclothoid.cpp
    src/unitclothoid.h
    src/unitcollision.cpp
    src/unitcollision.h
    src/unitcommon.cpp
    src/unitcommondata.cpp
    src/unitcommondata.h
    src/unitcommon.h
    src/unitcubic.cpp
    src/unitcubic.h
    src/unitcubicspline.cpp
    src/unitcubicspline.h
    src/unitdriver.cpp
    src/unitdriver.h
    src/unitfixcarparam.cpp
    src/unitfixcarparam.h
    src/unitglobal.h
    src/unitlane.cpp
    src/unitlane.h
    src/unitlanepoint.cpp
    src/unitlanepoint.h
    src/unitlinalg.cpp
    src/unitlinalg.h
    src/unitlinreg.cpp
    src/unitlinreg.h
    src/unitmain.cpp
    src/unitopponent.cpp
    src/unitopponent.h
    src/unitparabel.cpp
    src/unitparabel.h
    src/unitparam.cpp
    src/unitparam.h
    src/unitpidctrl.cpp
    src/unitpidctrl.h
    src/unitpit.cpp
    src/unitpit.h
    src/unitpitparam.cpp
    src/unitpitparam.h
    src/unitsection.cpp
    src/unitsection.h
    src/unitstrategy.cpp
    src/unitstrategy.h
    src/unitsysfoo.cpp
    src/unitsysfoo.h
    src/unittmpcarparam.cpp
    src/unittmpcarparam.h
    src/unittrack.cpp
    src/unittrack.h
    src/unitvec2d.cpp
    src/unitvec2d.h
    src/unitvec3d.cpp
    src/unitvec3d.h
)

# Official-only simplix instances.
SET(ROBOT_CLONES simplix_36GP simplix_sc simplix_srw simplix_trb1 simplix_ls1 simplix_ls2 simplix_mp5 
			simplix_mpa1 simplix_mpa11 simplix_mpa12 simplix_lp1 simplix_ref)

# The ubiquitous robot module and its clones.
ROBOT_MODULE(NAME ${ROBOT_NAME} VERSION 3.3.0 SOVERSION 1.0.0
             INTERFACE ${ROBOT_INTERFACE}
             SOURCES ${ROBOT_SOURCES}
             CLONENAMES ${ROBOT_CLONES})


# For data associated to the ubiquitous robot module and its clones,
# see data/drivers/simplix*/CMakeLists.txt
