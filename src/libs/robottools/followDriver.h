#ifndef FOLLOW_DRIVER_H
#define FOLLOW_DRIVER_H

#include <positionTracker.h> //vec2, PositionTracker

//Forward declaration
struct CarElt;
typedef struct CarElt tCarElt;
struct Track;
typedef struct Track tTrack;
struct Situation;
typedef struct Situation tSituation;

class PositionTracker;

struct FollowSettings
{
    tdble distThreshold; //
    tdble followDist; // Fixed follow distance
    tdble maxAccel; // maximum amount of acceleration;
    tdble maxBrake; // maximum amount of brake force;
};

class FollowDriver
{
public:
    FollowDriver(tdble distThreshold = 50.0, tdble followDist = 10.0, tdble maxAccel = 1.0, tdble maxBrake = 1.0);

    bool isFollowing(){return m_tracker.isPositionTracked();};

    void drive(tCarElt* car, tSituation* s, tTrack* track);

private:
    PositionTracker m_tracker;
    FollowSettings m_settings;
};

#endif //RT_FOLLOW_DRIVE_H