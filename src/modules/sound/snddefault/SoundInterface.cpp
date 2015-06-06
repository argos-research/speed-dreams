/***************************************************************************

    file                 : PlibSoundInterface.cpp
    created              : Thu Apr 7 04:21 CEST 2005
    copyright            : (C) 2005 Christos Dimitrakakis
    email                : dimitrak@idiap.ch
    version              : $Id: SoundInterface.cpp 4965 2012-09-29 14:34:55Z pouillot $

***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "SoundInterface.h"
#include "CarSoundData.h"


SoundInterface::SoundInterface(float sampling_rate, int n_channels)
{
	this->sampling_rate = sampling_rate;
	this->n_channels = n_channels;
	int i;
	for (i = 0; i<4; i++) {
		skid_sound[i]=NULL;
	}
	road_ride_sound=NULL;
	grass_ride_sound=NULL;
	grass_skid_sound=NULL;
	metal_skid_sound=NULL;
	axle_sound=NULL;
	turbo_sound=NULL;
	backfire_loop_sound=NULL;
	for (i = 0; i<NB_CRASH_SOUND; i++) {
		crash_sound[i]=NULL;
	}
	curCrashSnd = 0;
	bang_sound=NULL;
	bottom_crash_sound=NULL;
	gear_change_sound=NULL;
	
	n_engine_sounds = n_channels - 12;

	static const int MAX_N_ENGINE_SOUNDS = 8;
	if (n_engine_sounds < 1) {
		n_engine_sounds = 1;
		fprintf (stderr, "Warning: maybe not enough available channels\n");
	} else if (n_engine_sounds > MAX_N_ENGINE_SOUNDS) {
		n_engine_sounds = MAX_N_ENGINE_SOUNDS;
	}
	
	global_gain = 1.0f;
	silent = false;
}

void SoundInterface::sortSingleQueue (CarSoundData** car_sound_data, QueueSoundMap* smap, int n_cars)
{
	float max_vol = 0.0f;
	int max_id = 0;
	for (int id=0; id<n_cars; id++) {
		CarSoundData* sound_data = car_sound_data[id];
		QSoundChar CarSoundData::*p2schar = smap->schar;
		QSoundChar* schar = &(sound_data->*p2schar);
		float vol = sound_data->attenuation * schar->a;
		if (vol > max_vol) {
			max_vol = vol;
			max_id = id;
		}
	}
	smap->id = max_id;
	smap->max_vol = max_vol;
}

void SoundInterface::setMaxSoundCar(CarSoundData** car_sound_data, QueueSoundMap* smap)
{
	int id = smap->id;
	float max_vol = smap->max_vol;
	QSoundChar CarSoundData::*p2schar = smap->schar;
	QSoundChar* schar = &(car_sound_data[id]->*p2schar);
	Sound* snd = smap->snd;

	sgVec3 p;
	sgVec3 u;

	car_sound_data[id]->getCarPosition(p);
	car_sound_data[id]->getCarSpeed(u);
	snd->setSource (p, u);
	snd->setVolume (schar->a);
	snd->setPitch (schar->f);
	snd->update();
	if (max_vol > VOLUME_CUTOFF) {
		snd->start();
	} else {
		snd->stop();
	}
}

void SoundInterface::initSharedSourcePool()
{
}

void SoundInterface::setSkidSound (const char* sound_name)
{
	for (int i=0; i<4; i++) {
		Sound* sound = addSample (sound_name, ACTIVE_VOLUME|ACTIVE_PITCH, true);
		skid_sound[i] = sound;
	}

}
void SoundInterface::setRoadRideSound (const char* sound_name)
{
	Sound* sound = addSample (sound_name, ACTIVE_VOLUME|ACTIVE_PITCH, true);
	road_ride_sound = sound;
}
void SoundInterface::setGrassRideSound (const char* sound_name)
{
	Sound* sound = addSample (sound_name, ACTIVE_VOLUME|ACTIVE_PITCH, true);
	grass_ride_sound = sound;
}
void SoundInterface::setGrassSkidSound (const char* sound_name)
{
	Sound* sound = addSample (sound_name, ACTIVE_VOLUME|ACTIVE_PITCH, true);
	grass_skid_sound = sound;
}
void SoundInterface::setMetalSkidSound (const char* sound_name)
{
	Sound* sound = addSample (sound_name, ACTIVE_VOLUME|ACTIVE_PITCH, true);
	metal_skid_sound = sound;
}
void SoundInterface::setAxleSound (const char* sound_name)
{
	Sound* sound = addSample (sound_name, ACTIVE_VOLUME|ACTIVE_PITCH, true);
	axle_sound = sound;
}
void SoundInterface::setTurboSound (const char* sound_name)
{
	Sound* sound = addSample (sound_name, ACTIVE_VOLUME|ACTIVE_PITCH, true);
	turbo_sound = sound;
}
void SoundInterface::setBackfireLoopSound (const char* sound_name)
{
	Sound* sound = addSample (sound_name, ACTIVE_VOLUME|ACTIVE_PITCH, true);
	backfire_loop_sound = sound;
}
void SoundInterface::setCrashSound (const char* sound_name, int index)
{
	Sound* sound = addSample (sound_name, 0, false);
	assert (index>=0 && index<NB_CRASH_SOUND);
	crash_sound[index] = sound;
}

void SoundInterface::setBangSound (const char* sound_name)
{
	Sound* sound = addSample (sound_name, 0, false);
	bang_sound = sound;
}

void SoundInterface::setBottomCrashSound (const char* sound_name)
{
	Sound* sound = addSample (sound_name, 0, false);
	bottom_crash_sound = sound;
}

void SoundInterface::setBackfireSound (const char* sound_name)
{
	Sound* sound = addSample (sound_name, 0, false);
	backfire_sound = sound;
}

void SoundInterface::setGearChangeSound (const char* sound_name)
{
	Sound* sound = addSample (sound_name, 0, false);
	gear_change_sound = sound;
}

float SoundInterface::getGlobalGain() const
{ 
	return silent ? 0 : global_gain; 
}

void SoundInterface::setGlobalGain(float g) 
{
	if (g < 0)
		g = 0.0f;
	else if (g > 1.0f)
		g = 1.0f;
	
	global_gain = g;

	GfLogInfo("Sound global gain set to %.2f\n", global_gain);
}

void SoundInterface::mute(bool bOn)
{
	silent = bOn;
	
	GfLogInfo("Sound %s\n", silent ? "paused" : "restored");
}
