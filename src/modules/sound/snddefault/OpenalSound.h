/***************************************************************************
    file                 : OpenalSound.h
    created              : Tue Apr 5 19:57:35 CEST 2005
    copyright            : (C) 2005 Christos Dimitrakakis
    email                : dimitrak@idiap.ch
    version              : $Id: OpenalSound.h 6098 2015-08-30 23:30:51Z beaglejoe $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef OPENAL_SOUND_H
#define OPENAL_SOUND_H

#if defined(__APPLE__) && !defined(USE_MACPORTS)
#include"al.h"
#include"alc.h"
#else
#include <AL/al.h>
#include <AL/alc.h>
#endif

#include "Sound.h"

class OpenalSoundInterface;


class OpenalSound : public Sound {
protected:
	ALuint buffer; ///< buffer id
	ALuint source; ///< source id
	ALfloat source_position[3]; ///< source position
	ALfloat source_velocity[3]; ///< source velocity
	ALfloat zeroes[3]; ///< just a vector of 0s
	ALfloat back[6]; ///< direction of back
	ALfloat front[6]; ///< direction of front 
	ALfloat MAX_DISTANCE; ///< maximum allowed distance
	ALfloat MAX_DISTANCE_LOW; ///< maximum allowed distance
	ALfloat REFERENCE_DISTANCE; ///< reference distance for sound
	ALfloat ROLLOFF_FACTOR; ///< how fast we need to roll off
	int poolindex; ///< which pool the sound is assigned to
	OpenalSoundInterface* itf; ///< Handle to the interface
	bool static_pool;	///< dynamic or static source assignment?
	bool is_enabled;	///< is it available at all?
public:
	OpenalSound(const char* filename,
				OpenalSoundInterface* sitf,
				int flags = (ACTIVE_VOLUME|ACTIVE_PITCH),
				bool loop = false, bool static_pool = true);
	virtual ~OpenalSound();
	virtual void setVolume(float vol);
	virtual void setPitch(float pitch);
	virtual void setLPFilter(float lp);
	virtual void setSource(sgVec3 p, sgVec3 u);
	virtual void getSource (sgVec3 p, sgVec3 u);
	virtual void setReferenceDistance (float dist);
	//virtual void setListener (sgVec3 p, sgVec3 u);
	virtual void play();
	virtual void start();
	virtual void stop();
	virtual void resume();
	virtual void pause();
	virtual void update();
};

#endif

