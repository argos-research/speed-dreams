/***************************************************************************
                guieventloop.cpp -- Event loop for GfuiApplications
                             -------------------                                         
    created              : Thu Mar 8 10:00:00 CEST 2006
    copyright            : (C) 2006 by Brian Gavin ; 2008, 2010 Jean-Philippe Meuret
    web                  : http://www.speed-dreams.org
    version              : $Id: guieventloop.cpp 4761 2012-06-22 04:58:08Z mungewell $
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <SDL.h>

#include "tgfclient.h"


// Private data (pimp pattern) =============================================
class GfuiEventLoop::Private
{
 public:

	//!  Constructor.
	Private();
	
 public: // Public data members.

	// Callback function pointers.
	void (*cbMouseButton)(int button, int state, int x, int y);
	void (*cbMouseMotion)(int x, int y);
	void (*cbMousePassiveMotion)(int x, int y);
	
	void (*cbDisplay)(void);
	void (*cbReshape)(int width, int height);

	// Variables.
	bool bRedisplay; // Flag to say if a redisplay is necessary.
};

GfuiEventLoop::Private::Private()
: cbMouseButton(0), cbMouseMotion(0), cbMousePassiveMotion(0),
  cbDisplay(0), cbReshape(0), bRedisplay(false)
{
}

// GfuiEventLoop class ============================================================

GfuiEventLoop::GfuiEventLoop()
: GfEventLoop()
{
	_pPrivate = new Private;
}

GfuiEventLoop::~GfuiEventLoop()
{
	delete _pPrivate;
}

void GfuiEventLoop::injectKeyboardEvent(int code, int modifier, int state,
										int unicode, int x, int y)
{
#ifndef WIN32
	// Hard-coded Alt+Enter shortcut, to enable the user to quit/re-enter
	// the full-screen mode ; as in SDL's full screen mode, events never reach
	// the Window Manager, we need this trick for the user to enjoy
	// its WM keyboard shortcuts (didn't find any other way yet).
	if (code == SDLK_RETURN	&& (modifier & KMOD_ALT) && state == 0)
	{
		if (!GfScrToggleFullScreen())
			GfLogError("Failed to toggle on/off the full-screen mode\n");
	}
	else
#endif
	{
		SDL_GetMouseState(&x, &y);
		GfEventLoop::injectKeyboardEvent(code, modifier, state, unicode, x, y);
	}
}

void GfuiEventLoop::injectMouseMotionEvent(int state, int x, int y)
{
	if (state == 0)
	{
		if (_pPrivate->cbMousePassiveMotion)
			_pPrivate->cbMousePassiveMotion(x, y);
	}
	else
	{
		if (_pPrivate->cbMouseMotion)
			_pPrivate->cbMouseMotion(x, y);
	}
}

void GfuiEventLoop::injectMouseButtonEvent(int button, int state, int x, int y)
{
	if (_pPrivate->cbMouseButton)
		_pPrivate->cbMouseButton(button, state, x, y);
}

// The event loop itself.
void GfuiEventLoop::operator()()
{
	SDL_Event event; // Event structure

	// Check for events.
	while (!quitRequested())
	{  	
		// Loop until there are no events left in the queue.
		while (!quitRequested() && SDL_PollEvent(&event))
		{
		    // Process events we care about, and ignore the others.
			switch(event.type)
			{  
				case SDL_KEYDOWN:
					injectKeyboardEvent(event.key.keysym.sym, event.key.keysym.mod, 0,
										event.key.keysym.unicode);
					break;
				
				case SDL_KEYUP:
					injectKeyboardEvent(event.key.keysym.sym, event.key.keysym.mod, 1,
										event.key.keysym.unicode);
					break;

				case SDL_MOUSEMOTION:
					injectMouseMotionEvent(event.motion.state, event.motion.x, event.motion.y);
					break;

				
				case SDL_MOUSEBUTTONDOWN:
				case SDL_MOUSEBUTTONUP:
					injectMouseButtonEvent(event.button.button, event.button.state,
										   event.button.x, event.button.y);
					break;

				case SDL_QUIT:
					postQuit();
					break;
				
				case SDL_VIDEOEXPOSE:
					forceRedisplay();
					break;
			}
		}

		if (!quitRequested())
		{
			// Recompute if anything to.
			recompute();

			// Redisplay if anything to.
			redisplay();
		}
	}

	GfLogTrace("Quitting GFUI event loop.\n");
}

void GfuiEventLoop::setMouseButtonCB(void (*func)(int button, int state, int x, int y))
{
	_pPrivate->cbMouseButton = func;
}

void GfuiEventLoop::setMouseMotionCB(void (*func)(int x, int y))
{
	_pPrivate->cbMouseMotion = func;
}

void GfuiEventLoop::setMousePassiveMotionCB(void (*func)(int x, int y))
{
	_pPrivate->cbMousePassiveMotion = func;
}

void GfuiEventLoop::setRedisplayCB(void (*func)(void))
{
	_pPrivate->cbDisplay = func;
}

void GfuiEventLoop::setReshapeCB(void (*func)(int width, int height))
{
	_pPrivate->cbReshape = func;
}

void GfuiEventLoop::postRedisplay(void)
{
	_pPrivate->bRedisplay = true;
}

void GfuiEventLoop::forceRedisplay()
{
	if (_pPrivate->cbDisplay)
		_pPrivate->cbDisplay();
}

void GfuiEventLoop::redisplay()
{
	// Refresh display if requested and if any redisplay CB.
	if (_pPrivate->bRedisplay)
	{
		// Acknowledge the request
		// (Note: do it before forceRedisplay(), in case it calls postRedisplay() ;-).
		_pPrivate->bRedisplay = false;

		// Really call the redisplay call-back if any.
		forceRedisplay();
	}
}
