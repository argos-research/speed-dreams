/***************************************************************************

    file                 : OsgGraphicsWindows.cpp
    created              : Thu Sep 15 15:23:49 CEST 2015
    copyright            : (C)2015 by Xavier Bertaux
    email                : bertauxx@yahoo.fr
    version              : $Id: OsgGraphicsWindows.cpp 6125 2015-09-15 06:02:49Z torcs-ng $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   Based on OpenMW code                                                  *
 *                                                                         *
 ***************************************************************************/

#include "OsgGraphicsWindow.h"

#include <SDL_video.h>

#include <osg/DeleteHandler>
#include <osg/Version>

namespace OSGUtil
{

OsgGraphicsWindowSDL2::~OsgGraphicsWindowSDL2()
{
    close(true);
}

OsgGraphicsWindowSDL2::OsgGraphicsWindowSDL2(osg::GraphicsContext::Traits *traits)
    : m_Window(0)
    , m_Context(0)
    , m_Valid(false)
    , m_Realized(false)
    , m_OwnsWindow(false)
{
    _traits = traits;

    init();
    if(valid())
    {
        setState(new osg::State);
        getState()->setGraphicsContext(this);

        if(_traits.valid() && _traits->sharedContext.valid())
        {
            getState()->setContextID(_traits->sharedContext->getState()->getContextID());
            incrementContextIDUsageCount(getState()->getContextID());
        }
        else
        {
            getState()->setContextID(osg::GraphicsContext::createNewContextID());
        }
    }
}


bool OsgGraphicsWindowSDL2::setWindowDecorationImplementation(bool flag)
{
    if(!m_Window) return false;

    SDL_SetWindowBordered(m_Window, flag ? SDL_TRUE : SDL_FALSE);

    return true;
}

bool OsgGraphicsWindowSDL2::setWindowRectangleImplementation(int x, int y, int width, int height)
{
    if(!m_Window) return false;

    SDL_SetWindowPosition(m_Window, x, y);
    SDL_SetWindowSize(m_Window, width, height);

    return true;
}

void OsgGraphicsWindowSDL2::setWindowName(const std::string &name)
{
    if(!m_Window) return;

    SDL_SetWindowTitle(m_Window, name.c_str());
    _traits->windowName = name;
}

void OsgGraphicsWindowSDL2::setCursor(MouseCursor mouseCursor)
{
    _traits->useCursor = false;
}


void OsgGraphicsWindowSDL2::init()
{
    if(m_Valid)
        return;

    if(!_traits.valid())
        return;

    WindowData *inheritedWindowData = dynamic_cast<WindowData*>(_traits->inheritedWindowData.get());
    m_Window = inheritedWindowData ? inheritedWindowData->m_Window : NULL;

    m_OwnsWindow = (m_Window == 0);

    if(m_OwnsWindow)
    {
        OSG_NOTICE<<"Error: No SDL window provided."<<std::endl;
        return;
    }

    // SDL will change the current context when it creates a new one, so we
    // have to get the current one to be able to restore it afterward.
    SDL_Window *oldWin = SDL_GL_GetCurrentWindow();
    SDL_GLContext oldCtx = SDL_GL_GetCurrentContext();

    m_Context = SDL_GL_CreateContext(m_Window);
    if(!m_Context)
    {
        OSG_NOTICE<< "Error: Unable to create OpenGL graphics context: "<<SDL_GetError() <<std::endl;
        return;
    }

    SDL_GL_SetSwapInterval(_traits->vsync ? 1 : 0);

    SDL_GL_MakeCurrent(oldWin, oldCtx);

    m_Valid = true;

#if OSG_MIN_VERSION_REQUIRED(3,3,4)
    getEventQueue()->syncWindowRectangleWithGraphicsContext();
#else
    getEventQueue()->syncWindowRectangleWithGraphcisContext();
#endif
}

bool OsgGraphicsWindowSDL2::realizeImplementation()
{
    if(m_Realized)
    {
        OSG_NOTICE<< "GraphicsWindowSDL2::realizeImplementation() Already realized" <<std::endl;
        return true;
    }

    if(!m_Valid) init();
    if(!m_Valid) return false;

    SDL_ShowWindow(m_Window);
    SDL_RestoreWindow(m_Window);

#if OSG_MIN_VERSION_REQUIRED(3,3,4)
    getEventQueue()->syncWindowRectangleWithGraphicsContext();
#else
    getEventQueue()->syncWindowRectangleWithGraphcisContext();
#endif

    m_Realized = true;

    return true;
}

bool OsgGraphicsWindowSDL2::makeCurrentImplementation()
{
    if(!m_Realized)
    {
        OSG_NOTICE<<"Warning: GraphicsWindow not realized, cannot do makeCurrent."<<std::endl;
        return false;
    }

    return SDL_GL_MakeCurrent(m_Window, m_Context)==0;
}

bool OsgGraphicsWindowSDL2::releaseContextImplementation()
{
    if(!m_Realized)
    {
        OSG_NOTICE<< "Warning: GraphicsWindow not realized, cannot do releaseContext." <<std::endl;
        return false;
    }

    return SDL_GL_MakeCurrent(NULL, NULL) == 0;
}


void OsgGraphicsWindowSDL2::closeImplementation()
{
    if(m_Context)
        SDL_GL_DeleteContext(m_Context);

    m_Context = NULL;

    if(m_Window && m_OwnsWindow)
        SDL_DestroyWindow(m_Window);

    m_Window = NULL;

    m_Valid = false;
    m_Realized = false;
}

void OsgGraphicsWindowSDL2::swapBuffersImplementation()
{
    if(!m_Realized) return;

    SDL_GL_SwapWindow(m_Window);
}

void OsgGraphicsWindowSDL2::setSyncToVBlank(bool on)
{
    SDL_Window *oldWin = SDL_GL_GetCurrentWindow();
    SDL_GLContext oldCtx = SDL_GL_GetCurrentContext();

    SDL_GL_MakeCurrent(m_Window, m_Context);

    SDL_GL_SetSwapInterval(on ? 1 : 0);

    SDL_GL_MakeCurrent(oldWin, oldCtx);
}

void OsgGraphicsWindowSDL2::raiseWindow()
{
    SDL_RaiseWindow(m_Window);
}

}
