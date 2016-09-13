/***************************************************************************

    file                 : snddefault.cpp 
    created              : Thu Aug 17 23:19:19 CEST 2000
    copyright            : (C) 2000 by Eric Espie
    email                : torcs@free.fr
    version              : $Id: snddefault.cpp 6097 2015-08-30 23:12:09Z beaglejoe $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "snddefault.h"
#include "grsound.h"

// The SndDefault singleton.
SndDefault* SndDefault::_pSelf = 0;

int openGfModule(const char* pszShLibName, void* hShLibHandle)
{
	// Instanciate the (only) module instance.
	SndDefault::_pSelf = new SndDefault(pszShLibName, hShLibHandle);

	// Register it to the GfModule module manager if OK.
	if (SndDefault::_pSelf)
		GfModule::register_(SndDefault::_pSelf);

	// Report about success or error.
	return SndDefault::_pSelf ? 0 : 1;
}

int closeGfModule()
{
	// Unregister it from the GfModule module manager.
	if (SndDefault::_pSelf)
		SndDefault::unregister(SndDefault::_pSelf);
	
	// Delete the (only) module instance.
	delete SndDefault::_pSelf;
	SndDefault::_pSelf = 0;

	// Report about success or error.
	return 0;
}

SndDefault& SndDefault::self()
{
	// Pre-condition : 1 successfull openGfModule call.
	return *_pSelf;
}

SndDefault::SndDefault(const std::string& strShLibName, void* hShLibHandle)
: GfModule(strShLibName, hShLibHandle)
{
}

SndDefault::~SndDefault()
{
	// Terminate the PLib SSG layer.
	//delete _pDefaultSSGLoaderOptions;
}

// Implementation of ISoundEngine ****************************************

void SndDefault::init(Situation* s){
    grInitSound(s,s->_ncars);
}
void SndDefault::shutdown(){
    grShutdownSound();
}
void SndDefault::refresh(Situation *s, Camera	*camera){
    grRefreshSound(s, camera);
}

void SndDefault::mute(bool bOn)
{
	::grMuteSound(bOn);
}
