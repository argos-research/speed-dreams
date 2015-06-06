/*
 *      util.h
 *      
 *      Copyright 2008 kilo aka Gabor Kmetyko <kg.kilo@gmail.com>
 *      
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *      
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *      
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef SRC_DRIVERS_KILO2008_UTIL_H_
#define SRC_DRIVERS_KILO2008_UTIL_H_

extern double Mag(const double x, const double y);
extern bool BetweenStrict(const double val, const double min, const double max);
extern bool BetweenLoose(const double val, const double min, const double max);
extern double sign(const double d);

#if 0 // doesn't compile in Vis Studio 2010
// until nullptr is defined...
const class {   // this is a const object...
 public:
  template<class T>           // convertible to any type
    operator T*() const       // of null non-member
    { return 0; }             // pointer...
  template<class C, class T>  // or any type of null
    operator T C::*() const   // member pointer...
    { return 0; }
 private:                     // whose address can't be taken
  void operator&() const;     // NOLINT(runtime/operator)
} nullptr = {};               // and whose name is nullptr
#endif



#endif  // SRC_DRIVERS_KILO2008_UTIL_H_
