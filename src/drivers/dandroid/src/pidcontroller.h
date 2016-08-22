/***************************************************************************

    file                 : pidcontroller.h
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

#ifndef _PIDCONTROLLER_H_
#define _PIDCONTROLLER_H_


class PidController  
{
public:
  PidController();
  virtual ~PidController();

  double sample(double propValue);
  double sample(double propValue, double diffValue);

public:
  double m_lastPropValue;  // for calculating differential (if not supplied)
  double m_total;      // for integral.
  double m_maxTotal;      // for integral.
  double m_totalRate;    // for integral.

  double m_p;
  double m_i;
  double m_d;
};


#endif // _PIDCONTROLLER_H_

