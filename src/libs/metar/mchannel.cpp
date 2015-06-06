// iochannel.cxx -- High level IO channel class
//
// Written by Curtis Olson, started November 1999.
//
// Copyright (C) 1999  Curtis L. Olson - http://www.flightgear.org/~curt
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id: iochannel.cxx,v 1.6 2006-03-08 18:16:08 mfranz Exp $

#include "mchannel.h"

// constructor
MIOChannel::MIOChannel()
{
}

// destructor
MIOChannel::~MIOChannel()
{
}

// dummy configure routine
bool MIOChannel::open( const MProtocolDir d ) 
{
    return false;
}

// dummy process routine
int MIOChannel::read( char *buf, int length ) 
{
    return 0;
}

// dummy process routine
int MIOChannel::readline( char *buf, int length ) 
{
    return 0;
}

// dummy process routine
int MIOChannel::write( const char *buf, const int length ) 
{
    return false;
}

// dummy process routine
int MIOChannel::writestring( const char *str ) 
{
    return false;
}

// dummy close routine
bool MIOChannel::close() 
{
    return false;
}

// dummy eof routine
bool MIOChannel::eof() 
{
    return false;
}
