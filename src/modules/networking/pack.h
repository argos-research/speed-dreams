/* pack.h - convert to (pack) and from (unpack) network data format
 *
 * Created: 2012-09-28
 * Author:  Tom Low-Shang
 *
 * Original: http://bzflag.svn.sourceforge.net/viewvc/bzflag/trunk/bzflag/include/Pack.h?view=markup
 *
 * This package is free software;  you can redistribute it and/or modify it
 * under the terms of the license found in the file named COPYING that should
 * have accompanied this file.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */
#ifndef	SD_PACK_H
#define	SD_PACK_H

#include <string>
/* SDL data types */
#include <SDL_stdinc.h>

class PackedBuffer
{
public:
    PackedBuffer(size_t size=1024);
    PackedBuffer(unsigned char *, size_t);
    ~PackedBuffer();

    unsigned char *buffer();
    size_t length();

    void pack_ubyte(unsigned char);
    void pack_short(short);
    void pack_int(int);
    void pack_ushort(unsigned short);
    void pack_uint(unsigned int);
    void pack_float(float);
    void pack_double(double);
    void pack_vector(const float*);
    void pack_string(const void*, int);
    void pack_stdstring(const std::string& str);

    unsigned char unpack_ubyte();
    short unpack_short();
    int unpack_int();
    unsigned short unpack_ushort();
    unsigned int unpack_uint();
    float unpack_float();
    double unpack_double();
    float* unpack_vector(float*);
    void* unpack_string(void*, int len);
    std::string& unpack_stdstring(std::string& str);

private:
    size_t buf_size;
    Uint8 *buf_start;
    Uint8 *buf_data;
    bool buf_is_ours;
    size_t data_length;

    void next_data(size_t);
    bool bounds_error(size_t);
};

class PackedBufferException : public std::exception
{
};

#endif // SD_PACK_H

// Local Variables: ***
// mode:C++ ***
// tab-width: 8 ***
// c-basic-offset: 2 ***
// indent-tabs-mode: t ***
// End: ***
// ex: shiftwidth=4 tabstop=8
