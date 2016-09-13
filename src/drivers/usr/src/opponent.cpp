/***************************************************************************

    file                 : opponent.cpp
    created              : Thu Apr 22 01:20:19 CET 2003
    copyright            : (C) 2003-2004 Bernhard Wymann
    email                : berniw@bluewin.ch
    version              : $Id: opponent.cpp 5950 2015-04-05 19:34:04Z torcs-ng $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "opponent.h"

//#define OPP_DEBUG
// class variables and constants.
tTrack* Opponent::track;
const float Opponent::FRONTCOLLDIST = 200.0f;			// [m] distance on the track to check other cars.
const float Opponent::BACKCOLLDIST = 70.0f;				// [m] distance on the track to check other cars.
const float Opponent::LENGTH_MARGIN = 1.0f;				// [m] savety margin.
const float Opponent::SIDE_MARGIN = 1.0f;				// [m] savety margin.
const float Opponent::EXACT_DIST = 12.0f;				// [m] if the estimated distance is smaller, compute it more accurate
const float Opponent::LAP_BACK_TIME_PENALTY = -30.0f;	// [s]
const float Opponent::OVERLAP_WAIT_TIME = 5.0f;			// [s] overlaptimer must reach this time before we let the opponent pass.
const float Opponent::SPEED_PASS_MARGIN = 5.0f;			// [m/s] avoid overlapping opponents to stuck behind me.

#define MAG(x, y) (sqrt((x)*(x) + (y)*(y)))

Opponent::Opponent() : 
    distance(0.0f),
    brakedistance(0.0f),
    catchdist(0.0f),
    sidedist(0.0f),
    deltamult(0.0f),
    speedangle(0.0f),
    prevspeedangle(0.0f),
    lastyr(0.0f),
    prevleft(0.0f),
    t_impact(0.0f),
    brakemargin(0.0f),
    state(0),
    team(0),
    teamindex(0),
    index(0),
    overlaptimer(0.0f),
    car(NULL),
    cardata(NULL),
    teammate(false)
{
    team = -1;
    deltamult = lastyr = 0.0f;
    distance = brakedistance = catchdist = sidedist = prevleft = t_impact = overlaptimer = 0.0f;
    state = index = 0;
    car = NULL;
    cardata = NULL;
    teammate = false;
    track = NULL;
}

float Opponent::GetCloseDistance( float distn, tCarElt *mycar )
{
    straight2f carFrontLine(
                mycar->_corner_x(FRNT_LFT),
                mycar->_corner_y(FRNT_LFT),
                mycar->_corner_x(FRNT_RGT) - mycar->_corner_x(FRNT_LFT),
                mycar->_corner_y(FRNT_RGT) - mycar->_corner_y(FRNT_LFT)
                );

    float mindist = FLT_MAX;
    int i;
    for (i = 0; i < 4; i++)
    {
        vec2f corner(car->_corner_x(i), car->_corner_y(i));
        float dist = carFrontLine.dist(corner);
        if (dist < mindist)
        {
            mindist = dist;
        }
    }

    if (mindist < distn)
    {
        distn = mindist;
    }

    return distn;
}

void Opponent::update(tSituation *s, Driver *driver, int DebugMsg)
{
    tCarElt *mycar = driver->getCarPtr();
    cardata->update();

    // Init state of opponent to ignore.
    state = OPP_IGNORE;

    if (team == -1)
    {
        if ((!strcmp(car->_teamname, mycar->_teamname)))
        {
            team = TEAM_FRIEND;
        }
        else
            team = TEAM_FOE;
        deltamult = (float)(1.0 / s->deltaTime);
        brakemargin = (float)driver->getBrakeMargin();
    }

    // If the car is out of the simulation ignore it.
    if (car->_state & (RM_CAR_STATE_NO_SIMU & ~RM_CAR_STATE_PIT)) {
        return;
    }

    // Updating distance along the middle.
    //float oppToStart = car->_trkPos.seg->lgfromstart + getDistToSegStart();
    //distance = oppToStart - mycar->_distFromStartLine;
    distance = car->_distFromStartLine - mycar->_distFromStartLine;
    if (car->_distFromStartLine < 100.0 && mycar->_distFromStartLine > track->length-100.0)
        distance = (car->_distFromStartLine+track->length) - mycar->_distFromStartLine;

    if (distance > track->length/2.0f)
    {
        distance -= track->length;
    } else if (distance < -track->length/2.0f)
    {
        distance += track->length;
    }

    //double ospeed = MAG(car->_speed_Y, car->_speed_X);
    //double mspeed = MAG(mycar->_speed_Y, mycar->_speed_X);
    double ospeed = getSpeed();
    double mspeed = driver->getSpeed();
    //t_impact = MIN(10.0, MAX(0.0, (distance) / (mspeed - ospeed)));

    float SIDECOLLDIST = MAX(car->_dimension_x, mycar->_dimension_x);
    nextleft = car->_trkPos.toLeft + (car->_trkPos.toLeft - prevleft);
    prevspeedangle = speedangle;
    speedangle = (float) -(cardata->getTrackangle() - atan2(car->_speed_Y, car->_speed_X));
    FLOAT_NORM_PI_PI(speedangle);

    float trackangle = RtTrackSideTgAngleL(&(car->_trkPos));
    angle = trackangle - car->_yaw;
    FLOAT_NORM_PI_PI(angle);
    angle = -angle;

    // Is opponent in relevant range -BACKCOLLDIST..FRONTCOLLDIST m.
    if (distance > -BACKCOLLDIST && distance < FRONTCOLLDIST)
    {
#ifdef OPP_DEBUG
        fprintf(stderr,"## %s d=%.3f spd=%.3f acx=%.3f ",car->_name,distance,ospeed,car->_accel_x);
#endif
        // Is opponent aside.
        if (distance > -SIDECOLLDIST &&
                distance < car->_dimension_x)
        {
            sidedist = car->_trkPos.toMiddle - mycar->_trkPos.toMiddle;
            state |= OPP_SIDE;
#ifdef OPP_DEBUG
            fprintf(stderr," SIDE sd=%.3f",sidedist);
#endif
            float drsa = driver->getSpeedAngle();

            if (team == TEAM_FRIEND && car->_dammage-1000 < mycar->_dammage && distance > 2.0)
                state |= OPP_FRONT_FOLLOW;

            if (car->_trkPos.toLeft > mycar->_trkPos.toLeft)
                sidedist -= (speedangle - drsa) * 10;
            else
                sidedist -= (drsa - speedangle) * 10;
#ifdef OPP_DEBUG
            fprintf(stderr," %.3f (o%.3f d%.3f)\n",sidedist,speedangle,drsa);
#endif

            if (distance >= 1.0 && fabs(sidedist) < 2.0 && mspeed < ospeed+distance)
            {
                int collide = testCollision(driver, 0.0, (mspeed < 5.0 ? 1.0 : 0.0));
                if (DebugMsg & debug_brake)
                    fprintf(stderr,"SIDECOLLIDE %s %d\n",car->_name,collide);

                if (collide)
                    state |= (OPP_COLL | OPP_SIDE_COLL);
            }
        }

        // Is opponent in front and slower.
        if (distance > car->_dimension_x*0.9 && mspeed > ospeed)
        {
            state |= OPP_FRONT;
#ifdef OPP_DEBUG
            fprintf(stderr," FRONTSLOW\n");
#endif

            if (team == TEAM_FRIEND && car->_dammage-1000 < mycar->_dammage)
                state |= OPP_FRONT_FOLLOW;

            distance -= car->_dimension_x;
            //distance -= LENGTH_MARGIN;

            // If the distance is small we compute it more accurate.
            if (distance < EXACT_DIST)
            {
                distance = GetCloseDistance( distance, mycar );
            }

            catchdist = (float)(mspeed*distance/(mspeed - ospeed));
            //catchdist = driver->getSpeed()*distance/(driver->getSpeed() - getSpeed());
            //t_impact = MAX(0.0, (distance+0.5) / (mspeed - ospeed));
            t_impact = (float)MAX(0.0, (distance) / (mspeed - ospeed));

            if ((s->currentTime > 3.0 || t_impact < 0.7))
            {
                //unused variable lane? (kilo)
                //double lane = driver->getRaceLane() * mycar->_trkPos.seg->width;
                //unused code? (kilo)
                /*
                 * double max = MAX(lane, MAX(mycar->_trkPos.seg->width-mycar->_dimension_y/2, driver->getNextLeft()));
                 * double min = MIN(lane, MIN(mycar->_dimension_y/2, driver->getNextLeft()));
                 */
                //double mleft = MIN(max, MAX(min, (mycar->_trkPos.toLeft+(driver->getNextLeft()-mycar->_trkPos.toLeft) * (t_impact*deltamult)))) - driver->getAngle();
                //double mleft = MIN(max, MAX(min, (mycar->_trkPos.toLeft+(t_impact*driver->getSpeedAngle())) + driver->getAngle()));
                //unused code? (kilo)
                /*
                 * max = MAX(nextleft, car->_trkPos.seg->width-car->_dimension_y/2);
                 * min = MIN(nextleft, car->_dimension_y/2);
                 */
                //double oleft = MIN(max, MAX(min, (car->_trkPos.toLeft+(nextleft-car->_trkPos.toLeft) * (t_impact*deltamult)))) - angle;
                //double oleft = MIN(max, MAX(min, (car->_trkPos.toLeft+(t_impact*speedangle)) + angle));

                //double sepdist = fabs(mleft - oleft);
                double sepdist = fabs(car->_trkPos.toLeft - mycar->_trkPos.toLeft) + driver->getAngle()*2;
                //int diverging = 0; //(sepdist > (fabs(getWidth()/2.0f) + mycar->_dimension_y/3.0f) + 0.3);

                //t_impact = MIN(mspeed/7, t_impact);

#if 1
                if ((distance < 1.0 || t_impact < 0.5) &&
                        sepdist < MAX(car->_dimension_y, getWidth())+0.3)
                {
                    // BT collision method - sorta kinda
                    float cardist = car->_trkPos.toMiddle - mycar->_trkPos.toMiddle;
                    sidedist = cardist;
                    cardist = (float)(sepdist - fabs(getWidth()/2.0f) - mycar->_dimension_y/2.0);
                    //double deduction = t_impact / MAX(1.0, distance);
                    if (cardist < SIDE_MARGIN) {
                        if (DebugMsg & debug_brake)
                            fprintf(stderr,"FRONTCOLLIDE %s cardist=%.3f sepdist=%.3f\n",car->_name,cardist,sepdist);
                        state |= OPP_COLL;
                    }
#ifdef OPP_DEBUG
                    else
                        fprintf(stderr,">>> NOCOLL cd=%.3f >= %.3f ",cardist,SIDE_MARGIN - MAX(0.0, (distance-1.0)/3));
#endif

                }
#endif
#if 1
                if (!(state & OPP_COLL))
                {
                    // hymie collision method
                    int collide = 0;
                    vec2f targ;
                    double targoffset = 0.0, oltargoffset = mycar->_trkPos.toMiddle;// + driver->getSpeedAngle() * 10 * t_impact;
                    int mode = driver->GetMode();

                    if (mode != mode_normal &&
                            MIN(car->_trkPos.toLeft, car->_trkPos.toRight) < -2.0 &&
                            fabs(car->_trkPos.toMiddle - targoffset) > 4.0)
                    {
                        goto nocollide;
                    }

                    if (mode == mode_normal)// && t_impact > 1.0)
                        driver->GetRLSteerPoint(&targ, &targoffset, MAX(0.01, t_impact + 0.06));
                    else
                        driver->GetSteerPoint(0.0, &targ, oltargoffset, MAX(0.01, t_impact + 0.06));


                    //if (mode == mode_normal)
                    collide = testCollision(driver, t_impact, MIN(2.0, MAX(0.0, (mspeed-ospeed)/5)) + brakemargin/2, &targ);
                    //collide = testCollision(driver, t_impact, MIN(1.0, MAX(0.0, 1.0-distance/2))*0.5, &targ);
                    //else
                    //	collide = testCollision(driver, t_impact, MIN(1.0, MAX(0.0, 1.0-distance/2))*0.5, NULL);
                    //else
                    //	collide = testCollision(driver, t_impact, 1.2);

#if 0
                    if (0 && collide && distance > 1.0)
                    {
                        double sidemovt = fabs((driver->getNextLeft()-mycar->_trkPos.toLeft));
                        double cdist = fabs(car->_trkPos.toMiddle-mycar->_trkPos.toMiddle) - fabs(getWidth()/2.0) - mycar->_dimension_y/2;
                        if (cdist > 0.5+fabs(driver->getRInverse())*300 && sidemovt < (100.0 - mycar->_speed_x)/80)
                        {
                            collide = 0;
                        }
                    }
                    else if (distance < 4.0 && !diverging && (mleft < 3.0 || mleft > mycar->_trkPos.seg->width-3.0))
                    {
                        // getting pinched
                        collide = 5;
                    }
#endif

#if 0
                    if (!collide &&
                            (car->_state < RM_CAR_STATE_PIT || team == TEAM_FRIEND) &&
                            (mspeed-ospeed > MAX(distance/2, MAX(mspeed*0.1, mspeed-4.0)) ||
                             driver->getCarefulBrake()))
                    {
                        // sanity check to avoid slamming into a stationary
                        // opponent at high speed
                        float cardist = fabs(car->_trkPos.toMiddle - mycar->_trkPos.toMiddle) - ((fabs(getWidth()/2.0f) + mycar->_dimension_y/2.0f) - 1.5f);
                        if (cardist < SIDE_MARGIN)
                            collide = 6;
                    }
#endif

#if 1
                    {
                        double speed = ospeed;//getSpeed();
                        double dspeed = mspeed;//driver->getSpeed();
                        double time_margin = MAX(6.0, (dspeed-speed) / 12) + brakemargin;
                        double width = cardata->getWidthOnTrack();
                        double dwidth = driver->getWidth();
                        double ow = width;
                        double dw = dwidth;
                        double osa = ((car->_trkPos.toLeft - prevleft)*0.8) * (t_impact/s->deltaTime);
                        double dsa = ((driver->getNextLeft() - mycar->_trkPos.toLeft)*0.8) * (t_impact/s->deltaTime);
                        double oleft = car->_trkPos.toLeft + osa*0.8;
                        double dleft = mycar->_trkPos.toLeft + dsa*0.8;
                        tTrackSeg *cseg = car->_trkPos.seg;
                        tTrackSeg *seg = mycar->_trkPos.seg;


                        if (!collide && t_impact < time_margin/2)
                        {
                            // sanity check for cars that are about to plow full speed into the back of other cars
                            double margin = dwidth + 0.5 + brakemargin;
                            double time = 0.6;
                            double mti = t_impact;

                            if (mti < time || (team == TEAM_FRIEND && mti < 3.0))
                            {
                                double addition = 0.25;//0.5;
                                if (team != TEAM_FRIEND && fabs(car->_trkPos.toMiddle) > fabs(mycar->_trkPos.toMiddle))
                                {
                                    margin *= 1.0;
                                }

                                if (fabs(oleft - dleft) < ow/2 + dw/2 + addition ||
                                        (mti < 0.8 &&
                                         (((cseg->type != TR_STR && cseg->radius <= 120.0) || (seg->type != TR_STR && seg->radius <= 120.0)) &&
                                          ((oleft - ow/2 < margin && dleft - dw/4 < oleft + ow/2) ||
                                           (oleft + ow/2 > cseg->width-margin && dleft + dw/4 > oleft - ow/2)))))
                                {
                                    collide = 8;
                                }
                            }
                        }
                    }
#endif

#ifdef OPP_DEBUG
                    fprintf(stderr,"\n");fflush(stderr);
#endif

nocollide:
                    if (collide)
                    {
                        if (DebugMsg & debug_brake)
                            fprintf(stderr,"%s - %s FRONT COLLIDE=%d\n",mycar->_name,car->_name,collide);
                        state |= OPP_COLL;
                    }
                }
#endif
            }
        }
        // Is opponent behind and faster.
        else if (distance < -SIDECOLLDIST && distance > -(SIDECOLLDIST*5)
                 && ospeed > mspeed - SPEED_PASS_MARGIN)
            //&& getSpeed() > driver->getSpeed() - SPEED_PASS_MARGIN)
        {
#ifdef OPP_DEBUG
            fprintf(stderr," BACK\n");
#endif
            //catchdist = driver->getSpeed()*distance/(getSpeed() - driver->getSpeed());
            catchdist = (tdble) (mspeed*distance/(ospeed - mspeed));
            state |= OPP_BACK;
            distance -= MAX(car->_dimension_x, mycar->_dimension_x);
            distance -= LENGTH_MARGIN;
        }
        // Opponent is in front and faster.
        //else if (distance > SIDECOLLDIST && getSpeed() > driver->getSpeed())
        else if (distance > SIDECOLLDIST && ospeed > mspeed)
        {
#ifdef OPP_DEBUG
            fprintf(stderr," FRONTFAST\n");
#endif
            distance -= car->_dimension_x;
            state |= OPP_FRONT_FAST;
            if (team == TEAM_FRIEND && car->_dammage-1000 < mycar->_dammage)
                state |= OPP_FRONT_FOLLOW;

            //double dSpeed = mspeed;//driver->getSpeed();
            //if (distance < 20.0 - (getSpeed()-driver->getSpeed())*4)
            if (distance < 20.0 - (ospeed-mspeed)*4)
                state |= OPP_FRONT;

            float cardist = car->_trkPos.toMiddle - mycar->_trkPos.toMiddle;
            distance = GetCloseDistance( distance, mycar );
            if (distance <= 0.5 && fabs(cardist) < 6.0 && mspeed < ospeed+distance)
            {
                int collide = testCollision(driver, 0.0, 0.1);
                if (DebugMsg & debug_brake)
                    fprintf(stderr,"%s - %s FRONTFAST COLLIDE=%d\n",mycar->_name,car->_name,collide);

                if (collide)
                    state |= (OPP_COLL | OPP_SIDE_COLL);
            }
        }
    }

    // Check if we should let overtake the opponent.
    updateOverlapTimer(s, mycar, driver->getAlone());
    if (overlaptimer > OVERLAP_WAIT_TIME) {
        state |= OPP_LETPASS;
    }

    brakedistance = distance - car->_dimension_x;
    prevleft = car->_trkPos.toLeft;
}


// Compute the length to the start of the segment.
float Opponent::getDistToSegStart()
{
    if (car->_trkPos.seg->type == TR_STR)
    {
        return car->_trkPos.toStart;
    } else
    {
        return car->_trkPos.toStart*car->_trkPos.seg->radius;
    }
}


// Update overlaptimers of opponents.
void Opponent::updateOverlapTimer(tSituation *s, tCarElt *mycar, int alone)
{
    // If there is an opponent overlapping near by (60 m), let him go imediately
    if ((car->race.laps > mycar->race.laps) && (team != TEAM_FRIEND) && (mycar->_distFromStartLine - car->_distFromStartLine < 60))
        overlaptimer =  (tdble) (OVERLAP_WAIT_TIME + s->deltaTime);
    else
    {
        // Other cases
        if ((car->race.laps > mycar->race.laps && (mycar->_dammage > car->_dammage + 2000)) ||
                (alone && (team == TEAM_FRIEND) && mycar->_dammage > car->_dammage + 2000))
        {
            if (getState() & (OPP_BACK | OPP_SIDE))
            {
                overlaptimer += (tdble) s->deltaTime;
            } else if (getState() & OPP_FRONT)
            {
                overlaptimer = LAP_BACK_TIME_PENALTY;
            } else
            {
                if (overlaptimer > 0.0f)
                {
                    overlaptimer -= (tdble) s->deltaTime;
                } else
                {
                    overlaptimer += (tdble) s->deltaTime;
                }
            }
        } else
        {
            overlaptimer = 0.0;
        }
    }
    lastyr = car->_yaw_rate;
}

int Opponent::polyOverlap( tPosd *op, tPosd *dp )
{
    int i, j;

    // need this to ensure corners are used in the right order
    int cpos[4] = { 1, 0, 2, 3 };

    for (j=0; j<4; j++)
    {
        tPosd *j1 = op + cpos[j];
        tPosd *j2 = op + cpos[((j+1) % 4)];

        for (i=0; i<4; i++)
        {
            tPosd *i1 = dp + cpos[i];
            tPosd *i2 = dp + cpos[((i+1) % 4)];

            double aM, bM, aB, bB, isX=0, isY=0;
            double lineAx1 = j1->ax;
            double lineAx2 = j2->ax;
            double lineAy1 = j1->ay;
            double lineAy2 = j2->ay;
            double lineBx1 = i1->ax;
            double lineBx2 = i2->ax;
            double lineBy1 = i1->ay;
            double lineBy2 = i2->ay;

            if ((lineAx2 - lineAx1) == 0.0)
            {
                if ((lineBx2 - lineBx1) == 0.0)
                    continue;
                isX = lineAx1;
                bM = (lineBy2 - lineBy1) / (lineBx2 - lineBx1);
                bB = lineBy2 - bM * lineBx2;
                isY = bM * isX + bB;
            }
            else if ((lineBx2 - lineBx1) == 0.0)
            {
                isX = lineBx1;
                aM = (lineAy2 - lineAy1) / (lineAx2 - lineAx1);
                aB = lineAy2 - aM * lineAx2;
                isY = aM * isX + aB;
            }
            else
            {
                aM = (lineAy2 - lineAy1) / (lineAx2 - lineAx1);
                bM = (lineBy2 - lineBy1) / (lineBx2 - lineBx1);
                aB = lineAy2 - aM * lineAx2;
                bB = lineBy2 - bM * lineBx2;
                isX = MAX(((bB - aB) / (aM - bM)), 0);
                isY = aM * isX + aB;
            }

            if (isX < MIN(lineAx1, lineAx2) || isX < MIN(lineBx1, lineBx2) || isX > MAX(lineAx1, lineAx2) || isX > MAX(lineBx1, lineBx2))
                continue;
            if (isY < MIN(lineAy1, lineAy2) || isY < MIN(lineBy1, lineBy2) || isY > MAX(lineAy1, lineAy2) || isY > MAX(lineBy1, lineBy2))
                continue;

            return 1;
        }
    }

    return 0;
}

int Opponent::testCollision(Driver *driver, double impact, double sizefactor, vec2f *targ )
{
    tCarElt *mycar = driver->getCarPtr();
    int /*collide = 0,*/ i/*, j*/;
    //double nSlices = MAX(1.0, (impact * deltamult));
    tCarElt *dcar = driver->getCarPtr();
    double o_speedX = car->_speed_X;// + (cardata->getSpeedDeltaX()) * (impact * deltamult) / 2;
    double o_speedY = car->_speed_Y;// + (cardata->getSpeedDeltaY()) * (impact * deltamult) / 2;
    double d_speedX = dcar->_speed_X;// + driver->getSpeedDeltaX() * (impact * deltamult) / 2;
    double d_speedY = dcar->_speed_Y;// + driver->getSpeedDeltaY() * (impact * deltamult) / 2;

    tPosd o_cur[4], d_cur[4], /*o_curp[4], d_curp[4],*/ o_new[4], d_new[4], o_new2[4], d_new2[4];

    // set up car current positions
    for (i=0; i<4; i++)
    {
        o_cur[i].ax = car->_corner_x(i);
        o_cur[i].ay = car->_corner_y(i);
        d_cur[i].ax = dcar->_corner_x(i);
        d_cur[i].ay = dcar->_corner_y(i);
    }

    if (targ)
    {
        // position driver's car at targ
        for (i=0; i<4; i++)
        {
            d_new2[i].ax = d_new[i].ax = dcar->_corner_x(i) + (targ->x - mycar->_pos_X);
            d_new2[i].ay = d_new[i].ay = dcar->_corner_y(i) + (targ->y - mycar->_pos_Y);
        }
    }
    else
    {
        // position driver's car according to velocity angle
        //double d_newPos_x = dcar->pub.DynGC.pos.x + (d_speedX*impact);
        //double d_newPos_y = dcar->pub.DynGC.pos.y + (d_speedY*impact);

        // correct corner positions
        for (i=0; i<4; i++)
        {
            d_new2[i].ax = d_new[i].ax = (tdble) (dcar->_corner_x(i) + (d_speedX*impact));
            d_new2[i].ay = d_new[i].ay = (tdble) (dcar->_corner_y(i) + (d_speedY*impact));
        }
    }

#if 1
    {
        // position opponent car according to velocity angle
        //double o_newPos_x = car->pub.DynGC.pos.x + (o_speedX*impact);
        //double o_newPos_y = car->pub.DynGC.pos.y + (o_speedY*impact);

        // correct corner positions
        for (i=0; i<4; i++)
        {
            o_new2[i].ax = o_new[i].ax = (tdble) (car->_corner_x(i) + (o_speedX*impact));
            o_new2[i].ay = o_new[i].ay = (tdble) (car->_corner_y(i) + (o_speedY*impact));
        }
    }
#endif

    double rincr = (team == TEAM_FRIEND ? 2.0 : 4.0);

    // make our car's front extend a bit according to speed
#if 1
    d_new[FRNT_LFT].ax += (d_new[FRNT_LFT].ax - d_new[REAR_LFT].ax) / 3;
    d_new[FRNT_LFT].ay += (d_new[FRNT_LFT].ay - d_new[REAR_LFT].ay) / 3;
    d_new[FRNT_RGT].ax += (d_new[FRNT_RGT].ax - d_new[REAR_RGT].ax) / 3;
    d_new[FRNT_RGT].ay += (d_new[FRNT_RGT].ay - d_new[REAR_RGT].ay) / 3;
#else
    d_new[FRNT_LFT].ax += d_speedX/5;
    d_new[FRNT_LFT].ay += d_speedY/5;
    d_new[FRNT_RGT].ax += d_speedX/5;
    d_new[FRNT_RGT].ay += d_speedY/5;
#endif

    // make other car's future rectangle a tad larger
    double fsideincr_x = (o_new[FRNT_LFT].ax - o_new[FRNT_RGT].ax) / car->_dimension_x / 2;
    double fsideincr_y = (o_new[FRNT_LFT].ay - o_new[FRNT_RGT].ay) / car->_dimension_x / 2;
    double rsideincr_x = (o_new[REAR_LFT].ax - o_new[REAR_RGT].ax) / car->_dimension_x / 2;
    double rsideincr_y = (o_new[REAR_LFT].ay - o_new[REAR_RGT].ay) / car->_dimension_x / 2;
    double rlftincr_x = ((o_new[REAR_LFT].ax-o_new[FRNT_LFT].ax) / rincr);
    double rlftincr_y = ((o_new[REAR_LFT].ay-o_new[FRNT_LFT].ay) / rincr);
    double rrgtincr_x = ((o_new[REAR_RGT].ax-o_new[FRNT_RGT].ax) / rincr);
    double rrgtincr_y = ((o_new[REAR_RGT].ay-o_new[FRNT_RGT].ay) / rincr);
    //if (team == TEAM_FRIEND)
    //	 sizefactor *= 2;

    {
        fsideincr_x *= sizefactor;
        fsideincr_y *= sizefactor;
        rsideincr_x *= sizefactor;
        rsideincr_y *= sizefactor;
        rlftincr_x *= sizefactor + 1.0;
        rlftincr_y *= sizefactor + 1.0;
        rrgtincr_x *= sizefactor + 1.0;
        rrgtincr_y *= sizefactor + 1.0;
    }


#if 1
    if (t_impact < 1.0)
    {
#if 1
        o_new[REAR_LFT].ax += (tdble) ((o_new[REAR_LFT].ax-o_new[FRNT_RGT].ax) / 6.0);
        o_new[REAR_LFT].ay += (tdble) ((o_new[REAR_LFT].ay-o_new[FRNT_RGT].ay) / 6.0);
        o_new[REAR_RGT].ax += (tdble) ((o_new[REAR_RGT].ax-o_new[FRNT_LFT].ax) / 6.0);
        o_new[REAR_RGT].ay += (tdble) ((o_new[REAR_RGT].ay-o_new[FRNT_LFT].ay) / 6.0);
        o_new[FRNT_LFT].ax += (tdble) ((o_new[FRNT_LFT].ax-o_new[REAR_RGT].ax) / 6.0);
        o_new[FRNT_LFT].ay += (tdble) ((o_new[FRNT_LFT].ay-o_new[REAR_RGT].ay) / 6.0);
        o_new[FRNT_RGT].ax += (tdble) ((o_new[FRNT_RGT].ax-o_new[REAR_LFT].ax) / 6.0);
        o_new[FRNT_RGT].ay += (tdble) ((o_new[FRNT_RGT].ay-o_new[REAR_LFT].ay) / 6.0);
#endif

        o_new[REAR_LFT].ax += (tdble) ((o_new[REAR_LFT].ax-o_new[REAR_RGT].ax) / 4.0);
        o_new[REAR_LFT].ay += (tdble) ((o_new[REAR_LFT].ay-o_new[REAR_RGT].ay) / 4.0);
        o_new[REAR_RGT].ax += (tdble) ((o_new[REAR_RGT].ax-o_new[REAR_LFT].ax) / 4.0);
        o_new[REAR_RGT].ay += (tdble) ((o_new[REAR_RGT].ay-o_new[REAR_LFT].ay) / 4.0);
        o_new[FRNT_LFT].ax += (tdble) ((o_new[FRNT_LFT].ax-o_new[FRNT_RGT].ax) / 4.0);
        o_new[FRNT_LFT].ay += (tdble) ((o_new[FRNT_LFT].ay-o_new[FRNT_RGT].ay) / 4.0);
        o_new[FRNT_RGT].ax += (tdble) ((o_new[FRNT_RGT].ax-o_new[FRNT_LFT].ax) / 4.0);
        o_new[FRNT_RGT].ay += (tdble) ((o_new[FRNT_RGT].ay-o_new[FRNT_LFT].ay) / 4.0);
    }
#endif

    // test for collision
    if (polyOverlap(o_new, d_new))
    {
        //fprintf(stderr,"COLLIDE 1\n");fflush(stderr);
        return 1;
    }

    if (car->_speed_x < dcar->_speed_x/2)
    {
        // no collision, so make rectangles stretch back to current positions
        d_new[REAR_RGT].ax = dcar->_corner_x(FRNT_RGT);
        d_new[REAR_RGT].ay = dcar->_corner_y(FRNT_RGT);
        d_new[REAR_LFT].ax = dcar->_corner_x(FRNT_LFT);
        d_new[REAR_LFT].ay = dcar->_corner_y(FRNT_LFT);

        // test for collision
        if (polyOverlap(o_new, d_new))
        {
            return 2;
        }
    }

#if 0
    if (targ != NULL && t_impact < 1.0)
    {
        // locate front of car at steer target position
        double frx = (d_new[FRNT_RGT].ax + d_new[FRNT_LFT].ax) / 2;
        double fry = (d_new[FRNT_RGT].ay + d_new[FRNT_LFT].ay) / 2;

        d_new[FRNT_RGT].ax = targ->x + (d_cur[FRNT_RGT].ax - frx);
        d_new[FRNT_LFT].ax = targ->x + (d_cur[FRNT_LFT].ax - frx);
        d_new[FRNT_RGT].ay = targ->y + (d_cur[FRNT_RGT].ay - fry);
        d_new[FRNT_LFT].ay = targ->y + (d_cur[FRNT_LFT].ay - fry);

        if (polyOverlap(o_new, d_new))
        {
            //fprintf(stderr,"COLLIDE 5\n");fflush(stderr);
            return 5;
        }
    }
#endif


#if 1
    if (impact < 0.6 || distance < 0.5)
    {
        // close to impact, so lets try a linear prediction
        double ti = MAX(0.001, impact) + 0.1;
        tPosd *o_old1 = cardata->getCorner1();
        tPosd *d_old1 = driver->getCorner1();
        //tPosd *o_old2 = cardata->getCorner2();
        //tPosd *d_old2 = driver->getCorner2();

        for (i=0; i<4; i++)
        {
            o_new2[i].ax = (tdble) (o_cur[i].ax + (o_cur[i].ax - o_old1[i].ax) * (ti * deltamult));
            o_new2[i].ay = (tdble) (o_cur[i].ay + (o_cur[i].ay - o_old1[i].ay) * (ti * deltamult));
            d_new2[i].ax = (tdble) (d_cur[i].ax + (d_cur[i].ax - d_old1[i].ax) * (ti * deltamult));
            d_new2[i].ay = (tdble) (d_cur[i].ay + (d_cur[i].ay - d_old1[i].ay) * (ti * deltamult));
            o_new[i].ax = (tdble) (o_cur[i].ax + (o_speedX*ti));
            o_new[i].ay = (tdble) (o_cur[i].ay + (o_speedY*ti));
            d_new[i].ax = (tdble) (d_cur[i].ax + (d_speedX*ti));
            d_new[i].ay = (tdble) (d_cur[i].ay + (d_speedY*ti));
        }

        o_new[REAR_LFT].ax += (tdble) ((o_new[REAR_LFT].ax-o_new[FRNT_RGT].ax) / 6.0);
        o_new[REAR_LFT].ay += (tdble) ((o_new[REAR_LFT].ay-o_new[FRNT_RGT].ay) / 6.0);
        o_new[REAR_RGT].ax += (tdble) ((o_new[REAR_RGT].ax-o_new[FRNT_LFT].ax) / 6.0);
        o_new[REAR_RGT].ay += (tdble) ((o_new[REAR_RGT].ay-o_new[FRNT_LFT].ay) / 6.0);
        o_new[FRNT_LFT].ax += (tdble) ((o_new[FRNT_LFT].ax-o_new[REAR_RGT].ax) / 6.0);
        o_new[FRNT_LFT].ay += (tdble) ((o_new[FRNT_LFT].ay-o_new[REAR_RGT].ay) / 6.0);
        o_new[FRNT_RGT].ax += (tdble) ((o_new[FRNT_RGT].ax-o_new[REAR_LFT].ax) / 6.0);
        o_new[FRNT_RGT].ay += (tdble) ((o_new[FRNT_RGT].ay-o_new[REAR_LFT].ay) / 6.0);

        o_new[REAR_LFT].ax += (tdble) rlftincr_x;
        o_new[REAR_LFT].ay += (tdble) rlftincr_y;
        o_new[REAR_RGT].ax += (tdble) rrgtincr_x;
        o_new[REAR_RGT].ay += (tdble) rrgtincr_x;
        o_new[REAR_LFT].ax += (tdble) rsideincr_x;
        o_new[REAR_LFT].ay += (tdble) rsideincr_y;
        o_new[REAR_RGT].ax -= (tdble) rsideincr_x;
        o_new[REAR_RGT].ay -= (tdble) rsideincr_y;
        o_new[FRNT_LFT].ax += (tdble) fsideincr_x;
        o_new[FRNT_LFT].ay += (tdble) fsideincr_y;
        o_new[FRNT_RGT].ax -= (tdble) fsideincr_x;
        o_new[FRNT_RGT].ay -= (tdble) fsideincr_y;
        o_new2[REAR_LFT].ax += (tdble) rlftincr_x;
        o_new2[REAR_LFT].ay += (tdble) rlftincr_y;
        o_new2[REAR_RGT].ax += (tdble) rrgtincr_x;
        o_new2[REAR_RGT].ay += (tdble) rrgtincr_y;
        o_new2[REAR_LFT].ax += (tdble) rsideincr_x;
        o_new2[REAR_LFT].ay += (tdble) rsideincr_y;
        o_new2[REAR_RGT].ax -= (tdble) rsideincr_x;
        o_new2[REAR_RGT].ay -= (tdble) rsideincr_y;
        o_new2[FRNT_LFT].ax += (tdble) fsideincr_x;
        o_new2[FRNT_LFT].ay += (tdble) fsideincr_y;
        o_new2[FRNT_RGT].ax -= (tdble) fsideincr_x;
        o_new2[FRNT_RGT].ay -= (tdble) fsideincr_y;

#if 0
        if (team == TEAM_FRIEND)
        {
            tTrackSeg *seg = car->_trkPos.seg;
            double rInv = driver->getRInverse();

            if (rInv > 0.0)
            {
                d_new[FRNT_LFT].ax += ((d_new[FRNT_LFT].ax - d_new[FRNT_RGT].ax) / (dcar->_dimension_y)) * MIN(2.0, rInv * 180);
                o_new[FRNT_LFT].ax += ((o_new[FRNT_LFT].ax - o_new[FRNT_RGT].ax) / (car->_dimension_y)) * MIN(2.0, rInv * 120);
                o_new[REAR_LFT].ax += ((o_new[REAR_LFT].ax - o_new[REAR_RGT].ax) / (car->_dimension_y)) * MIN(2.0, rInv * 120);
                d_new2[FRNT_LFT].ax += ((d_new2[FRNT_LFT].ax - d_new2[FRNT_RGT].ax) / (dcar->_dimension_y)) * MIN(2.5, rInv * 300);
                o_new2[FRNT_LFT].ax += ((o_new2[FRNT_LFT].ax - o_new2[FRNT_RGT].ax) / (car->_dimension_y)) * MIN(2.5, rInv * 160);
                o_new2[REAR_LFT].ax += ((o_new2[REAR_LFT].ax - o_new2[REAR_RGT].ax) / (car->_dimension_y)) * MIN(2.5, rInv * 160);
            }
            else if (rInv < 0.0)
            {
                d_new[FRNT_RGT].ax += ((d_new[FRNT_RGT].ax - d_new[FRNT_LFT].ax) / (dcar->_dimension_y)) * MIN(2.0, fabs(rInv*180));
                o_new[FRNT_RGT].ax += ((o_new[FRNT_RGT].ax - o_new[FRNT_LFT].ax) / (car->_dimension_y)) * MIN(2.0, fabs(rInv*120));
                o_new[REAR_RGT].ax += ((o_new[REAR_RGT].ax - o_new[REAR_LFT].ax) / (car->_dimension_y)) * MIN(2.0, fabs(rInv*120));
                d_new2[FRNT_RGT].ax += ((d_new2[FRNT_RGT].ax - d_new2[FRNT_LFT].ax) / (dcar->_dimension_y)) * MIN(2.5, fabs(rInv*300));
                o_new2[FRNT_RGT].ax += ((o_new2[FRNT_RGT].ax - o_new2[FRNT_LFT].ax) / (car->_dimension_y)) * MIN(2.5, fabs(rInv*160));
                o_new2[REAR_RGT].ax += ((o_new2[REAR_RGT].ax - o_new2[REAR_LFT].ax) / (car->_dimension_y)) * MIN(2.5, fabs(rInv*160));
            }
            if (seg->type == TR_RGT && seg->radius <= 200.0)
            {
                d_new[FRNT_LFT].ax += ((d_new[FRNT_LFT].ax - d_new[FRNT_RGT].ax) / (dcar->_dimension_y)) * ((1.0 - seg->radius/205.0) * 2.0);
                o_new[FRNT_LFT].ax += ((o_new[FRNT_LFT].ax - o_new[FRNT_RGT].ax) / (car->_dimension_y)) * ((1.0 - seg->radius/205.0) * 0.5);
                o_new[REAR_LFT].ax += ((o_new[REAR_LFT].ax - o_new[REAR_RGT].ax) / (car->_dimension_y)) * ((1.0 - seg->radius/125.0) * 0.5);
                d_new2[FRNT_LFT].ax += ((d_new2[FRNT_LFT].ax - d_new2[FRNT_RGT].ax) / (dcar->_dimension_y)) * ((1.0 - seg->radius/205.0) * 2.0);
                o_new2[FRNT_LFT].ax += ((o_new2[FRNT_LFT].ax - o_new2[FRNT_RGT].ax) / (car->_dimension_y)) * ((1.0 - seg->radius/125.0) * 2.0);
                o_new2[REAR_LFT].ax += ((o_new2[REAR_LFT].ax - o_new2[REAR_RGT].ax) / (car->_dimension_y)) * ((1.0 - seg->radius/125.0) * 2.0);
            }
            else if (seg->type == TR_LFT && seg->radius <= 200.0)
            {
                d_new[FRNT_RGT].ax += ((d_new[FRNT_RGT].ax - d_new[FRNT_LFT].ax) / (dcar->_dimension_y)) * ((1.0 - seg->radius/205.0) * 2.0);
                o_new[FRNT_RGT].ax += ((o_new[FRNT_RGT].ax - o_new[FRNT_LFT].ax) / (car->_dimension_y)) * ((1.0 - seg->radius/205.0) * 0.5);
                o_new[REAR_RGT].ax += ((o_new[REAR_RGT].ax - o_new[REAR_LFT].ax) / (car->_dimension_y)) * ((1.0 - seg->radius/125.0) * 0.5);
                d_new2[FRNT_RGT].ax += ((d_new2[FRNT_RGT].ax - d_new2[FRNT_LFT].ax) / (dcar->_dimension_y)) * ((1.0 - seg->radius/205.0) * 2.0);
                o_new2[FRNT_RGT].ax += ((o_new2[FRNT_RGT].ax - o_new2[FRNT_LFT].ax) / (car->_dimension_y)) * ((1.0 - seg->radius/125.0) * 2.0);
                o_new2[REAR_RGT].ax += ((o_new2[REAR_RGT].ax - o_new2[REAR_LFT].ax) / (car->_dimension_y)) * ((1.0 - seg->radius/125.0) * 2.0);
            }
        }
#endif
    }

    // test for collision
    if (polyOverlap(o_new, d_new))
    {
        //fprintf(stderr,"COLLIDE 3\n");fflush(stderr);
        return 3;
    }

    if (impact < 1.0 && polyOverlap(o_new2, d_new2))
    {
        //fprintf(stderr,"COLLIDE 4\n");fflush(stderr);
        return 4;
    }
#endif

    return 0;
}


// Initialize the list of opponents.
Opponents::Opponents(tSituation *s, Driver *driver, Cardata *c)
{
    opponent = new Opponent[s->_ncars - 1];
    int i, j = 0;
    for (i = 0; i < s->_ncars; i++) {
        if (s->cars[i] != driver->getCarPtr()) {
            opponent[j].setCarPtr(s->cars[i]);
            opponent[j].setCarDataPtr(c->findCar(s->cars[i]));
            opponent[j].setIndex(i);
            j++;
        }
    }
    Opponent::setTrackPtr(driver->getTrackPtr());
    nopponents = s->_ncars - 1;
}


Opponents::~Opponents()
{
    delete [] opponent;
}


void Opponents::update(tSituation *s, Driver *driver, int DebugMsg)
{
    int i;
    for (i = 0; i < s->_ncars - 1; i++) {
        opponent[i].update(s, driver, DebugMsg);
    }
}


void Opponents::setTeamMate(const char *teammate)
{
    int i;
    for (i = 0; i < nopponents; i++) {
        if (strcmp(opponent[i].getCarPtr()->_name, teammate) == 0) {
            opponent[i].markAsTeamMate();
            break;	// Name should be unique, so we can stop.
        }
    }
}




