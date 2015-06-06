/***************************************************************************
    file                 : Sound.h
    created              : Tue Jul 18 19:57:35 CEST 2011
    copyright            : (C) 2005 Christos Dimitrakakis
    email                : dimitrak@idiap.ch
    version              : $Id: Sound.h 4965 2012-09-29 14:34:55Z pouillot $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef SOUND_H
#define SOUND_H

#define VOLUME_SLOT 0
#define PITCH_SLOT 1
#define FILTER_SLOT 2


#include <plib/sg.h>

#include "sound_defines.h"

class SoundInterface;


/** A generic sound.  The aim is to have a more or less
 * identical interface to sounds, no matter what the backend is. In
 * practice, there are some minor differences across interfaces. In
 * particular, for Plib sounds, setting pitch and volume actually sets
 * the pitch/volume of the sound that we are listening at, while for
 * Openal sounds, it sets only the pitch/volume of the source
 * sound. Thus, the interface is not consistent across
 * backends. Making it consistent is not easily doable because some
 * things simply work too differently. Currently I have traded
 * complexity in Sound for replicating some code in the two
 * SoundInterface implementations: OpenalSoundInterface and
 * PlibSoundInterface both take into account the individual
 * pecularities of each PlibSound and OpenalSound. However,
 * if it is deemed necessary later on, it is possible to make the
 * interface consistent.
 */
class Sound
{
 protected:

	class SoundInterface* iface; ///< Handler to the interface
	int flags; ///< Flags relating to what effects are to be used.
	float MAX_VOL; ///< Maximum volume
	float volume; ///< Current volume
	float pitch; ///< Current pitch
	float lowpass; ///< Current low pass filter
	bool loop; ///< Whether it's a looping sound
	bool playing; ///< Sound is playing
    bool paused; ///< sound is paused

 public:

	/// Construct a sound.
	Sound(int flags = (ACTIVE_VOLUME|ACTIVE_PITCH), bool loop = false);
	
    /// Destructor
	virtual ~Sound();
	
	virtual void setVolume(float vol);
	virtual void setPitch(float pitch);
	virtual void setLPFilter(float lp);
	virtual void setSource(sgVec3 p, sgVec3 u);
	virtual void setReferenceDistance (float dist);
	//virtual void setListener(sgVec3 p, sgVec3 u) = 0;

	virtual float getVolume() const;
	virtual float getPitch() const;
	virtual float getLPfilter() const;
	virtual void getSource (sgVec3 p, sgVec3 u) const;

	virtual void play() = 0;
	virtual void start() = 0;
	virtual void stop() = 0;
	virtual void resume() = 0;
	virtual void pause() = 0;
	virtual void update() = 0;
	
	virtual bool isPlaying() const;
	virtual bool isPaused() const;
};

/** Sound source management.
 *
 * The sound source is different from the sound itself. It should
 * describe the position, speed, environment and other aspects of the
 * sound's source. Each sound source can emit many different actual
 * sounds.
 * 
 * Since sources can be coupled to particular listeners, it is in
 * principle possible to implement multiple listeners, something which
 * should be extremely useful for same-computer multiplayer games.
 */
class SoundSource
{
 public:
	
	sgVec3 p_lis; ///< listener position for this source
	sgVec3 u_lis; ///< listener velocity for this source
	sgVec3 p_src; ///< source position
	sgVec3 u_src; ///< source velocity;
	float a; ///< Environmental attenuation
    float f; ///< Environmental frequency shift
    float lp; ///< Environmental filtering
	SoundSource();
	void update();
	void setSource(sgVec3 p, sgVec3 u);
	void setListener (sgVec3 p, sgVec3 u);
};

#endif
