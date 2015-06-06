INCLUDE(../../../cmake/macros.cmake)

SET(_SOURCES constants.h linalg_t.h straight2_t.h  v2_t.h v3_t.h  v4_t.h
             point3d.h SGCMath.h SGGeoc.h SGGeod.h sg_geodesy.h SGGeodesy.h
             SGLimits.h SGMath.h SGMathFwd.h SGMatrix.h SGMisc.h
             SGQuat.h sg_random.h sg_types.h SGVec3.h SGVec4.h vector.h)

SD_INSTALL_FILES(INCLUDE tmath FILES ${_SOURCES})

# Fictive = no-output target, for having source files available in IDEs.
ADD_CUSTOM_TARGET(math SOURCES ${_SOURCES})
