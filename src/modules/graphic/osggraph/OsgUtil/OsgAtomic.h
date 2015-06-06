/***************************************************************************

    file                 : OsgAtomic.h
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

#ifndef OsgAtomic_H
#define OsgAtomic_H

#if defined(__GNUC__) && ((4 < __GNUC__)||(4 == __GNUC__ && 1 <= __GNUC_MINOR__)) && defined(__x86_64__)
// No need to include something. Is a Compiler API ...
# define SDATOMIC_USE_GCC4_BUILTINS
#elif defined(__GNUC__) && defined(__i386__)
# define SDATOMIC_USE_LIBRARY_FUNCTIONS
#elif defined(__sgi) && defined(_COMPILER_VERSION) && (_COMPILER_VERSION>=730)
// No need to include something. Is a Compiler API ...
# define SDATOMIC_USE_MIPSPRO_BUILTINS
#elif defined(_WIN32)
# define SDATOMIC_USE_LIBRARY_FUNCTIONS
#endif

class SDAtomic
{
public:
  SDAtomic(unsigned value = 0) : mValue(value)
  { }

#if defined(SDATOMIC_USE_LIBRARY_FUNCTIONS)
  unsigned operator++();
#else
  unsigned operator++()
  {
# if defined(SDATOMIC_USE_GCC4_BUILTINS)
    return __sync_add_and_fetch(&mValue, 1);
# elif defined(SDATOMIC_USE_MIPOSPRO_BUILTINS)
    return __add_and_fetch(&mValue, 1);
# else
#  error
# endif
  }
#endif

#if defined(SDATOMIC_USE_LIBRARY_FUNCTIONS)
  unsigned operator--();
#else
  unsigned operator--()
  {
# if defined(SDATOMIC_USE_GCC4_BUILTINS)
    return __sync_sub_and_fetch(&mValue, 1);
# elif defined(SDATOMIC_USE_MIPOSPRO_BUILTINS)
    return __sub_and_fetch(&mValue, 1);
# else
#  error
# endif
  }
#endif

#if defined(SDATOMIC_USE_LIBRARY_FUNCTIONS)
  operator unsigned() const;
#else
  operator unsigned() const
  {
# if defined(SDATOMIC_USE_GCC4_BUILTINS)
    __sync_synchronize();
    return mValue;
# elif defined(SDATOMIC_USE_MIPOSPRO_BUILTINS)
    __synchronize();
    return mValue;
# else
#  error
# endif
  }
#endif

#if defined(SDATOMIC_USE_LIBRARY_FUNCTIONS)
  bool compareAndExchange(unsigned oldValue, unsigned newValue);
#else
  bool compareAndExchange(unsigned oldValue, unsigned newValue)
  {
# if defined(SDATOMIC_USE_GCC4_BUILTINS)
    return __sync_bool_compare_and_swap(&mValue, oldValue, newValue);
# elif defined(SDATOMIC_USE_MIPOSPRO_BUILTINS)
    return __compare_and_swap(&mValue, oldValue, newValue);
# else
#  error
# endif
  }
#endif

private:
  SDAtomic(const SDAtomic&);
  SDAtomic& operator=(const SDAtomic&);

  unsigned mValue;
};

namespace osggraph
{
// Typesafe wrapper around SGSwappable
template <typename T>
class Swappable : private SDAtomic
{
public:
    Swappable(const T& value) : SDAtomic(static_cast<unsigned>(value))
    {
    }
    T operator() () const
    {
        return static_cast<T>(SDAtomic::operator unsigned ());
    }
    Swappable& operator=(const Swappable& rhs)
    {
        for (unsigned oldval = unsigned(*this);
             !compareAndExchange(oldval, unsigned(rhs));
             oldval = unsigned(*this))
            ;
        return *this;
    }
    bool compareAndSwap(const T& oldVal, const T& newVal)
    {
        return SDAtomic::compareAndExchange(static_cast<unsigned>(oldVal),
                                           static_cast<unsigned>(newVal));
    }
};
}
#endif
