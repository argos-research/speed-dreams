#include "followDriver.h"
#include "robottools.h"


#include <car.h>
#include <track.h>
#include <raceman.h>


FollowDriver::FollowDriver(tdble distThreshold, tdble followDist, tdble maxAccel, tdble maxBrake)
: m_tracker(PositionTracker(distThreshold))
, m_settings{distThreshold, followDist, maxAccel, maxBrake}
{
}

void FollowDriver::drive(tCarElt *car, tSituation *s, tTrack* track)
{
    // Get position of nearest opponent in front
    // via: - Sensor utility
    //      - situation data
    // If distance below certain threshold
    // Drive in that direction (set angle)
    // If position in previous frame is known:
    //     Calculate speed from old and new world positon
    //     Try to adjust accel and brake to match speed of opponent
    //     (Try to shift gear accordingly)
    // Save new world position in old position

    m_tracker.updatePosition(car, s, track);

    vec2 curLeadPos = m_tracker.getCurLeadPos();

    if(!m_tracker.isPositionTracked())
    {
        return;
    }

    vec2 ownPos = vec2(car->_pos_X, car->_pos_Y);

    // Get point of view axis of car in world coordinates
    // by substracting the positon of front corners and position of rear corners
    vec2 cfr = vec2(car->_corner_x(FRNT_RGT), car->_corner_y(FRNT_RGT));
    vec2 cfl = vec2(car->_corner_x(FRNT_LFT), car->_corner_y(FRNT_LFT));
    vec2 crr = vec2(car->_corner_x(REAR_RGT), car->_corner_y(REAR_RGT));
    vec2 crl = vec2(car->_corner_x(REAR_LFT), car->_corner_y(REAR_LFT));
    vec2 axis = (cfr - crr) + (cfl - crl);
    axis.normalize();

    //Get angle beween view axis and curleadPos to adjust steer
    vec2 leadVec = curLeadPos - ownPos;
    tdble dist = leadVec.len(); // absolute distance between cars

    // printf("DISTANCE: %f\n", leadVec.len());
    leadVec.normalize();


    // printf("CROSS: %f\n", axis.fakeCrossProduct(&leadVec));
    // printf("ANGLE: %f\n", RAD2DEG(asin(axis.fakeCrossProduct(&leadVec))));

    tdble angle = asin(axis.fakeCrossProduct(&leadVec));
    angle = angle/car->_steerLock;
    car->_steerCmd = angle; // Set steering angle

    // Only possible to calculate accel and brake if speed of leading car known
    if(!m_tracker.isSpeedTracked()) // If position of leading car known in last frame
    {
        return;
    }

    tdble fspeed = car->_speed_x; // speed of following car

    tdble lspeed = m_tracker.getSpeed(s->deltaTime); // speed of leading car

    // adjusted distance to account for different speed, but keep it positive so brake command will not be issued if leading speed is too high
    tdble adist = std::max<tdble>(0.1, m_settings.followDist + (fspeed - lspeed));

    // Accel gets bigger if we are further away from the leading car
    // Accel goes to zero if we are at the target distance from the leading car
    // Target distance is adjusted, dependent on the the speed difference of both cars
    // Accel = maxAccel if dist = threshold
    // Accel = 0 if dist = adist (adjusted target dist)
    car->_accelCmd = std::sqrt(std::max<tdble>(0, std::min<tdble>(m_settings.maxAccel, m_settings.maxAccel * (dist - adist) / (m_settings.distThreshold - adist))));

    // Äquivalent to accel but the other way round
    tdble b = std::sqrt(std::max<tdble>(0, std::min<tdble>(m_settings.maxBrake, m_settings.maxBrake * (adist - dist) / adist)));

    // Individual brake commands for each wheel
    car->_singleWheelBrakeMode = 1;
    car->_brakeFLCmd = b;
    car->_brakeFRCmd = b;
    car->_brakeRLCmd = b;
    car->_brakeRRCmd = b;
    car->_brakeCmd = b; // Just for display in TORCS

    car->_gearCmd = getSpeedDepGear(car, car->_gearCmd);
    car->_clutchCmd = 0;
}