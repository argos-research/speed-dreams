/***************************************************************************

    file                 : grtrackmap.h
    created              : Fri Aug 29 00:57:00 CEST 2003
    copyright            : (C) 2003 by Bernhard Wymann
    email                : berniw@bluewin.ch
    version              : $Id: grtrackmap.h 5100 2013-01-14 23:16:27Z kmetykog $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

/*
	This class displays an overview map of the track, such that you can see the
	upcoming track layout. Your car is displayed as a dot.
	To get reasonable efficiency, the track is first rendered into a texture. During the
	game just the texture needs to be redrawn.
*/

#ifndef _GRTRACKMAP_H_
#define _GRTRACKMAP_H_

#ifdef WIN32
#include <windows.h>
#endif //WIN32

#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif

#ifndef WIN32
#include <unistd.h>
#endif

#define TRACK_MAP_NONE												(1<<0)
#define TRACK_MAP_NORMAL											(1<<1)
#define TRACK_MAP_NORMAL_WITH_OPPONENTS				(1<<2)
#define TRACK_MAP_PAN													(1<<3)
#define TRACK_MAP_PAN_WITH_OPPONENTS					(1<<4)
#define TRACK_MAP_PAN_ALIGNED									(1<<5)
#define TRACK_MAP_PAN_ALIGNED_WITH_OPPONENTS	(1<<6)

// Must equal the biggest TRACK_MAP_*
#define TRACK_MAP_MASK (1<<6)

// Pointer to the track data found in grscene.cpp.
#include <track.h>
extern tTrack *grTrack;

class cGrTrackMap
{
	public:
		// The constructor creates a texture of the track data, such that the track
		// layout can be displayed efficiently. Additional data gets initialized.
		cGrTrackMap();

		// Release the texture and data.
		~cGrTrackMap();

		// Walk trough the different available display modes.
		void selectTrackMap();

		// Draw the track map according to the display mode.
		void display(
			const tCarElt *currentCar,
			tSituation *situation,
			int Winx,
			int Winy,
			int Winw,
			int Winh
		);

		inline void setViewMode(const int vm) { viewmode = vm; }
		inline int getViewMode() const { return viewmode; }
		inline int getDefaultViewMode() const { return TRACK_MAP_NORMAL_WITH_OPPONENTS; }

	private:
		// The resolution in [m] to analyse turns.
		static const float RESOLUTION;

		// Minimum and Maximum line width in pixels to draw the track.
		static const float MINLINEWIDTH;
		static const float MAXLINEWIDTH;

		// Some data needs just one initalization, after first initialization this is set to true.
		static bool isinitalized;

		// Texture object
		static GLuint mapTexture;

		// The car "dot" display list.
		static GLuint cardot;

		// Track bounding box properties, lower left, upper right corner, width and height.
		static float track_min_x;
		static float track_max_x;
		static float track_min_y;
		static float track_max_y;
		static float track_width;
		static float track_height;

		// The ratio of width and height to MAX(width, height).
		static float track_x_ratio;
		static float track_y_ratio;

		// Position and size of the map (relative to top left).
		static int map_x;
		static int map_y;
		static int map_size;

		// Scaling factor from meters to texels.
		static float ratio;

		// Color of the cars "dots".
		static GLfloat currentCarColor[4];
		static GLfloat aheadCarColor[4];
		static GLfloat behindCarColor[4];

		// Holds the current view (look at TRACK_MAP_*).
		int viewmode;
		tSituation *situ;

		// Draw the track full visible and static.
		void drawTrackNormal(int x, int y);

		// Draw the track in the panning mode.
		void drawTrackPanning(int Winx, int Winy, int Winw, int Winh, const tCarElt *currentCar, tSituation *situation);

		// Draw the track in the panning aligned mode.
		void drawTrackPanningAligned(int Winx, int Winy, int Winw, int Winh, const tCarElt *currentCar, tSituation *s);

		// Draw the dot of the car.
		void drawCar(const tCarElt *currentCar, GLfloat* color, int x, int y);

		// Draw all opponents of the current car.
		void drawCars(const tCarElt *currentCar, tSituation *s, int x, int y);

		// Setus up colors.
		void initColors();
		void initColors(const float *current_color, const float *ahead_color, const float *behind_color);

		void checkAndSetMinimum(float &currentmin, float &value);
		void checkAndSetMaximum(float &currentmax, float &value);
};

#endif // _GRTRACKMAP_H_
