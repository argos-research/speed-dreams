/**
 * \file exception.hxx
 * Interface definition for SimGear base exceptions.
 * Started Spring 2001 by David Megginson, david@megginson.com
 * This code is released into the Public Domain.
 *
 * $Id: exception.hxx,v 1.1 2003-09-24 17:19:23 ehofman Exp $
 */

#ifndef __MEXCEPTION_H
#define __MEXCEPTION_H 1

#include <string>

using namespace std;


/**
 * Information encapsulating a single location in an external resource
 *
 * A position in the resource my optionally be provided, either by
 * line number, line number and column number, or byte offset from the
 * beginning of the resource.
 */
class m_location
{
public:
  m_location ();
  m_location (const string &path, int line = -1, int column = -1);
  virtual ~m_location ();
  virtual const string &getPath () const;
  virtual void setPath (const string &path);
  virtual int getLine () const;
  virtual void setLine (int line);
  virtual int getColumn () const;
  virtual void setColumn (int column);
  virtual int getByte () const;
  virtual void setByte (int byte);
  virtual string asString () const;
private:
  string _path;
  int _line;
  int _column;
  int _byte;
};


/**
 * Abstract base class for all throwables.
 */
class m_throwable
{
public:
  m_throwable ();
  m_throwable (const string &message, const string &origin = "");
  virtual ~m_throwable ();
  virtual const string &getMessage () const;
  virtual const string getFormattedMessage () const;
  virtual void setMessage (const string &message);
  virtual const string &getOrigin () const;
  virtual void setOrigin (const string &origin);
private:
  string _message;
  string _origin;
};

/**
 * An unexpected fatal error.
 *
 * Methods and functions show throw this exception when something
 * very bad has happened (such as memory corruption or
 * a totally unexpected internal value).  Applications should catch
 * this exception only at the top level if at all, and should
 * normally terminate execution soon afterwards.
 */
class m_error : public m_throwable
{
public:
  m_error ();
  m_error (const string &message, const string &origin = "");
  virtual ~m_error ();
};


/**
 * Base class for all SimGear exceptions.
 *
 * SimGear-based code should throw this exception only when no
 * more specific exception applies.  It may not be caught until
 * higher up in the application, where it is not possible to
 * resume normal operations if desired.
 *
 * A caller can catch sg_exception by default to ensure that
 * all exceptions are caught.  Every SimGear exception can contain
 * a human-readable error message and a human-readable string
 * indicating the part of the application causing the exception
 * (as an aid to debugging, only).
 */
class m_exception : public m_throwable
{
public:
  m_exception ();
  m_exception (const string &message, const string &origin = "");
  virtual ~m_exception ();
};

/**
 * An I/O-related SimGear exception.
 *
 * SimGear-based code should throw this exception when it fails
 * to read from or write to an external resource, such as a file,
 * socket, URL, or database.
 *
 * In addition to the functionality of sg_exception, an
 * sg_io_exception may contain location information, such as the name
 * of a file or URL, and possible also a location in that file or URL.
 */
class m_io_exception : public m_exception
{
public:
  m_io_exception ();
  m_io_exception (const string &message, const string &origin = "");
  m_io_exception (const string &message, const m_location &location,
		   const string &origin = "");
  virtual ~m_io_exception ();
  virtual const string getFormattedMessage () const;
  virtual const m_location &getLocation () const;
  virtual void setLocation (const m_location &location);
private:
  m_location _location;
};

/**
 * A format-related SimGear exception.
 *
 * SimGear-based code should throw this exception when a string
 * does not appear in the expected format (for example, a date
 * string does not conform to ISO 8601).
 *
 * In addition to the functionality of sg_exception, an
 * sg_format_exception can contain a copy of the original malformated
 * text.
 */
class m_format_exception : public m_exception
{
public:
  m_format_exception ();
  m_format_exception (const string &message, const string &text,
		       const string &origin = "");
  virtual ~m_format_exception ();
  virtual const string &getText () const;
  virtual void setText (const string &text);
private:
  string _text;
};

/**
 * A range-related SimGear exception.
 *
 * SimGear-based code should throw this exception when a value falls
 * outside the range where it can reasonably be handled; examples
 * include longitude outside the range -180:180, unrealistically high
 * forces or velocities, an illegal airport code, etc.  A range
 * exception usually means that something has gone wrong internally.
 */
class m_range_exception : public m_exception
{
public:
  m_range_exception ();
  m_range_exception (const string &message, const string &origin = "");
  virtual ~m_range_exception ();
};

#endif

// end of mexception.h
