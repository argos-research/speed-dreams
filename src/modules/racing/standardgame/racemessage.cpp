/***************************************************************************

    file        : racemessage.cpp
    created     : Sat Nov 23 09:05:23 CET 2002
    copyright   : (C) 2002 by Eric Espie
    email       : eric.espie@torcs.org 
    version     : $Id: racemessage.cpp 5081 2012-12-30 18:24:16Z pouillot $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

/** @file   
    		Race message management. Don't use directly, call ReSituation::setRaceMessage.
    @author	<a href=mailto:eric.espie@torcs.org>Eric Espie</a>
    @version	$Id: racemessage.cpp 5081 2012-12-30 18:24:16Z pouillot $
*/

#include <limits>

#include <portability.h>

#include <raceman.h>

#include "standardgame.h"

#include "racemessage.h"

// Avoid C lib <cstdlib> "max" to overload <limits> ones.
#undef max

void
ReRaceMsgManage(tRmInfo* pReInfo)
{
	if (pReInfo->_reMessage && pReInfo->_reCurTime > pReInfo->_reMessageEnd)
	{
		free(pReInfo->_reMessage);
		pReInfo->_reMessage = 0;
	}
	
	if (pReInfo->_reBigMessage && pReInfo->_reCurTime > pReInfo->_reBigMessageEnd)
	{
		free(pReInfo->_reBigMessage);
		pReInfo->_reBigMessage = 0;
	}
}

void
ReRaceMsgSet(tRmInfo* pReInfo, const char *msg, double life)
{
	//GfLogDebug("ReRaceMsgSet('%s', %.2fs)\n", msg ? msg : "<null>", life);
    if (pReInfo->_reMessage)
		free(pReInfo->_reMessage);
	pReInfo->_reMessage = msg ? strdup(msg) : 0;
	if (life < 0)
		pReInfo->_reMessageEnd = std::numeric_limits<double>::max();
	else
		pReInfo->_reMessageEnd = pReInfo->_reCurTime + life;
}

void
ReRaceMsgSetBig(tRmInfo* pReInfo, const char *msg, double life)
{
	//GfLogDebug("ReRaceMsgSetBig('%s', %.2fs)\n", msg ? msg : "<null>", life);
    if (pReInfo->_reBigMessage)
		free(pReInfo->_reBigMessage);
	pReInfo->_reBigMessage = msg ? strdup(msg) : 0;
	if (life < 0)
		pReInfo->_reBigMessageEnd = std::numeric_limits<double>::max();
	else
		pReInfo->_reBigMessageEnd = pReInfo->_reCurTime + life;
}
