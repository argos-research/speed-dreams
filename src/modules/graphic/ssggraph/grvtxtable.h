/***************************************************************************

    file                 : grvtxtable.h
    created              : Fri Mar 22 23:16:44 CET 2002
    copyright            : (C) 2001 by Christophe Guionneau
    version              : $Id: grvtxtable.h 4012 2011-10-29 11:01:26Z pouillot $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _GRVTXTABLE_H_
#define _GRVTXTABLE_H_

#include <plib/ssg.h>		//ssgXXX

#include "grmultitexstate.h"


class cgrVtxTable : public ssgVtxTable
{
public:

  virtual ssgBase* clone ( int clone_flags = 0 ) ;

  cgrVtxTable () ;

  // TODO: Document API.
  cgrVtxTable (int nTexMaps,
			   GLenum ty, ssgVertexArray* vl,
			   ssgNormalArray* nl,
			   ssgTexCoordArray* tl, // TODO : Generalize to N states (not only 4).
			   ssgTexCoordArray* tl1, ssgTexCoordArray* tl2, ssgTexCoordArray* tl3,
			   ssgColourArray* cl,
			   ssgIndexArray* stripeIndex = 0, int numstripes = -1, ssgIndexArray* il = 0) ;

  virtual ~cgrVtxTable (void);

  float* getMultiTexCoord(int nStateInd, int nCoordInd);

  cgrMultiTexState* getMultiTexState (int nStateInd);
  void setMultiTexState (int nStateInd, cgrMultiTexState* st);

  void draw_geometry_array();

protected:
	
  virtual void copy_from ( cgrVtxTable* src, int clone_flags = 0) ;

protected:

  // Max supported number of texture maps (1 texture unit needed for each)
  enum { NMaxTexMaps = 4 };

  // Number of texture maps (>= 1).
  int _nTexMaps;

  ssgIndexArray* _indices;
  ssgIndexArray* _stripes;
  int _numStripes;

  // 1 state for each other texture map (see ssgVtxTable for the 1st one)
  cgrMultiTexState* _mTexStates[NMaxTexMaps-1] ; // 0 => texture unit 1 ; 1 => TU 2 ; ...

_SSG_PUBLIC:  

  // 1 texCoordArray for each other state.
  ssgTexCoordArray* _mTexCoords[NMaxTexMaps-1] ;
};

class cgrVtxTableTrackPart : public cgrVtxTable
{
public:

  cgrVtxTableTrackPart (int nTexMaps,
						GLenum ty, ssgVertexArray* vl,
						ssgNormalArray* nl,
						ssgTexCoordArray* tl,
						ssgTexCoordArray* tl1, ssgTexCoordArray* tl2, ssgTexCoordArray* tl3,
						ssgColourArray* cl,
						ssgIndexArray* stripeIndex = 0, int numstripes = -1, ssgIndexArray* il = 0) ;

  virtual void draw () ;

  void draw_geometry();
};

class cgrVtxTableCarPart : public cgrVtxTable
{
public:

  virtual ssgBase* clone ( int clone_flags = 0 ) ;

  cgrVtxTableCarPart () ;
  
  cgrVtxTableCarPart (int nTexMaps, int carIndex,
					  GLenum ty, ssgVertexArray* vl,
					  ssgNormalArray* nl,
					  ssgTexCoordArray* tl,
					  ssgTexCoordArray* tl1, ssgTexCoordArray* tl2, ssgTexCoordArray* tl3,
					  ssgColourArray* cl,
					  ssgIndexArray* stripeIndex = 0, int numstripes = -1, ssgIndexArray* il = 0) ;

  virtual void draw () ;

  void draw_geometry () ;
  void draw_geometry_array () ;

protected:
	
  virtual void copy_from ( cgrVtxTableCarPart* src, int clone_flags = 0) ;

protected:

  // The car to draw.
  int _carIndex;

};

#endif //_GRVTXTABLE_H_
