/***************************************************************************

    file                 : OsgSharedPtr.h
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

#ifndef OsgSharedPtr_H
#define OsgSharedPtr_H

#include "OsgReferenced.h"

template<typename T>
class SDWeakPtr;

template<typename T>
class SDSharedPtr
{
public:
  typedef T element_type;

  SDSharedPtr(void) : _ptr(0)
  {}
  SDSharedPtr(T* ptr) : _ptr(ptr)
  { get(_ptr); }
  SDSharedPtr(const SDSharedPtr& p) : _ptr(p.get())
  { get(_ptr); }
  template<typename U>
  SDSharedPtr(const SDSharedPtr<U>& p) : _ptr(p.get())
  { get(_ptr); }
  ~SDSharedPtr(void)
  { put(); }
  
  SDSharedPtr& operator=(const SDSharedPtr& p)
  { assign(p.get()); return *this; }
  template<typename U>
  SDSharedPtr& operator=(const SDSharedPtr<U>& p)
  { assign(p.get()); return *this; }
  template<typename U>
  SDSharedPtr& operator=(U* p)
  { assign(p); return *this; }

  T* operator->(void) const
  { return _ptr; }
  T& operator*(void) const
  { return *_ptr; }
  operator T*(void) const
  { return _ptr; }
  T* ptr(void) const
  { return _ptr; }
  T* get(void) const
  { return _ptr; }
  T* release()
  { T* tmp = _ptr; _ptr = 0; T::put(tmp); return tmp; }

  bool isShared(void) const
  { return T::shared(_ptr); }
  unsigned getNumRefs(void) const
  { return T::count(_ptr); }

  bool valid(void) const
  { return _ptr != (T*)0; }

  void clear()
  { put(); }
  void swap(SDSharedPtr& sharedPtr)
  { T* tmp = _ptr; _ptr = sharedPtr._ptr; sharedPtr._ptr = tmp; }

private:
  void assign(T* p)
  { get(p); put(); _ptr = p; }
  void assignNonRef(T* p)
  { put(); _ptr = p; }

  void get(const T* p) const
  { T::get(p); }
  void put(void)
  { if (!T::put(_ptr)) delete _ptr; _ptr = 0; }
  
  // The reference itself.
  T* _ptr;

  template<typename U>
  friend class SDWeakPtr;
};

#endif
