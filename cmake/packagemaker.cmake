##########################################################################################
# Settings that are common to all target systems.

SET(INTERNAL_NAME "speed-dreams")

SET(CPACK_PACKAGE_NAME "Speed Dreams")
SET(CPACK_PACKAGE_VENDOR "the Speed Dreams team")
SET(CPACK_PACKAGE_CONTACT "http:\\\\\\\\www.speed-dreams.org")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Speed Dreams (an Open Motorsport Sim) is a racing simulation that allows you to drive in races against opponents simulated by the computer ; it is GPL 2+ and has been forked from TORCS in late 2008")
SET(CPACK_RESOURCE_FILE_LICENSE "${SOURCE_DIR}/COPYING.txt")
SET(CPACK_RESOURCE_FILE_README "${SOURCE_DIR}/README.txt")

SET(EXECUTABLE_NAME "${INTERNAL_NAME}")
SET(CPACK_PACKAGE_EXECUTABLES "${EXECUTABLE_NAME};Start ${CPACK_PACKAGE_NAME}")

# Version settings.
# * the short way.
SET(CPACK_PACKAGE_VERSION "${VERSION_LONG}")

# * another way.
#SET(CPACK_PACKAGE_VERSION_MAJOR "2")
#SET(CPACK_PACKAGE_VERSION_MINOR "0")
#SET(CPACK_PACKAGE_VERSION_PATCH "0")
#SET(CPACK_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}-alpha")
#IF(NOT SVN_FIND_REV_FAILED)
#    SET(CPACK_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION}-r${SVN_REV}")
#ENDIF(NOT SVN_FIND_REV_FAILED)

# Binary package settings.
SET(PACKAGE_FILE_PREFIX "${INTERNAL_NAME}")
#SET(CPACK_OUTPUT_CONFIG_FILE "/home/andy/vtk/CMake-bin/CPackConfig.cmake")
#SET(CPACK_PACKAGE_DESCRIPTION_FILE "/home/andy/vtk/CMake/Copyright.txt")

#SET(CPACK_IGNORE_FILES "/\\.svn/;\\.swp$;\\.#;/#;${CPACK_IGNORE_FILES}")

# Source package settings.
#SET(CPACK_SOURCE_OUTPUT_CONFIG_FILE "/home/andy/vtk/CMake-bin/CPackSourceConfig.cmake")
SET(CPACK_SOURCE_PACKAGE_FILE_NAME "${PACKAGE_FILE_PREFIX}-${CPACK_PACKAGE_VERSION}-src")

#SET(CPACK_RESOURCE_FILE_LICENSE "/home/andy/vtk/CMake/Copyright.txt")
#SET(CPACK_RESOURCE_FILE_README "/home/andy/vtk/CMake/Templates/CPack.GenericDescription.txt")
#SET(CPACK_RESOURCE_FILE_WELCOME "/home/andy/vtk/CMake/Templates/CPack.GenericWelcome.txt")

SET(CPACK_SOURCE_IGNORE_FILES
    "/installer/" "/doc/design/" "/doc/develdoc" "/doc/website/" "/_CPack_Packages/" 
    "/CMakeCache\\\\.txt$" "/install_manifest\\\\.txt$" "/xmlversion_loc\\\\.txt$" 
    "/config\\\\.h$" "/version\\\\.h$" "/doxygen_config$"
    "/\\\\.svn/" "/\\\\.dir/" "/CMakeFiles/" 
    "cmake_install\\\\.cmake$" "CPackConfig\\\\.cmake$" "CPackSourceConfig\\\\.cmake$"
    "\\\\.bak$" "\\\\.flc$" "#.*#$" "~$" "\\\\.~.*"
    "\\\\.xcf$" "\\\\.xcf\\\\.bz2$" "\\\\.psd$" 
    "\\\\.exe$" "/sd2-.*$" "/speed-dreams-2$" "/xmlversion$" 
    "\\\\.zip$" "\\\\.tar\\\\.bz2$" "\\\\.tar\\\\.gz$" "\\\\.tar\\\\.Z$" "\\\\.tar\\\\.7z$")

##########################################################################################
# Put Linux install information here
IF(UNIX)

    SET(PACKAGERS_BINARY "DEB" CACHE STRING "CPack binary package generators to use (separated with ';', among DEB, RPM, STGZ, TGZ, TBZ2, TZ, ZIP)")
    MARK_AS_ADVANCED(PACKAGERS_BINARY)
    SET(PACKAGERS_SOURCE "TBZ2" CACHE STRING "CPack source package generators to use (separated with ';', among TGZ, TBZ2, TZ, ZIP)")
    MARK_AS_ADVANCED(PACKAGERS_SOURCE)

    # 9.10 ubuntu depends
    SET(CPACK_DEBIAN_PACKAGE_DEPENDS "freeglut3,libc6(>=2.7),libgcc1(>=1:4.1.1),libgl1-mesa-glx | libgl1,libglu1-mesa | libglu1,libice6(>=1:1.0.0),libopenal1(>=1:1.3.253),libpng12-0(>=1.2.13-4),libsm6,libstdc++6(>=4.2.1),libx11-6,libxext6,libxi6(>=2:1.1.3-1ubuntu1),libxmu6,libxrandr2,libxrender1,libxt6,libxxf86vm1,plib1.8.4c2(>=1.2.4),zlib1g(>=1:1.1.4)")

    # Put other Debian-based distros settings here.

    # Source package specific settings.
    LIST(APPEND CPACK_SOURCE_IGNORE_FILES "Makefile$" "\\\\.so$")

ENDIF(UNIX)

##########################################################################################
# Put Windows install information here.
# (NSIS must be installed on your computer for this to work)
IF(WIN32)

    # General note: There is a bug in NSI that does not handle full unix paths properly.
    # Make sure there is at least one set of four (4) backlasshes.

    SET(CPACK_PACKAGE_INSTALL_DIRECTORY "${INTERNAL_NAME}-${VERSION_LONG}")

    SET(EXECUTABLE_PATHNAME "$INSTDIR\\\\bin\\\\${EXECUTABLE_NAME}.exe")

    SET(PACKAGERS_BINARY "NSIS" CACHE STRING "CPack binary package generators to use (separated with ';', among NSIS, CygwinBinary, STGZ, TGZ, TBZ2, TZ, ZIP)")
    MARK_AS_ADVANCED(PACKAGERS_BINARY)
    SET(PACKAGERS_SOURCE "ZIP" CACHE STRING "CPack source package generators to use (separated with ';', among TGZ, TBZ2, TZ, ZIP)")
    MARK_AS_ADVANCED(PACKAGERS_SOURCE)

    SET(CPACK_PACKAGE_FILE_NAME "${PACKAGE_FILE_PREFIX}-${CPACK_PACKAGE_VERSION}-win32-setup")
    SET(CPACK_NSIS_DISPLAY_NAME "${CPACK_PACKAGE_NAME} ${CPACK_PACKAGE_VERSION}")
    SET(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "${CPACK_NSIS_DISPLAY_NAME}")

    # Icon for the generated installer/uninstaller files.
    SET(CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}\\\\data\\\\data\\\\icons\\\\icon.ico")
    #SET(CPACK_NSIS_MUI_UNIICON "${CMAKE_SOURCE_DIR}\\\\data\\\\data\\\\icons\\\\icon.ico")
    SET(CPACK_PACKAGE_ICON ${CMAKE_SOURCE_DIR}\\\\data\\\\data\\\\img\\\\header.bmp)
    
    # Extra shortcuts to add in the start menu (a list of pairs : URL, Menu label).
    SET(CPACK_NSIS_MENU_LINKS 
        "${CPACK_PACKAGE_CONTACT}" "Project Homepage")
        #"$INSTDIR\\\\share\\\\doc\\\\userman\\\\how_to_drive.html" "User manual")

    # Icon in the add/remove control panel. Must be an .exe file 
    Set(CPACK_NSIS_INSTALLED_ICON_NAME "${EXECUTABLE_PATHNAME}")

    SET(CPACK_NSIS_URL_INFO_ABOUT "${CPACK_PACKAGE_CONTACT}")
    SET(CPACK_NSIS_HELP_LINK "${CPACK_PACKAGE_CONTACT}")
    
    # Add a page in the install wizard for options :
    # - adding the installation path in the PATH,
    # - adding a shortcut to start the installed app on the desktop.
    #SET(CPACK_NSIS_MODIFY_PATH "ON")
    
    # Another way to add a shortcut to start the installed app on the desktop :
    # This doesn't work.
    #SET(CPACK_CREATE_DESKTOP_LINKS "${EXECUTABLE_NAME}")
    
    # But this works.
    SET(SHORTCUT_TARGET "$DESKTOP\\\\${CPACK_PACKAGE_NAME} ${CPACK_PACKAGE_VERSION}.lnk")
    SET(CPACK_NSIS_EXTRA_INSTALL_COMMANDS
        "CreateShortCut \\\"${SHORTCUT_TARGET}\\\" \\\"${EXECUTABLE_PATHNAME}\\\"")
    SET(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "Delete \\\"${SHORTCUT_TARGET}\\\"")

    # Source package specific settings.
    LIST(APPEND CPACK_SOURCE_IGNORE_FILES 
                "/VTune/"
                "/Release/" "/Debug/" "/RelWithDebInfo/" "/MinSizeRel/"
                "/release/" "/debug/" "/relwithdebinfo/" "/minsizerel/"
                "\\\\.sln$" "\\\\.suo$" "\\\\.ncb$" "\\\\.vcproj*$" "\\\\.dll$")

    # Add the PACKAGE_SRC project in the MSVC solution
    # (CMake 2.6 and 2.8 fail to do this itself).
    ADD_CUSTOM_TARGET(PACKAGE_SRC)
    ADD_CUSTOM_COMMAND(TARGET PACKAGE_SRC
                       COMMAND ${CMAKE_CPACK_COMMAND} -C $(OutDir) --config ./CPackSourceConfig.cmake)

ENDIF(WIN32)

##########################################################################################
# Put Mac OS X install information here
IF(APPLE)

    SET(PACKAGERS_BINARY "DragNDrop" CACHE STRING "CPack binary package generators to use (separated with ';', among Bundle, DragNDrop, PackageMaker, OSXX11, STGZ, TGZ, TBZ2, TZ, ZIP)")
    MARK_AS_ADVANCED(PACKAGERS_BINARY)
    SET(PACKAGERS_SOURCE "TBZ2" CACHE STRING "CPack source package generators to use (separated with ';', among TGZ, TBZ2, TZ, ZIP)")
    MARK_AS_ADVANCED(PACKAGERS_SOURCE)

    # Source package specific settings.
    LIST(APPEND CPACK_SOURCE_IGNORE_FILES "Makefile$")

ENDIF(APPLE)

##########################################################################################
# Final settings.
SET(CPACK_GENERATOR ${PACKAGERS_BINARY})
SET(CPACK_SOURCE_GENERATOR ${PACKAGERS_SOURCE})

INCLUDE(CPack)
