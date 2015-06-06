/***************************************************************************
    file                 : Sound.cpp
    created              : Tue Apr 5 19:57:35 CEST 2005
    copyright            : (C) 2005 Christos Dimitrakakis, Bernhard Wymann
    email                : dimitrak@idiap.ch
    version              : $Id: Sound.cpp 4965 2012-09-29 14:34:55Z pouillot $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <tgf.h>

#include "Sound.h"
#include "SoundInterface.h"


/// Construct a sound.
Sound::Sound(int flags, bool loop)
{
	this->flags = flags;
	MAX_VOL = 1.0f;
	volume = 0.0f;
	pitch = 1.0f;
	lowpass = 1.0f;
	this->loop = loop;
	playing = false;
	paused = false;
}

/// Destructor
Sound::~Sound()
{
}

/// Set the volume \note effect not consistent across backends
void Sound::setVolume(float vol)
{
	this->volume = vol;
}

/// Set the pitch \note Effect not consistent across backends
void Sound::setPitch(float pitch)
{
	this->pitch = pitch;
}

/// Set the filter \note Effect not consistent across backends
void Sound::setLPFilter(float lp)
{
	this->lowpass = lp;
}

void Sound::setSource(sgVec3 p, sgVec3 u)
{
}

float Sound::getVolume() const
{
	return volume;
}

float Sound::getPitch() const
{
	return pitch;
}

float Sound::getLPfilter() const
{
	return lowpass;
}

void Sound::setReferenceDistance (float dist)
{
	// Do nothing implementation.
}

void Sound::getSource (sgVec3 p, sgVec3 u) const
{
	// Do nothing implementation.
}

/// True if the sound is playing.
bool Sound::isPlaying() const
{
	return playing;
}

/// True if the sound is paused.
bool Sound::isPaused()  const
{
	return paused;
}

/// Create a sound source
SoundSource::SoundSource()
{
	a = 0.0;
	f = 1.0;
	lp = 1.0;
}

/** Calculate environmental parameters for current situation.
 *
 * At the moment this
 */

void SoundSource::update()
{
	// Get relative speed/position vector
	sgVec3 u;
	sgVec3 p;
	float u_rel = 0.0f;
	float u_rel_src = 0.0f;
	float u_rel_lis = 0.0f;
	float p_rel = 0.0f;
	int i;
	for (i=0; i<3; i++) {
		u[i] = u_src[i] - u_lis[i];
		p[i] = p_src[i] -  p_lis[i];
		p_rel += p[i]*p[i];
	}
    
	a = 1.0;
	f = 1.0f;
	lp = 1.0f;

	// Only the vector component on the LOV is significant
	//    u_rel = sqrt(u_rel);
	p_rel = 0.01f + sqrt(p_rel);
	float p_cosx = p[0]/p_rel;
	float p_cosy = p[1]/p_rel;
	float p_cosz = p[2]/p_rel;
	float p_x_comp = u[0]*p_cosx;
	float p_y_comp = u[1]*p_cosy;
	float p_z_comp = u[2]*p_cosz;
	float p_x_src = u_src[0]*p_cosx;
	float p_y_src = u_src[1]*p_cosy;
	float p_z_src = u_src[2]*p_cosz;
	float p_x_lis = u_lis[0]*p_cosx;
	float p_y_lis = u_lis[1]*p_cosy;
	float p_z_lis = u_lis[2]*p_cosz;
	u_rel = (p_y_comp + p_x_comp + p_z_comp);
	u_rel_src = (p_y_src + p_x_src + p_z_src);
	u_rel_lis = (p_y_lis + p_x_lis + p_z_lis);
	if (fabs(u_rel)>=0.9f*SPEED_OF_SOUND) {
		// Cut-off sound when relative speed approaches speed of sound.
		a = 0.0f;
		f = 1.0f;
		lp = 1.0f;
	} else {
		// attenuate and filter sound with distance
		// and shift pitch with speed
		float ref = 5.0f;
		float rolloff = 0.5f;
		float atten = ref / ( ref + rolloff * (p_rel - ref));
		//f = SPEED_OF_SOUND/(SPEED_OF_SOUND+u_rel);
		f = (tdble)((SPEED_OF_SOUND - u_rel_src)/(SPEED_OF_SOUND - u_rel_lis));
		a = atten;
		float atten_filter = MIN (atten, 1.0f);
		lp = exp(atten_filter - 1.0f);
	}

}

/** Set source position and velocity.
 */
void SoundSource::setSource(sgVec3 p, sgVec3 u)
{
	for (int i=0; i<3; i++) {
		p_src[i] = p[i];
		u_src[i] = u[i];
	}
}

/** Set listener position and velocity.
 */
void SoundSource::setListener (sgVec3 p, sgVec3 u)
{
	for (int i=0; i<3; i++) {
		p_lis[i] = p[i];
		u_lis[i] = u[i];
	}
}
