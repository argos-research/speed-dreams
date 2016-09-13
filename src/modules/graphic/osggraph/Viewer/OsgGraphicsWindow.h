/***************************************************************************

    file                 : OsgGraphicsWindows.h
    created              : Thu Sep 15 15:23:49 CEST 2015
    copyright            : (C)2015 by Xavier Bertaux
    email                : bertauxx@yahoo.fr
    version              : $Id: OsgGraphicsWindows.h 6125 2015-09-15 06:02:49Z torcs-ng $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *	 Based on OpenMW code                                                  *
 *                                                                         *
 ***************************************************************************/
#ifndef OSGGRAPHICSWINDOW_H
#define OSGGRAPHICSWINDOW_H

#include <SDL_video.h>

#include <osgViewer/GraphicsWindow>

namespace OSGUtil
{
class OsgGraphicsWindowSDL2 : public osgViewer::GraphicsWindow
{
    SDL_Window*     m_Window;
    SDL_GLContext   m_Context;

    bool            m_Valid;
    bool            m_Realized;
    bool            m_OwnsWindow;

    void init();

    virtual ~OsgGraphicsWindowSDL2();

public:
    OsgGraphicsWindowSDL2(osg::GraphicsContext::Traits *traits);

    virtual bool isSameKindAs(const Object* object) const { return dynamic_cast<const OsgGraphicsWindowSDL2*>(object)!=0; }
    virtual const char* libraryName() const { return "osgViewer"; }
    virtual const char* className() const { return "OsgGraphicsWindowSDL2"; }

    virtual bool valid() const { return m_Valid; }

    /** Realise the GraphicsContext.*/
    virtual bool realizeImplementation();

    /** Return true if the graphics context has been realised and is ready to use.*/
    virtual bool isRealizedImplementation() const { return m_Realized; }

    /** Close the graphics context.*/
    virtual void closeImplementation();

    /** Make this graphics context current.*/
    virtual bool makeCurrentImplementation();

    /** Release the graphics context.*/
    virtual bool releaseContextImplementation();

    /** Swap the front and back buffers.*/
    virtual void swapBuffersImplementation();

    /** Set sync-to-vblank. */
    virtual void setSyncToVBlank(bool on);

    /** Set Window decoration.*/
    virtual bool setWindowDecorationImplementation(bool flag);

    /** Raise specified window */
    virtual void raiseWindow();

    /** Set the window's position and size.*/
    virtual bool setWindowRectangleImplementation(int x, int y, int width, int height);

    /** Set the name of the window */
    virtual void setWindowName(const std::string &name);

    /** Set mouse cursor to a specific shape.*/
    virtual void setCursor(MouseCursor cursor);

    /** Get focus.*/
    virtual void grabFocus() {}

    /** Get focus on if the pointer is in this window.*/
    virtual void grabFocusIfPointerInWindow() {}

    /** WindowData is used to pass in the SDL2 window handle attached to the GraphicsContext::Traits structure. */
    struct WindowData : public osg::Referenced
    {
        WindowData(SDL_Window *window) : m_Window(window)
        { }

        SDL_Window *m_Window;
    };
};

}

#endif /* OSGGRAPHICSWINDOW_H */
