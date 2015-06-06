/***************************************************************************
    file                 : PlibSoundInterface.h
    created              : Tue Jul 18 19:57:35 CEST 2011
    copyright            : (C) 2005 Christos Dimitrakakis, Bernhard Wymann
    email                : dimitrak@idiap.ch
    version              : $Id: PlibSoundInterface.h 4965 2012-09-29 14:34:55Z pouillot $

***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef PLIB_SOUND_INTERFACE_H
#define PLIB_SOUND_INTERFACE_H

#include <plib/sl.h>

#include "SoundInterface.h"


class PlibSoundInterface : public SoundInterface
{
	typedef struct SoundChar_
	{
		float f; //frequency modulation
		float a; //amplitude modulation
	} SoundChar;
	
 protected:
	
	slScheduler* sched;
	std::vector<Sound*> sound_list;
	SoundPri* engpri;
	SoundSource* car_src;
	SoundSource tyre_src[4];
	void setMaxSoundCar(CarSoundData** car_sound_data, QueueSoundMap* smap);
	
 public:
	
	PlibSoundInterface(float sampling_rate, int n_channels);
	virtual ~PlibSoundInterface();
	virtual void setNCars(int n_cars);
	virtual slScheduler* getScheduler();
	virtual Sound* addSample(const char* filename, int flags = (ACTIVE_VOLUME|ACTIVE_PITCH),
							 bool loop = false, bool static_pool = true);
	virtual void update(CarSoundData** car_sound_data, int n_cars,
						sgVec3 p_obs, sgVec3 u_obs, sgVec3 c_obs = NULL, sgVec3 a_obs = NULL);
};

#endif /* PLIB_SOUND_INTERFACE_H */
