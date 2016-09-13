# Locate VORBISFILE libraries ()
# This module defines
# VORBISFILE_LIBRARY : list of lib names
# VORBISFILE_FOUND : if false, do not try to link to VORBISFILE
# VORBISFILE_INCLUDE_DIR : where to find the headers
#
# $VORBISFILE_DIR is an environment variable that would
# correspond to the ./configure --prefix=$VORBISFILE_DIR
# used in building VORBISFILE.
#
# Created by Joe Thompson

find_path(VORBISFILE_INCLUDE_DIR vorbis/vorbisfile.h)

set(VORBISFILE_NAMES ${VORBISFILE_NAMES} vorbisfile)
find_library(VORBISFILE_LIBRARY NAMES ${VORBISFILE_NAMES})

# handle the QUIETLY and REQUIRED arguments and set VORBISFILE_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(VORBISFILE DEFAULT_MSG VORBISFILE_LIBRARY VORBISFILE_INCLUDE_DIR)

if(VORBISFILE_FOUND)
  set(VORBISFILE_LIBRARIES ${VORBISFILE_LIBRARY})
endif()

mark_as_advanced(VORBISFILE_LIBRARY VORBISFILE_INCLUDE_DIR)
