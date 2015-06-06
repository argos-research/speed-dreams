/***************************************************************************

    file                 : OsgReferenced.h
    created              : Sun Jan 20 10:24:02 CEST 2013
    copyright            : (C) 2013 by Xavier Bertaux
    email                : bertauxx@yahoo.fr
    version              : $Id$

***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef OsgReferenced_H
#define OsgReferenced_H

#include "OsgAtomic.h"

class SDReferenced
{
public:
  SDReferenced(void) : _refcount(0u)
  {}
  /// Do not copy reference counts. Each new object has it's own counter
  SDReferenced(const SDReferenced&) : _refcount(0u)
  {}
  /// Do not copy reference counts. Each object has it's own counter
  SDReferenced& operator=(const SDReferenced&)
  { return *this; }

  static unsigned get(const SDReferenced* ref)
  { if (ref) return ++(ref->_refcount); else return ~0u; }
  static unsigned put(const SDReferenced* ref)
  { if (ref) return --(ref->_refcount); else return ~0u; }
  static unsigned count(const SDReferenced* ref)
  { if (ref) return ref->_refcount; else return ~0u; }
  static bool shared(const SDReferenced* ref)
  { if (ref) return 1u < ref->_refcount; else return false; }

private:
  mutable SDAtomic _refcount;
};

#endif
