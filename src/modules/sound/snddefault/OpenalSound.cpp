/***************************************************************************
    file                 : Sound.cpp
    created              : Tue Jul 18 19:57:35 CEST 2011
    copyright            : (C) 2005 Christos Dimitrakakis, Bernhard Wymann
    email                : dimitrak@idiap.ch
    version              : $Id: OpenalSound.cpp 5485 2013-05-28 15:50:38Z beaglejoe $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <SDL.h>
#include <tgf.h>

#include "OpenalSoundInterface.h"
#include "OpenalSound.h"


OpenalSound::OpenalSound(const char* filename, OpenalSoundInterface* sitf,
						 int flags, bool loop, bool static_pool)
: Sound(flags, loop)
{
	this->static_pool = static_pool;
	poolindex = -1;
	itf = sitf;

	MAX_DISTANCE = 10000.0f;
	MAX_DISTANCE_LOW = 5.0f;
	REFERENCE_DISTANCE = 5.0f;
	ROLLOFF_FACTOR = 0.5f;

	int i;
	for (i = 0; i<3; i++) {
		source_position[i] = 0.0f;
		source_velocity[i] = 0.0f;
		zeroes[i] = 0.0f;
	}

	GfLogTrace("OpenAL : Creating %s source from %s\n",
			   static_pool ? "static" : "dynamic", filename);

	int error = alGetError();
	if (error != AL_NO_ERROR) {
		printf("Uncatched OpenAL Error on entry: %d with file %s\n", error, filename);
	}
	
	alGenBuffers (1, &buffer);
	error = alGetError();
	if (error != AL_NO_ERROR) {
		printf("OpenAL Error: %d, alGenBuffers failed %s\n", error, filename);
		is_enabled = false;
		return;
	}

	SDL_AudioSpec wavspec;
	Uint32 wavlen;
	Uint8 *wavbuf;
	if (!SDL_LoadWAV(filename, &wavspec, &wavbuf, &wavlen))
	{
		if (alIsBuffer(buffer))
			alDeleteBuffers(1, &buffer);
		GfError("OpenAL Error: Could not load %s (%s)\n", filename, SDL_GetError());
		is_enabled = false;
		return;
	}

	if (wavspec.channels > 1)
	{
		if (alIsBuffer(buffer))
			alDeleteBuffers(1, &buffer);
		GfError("OpenAL Error: Unsupported stereo sample %s\n", filename);
		is_enabled = false;
		return;
	}

	// Map WAV header to OpenAL format
	ALenum format;
	switch(wavspec.format)
	{
		case AUDIO_U8:
		case AUDIO_S8:
			format = AL_FORMAT_MONO8;
			break;
		case AUDIO_U16:
		case AUDIO_S16:
			format = AL_FORMAT_MONO16;
			break;
		default:
			SDL_FreeWAV(wavbuf);
			if (alIsBuffer(buffer))
				alDeleteBuffers(1, &buffer);
			GfError("OpenAL Error: Unsupported WAV format %d for %s (not among U8, S8, U16, S16)\n",
					wavspec.format, filename);
			is_enabled = false;
			return;
	}

	alBufferData(buffer, format, wavbuf, wavlen, wavspec.freq);
	error = alGetError();
	if (error != AL_NO_ERROR)
	{
		GfError("OpenAL Error: %d, alBufferData %s\n", error, filename);
		SDL_FreeWAV(wavbuf);
		if (alIsBuffer(buffer)) {
			alDeleteBuffers(1, &buffer);
			alGetError();
		}
		is_enabled = false;
		return;
	}
	
	SDL_FreeWAV(wavbuf);
	
	if (!static_pool) {
		is_enabled = true;
		return;
	}
	
	if (!sitf->getStaticSource(&source)) {
		is_enabled = false;
		printf("    No static sources left: %s\n", filename);
		if (alIsBuffer(buffer)) {
			alDeleteBuffers(1, &buffer);
			alGetError();
		}
		return;
	} else {
		is_enabled = true;	
	}

	alSourcefv (source, AL_POSITION, source_position);
	error = alGetError();
	if (error != AL_NO_ERROR) {
		printf("OpenAL Error: %d, alSourcefv AL_POSITION %s\n", error, filename);
	}

	alSourcefv (source, AL_VELOCITY, source_velocity);
	error = alGetError();
	if (error != AL_NO_ERROR) {
		printf("OpenAL Error: %d, alSourcefv AL_VELOCITY %s\n", error, filename);
	}

	alSourcei (source, AL_BUFFER, buffer);
	error = alGetError();
	if (error != AL_NO_ERROR) {
		printf("OpenAL Error: %d, alSourcei AL_BUFFER %s\n", error, filename);
	}

	alSourcei (source, AL_LOOPING, loop);
	if (error != AL_NO_ERROR) {
		printf("OpenAL Error: %d, alSourcei AL_LOOPING %s\n", error, filename);
	}

	alSourcef (source, AL_MAX_DISTANCE, MAX_DISTANCE);
	if (error != AL_NO_ERROR) {
		printf("OpenAL Error: %d, alSourcef AL_MAX_DISTANCE %s\n", error, filename);
	}

	alSourcef (source, AL_REFERENCE_DISTANCE, REFERENCE_DISTANCE);
	if (error != AL_NO_ERROR) {
		printf("OpenAL Error: %d, alSourcef AL_REFERENCE_DISTANCE %s\n", error, filename);
	}

	alSourcef (source, AL_ROLLOFF_FACTOR, ROLLOFF_FACTOR);
	if (error != AL_NO_ERROR) {
		printf("OpenAL Error: %d, alSourcef AL_ROLLOFF_FACTOR %s\n", error, filename);
	}

	alSourcef (source, AL_GAIN, 0.0f);
	if (error != AL_NO_ERROR) {
		printf("OpenAL Error: %d, alSourcef AL_GAIN %s\n", error, filename);
	}
}

OpenalSound::~OpenalSound()
{
	if (alIsSource(source)) {
		alSourceStop (source);
		alDeleteSources(1, &source);
	}
	if (alIsBuffer(buffer)) {
		alDeleteBuffers(1, &buffer);
	}
}

void OpenalSound::setVolume (float vol)
{
	this->volume = vol;
}

void OpenalSound::setPitch(float pitch)
{
	this->pitch = pitch;
}

void OpenalSound::setLPFilter(float lp)
{
	this->lowpass = lp;
}

void OpenalSound::setReferenceDistance(float dist)
{
	if (static_pool) {
		if (is_enabled) {
			alSourcef (source, AL_REFERENCE_DISTANCE, dist);
		}
	} else {
		if (itf->getSourcePool()->isSourceActive(this, &poolindex)) {
			alSourcef (source, AL_REFERENCE_DISTANCE, dist);		
			REFERENCE_DISTANCE = dist;
		}
	}
}

void OpenalSound::setSource (sgVec3 p, sgVec3 u)
{
	for (int i=0; i<3; i++) {
		source_position[i] = p[i];
		source_velocity[i] = u[i];
	}
}

void OpenalSound::getSource(sgVec3 p, sgVec3 u)
{
	for (int i=0; i<3; i++) {
		p[i] = source_position[i];
		u[i] = source_velocity[i];
	}
}

void OpenalSound::play()
{
	start();
}

void OpenalSound::start()
{
	if (static_pool) {
		if (is_enabled) {
			if (!playing) {
				if (loop) {
					playing = true;
				}
				alSourcePlay (source);
			}
		}
	} else {
		// shared source.
		bool needs_init;
		if (itf->getSourcePool()->getSource(this, &source, &needs_init, &poolindex)) {
			if (needs_init) {
				// Setup source.
				alSourcefv (source, AL_POSITION, source_position);
				alSourcefv (source, AL_VELOCITY, source_velocity);
				alSourcei (source, AL_BUFFER, buffer);
				alSourcei (source, AL_LOOPING, loop);
				alSourcef (source, AL_MAX_DISTANCE, MAX_DISTANCE);
				alSourcef (source, AL_REFERENCE_DISTANCE, REFERENCE_DISTANCE);
				alSourcef (source, AL_ROLLOFF_FACTOR, ROLLOFF_FACTOR);
				alSourcef (source, AL_GAIN, 0.0f);
			}

			// play
			if (!playing) {
				if (loop) {
					playing = true;
				}
				alSourcePlay (source);
			}
		}
	}
}

void OpenalSound::stop()
{
	if (static_pool) {
		if (is_enabled) {
			if (playing) {
				playing = false;
				alSourceStop (source);
			}
		}
	} else {
		// Shared source.	
		if (itf->getSourcePool()->releaseSource(this, &poolindex)) {
			if (playing) {
				playing = false;
				alSourceStop (source);
			}
		}
	}
}

void OpenalSound::resume()
{
	if (paused) {
		paused = false;
		alSourcePlay (source);
	}
}


void OpenalSound::pause()
{
	if (!paused) {
		paused = true;
		alSourcePause (source);
	}
}

void OpenalSound::update ()
{
    static const ALfloat zero_velocity[3] = {0.0f, 0.0f, 0.0f};
	if (static_pool) {
		if (is_enabled) {
			alSourcefv (source, AL_POSITION, source_position);
			alSourcefv (source, AL_VELOCITY, zero_velocity);
			alSourcef (source, AL_PITCH, pitch);
			alSourcef (source, AL_GAIN, volume);
		}
	} else {
		if (itf->getSourcePool()->isSourceActive(this, &poolindex)) {
			alSourcefv (source, AL_POSITION, source_position);
			alSourcefv (source, AL_VELOCITY, zero_velocity);
			alSourcef (source, AL_PITCH, pitch);
			alSourcef (source, AL_GAIN, volume);		
		}
	}
}
