#ifndef GPS_TRACKER_H
#define GPS_TRACKER_H

#include <linalg_t.h> //v2t
#include <tgf.h>

typedef v2t<tdble> vec2;

struct CarElt;
typedef struct CarElt tCarElt;

class GPSSensor
{
public:
	GPSSensor();
	void update(tCarElt* car);
	vec2 getPosition();
	tdble getSpeed(double frameTime);

private:
	vec2 m_position;
	vec2 m_lastPosition;
};

#endif