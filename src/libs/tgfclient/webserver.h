/***************************************************************************
                    webserver.h -- Interface file for The Gaming Framework
                             -------------------
    created              : 04/11/2015
    copyright            : (C) 2015 by MadBad
    email                : madbad82@gmail.com
    version              : $Id$
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
        The Gaming Framework API (client part).
    @author     <a href=mailto:madbad82@gmail.com>MadBad</a>
    @version    $Id$
*/
#ifdef WEBSERVER
#include "tgfclient.h"
#include <vector>
#include <string>
#include <ctime>
#include <curl/multi.h>

struct webRequest_t {
  int id;
  std::string data;
};


class NotificationManager {

	public:
		//a list of notification messages
		std::vector<std::string> msglist;

		std::clock_t animationLastExecTime; //the current time
	
		//constructor
		NotificationManager();
	
		//destructor
		~NotificationManager();

		void updateStatus();


	private:
		void startNewNotification();
		void runAnimation();
		void removeOldUi();
		void createUi();
		void updateWebserverStatusUi();
		
		void* screenHandle;
		void* prevScreenHandle;	
		void* menuXMLDescHdle;
		int	notifyUiIdBg;//the bg image uiid
		int notifyUiIdBusyIcon; //the webserver busy icon
		std::vector<int> notifyUiId;//the text lines uiid
		bool busy;
		int textPadding;
		std::clock_t animationStartTime; //when the animation started
		std::clock_t animationRestStartTime; //when the animation started

		float totalAnimationDuration;//how much the animation should take to fully run in one direction
		float animationRestTime; //how much wes should wait when we a re fully displayed
		int animationDirection;
		int propertyFinalValue;
		std::vector<std::string> messageLines;
		int propertyChangeNeeded;
		
};



class TGFCLIENT_API WebServer {

	public:
		//local data
		bool raceEndSent;
		int previousLaps;
		const char* username;
		const char* password;
		const char* url;

		//dynamic data retrieved with some request to the webserver
		int raceId;
		int userId;
		const char* sessionId;
		
		//configuration readers
		void readConfiguration();	
		int readUserConfig(int userId);
		
		//sync request
		int sendGenericRequest (std::string data, std::string& serverReply);

		//async requests
		int updateAsyncStatus();
		int addAsyncRequest(std::string const data);
		int addOrderedAsyncRequest(std::string const data);
		int pendingAsyncRequestId;
		std::vector<webRequest_t> orderedAsyncRequestQueque;	

		//specific requests
		int sendLogin (int userId);
		int sendLogin (const char* username, const char* password);
		int sendRaceStart (int user_skill, const char *track_id, char *car_id, int type, void *setup, int startposition, const char *sdversion);
		int sendRaceEnd (int race_id, int endposition);
		int sendLap (int race_id, double laptime, double fuel, int position, int wettness);
		
		//curl
		CURLM* multi_handle;
		int handle_count;
		std::string curlServerReply;
	
		//constructor
		WebServer();
	
		//destructor
		~WebServer();	
};
#endif //WEBSERVER


