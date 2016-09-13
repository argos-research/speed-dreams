# Locate VORBIS libraries ()
# This module defines
# VORBIS_LIBRARY : list of lib names
# VORBIS_FOUND : if false, do not try to link to VORBIS
# VORBIS_INCLUDE_DIR : where to find the headers
#
# $VORBIS_DIR is an environment variable that would
# correspond to the ./configure --prefix=$VORBIS_DIR
# used in building VORBIS.
#
# Created by Joe Thompson

find_path(VORBIS_INCLUDE_DIR vorbis/vorbisfile.h)

set(VORBIS_NAMES ${VORBIS_NAMES} vorbis)
find_library(VORBIS_LIBRARY NAMES ${VORBIS_NAMES})

# handle the QUIETLY and REQUIRED arguments and set VORBIS_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(VORBIS DEFAULT_MSG VORBIS_LIBRARY VORBIS_INCLUDE_DIR)

if(VORBIS_FOUND)
  set(VORBIS_LIBRARIES ${VORBIS_LIBRARY})
endif()

mark_as_advanced(VORBIS_LIBRARY VORBIS_INCLUDE_DIR)
