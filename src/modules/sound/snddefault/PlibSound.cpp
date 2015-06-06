/***************************************************************************
    file                 : Sound.cpp
    created              : Tue Apr 5 19:57:35 CEST 2005
    copyright            : (C) 2005 Christos Dimitrakakis, Bernhard Wymann
    email                : dimitrak@idiap.ch
    version              : $Id: PlibSound.cpp 4965 2012-09-29 14:34:55Z pouillot $

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

#include "PlibSound.h"


/// Create a new PLib sound. It requires a scheduler to be set up
/// and a filename to read data from.
PlibSound::PlibSound(slScheduler* sched, const char* filename, int flags, bool loop)
: Sound(flags, loop)
{
	this->sched = sched;
	sample = new slSample (filename, sched);
	if (flags & ACTIVE_VOLUME) {
		volume_env = new slEnvelope(1, SL_SAMPLE_ONE_SHOT);
	}
	if (flags & ACTIVE_PITCH) {
		pitch_env = new slEnvelope(1, SL_SAMPLE_ONE_SHOT);
	}
	if (flags & ACTIVE_LP_FILTER) {
		lowpass_env = new slEnvelope(1, SL_SAMPLE_ONE_SHOT);
	}
	if (loop) {
		sched->loopSample (sample);
	}
	if (flags & ACTIVE_VOLUME) {
		sched->addSampleEnvelope (sample, 0, VOLUME_SLOT, volume_env,
					  SL_VOLUME_ENVELOPE);
	}
	if (flags & ACTIVE_PITCH) {
		sched->addSampleEnvelope (sample, 0, PITCH_SLOT, pitch_env,
					  SL_PITCH_ENVELOPE);
	}
	if (flags & ACTIVE_LP_FILTER) {
		sched->addSampleEnvelope(sample, 0, FILTER_SLOT, lowpass_env,
					 SL_FILTER_ENVELOPE);
	}
	if (flags & ACTIVE_VOLUME) {
		volume_env->setStep(0, 0.0f, 0.0f);
	}
	if (flags & ACTIVE_PITCH) {
		pitch_env->setStep(0, 0.0f, 1.0f);
	}
	if (flags & ACTIVE_LP_FILTER) {
		lowpass_env->setStep(0, 0.0, 1.0f);
	}
}

/// Destructor.
PlibSound::~PlibSound()
{
	sched->stopSample(sample);
	if (flags & ACTIVE_VOLUME) {
		sched->addSampleEnvelope(sample, 0, VOLUME_SLOT, NULL,
					 SL_NULL_ENVELOPE);
		delete volume_env;
	}
	if (flags & ACTIVE_PITCH) {
		sched->addSampleEnvelope(sample, 0, PITCH_SLOT, NULL,
					 SL_NULL_ENVELOPE);
		delete pitch_env;
	}
	if (flags & ACTIVE_LP_FILTER) {
		sched->addSampleEnvelope(sample, 0, FILTER_SLOT, NULL,
					 SL_NULL_ENVELOPE);
		delete lowpass_env;
	}
	delete sample;
}

/** Set the volume.  Since plib does not support envelopes for
 * one-shot samples, we pre-adjust their volume
 */
void PlibSound::setVolume(float vol)
{
	if (vol > MAX_VOL) {
		vol = MAX_VOL;
	}
	this->volume = vol;

    if (!loop) {
        sample->adjustVolume (vol);
    }
}

/// Start the sample
void PlibSound::play()
{
	start();
}

/// Start the sample
void PlibSound::start()
{
	// TODO: consistency check?
	if (loop) {
		if (!playing) {
			playing = true;
			sched->loopSample (sample);
		}
	} else {
		playing = true;
		sched->playSample (sample);
	}
}

/// Stop the sample
void PlibSound::stop()
{
	if (playing) {
		playing = false;
		sched->stopSample (sample);
	}
}

/// Resume a paused sample.
void PlibSound::resume()
{
	sched->resumeSample (sample);
	paused = false;
}

/// Pause a sample
void PlibSound::pause()
{
	sched->pauseSample (sample);
	paused = true;
}


/** Update the plib sounds.
 * This should be called as often as possible from the main sound code,
 * probably by looping through all the sounds used.
 */
void PlibSound::update()
{
	if (flags & ACTIVE_VOLUME) {
		volume_env->setStep(0, 0.0f, volume);
	}
	if (flags & ACTIVE_PITCH) {
		pitch_env->setStep(0, 0.0f, pitch);

	}
	if (flags & ACTIVE_LP_FILTER) {
		lowpass_env->setStep(0, 0.0f, lowpass);
	}
}
