##############################################################################
#
#    file                 : Makefile
#    created              : Do 8. Feb 12:48:50 CET 2018
#    copyright            : (C) 2002 Alexander Weidinger
#
##############################################################################
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
##############################################################################

ROBOT       = ACC
MODULE      = ${ROBOT}.so
MODULEDIR   = drivers/${ROBOT}
SOURCES     = ${ROBOT}.cpp

SHIPDIR     = drivers/${ROBOT}
SHIP        = ${ROBOT}.xml sc-lynx-220.rgb logo.rgb
SHIPSUBDIRS = 

PKGSUBDIRS  = ${SHIPSUBDIRS}
src-robots-ACC_PKGFILES = $(shell find * -maxdepth 0 -type f -print)
src-robots-ACC_PKGDIR   = ${PACKAGE}-${VERSION}/$(subst ${SPEED_DREAMS_BASE},,$(shell pwd))

include ${MAKE_DEFAULT}
