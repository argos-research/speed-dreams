#include "positionTracker.h"

#include <car.h>
#include <track.h>
#include <raceman.h>

PositionTracker::PositionTracker(tdble distThreshold)
: m_info(TrackingInfo())
, m_distThreshold(distThreshold)
{
}

tdble PositionTracker::getDistance(tCarElt* ownCar, tCarElt* leadCar, tTrack* track)
{
    tdble distance = leadCar->_distFromStartLine - ownCar->_distFromStartLine;
    // If leader crossed the start line, but we did not (assuming we are close behind the leader)
    if (leadCar->_distFromStartLine < m_distThreshold && ownCar->_distFromStartLine > track->length - m_distThreshold)
    {
        distance = (leadCar->_distFromStartLine + track->length) - ownCar->_distFromStartLine;
    }

    return distance;
}

tdble PositionTracker::getSpeed(double frameTime)
{
    return (m_info.lastLeadPos - m_info.curLeadPos).len() / frameTime;
}

void PositionTracker::updatePosition(tCarElt* car, tSituation* s, tTrack* track)
{
    vec2 ownPos = vec2(car->_pos_X, car->_pos_Y);
    tdble minDist = FLT_MAX;

    // If we are currently following somebody
    if(m_info.leadCar != nullptr)
    {
        tdble distance = getDistance(car, m_info.leadCar, track);
        // If distance is to far, or we overtook car, stop following that car
        if(distance > m_distThreshold || distance <= 0.0)
        {
            printf("LOST LEADER\n");
            //Reset tracking info
            m_info = TrackingInfo();
        }
        else
        {
            m_info.lastLeadPos = m_info.curLeadPos;
            m_info.curLeadPos = vec2(m_info.leadCar->_pos_X, m_info.leadCar->_pos_Y);
            m_info.speedTracked = true;
            printf("ASSIGN LEADER POSITION: x: %f, y: %f\n", m_info.curLeadPos.x, m_info.curLeadPos.y);
        }
    }

    // If not following somebody
    if(m_info.leadCar == nullptr)
    {
        for(int i = 0; i < s->_ncars; i++)
        {
            tCarElt* lead = s->cars[i]; //possibly leading car

            if(lead == car) continue; //if own car

            tdble distance = getDistance(car, lead, track);

            // If distance is negative, we are in front of car (not possible to follow)
            if(distance <= 0.0) continue;

            // If distance is larger than our follow mode distance threshold
            if(distance > m_distThreshold) continue;

            vec2 leadPos = vec2(lead->_pos_X, lead->_pos_Y);
            tdble leadDist = ownPos.dist(leadPos);
            if(leadDist < minDist)
            {
                printf("FOUND LEADER at global position (%f, %f) in distance %f\n", leadPos.x, leadPos.y, distance);
                // From now on we may follow this car
                m_info.leadCar = lead;
                m_info.curLeadPos = leadPos;
                minDist = leadDist;
            }
        }
    }
    return;
}