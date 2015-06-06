#ifndef __OggSoundStream_h__
#define __OggSoundStream_h__

/***************************************************************************

    file                 : OggSoundStream.h
    created              : Fri Dec 23 17:35:18 CET 2011
    copyright            : (C) 2011 Bernhard Wymann
    email                : berniw@bluewin.ch
    version              : $Id: oggsoundstream.h 5145 2013-02-16 14:22:32Z pouillot $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

/* Concrete implementation for ogg sound streams */

#define OV_EXCLUDE_STATIC_CALLBACKS

#include <vorbis/vorbisfile.h>
#include "soundstream.h"

class OggSoundStream : public SoundStream
{
	public:
		OggSoundStream(char* path);
		virtual ~OggSoundStream();
		
		virtual int getRateInHz() { return _rateInHz; }
		virtual SoundFormat getSoundFormat() { return _format; }
		
		virtual bool read(char* buffer, const int bufferSize, int* resultSize, const char*& error);
		virtual void rewind();
		virtual void display();
		virtual bool isValid() { return _valid; }

	protected:
		
	private:
		const char* errorString(int code);
		
		OggVorbis_File	_oggStream;
		bool			_valid;
		int				_rateInHz;
		SoundFormat		_format;
};

#endif // __OggSoundStream_h__
