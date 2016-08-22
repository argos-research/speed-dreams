/***************************************************************************

    file                 : pidcontroller.cpp
    created              : Apr 09 01:21:49 UTC 2006
    copyright            : (C) 2006 Tim Foden

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#include "pidcontroller.h"


PidController::PidController() :
  m_lastPropValue(0),
  m_total(0),
  m_maxTotal(100),
  m_totalRate(0),
  m_p(1),
  m_i(0),
  m_d(0)
{
}

PidController::~PidController()
{
}

double PidController::sample(double propValue)
{
  return sample(propValue, propValue - m_lastPropValue);
}

double PidController::sample(double propValue, double diffValue)
{
  m_lastPropValue = propValue;

  double  cntrl = propValue * m_p;

  if( m_d != 0 )
  {
    cntrl += diffValue * m_d;
  }

  if( m_i != 0 )
  {
//    m_total = m_total * (1 - m_totalRate) + value * m_totalRate;
    if( m_totalRate == 0 )
      m_total += propValue;
    else
      m_total += (propValue - m_total) * m_totalRate;
    if( m_total > m_maxTotal )
      m_total = m_maxTotal;
    else if( m_total < -m_maxTotal )
      m_total = -m_maxTotal;
    cntrl += m_total * m_i;
  }

  return cntrl;
}
