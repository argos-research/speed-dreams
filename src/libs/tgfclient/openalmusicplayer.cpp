/***************************************************************************

    file                 : OpenAlMusicPlayer.cpp
    created              : Fri Dec 23 17:35:18 CET 2011
    copyright            : (C) 2011 Bernhard Wymann
    email                : berniw@bluewin.ch
    version              : $Id: openalmusicplayer.cpp 5353 2013-03-24 10:26:22Z pouillot $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <cstdio>
#include <tgf.h>
#include "openalmusicplayer.h"

const int OpenALMusicPlayer::BUFFERSIZE = 4096*64;
const ALfloat OpenALMusicPlayer::FADESTEP = 0.01f;

OpenALMusicPlayer::OpenALMusicPlayer(SoundStream* soundStream):
	_device(NULL),
	_context(NULL),
	_originalcontext(NULL),
	_source(0),
	_maxVolume(1.0),
	_fadestate(FADEIN),
	_stream(soundStream),
	_ready(false)
{
	_buffers[0] = 0;
	_buffers[1] = 0;
}




OpenALMusicPlayer::~OpenALMusicPlayer()
{
	if (_ready) {
		stop();
	}
	if(_originalcontext == NULL) {
		alcMakeContextCurrent(0);
		alcDestroyContext(_context);
		alcCloseDevice(_device);
	}
	if(_stream) {
		delete _stream;
		_stream = NULL;
	}
}




void OpenALMusicPlayer::stop()
{
	if (!_ready) {
		return;
	}
	
	alSourceStop(_source);
    
	int queued = 0;
	
	alGetSourcei(_source, AL_BUFFERS_QUEUED, &queued);
	while (queued--) {
		ALuint buffer;
		alSourceUnqueueBuffers(_source, 1, &buffer);
		check();
	}
	
    alDeleteSources(1, &_source);
    check();
    alDeleteBuffers(2, _buffers);
    check();
	
	_ready = false;
}




bool OpenALMusicPlayer::initContext()
{
	_originalcontext = alcGetCurrentContext();
	if(_originalcontext == NULL) {
		_device = alcOpenDevice(NULL);
		if(_device == NULL ) {
			GfLogError("OpenALMusicPlayer: OpenAL could not open device\n");
			return false;
		}

		_context = alcCreateContext(_device, NULL);
		if(_context == NULL) {
			alcCloseDevice(_device);
			GfLogError("OpenALMusicPlayer: OpenAL could not create contect for device\n");
			return false;
		}
		alcMakeContextCurrent(_context);
		alcGetError(_device);
	}
	return check();
}




bool OpenALMusicPlayer::initBuffers()
{
	alGenBuffers(2, _buffers);
	return check();
}




bool OpenALMusicPlayer::initSource()
{
    alGenSources(1, &_source);
    if (!check()) {
		GfLogError("OpenALMusicPlayer: initSource failed to get sound source.\n");
		return false;
	};
    
    alSource3f(_source, AL_POSITION,        0.0, 0.0, 0.0);
    alSource3f(_source, AL_VELOCITY,        0.0, 0.0, 0.0);
    alSource3f(_source, AL_DIRECTION,       0.0, 0.0, 0.0);
    alSourcef (_source, AL_ROLLOFF_FACTOR,  0.0          );
    alSourcei (_source, AL_SOURCE_RELATIVE, AL_TRUE      );
	
	return true;
}




bool OpenALMusicPlayer::check()
{
	int error = alGetError();

	if(error != AL_NO_ERROR) {
		GfLogError("OpenALMusicPlayer: OpenAL error was raised: %d\n", error);
		return false;
	}

	return true;
}




bool OpenALMusicPlayer::isPlaying()
{
    ALenum state;
	
    alGetSourcei(_source, AL_SOURCE_STATE, &state);    
    return (state == AL_PLAYING);
}



bool OpenALMusicPlayer::streamBuffer(ALuint buffer)
{
	char pcm[BUFFERSIZE];
	int size = 0;
	const char* error = '\0';
	
	if (!_stream->read(pcm, BUFFERSIZE, &size, error)) {
		GfLogError("OpenALMusicPlayer: Stream read error: %s\n", error);
		return false;
	}

	int format;
	switch (_stream->getSoundFormat()) {
		case SoundStream::FORMAT_MONO16:
			format = AL_FORMAT_MONO16;
			break;
		case SoundStream::FORMAT_STEREO16:
			format = AL_FORMAT_STEREO16;
			break;
		default:
			GfLogError("OpenALMusicPlayer: Format error: \n");
			return false;
	}
	
	alBufferData(buffer, format, pcm, size, _stream->getRateInHz());
	return check();
}




void OpenALMusicPlayer::start()
{
	if (!_ready) {
		if (_stream->getSoundFormat() == SoundStream::FORMAT_INVALID) {
			GfLogError("OpenALMusicPlayer: Sound stream has invalid format\n");
			return;
		}
		
		if (initContext() && initBuffers() && initSource()) {
			_ready = true;
			startPlayback();
		}
		
		return;
	}
}

void OpenALMusicPlayer::pause()
{
	if(isPlaying()) {
		alSourceStop(_source);
	}
}

void OpenALMusicPlayer::resume()
{
	if(!isPlaying()) {
		alSourcePlay(_source);
	}
}
void OpenALMusicPlayer::rewind()
{
	_stream->rewind();
}




bool OpenALMusicPlayer::playAndManageBuffer()
{
	if (!_ready) {
		return false;
	}
	
	int processed = 0;
	bool active = true;

	doFade();

	alGetSourcei(_source, AL_BUFFERS_PROCESSED, &processed);

	while(processed--) {
		ALuint buffer;
		
		alSourceUnqueueBuffers(_source, 1, &buffer);
		check();
		active = streamBuffer(buffer);
		alSourceQueueBuffers(_source, 1, &buffer);
		check();
	}

	if (!active && !isPlaying()) {
		// Try to reanimate playback
		if(!startPlayback()) {
			GfLogError("OpenALMusicPlayer: Cannot play stream.\n");
		}
	}
	
	return true;
}




bool OpenALMusicPlayer::startPlayback()
{
    if(isPlaying()) {
        return true;
	}
	
    if(!streamBuffer(_buffers[0])) {
        return false;
	}
        
    if(!streamBuffer(_buffers[1])) {
        return false;
	}
    
    alSourceQueueBuffers(_source, 2, _buffers);
    alSourcePlay(_source);
    
    return true;
}

void OpenALMusicPlayer::fadeout()
{
	_fadestate = FADEOUT;
}

void OpenALMusicPlayer::fadein()
{
	_fadestate = FADEIN;
	alSourcef(_source, AL_GAIN, 0.0f);
}

void OpenALMusicPlayer::setvolume(float volume)
{
	_maxVolume = volume;
}

float OpenALMusicPlayer::getvolume()
{
	return _maxVolume;
}

void OpenALMusicPlayer::doFade()
{
	ALfloat currentVol = 0.0;
	switch(_fadestate){
		case FADEIN:
			alGetSourcef(_source, AL_GAIN, &currentVol);
			currentVol += FADESTEP;
			if(currentVol >= _maxVolume){
				currentVol = _maxVolume;
				_fadestate = NONE;
			}
			alSourcef(_source, AL_GAIN, currentVol);

			break;
		case FADEOUT:
			alGetSourcef(_source, AL_GAIN, &currentVol);
			currentVol -= FADESTEP;
			if(currentVol <= 0.0){
				currentVol = 0.0;
				_fadestate = NONE;
			}
			alSourcef(_source, AL_GAIN, currentVol);
			break;
		case NONE:
			break;
	}
	
}
