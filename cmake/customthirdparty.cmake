############################################################################
#
#   file        : FindCustom3rdParty.cmake
#   copyright   : (C) 2009 by Brian Gavin, 2012 Jean-Philippe Meuret
#   web         : www.speed-dreams.org 
#   version     : $Id: customthirdparty.cmake 6270 2015-11-23 19:44:40Z madbad $
#
############################################################################

############################################################################
#                                                                          #
#   This program is free software; you can redistribute it and/or modify   #
#   it under the terms of the GNU General Public License as published by   #
#   the Free Software Foundation; either version 2 of the License, or      #
#   (at your option) any later version.                                    #
#                                                                          #
############################################################################

# @file     Custom 3rdParty location handling for some Windows builds
#           (standard CMake Find<package> macros can't find it, or don't do
#            it the way we need, so we needed another solution).
#           Heavily based on OpenScenGraph cmake scripts.
# @author   Brian Gavin, Jean-Philippe Meuret
# @version  $Id: customthirdparty.cmake 6270 2015-11-23 19:44:40Z madbad $

################################################################################################
# Find a generic dependency, handling debug suffix
# All the parameters are required ; in case of lists or empty parameter, use "" when calling

MACRO(_FIND_3RDPARTY_DEPENDENCY DEP_NAME INCLUDE_FILE INCLUDE_SUBDIRS LIBRARY_NAMES SEARCH_PATH_LIST DEBUG_SUFFIX)

	#MESSAGE(STATUS "Searching for 3rd party dependency DEP_NAME='${DEP_NAME}' INCLUDE_FILE='${INCLUDE_FILE}' INCLUDE_SUBDIRS='${INCLUDE_SUBDIRS}' LIBRARY_NAMES='${LIBRARY_NAMES}' SEARCH_PATH_LIST='${SEARCH_PATH_LIST}' DEBUG_SUFFIX='${DEBUG_SUFFIX}' ...")

	# Convert possibly a simple string to a real list.
	SET(_INCLUDE_SUBDIRS)
	LIST(APPEND _INCLUDE_SUBDIRS ${INCLUDE_SUBDIRS})
	LIST(LENGTH _INCLUDE_SUBDIRS _NB_DIRS)
	#MESSAGE(STATUS "_INCLUDE_SUBDIRS=${_INCLUDE_SUBDIRS}, _NB_DIRS=${_NB_DIRS}")
	
	# Find include dirs
	SET(MY_PATH_INCLUDE )
	FOREACH(MY_PATH ${SEARCH_PATH_LIST} )
		IF(${_NB_DIRS} GREATER 0)
			FOREACH(MY_SUBDIR ${_INCLUDE_SUBDIRS} )
				#MESSAGE(STATUS "MY_PATH='${MY_PATH}', MY_SUBDIR='${MY_SUBDIR}'")
				IF(NOT "${MY_SUBDIR}" STREQUAL ".")
					SET(MY_SUBDIR "/${MY_SUBDIR}")
				ENDIF(NOT "${MY_SUBDIR}" STREQUAL ".")
				SET(MY_PATH_INCLUDE ${MY_PATH_INCLUDE} ${MY_PATH}/include${MY_SUBDIR})
			ENDFOREACH(MY_SUBDIR ${_INCLUDE_SUBDIRS} )
		ELSE(${_NB_DIRS} GREATER 0)
			SET(MY_PATH_INCLUDE ${MY_PATH_INCLUDE} ${MY_PATH}/include)
		ENDIF(${_NB_DIRS} GREATER 0)
	ENDFOREACH(MY_PATH ${SEARCH_PATH_LIST} )
	
	#MESSAGE(STATUS "MY_PATH_INCLUDE='${MY_PATH_INCLUDE}'")
	FIND_PATH("${DEP_NAME}_INCLUDE_DIR" ${INCLUDE_FILE}
	  ${MY_PATH_INCLUDE}
	  NO_DEFAULT_PATH
	)
	MARK_AS_ADVANCED("${DEP_NAME}_INCLUDE_DIR")
	#MESSAGE(STATUS "${DEP_NAME}_INCLUDE_DIR = '${${DEP_NAME}_INCLUDE_DIR}'")
	
	# Find library files
	SET(MY_PATH_LIB )
	FOREACH(MY_PATH ${SEARCH_PATH_LIST} )
		SET(MY_PATH_LIB ${MY_PATH_LIB} ${MY_PATH}/lib)
	ENDFOREACH(MY_PATH ${SEARCH_PATH_LIST} )
	
	#MESSAGE(STATUS "LIBRARY_NAMES='${LIBRARY_NAMES}', MY_PATH_LIB=${MY_PATH_LIB}")
	FIND_LIBRARY("${DEP_NAME}_LIBRARY" 
	  NAMES ${LIBRARY_NAMES}
	  PATHS ${MY_PATH_LIB}
	  NO_DEFAULT_PATH
	)
	MARK_AS_ADVANCED("${DEP_NAME}_LIBRARY")
	#MESSAGE(STATUS " ${DEP_NAME}_LIBRARY = '${${DEP_NAME}_LIBRARY}'")

	# Whatever happened, done.
	SET(${DEP_NAME}_FOUND "NO" )
	IF(${DEP_NAME}_INCLUDE_DIR AND ${DEP_NAME}_LIBRARY)
	  SET( ${DEP_NAME}_FOUND "YES" )
	ENDIF(${DEP_NAME}_INCLUDE_DIR AND ${DEP_NAME}_LIBRARY)

ENDMACRO(_FIND_3RDPARTY_DEPENDENCY DEP_NAME INCLUDE_FILE INCLUDE_SUBDIRS LIBRARY_NAMES SEARCH_PATH_LIST DEBUG_SUFFIX)


MACRO(_FIND_3RDPARTY_DEPENDENCIES ROOT_DIR)

	# SDL.
	IF(OPTION_SDL2)
		_FIND_3RDPARTY_DEPENDENCY(SDL2MAIN sdl_main.h "SDL2" "sdl2main" "${ROOT_DIR}" "")
		_FIND_3RDPARTY_DEPENDENCY(SDL2 sdl.h "SDL2" "sdl2" "${ROOT_DIR}" "")
		
		#IF(SDL_FOUND) # Dirty hack to make FindPackage(SDL) work later.
		#	SET(SDL_LIBRARY_TEMP ${SDL_LIBRARY} CACHE FILEPATH "")
		#ENDIF(SDL_FOUND)
	ELSE()
		_FIND_3RDPARTY_DEPENDENCY(SDLMAIN sdl_main.h "SDL" "sdlmain" "${ROOT_DIR}" "")
		_FIND_3RDPARTY_DEPENDENCY(SDL sdl.h "SDL" "sdl" "${ROOT_DIR}" "")
	ENDIF()

	# PLib.
	_FIND_3RDPARTY_DEPENDENCY(PLIB plib/sg.h "" "sg;plibsg" ${ROOT_DIR} "")
	_FIND_3RDPARTY_DEPENDENCY(PLIB_SSG plib/ssg.h "" "ssg;plibssg" ${ROOT_DIR} "")
	_FIND_3RDPARTY_DEPENDENCY(PLIB_SG plib/sg.h "" "sg;plibsg" ${ROOT_DIR} "")
	_FIND_3RDPARTY_DEPENDENCY(PLIB_SL plib/sl.h "" "sl;plibsl" ${ROOT_DIR} "")
	_FIND_3RDPARTY_DEPENDENCY(PLIB_SSGAUX plib/ssgaux.h "" "ssgaux;plibssgaux" ${ROOT_DIR} "")
	_FIND_3RDPARTY_DEPENDENCY(PLIB_UL plib/ul.h "" "ul;plibul" ${ROOT_DIR} "")
	_FIND_3RDPARTY_DEPENDENCY(PLIB_JS plib/js.h "" "js;plibjs" ${ROOT_DIR} "")
	
	# Open GL : Note that the Open GL includes are automatically added by MSVC 2005.
	# We simply add here the include path for the Open GL extensions headers,
	# and we use OPENGL_INCLUDE_DIR variable for this,
	# as Find_Package(OpenGL) doesn't seem to set it.
	Find_Package(OpenGL)
	FIND_PATH(OPENGL_INCLUDE_DIR GL/glext.h ${ROOT_DIR}/include NO_DEFAULT_PATH)

	# Open AL.
	_FIND_3RDPARTY_DEPENDENCY(OPENAL AL/al.h "" openal32 ${ROOT_DIR} "")

	# Menu Music requires ogg, vorbis, and vorbisfile 
	# OGG.
	_FIND_3RDPARTY_DEPENDENCY(OGG ogg/ogg.h "" libogg ${ROOT_DIR} "")

	# Vorbis.
	_FIND_3RDPARTY_DEPENDENCY(VORBIS vorbis/vorbisfile.h "" libvorbis ${ROOT_DIR} "")

	# VorbisFile.
	_FIND_3RDPARTY_DEPENDENCY(VORBISFILE vorbis/vorbisfile.h "" libvorbisfile ${ROOT_DIR} "")
	
	# ENet.
	_FIND_3RDPARTY_DEPENDENCY(ENET enet/enet.h "" enet ${ROOT_DIR} "")

	# SQlite.
	IF(OPTION_3RDPARTY_SQLITE3)
		_FIND_3RDPARTY_DEPENDENCY(SQLITE3 sqlite3.h "" sqlite3 ${ROOT_DIR} "")
	ENDIF(OPTION_3RDPARTY_SQLITE3)
	
	# OpenSceneGraph
	IF(OPTION_OSGGRAPH)
	
		_FIND_3RDPARTY_DEPENDENCY(OPENTHREADS OpenThreads/Thread "" OpenThreads ${ROOT_DIR} "")
		_FIND_3RDPARTY_DEPENDENCY(OSGDB osgDB/fstream "" osgDB ${ROOT_DIR} "")
		_FIND_3RDPARTY_DEPENDENCY(OSGFX osgFX/version "" osgFX ${ROOT_DIR} "")
		_FIND_3RDPARTY_DEPENDENCY(OSGGA osgGA/export "" osgGA ${ROOT_DIR} "")
		_FIND_3RDPARTY_DEPENDENCY(OSG osg/viewport "" osg ${ROOT_DIR} "")
		_FIND_3RDPARTY_DEPENDENCY(OSGPARTICLE osgParticle/Particle "" osgParticle ${ROOT_DIR} "")
		_FIND_3RDPARTY_DEPENDENCY(OSGSHADOW osgShadow/ShadowedScene "" osgShadow ${ROOT_DIR} "")
		_FIND_3RDPARTY_DEPENDENCY(OSGVIEWER osgViewer/api/Win32/GraphicsHandleWin32 "" osgViewer ${ROOT_DIR} "")
		_FIND_3RDPARTY_DEPENDENCY(OSGUTIL osgUtil/Optimizer "" osgUtil ${ROOT_DIR} "")
		
		# If everything found, set things as if it was Find_Package(OpenSceneGraph) which had did it,
		# in order CHECK_LIBRARIES does not call it again.
		IF(OPENTHREADS_FOUND AND OSGDB_FOUND AND OSGFX_FOUND AND OSGGA_FOUND
		   AND OSG_FOUND AND OSGVIEWER_FOUND AND OSGUTIL_FOUND AND OSGPARTICLE_FOUND
		   AND OSGSHADOW_FOUND)

			SET(OPENSCENEGRAPH_FOUND "YES")
			
			SET(OPENSCENEGRAPH_INCLUDE_DIRS "${OSG_INCLUDE_DIR}") # We assume they are all together.
			SET(OPENSCENEGRAPH_LIBRARIES "${OPENTHREADS_LIBRARY};${OSGDB_LIBRARY};${OSGFX_LIBRARY}")
			SET(OPENSCENEGRAPH_LIBRARIES "${OPENSCENEGRAPH_LIBRARIES};${OSGGA_LIBRARY};${OSG_LIBRARY}")
			SET(OPENSCENEGRAPH_LIBRARIES "${OPENSCENEGRAPH_LIBRARIES};${OSGVIEWER_LIBRARY};${OSGUTIL_LIBRARY}")
			SET(OPENSCENEGRAPH_LIBRARIES "${OPENSCENEGRAPH_LIBRARIES};${OSGPARTICLE_LIBRARY};${OSGSHADOW_LIBRARY}")
			
			MESSAGE(STATUS "OPENSCENEGRAPH_INCLUDE_DIRS=${OPENSCENEGRAPH_INCLUDE_DIRS}")
			MESSAGE(STATUS "OPENSCENEGRAPH_LIBRARIES=${OPENSCENEGRAPH_LIBRARIES}")

		ENDIF()
		
	ENDIF(OPTION_OSGGRAPH)	
	
	# Expat : Replaces bundled libs/txml (that will soon be removed).
	IF(OPTION_3RDPARTY_EXPAT)
		_FIND_3RDPARTY_DEPENDENCY(EXPAT expat.h "" "expat;expat-1" ${ROOT_DIR} "")
	ENDIF(OPTION_3RDPARTY_EXPAT)
	
	# FreeSOLID : Replaces bundled modules/simu/.../SOLID2.0 (that will soon be removed).
	IF(OPTION_3RDPARTY_SOLID)

		_FIND_3RDPARTY_DEPENDENCY(SOLID SOLID/solid.h ".;FreeSOLID" "solid2;solid;broad" ${ROOT_DIR} "")
		_FIND_3RDPARTY_DEPENDENCY(SOLID_SOLID SOLID/solid.h ".;FreeSOLID" "solid2;solid" ${ROOT_DIR} "")
		_FIND_3RDPARTY_DEPENDENCY(SOLID_BROAD SOLID/broad.h ".;FreeSOLID" "broad" ${ROOT_DIR} "")

	ENDIF(OPTION_3RDPARTY_SOLID)
	
	# JPEG.
	_FIND_3RDPARTY_DEPENDENCY(JPEG jpeglib.h "" "jpeg_s;jpeg;jpeg-8;jpeg-9" ${ROOT_DIR} "")

	IF(OPTION_WEBSERVER)
		# CURL.
		_FIND_3RDPARTY_DEPENDENCY(CURL curl/curl.h "" "libcurl;libcurl_imp" ${ROOT_DIR} "")	
	ENDIF(OPTION_WEBSERVER)

	# ZLib.
	_FIND_3RDPARTY_DEPENDENCY(ZLIB zlib.h "" "z;zlib;zlib1" ${ROOT_DIR} "D")
	
	IF(ZLIB_FOUND)

		# PNG.	
		_FIND_3RDPARTY_DEPENDENCY(PNG png.h "" "libpng;png13;png14;png15;png16" ${ROOT_DIR} "D")
		
		IF(PNG_FOUND)
			# Force subsequent FindPNG stuff not to search for other variables ... kind of a hack 
			SET(PNG_PNG_INCLUDE_DIR ${PNG_INCLUDE_DIR} CACHE FILEPATH "")
			MARK_AS_ADVANCED(PNG_PNG_INCLUDE_DIR)
		ENDIF(PNG_FOUND)
		
	ENDIF(ZLIB_FOUND)
	
ENDMACRO(_FIND_3RDPARTY_DEPENDENCIES ROOT_DIR)

################################################################################################
# Handling of optional 3rd party package (usefull only when building under Windows)

MACRO(SD_FIND_CUSTOM_3RDPARTY)

	MESSAGE(STATUS "Using custom 3rd party libs location ...")

	GET_FILENAME_COMPONENT(PARENT_DIR ${PROJECT_SOURCE_DIR} PATH)
	SET(SDEXT_CUSTOM_3DPARTY_DIR "${PARENT_DIR}/3rdparty" CACHE PATH 
	    "Location of 3rdParty dependencies")
	IF(EXISTS ${SDEXT_CUSTOM_3DPARTY_DIR})
		_FIND_3RDPARTY_DEPENDENCIES(${SDEXT_CUSTOM_3DPARTY_DIR})
	ELSE(EXISTS ${SDEXT_CUSTOM_3DPARTY_DIR})
		MESSAGE(STATUS "... but it doesn't exist : ${SDEXT_CUSTOM_3DPARTY_DIR}")
	ENDIF(EXISTS ${SDEXT_CUSTOM_3DPARTY_DIR})

	MARK_AS_ADVANCED(SDEXT_CUSTOM_3DPARTY_DIR)

ENDMACRO(SD_FIND_CUSTOM_3RDPARTY)

################################################################################################
# Under Windows, install needed 3rd party DLLs close to Speed Dreams executable
# (but stay compatible with the old 2.0.0 3rd party package which had less DLLs inside)

# Find the full path-name of the 3rd party DLL corresponding to the given 3rd party link library
#
# Parameters :
# * LIB_PATH_NAMES : The link library (or list of link libraries) path-name.
# * LIB_NAME_HINTS : Hints for retrieving in LIB_PATH_NAMES the only lib we are taking care of,
#                    and for retrieving on disk the corresponding DLL.
# * DLL_NAME_PREFIXES : Possible prefixes (to the lib name hint) for retrieving the DLL.
#                       Note: the empty "" prefix is always tried at the end.
#                       Ex: "lib;xx" for "lib" and "xx" prefixes.
# * DLL_PATHNAME_VAR : Name of the output variable for the retrieved DLL path-name.

MACRO(_FIND_3RDPARTY_DLL LIB_PATH_NAMES LIB_NAME_HINTS DLL_NAME_PREFIXES DLL_PATHNAME_VAR)

	FOREACH(_LIB_NAME_HINT ${LIB_NAME_HINTS})

		# Must handle the case of multiple libs listed in ${LIB_PATH_NAMES} :
		# Use LIB_NAME_HINTS to retrieve the one we are interested in here.
		SET(_LIB_PATHNAME ${LIB_PATH_NAMES})
		FOREACH(_LIB_PATHNAME_ ${LIB_PATH_NAMES})
			IF(${_LIB_PATHNAME_} MATCHES "${_LIB_NAME_HINT}\\.")
				SET(_LIB_PATHNAME ${_LIB_PATHNAME_})
				BREAK()
			ENDIF(${_LIB_PATHNAME_} MATCHES "${_LIB_NAME_HINT}\\.")
		ENDFOREACH(_LIB_PATHNAME_ ${LIB_PATH_NAMES})

		# Got the link library pathname : check if any corresponding DLL around (try all prefixes).
		# 1) Check the empty prefix
		#    (CMake ignores it when specified at the beginning of DLL_NAME_PREFIXES ... bull shit).
		GET_FILENAME_COMPONENT(_LIB_PATH "${_LIB_PATHNAME}" PATH)
		SET(${DLL_PATHNAME_VAR} "${_LIB_PATH}/../bin/${_LIB_NAME_HINT}${CMAKE_SHARED_LIBRARY_SUFFIX}")
		#MESSAGE(STATUS "Trying 3rdParty DLL ${${DLL_PATHNAME_VAR}}")
		IF(EXISTS "${${DLL_PATHNAME_VAR}}")
			MESSAGE(STATUS "Will install 3rdParty DLL ${${DLL_PATHNAME_VAR}}")
			BREAK() # First found is the one.
		ELSE(EXISTS "${${DLL_PATHNAME_VAR}}")
			UNSET(${DLL_PATHNAME_VAR})
		ENDIF(EXISTS "${${DLL_PATHNAME_VAR}}")

		# 2) Check other (specified) prefixes.
		FOREACH(_DLL_NAME_PREFIX ${DLL_NAME_PREFIXES})
			SET(${DLL_PATHNAME_VAR} "${_LIB_PATH}/../bin/${_DLL_NAME_PREFIX}${_LIB_NAME_HINT}${CMAKE_SHARED_LIBRARY_SUFFIX}")
			#MESSAGE(STATUS "Trying 3rdParty DLL ${${DLL_PATHNAME_VAR}}")
			IF(EXISTS "${${DLL_PATHNAME_VAR}}")
				BREAK() # First found is the one.
			ELSE(EXISTS "${${DLL_PATHNAME_VAR}}")
				UNSET(${DLL_PATHNAME_VAR})
			ENDIF(EXISTS "${${DLL_PATHNAME_VAR}}")
		ENDFOREACH(_DLL_NAME_PREFIX ${DLL_NAME_PREFIXES})

		IF(EXISTS "${${DLL_PATHNAME_VAR}}")
			MESSAGE(STATUS "Will install 3rdParty DLL ${${DLL_PATHNAME_VAR}}")
			BREAK() # First found is the one.
		ELSE(EXISTS "${${DLL_PATHNAME_VAR}}")
			UNSET(${DLL_PATHNAME_VAR})
		ENDIF(EXISTS "${${DLL_PATHNAME_VAR}}")

	ENDFOREACH(_LIB_NAME_HINT ${LIB_NAME_HINTS})

	#IF(NOT EXISTS "${${DLL_PATHNAME_VAR}}")
	#	MESSAGE(STATUS "Could not find 3rdParty DLL for lib ${LIB_NAME_HINTS} (prefixes ${DLL_NAME_PREFIXES})")
	#ENDIF()

ENDMACRO(_FIND_3RDPARTY_DLL DLL_PATHNAME)

MACRO(SD_INSTALL_CUSTOM_3RDPARTY TARGET_NAME)

	# 1) Find 3rd party DLL files to install.
	SET(_THIRDPARTY_DLL_PATHNAMES)

	_FIND_3RDPARTY_DLL("${OPENAL_LIBRARY}" "OpenAL32" "lib" _DLL_PATHNAME)
	LIST(APPEND _THIRDPARTY_DLL_PATHNAMES "${_DLL_PATHNAME}")

	# Menu Music requires ogg, vorbis, and vorbisfile 
	_FIND_3RDPARTY_DLL("${OGG_LIBRARY}" "libogg;libogg-0" "" _DLL_PATHNAME)
	LIST(APPEND _THIRDPARTY_DLL_PATHNAMES "${_DLL_PATHNAME}")

	_FIND_3RDPARTY_DLL("${VORBIS_LIBRARY}" "libvorbis;libvorbis-0" "" _DLL_PATHNAME)
	LIST(APPEND _THIRDPARTY_DLL_PATHNAMES "${_DLL_PATHNAME}")

	_FIND_3RDPARTY_DLL("${VORBISFILE_LIBRARY}" "libvorbisfile;libvorbisfile-3" "" _DLL_PATHNAME)
	LIST(APPEND _THIRDPARTY_DLL_PATHNAMES "${_DLL_PATHNAME}")


	IF(OPTION_SDL2)
		_FIND_3RDPARTY_DLL("${SDL2_LIBRARY}" "SDL2" ";lib" _DLL_PATHNAME)
		LIST(APPEND _THIRDPARTY_DLL_PATHNAMES "${_DLL_PATHNAME}")
	ELSE()
		_FIND_3RDPARTY_DLL("${SDL_LIBRARY}" "SDL" ";lib" _DLL_PATHNAME)
		LIST(APPEND _THIRDPARTY_DLL_PATHNAMES "${_DLL_PATHNAME}")
	ENDIF()

	IF(OPTION_3RDPARTY_EXPAT)

		_FIND_3RDPARTY_DLL("${EXPAT_LIBRARY}" "expat;expat-1" "lib" _DLL_PATHNAME)
		LIST(APPEND _THIRDPARTY_DLL_PATHNAMES "${_DLL_PATHNAME}")

	ENDIF(OPTION_3RDPARTY_EXPAT)
	
	IF(OPTION_OSGGRAPH)

		# DLLs whose libs we link with.
		SET(_OSG_DLLS_NAME_HINTS "OpenThreads;osgDB;osgFX;osgGA;osgParticle;osgShadow;osgViewer;osgUtil;osg;")
		FOREACH(_LIB_NAME ${OPENSCENEGRAPH_LIBRARIES})
			FOREACH(_NAME_HINT ${_OSG_DLLS_NAME_HINTS})
				IF("${_LIB_NAME}" MATCHES "${_NAME_HINT}\\.")
					_FIND_3RDPARTY_DLL("${_LIB_NAME}" "${_NAME_HINT}" "lib;ot12-;ot20-;osg80-;osg97-;osg100-;osg118-;osg123-;osg130-" _DLL_PATHNAME)
					SET(_NAME_HINT_ "${_NAME_HINT}") # For later (see below DLLs we don't link with).
					SET(_LIB_NAME_ "${_LIB_NAME}") # For later (see below DLLs we don't link with).
					SET(_DLL_PATHNAME_ "${_DLL_PATHNAME}") # For later (see below plugins).
					BREAK()
				ENDIF()
			ENDFOREACH()
			LIST(APPEND _THIRDPARTY_DLL_PATHNAMES "${_DLL_PATHNAME}")
		ENDFOREACH()

		# Other needed DLLs we don't link with.
		# We use _LIB_NAME_ as a template, and _NAME_HINT_ as the string to replace inside. 
		SET(_EXTRA_OSG_DLLS_NAME_HINTS "osgText") # ';'-separated list
		FOREACH(_NAME_HINT ${_EXTRA_OSG_DLLS_NAME_HINTS})
			STRING(REPLACE "${_NAME_HINT_}" "${_NAME_HINT}" _LIB_NAME "${_LIB_NAME_}")
			_FIND_3RDPARTY_DLL("${_LIB_NAME}" "${_NAME_HINT}" ";lib;ot12-;ot20-;osg80-;osg97-;osg100-;osg118-;osg123-;osg130-" _DLL_PATHNAME)
			LIST(APPEND _THIRDPARTY_DLL_PATHNAMES "${_DLL_PATHNAME}")
		ENDFOREACH()
		
		# Plugins : Complete the list right below according to the actual needs.
		# TODO: Find a way to install them in the osgPlugins-xxx subdir (works as is, but ...)
		SET(_OSG_PLUGIN_NAME_HINTS "osgdb_ac;osgdb_dds;osgdb_glsl") # ';'-separated list
      LIST(APPEND _OSG_PLUGIN_NAME_HINTS "osgdb_ive;osgdb_jpeg;osgdb_osg;osgdb_curl")
      LIST(APPEND _OSG_PLUGIN_NAME_HINTS "osgdb_osga;osgdb_osgshadow;osgdb_osgtgz;osgdb_png;osgdb_rgb")
      LIST(APPEND _OSG_PLUGIN_NAME_HINTS "osgdb_serializers_osg;osgdb_serializers_osganimation;osgdb_serializers_osgfx;osgdb_serializers_osgga;osgdb_serializers_osgmanipulator;osgdb_serializers_osgparticle")
	  LIST(APPEND _OSG_PLUGIN_NAME_HINTS "osgdb_serializers_osgshadow;osgdb_serializers_osgsim;osgdb_serializers_osgtext;osgdb_serializers_osggui;osgdb_serializers_osgutil;osgdb_serializers_osgviewer")
	  LIST(APPEND _OSG_PLUGIN_NAME_HINTS "osgdb_deprecated_osg;osgdb_deprecated_osganimation;osgdb_deprecated_osgfx;osgdb_deprecated_osgparticle;osgdb_deprecated_osgshadow")
	  LIST(APPEND _OSG_PLUGIN_NAME_HINTS "osgdb_deprecated_osgsim;osgdb_deprecated_osgtext;osgdb_deprecated_osgwidget;osgdb_deprecated_osgviewer")

		GET_FILENAME_COMPONENT(_OSG_PLUGINS_DIR "${_DLL_PATHNAME_}" PATH)
		FILE(GLOB_RECURSE _OSG_PLUGIN_NAMES "${_OSG_PLUGINS_DIR}/*${CMAKE_SHARED_LIBRARY_SUFFIX}")
		FOREACH(_NAME_HINT ${_OSG_PLUGIN_NAME_HINTS})
			FOREACH(_PLUGIN_NAME ${_OSG_PLUGIN_NAMES})
				IF("${_PLUGIN_NAME}" MATCHES "osgPlugins.*/.*${_NAME_HINT}\\.")
					LIST(APPEND _THIRDPARTY_DLL_PATHNAMES "${_PLUGIN_NAME}")
					MESSAGE(STATUS "Will install 3rdParty plugin ${_PLUGIN_NAME}")
					BREAK()
				ENDIF()
			ENDFOREACH()
		ENDFOREACH()
		
	ENDIF(OPTION_OSGGRAPH)

	IF(OPTION_3RDPARTY_SOLID)
		_FIND_3RDPARTY_DLL("${SOLID_SOLID_LIBRARY}" "solid2;solid" "lib" _DLL_PATHNAME)
		LIST(APPEND _THIRDPARTY_DLL_PATHNAMES "${_DLL_PATHNAME}")

		IF(SOLID_BROAD_LIBRARY)
			_FIND_3RDPARTY_DLL("${SOLID_BROAD_LIBRARY}" "broad" "lib" _DLL_PATHNAME)
			LIST(APPEND _THIRDPARTY_DLL_PATHNAMES "${_DLL_PATHNAME}")
		ENDIF(SOLID_BROAD_LIBRARY)

	ENDIF(OPTION_3RDPARTY_SOLID)

	_FIND_3RDPARTY_DLL("${ZLIB_LIBRARY}" "zlib;zlib1" "lib" _DLL_PATHNAME)
	LIST(APPEND _THIRDPARTY_DLL_PATHNAMES "${_DLL_PATHNAME}")

	_FIND_3RDPARTY_DLL("${PNG_LIBRARY}" "png;png15;png16" "lib" _DLL_PATHNAME)
	LIST(APPEND _THIRDPARTY_DLL_PATHNAMES "${_DLL_PATHNAME}")

	_FIND_3RDPARTY_DLL("${JPEG_LIBRARY}" "jpeg;jpeg-8;jpeg-9" "lib" _DLL_PATHNAME)
	LIST(APPEND _THIRDPARTY_DLL_PATHNAMES "${_DLL_PATHNAME}")

	IF(OPTION_WEBSERVER)
		_FIND_3RDPARTY_DLL("${CURL_LIBRARY}" "curl" "lib" _DLL_PATHNAME)
		LIST(APPEND _THIRDPARTY_DLL_PATHNAMES "${_DLL_PATHNAME}")
	ENDIF(OPTION_WEBSERVER)

	# 2) Copy found 3rd party DLL files to the bin folder (for running without installing).
	#MESSAGE(STATUS "3rdParty dependencies : Will install ${_THIRDPARTY_DLL_PATHNAMES}")
    SET(_NOINST_DIR "${CMAKE_BINARY_DIR}/${SD_BINDIR}")
    ADD_CUSTOM_COMMAND(TARGET ${TARGET_NAME} POST_BUILD
                       COMMAND ${CMAKE_COMMAND} -E make_directory "${_NOINST_DIR}"
                       VERBATIM)
    FOREACH(_DLL ${_THIRDPARTY_DLL_PATHNAMES})
      ADD_CUSTOM_COMMAND(TARGET ${TARGET_NAME} POST_BUILD
                         COMMAND ${CMAKE_COMMAND} -E echo Copying "${_DLL}" to "${_NOINST_DIR}"
                         COMMAND ${CMAKE_COMMAND} -E copy "${_DLL}" "${_NOINST_DIR}"
                         VERBATIM)
    ENDFOREACH()

	# 3) Install found 3rd party DLL files to the install folder.
	SD_INSTALL_FILES(BIN FILES ${_THIRDPARTY_DLL_PATHNAMES})

	# 4) Find Windows compilers run-time DLLs.
	IF(MSVC)

		# We do it ourselves, but use InstallRequiredSystemLibraries to figure out which ones.
		SET(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP TRUE)
		INCLUDE(InstallRequiredSystemLibraries)
		SET(_COMPILER_DLL_PATHNAMES "${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS}")

	ELSEIF(MINGW)

		# Works with MinGW 4.4 and 4.7.
		GET_FILENAME_COMPONENT(_MINGW_BINDIR "${CMAKE_CXX_COMPILER}" PATH)
		SET(_COMPILER_DLL_PATHNAMES "${_MINGW_BINDIR}/libstdc++-6.dll;${_MINGW_BINDIR}/libgcc_s_dw2-1.dll")

	ENDIF(MSVC)

	# 5) Copy found compiler DLL files to the bin folder (for running without installing).
	FOREACH(_DLL ${_COMPILER_DLL_PATHNAMES})
		ADD_CUSTOM_COMMAND(TARGET ${TARGET_NAME} POST_BUILD
                 		   COMMAND ${CMAKE_COMMAND} -E echo Copying "${_DLL}" to "${_NOINST_DIR}"
                 		   COMMAND ${CMAKE_COMMAND} -E copy "${_DLL}" "${_NOINST_DIR}"
                 		   VERBATIM)
	ENDFOREACH()

	# 6) Install found compiler DLL files to the install folder.
	SD_INSTALL_FILES(BIN FILES ${_COMPILER_DLL_PATHNAMES})

ENDMACRO(SD_INSTALL_CUSTOM_3RDPARTY TARGET_NAME)

