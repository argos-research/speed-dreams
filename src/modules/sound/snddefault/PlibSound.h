/***************************************************************************
    file                 : PlibSound.h
    created              : Tue Jul 18 19:57:35 CEST 2011
    copyright            : (C) 2005 Christos Dimitrakakis
    email                : dimitrak@idiap.ch
    version              : $Id: PlibSound.h 4965 2012-09-29 14:34:55Z pouillot $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef PLIB_SOUND_H
#define PLIB_SOUND_H

#include <plib/sl.h>

#include "Sound.h"


class PlibSound : public Sound {
protected:
	slSample* sample; ///< sample data
	slEnvelope* volume_env; ///< volume envelope
	slEnvelope* pitch_env; ///< pitch envelope
	slEnvelope* lowpass_env; ///< low pass filter envelope
	slScheduler* sched; ///< plib sl scheduler (see sl.h)
public:
	PlibSound(slScheduler* sched,
			  const char* filename,
			  int flags = (ACTIVE_VOLUME|ACTIVE_PITCH),
			  bool loop = false);
	virtual ~PlibSound();
	virtual void setVolume(float vol);
	//virtual void setSource(sgVec3 p, sgVec3 u);
	//virtual void setListener (sgVec3 p, sgVec3 u);
	virtual void play();
	virtual void start();
	virtual void stop();
	virtual void resume();
	virtual void pause();
	virtual void update();
};

#endif
