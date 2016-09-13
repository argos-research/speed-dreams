/***************************************************************************

    file                 : grsmoke.cpp
    created              : Fri Mar 22 23:17:54 CET 2002
    copyright            : (C) 2001 by Christophe Guionneau
    version              : $Id: grsmoke.cpp 6166 2015-10-10 21:41:03Z torcs-ng $

***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <plib/ssg.h>

#include <robottools.h>

#include "grsmoke.h"

#include "grmain.h"
#include "grcar.h"      //grCarInfo
#include "grscene.h"	//SmokeAnchor
#include "grutil.h"     //urandom

#ifdef DMALLOC
#include "dmalloc.h"
#endif

static int      grSmokeMaxNumber;
static double   grSmokeDeltaT;
static double   grFireDeltaT;
static double   grSmokeLife;
static int      grWater;


static ssgSimpleState *mst      = NULL;	//Smoke img
static ssgSimpleState *mstf0    = NULL;	//Fire img #1
static ssgSimpleState *mstf1    = NULL;	//Fire img #2
static double *timeSmoke        = NULL;	//Stores the time of the wheel smoke last update
static double *timeFire         = NULL;	//Stores the time of the exhaust fire last update

//! All smokes are stored here in this list.
static std::list<cGrSmoke> *smokeList = NULL;

/**
 * class cSmokeDef
 *
 * Utility class for passing parameters to cGrSmoke::Add easily.
 */
class cSmokeDef
{
public:
    inline void Init(const double c1, const double c2, const double c3,
                     const tdble is, const tdble th, const tdble slc, const tdble ssc)
    {
        cur_clr[0] = c1;
        cur_clr[1] = c2;
        cur_clr[2] = c3;
        init_speed = is;
        threshold = th;
        smoke_life_coefficient = slc;
        smoke_speed_coefficient = ssc;
    }

    sgVec3 cur_clr;
    tdble init_speed;
    tdble threshold;
    tdble smoke_life_coefficient;
    tdble smoke_speed_coefficient;
};


/**
 * grInitSmoke
 * Initializes the smoke structure.
 *
 * @param index The car index
 */
void
grInitSmoke(const int index)
{
    grSmokeMaxNumber = (int)GfParmGetNum(grHandle, GR_SCT_GRAPHIC, GR_ATT_SMOKENB,
                                         (char*)NULL, MAX_SMOKE_NUMBER);
    grSmokeDeltaT = (double)GfParmGetNum(grHandle, GR_SCT_GRAPHIC, GR_ATT_SMOKEDELTAT,
                                         (char*)NULL, DELTAT);
    grSmokeLife = (double)GfParmGetNum(grHandle, GR_SCT_GRAPHIC, GR_ATT_SMOKEDLIFE,
                                       (char*)NULL, 2.0);

    //Only proceed if there is any need for smokes to display
    if(grSmokeMaxNumber) {
        if (grSmokeLife > MAX_SMOKE_LIFE)
            grSmokeLife = MAX_SMOKE_LIFE;

        grFireDeltaT = grSmokeDeltaT * 8;

        if (!timeSmoke)
        {
            timeSmoke = new double[index * 4];
            memset(timeSmoke, 0, sizeof(double) * index * 4);
        }

        if (!timeFire)
        {
            timeFire = new double[index];
            memset(timeFire, 0 , sizeof(double) * index);
        }

        if (!smokeList)
        {
            smokeList = new std::list<cGrSmoke>;
        }

        char buf[256];
        // add temp object to get a reference on the states
        if (!mst)
        {
            sprintf(buf, "data/textures;data/img;.");
            mst = (ssgSimpleState*)grSsgLoadTexStateEx("smoke.png", buf, FALSE, FALSE);

            if (mst)
            {
                mst->disable(GL_LIGHTING);
                mst->enable(GL_BLEND);
                mst->disable(GL_CULL_FACE);
                mst->setTranslucent();
                mst->setColourMaterial(GL_AMBIENT_AND_DIFFUSE);
            }//if mst
        }//if !mst

        if (!mstf0)
        {
            sprintf(buf, "data/textures;data/img;.");
            mstf0 = (ssgSimpleState*)grSsgLoadTexStateEx("fire0.png", buf, FALSE, FALSE);

            if (mstf0)
            {
                mstf0->disable(GL_LIGHTING);
                mstf0->enable(GL_BLEND);
                mstf0->disable(GL_CULL_FACE);
                mstf0->setTranslucent();
                mstf0->setColourMaterial(GL_AMBIENT_AND_DIFFUSE);
            }//if mstf0
        }//if !mstf0

        if (!mstf1)
        {
            sprintf(buf, "data/textures;data/img;.");
            mstf1 = (ssgSimpleState*)grSsgLoadTexStateEx("fire1.png", buf, FALSE, FALSE);

            if (mstf1)
            {
                mstf1->disable(GL_LIGHTING);
                mstf1->enable(GL_BLEND);
                mstf1->disable(GL_CULL_FACE);
                mstf1->setTranslucent();
                mstf1->setColourMaterial(GL_AMBIENT_AND_DIFFUSE);
            }//if mstf1
        }//if !mstf1
    }//if grSmokeMaxNumber
}//grInitSmoke


/**
 * grUpdateSmoke
 * Updates the smokes (wheel, exhaust) for a car, if necessary.
 * Also removes smokes that reached their end of life.
 *
 * @param t	Current simulation time
 */
void
grUpdateSmoke(const double t)
{
    if(grSmokeMaxNumber)
    {
        for(std::list<cGrSmoke>::iterator it = smokeList->begin();
            it != smokeList->end();
            it++)
        {
            if (!it->smoke->isAlive())
            {
                //Smoke too old, get rid of it.
                SmokeAnchor->removeKid(it->smoke);
                it = smokeList->erase(it);

                if (it == smokeList->end())
                    break;

            }
            else
            {
                //Update the smoke
                it->Update(t);
            }//if !it->smoke->isAlive
        }//for it
    }//if grSmokeMaxNumber
}//grUpdateSmoke


/**
 * grAddSmoke
 * Adds smoke to a car.
 * If it is skidding or slipping, adds 4 smokes (one to each wheel).
 * If it has a sudden RPM change, adds exhaust smoke(s).
 *
 * @param car	Pointer to the car in need of smokes
 * @param t	Current simulation time
 */
void
grAddSmoke(tCarElt *car, const double t)
{
    if(grSmokeMaxNumber)
    {
        //Speed ^ 2
        tdble spd2 = car->_speed_x * car->_speed_x + car->_speed_y * car->_speed_y;

        //!Add smoke from spinning/skidding wheels
        if (spd2 > 0.001f)
        {				//If the car is moving
            for(int i = 0; i < 4; i++)
            {	//Loop through all 4 wheels
                if ((int)smokeList->size() < grSmokeMaxNumber)
                {	//If there is space for more smoke
                    //If the car's smoke is not too current, let's add it.
                    if ((t - timeSmoke[car->index * 4 + i]) > grSmokeDeltaT)
                    {
                        timeSmoke[car->index * 4 + i] = t;

                        //!Different smoke for different surfaces
                        cSmokeDef sd;//(0.8f, 0.8f, 0.8f, 0.01f, 0.1f, 30.0f, 0.0f);
                        if (car->priv.wheel[i].seg) { // sanity check
                            const char* surface = car->priv.wheel[i].seg->surface->material;

                            if (strstr(surface, "sand"))
                            {
                                sd.Init(0.8f, 0.7f + urandom() * 0.1f, 0.4f + urandom() * 0.2f,
                                        0.5f, 0.05f, 12.5f, 0.25f);
                            }
                            else if (strstr(surface, "dirt"))
                            {
                                sd.Init(0.7f + urandom() * 0.1f, 0.6f + urandom() * 0.1f, 0.5f + urandom() * 0.1f,	0.45f, 0.0f, 10.0f, 0.5f);
                            }
                            else if (strstr(surface,"mud"))
                            {
                                sd.Init(0.25f, 0.17f + urandom() * 0.02f, 0.05f + urandom() * 0.02f, 0.2f, 0.25f, 15.0f, 0.25f);
                            }
                            else if (strstr(surface,"gravel"))
                            {
                                sd.Init(0.6f, 0.6f, 0.6f,
                                        0.35f, 0.1f, 20.0f, 0.1f);
                            }
                            else if (strstr(surface,"grass"))
                            {
                                sd.Init(0.4f + urandom() * 0.2f, 0.5f + urandom() * 0.1f, 0.3f + urandom() * 0.1f, 0.3f, 0.1f, 25.0f, 0.0f);
                            }
                            else if (strstr(surface,"snow"))
                            {
                                sd.Init(0.75f, 0.75f + urandom() * 0.1f, 0.75f + urandom() * 0.1f, 0.35f, 0.0f, 8.0f, 0.4f);
                            }
                            else
                            {
                                sd.Init(0.8f, 0.8f, 0.8f,
                                        0.01f, 0.1f, 30.0f, 0.0f);
                            }
                        }//if car->priv.wheel

                        //! Ground water can change the tire smoke attributes.
                        grWater = grTrack->local.water;

                        if (grWater > 0)
                        {
                            sd.Init(0.6f, 0.6f, 0.6f, 0.45f, 0.0f, 10.5f, 0.25f);
                        }//if grWater

                        //!Add smoke
                        cGrSmoke tmp;
                        if(tmp.Add(car, i, t, SMOKE_TYPE_TIRE, &sd))
                        {
                            smokeList->push_back(tmp);
                        }//if tmp.Add
                    }//if t
                }//if smokeList->size
            }//for i
        }//if spd2

        //!Add exhaust fire smoke
        if (car->_exhaustNb && (spd2 > 10.0))
        {	//If the car has exhaust pipes and is moving
            if ((int)smokeList->size() < grSmokeMaxNumber)
            {	//If there is space for more smoke
                const int index = car->index;	//current car's index
                //If the car's enxhaust smoke is not too current, let's add it.
                if ((t - timeFire[index]) > grFireDeltaT)
                {
                    timeFire[index] = t;
                    //!The need for exhaust fire is calculated from the RPM change
                    //Only need to display exhaust fire when the gear is changing backwards
                    tgrCarInstrument *curInst = &(grCarInfo[index].instrument[0]);
                    tdble val = ((*(curInst->monitored) - curInst->minValue)
                                 - (curInst->rawPrev - curInst->minValue)) / curInst->maxValue;
                    curInst->rawPrev = *(curInst->monitored);

                    if (val > 0.1 && val < 0.5)
                    {//need upper limit to prevent multiple exhaust fires at race starts
                        grCarInfo[index].fireCount = (int)(10.0 * val * car->_exhaustPower);
                    }

                    if (grCarInfo[index].fireCount)
                    {
                        grCarInfo[index].fireCount--;
                        //Add fire to each exhaust
                        for (int i = 0; i < car->_exhaustNb; i++)
                        {
                            cGrSmoke tmp;
                            if(tmp.Add(car, i, t, SMOKE_TYPE_ENGINE, NULL))
                            {
                                smokeList->push_back(tmp);
                            }//if tmp.Add
                        }//for i
                    }//if fireCount
                }//if t
            }//is smokeList->size
        }//if car-_exhaustNb
    }//if grSmokeMaxNumber
}//grAddSmoke


/**
 * grShutdownSmoke
 * Removes all smokes for all cars.
 */
void
grShutdownSmoke()
{
    GfOut("-- grShutdownSmoke\n");

    if(grSmokeMaxNumber)
    {
        SmokeAnchor->removeAllKids();
        if (smokeList)
        {
            //~ std::list<cGrSmoke>::iterator tmp = smokeList->begin();
            //~ while(tmp != smokeList->end())
            //~ {
            //~ /* SmokeAnchor->removeKid(tmp->smoke); */
            //~ tmp = smokeList->erase(tmp);
            //~ }
            smokeList->clear();
            delete [] timeSmoke;
            delete [] timeFire;
            delete smokeList;

            timeSmoke = NULL;
            timeFire = NULL;
            smokeList = NULL;
        }//if smokeList
    }//if grSmokeMaxNumber
}//grShutdownSmoke


void ssgVtxTableSmoke::copy_from ( ssgVtxTableSmoke *src, int clone_flags )
{
    ssgVtxTable::copy_from ( src, clone_flags ) ;
}

ssgBase *ssgVtxTableSmoke::clone ( int clone_flags )
{
    ssgVtxTableSmoke *b = new ssgVtxTableSmoke ;
    b -> copy_from ( this, clone_flags ) ;
    return b ;
}

ssgVtxTableSmoke::ssgVtxTableSmoke ()
{
    ssgVtxTable();
}

ssgVtxTableSmoke:: ssgVtxTableSmoke (ssgVertexArray	*shd_vtx , float initsize, int typ)
{
    sizex = sizey = sizez = initsize;

    gltype = GL_TRIANGLE_STRIP;
    type = ssgTypeVtxTable () ;
    stype = typ;
    vertices  = (shd_vtx!=NULL) ? shd_vtx : new ssgVertexArray   () ;
    normals   =  new ssgNormalArray   () ;
    texcoords =  new ssgTexCoordArray () ;
    colours   =  new ssgColourArray   () ;

    vertices  -> ref () ;
    normals   -> ref () ;
    texcoords -> ref () ;
    colours   -> ref () ;
    cur_col[0] = cur_col[1] = cur_col[2] = 0.8;
    vvx = vvy = vvz = 0.0;
    init_alpha = 0.9;
    recalcBSphere () ;
}

ssgVtxTableSmoke::~ssgVtxTableSmoke ()
{
}

void ssgVtxTableSmoke::draw_geometry ()
{
    int num_colours = getNumColours();
    int num_normals = getNumNormals();
    float alpha;
    GLfloat modelView[16];
    sgVec3 A, B, C, D;
    sgVec3 right, up, offset;

    sgVec3 *vx = (sgVec3 *) vertices->get(0);
    sgVec3 *nm = (sgVec3 *) normals->get(0);
    sgVec4 *cl = (sgVec4 *) colours->get(0);
    alpha =  0.9f - ((float)(cur_life/max_life));
    glDepthMask(GL_FALSE);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    /*glPolygonOffset(-5.0f, +10.0f);*/
    /*glEnable(GL_POLYGON_OFFSET_FILL);*/

    // the principle is to have a right and up vector
    // to determine how the points of the quadri should be placed
    // orthogonaly to the view, parallel to the screen.

    /* get the matrix */
    // TODO: replace that, glGet stalls rendering pipeline (forces flush).
    glGetFloatv(GL_MODELVIEW_MATRIX, modelView);

    // get the up and right vector from the matrice view

    offset[0] = offset[1] = offset[2] = 0.0f;

    int i;
    for (i = 0; i < 3; i++)
    {
        int j = i;
        int k;
        for (k = 0; k < 4; k++, j+=4)
        {
            if (k != 3)
            {
                offset[i] += modelView[j] * vx[0][k];
            }
            else
            {
                offset[i] += modelView[j];
            }
        }
    }
    //printf ("%f %f %f\n", offset[0], offset[1], offset[2]);

    tdble dist = sqrt(offset[0]*offset[0]
            + offset[1]*offset[1]
            + offset[2]*offset[2]);

    up[0] = modelView[1];
    up[1] = modelView[5];
    up[2] = modelView[9];

    right[0] = modelView[0];
    right[1] = modelView[4];
    right[2] = modelView[8];

    // compute the coordinates of the four points of the quadri.

    // up and right points
    C[0] = right[0]+up[0];
    C[1] = right[1]+up[1];
    C[2] = right[2]+up[2];

    // left and up
    D[0] = -right[0]+up[0];
    D[1] = -right[1]+up[1];
    D[2] = -right[2]+up[2];

    // down and left
    A[0] = -right[0]-up[0];
    A[1] = -right[1]-up[1];
    A[2] = -right[2]-up[2];

    // right and down
    B[0] = right[0]-up[0];
    B[1] = right[1]-up[1];
    B[2] = right[2]-up[2];

    glBegin ( gltype ) ;

    if (dist < 50.0f)
    {
        alpha *= (1.0f - exp(-0.1f * dist));
    }

    glColor4f(cur_col[0],cur_col[1],cur_col[2],alpha);

    if (num_colours == 1)
    {
        glColor4fv(cl[0]);
    }

    if (num_normals == 1)
    {
        glNormal3fv(nm[0]);
    }

    // the computed coordinates are translated from the smoke position with the x, y, z speed
    glTexCoord2f(0,0);
    glVertex3f(vx[0][0]+sizex*A[0],vx[0][1]+sizey*A[1], vx[0][2]+sizez*A[2]);
    glTexCoord2f(0,1);
    glVertex3f(vx[0][0]+sizex*B[0],vx[0][1]+sizey*B[1], vx[0][2]+sizez*B[2]);
    glTexCoord2f(1,0);
    glVertex3f(vx[0][0]+sizex*D[0],vx[0][1]+sizey*D[1], vx[0][2]+sizez*D[2]);
    glTexCoord2f(1,1);
    glVertex3f(vx[0][0]+sizex*C[0],vx[0][1]+sizey*C[1], vx[0][2]+sizez*C[2]);
    glEnd();

    glDisable(GL_POLYGON_OFFSET_FILL);
    glDepthMask(GL_TRUE);
    glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
}


/**
 * Update
 * Updates the smoke, based on time since last update.
 * @param t Current simulation time
 */
void
cGrSmoke::Update(const double t)
{
    // update the smoke
    smoke->dt = t - smoke->lastTime;
    // expand the Y value
    smoke->sizey += smoke->dt * smoke->vexp * 2.0f;
    smoke->sizez += smoke->dt * smoke->vexp * 0.25f;
    smoke->sizex += smoke->dt * smoke->vexp * 2.0f;

    //!Exhaust fire can change shape, hence the need for two bitmaps
    if (smoke->smokeType == SMOKE_TYPE_ENGINE)
    {
        if (smoke->smokeTypeStep == 0)
        {
            if (smoke->cur_life >= smoke->step0_max_life)
            {
                // changing from fire to smoke
                smoke->smokeTypeStep = 1;
                smoke->setState(mstf1);
            }
        }
        else if (smoke->smokeTypeStep == 1)
        {
            if (smoke->cur_life >= smoke->step1_max_life)
            {
                // changing from fire to smoke
                smoke->smokeTypeStep = 2;
                smoke->setState(mst);
            }
        }
    }//if smokeType SMOKE_TYPE_ENGINE

    sgVec3 *vx = (sgVec3 *) smoke->getVertices()->get(0);

    tdble dt = smoke->dt;
    tdble damp = 0.2f;
    smoke->vvx -= damp * smoke->vvx * fabs(smoke->vvx) * dt;
    smoke->vvy -= damp * smoke->vvy * fabs(smoke->vvy) * dt;
    smoke->vvz -= damp * smoke->vvz * fabs(smoke->vvz) * dt;

    if (grWater > 0)
    {
        smoke->vvx += 0.0039f;
        smoke->vvy += 0.0039f;
        smoke->vvz += 0.0039f;
    } else {
        smoke->vvz += 0.0001f;
    }

    vx[0][0] += smoke->vvx * dt;
    vx[0][1] += smoke->vvy * dt;
    vx[0][2] += smoke->vvz * dt;

    smoke->lastTime = t;
    smoke->cur_life += smoke->dt;
}//cGrSmoke::Update


/**
 * Add
 * Adds one smoke (either wheel smoke or an exhaust fire)
 * @param car	the car to connect the smoke to
 * @param i wheel number in case of wheel smoke, NA in case of exhaust
 * @param t	Current simulation time
 * @param type Wheel smoke or exhaust fire?
 * @param sd Definition of smoke
 * @return True if everything went fine, smoke can be added to the main list.
 */
bool
cGrSmoke::Add(tCarElt *car, const int i, const double t,
              const int type, const cSmokeDef *sd)
{
    bool ret = false;

    if(type == SMOKE_TYPE_TIRE)
    {
        //!Add tire smoke
        tdble sinCarYaw = sin(car->_yaw);
        tdble cosCarYaw = cos(car->_yaw);
        tdble spd2 = car->_speed_x * car->_speed_x + car->_speed_y * car->_speed_y;
        tdble smoke_life_coefficient = sd->smoke_life_coefficient * (1.0f - urandom() * urandom());
        tdble spd_fx = tanh(0.001f * car->_reaction[i]) * sd->smoke_speed_coefficient * sqrt(spd2);
        double slip = MAX(0.0, ((car->_wheelSpinVel(i) * car->_wheelRadius(i)) - fabs(car->_speed_x)) - 9.0);
        bool skidsmoke = (car->_skid[i] + 0.025f * urandom() * spd_fx > urandom() + sd->threshold); // instead of 0.3, to randomize

        if (skidsmoke || slip > 0.0)
        { // smoke from driven wheel spin
            float init_speed_z = 0.1f + (float) (slip/20);
            float stretch_factor = 0.2f;

            ssgVertexArray *shd_vtx = new ssgVertexArray(1);
            //shd_clr = new ssgColourArray(1);

            //Smoke originates from the given wheel
            sgVec3 vtx;
            vtx[0] = car->priv.wheel[i].relPos.x;
            vtx[1] = car->priv.wheel[i].relPos.y;
            vtx[2] = car->priv.wheel[i].relPos.z - car->_wheelRadius(i) * 1.0f + 0.5f * SMOKE_INIT_SIZE;
            tdble stretchX = 0.1f * (spd_fx + stretch_factor * fabs(car->_speed_X));
            tdble stretchY = 0.1f * (spd_fx + stretch_factor * fabs(car->_speed_Y));
            vtx[0] -= 0.05f * car->_speed_x;
            shd_vtx->add(vtx);

            tdble init_speed = sd->init_speed * urandom();

            smoke = new ssgVtxTableSmoke(shd_vtx, SMOKE_INIT_SIZE, SMOKE_TYPE_TIRE);

            smoke->vvx = -sinCarYaw * car->_wheelSlipSide(i);
            smoke->vvy =  cosCarYaw * car->_wheelSlipSide(i);
            smoke->vvx += cosCarYaw * car->_wheelSlipAccel(i);
            smoke->vvy += sinCarYaw * car->_wheelSlipAccel(i);
            smoke->vvy += cosCarYaw * slip;
            smoke->vvy += sinCarYaw * slip;

            smoke->vvz = init_speed_z;

            smoke->vvx *= init_speed;
            smoke->vvy *= init_speed;
            smoke->setState(mst);
            smoke->setCullFace(0);

            //printf("%f\n", car->_reaction[i]);
            smoke->max_life = 0.0;
            if (skidsmoke)
                smoke->max_life = grSmokeLife *
                        ((car->_skid[i])*sqrt(spd2)+urandom()*spd_fx)/ smoke_life_coefficient;
            else
                smoke->max_life = grSmokeLife *
                        ((slip/10)*sqrt(spd2)+urandom()*spd_fx)/ smoke_life_coefficient;

            for (int c = 0; c < 3; c++)
            {
                smoke->cur_col[c] = sd->cur_clr[c];
            }

            smoke->cur_life = 0;
            smoke->sizex = VX_INIT + stretchX;
            smoke->sizey = VY_INIT + stretchY;
            smoke->sizez = VZ_INIT + 0.1f * spd_fx;

            smoke->init_alpha = 1.0/(1.0+0.1*spd_fx);
            if (skidsmoke)
                smoke->vexp = V_EXPANSION+((car->_skid[i])+.1*spd_fx)*(((float)rand()/(float)RAND_MAX));
            else
                smoke->vexp = V_EXPANSION+((slip/3)+.1*spd_fx)*(((float)rand()/(float)RAND_MAX));
            smoke->smokeType = SMOKE_TYPE_TIRE;
            smoke->smokeTypeStep = 0;
            smoke->lastTime = t;
            smoke->transform(grCarInfo[car->index].carPos);
            SmokeAnchor->addKid(smoke);
            ret = true;
        }//if skidsmoke || slip
    }
    else
    {
        //!Add engine fire
        //Smoke originates from the exhaust pipe
        sgVec3 vtx;
        vtx[0] = car->_exhaustPos[i].x;
        vtx[1] = car->_exhaustPos[i].y;
        vtx[2] = car->_exhaustPos[i].z;
        ssgVertexArray *shd_vtx = new ssgVertexArray(1);
        shd_vtx->add(vtx);

        smoke = new ssgVtxTableSmoke(shd_vtx, SMOKE_INIT_SIZE * 4, SMOKE_TYPE_ENGINE);

        smoke->setState(mstf0);	//Starts life lloking like fire0.rgb
        smoke->setCullFace(0);
        smoke->max_life = grSmokeLife / 8;
        smoke->step0_max_life = (grSmokeLife) / 50.0;
        smoke->step1_max_life = (grSmokeLife) / 50.0 + smoke->max_life / 2.0;
        smoke->cur_life = 0;
        smoke->sizex = VX_INIT * 4;
        smoke->sizey = VY_INIT * 4;
        smoke->sizez = VZ_INIT * 4;
        smoke->vexp = V_EXPANSION + 5.0 * rand() / (RAND_MAX + 1.0) * car->_exhaustPower / 2.0;
        smoke->smokeType = SMOKE_TYPE_ENGINE;
        smoke->smokeTypeStep = 0;
        smoke->lastTime = t;
        smoke->transform(grCarInfo[car->index].carPos);
        SmokeAnchor->addKid(smoke);
        ret = true;
    }//if type

    return ret;
}//cGrSmoke::Add
