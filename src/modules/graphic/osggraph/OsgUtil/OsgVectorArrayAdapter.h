/***************************************************************************

    file                 : OsgVectorArrayAdapter.h
    created              : Mon Aug 21 18:24:02 CEST 2012
    copyright            : (C) 2012 by Xavier Bertaux
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

#ifndef OSGVECTORARRAYADAPTERH
#define OSGVECTORARRAYADAPTERH 1

namespace osggraph
{
template <typename Vector>
class SDVectorArrayAdapter 
{
public:

    SDVectorArrayAdapter(Vector& v, int rowStride, int baseOffset = 0,
        int rowOffset = 0):
        _v(v),  _rowStride(rowStride), _baseOffset(baseOffset),
        _rowOffset(rowOffset)
    {
    }

    typename Vector::value_type& operator() (int i, int j)
    {
        return _v[_baseOffset + i * _rowStride + _rowOffset + j];
    }
    const typename Vector::value_type& operator() (int i, int j) const
    {
        return _v[_baseOffset + i * _rowStride + _rowOffset + j];
    }

private:
    Vector& _v;
    const int _rowStride;
    const int _baseOffset;
    const int _rowOffset;
};
}
#endif
