#ifndef POSITION_TRACKER_H
#define POSITION_TRACKER_H

#include <linalg_t.h> //v2t
#include <tgf.h> //tdble

typedef v2t<tdble> vec2;

struct CarElt;
typedef struct CarElt tCarElt;
struct Track;
typedef struct Track tTrack;
struct Situation;
typedef struct Situation tSituation;

struct TrackingInfo
{
    tCarElt* leadCar; // Car structure of current leader
    vec2 curLeadPos; // Position of leader in current frame
    vec2 lastLeadPos; // Position of leader in last frame
    bool speedTracked; // Indicates if same car was leading last frame, enabling speed calculation

    TrackingInfo()
    : leadCar(nullptr)
    , curLeadPos(vec2(-1, -1))
    , lastLeadPos(vec2(-1, -1))
    , speedTracked(false)
    {
    }

};

class PositionTracker
{
public:
    PositionTracker(tdble distThreshold = 50.0);

    // Update leading car and its positions
    // Especially check if we are still able to follow the car and may be choose a
    // new leader if the old one is beyond our distance threshold
    void updatePosition(tCarElt* car, tSituation *s, tTrack* track);

    vec2 getCurLeadPos(){return addNoiseV2(m_info.curLeadPos, 0.5);};
    vec2 getLastLeadPos(){return addNoiseV2(m_info.lastLeadPos, 0.5);};
    bool isSpeedTracked() {return m_info.speedTracked;};
    bool isPositionTracked() {return m_info.leadCar != nullptr;};

    tdble getSpeed(double frameTime);

    void setDistThreshold(tdble distThreshold) {m_distThreshold = distThreshold;};

private:

    // Returns signed distance between own car and leading car
    // if leading car is in front of own car, returned distance will be positive
    // otherwise, distance will be negative
    tdble getDistance(tCarElt* ownCar, tCarElt* leadCar, tTrack* track);

    // Add normal noise to a 2d vector (i.e. positions)
    vec2 addNoiseV2(vec2 v, double deviation);

    // Member variables
    TrackingInfo m_info;

    tdble m_distThreshold;

};

#endif //POSITION_TRACKER_H