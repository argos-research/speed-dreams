/***************************************************************************
                guieventloop.cpp -- Event loop for GfuiApplications
                             -------------------
    created              : Thu Mar 8 10:00:00 CEST 2006
    copyright            : (C) 2006 by Brian Gavin ; 2008, 2010 Jean-Philippe Meuret
    web                  : http://www.speed-dreams.org
    version              : $Id: guieventloop.cpp 6395 2016-04-04 03:40:56Z beaglejoe $
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

#ifdef WEBSERVER
#include "webserver.h"

extern NotificationManager notifications;
extern TGFCLIENT_API WebServer webServer;
#endif //WEBSERVER



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
#if SDL_JOYSTICK
	void (*cbJoystickAxis)(int joy, int axis, float value);
	void (*cbJoystickButton)(int joy, int button, int value);
#endif

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
	//printf("Key %x State %x mod %x\n", code, state, modifier);
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
#if SDL_JOYSTICK
void GfuiEventLoop::injectJoystickAxisEvent(int joy, int axis, float value)
{
	if (_pPrivate->cbJoystickAxis)
		_pPrivate->cbJoystickAxis(joy, axis, value);
}

void GfuiEventLoop::injectJoystickButtonEvent(int joy, int button, int value)
{
	if (_pPrivate->cbJoystickButton)
		_pPrivate->cbJoystickButton(joy, button, value);
}
#endif

// The event loop itself.
void GfuiEventLoop::operator()()
{
	SDL_Event event; // Event structure
#if SDL_MAJOR_VERSION >= 2
	static int unicode = 0;
   static SDL_Keymod modifier = KMOD_NONE;
   static SDL_Keycode keysym = SDLK_UNKNOWN;
#endif

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
#if SDL_MAJOR_VERSION < 2
					   injectKeyboardEvent(event.key.keysym.sym, event.key.keysym.mod, 0,event.key.keysym.unicode);
#else
					if((event.key.keysym.sym & SDLK_SCANCODE_MASK) == SDLK_SCANCODE_MASK)
					{
						injectKeyboardEvent(event.key.keysym.sym, event.key.keysym.mod, 0,0);
					}
					else if(false == isprint(event.key.keysym.sym))
					{
						injectKeyboardEvent(event.key.keysym.sym, event.key.keysym.mod, 0,0);
					}
					else if((event.key.keysym.mod & KMOD_CTRL)
							||(event.key.keysym.mod & KMOD_ALT)
							||(event.key.keysym.mod & KMOD_GUI))
					{
						injectKeyboardEvent(event.key.keysym.sym, event.key.keysym.mod, 0,0);
					}
					else
					{
						//GfLogDebug("SDL_KEYDOWN: %c\r\n",(char)event.key.keysym.sym);
						keysym = event.key.keysym.sym;
					}
#endif
					break;

#if SDL_MAJOR_VERSION >= 2
				case SDL_TEXTINPUT:
					unicode = (int)(event.text.text[0]);
					modifier = SDL_GetModState();
					injectKeyboardEvent(keysym, modifier, 0, unicode);
					//GfLogDebug("SDL_TEXTINPUT: %c %X\r\n",(char)unicode,modifier);
					break;
#endif

				case SDL_KEYUP:
#if SDL_MAJOR_VERSION < 2
					injectKeyboardEvent(event.key.keysym.sym, event.key.keysym.mod, 1,event.key.keysym.unicode);
#else
					injectKeyboardEvent(event.key.keysym.sym, event.key.keysym.mod, 1,0);
					//GfLogDebug("SDL_KEYUP: %c\r\n",(char)event.key.keysym.sym);
#endif
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

#if SDL_MAJOR_VERSION >= 2
#if SDL_JOYSTICK
				case SDL_JOYAXISMOTION:
					injectJoystickAxisEvent(event.jaxis.which, event.jaxis.axis, (float) event.jaxis.value / 32768);
					break;

				case SDL_JOYBUTTONDOWN:
					injectJoystickButtonEvent(event.jbutton.which, event.jbutton.button, SDL_PRESSED);
					break;

				case SDL_JOYBUTTONUP:
					injectJoystickButtonEvent(event.jbutton.which, event.jbutton.button, 0);
					break;
#endif
				case SDL_WINDOWEVENT_EXPOSED:
#else
				case SDL_VIDEOEXPOSE:
#endif
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
#if SDL_JOYSTICK
void GfuiEventLoop::setJoystickAxisCB(void (*func)(int joy, int axis, float value))
{
	_pPrivate->cbJoystickAxis = func;
}

void GfuiEventLoop::setJoystickButtonCB(void (*func)(int joy, int button, int value))
{
	_pPrivate->cbJoystickButton = func;
}
#endif

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
	#ifdef WEBSERVER
	std::clock_t currentTime =  std::clock();
	
	//run the webserver update process and ui at 30 FPS
	if( ( currentTime - notifications.animationLastExecTime ) > 0,033333333 ){

		webServer.updateAsyncStatus();
		notifications.updateStatus();		

	}
	#endif //WEBSERVER
	
	if (_pPrivate->cbDisplay)
		_pPrivate->cbDisplay();
}


void GfuiEventLoop::redisplay()
{
	#ifdef WEBSERVER
	//temp
	_pPrivate->bRedisplay=true;
	#endif //WEBSERVER	

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
