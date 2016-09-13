/*
 *      grsmoke.h
 *
 *			Created              : Fri Mar 22 23:17:54 CET 2002
 *			Copyright: (C) 2001 by Christophe Guionneau
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
 * 			$Id: grsmoke.h 6166 2015-10-10 21:41:03Z torcs-ng $
 *
 */

#ifndef _GRSMOKE_H_
#define _GRSMOKE_H_

#include <list>

#define SMOKE_INIT_SIZE 0.2f
#define MAX_SMOKE_LIFE  120
#define MAX_SMOKE_NUMBER  300
#define DELTAT 0.1f
#define VZ_INIT 0.05f
#define VY_INIT 0.1f
#define VX_INIT 0.1f
#define V_EXPANSION 0.4f
#define SMOKE_TYPE_TIRE   1
#define SMOKE_TYPE_ENGINE 2
#define RAIN 0

class ssgVtxTableSmoke : public ssgVtxTable
{
protected:
    virtual void copy_from ( ssgVtxTableSmoke *src, int clone_flags ) ;

public:
    inline bool isAlive() const {return cur_life < max_life;}
    double max_life;
    double step0_max_life;
    double step1_max_life;
    double step2_max_life;
    double cur_life;
    tdble vvx, vvy,vvz;
    sgVec3 cur_col;
    tdble vexp;
    int smokeType;
    int smokeTypeStep;
    double dt;
    double lastTime;
    float sizex;
    float sizey;
    float sizez;
    float init_alpha;
    int stype;
    virtual ssgBase *clone ( int clone_flags = 0 ) ;
    ssgVtxTableSmoke () ;
    ssgVtxTableSmoke (ssgVertexArray	*shd_vtx , float initsize, int type);

    void draw_geometry();
    virtual void drawHighlight ( sgVec4 colour ){ssgVtxTable::drawHighlight(colour);}
    virtual void drawHighlight ( sgVec4 colour, int i ){ssgVtxTable::drawHighlight(colour,i);}

    virtual void pick ( int baseName )  { ssgVtxTable::pick(baseName);}
    virtual void transform ( const sgMat4 m )  { ssgVtxTable::transform(m);}

    virtual void setVertices  ( ssgVertexArray   *vl ) {  ssgVtxTable::setVertices(vl);}
    virtual void setNormals   ( ssgNormalArray   *nl ) {  ssgVtxTable::setNormals(nl);}
    virtual void setTexCoords ( ssgTexCoordArray *tl ) {  ssgVtxTable::setTexCoords(tl);}
    virtual void setColours   ( ssgColourArray   *cl ) {  ssgVtxTable::setColours(cl);}

    int getNumVertices  () { return vertices  -> getNum () ; }
    int getNumNormals   () { return normals   -> getNum () ; }
    int getNumColours   () { return colours   -> getNum () ; }
    int getNumTexCoords () { return texcoords -> getNum () ; }

    int getNumTriangles ()  { return ssgVtxTable::getNumTriangles();}
    void getTriangle ( int n, short *v1, short *v2, short *v3 )  { ssgVtxTable::getTriangle(n,v1,v2,v3);}
    int  getNumLines ()  {return ssgVtxTable::getNumLines();}
    void getLine ( int n, short *v1, short *v2 )  { ssgVtxTable::getLine(n,v1,v2);}


    virtual ~ssgVtxTableSmoke (void);

    virtual const char *getTypeName(void)  { return ssgVtxTable::getTypeName();}

    virtual void print ( FILE *fd = stderr, char *indent = "", int how_much = 2) { ssgVtxTable::print(fd,indent,how_much);}
    virtual int load ( FILE *fd )  {return  ssgVtxTable::load(fd);}
    virtual int save ( FILE *fd )  {return  ssgVtxTable::save(fd);}
};


class cSmokeDef;

class cGrSmoke
{
public:
    bool Add(tCarElt* car, const int i, const double t, int smokeType,
             const cSmokeDef *sd);
    void Update(const double t);

    ssgVtxTableSmoke *smoke;
};


extern void grInitSmoke(const int index);
extern void grAddSmoke(tCarElt *car, const double t);
extern void grUpdateSmoke(const double t);
extern void grShutdownSmoke();

#endif //_GRSMOKE_H_
