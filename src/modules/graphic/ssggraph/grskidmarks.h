/*
 *      grskidmarks.h
 *      
 *			created              : Fri Mar 22 23:17:24 CET 2002
 *			Copyright: (C) 2001-2005 by Christophe Guionneau
 * 																	Christos Dimitrakakis
 *																	Bernhard Wymann
 *      
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *      
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *      
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *      MA 02110-1301, USA.
 * 
 * 			$Id: grskidmarks.h 1106 2009-07-14 11:30:35Z kmetykog $
 * 
 */

#ifndef _GRSKIDMARKS_H_
#define _GRSKIDMARKS_H_

#define DELTATSTRIP 0.3f
#define MAXPOINT_BY_STRIP 600
#define MAXSTRIP_BYWHEEL  40
#define DIST_INTERVAL     0.2f

#define SKID_UNUSED  1
#define SKID_BEGIN   2
#define SKID_RUNNING 3
#define SKID_STOPPED 4

#include <plib/ssg.h>	//ssgXXX
#include <car.h>			//tCarElt

class ssgVerterArray;
class ssgVtxTableShadow;

extern void grInitSkidmarks(const tCarElt *car);
extern void grUpdateSkidmarks(const tCarElt *car, const double t);
extern void grShutdownSkidmarks(void);
extern void grDrawSkidmarks(const tCarElt *car);

class cGrSkidStrip
{
public:
	cGrSkidStrip();
	virtual ~cGrSkidStrip();

	virtual void Begin(sgVec3 * tvtx, sgVec2 *tclist, sgVec4 *cur_clr, const double t);
	virtual void End();
	
  ssgVertexArray		**vtx; //the strips
  ssgVtxTableShadow	**vta;
  ssgTexCoordArray	**tex; 
  ssgColourArray		**clr;
	sgVec4						smooth_colour;
	int								*state;
  int								*size;
  double						timeStrip;
  int								running_skid;
  int								next_skid;
  bool							last_state_of_skid;
  int								skid_full;
  float							tex_state;
};


class cGrSkidmarks
{
public:
	cGrSkidmarks() {}
	virtual ~cGrSkidmarks() {}
	
	virtual void Update(const tCarElt* car, const double t);
	
  ssgVtxTable		*base; 			//to remember the pos of the wheel line before transform
  cGrSkidStrip	strips[4];	//the strips of the four wheels
};

#endif //_GRSKIDMARKS_
