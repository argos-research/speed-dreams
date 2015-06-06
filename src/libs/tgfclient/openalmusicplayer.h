#ifndef __OpenALMusicPlayer_h__
#define __OpenALMusicPlayer_h__

/***************************************************************************

    file                 : OpenAlMusicPlayer.h
    created              : Fri Dec 23 17:35:18 CET 2011
    copyright            : (C) 2011 Bernhard Wymann
    email                : berniw@bluewin.ch
    version              : $Id: openalmusicplayer.h 5282 2013-03-09 17:18:06Z beaglejoe $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <AL/al.h>
#include <AL/alc.h>
#include "soundstream.h"

class OpenALMusicPlayer
{
	public:
		OpenALMusicPlayer(SoundStream* soundStream);
		virtual ~OpenALMusicPlayer();
		
		virtual void start();
		virtual void stop();
		virtual void pause();
		virtual void resume();
		virtual void rewind();
		virtual bool playAndManageBuffer();
		virtual void setvolume(float volume);
		virtual float getvolume();
		virtual void fadeout();
		virtual void fadein();

	protected:
		virtual bool initContext();
		virtual bool initBuffers();
		virtual bool initSource();
		virtual bool check();
		virtual bool startPlayback();
		virtual bool isPlaying();
		virtual bool streamBuffer(ALuint buffer);
		virtual void doFade();
		
		ALCdevice* _device;
		ALCcontext* _context;
		ALCcontext* _originalcontext;
		ALuint _source;								// audio source 
		ALuint _buffers[2];							// front and back buffers
		ALfloat _maxVolume;

		typedef enum { NONE, FADEIN, FADEOUT } eFadeState;

		eFadeState _fadestate;
		
		SoundStream* _stream;
		bool _ready;									// initialization sucessful
		static const int BUFFERSIZE;
		static const ALfloat FADESTEP;
};
#endif // __OpenALMusicPlayer_h__
