INCLUDE(../../../cmake/macros.cmake)

PROJECT(nfs2ac)

SET(_TARGET_NAME ${SD_TOOLS_EXECPREFIX}nfs2ac)

SET(NFS2AC_SOURCES mainnfs2ac.cpp)

SD_ADD_EXECUTABLE(${_TARGET_NAME} ${NFS2AC_SOURCES})

IF(UNIX)
    SD_INSTALL_FILES(MAN man6 PREFIX ${SOURCE_DIR}/doc/man FILES ${_TARGET_NAME}.6)
ENDIF(UNIX)

SD_INSTALL_FILES(BIN TARGETS ${_TARGET_NAME})

IF((APPLE) AND ("${CMAKE_INSTALL_PREFIX}" MATCHES "\\.app$"))
   set(APPS "\${CMAKE_INSTALL_PREFIX}/games/${_TARGET_NAME}")
   set(DIRS /opt/local)
   set(DIRS ${DIRS} "\${CMAKE_INSTALL_PREFIX}/lib64/games/${_TARGET_NAME}/lib")
   set(PlugIns)

   INSTALL(CODE "
   include(BundleUtilities)
   fixup_bundle(\"${APPS}\" \"${PlugIns}\" \"${DIRS}\")
   " COMPONENT Runtime)
endif()
