/* pack.cpp - convert to (pack) and from (unpack) network data format
 *
 * Created: 2012-09-28
 * Author:  Tom Low-Shang
 *
 * Original: http://bzflag.svn.sourceforge.net/viewvc/bzflag/trunk/bzflag/src/net/Pack.cxx?view=markup
 *
 * This package is free software;  you can redistribute it and/or modify it
 * under the terms of the license found in the file named COPYING that should
 * have accompanied this file.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#include <string>

#ifdef _WIN32
# if defined(_MSC_VER)
#   pragma warning(disable: 4786)
# endif
# include <winsock2.h>
# include <ws2tcpip.h>
#endif

#if !defined(_WIN32)
#include <arpa/inet.h>
#endif

/* SDL data types */
#include <SDL_stdinc.h>

/* SDL endian swap functions */
#include <SDL_endian.h>

#include <tgf.h>
#include "pack.h"

union float_uint32 {
    float floatval;
    Uint32 intval;
};

union double_uint64 {
    double d;
    Uint64 i;
};

// Initialize for packing
PackedBuffer::PackedBuffer(size_t size)
{
    buf_start = new Uint8[size];
    buf_data = buf_start;
    buf_size = size;
    buf_is_ours = true;
    data_length = 0;
}

// Initialize of unpacking
PackedBuffer::PackedBuffer(unsigned char *buf, size_t len)
{
    buf_start = buf_data = buf;
    buf_size = len;
    buf_is_ours = false;
    data_length = 0;
}

PackedBuffer::~PackedBuffer()
{
    if (buf_is_ours)
    {
        delete [] buf_start;
    }
}

unsigned char *PackedBuffer::buffer()
{
    return buf_start;
}

size_t PackedBuffer::length()
{
    return buf_is_ours ? data_length : buf_size;
}

void PackedBuffer::next_data(size_t data_size)
{
    buf_data = buf_data + data_size;
    data_length += data_size;
}

bool PackedBuffer::bounds_error(size_t item_size)
{
    return data_length + item_size > buf_size;
}

void PackedBuffer::pack_ubyte(unsigned char v)
{
    const size_t size = sizeof(Uint8);

    if (bounds_error(size))
    {
        GfLogError("pack_ubyte: buffer overflow: buf_size=%d "
                "data_length=%d\n", buf_size, data_length);
        throw PackedBufferException();
    }

    Uint8 w = (Uint8)v;

    ::memcpy(buf_data, &w, size);
    next_data(size);
}

void PackedBuffer::pack_short(short v)
{
    const size_t size = sizeof(Sint16);

    if (bounds_error(size))
    {
        GfLogError("pack_short: buffer overflow: buf_size=%d "
                "data_length=%d\n", buf_size, data_length);
        throw PackedBufferException();
    }

    const Uint16 x = (Uint16)htons((Uint16)v);

    ::memcpy(buf_data, &x, size);
    next_data(size);
}

void PackedBuffer::pack_int(int v)
{
    const size_t size = sizeof(Sint32);

    if (bounds_error(size))
    {
        GfLogError("pack_int: buffer overflow: buf_size=%d "
                "data_length=%d\n", buf_size, data_length);
        throw PackedBufferException();
    }

    const Uint32 x = (Uint32)htonl((Uint32)v);

    ::memcpy(buf_data, &x, size);
    next_data(size);
}

void PackedBuffer::pack_ushort(unsigned short v)
{
    const size_t size = sizeof(Uint16);

    if (bounds_error(size))
    {
        GfLogError("pack_ushort: buffer overflow: buf_size=%d "
                "data_length=%d\n", buf_size, data_length);
        throw PackedBufferException();
    }

    const Uint16 x = (Uint16)htons((Uint16)v);

    ::memcpy(buf_data, &x, size);
    next_data(size);
}

void PackedBuffer::pack_uint(unsigned int v)
{
    const size_t size = sizeof(Uint32);

    if (bounds_error(size))
    {
        GfLogError("pack_uint: buffer overflow: buf_size=%d "
                "data_length=%d\n", buf_size, data_length);
        throw PackedBufferException();
    }

    const Uint32 x = (Uint32)htonl((Uint32)v);

    ::memcpy(buf_data, &x, size);
    next_data(size);
}


void PackedBuffer::pack_float(float v)
{
    const size_t size = sizeof(Uint32);

    if (bounds_error(size))
    {
        GfLogError("pack_float: buffer overflow: buf_size=%d "
                "data_length=%d\n", buf_size, data_length);
        throw PackedBufferException();
    }

    // assume that float is a 4 byte IEEE 754 standard encoding
    float_uint32 u;
    u.floatval = v;

    const Uint32 x = (Uint32)htonl(u.intval);

    ::memcpy(buf_data, &x, size);
    next_data(size);
}

/* There are no standard 64bit network endian swap functions. SDL has two types
 * of endian swap functions that either always swaps, or only swaps to native
 * endianness. For packing use a function from the first group on little endian
 * systems. On big endian systems, use direct copy. Functions from the second
 * group are good for unpacking.
 */
void PackedBuffer::pack_double(double v) { const size_t size = sizeof(Uint64);

    if (bounds_error(size))
    {
        GfLogError("pack_double: buffer overflow: buf_size=%d "
                "data_length=%d\n", buf_size, data_length);
        throw PackedBufferException();
    }

#if SDL_BYTEORDER == SDL_LIL_ENDIAN
    // assume that double is a 8 byte IEEE 754 standard encoding
    double_uint64 u;
    u.d = v;

    const Uint64 x = SDL_Swap64(u.i);

    ::memcpy(buf_data, &x, size);
    next_data(size);
#else
    const Uint64 x = v;

    ::memcpy(buf_data, &x, size);
    next_data(size);
#endif
}

/* How does SD store vectors? This method should modified accordingly or
 * if it is unnecesary.
 */
void PackedBuffer::pack_vector(const float *v)
{
    const size_t size = 3 * sizeof(Uint32);

    if (bounds_error(size))
    {
        GfLogError("pack_vector: buffer overflow: buf_size=%d "
                "data_length=%d\n", buf_size, data_length);
        throw PackedBufferException();
    }

    // assume that float is a 4 byte IEEE 754 standard encoding
    float_uint32 u;
    Uint32 data[3];

    for (int i = 0; i < 3; i++) {
        u.floatval = v[i];
        data[i] = (Uint32)htonl(u.intval);
    }

    ::memcpy(buf_data, data, size);
    next_data(size);
}

/* "String" means "a buffer of known length" in a network message context.
 * "Known length" could mean fixed length or variable length if the length is
 * included in the message
 */
void PackedBuffer::pack_string(const void *m, int len)
{
    if (bounds_error(len))
    {
        GfLogError("pack_string: buffer overflow: buf_size=%d "
                "data_length=%d\n", buf_size, data_length);
        throw PackedBufferException();
    }

    if (!m || len == 0) {
        return;
    }

    ::memcpy(buf_data, m, len);
    next_data(len);
}

void PackedBuffer::pack_stdstring(const std::string& str)
{
    const size_t size = str.size();

    if (bounds_error(size))
    {
        GfLogError("pack_stdstring: buffer overflow: buf_size=%d "
                "data_length=%d\n", buf_size, data_length);
        throw PackedBufferException();
    }

    pack_uint(size);
    pack_string(str.c_str(), size);
}

unsigned char PackedBuffer::unpack_ubyte()
{
    const size_t size = sizeof(Uint8);

    if (bounds_error(size))
    {
        GfLogError("unpack_ubyte: buffer overrun: buf_size=%d "
                "data_length=%d\n", buf_size, data_length);
        throw PackedBufferException();
    }

    Uint8 v;

    ::memcpy(&v, buf_data, size);
    next_data(size);

    return (unsigned char)v;
}

short PackedBuffer::unpack_short()
{
    const size_t size = sizeof(Sint16);

    if (bounds_error(size))
    {
        GfLogError("unpack_short: buffer overrun: buf_size=%d "
                "data_length=%d\n", buf_size, data_length);
        throw PackedBufferException();
    }

    Sint16 x;

    ::memcpy(&x, buf_data, size);
    next_data(size);

    return (short)ntohs(x);
}

int PackedBuffer::unpack_int()
{
    const size_t size = sizeof(Sint32);

    if (bounds_error(size))
    {
        GfLogError("unpack_int: buffer overrun: buf_size=%d "
                "data_length=%d\n", buf_size, data_length);
        throw PackedBufferException();
    }

    Sint32 x;

    ::memcpy(&x, buf_data, size);
    next_data(size);

    return (int)ntohl(x);
}

unsigned short PackedBuffer::unpack_ushort()
{
    const size_t size = sizeof(Uint16);

    if (bounds_error(size))
    {
        GfLogError("unpack_ushort: buffer overrun: buf_size=%d "
                "data_length=%d\n", buf_size, data_length);
        throw PackedBufferException();
    }

    Uint16 x;
    
    ::memcpy(&x, buf_data, size);
    next_data(size);

    return (unsigned short)ntohs(x);
}

unsigned int PackedBuffer::unpack_uint()
{
    const size_t size = sizeof(Uint32);

    if (bounds_error(size))
    {
        GfLogError("unpack_uint: buffer overrun: buf_size=%d "
                "data_length=%d\n", buf_size, data_length);
        throw PackedBufferException();
    }

    Uint32 x;

    ::memcpy(&x, buf_data, size);
    next_data(size);
    
    return (unsigned int)ntohl(x);
}

float PackedBuffer::unpack_float()
{
    const size_t size = sizeof(Uint32);

    if (bounds_error(size))
    {
        GfLogError("unpack_float: buffer overrun: buf_size=%d "
                "data_length=%d\n", buf_size, data_length);
        throw PackedBufferException();
    }

    // assume that float is a 4 byte IEEE 754 standard encoding
    Uint32 x;
    float_uint32 u;

    ::memcpy(&x, buf_data, size);
    next_data(size);
    u.intval = (Uint32)ntohl(x);
    
    return u.floatval;
}

/* The SDL_Swap{BE,LE} functions swap from the specified endianness (BE or LE)
 * to native and are no-operations if the endianness is the same.
 */
double PackedBuffer::unpack_double()
{
    const size_t size = sizeof(Uint64);

    if (bounds_error(size))
    {
        GfLogError("unpack_double: buffer overrun: buf_size=%d "
                "data_length=%d\n", buf_size, data_length);
        throw PackedBufferException();
    }

    // assume that double is a 8 byte IEEE 754 standard encoding
    Uint64 x;
    double_uint64 u;

    ::memcpy(&x, buf_data, size);
    next_data(size);
    u.i = SDL_SwapBE64(x);

    return u.d;
}

float *PackedBuffer::unpack_vector(float *v)
{
    const size_t size = 3 * sizeof(Uint32);

    if (bounds_error(size))
    {
        GfLogError("unpack_vector: buffer overrun: buf_size=%d "
                "data_length=%d\n", buf_size, data_length);
        throw PackedBufferException();
    }

    // assume that float is a 4 byte IEEE 754 standard encoding
    Uint32 data[3];
    float_uint32 u;

    ::memcpy(data, buf_data, size);
    next_data(size);

    for (int i = 0; i < 3; i++)
    {
        u.intval = (Uint32)ntohl(data[i]);
        v[i] = u.floatval;
    }

    return v;
}

void *PackedBuffer::unpack_string(void *m, int len)
{
    if (bounds_error(len))
    {
        GfLogError("unpack_string: buffer overrun: buf_size=%d "
                "data_length=%d\n", buf_size, data_length);
        throw PackedBufferException();
    }

    ::memcpy(m, buf_data, len);
    next_data(len);

    return m;
}

std::string &PackedBuffer::unpack_stdstring(std::string& str)
{
    const Uint32 size = unpack_uint();

    if (bounds_error(size))
    {
        GfLogError("unpack_stdstring: packed overrun: buf_size=%d "
                "data_length=%d\n", buf_size, data_length);
        throw PackedBufferException();
    }

    char *buffer = new char[size + 1];

    unpack_string(buffer, size);
    buffer[size] = 0;
    str = buffer;

    delete [] buffer;

    return str;
}

// Local Variables: ***
// mode:C++ ***
// tab-width: 8 ***
// c-basic-offset: 2 ***
// indent-tabs-mode: t ***
// End: ***
// ex: shiftwidth=4 tabstop=8

