/***************************************************************************
    file                 : SoundInterface.h
    created              : Tue Jul 18 19:57:35 CEST 2011
    copyright            : (C) 2005 Christos Dimitrakakis, Bernhard Wymann
    email                : dimitrak@idiap.ch
    version              : $Id: OpenalSoundInterface.h 6098 2015-08-30 23:30:51Z beaglejoe $

***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef OPENAL_SOUND_INTERFACE_H
#define OPENAL_SOUND_INTERFACE_H

#if defined(__APPLE__) && !defined(USE_MACPORTS)
#include"al.h"
#include"alc.h"
#else
#include <AL/al.h>
#include <AL/alc.h>
#endif

#include "SoundInterface.h"

class SharedSourcePool;


class OpenalSoundInterface : public SoundInterface
{
	typedef struct SoundChar_
	{
		float f; //frequency modulation
		float a; //amplitude modulation
	} SoundChar;
	
 protected:
	
	SoundSource* car_src;
	SoundSource tyre_src[4];
	ALCcontext* originalcontext;
	ALCcontext* cc;
	ALCdevice* dev;
	int OSI_MAX_BUFFERS;
	int OSI_MAX_SOURCES;
	int OSI_MAX_STATIC_SOURCES;
	int n_static_sources_in_use;
	SharedSourcePool* sourcepool;
	static const int OSI_MIN_DYNAMIC_SOURCES;

    virtual void setMaxSoundCar(CarSoundData** car_sound_data, QueueSoundMap* smap);

 public:
	
	OpenalSoundInterface(float sampling_rate, int n_channels);
	virtual ~OpenalSoundInterface();
	virtual void setNCars(int n_cars);
	virtual Sound* addSample(const char* filename, int flags = (ACTIVE_VOLUME|ACTIVE_PITCH),
								  bool loop = false, bool static_pool = true);
	virtual void update(CarSoundData** car_sound_data, int n_cars,
						sgVec3 p_obs, sgVec3 u_obs, sgVec3 c_obs, sgVec3 a_obs);
	virtual void initSharedSourcePool();
	virtual bool getStaticSource(ALuint *source);

	virtual SharedSourcePool* getSourcePool(void);
	
	virtual void mute(bool bOn = true);
};

struct sharedSource {
	ALuint source;
	Sound* currentOwner;
	bool in_use;
};


class SharedSourcePool {
	public:
		SharedSourcePool(int nbsources):nbsources(nbsources) {
			pool = new sharedSource[nbsources];
			int i;
			for (i = 0; i < nbsources; i++) {
				pool[i].currentOwner = NULL;
				pool[i].in_use = false;
				alGenSources(1, &(pool[i].source));
				int error = alGetError();
				if (error != AL_NO_ERROR) {
					printf("OpenAL error, allocating dynamic source index %d\n", i);
					this->nbsources = i;
					break;
				}
			}
			GfLogInfo("  Dynamic Sources: requested: %d, created: %d\n", nbsources, this->nbsources);
		}
		
		virtual ~SharedSourcePool() {
			int i;
			for (i = 0; i < nbsources; i++) {
				alDeleteSources(1, &(pool[i].source));
				alGetError();
			}			
			delete [] pool;
		}

		bool getSource(Sound* sound, ALuint* source, bool* needs_init, int* index) {
			if (*index >= 0 && *index < nbsources) {
				if (sound == pool[*index].currentOwner) {
					// Resurrect source from pool.
					*source = pool[*index].source;
					*needs_init = false;
					pool[*index].in_use = true;
					return true;
				}
			}
			
			// TODO: Implement free list with ring buffer or whatever data structure
			// to go from O(n) to O(1). If the ordering is done well it will automatically
			// result in LRU (least recently used).
			int i, firstfree = -1;
			for (i = 0; i < nbsources; i++) {
				if (pool[i].in_use == false && firstfree < 0) {
					firstfree = i;
					break;
				}
			}
			
			if (firstfree < 0) {
				// No free source.
				return false;
			}

			pool[firstfree].currentOwner = sound;
			pool[firstfree].in_use = true;
			*source = pool[firstfree].source;
			*needs_init = true;
			*index = firstfree;
			return true;
		}

		bool releaseSource(Sound* sound, int* index) {
			if (*index >= 0 && *index < nbsources) {
				if (sound == pool[*index].currentOwner) {
					pool[*index].in_use = false;
					return true;
				}
			}
			return false;
		}

		bool isSourceActive(Sound* sound, int* index) {
			if (*index >= 0 && *index < nbsources &&
				sound == pool[*index].currentOwner &&
				true == pool[*index].in_use)
			{
				return true;
			} else {
				return false;
			}
		}

		int getNbSources(void) { return nbsources; }

	protected:
		int nbsources;
		sharedSource *pool;
};

#endif /* OPENAL_SOUND_INTERFACE_H */
