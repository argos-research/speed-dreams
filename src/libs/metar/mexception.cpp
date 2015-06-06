// exception.cxx - implementation of SimGear base exceptions.
// Started Summer 2001 by David Megginson, david@megginson.com
// This code is released into the Public Domain.
//
// $Id: exception.cxx,v 1.3.2.2 2007-10-13 13:51:51 durk Exp $

#include <stdio.h>

#include "mexception.h"


////////////////////////////////////////////////////////////////////////
// Implementation of m_location class.
////////////////////////////////////////////////////////////////////////

m_location::m_location ()
  : _path(""),
    _line(-1),
    _column(-1),
    _byte(-1)
{
}

m_location::m_location (const string &path, int line, int column)
  : _path(path),
    _line(line),
    _column(column),
    _byte(-1)
{
}

m_location::~m_location ()
{
}

const string & m_location::getPath () const
{
  return _path;
}

void m_location::setPath (const string &path)
{
  _path = path;
}

int m_location::getLine () const
{
  return _line;
}

void m_location::setLine (int line)
{
  _line = line;
}

int m_location::getColumn () const
{
  return _column;
}

void m_location::setColumn (int column)
{
  _column = column;
}

int m_location::getByte () const
{
  return _byte;
}

void m_location::setByte (int byte)
{
  _byte = byte;
}

string m_location::asString () const
{
	char buf[128];
  	string out = "";
  	if (!_path.empty()) 
  	{
    		out += _path;
    		if (_line != -1 || _column != -1)
      			out += ",\n";
  	}
  	if (_line != -1) 
  	{
    		snprintf(buf, 128, "line %d", _line);
    		out += buf;
    		if (_column != -1)
      			out += ", ";
  	}
  	if (_column != -1) 
  	{
    		snprintf(buf, 128, "column %d", _column);
    		out += buf;
  	}
  	
  	return out;    
}

////////////////////////////////////////////////////////////////////////
// Implementation of m_throwable class.
////////////////////////////////////////////////////////////////////////

m_throwable::m_throwable ()
  : _message(""),
    _origin("")
{
}

m_throwable::m_throwable (const string &message, const string &origin)
  : _message(message),
    _origin(origin)
{
}

m_throwable::~m_throwable ()
{
}

const string & m_throwable::getMessage () const
{
	return _message;
}

const string m_throwable::getFormattedMessage () const
{
  	return getMessage();
}

void m_throwable::setMessage (const string &message)
{
  	_message = message;
}

const string & m_throwable::getOrigin () const
{
  	return _origin;
}

void m_throwable::setOrigin (const string &origin)
{
  	_origin = origin;
}

////////////////////////////////////////////////////////////////////////
// Implementation of m_error class.
////////////////////////////////////////////////////////////////////////

m_error::m_error ()
  : m_throwable ()
{
}

m_error::m_error (const string &message, const string &origin)
  : m_throwable(message, origin)
{
}

m_error::~m_error ()
{
}

////////////////////////////////////////////////////////////////////////
// Implementation of m_exception class.
////////////////////////////////////////////////////////////////////////

m_exception::m_exception ()
  : m_throwable ()
{
}

m_exception::m_exception (const string &message, const string &origin)
  : m_throwable(message, origin)
{
}

m_exception::~m_exception ()
{
}

////////////////////////////////////////////////////////////////////////
// Implementation of m_io_exception.
////////////////////////////////////////////////////////////////////////

m_io_exception::m_io_exception ()
  : m_exception()
{
}

m_io_exception::m_io_exception (const string &message, const string &origin)
  : m_exception(message, origin)
{
}

m_io_exception::m_io_exception (const string &message,
				  const m_location &location,
				  const string &origin)
  : m_exception(message, origin),
    _location(location)
{
}

m_io_exception::~m_io_exception ()
{
}

const string m_io_exception::getFormattedMessage () const
{
  	string ret = getMessage();
  	string loc = getLocation().asString();
  	if (loc.length()) 
  	{
    		ret += "\n at ";
    		ret += loc;
  	}
  	
  	return ret;
}

const m_location & m_io_exception::getLocation () const
{
  	return _location;
}

void m_io_exception::setLocation (const m_location &location)
{
  	_location = location;
}

////////////////////////////////////////////////////////////////////////
// Implementation of m_format_exception.
////////////////////////////////////////////////////////////////////////

m_format_exception::m_format_exception ()
  : m_exception(),
    _text("")
{
}

m_format_exception::m_format_exception (const string &message,
					  const string &text,
					  const string &origin)
  : m_exception(message, origin),
    _text(text)
{
}

m_format_exception::~m_format_exception ()
{
}

const string & m_format_exception::getText () const
{
  return _text;
}

void m_format_exception::setText (const string &text)
{
  _text = text;
}

////////////////////////////////////////////////////////////////////////
// Implementation of m_range_exception.
////////////////////////////////////////////////////////////////////////

m_range_exception::m_range_exception ()
  : m_exception()
{
}

m_range_exception::m_range_exception (const string &message,
					const string &origin)
  : m_exception(message, origin)
{
}

m_range_exception::~m_range_exception ()
{
}

// end of mexception.cpp
