/***************************************************************************

    file                 : musicplayer.cpp
    created              : Fri Dec 23 17:35:18 CET 2011
    copyright            : (C) 2011 Bernhard Wymann
    email                : berniw@bluewin.ch
    version              : $Id: musicplayer.cpp 5485 2013-05-28 15:50:38Z beaglejoe $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "musicplayer.h"

#include <string.h>
#include <string>
#include <map>
#include <tgf.h>
#include "tgfclient.h"
#include <portability.h>
#include <sound.h>

#define MAX_MUSIC_PATH 1024

#include "oggsoundstream.h"
#include "openalmusicplayer.h"
 
static const char *musicDisabledStr = SND_VAL_MUSIC_STATE_DISABLED;

static bool enabled = true;
static char currentMusicfile[MAX_MUSIC_PATH] = {0};
static char defaultMusic[MAX_MUSIC_PATH] = {0}; //"data/music/main.ogg";
static float maxMusicVolume = 1.0;

#define NOMUSIC "None"
std::map<std::string,OpenALMusicPlayer*> mapOpenAlPlayers;
 
static SDL_mutex *mapMutex = NULL;

static void playMenuMusic();
static void pauseMenuMusic();
static void readConfig();

 static bool isEnabled()
 {
	return enabled;
}


// Path relative to CWD, e.g "data/music/main.ogg"
static SoundStream* getMenuSoundStream(char* oggFilePath)
{
	OggSoundStream* stream = new OggSoundStream(oggFilePath);
	return stream;
}


static OpenALMusicPlayer* getMusicPlayer(char* oggFilePath)
{
	OpenALMusicPlayer* player = NULL;
	
	SDL_LockMutex(mapMutex);
	const std::map<std::string, OpenALMusicPlayer*>::const_iterator itPlayers = mapOpenAlPlayers.find(oggFilePath);

	if (itPlayers == mapOpenAlPlayers.end()) {
		player = new OpenALMusicPlayer(getMenuSoundStream(oggFilePath));
		mapOpenAlPlayers[oggFilePath] = player;
		player->setvolume(maxMusicVolume);
		player->start();
	} else {
		player = mapOpenAlPlayers[oggFilePath];
	}
	SDL_UnlockMutex(mapMutex);
	return player;
}

static Uint32 sdlTimerFunc(Uint32 interval, void* /* pEvLoopPriv */)
{
	playMenuMusic();
	return 1;
}

SDL_TimerID timerId = 0;
static void playMenuMusic()
{
	const int nextcallinms = 100;
	SDL_LockMutex(mapMutex);
	std::map<std::string, OpenALMusicPlayer*>::const_iterator itPlayers = mapOpenAlPlayers.begin();
	while(itPlayers != mapOpenAlPlayers.end()) {
		OpenALMusicPlayer* player = itPlayers->second;
	
		if (player) {
			player->playAndManageBuffer();
		}
		itPlayers++;
	}
	SDL_UnlockMutex(mapMutex);
	if(timerId == 0){
		timerId = SDL_AddTimer(nextcallinms, sdlTimerFunc, (void*)NULL);
	}
}

void initMusic()
{
	readConfig();
	if (isEnabled()) {
		mapMutex = SDL_CreateMutex();
		(void)getMusicPlayer(defaultMusic);
		strcpy(currentMusicfile,defaultMusic);
		playMenuMusic();
	}
}


void shutdownMusic()
{
	if(timerId != 0){
		SDL_RemoveTimer(timerId);
		timerId = 0;
	}
	SDL_LockMutex(mapMutex);
	std::map<std::string, OpenALMusicPlayer*>::const_iterator itPlayers = mapOpenAlPlayers.begin();
	while(itPlayers != mapOpenAlPlayers.end()) {
		OpenALMusicPlayer* player = itPlayers->second;
		player->stop();
		player->rewind();
		itPlayers++;
	}
	itPlayers = mapOpenAlPlayers.begin();
	while(itPlayers != mapOpenAlPlayers.end()) {
		OpenALMusicPlayer* player = itPlayers->second;
		delete player;
		itPlayers++;
	}
	mapOpenAlPlayers.clear();
	SDL_UnlockMutex(mapMutex);
	SDL_DestroyMutex(mapMutex);
	mapMutex = NULL;
}

void pauseMenuMusic()
{

		if(timerId != 0){
		SDL_RemoveTimer(timerId);
		timerId = 0;
		}
		SDL_LockMutex(mapMutex);
		std::map<std::string, OpenALMusicPlayer*>::const_iterator itPlayers = mapOpenAlPlayers.begin();
		while(itPlayers != mapOpenAlPlayers.end()) {
			OpenALMusicPlayer* player = itPlayers->second;
			player->pause();
			itPlayers++;
		}
		SDL_UnlockMutex(mapMutex);
		

}

void playMusic(char* filename)
{
	if (isEnabled()) {
		OpenALMusicPlayer* player = NULL;
		if(filename != NULL) {
			if(0 == strcmp(NOMUSIC,filename)){
				strcpy(currentMusicfile,filename);
				GfLogInfo("Music changing to: %s \n", filename);
				pauseMenuMusic();
				return;
			}
			if(0 != strcmp(currentMusicfile,filename)){
				if(0 != strcmp(NOMUSIC,currentMusicfile)){
					player = getMusicPlayer(currentMusicfile);
					player->fadeout();
				}
				strcpy(currentMusicfile,filename);
				GfLogInfo("Music changing to: %s \n", filename);
				player = getMusicPlayer(filename);
				player->fadein();
			}
		} else {
			if(0 != strcmp(currentMusicfile,defaultMusic)){
				if(0 != strcmp(NOMUSIC,currentMusicfile)){
					player = getMusicPlayer(currentMusicfile);
					player->fadeout();
				}
				strcpy(currentMusicfile,defaultMusic);
				GfLogInfo("Music changing to: %s \n", defaultMusic);
				player = getMusicPlayer(defaultMusic);
				player->fadein();
			}
		}
		if(player) {
			player->resume();
		}
		playMenuMusic();
	}
}

void setDefaultMusic(const char* filename)
{
	if(0 != filename){
		if(strlen(filename) < MAX_MUSIC_PATH){
			if(0 != strcmp(defaultMusic,filename)){
				strcpy(defaultMusic,filename);
				GfLogInfo("Default Music changing to: %s \n", filename);
			}
		} else {
			GfLogError("Default Music File Path too long. [  %s  ]\n", filename);
		}
	} else {
		defaultMusic[0] = 0;
		GfLogInfo("Default Music changing to: %s \n", "NULL");
	}
}

static void readConfig()
{
	char fnbuf[1024];
	sprintf(fnbuf, "%s%s", GfLocalDir(), SND_PARAM_FILE);
	void *paramHandle = GfParmReadFile(fnbuf, GFPARM_RMODE_REREAD | GFPARM_RMODE_CREAT);
	const char *musicenabled = GfParmGetStr(paramHandle, SND_SCT_MUSIC, SND_ATT_MUSIC_STATE, musicDisabledStr);

	float music_volume = GfParmGetNum(paramHandle, SND_SCT_MUSIC, SND_ATT_MUSIC_VOLUME, "%", 100.0f);
	if (music_volume>100.0f) {
		music_volume = 100.0f;
	} 
	else if (music_volume < 0.0f) {
		music_volume = 0.0f;
	}
	maxMusicVolume = music_volume/100.0f;

	if (0 == strcmp(musicenabled, SND_VAL_MUSIC_STATE_ENABLED)) {
		enabled = true;
	} else {
		enabled = false;
	}
	const char* defmusic = GfParmGetStr(paramHandle, SND_SCT_MUSIC, SND_ATT_MUSIC_DEFAULT_MUSIC, "data/music/main.ogg");
	setDefaultMusic(defmusic);

	//TODO: Remove this hack after plib is deprecated
	// Using plib for the sound effects sometimes crashes when OpenAL already has the sound device
	const char* isplib = GfParmGetStr(paramHandle, SND_SCT_SOUND, SND_ATT_SOUND_STATE, "");
	if (!strcmp(isplib, "plib")) {
		enabled = false;
		GfLogInfo("Music player disabled for PLIB\n");
	}
	//TODO end of section to Remove

	GfLogInfo("(Re-)Initializing music player \n");
	GfParmReleaseHandle(paramHandle);
	paramHandle = NULL;
}

void setMusicVolume(float vol)
{
	if (vol < 0)
		vol = 0.0f;
	else if (vol > 1.0f)
		vol = 1.0f;
	
	maxMusicVolume = vol;

	GfLogInfo("Music maximum volume set to %.2f\n", maxMusicVolume);
}