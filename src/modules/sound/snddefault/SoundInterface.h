/***************************************************************************
    file                 : SoundInterface.h
    created              : Tue Apr 5 19:57:35 CEST 2005
    copyright            : (C) 2005 Christos Dimitrakakis, Bernhard Wymann
    email                : dimitrak@idiap.ch
    version              : $Id: SoundInterface.h 4965 2012-09-29 14:34:55Z pouillot $

***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef SOUND_INTERFACE_H
#define SOUND_INTERFACE_H

#include <plib/sg.h>
#include <vector>

#include "Sound.h"
#include "QSoundChar.h"
#include "snddefault.h"


#define VOLUME_CUTOFF 0.001f


class CarSoundData;

/** A queue containing mappings between sounds and sound sources.
 *
 * Provides a mapping various sound sources and actual sounds. This is
 * used for the case where we have many sound sources emitting exactly
 * the same sound and where we don't allow more than 1 source to play
 * simultaneously. This structure can be used to sort competing
 * sources in order to decide which one is going to take priority.
 *
 * \sa SortSingleQueue(), SetMaxSoundChar()
 */
typedef struct QueueSoundMap_ {
	QSoundChar CarSoundData::*schar; ///< The calculated sound characteristic
	Sound* snd; ///< The raw sound.
	float max_vol; ///< Max.
	int id; ///< The id of the car producing the sound, used for retrieving doppler effects etc
} QueueSoundMap;

/** Current state of sound*/
enum SoundPriState {
	None=0x0, Loaded, Playing, Paused, Stopped, Cleared
};

/** Sound priority, used to sort cars according to amplitude attenuation */
typedef struct SoundPri_ {
	float a; ///< amplitude
	int id; ///< car ID.
} SoundPri;

/// Sound interface
class SoundInterface {
 protected:
	float sampling_rate; ///< sampling rate
	int n_channels; ///< number of channels
	int n_engine_sounds; ///< number of simultaneous engines
	int curCrashSnd; ///< holds current crash sound used - the sound cycles
	Sound* skid_sound[4]; ///< set of skid sounds, one per tyre
	Sound* road_ride_sound; ///< rolling on normal road
	Sound* grass_ride_sound; ///< rolling on dirt/grass
	Sound* grass_skid_sound; ///< skidding on dirt/grass
	Sound* metal_skid_sound; ///< metal skidding on metal 
	Sound* axle_sound; ///< axle/gear spinning sound
	Sound* turbo_sound; ///< turbo spinning sound
	Sound* backfire_loop_sound; ///< exhaust backfire sound
	Sound* crash_sound[NB_CRASH_SOUND]; ///< list of crash sounds
	Sound* bang_sound; ///< sounds when suspension fully compressed
	Sound* bottom_crash_sound; ///< bang when crashing from a great height
	Sound* backfire_sound; ///< one-off backfire sound
	Sound* gear_change_sound; ///< sound when changing gears

	std::vector<Sound*> sound_list; ///< keeps track of sounds used
	SoundPri* engpri; ///< the engine priority, used for sorting

	/// The following are mappings for sound prioritisation
	QueueSoundMap road; 
	QueueSoundMap grass; 
	QueueSoundMap grass_skid;
	QueueSoundMap metal_skid;
	QueueSoundMap backfire_loop;
	QueueSoundMap turbo;
	QueueSoundMap axle;

	/// Current global gain [0, 1] and mute flag. 
	float global_gain;
	bool silent;
	
	/** Find the max amplitude sound in car_sound_data and put it in smap  */
	void sortSingleQueue (CarSoundData** car_sound_data, QueueSoundMap* smap, int n_cars);

	/** Using the smap->id, get the appropriate entry in
	    car_sound_data and call apprioriate methods for smap->snd in order
	    to play the sound.*/
	void setMaxSoundCar(CarSoundData** car_sound_data, QueueSoundMap* smap);
	
 public:
	/// Make a new sound interface
	SoundInterface(float sampling_rate, int n_channels);

	/// Destructor - does nothing
	virtual ~SoundInterface() {}

	/// Set the number of cars - must be defined in children classes
	virtual void setNCars(int n_cars) = 0;

	/// Add a new sample - must be defined in children classes
	virtual Sound* addSample(const char* filename,
							 int flags = (ACTIVE_VOLUME|ACTIVE_PITCH),
							 bool loop = false, bool static_pool = true) = 0;

	/// initialised the pool of shared sources
	virtual void initSharedSourcePool();

	void setSkidSound (const char* sound_name);
	void setRoadRideSound (const char* sound_name);
	void setGrassRideSound (const char* sound_name);
	void setGrassSkidSound (const char* sound_name);
	void setMetalSkidSound (const char* sound_name);
	void setAxleSound (const char* sound_name);
	void setTurboSound (const char* sound_name);
	void setBackfireLoopSound (const char* sound_name);
	void setCrashSound (const char* sound_name, int index);
	void setBangSound (const char* sound_name);
	void setBottomCrashSound (const char* sound_name);
	void setBackfireSound (const char* sound_name);
	void setGearChangeSound (const char* sound_name);

	/// Update sound for a given observer.
	virtual void update(CarSoundData** car_sound_data,
						int n_cars, sndVec3 p_obs, sndVec3 u_obs, 
						sndVec3 c_obs = NULL, sndVec3 a_obs = NULL) = 0;

	virtual float getGlobalGain() const;

	virtual void setGlobalGain(float g);

	virtual void mute(bool bOn = true);
};


int sortSndPriority(const void* a, const void* b);

#endif /* SOUND_INTERFACE_H */
