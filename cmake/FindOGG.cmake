# Locate OGG libraries ()
# This module defines
# OGG_LIBRARY : list of lib names
# OGG_FOUND : if false, do not try to link to OGG
# OGG_INCLUDE_DIR : where to find the headers
#
# $OGG_DIR is an environment variable that would
# correspond to the ./configure --prefix=$OGG_DIR
# used in building OGG.
#
# Created by Joe Thompson

find_path(OGG_INCLUDE_DIR ogg/ogg.h)

set(OGG_NAMES ${OGG_NAMES} ogg)
find_library(OGG_LIBRARY NAMES ${OGG_NAMES})

# handle the QUIETLY and REQUIRED arguments and set OGG_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(OGG DEFAULT_MSG OGG_LIBRARY OGG_INCLUDE_DIR)

if(OGG_FOUND)
  set(OGG_LIBRARIES ${OGG_LIBRARY})
endif()

mark_as_advanced(OGG_LIBRARY OGG_INCLUDE_DIR)
