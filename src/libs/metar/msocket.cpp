// sg_socket.cxx -- Socket I/O routines
//
// Written by Curtis Olson, started November 1999.
// Modified by Bernie Bright <bbright@bigpond.net.au>, May 2002.
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
// $Id: sg_socket.cxx,v 1.13 2006-03-08 18:16:08 mfranz Exp $

#include <string>

#include "msocket.h"

bool MSocket::init = false;

MSocket::MSocket( const string& host, const string& port_, 
		    const string& style ) :
    hostname(host),
    port_str(port_),
    save_len(0),
    client(0),
    is_tcp(false),
    is_server(false),
    first_read(false),
    timeout(0)
{
    if (!init)
    {
	netInit(NULL, NULL);	// plib-1.4.2 compatible
	init = true;
    }

    if ( style == "tcp" )
    {
	is_tcp = true;
    }
    else if ( style != "udp" )
    {
    }

    set_type( sgSocketType );
}

MSocket::~MSocket()
{
    this->close();
}

bool MSocket::make_server_socket()
{
    if (!sock.open( is_tcp ))
    {
	return false;
    }

    if (sock.bind( "", port ) < 0)
    {
	sock.close();
        return false;
    }

    return true;
}

bool MSocket::make_client_socket()
{
    if (!sock.open( is_tcp ))
    {
	return false;
    }

    if (sock.connect( hostname.c_str(), port ) < 0)
    {
	sock.close();
        return false;
    }

    return true;
}

// If specified as a server (in direction for now) open the master
// listening socket.  If specified as a client (out direction), open a
// connection to a server.
bool MSocket::open( MProtocolDir direction )
{
    set_dir( direction );

    is_server = is_tcp &&
	(direction == M_IO_IN || direction == M_IO_BI);

    if ( port_str == "" || port_str == "any" ) 
    {
	port = 0; 
    } else 
    {
	port = atoi( port_str.c_str() );
    }
    
    if (direction == M_IO_IN)
    {
	// this means server for now

	// Setup socket to listen on.  Set "port" before making this
	// call.  A port of "0" indicates that we want to let the os
	// pick any available port.
	if (!make_server_socket())
	{
	    return false;
	}

	if ( !is_tcp )
	{
	    nonblock();
	}
	else
	{
	    // Blocking TCP
	    // Specify the maximum length of the connection queue
	    sock.listen( M_MAX_SOCKET_QUEUE );
	}

    }
    else if (direction == M_IO_OUT)
    {
	// this means client for now

	if (!make_client_socket())
	{
	    return false;
	}

	if ( !is_tcp )
	{
	    // Non-blocking UDP
	    nonblock();
	}
    }
    else if (direction == M_IO_BI && is_tcp)
    {
	// this means server for TCP sockets

	// Setup socket to listen on.  Set "port" before making this
	// call.  A port of "0" indicates that we want to let the os
	// pick any available port.
	if (!make_server_socket())
	{
	    return false;
	}
	// Blocking TCP
	// Specify the maximum length of the connection queue
	sock.listen( M_MAX_SOCKET_QUEUE );
    }
    else
    {
	return false;
    }

    first_read = false;

    return true;
}


// read data from socket (server)
// read a block of data of specified size
int MSocket::read( char *buf, int length )
{
    if (sock.getHandle() == -1 && (client == 0 || client->getHandle() == -1))
    {
	return 0;
    }

    // test for any input available on sock (returning immediately, even if
    // nothing)
    int result = poll();

    if (result > 0)
    {
       if (is_tcp && is_server)
	{
            result = client->recv( buf, length );
        }
        else
        {
            result = sock.recv( buf, length );
        }

	if ( result != length )
	{
	}
    }

    return result;
}


// read a line of data, length is max size of input buffer
int MSocket::readline( char *buf, int length )
{
    if (sock.getHandle() == -1 && (client == 0 || client->getHandle() == -1))
    {
	return 0;
    }

    // test for any input read on sock (returning immediately, even if
    // nothing)
    int result = this->poll();

    if (result > 0)
    {
	// read a chunk, keep in the save buffer until we have the
	// requested amount read

       if (is_tcp && is_server)
	{
	    char *buf_ptr = save_buf + save_len;
	    result = client->recv( buf_ptr, M_IO_MAX_MSG_SIZE - save_len );
		
	    if ( result > 0 )
	    {
		first_read = true;
	    }

	    save_len += result;

	    // Try and detect that the remote end died.  This
	    // could cause problems so if you see connections
	    // dropping for unexplained reasons, LOOK HERE!
	    if (result == 0 && save_len == 0 && first_read == true)
	    {
		delete client;
		client = 0;
	    }
	}
	else
	{
	    char *buf_ptr = save_buf + save_len;
	    result = sock.recv( buf_ptr, M_IO_MAX_MSG_SIZE - save_len );
	    save_len += result;
	}
    }

    // look for the end of line in save_buf
    int i;
    for ( i = 0; i < save_len && save_buf[i] != '\n'; ++i )
	;
    if ( save_buf[i] == '\n' ) 
    {
	result = i + 1;
    } else 
    {
	// no end of line yet
	return 0;
    }

    // we found an end of line

    // copy to external buffer
    strncpy( buf, save_buf, result );
    buf[result] = '\0';

    // shift save buffer
    //memmove( save_buf+, save_buf+, ? );
    for ( i = result; i < save_len; ++i ) 
    {
	save_buf[ i - result ] = save_buf[i];
    }
    save_len -= result;

    return result;
}

// write data to socket (client)
int MSocket::write( const char *buf, const int length )
{
    netSocket* s = client == 0 ? &sock : client;
    if (s->getHandle() == -1)
    {
	return 0;
    }

    bool error_condition = false;

    if ( s->send( buf, length ) < 0 )
    {
	error_condition = true;
    }

    if ( error_condition ) 
    {
	return 0;
    }

    return length;
}


// write null terminated string to socket (server)
int MSocket::writestring( const char *str )
{
    int length = strlen( str );
    return this->write( str, length );
}


// close the port
bool MSocket::close()
{
    delete client;
    client = 0;

    sock.close();
    return true;
}


// configure the socket as non-blocking
bool MSocket::nonblock()
{
    if (sock.getHandle() == -1) 
    {
	return false;
    }

    sock.setBlocking( false );
    return true;
}

int MSocket::poll()
{
    netSocket* readers[2];

    readers[0] = client != 0 ? client : &sock;
    readers[1] = 0;

    netSocket* writers[1];
    writers[0] = 0;

    int result = netSocket::select( readers, writers, timeout );

    if (result > 0 && is_server && client == 0)
    {
	// Accept a new client connection
	netAddress addr;
	int new_fd = sock.accept( &addr );
	client = new netSocket();
	client->setHandle( new_fd );
	return 0;
    }

    return result;
}
