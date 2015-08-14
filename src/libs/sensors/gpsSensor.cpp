#include "gpsSensor.h"

#include "randomUtil.h"

#include <car.h>

GPSSensor::GPSSensor()
: m_position(vec2(0,0))
, m_lastPosition(vec2(0,0))
{
}

void GPSSensor::update(tCarElt* car)
{
    m_lastPosition = m_position;
    m_position = vec2(car->_pos_X, car->_pos_Y);
}

vec2 GPSSensor::getPosition()
{
    return vec2(RandomUtil::addNormalNoise(m_position.x, 3.0), RandomUtil::addNormalNoise(m_position.y, 3.0));
}

tdble GPSSensor::getSpeed(double frameTime)
{
    return RandomUtil::addNormalNoise((m_lastPosition - m_position).len() / frameTime, 1.0);
}

