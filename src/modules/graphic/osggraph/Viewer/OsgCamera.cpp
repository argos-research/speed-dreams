/***************************************************************************

    file                 : OsgCamera.cpp
    created              : Tue Feb 26 12:24:02 CEST 2013
    copyright            : (C) 2013 by Gaëtan André
    email                : gaetan.andre@gmail.com
    version              : $Id$

***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <osg/Camera>
#include <osg/Matrix>
#include <osg/ValueObject>

#include <car.h>
#include <raceman.h>
#include <robottools.h>
#include <portability.h>
#include <tgf.h>
#include <guiscreen.h>
#include <graphic.h>

#include "OsgCamera.h"
#include "OsgView.h"
#include "OsgScenery.h"
#include "OsgMain.h"

static char path[1024];
char 		buf[256];

static float spanfovy;
static float bezelComp;
static float screenDist;
static float arcRatio;
static float spanaspect;
static tdble spanA;

static double lastTime;



SDCamera::SDCamera(SDView  * c, int myid, int mydrawCurrent, int mydrawCkt, int mydrawdrv, int mydrawBackground, int mymirrorAllowed)
{
    screen = c;
    id = myid;
    drawCurrent = mydrawCurrent;
	drawCockpit = mydrawCkt;
    drawDriver = mydrawdrv;
    drawBackground = mydrawBackground;
    mirrorAllowed = mymirrorAllowed;
    speed[0] = speed[1] = speed[2] = 0.0;
    eye[0] = eye[1] = eye[2] = 0.0;
    center[0] = center[1] = center[2] = 0.0;
    up[0] = up[1] = 0.0; up[2] = 1.0;
}

Camera * SDCamera::getGenericCamera()
{
    Camera * c = new Camera;
    c->Centerv = &center._v;
    c->Posv = &eye._v;
    c->Upv = &up._v;
    c->Speedv = &speed._v;
    return c;
}

void SDCamera::setViewOffset(float v)
{

}

void SDCamera::update(tCarElt * car, tSituation * s)
{
}

// SdPerspCamera ================================================================

SDPerspCamera::SDPerspCamera(SDView *myscreen, int id, int drawCurr, int drawCkt, int drawDrv, int drawBG, int mirrorAllowed,
                             float myfovy, float myfovymin, float myfovymax,
                             float myfnear, float myffar, float myfogstart, float myfogend)
    : SDCamera(myscreen, id, drawCurr, drawCkt, drawDrv, drawBG, mirrorAllowed)
{
    fovy     = myfovy;
    fovymin  = myfovymin;
    fovymax  = myfovymax;
    fnear    = myfnear;
    ffar     = myffar;
    fovydflt = myfovy;
    fogstart = myfogstart;
    fogend   = myfogend;

    viewOffset = 0;
    spanOffset = 0;
}

void SDPerspCamera::setProjection(void)
{    
    screen->getOsgCam()->setProjectionMatrixAsPerspective(fovy,screen->getViewRatio() / spanaspect,fnear,ffar);

    // correct view for split screen spanning
    if (viewOffset != 0 && spanOffset != 0)
    {
        float dist, left, right;
        double frnear,frfar,frtop,frbottom,frleft,frright;

        screen->getOsgCam()->getProjectionMatrixAsFrustum(frleft,frright, frbottom, frtop, frnear, frfar);
        if (spanAngle)
            dist = (screenDist / arcRatio) - (((screenDist / arcRatio) - screenDist) * cos(spanAngle));
        else
            dist = screenDist;

        if (dist !=0)
        {
            left = frleft + (spanOffset * frnear/dist);
            right = frright + (spanOffset * frnear/dist);

            screen->getOsgCam()->setProjectionMatrixAsFrustum(left,right,frbottom,frtop,frnear,frfar);
        }
    }
}

void SDPerspCamera::setModelView(void)
{
    screen->getOsgCam()->setViewMatrixAsLookAt(eye,center,up);
}

void SDPerspCamera::loadDefaults(char *attr)
{
    sprintf(path, "%s/%d", GR_SCT_DISPMODE, screen->getId());
    fovy = (float)GfParmGetNum(grHandle, path, attr, (char*)NULL, fovydflt);
    limitFov();
    setProjection();
}

/* Give the height in pixels of 1 m high object on the screen at this point */
float SDPerspCamera::getLODFactor(float x, float y, float z)
{
    tdble	dx, dy, dz, dd;
    float	ang;
    float	res;

    dx = x - eye[0];
    dy = y - eye[1];
    dz = z - eye[2];

    dd = sqrt(dx*dx+dy*dy+dz*dz);

    ang = DEG2RAD(fovy / 2.0);
    res = (float)screen->getScreenHeight() / 2.0 / dd / tan(ang);
    if (res < 0)
    {
        res = 0;
    }
    return res;
}

float SDPerspCamera::getSpanAngle(void)
{
    float angle = 0;

    // check if already computed
    if (fovy == spanfovy)
        return spanAngle;

    fovy = spanfovy;

    //PreCalculate the spanOffset
    if (viewOffset)
    {
        float width = 2 * (bezelComp / 100) * screenDist * tan(spanfovy * M_PI / 360.0) * screen->getViewRatio() / spanaspect;

#if 1
        // New method
        if (arcRatio > 0)
        {
            float fovxR = 2 * atan(width * arcRatio / (2 * screenDist));

            angle = (viewOffset - 10) * fovxR;

            spanOffset = fabs((screenDist / arcRatio) - screenDist) / sqrt((tan((M_PI/2) - angle) * tan((M_PI/2) - angle)) + 1);

            if (viewOffset < 10) spanOffset *= -1;
            if (arcRatio > 1) spanOffset *= -1;
        } else
        {
            // monitors mounted flat on wall
            angle = 0;
            spanOffset = (viewOffset - 10) * width;
        }
#else
        // Old method
        angle = (viewOffset - 10 + (int((viewOffset - 10) * 2) * (bezelComp - 100)/200)) *
                atan(screen->getViewRatio() / spanaspect * tan(spanfovy * M_PI / 360.0)) * 2;

        spanOffset = 0;
#endif
        spanAngle = angle;

        GfLogInfo("ViewOffset %f : fovy %f, arcRatio %f => width %f, angle %f, SpanOffset %f\n", viewOffset, fovy, arcRatio, width, angle, spanOffset);
    }

    return angle;
}

void SDPerspCamera::setViewOffset(float newOffset)
{
    viewOffset = newOffset;

    //PreCalculate the spanAngle and spanOffset
    if (newOffset)
    {
        spanfovy = fovy;
        fovy = 0;
        spanAngle = getSpanAngle();
    } else
    {
        spanOffset = 0;
    }
}

void SDPerspCamera::setZoom(int cmd)
{
    switch(cmd)
    {
    case GR_ZOOM_IN:
        if (fovy > 2)
        {
            fovy--;
        } else
        {
            fovy /= 2.0;
        }

        if (fovy < fovymin)
        {
            fovy = fovymin;
        }

        break;

    case GR_ZOOM_OUT:
        fovy++;
        if (fovy > fovymax)
        {
            fovy = fovymax;
        }
        break;

    case GR_ZOOM_MIN:
        fovy = fovymax;
        break;

    case GR_ZOOM_MAX:
        fovy = fovymin;
        break;

    case GR_ZOOM_DFLT:
        fovy = fovydflt;
        break;
    }

    limitFov();

    if (viewOffset)
    {
        spanfovy = fovy;
        fovy = 0;
        spanAngle = getSpanAngle();
    } else
    {
        spanOffset = 0;
    }

    this->setProjection();
    sprintf(buf, "%s-%d-%d", GR_ATT_FOVY, screen->getCameras()->getIntSelectedList(), getId());
    sprintf(path, "%s/%d", GR_SCT_DISPMODE, screen->getId());
    GfParmSetNum(grHandle, path, buf, (char*)NULL, (tdble)fovy);
    GfParmWriteFile(NULL, grHandle, "Graph");
}


class SDCarCamInsideDriverEye : public SDPerspCamera
{
public:
    SDCarCamInsideDriverEye(SDView *myscreen, int id, int drawCurr, int drawBG,
                            float myfovy, float myfovymin, float myfovymax,
                            float myfnear, float myffar = 1500.0,
                            float myfogstart = 1400.0, float myfogend = 1500.0)
        : SDPerspCamera(myscreen, id, drawCurr, 1, 0, drawBG, 1,
                        myfovy, myfovymin, myfovymax,
                        myfnear, myffar, myfogstart, myfogend)
    {
    }

    void update(tCarElt *car, tSituation *s)
    {
        sgVec3 P, p;
        float offset = 0;

        p[0] = car->_drvPos_x;
        p[1] = car->_drvPos_y;
        p[2] = car->_drvPos_z;
        sgXformPnt3(p, car->_posMat);

        eye[0] = p[0];
        eye[1] = p[1];
        eye[2] = p[2];

        // Compute offset angle and bezel compensation)
        if (viewOffset)
        {
            offset += getSpanAngle();
        }

        P[0] = car->_drvPos_x + 30.0 * cos(2*PI/3 * car->_glance + offset);
        P[1] = car->_bonnetPos_y - 30.0 * sin(2*PI/3 * car->_glance + offset);
        P[2] = car->_drvPos_z;
        sgXformPnt3(P, car->_posMat);

        center[0] = P[0];
        center[1] = P[1];
        center[2] = P[2];

        up[0] = car->_posMat[2][0];
        up[1] = car->_posMat[2][1];
        up[2] = car->_posMat[2][2];

        speed[0] = car->pub.DynGCg.vel.x;
        speed[1] = car->pub.DynGCg.vel.y;
        speed[2] = car->pub.DynGCg.vel.z;

        Speed = car->_speed_x * 3.6;
    }
};


// cGrCarCamInsideDynDriverEye ====================================================
// Change define value to choose desired dynamic behaviour of the CamInsideDriverEye cameras
// * 1 = Torcs's one : strange rotation of the camera around speed vector axis
// * 2 = Use attenuated car yaw to translate camera center (by Andrew)
#define CamDriverEyeDynamicBehaviour 3

class SDCarCamInsideDynDriverEye : public SDCarCamInsideDriverEye
{
#if (CamDriverEyeDynamicBehaviour != 1)
private:
    tdble PreA;
#endif

public:
    SDCarCamInsideDynDriverEye(SDView *myscreen, int id, int drawCurr, int drawBG,
                               float myfovy, float myfovymin, float myfovymax,
                               float myfnear, float myffar = 1500.0,
                               float myfogstart = 1400.0, float myfogend = 1500.0)
        : SDCarCamInsideDriverEye(myscreen, id, drawCurr, drawBG,
                                  myfovy, myfovymin, myfovymax,
                                  myfnear, myffar, myfogstart, myfogend)
    {
#if (CamDriverEyeDynamicBehaviour == 1)
        up[0] = 0;
        up[1] = 0;
        up[2] = 1;
#else
        PreA = 0.0f;
#endif
    }

    void update(tCarElt *car, tSituation *s)
    {
        sgVec3 P, p;
        float offset = 0;

        p[0] = car->_drvPos_x;
        p[1] = car->_drvPos_y;
        p[2] = car->_drvPos_z;
        sgXformPnt3(p, car->_posMat);

        eye[0] = p[0];
        eye[1] = p[1];
        eye[2] = p[2];

        // Compute offset angle and bezel compensation)
        if (viewOffset)
        {
            offset += getSpanAngle();
        }

        P[0] = car->_drvPos_x + 30.0 * cos(2*PI/3 * car->_glance + offset);
        P[1] = car->_drvPos_y - 30.0 * sin(2*PI/3 * car->_glance + offset);
        P[2] = car->_drvPos_z;

#if (CamDriverEyeDynamicBehaviour == 3)
        tdble A = 0;

        // We want uniform movement across split screens when 'spanning'
        if (viewOffset && lastTime == s->currentTime)
        {
            A = spanA;
        } else
        {
            A = car->_yaw;
            if (fabs(PreA - A) > fabs(PreA - A + 2*PI))
            {
                PreA += 2*PI;
            } else if (fabs(PreA - A) > fabs(PreA - A - 2*PI))
            {
                PreA -= 2*PI;
            }
            RELAXATION(A, PreA, 8.0f);
            spanA = A;
        }
        lastTime = s->currentTime;

        // ignore head movement if glancing left/right
        if (car->_glance == 0)
        {
            tdble headTurn = (A - car->_yaw)/2;

            if (headTurn > PI/3) headTurn = PI/3;
            if (headTurn < -PI/3) headTurn = -PI/3;

            P[0] = car->_drvPos_x + 30.0 * cos(2*PI/3 * car->_glance + offset + headTurn);
            P[1] = car->_drvPos_y - 30.0 * sin(2*PI/3 * car->_glance + offset + headTurn);
        }
#endif

        sgXformPnt3(P, car->_posMat);

#if (CamDriverEyeDynamicBehaviour == 2)
        tdble A = 0;

        // We want uniform movement across split screens when 'spanning'
        if (viewOffset && lastTime == s->currentTime) {
            A = spanA;
        } else {
            A = car->_yaw;
            if (fabs(PreA - A) > fabs(PreA - A + 2*PI)) {
                PreA += 2*PI;
            } else if (fabs(PreA - A) > fabs(PreA - A - 2*PI)) {
                PreA -= 2*PI;
            }
            RELAXATION(A, PreA, 4.0f);
            spanA = A;
        }
        lastTime = s->currentTime;

        // ignore if glancing left/right
        if (car->_glance != 0)
            A = 0;

        const tdble CosA = cos(A);
        const tdble SinA = sin(A);

        tdble brake = 0.0f;
        if (car->_accel_x < 0.0)
            brake = MIN(2.0, fabs(car->_accel_x) / 20.0);

        center[0] = P[0] - (10 - 1) * CosA;
        center[1] = P[1] - (10 - 1) * SinA;
        center[2] = P[2]  - brake;

#else
        center[0] = P[0];
        center[1] = P[1];
        center[2] = P[2];
#endif

#if (CamDriverEyeDynamicBehaviour != 1)
        up[0] = car->_posMat[2][0];
        up[1] = car->_posMat[2][1];
        up[2] = car->_posMat[2][2];
#endif

        speed[0] = car->pub.DynGCg.vel.x;
        speed[1] = car->pub.DynGCg.vel.y;
        speed[2] = car->pub.DynGCg.vel.z;

        Speed = car->_speed_x * 3.6;
    }
};

// SDCarCamBehind ================================================================

class SDCarCamBehind : public SDPerspCamera
{
    tdble PreA;

protected:
    float dist;
    float height;
    float relax;

public:
    SDCarCamBehind(SDView *myscreen, int id, int drawCurr, int drawBG,
                   float fovy, float fovymin, float fovymax,
                   float mydist, float myHeight, float fnear, float ffar = 1500.0,
                   float myfogstart = 1400.0, float myfogend = 1500.0, float relaxation = 10.0f)
        : SDPerspCamera(myscreen, id, drawCurr, 0, 1, drawBG, 0, fovy, fovymin,
                        fovymax, fnear, ffar, myfogstart, myfogend)
    {
        dist = mydist;
        height = myHeight;
        relax = relaxation;
        PreA = 0.0;
        up[0] = 0;
        up[1] = 0;
        up[2] = 1;
    }

    void update(tCarElt *car, tSituation *s)
    {
        tdble A;
        float offset = 0;

        // We want uniform movement across split screens when 'spanning'
        if (viewOffset && lastTime == s->currentTime)
        {
            A = spanA;
        } else
        {
            A = car->_yaw;
            if (fabs(PreA - A) > fabs(PreA - A + 2*PI))
            {
                PreA += 2*PI;
            } else if (fabs(PreA - A) > fabs(PreA - A - 2*PI))
            {
                PreA -= 2*PI;
            }

            if (relax > 0.1)
                RELAXATION(A, PreA, relax);
            spanA = A;
        }

        lastTime = s->currentTime;

        eye[0] = car->_pos_X - dist * cos(A + PI * car->_glance);
        eye[1] = car->_pos_Y - dist * sin(A + PI * car->_glance);
        eye[2] = RtTrackHeightG(car->_trkPos.seg, eye[0], eye[1]) + height;

        // Compute offset angle and bezel compensation)
        if (viewOffset)
        {
            offset += getSpanAngle();
        }

        center[0] = car->_pos_X - dist * cos(A + PI * car->_glance) + dist * cos(A + PI * car->_glance - offset);
        center[1] = car->_pos_Y - dist * sin(A + PI * car->_glance) + dist * sin(A + PI * car->_glance - offset);
        center[2] = car->_pos_Z;

        speed[0] = car->pub.DynGCg.vel.x;
        speed[1] = car->pub.DynGCg.vel.y;
        speed[2] = car->pub.DynGCg.vel.z;

        Speed = car->_speed_x * 3.6;
    }
};

// SDCarCamInsideFixedCar ================================================================
class SDCarCamInsideFixedCar : public SDPerspCamera
{
public:
    SDCarCamInsideFixedCar(SDView *myscreen, int id, int drawCurr, int drawBG,
                           float myfovy, float myfovymin, float myfovymax,
                           float myfnear, float myffar = 1500.0,
                           float myfogstart = 1400.0, float myfogend = 1500.0)
        : SDPerspCamera(myscreen, id, drawCurr, 1, 0, drawBG, 1,
                        myfovy, myfovymin, myfovymax,
                        myfnear, myffar, myfogstart, myfogend) {}

    void update(tCarElt *car, tSituation *s)
    {
        sgVec3 P, p;
        float offset = 0;

        p[0] = car->_bonnetPos_x;
        p[1] = car->_bonnetPos_y;
        p[2] = car->_bonnetPos_z;
        sgXformPnt3(p, car->_posMat);

        eye[0] = p[0];
        eye[1] = p[1];
        eye[2] = p[2];

        // Compute offset angle and bezel compensation)
        if (viewOffset)
        {
            offset += getSpanAngle();
        }

        P[0] = car->_bonnetPos_x + 30.0 * cos(2*PI/3 * car->_glance + offset);
        P[1] = car->_bonnetPos_y - 30.0 * sin(2*PI/3 * car->_glance + offset);
        P[2] = car->_bonnetPos_z;
        sgXformPnt3(P, car->_posMat);

        center[0] = P[0];
        center[1] = P[1];
        center[2] = P[2];

        up[0] = car->_posMat[2][0];
        up[1] = car->_posMat[2][1];
        up[2] = car->_posMat[2][2];

        speed[0] =car->pub.DynGCg.vel.x;
        speed[1] =car->pub.DynGCg.vel.y;
        speed[2] =car->pub.DynGCg.vel.z;

        Speed = car->_speed_x * 3.6;
    }
};

// SDCarCamInfrontFixedCar ================================================================

class SDCarCamInfrontFixedCar : public SDPerspCamera
{
public:
    SDCarCamInfrontFixedCar(SDView *myscreen, int id, int drawCurr, int drawBG,
                            float myfovy, float myfovymin, float myfovymax,
                            float myfnear, float myffar = 1500.0,
                            float myfogstart = 1400.0, float myfogend = 1500.0)
        : SDPerspCamera(myscreen, id, drawCurr, 1, 0, drawBG, 1,
                        myfovy, myfovymin, myfovymax,
                        myfnear, myffar, myfogstart, myfogend) {}

    void update(tCarElt *car, tSituation *s)
    {
        sgVec3 P, p;
        float offset = 0;

        p[0] = car->_dimension_x / 2;
        p[1] = car->_bonnetPos_y;
        p[2] = car->_statGC_z;
        sgXformPnt3(p, car->_posMat);

        eye[0] = p[0];
        eye[1] = p[1];
        eye[2] = p[2];

        // Compute offset angle and bezel compensation)
        if (viewOffset)
        {
            offset += getSpanAngle();
        }

#if 0 // SDW test
        spanOffset = car->_glance * (viewOffset - 10) / 5;

        P[0] = (car->_dimension_x / 2) + 30.0 * cos(offset);
        P[1] = car->_bonnetPos_y - 30.0 * sin(offset);
#else
        P[0] = (car->_dimension_x / 2) + 30.0 * cos(2*PI/3 * car->_glance + offset);
        P[1] = car->_bonnetPos_y - 30.0 * sin(2*PI/3 * car->_glance + offset);
#endif
        P[2] = car->_statGC_z;

        sgXformPnt3(P, car->_posMat);

        center[0] = P[0];
        center[1] = P[1];
        center[2] = P[2];

        up[0] = car->_posMat[2][0];
        up[1] = car->_posMat[2][1];
        up[2] = car->_posMat[2][2];

        speed[0] =car->pub.DynGCg.vel.x;
        speed[1] =car->pub.DynGCg.vel.y;
        speed[2] =car->pub.DynGCg.vel.z;

        Speed = car->_speed_x * 3.6;
    }
};

// cGrCarCamBehindReverse ================================================================

class SDCarCamBehindReverse : public SDPerspCamera
{
public:
    SDCarCamBehindReverse (SDView *myscreen, int id, int drawCurr, int drawBG,
                           float myfovy, float myfovymin, float myfovymax,
                           float myfnear, float myffar = 1500.0,
                           float myfogstart = 1400.0, float myfogend = 1500.0)
        : SDPerspCamera(myscreen, id, drawCurr, 0, 0, drawBG, 0,
                        myfovy, myfovymin, myfovymax,
                        myfnear, myffar, myfogstart, myfogend) {}

    void setModelView(void)
    {
        osg::Matrix m;
        m.makeLookAt(eye,center,up);
        osg::Matrix mir(1,0,0,0,
                        0,1,0,0,
                        0,0,-1,0,
                        0,0,0,1);
        osg::Matrix res = m*mir;

        screen->getOsgCam()->setViewMatrix(res);
        screen->getOsgCam()->setUserValue("eye",eye);

        //glFrontFace( GL_CW );
    }

    void update(tCarElt *car, tSituation *s)
    {
        sgVec3 P, p;
        float offset = 0;

        p[0] = car->_bonnetPos_x - (car->_dimension_x/2);
        p[1] = car->_bonnetPos_y;
        p[2] = car->_bonnetPos_z;
        sgXformPnt3(p, car->_posMat);

        eye[0] = p[0];
        eye[1] = p[1];
        eye[2] = p[2];

        // Compute offset angle and bezel compensation)
        if (viewOffset)
        {
            offset += getSpanAngle();
        }

        P[0] = car->_bonnetPos_x - (car->_dimension_x/2) + 30.0 * cos(offset);
        P[1] = car->_bonnetPos_y + 30.0 * sin(offset);
        P[2] = car->_bonnetPos_z;
        sgXformPnt3(P, car->_posMat);

        center[0] = P[0];
        center[1] = P[1];
        center[2] = P[2];

        up[0] = car->_posMat[2][0];
        up[1] = car->_posMat[2][1];
        up[2] = car->_posMat[2][2];

        speed[0] =car->pub.DynGCg.vel.x;
        speed[1] =car->pub.DynGCg.vel.y;
        speed[2] =car->pub.DynGCg.vel.z;
    }
};

// cGrCarCamFront ================================================================

class SDCarCamFront : public SDPerspCamera
{
protected:
    float dist;

public:
    SDCarCamFront(SDView *myscreen, int id, int drawCurr, int drawBG,
                  float fovy, float fovymin, float fovymax,
                  float mydist, float fnear, float ffar = 1500.0,
                  float myfogstart = 1400.0, float myfogend = 1500.0)
        : SDPerspCamera(myscreen, id, drawCurr, 0, 1, drawBG, 0, fovy, fovymin,
                        fovymax, fnear, ffar, myfogstart, myfogend)
    {
        dist = mydist;
        up[0] = 0;
        up[1] = 0;
        up[2] = 1;
    }

    void update(tCarElt *car, tSituation *s) {
        float offset = 0;

        eye[0] = car->_pos_X + dist * cos(car->_yaw + PI * car->_glance);
        eye[1] = car->_pos_Y + dist * sin(car->_yaw + PI * car->_glance);
        eye[2] = RtTrackHeightG(car->_trkPos.seg, eye[0], eye[1]) + 0.5;

        // Compute offset angle and bezel compensation)
        if (viewOffset)
        {
            offset += getSpanAngle();
        }

        center[0] = car->_pos_X + dist * cos(car->_yaw + PI * car->_glance) - dist * cos(car->_yaw + PI * car->_glance - offset);
        center[1] = car->_pos_Y + dist * sin(car->_yaw + PI * car->_glance) - dist * sin(car->_yaw + PI * car->_glance - offset);
        center[2] = car->_pos_Z;

        speed[0] = car->pub.DynGCg.vel.x;
        speed[1] = car->pub.DynGCg.vel.y;
        speed[2] = car->pub.DynGCg.vel.z;

        Speed = car->_speed_x * 3.6;
    }
};

// SCarCamSide ================================================================

class SDCarCamSide : public SDPerspCamera
{
protected:
    float distx;
    float disty;
    float distz;

public:
    SDCarCamSide(class SDView *myscreen, int id, int drawCurr, int drawBG,
                 float fovy, float fovymin, float fovymax,
                 float mydistx, float mydisty, float mydistz,
                 float fnear, float ffar = 1500.0,
                 float myfogstart = 1400.0, float myfogend = 1500.0)
        : SDPerspCamera(myscreen, id, drawCurr, 0, 1, drawBG, 0, fovy, fovymin,
                        fovymax, fnear, ffar, myfogstart, myfogend)
    {
        distx = mydistx;
        disty = mydisty;
        distz = mydistz;

        up[0] = 0;
        up[1] = 0;
        up[2] = 1;
    }

    void update(tCarElt *car, tSituation *s)
    {
        tdble x = car->_pos_X + distx;
        tdble y = car->_pos_Y + disty;
        tdble z = car->_pos_Z + distz;

        eye[0] = x;
        eye[1] = y;
        eye[2] = z;
        center[0] = car->_pos_X;
        center[1] = car->_pos_Y;
        center[2] = car->_pos_Z;

        speed[0] = car->pub.DynGCg.vel.x;
        speed[1] = car->pub.DynGCg.vel.y;
        speed[2] = car->pub.DynGCg.vel.z;

        Speed = car->_speed_x * 3.6;
    }
};

// cGrCarCamUp ================================================================

class SDCarCamUp : public SDPerspCamera
{
protected:
    float distz;

public:
    SDCarCamUp(SDView *myscreen, int id, int drawCurr, int drawBG,
               float fovy, float fovymin, float fovymax,
               float mydistz, int axis,
               float fnear, float ffar = 1500.0,
               float myfogstart = 1600.0, float myfogend = 1700.0)
        : SDPerspCamera(myscreen, id, drawCurr, 0, 1, drawBG, 0, fovy, fovymin,
                        fovymax, fnear, ffar, myfogstart, myfogend)
    {
        distz = mydistz;
        up[2] = 0;
        switch (axis)
        {
        case 0:
            up[0] = 0;
            up[1] = 1;
            break;
        case 1:
            up[0] = 0;
            up[1] = -1;
            break;
        case 2:
            up[0] = 1;
            up[1] = 0;
            break;
        case 3:
            up[0] = -1;
            up[1] = 0;
            break;
        default:
            up[0] = 0;
            up[1] = 1;
            break;
        }
    }

    void update(tCarElt *car, tSituation *s)
    {
        tdble x = car->_pos_X;
        tdble y = car->_pos_Y;
        tdble z = car->_pos_Z + distz;

        eye[0] = x;
        eye[1] = y;
        eye[2] = z;
        center[0] = x;
        center[1] = y;
        center[2] = car->_pos_Z;


        speed[0] = car->pub.DynGCg.vel.x;
        speed[1] = car->pub.DynGCg.vel.y;
        speed[2] = car->pub.DynGCg.vel.z;

        Speed = car->_speed_x * 3.6;
    }
};

// cGrCarCamCenter ================================================================

class SDCarCamCenter : public SDPerspCamera
{
protected:
    float distz;
    float locfar;
    float locfovy;

public:
    SDCarCamCenter(SDView *myscreen, int id, int drawCurr, int drawBG,
                   float fovy, float fovymin, float fovymax,
                   float mydistz,
                   float fnear, float ffar = 1500.0,
                   float myfogstart = 1400.0, float myfogend = 1500.0)
        : SDPerspCamera(myscreen, id, drawCurr, 0, 1, drawBG, 0, fovy, fovymin,
                        fovymax, fnear, ffar, myfogstart, myfogend)
    {
        distz = mydistz;
        locfar = ffar;
        locfovy = fovy;

        eye[0] = SDScenery::getWorldX() * 0.5;
        eye[1] = SDScenery::getWorldY() * 0.6;
        eye[2] = distz;

        up[0] = 0;
        up[1] = 0;
        up[2] = 1;
    }

    void loadDefaults(char *attr)
    {
        sprintf(path, "%s/%d", GR_SCT_DISPMODE, screen->getId());
        locfovy = (float)GfParmGetNum(grHandle, path,
                                      attr, (char*)NULL, fovydflt);
    }

    void setZoom(int cmd)
    {
        fovy = locfovy;
        SDPerspCamera::setZoom(cmd);
        locfovy = fovy;
    }

    void update(tCarElt *car, tSituation *s)
    {
        tdble	dx, dy, dz, dd;

        center[0] = car->_pos_X;
        center[1] = car->_pos_Y;
        center[2] = car->_pos_Z;

        dx = center[0] - eye[0];
        dy = center[1] - eye[1];
        dz = center[2] - eye[2];

        dd = sqrt(dx*dx+dy*dy+dz*dz);

        fnear = dz - 5;
        if (fnear < 1)
        {
            fnear = 1;
        }
        ffar  = dd + locfar;

        fovy = RAD2DEG(atan2(locfovy, dd));

        speed[0] = 0;
        speed[1] = 0;
        speed[2] = 0;

        Speed = car->_speed_x * 3.6;
    }
};

// SDCarCamLookAt ================================================================

class SDCarCamLookAt : public SDPerspCamera
{
protected:

public:
    SDCarCamLookAt(SDView *myscreen, int id, int drawCurr, int drawBG,
                   float fovy, float fovymin, float fovymax,
                   int axis,
                   float eyex, float eyey, float eyez,
                   float centerx, float centery, float centerz,
                   float fnear, float ffar = 1500.0,
                   float myfogstart = 1600.0, float myfogend = 1700.0)
        : SDPerspCamera(myscreen, id, drawCurr, 0, 1, drawBG, 0, fovy, fovymin,
                        fovymax, fnear, ffar, myfogstart, myfogend)
    {

        eye[0] = eyex;
        eye[1] = eyey;
        eye[2] = eyez;

        center[0] = centerx;
        center[1] = centery;
        center[2] = centerz;

        switch (axis)
        {
        case 0:
            up[0] = 0;
            up[1] = 1;
            up[2] = 0;
            break;
        case 1:
            up[0] = 0;
            up[1] = -1;
            up[2] = 0;
            break;
        case 2:
            up[0] = 1;
            up[1] = 0;
            up[2] = 0;
            break;
        case 3:
            up[0] = -1;
            up[1] = 0;
            up[2] = 0;
            break;
        case 4:
            up[0] = 0;
            up[1] = 0;
            up[2] = 1;
            break;
        case 5:
            up[0] = 0;
            up[1] = 0;
            up[2] = -1;
            break;
        default:
            up[0] = 0;
            up[1] = 0;
            up[2] = 1;
            break;
        }
    }

    void update(tCarElt *car, tSituation *s)
    {
    }
};

// cGrCarCamGoPro1 ================================================================

class SDCarCamGoPro1 : public SDPerspCamera
{
protected:

public:
    SDCarCamGoPro1(SDView *myscreen, int id, int drawCurr, int drawBG,
                   float fovy, float fovymin, float fovymax,
                   float fnear, float ffar = 1500.0,
                   float myfogstart = 1400.0, float myfogend = 1500.0)
        : SDPerspCamera(myscreen, id, drawCurr, 0, 1, drawBG, 0, fovy, fovymin,
                        fovymax, fnear, ffar, myfogstart, myfogend) {}

    void update(tCarElt *car, tSituation *s)
    {
        sgVec3 P, p;
        float offset = 0;

        p[0] = car->_drvPos_x;
        p[1] = (car->_dimension_y / 2) + 0.1;
        p[2] = car->_statGC_z;
        sgXformPnt3(p, car->_posMat);

        eye[0] = p[0];
        eye[1] = p[1];
        eye[2] = p[2];

        // Compute offset angle and bezel compensation)
        if (viewOffset)
        {
            offset += getSpanAngle();
        }

        P[0] = 30 * cos(offset);
        P[1] = (car->_dimension_y / 2) + 0.1 - 30.0 * sin(offset);
        P[2] = car->_statGC_z;

        sgXformPnt3(P, car->_posMat);

        center[0] = P[0];
        center[1] = P[1];
        center[2] = P[2];

        up[0] = car->_posMat[2][0];
        up[1] = car->_posMat[2][1];
        up[2] = car->_posMat[2][2];

        speed[0] =car->pub.DynGCg.vel.x;
        speed[1] =car->pub.DynGCg.vel.y;
        speed[2] =car->pub.DynGCg.vel.z;

        Speed = car->_speed_x * 3.6;
    }
};

// cGrCarCamGoPro2 ================================================================

class SDCarCamGoPro2 : public SDPerspCamera
{
protected:

public:
    SDCarCamGoPro2(SDView*myscreen, int id, int drawCurr, int drawBG,
                   float fovy, float fovymin, float fovymax,
                   float fnear, float ffar = 1500.0,
                   float myfogstart = 1400.0, float myfogend = 1500.0)
        : SDPerspCamera(myscreen, id, drawCurr, 0, 1, drawBG, 0, fovy, fovymin,
                        fovymax, fnear, ffar, myfogstart, myfogend) {}

    void update(tCarElt *car, tSituation *s)
    {
        sgVec3 P, p;
        float offset = 0;

        p[0] = car->_drvPos_x;
        p[1] = 0 - (car->_dimension_y / 2) - 0.1;
        p[2] = car->_statGC_z;
        sgXformPnt3(p, car->_posMat);

        eye[0] = p[0];
        eye[1] = p[1];
        eye[2] = p[2];

        // Compute offset angle and bezel compensation)
        if (viewOffset) {
            offset += getSpanAngle();
        }

        P[0] = 30 * cos(offset);
        P[1] = 0 - (car->_dimension_y / 2) - 0.1 - 30.0 * sin(offset);
        P[2] = car->_statGC_z;

        sgXformPnt3(P, car->_posMat);

        center[0] = P[0];
        center[1] = P[1];
        center[2] = P[2];

        up[0] = car->_posMat[2][0];
        up[1] = car->_posMat[2][1];
        up[2] = car->_posMat[2][2];

        speed[0] =car->pub.DynGCg.vel.x;
        speed[1] =car->pub.DynGCg.vel.y;
        speed[2] =car->pub.DynGCg.vel.z;

        Speed = car->_speed_x * 3.6;
    }
};

// SDCarCamRoadZoom ================================================================

class SDCarCamRoadZoom : public SDPerspCamera
{
protected:
    float locfar;
    float locfovy;

public:
    SDCarCamRoadZoom(SDView *myscreen, int id, int drawCurr, int drawBG,
                     float fovy, float fovymin, float fovymax,
                     float fnear, float ffar = 1500.0,
                     float myfogstart = 1400.0, float myfogend = 1500.0)
        : SDPerspCamera(myscreen, id, drawCurr, 0, 1, drawBG, 0, fovy, fovymin,
                        fovymax, fnear, ffar, myfogstart, myfogend)
    {
        locfar = ffar;
        locfovy = fovy;

        up[0] = 0;
        up[1] = 0;
        up[2] = 1;
    }

    void loadDefaults(char *attr)
    {
        sprintf(path, "%s/%d", GR_SCT_DISPMODE, screen->getId());
        locfovy = (float)GfParmGetNum(grHandle, path,
                                      attr, (char*)NULL, fovydflt);
    }

    void setZoom(int cmd)
    {
        fovy = locfovy;
        SDPerspCamera::setZoom(cmd);
        locfovy = fovy;
    }

    void update(tCarElt *car, tSituation *s)
    {
        tdble	dx, dy, dz, dd;
        tRoadCam *curCam;

        curCam = car->_trkPos.seg->cam;

        if (curCam == NULL)
        {
            eye[0] = SDScenery::getWorldX() * 0.5;
            eye[1] = SDScenery::getWorldY() * 0.6;
            eye[2] = 120;
        } else
        {
            eye[0] = curCam->pos.x;
            eye[1] = curCam->pos.y;
            eye[2] = curCam->pos.z;
        }

        center[0] = car->_pos_X;
        center[1] = car->_pos_Y;
        center[2] = car->_pos_Z;

        dx = center[0] - eye[0];
        dy = center[1] - eye[1];
        dz = center[2] - eye[2];

        dd = sqrt(dx*dx+dy*dy+dz*dz);

        fnear = dz - 5;
        if (fnear < 1)
        {
            fnear = 1;
        }
        ffar  = dd + locfar;
        fovy = RAD2DEG(atan2(locfovy, dd));
        limitFov();

        speed[0] = 0.0;
        speed[1] = 0.0;
        speed[2] = 0.0;
    }
};

// SDCarCamRoadNoZoom ================================================================

class SDCarCamRoadNoZoom : public SDPerspCamera
{
protected:

public:
    SDCarCamRoadNoZoom(SDView *myscreen, int id, int drawCurr, int drawBG,
                       float fovy, float fovymin, float fovymax,
                       float fnear, float ffar = 1500.0,
                       float myfogstart = 1400.0, float myfogend = 1500.0)
        : SDPerspCamera(myscreen, id, drawCurr, 0, 1, drawBG, 0, fovy, fovymin,
                        fovymax, fnear, ffar, myfogstart, myfogend)
    {
        up[0] = 0;
        up[1] = 0;
        up[2] = 1;
    }

    void update(tCarElt *car, tSituation *s)
    {
        tRoadCam *curCam;


        curCam = car->_trkPos.seg->cam;

        if (curCam == NULL)
        {
            eye[0] = SDScenery::getWorldX() * 0.5;
            eye[1] = SDScenery::getWorldY() * 0.6;
            eye[2] = 120;
            center[2] = car->_pos_Z;
        } else
        {
            eye[0] = curCam->pos.x;
            eye[1] = curCam->pos.y;
            eye[2] = curCam->pos.z;
            center[2] = curCam->pos.z;
        }

        center[0] = car->_pos_X;
        center[1] = car->_pos_Y;
        center[2] = car->_pos_Z;

        speed[0] = 0.0;
        speed[1] = 0.0;
        speed[2] = 0.0;
    }
};

// SDCarCamRoadFly ================================================================
class SDCarCamRoadFly : public SDPerspCamera
{
protected:
    int current;
    int timer;
    float zOffset;
    float gain;
    float damp;
    float offset[3];
    double currenttime;

public:
    SDCarCamRoadFly(SDView *myscreen, int id, int drawCurr, int drawBG,
                    float fovy, float fovymin, float fovymax,
                    float fnear, float ffar = 1500.0,
                    float myfogstart = 1400.0, float myfogend = 1500.0)
        : SDPerspCamera(myscreen, id, drawCurr, 0, 1, drawBG, 0, fovy, fovymin,
                        fovymax, fnear, ffar, myfogstart, myfogend)
    {
        up[0] = 0;
        up[1] = 0;
        up[2] = 1;
        timer = 0;
        offset[0]=0.0;
        offset[1]=0.0;
        offset[2]=60.0;
        current = -1;
        currenttime = 0.0;
        speed[0] = 0.0;
        speed[1] = 0.0;
        speed[2] = 0.0;
    }

    void update(tCarElt *car, tSituation *s)
    {
        float height;
        float dt;

        if (currenttime == 0.0)
        {
            currenttime = s->currentTime;
        }

        if (currenttime == s->currentTime)
        {
            return;
        }

        bool reset_camera = false;
        dt = s->currentTime - currenttime;
        currenttime = s->currentTime;
        if (fabs(dt) > 1.0f)
        {
            dt = 0.1f; // avoid overflow
            reset_camera = true;
        }

        timer--;
        if (timer<0)
        {
            reset_camera = true;
        }

        if (current != car->index)
        {
            /* the target car changed */
            zOffset = 50.0;
            current = car->index;
            reset_camera = true;
        } else
        {
            zOffset = 0.0;
        }

        if ((timer <= 0) || (zOffset > 0.0))
        {
            timer = 500 + (int)(500.0*rand()/(RAND_MAX+1.0));
            offset[0] = -0.5 + (rand()/(RAND_MAX+1.0));
            offset[1] = -0.5 + (rand()/(RAND_MAX+1.0));
            offset[2] = 10.0f + (50.0*rand()/(RAND_MAX+1.0)) + zOffset;
            offset[0] = offset[0]*(offset[2]+1.0);
            offset[1] = offset[1]*(offset[2]+1.0);
            // follow the car more closely when low
            gain = 300.0/(10.0f+offset[2]);
            damp = 5.0f;
        }

        if (reset_camera)
        {
            eye[0] = car->_pos_X + 50.0 + (50.0*rand()/(RAND_MAX+1.0));
            eye[1] = car->_pos_Y + 50.0 + (50.0*rand()/(RAND_MAX+1.0));
            eye[2] = car->_pos_Z + 50.0 + (50.0*rand()/(RAND_MAX+1.0));
            speed[0] = speed[1] = speed[2] = 0.0f;
        }

        speed[0] += (gain*(offset[0]+car->_pos_X - eye[0]) - speed[0]*damp)*dt;
        speed[1] += (gain*(offset[1]+car->_pos_Y - eye[1]) - speed[1]*damp)*dt;
        speed[2] += (gain*(offset[2]+car->_pos_Z - eye[2]) - speed[2]*damp)*dt;

        eye[0] = eye[0] + speed[0]*dt;
        eye[1] = eye[1] + speed[1]*dt;
        eye[2] = eye[2] + speed[2]*dt;

        center[0] = (car->_pos_X);
        center[1] = (car->_pos_Y);
        center[2] = (car->_pos_Z);

        // avoid going under the scene
        //height = grGetHOT(eye[0], eye[1]) + 1.0;
        height =1;
        if (eye[2] < height)
        {
            timer = 500 + (int)(500.0*rand()/(RAND_MAX+1.0));
            offset[2] = height - car->_pos_Z + 1.0;
            eye[2] = height;
        }
    }

    void onSelect(tCarElt *car, tSituation *s)
    {
        timer = 0;
        current = -1;
    }
};

// cGrCarCamBehind2 ================================================================

class SDCarCamBehind2 : public SDPerspCamera
{
    tdble PreA;

protected:
    float dist;

public:
    SDCarCamBehind2(SDView *myscreen, int id, int drawCurr, int drawBG,
                    float fovy, float fovymin, float fovymax,
                    float mydist, float fnear, float ffar = 1500.0,
                    float myfogstart = 1400.0, float myfogend = 1500.0)
        : SDPerspCamera(myscreen, id, drawCurr, 0, 1, drawBG, 0, fovy, fovymin,
                        fovymax, fnear, ffar, myfogstart, myfogend)
    {
        dist = mydist;
        PreA = 0.0;
        up[0] = 0;
        up[1] = 0;
        up[2] = 1;
    }

    void update(tCarElt *car, tSituation *s)
    {
        tdble A;
        tdble CosA;
        tdble SinA;
        tdble x;
        tdble y;

        A = RtTrackSideTgAngleL(&(car->_trkPos));

        if (fabs(PreA - A) > fabs(PreA - A + 2*PI))
        {
            PreA += 2*PI;
        } else if (fabs(PreA - A) > fabs(PreA - A - 2*PI))
        {
            PreA -= 2*PI;
        }

        RELAXATION(A, PreA, 5.0);
        CosA = cos(A);
        SinA = sin(A);
        x = car->_pos_X - dist * CosA;
        y = car->_pos_Y - dist * SinA;

        eye[0] = x;
        eye[1] = y;
        eye[2] = RtTrackHeightG(car->_trkPos.seg, x, y) + 5.0;

        center[0] = car->_pos_X;
        center[1] = car->_pos_Y;
        center[2] = car->_pos_Z;

        speed[0] = car->pub.DynGCg.vel.x;
        speed[1] = car->pub.DynGCg.vel.y;
        speed[2] = car->pub.DynGCg.vel.z;

        Speed = car->_speed_x * 3.6;
    }
};

// cGrCarCamMirror ================================================================

SDCarCamMirror::SDCarCamMirror(SDView *myscreen, int id, int drawCurr, int drawBG,
                               float myfovy, float myfovymin, float myfovymax,
                               float myfnear, float myffar,
                               float myfogstart, float myfogend)
    : SDPerspCamera(myscreen, id, drawCurr, 0, 1, drawBG, 1,
                    myfovy, myfovymin, myfovymax,
                    myfnear, myffar, myfogstart, myfogend)
    , origFovY(myfovy)
{
    this->adaptScreenSize();
}

void SDCarCamMirror::setModelView(void)
{
    osg::Matrix m;
    m.makeLookAt(eye,center,up);

    osg::Matrix mir(1,0,0,0,
                    0,1,0,0,
                    0,0,-1,0,
                    0,0,0,1);

    osg::Matrix res = m*mir;

    screen->getOsgMirrorCam()->setViewMatrix(res);
    screen->getOsgCam()->setUserValue("eye",eye);
}

void SDCarCamMirror::update(tCarElt *car, tSituation * /* s */)
{
    sgVec3 P, p;

    P[0] = car->_bonnetPos_x - (car->_dimension_x/2); // behind car
    P[1] = car->_bonnetPos_y;
    P[2] = car->_bonnetPos_z;
    sgXformPnt3(P, car->_posMat);

    eye[0] = P[0];
    eye[1] = P[1];
    eye[2] = P[2];

    p[0] = car->_bonnetPos_x + 30.0;
    p[1] = car->_bonnetPos_y;
    p[2] = car->_bonnetPos_z;
    sgXformPnt3(p, car->_posMat);

    center[0] = p[0];
    center[1] = p[1];
    center[2] = p[2];

    up[0] = car->_posMat[2][0];
    up[1] = car->_posMat[2][1];
    up[2] = car->_posMat[2][2];

    speed[0] =car->pub.DynGCg.vel.x;
    speed[1] =car->pub.DynGCg.vel.y;
    speed[2] =car->pub.DynGCg.vel.z;
}

void SDCarCamMirror::adaptScreenSize()
{
    vpx = screen->getScreenXPos();
    vpy = screen->getScreenYPos();
    vpw = screen->getScreenWidth();
    vph = screen->getScreenHeight();

    // mirror width adjusted to fit board size
    // int boardW = screen->getBoardWidth();

    int boardW =100;
    mx = vpx + vpw / 2 - (vpw * boardW /400);
    my = vpy +  5 * vph / 6 - vph / 10;
    mw = vpw * boardW /200;
    mh = vph / 6;

    aspectRatio = float(mw) / mh;

    limitFov();

    screen->getOsgMirrorCam()->setProjectionMatrixAsPerspective(fovy,aspectRatio , this->fnear, ffar);
    screen->getOsgMirrorCam()->setViewport(new osg::Viewport(mx,my,mw,mh));
}

void SDCarCamMirror::limitFov(void)
{
    fovy = origFovY / getAspectRatio();
}

// SDCarCamRoadZoomTVD ================================================================
static tdble
GetDistToStart(tCarElt *car)
{
    tTrackSeg	*seg;
    tdble	lg;

    seg = car->_trkPos.seg;
    lg = seg->lgfromstart;

    switch (seg->type)
    {
    case TR_STR:
        lg += car->_trkPos.toStart;
        break;
    default:
        lg += car->_trkPos.toStart * seg->radius;
        break;
    }
    return lg;
}

typedef struct
{
    double	prio;
    int		viewable;
    int		event;
} tSchedView;

class SDCarCamRoadZoomTVD : public SDCarCamRoadZoom
{
    tSchedView *schedView;
    double camChangeInterval;
    double camEventInterval;
    double lastEventTime;
    double lastViewTime;
    tdble  proximityThld;
    int		current;
    int ncars;

public:
    SDCarCamRoadZoomTVD(SDView *myscreen, int id, int drawCurr, int drawBG,
                        float fovy, float fovymin, float fovymax,
                        float fnear,int ncars, float ffar = 1500.0,
                        float myfogstart = 1400.0, float myfogend = 1500.0)
        : SDCarCamRoadZoom(myscreen, id, drawCurr, drawBG, fovy, fovymin,
                           fovymax, fnear, ffar, myfogstart, myfogend)
    {
        this->ncars = ncars;
        schedView = (tSchedView *)calloc(ncars, sizeof(tSchedView));
        if (!schedView)
        {
            GfTrace("malloc error");
            //GfScrShutdown();
            exit (1);
        }

        lastEventTime = 0;
        lastViewTime = 0;
        current = -1;

        camChangeInterval = GfParmGetNum(grHandle, GR_SCT_TVDIR, GR_ATT_CHGCAMINT, (char*)NULL, 10.0);
        camEventInterval  = GfParmGetNum(grHandle, GR_SCT_TVDIR, GR_ATT_EVTINT, (char*)NULL, 1.0);
        proximityThld     = GfParmGetNum(grHandle, GR_SCT_TVDIR, GR_ATT_PROXTHLD, (char*)NULL, 10.0);
    }

    ~SDCarCamRoadZoomTVD() { free(schedView); }

    void update(tCarElt *car, tSituation *s)
    {
        int grNbCars = ncars;
        int	i, j;
        int	curCar;
        double	curPrio;
        double	deltaEventTime = s->currentTime - lastEventTime;
        double	deltaViewTime = s->currentTime - lastViewTime;
        int	event = 0;

        if (current == -1)
        {
            current = 0;
            for (i = 0; i < ncars; i++)
            {
                if (car == s->cars[i])
                {
                    current = i;
                    break;
                }
            }
        }

        /* Track events*/
        if (deltaEventTime > camEventInterval)
        {
            memset(schedView, 0, ncars * sizeof(tSchedView));
            for (i = 0; i < grNbCars; i++)
            {
                schedView[i].viewable = 1;
            }

            car = screen->getCurrentCar();
            schedView[car->index].viewable = 0;
            schedView[car->index].prio -= 10000;

            for (i = 0; i < grNbCars; i++)
            {
                tdble dist, fs;

                car = s->cars[i];
                schedView[car->index].prio += grNbCars - i;
                fs = GetDistToStart(car);

                if ((car->_state & RM_CAR_STATE_NO_SIMU) != 0)
                {
                    schedView[car->index].viewable = 0;
                } else
                {
                }

                if ((car->_state & RM_CAR_STATE_NO_SIMU) == 0)
                {
                    dist = fabs(car->_trkPos.toMiddle) ;//- grTrack->width / 2.0;
                    /* out of track*/
                    if (dist > 0)
                    {
                        schedView[car->index].prio += grNbCars;
                        if (car->ctrl.raceCmd & RM_CMD_PIT_ASKED)
                        {
                            schedView[car->index].prio += grNbCars;
                            event = 1;
                        }
                    }

                    for (j = i+1; j < grNbCars; j++)
                    {
                        tCarElt *car2 = s->cars[j];
                        tdble fs2 = GetDistToStart(car2);
                        tdble d = fabs(fs2 - fs);

                        if ((car2->_state & RM_CAR_STATE_NO_SIMU) == 0)
                        {
                            if (d < proximityThld)
                            {
                                d = proximityThld - d;
                                schedView[car->index].prio  += d * grNbCars / proximityThld;
                                schedView[car2->index].prio += d * (grNbCars - 1) / proximityThld;
                                if (i == 0)
                                {
                                    event = 1;
                                }
                            }
                        }
                    }

                    if (car->priv.collision)
                    {
                        schedView[car->index].prio += grNbCars;
                        event = 1;
                    }
                } else
                {
                    if (i == current)
                    {
                        event = 1;	/* update view*/
                    }
                }
            }

            /* change current car*/
            if ((event && (deltaEventTime > camEventInterval)) || (deltaViewTime > camChangeInterval))
            {
                int	last_current = current;

                curCar = 0;
                curPrio = -1000000.0;
                for (i = 0; i < grNbCars; i++)
                {
                    if ((schedView[i].prio > curPrio) && (schedView[i].viewable))
                    {
                        curPrio = schedView[i].prio;
                        curCar = i;
                    }
                }

                for (i = 0; i < grNbCars; i++)
                {
                    if (s->cars[i]->index == curCar)
                    {
                        current = i;
                        break;
                    }
                }

                if (last_current != current)
                {
                    lastEventTime = s->currentTime;
                    lastViewTime = s->currentTime;
                }
            }
        }

        screen->setCurrentCar(s->cars[current]);

        SDCarCamRoadZoom::update(s->cars[current], s);
    }
};

SDCameras::SDCameras(SDView *c, int ncars)
{
    cameraHasChanged = false;

    loadSpanValues();

    // Get the factor of visibiity from the graphics settings and from the track.
    tdble fovFactor = GfParmGetNum(grHandle, GR_SCT_GRAPHIC, GR_ATT_FOVFACT, (char*)NULL, 1.0);
    fovFactor *= GfParmGetNum(grTrackHandle, TRK_SECT_GRAPH, TRK_ATT_FOVFACT, (char*)NULL, 1.0);

    //tdble fovFactor =1;
    tdble fixedFar =41000.0;

    // If sky dome is enabled, we have a "fixed far" cut plane.
    // Warning: In theory, 2*grSkyDomeDistance+1 should be enough, but it is not (why ?).
    //const tdble fixedFar = grSkyDomeDistance ? (2.1f * grSkyDomeDistance + 1.0f) : 0;

    //GfLogTrace("Screen #%d : FOV = %.2f, Far=%.0f\n", id, fovFactor, fixedFar);

    screen = c;
    SDView * myscreen = screen;

    int id=0;

    /* F2 - First Person Views  - cameras index 0*/
    /* cam F2 = inside, from the driver's eye, with head movements (driver's view) */
    cameras[0].insert(cameras[0].end(),new SDCarCamInsideDynDriverEye(myscreen,
                                                                      id,
                                                                      1,	/* drawCurr */
                                                                      1,	/* drawBG  */
                                                                      75.5,	/* fovy */
                                                                      10.0,	/* fovymin */
                                                                      95.0,	/* fovymax */
                                                                      0.03,	/* near */
                                                                      fixedFar ? fixedFar : 600.0 * fovFactor,	/* far */
                                                                      fixedFar ? fixedFar/2 : 300.0 * fovFactor,	/* fogstart */
                                                                      fixedFar ? fixedFar : 600.0 * fovFactor	/* fogend */
                                                                                 ));
    id++;

    /* cam F2 = inside, from the driver's eye, without head movements (driver's view) */
    cameras[0].insert(cameras[0].end(),new SDCarCamInsideDriverEye(myscreen,
                                                                   id,
                                                                   1,	/* drawCurr */
                                                                   1,	/* drawBG  */
                                                                   75.5,	/* fovy */
                                                                   10.0,	/* fovymin */
                                                                   95.0,	/* fovymax */
                                                                   0.03,	/* near */
                                                                   fixedFar ? fixedFar : 600.0 * fovFactor,	/* far */
                                                                   fixedFar ? fixedFar/2 : 300.0 * fovFactor,	/* fogstart */
                                                                   fixedFar ? fixedFar : 600.0 * fovFactor	/* fogend */
                                                                              ));
    id++;
    /* cam F2 = inside, from the board (bonnet view), fixed to the car */
    cameras[0].insert(cameras[0].end(),new SDCarCamInsideFixedCar(myscreen,
                                                                  id,
                                                                  1,	/* drawCurr */
                                                                  1,	/* drawBG  */
                                                                  67.5,	/* fovy */
                                                                  10.0,	/* fovymin */
                                                                  95.0,	/* fovymax */
                                                                  0.3,	/* near */
                                                                  fixedFar ? fixedFar : 600.0 * fovFactor,	/* far */
                                                                  fixedFar ? fixedFar/2 : 300.0 * fovFactor,	/* fogstart */
                                                                  fixedFar ? fixedFar : 600.0 * fovFactor	/* fogend */
                                                                             ));
    id++;
    /* cam F2 = inside, from the board (bonnet view), fixed to the car */
    cameras[0].insert(cameras[0].end(),new SDCarCamInfrontFixedCar(myscreen,
                                                                   id,
                                                                   1,	/* drawCurr */
                                                                   1,	/* drawBG  */
                                                                   67.5,	/* fovy */
                                                                   10.0,	/* fovymin */
                                                                   95.0,	/* fovymax */
                                                                   0.3,	/* near */
                                                                   fixedFar ? fixedFar : 600.0 * fovFactor,	/* far */
                                                                   fixedFar ? fixedFar/2 : 300.0 * fovFactor,	/* fogstart */
                                                                   fixedFar ? fixedFar : 600.0 * fovFactor	/* fogend */
                                                                              ));
    id++;
    /* TODO BUG F2 = just behind the car; camera looking back  */
    cameras[0].insert(cameras[0].end(),new SDCarCamBehindReverse(myscreen,
                                                                 id,
                                                                 0,	/* drawCurr */
                                                                 2,	/* drawBG  */
                                                                 67.5,	/* fovy */
                                                                 10.0,	/* fovymin */
                                                                 95.0,	/* fovymax */
                                                                 0.3,	/* near */
                                                                 fixedFar ? fixedFar : 600.0 * fovFactor,	/* far */
                                                                 fixedFar ? fixedFar/2 : 300.0 * fovFactor,	/* fogstart */
                                                                 fixedFar ? fixedFar : 600.0 * fovFactor	/* fogend */
                                                                            ));

    /* F3 - 3rd Person Views - cameras index 1*/
    id=0;

    /* cam F2 = behind the car, near, looking forward */
    cameras[1].insert(cameras[1].end(),new SDCarCamBehind(myscreen,
                                                          id,
                                                          1,	/* drawCurr */
                                                          1,	/* drawBG  */
                                                          40.0,	/* fovy */
                                                          5.0,	/* fovymin */
                                                          95.0,	/* fovymax */
                                                          10.0,	/* dist */
                                                          2.0,	/* height */
                                                          1.0,	/* near */
                                                          fixedFar ? fixedFar : 600.0 * fovFactor,	/* far */
                                                          fixedFar ? fixedFar/2 : 300.0 * fovFactor,	/* fogstart */
                                                          fixedFar ? fixedFar : 600.0 * fovFactor,	/* fogend */
                                                          25.0	/* relaxation */
                                                          ));
    id++;
    /* cam F3 = car behind */
    cameras[1].insert(cameras[1].end(),new SDCarCamBehind(myscreen,
                                                          id,
                                                          1,	/* drawCurr */
                                                          1,	/* drawBG  */
                                                          40.0,	/* fovy */
                                                          5.0,	/* fovymin */
                                                          95.0,	/* fovymax */
                                                          8.0,	/* dist */
                                                          .50,	/* height */
                                                          .50,	/* near */
                                                          fixedFar ? fixedFar : 600.0 * fovFactor,	/* far */
                                                          fixedFar ? fixedFar/2 : 300.0 * fovFactor,	/* fogstart */
                                                          fixedFar ? fixedFar : 600.0 * fovFactor	/* fogend */
                                                                     ));

    id++;
    /* cam F3 = car reverse */
    cameras[1].insert(cameras[1].end(),new SDCarCamFront(myscreen,
                                                         id,
                                                         1,	/* drawCurr */
                                                         1,	/* drawBG  */
                                                         40.0,	/* fovy */
                                                         5.0,	/* fovymin */
                                                         95.0,	/* fovymax */
                                                         8.0,	/* dist */
                                                         0.5,	/* near */
                                                         fixedFar ? fixedFar : 1000.0 * fovFactor,	/* far */
                                                         fixedFar ? fixedFar/2 : 500.0 * fovFactor,	/* fogstart */
                                                         fixedFar ? fixedFar : 1000.0 * fovFactor	/* fogend */
                                                                    ));

    id++;
    /* cam F3 = behind the car, very near, looking forward */
    cameras[1].insert(cameras[1].end(),new SDCarCamBehind(myscreen,
                                                          id,
                                                          1,	/* drawCurr */
                                                          1,	/* drawBG  */
                                                          40.0,	/* fovy */
                                                          5.0,	/* fovymin */
                                                          95.0,	/* fovymax */
                                                          8.0,	/* dist */
                                                          2.5,	/* height */
                                                          1.0,	/* near */
                                                          fixedFar ? fixedFar : 600.0 * fovFactor,	/* far */
                                                          fixedFar ? fixedFar/2 : 300.0 * fovFactor,	/* fogstart */
                                                          fixedFar ? fixedFar : 600.0 * fovFactor,	/* fogend */
                                                          25.0	/* relaxation */
                                                          ));

    /* F4 - Tracking side/front/back shots -index list 2*/
    id=0;

    /* cam F4 = car side 1 */
    cameras[2].insert(cameras[2].end(),new SDCarCamSide(myscreen,
                                                        id,
                                                        1,	/* drawCurr */
                                                        1,	/* drawBG  */
                                                        30.0,	/* fovy */
                                                        5.0,	/* fovymin */
                                                        60.0,	/* fovymax */
                                                        0.0,	/* distx */
                                                        -20.0,	/* disty */
                                                        3.0,	/* distz */
                                                        1.0,	/* near */
                                                        fixedFar ? fixedFar : 1000.0 * fovFactor,	/* far */
                                                        fixedFar ? fixedFar/2 : 500.0 * fovFactor,	/* fogstart */
                                                        fixedFar ? fixedFar : 1000.0 * fovFactor	/* fogend */
                                                                   ));
    id++;
    /* cam F4 = car side 2 */
    cameras[2].insert(cameras[2].end(),new SDCarCamSide(myscreen,
                                                        id,
                                                        1,	/* drawCurr */
                                                        1,	/* drawBG  */
                                                        30.0,	/* fovy */
                                                        5.0,	/* fovymin */
                                                        60.0,	/* fovymax */
                                                        0.0,	/* distx */
                                                        20.0,	/* disty */
                                                        3.0,	/* distz */
                                                        1.0,	/* near */
                                                        fixedFar ? fixedFar : 1000.0 * fovFactor,	/* far */
                                                        fixedFar ? fixedFar/2 : 500.0 * fovFactor,	/* fogstart */
                                                        fixedFar ? fixedFar : 1000.0 * fovFactor	/* fogend */
                                                                   ));
    id++;

    /* cam F4 = car side 3 */
    cameras[2].insert(cameras[2].end(),new SDCarCamSide(myscreen,
                                                        id,
                                                        1,	/* drawCurr */
                                                        1,	/* drawBG  */
                                                        30.0,	/* fovy */
                                                        5.0,	/* fovymin */
                                                        60.0,	/* fovymax */
                                                        -20.0,	/* distx */
                                                        0.0,	/* disty */
                                                        3.0,	/* distz */
                                                        1.0,	/* near */
                                                        fixedFar ? fixedFar : 1000.0 * fovFactor,	/* far */
                                                        fixedFar ? fixedFar/2 : 500.0 * fovFactor,	/* fogstart */
                                                        fixedFar ? fixedFar : 1000.0 * fovFactor	/* fogend */
                                                                   ));
    id++;

    /* cam F4 = car side 4 */
    cameras[2].insert(cameras[2].end(),new SDCarCamSide(myscreen,
                                                        id,
                                                        1,	/* drawCurr */
                                                        1,	/* drawBG  */
                                                        30.0,	/* fovy */
                                                        5.0,	/* fovymin */
                                                        60.0,	/* fovymax */
                                                        20.0,	/* distx */
                                                        0.0,	/* disty */
                                                        3.0,	/* distz */
                                                        1.0,	/* near */
                                                        fixedFar ? fixedFar : 1000.0 * fovFactor,	/* far */
                                                        fixedFar ? fixedFar/2 : 500.0 * fovFactor,	/* fogstart */
                                                        fixedFar ? fixedFar : 1000.0 * fovFactor	/* fogend */
                                                                   ));
    id++;

    /* cam F4 = car side 5 */
    cameras[2].insert(cameras[2].end(),new SDCarCamSide(myscreen,
                                                        id,
                                                        1,	/* drawCurr */
                                                        1,	/* drawBG  */
                                                        30.0,	/* fovy */
                                                        5.0,	/* fovymin */
                                                        60.0,	/* fovymax */
                                                        0.0,	/* distx */
                                                        -40.0,	/* disty */
                                                        6.0,	/* distz */
                                                        1.0,	/* near */
                                                        fixedFar ? fixedFar : 1000.0 * fovFactor,	/* far */
                                                        fixedFar ? fixedFar/2 : 500.0 * fovFactor,	/* fogstart */
                                                        fixedFar ? fixedFar : 1000.0 * fovFactor	/* fogend */
                                                                   ));
    id++;
    /* cam F4 = car side 6 */
    cameras[2].insert(cameras[2].end(),new SDCarCamSide(myscreen,
                                                        id,
                                                        1,	/* drawCurr */
                                                        1,	/* drawBG  */
                                                        30.0,	/* fovy */
                                                        5.0,	/* fovymin */
                                                        60.0,	/* fovymax */
                                                        0.0,	/* distx */
                                                        40.0,	/* disty */
                                                        6.0,	/* distz */
                                                        1.0,	/* near */
                                                        fixedFar ? fixedFar : 1000.0 * fovFactor,	/* far */
                                                        fixedFar ? fixedFar/2 : 500.0 * fovFactor,	/* fogstart */
                                                        fixedFar ? fixedFar : 1000.0 * fovFactor	/* fogend */
                                                                   ));
    id++;

    /* cam F4 = car side 7 */
    cameras[2].insert(cameras[2].end(),new SDCarCamSide(myscreen,
                                                        id,
                                                        1,	/* drawCurr */
                                                        1,	/* drawBG  */
                                                        30.0,	/* fovy */
                                                        5.0,	/* fovymin */
                                                        60.0,	/* fovymax */
                                                        -40.0,	/* distx */
                                                        0.0,	/* disty */
                                                        6.0,	/* distz */
                                                        1.0,	/* near */
                                                        fixedFar ? fixedFar : 1000.0 * fovFactor,	/* far */
                                                        fixedFar ? fixedFar/2 : 500.0 * fovFactor,	/* fogstart */
                                                        fixedFar ? fixedFar : 1000.0 * fovFactor	/* fogend */
                                                                   ));
    id++;

    /* cam F4 = car side 8 */
    cameras[2].insert(cameras[2].end(),new SDCarCamSide(myscreen,
                                                        id,
                                                        1,	/* drawCurr */
                                                        1,	/* drawBG  */
                                                        30.0,	/* fovy */
                                                        5.0,	/* fovymin */
                                                        60.0,	/* fovymax */
                                                        40.0,	/* distx */
                                                        0.0,	/* disty */
                                                        6.0,	/* distz */
                                                        1.0,	/* near */
                                                        fixedFar ? fixedFar : 1000.0 * fovFactor,	/* far */
                                                        fixedFar ? fixedFar/2 : 500.0 * fovFactor,	/* fogstart */
                                                        fixedFar ? fixedFar : 1000.0 * fovFactor	/* fogend */
                                                                   ));
    id++;

    /* F5 - Views from above - list index 3 */
    id=0;

    /* cam F5 = car up 1 */
    cameras[3].insert(cameras[3].end(),new SDCarCamUp(myscreen,
                                                      id,
                                                      1,	/* drawCurr */
                                                      1,	/* drawBG  */
                                                      //12.0,	/* fovy */
                                                      37.5,	/* fovy */
                                                      1.0,	/* fovymin */
                                                      90.0,	/* fovymax */
                                                      //300.0,	/* distz */
                                                      200.0,	/* distz */
                                                      0,		/* axis */
                                                      //200.0,	/* near */
                                                      100.0,	/* near */
                                                      fixedFar ? fixedFar : 1000.0 * fovFactor,/* far */
                                                      fixedFar ? fixedFar/2 : 500.0 * fovFactor,	/* fogstart */
                                                      fixedFar ? fixedFar : 1000.0 * fovFactor	/* fogend */
                                                                 ));
    id++;

    /* cam F5 = car up 2 */
    cameras[3].insert(cameras[3].end(),new SDCarCamUp(myscreen,
                                                      id,
                                                      1,	/* drawCurr */
                                                      1,	/* drawBG  */
                                                      //12.0,	/* fovy */
                                                      37.5,	/* fovy */
                                                      1.0,	/* fovymin */
                                                      90.0,	/* fovymax */
                                                      //300.0,	/* distz */
                                                      250.0,	/* distz */
                                                      1,		/* axis */
                                                      200.0,	/* near */
                                                      fixedFar ? fixedFar : 1000.0 * fovFactor,/* far */
                                                      fixedFar ? fixedFar/2 : 500.0 * fovFactor,	/* fogstart */
                                                      fixedFar ? fixedFar : 1000.0 * fovFactor	/* fogend */
                                                                 ));
    id++;

    /* cam F5 = car up 3 */
    cameras[3].insert(cameras[3].end(),new SDCarCamUp(myscreen,
                                                      id,
                                                      1,	/* drawCurr */
                                                      1,	/* drawBG  */
                                                      //12.0,	/* fovy */
                                                      37.5,	/* fovy */
                                                      1.0,	/* fovymin */
                                                      90.0,	/* fovymax */
                                                      //300.0,	/* distz */
                                                      350.0,	/* distz */
                                                      2,		/* axis */
                                                      200.0,	/* near */
                                                      fixedFar ? fixedFar : 1000.0 * fovFactor,/* far */
                                                      fixedFar ? fixedFar/2 : 500.0 * fovFactor,	/* fogstart */
                                                      fixedFar ? fixedFar : 1000.0 * fovFactor	/* fogend */
                                                                 ));
    id++;

    /* cam F5 = car up 4 */
    cameras[3].insert(cameras[3].end(),new SDCarCamUp(myscreen,
                                                      id,
                                                      1,	/* drawCurr */
                                                      1,	/* drawBG  */
                                                      //12.0,	/* fovy */
                                                      37.5,	/* fovy */
                                                      1.0,	/* fovymin */
                                                      90.0,	/* fovymax */
                                                      //300.0,	/* distz */
                                                      400.0,	/* distz */
                                                      3,		/* axis */
                                                      200.0,	/* near */
                                                      fixedFar ? fixedFar : 1000.0 * fovFactor,/* far */
                                                      fixedFar ? fixedFar/2 : 500.0 * fovFactor,	/* fogstart */
                                                      fixedFar ? fixedFar : 1000.0 * fovFactor	/* fogend */
                                                                 ));
    id++;

    /* F6 - index 4*/
    id=0;

    /* BUG TODO cam F6 = car from circuit centre */
    cameras[4].insert(cameras[4].end(),new SDCarCamCenter(myscreen,
                                                          id,
                                                          1,	/* drawCurr */
                                                          1,	/* drawBG  */
                                                          21.0,	/* fovy */
                                                          2.0,	/* fovymin */
                                                          60.0,	/* fovymax */
                                                          120.0,	/* distz */
                                                          100.0,	/* near */
                                                          fixedFar ? fixedFar : 1000.0 * fovFactor,/* far */
                                                          fixedFar ? fixedFar/2 : 500.0 * fovFactor,/* fogstart */
                                                          fixedFar ? fixedFar : 1000.0 * fovFactor	/* fogend */
                                                                     ));
    id++;


    /* F7 -index 5*/
    id =0;
    /* BUGGY too cam F7 = panoramic */
    cameras[5].insert(cameras[5].end(),new SDCarCamLookAt(myscreen,
                                                          id,
                                                          1,		/* drawCurr */
                                                          0,		/* drawBG  */
                                                          74.0,		/* fovy */
                                                          1.0,		/* fovymin */
                                                          110.0,		/* fovymax */
                                                          0,		/* up axis */
                                                          SDScenery::getWorldX()/2,	/* eyex */
                                                          SDScenery::getWorldY()/2,	/* eyey */
                                                          MAX(SDScenery::getWorldX()/2, SDScenery::getWorldY()*4/3/2) + SDScenery::getWorldZ(), /* eyez */
                                                          SDScenery::getWorldX()/2,	/* centerx */
                                                          SDScenery::getWorldY()/2,	/* centery */
                                                          0,		/* centerz */
                                                          10.0,		/* near */
                                                          SDScenery::getWorldMaxSize() * 2.0,	/* far */
                                                          SDScenery::getWorldMaxSize()* 10.0,	/* fogstart */
                                                          SDScenery::getWorldMaxSize() * 20.0	/* fogend */
                                                          ));

    /* F8 - GoPro like views-index 6*/
    id=0;

    cameras[6].insert(cameras[6].end(),new SDCarCamGoPro1(myscreen,
                                                          id,
                                                          1,	/* drawCurr */
                                                          1,	/* drawBG  */
                                                          67.5,	/* fovy */
                                                          10.0,	/* fovymin */
                                                          95.0,	/* fovymax */
                                                          0.05,	/* near */
                                                          fixedFar ? fixedFar : 600.0 * fovFactor,	/* far */
                                                          fixedFar ? fixedFar : 300.0 * fovFactor,	/* fogstart */
                                                          fixedFar ? fixedFar : 600.0 * fovFactor	/* fogend */
                                                                     ));
    id++;
    cameras[6].insert(cameras[6].end(),new SDCarCamGoPro2(myscreen,
                                                          id,
                                                          1,	/* drawCurr */
                                                          1,	/* drawBG  */
                                                          67.5,	/* fovy */
                                                          10.0,	/* fovymin */
                                                          95.0,	/* fovymax */
                                                          0.05,	/* near */
                                                          fixedFar ? fixedFar : 600.0 * fovFactor,	/* far */
                                                          fixedFar ? fixedFar : 300.0 * fovFactor,	/* fogstart */
                                                          fixedFar ? fixedFar : 600.0 * fovFactor	/* fogend */
                                                                     ));
    /* F9 - TV like coverage -index 7*/

    id=0;
    //TODO correct when params class
    /* cam F9 = road cam zoomed */
    cameras[7].insert(cameras[7].end(),new SDCarCamRoadZoom(myscreen,
                                                            id,
                                                            1,		/* drawCurr */
                                                            1,		/* drawBG  */
                                                            9.0,	/* fovy */
                                                            1.0,		/* fovymin */
                                                            90.0,	/* fovymax */
                                                            1.0,		/* near */
                                                            fixedFar ? fixedFar : 1000.0 * fovFactor,	/* far */
                                                            fixedFar ? fixedFar/2 : 500.0 * fovFactor,	/* fogstart */
                                                            fixedFar ? fixedFar : 1000.0 * fovFactor	/* fogend */
                                                                       ));
    id++;
    //TODO correct when params class
    /* cam F9 = road cam fixed fov */
    cameras[7].insert(cameras[7].end(),new SDCarCamRoadNoZoom(myscreen,
                                                              id,
                                                              1,		/* drawCurr */
                                                              1,		/* drawBG  */
                                                              30.0,	/* fovy */
                                                              5.0,	/* fovymin */
                                                              60.0,	/* fovymax */
                                                              1.0,	/* near */
                                                              fixedFar ? fixedFar : 1000.0 * fovFactor,/* far */
                                                              fixedFar ? fixedFar/2 : 500.0 * fovFactor,	/* fogstart */
                                                              fixedFar ? fixedFar : 1000.0 * fovFactor	/* fogend */
                                                                         ));
    /* F10 - Helicopter like views -index 8*/
    id=0;
    //TODO correct when params class
    cameras[8].insert(cameras[8].end(),new SDCarCamRoadFly(myscreen,
                                                           id,
                                                           1,		/* drawCurr */
                                                           1,		/* drawBG  */
                                                           //17.0,	/* fovy */
                                                           67.5,	/* fovy */
                                                           1.0,		/* fovymin */
                                                           90.0,	/* fovymax */
                                                           1.0,		/* near */
                                                           fixedFar ? fixedFar : 1000.0 * fovFactor,	/* far */
                                                           fixedFar ? fixedFar/2 : 500.0 * fovFactor,	/* fogstart */
                                                           fixedFar ? fixedFar : 1000.0 * fovFactor	/* fogend */
                                                                      ));
    id++;
    //TODO correct when params class
    cameras[8].insert(cameras[8].end(),new SDCarCamBehind2(myscreen,
                                                           id,
                                                           1,	/* drawCurr */
                                                           1,	/* drawBG  */
                                                           40.0,	/* fovy */
                                                           5.0,	/* fovymin */
                                                           95.0,	/* fovymax */
                                                           30.0,	/* dist */
                                                           1.0,	/* near */
                                                           fixedFar ? fixedFar : 1000.0 * fovFactor,	/* far */
                                                           fixedFar ? fixedFar/2 : 500.0 * fovFactor,	/* fogstart */
                                                           fixedFar ? fixedFar : 1000.0 * fovFactor	/* fogend */
                                                                      ));
    /* F11 - The Directors cut -index 9 */
    id=0;
    //TODO correct when params class
    cameras[9].insert(cameras[9].end(),new SDCarCamRoadZoomTVD(myscreen,
                                                               id,
                                                               1,	// drawCurr
                                                               1,	// drawBG
                                                               9.0,	// fovy
                                                               1.0,	// fovymin
                                                               90.0,	// fovymax
                                                               1.0,	// near
                                                               ncars,
                                                               fixedFar ? fixedFar : 1000.0 * fovFactor,	// far
                                                               fixedFar ? fixedFar/2 : 500.0 * fovFactor,	// fogstart
                                                               fixedFar ? fixedFar : 1000.0 * fovFactor	// fogend
                                                                          ));
    selectedCamera =0;
    selectedList=0;

    cameras[selectedList][selectedCamera]->setProjection();
}

SDCamera * SDCameras::getSelectedCamera()
{
    return cameras[selectedList][selectedCamera];
}

void SDCameras::nextCamera(int list)
{
    if(list == selectedList)
    {
        selectedCamera = (selectedCamera +1)%cameras[selectedList].size();
    }else
    {
        selectedCamera =0;
        selectedList = list;
    }

    cameraHasChanged = true;

    cameras[selectedList][selectedCamera]->setViewOffset(screen->getViewOffset());
    cameras[selectedList][selectedCamera]->setProjection();
    this->screen->de_activateMirror();
    screen->saveCamera();
}

void SDCameras::selectCamera(int list,int cam)
{
    if(list>=0 && list< (int)CAMERA_LISTS && cam >=0 && cam<(int)(cameras[list].size()))
    {
        selectedCamera = cam;
        selectedList = list;
    }else
    {
        selectedCamera =0;
        selectedList = 0;
    }

    cameraHasChanged = true;

    cameras[selectedList][selectedCamera]->setViewOffset(screen->getViewOffset());
    cameras[selectedList][selectedCamera]->setProjection();
    this->screen->de_activateMirror();
    screen->saveCamera();
}

void SDCameras::update(tCarElt * car, tSituation * s)
{
    if(cameraHasChanged)
    {
        cameras[selectedList][selectedCamera]->onSelect(car,s);
        cameraHasChanged = false;
    }

    cameras[selectedList][selectedCamera]->update(car,s);
    cameras[selectedList][selectedCamera]->setModelView();
}

SDCameras::~SDCameras()
{
    for(int i=0;i<(int)CAMERA_LISTS;i++)
    {
        for(unsigned j=0; j<cameras[i].size();j++)
        {
            delete cameras[i][j];
        }
    }
}

void SDCameras::loadSpanValues()
{
    /* Check Bezel compensation - used when spaning view across multiple splits */
    bezelComp = (float)GfParmGetNum(grHandle, GR_SCT_GRAPHIC, GR_ATT_BEZELCOMP, "%", 110);
    screenDist= (float)GfParmGetNum(grHandle, GR_SCT_GRAPHIC, GR_ATT_SCREENDIST, NULL, 1);
    arcRatio = (float)GfParmGetNum(grHandle, GR_SCT_GRAPHIC, GR_ATT_ARCRATIO, NULL, 1.0);

    const char *pszMonitorType =
            GfParmGetStr(grHandle, GR_SCT_GRAPHIC, GR_ATT_MONITOR, GR_VAL_MONITOR_16BY9);

    if (strcmp(pszMonitorType, GR_VAL_MONITOR_16BY9) == 0)
        spanaspect = 1.7777f;
    if (strcmp(pszMonitorType, GR_VAL_MONITOR_4BY3) == 0)
        spanaspect = 1.3333f ;
    if (strcmp(pszMonitorType, GR_VAL_MONITOR_NONE) == 0)
        spanaspect = 1.0f;
}
