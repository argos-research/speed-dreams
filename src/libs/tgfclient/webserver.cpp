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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sstream>
#include <playerpref.h>
#include <tgf.h>
#include "tgfclient.h"
#include "webserver.h"

//string splitting utils
std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, elems);
    return elems;
}

//to string (from c++11)
template <typename T>
std::string to_string(T value)
{
	std::ostringstream os ;
	os << value ;
	return os.str() ;
}

static size_t WriteStringCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

void replaceAll(std::string& str, const std::string& from, const std::string& to) {
    if(from.empty())
        return;
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
    }
}
//unique id generator
int uniqueId=1;
int getUniqueId(){
	return uniqueId++;
}

//

const int WEBSERVER_IDLE = 0;
const int WEBSERVER_SENDING = 1;
const int WEBSERVER_RECEIVING = 2;
int webserverState = WEBSERVER_IDLE;




NotificationManager::NotificationManager(){

	this->busy = false;
	this->notifyUiIdBg = -1;//the bg image ui id
	this->notifyUiIdBusyIcon = -1;//the bg image ui id

	this->animationRestTime = 4 ; //how much wes should wait when we a re fully displayed
	this->totalAnimationDuration = 0.3 ;//how much the animation should take to fully run in one direction
	this->animationLastExecTime = std::clock(); //the current time

}
NotificationManager::~NotificationManager(){

}
void NotificationManager::updateStatus(){

	//get the current screen
	this->screenHandle = GfuiGetScreen();

	//get the ui descriptor
	this->menuXMLDescHdle = GfuiMenuLoad("notifications.xml");
	
	//if we are doing nothing and we have some message to display: let's do it
	if(this->busy==false && !this->msglist.empty()){

		this->startNewNotification();

	}

	//if we are running an animation
	if(this->busy==true){

		this->runAnimation();

	}
	
	//update webserver status icon
	this->updateWebserverStatusUi();
	
	//remember the current screen for the next run
	this->prevScreenHandle = this->screenHandle;

}

void NotificationManager::startNewNotification(){

	//we are running an animation
	this->busy=true;
	
	//set the animation direction
	this->animationDirection=1;
	
	//retrieve the message to display
	std::string newText = this->msglist.front().c_str();
	
	//divide the current message in lines
	this->messageLines = split(this->msglist.front().c_str(), '\n');
	
	//reset the start time(s)
	this->animationStartTime = this->animationLastExecTime = std::clock();
	this->animationRestStartTime = 0;
	
	//reset the start property
	int propertyCurrentValue=(int)GfParmGetNum(this->menuXMLDescHdle, "dynamic controls/slide", "x", "null", 0);
	this->propertyChangeNeeded=(int)GfParmGetNum(this->menuXMLDescHdle, "dynamic controls/slide", "width", "null", 0);
	this->propertyFinalValue = propertyCurrentValue + this->propertyChangeNeeded;

	//padding between the text and the bg image
	this->textPadding = propertyCurrentValue - (int)GfParmGetNum(this->menuXMLDescHdle, "dynamic controls/slidebg", "x", "null", 0);

	//start
	this->animationDirection = 1;
	this->runAnimation();

}
void NotificationManager::runAnimation(){
	
	//read the initial state of the UI
	int propertyCurrentValue = (int)GfParmGetNum(this->menuXMLDescHdle, "dynamic controls/slide", "x", "null", 0);
	// change needed from current status of the animation to the end
	int remainingChangeNeeded = (this->propertyFinalValue - propertyCurrentValue);

	//log current time
	std::clock_t currentTime = std::clock();

	//CASE 1	
	//we still need to apply some change to reach the final value
	if(remainingChangeNeeded != 0){

		//how much time is we are running the animation
		float animationRunningTime = (currentTime - this->animationStartTime) / (float) CLOCKS_PER_SEC;

		//how much time is passed from the last run
		float animationTimeFromLastStep = (currentTime - this->animationLastExecTime) / (float) CLOCKS_PER_SEC;
		//time remaining for the animation
		float animationTimeRemaining = this->totalAnimationDuration - animationRunningTime;

		//
		//int propertyStepChange = remainingChangeNeeded / animationTimeRemaining * animationTimeFromLastStep;
		//int propertyStepChange = remainingChangeNeeded / animationTimeRemaining;
		//if we have not arhieving 30fps slow down the animation
		if(animationTimeFromLastStep > 0,033333333){

			animationTimeFromLastStep = animationTimeFromLastStep;

		}
		int propertyStepChange = this->propertyChangeNeeded / this->totalAnimationDuration * this->animationDirection * animationTimeFromLastStep;

		// if the change is too little we round it up to 1 unit at least
		if((propertyStepChange * this->animationDirection) < 1 ){

			propertyStepChange = 1 * this->animationDirection;	

		}

		//new value for the property
		int propertyNewValue = propertyCurrentValue + propertyStepChange;

		//it he new value with the change applied is greater that the final result we want we correct it to be equal to the final result
		if (propertyNewValue * this->animationDirection  > propertyFinalValue * this->animationDirection ){

			propertyNewValue = propertyFinalValue;

		}
		
		//apply the new values
		GfParmSetNum(this->menuXMLDescHdle, "dynamic controls/slide", "x", "null", propertyNewValue);
		GfParmSetNum(this->menuXMLDescHdle, "dynamic controls/slidebg", "x", "null", propertyNewValue - this->textPadding);

		//remember the time we ran the last(this) animation frame
		this->animationLastExecTime = currentTime;
		
		/*
		GfLogInfo("###############################\n");
		GfLogInfo("StartTime: %d \n",this->animationStartTime);
		GfLogInfo("CurrentTime: %d \n",currentTime);
		GfLogInfo("RunningTime: %f \n ",(currentTime - this->animationStartTime) / (float) CLOCKS_PER_SEC);
		GfLogInfo("RunningTime: %f \n ",(currentTime - this->animationStartTime));
		GfLogInfo("RunningTime: %f \n ",(float) CLOCKS_PER_SEC);

		GfLogInfo("\n ");
		GfLogInfo("AnimationDuration: %f \n ",this->totalAnimationDuration);
		GfLogInfo("TimeRemaining: %f \n ",animationTimeRemaining);
		GfLogInfo("\n ");
		GfLogInfo("FinalValue: %i \n ",this->propertyFinalValue);
		GfLogInfo("CurrentValue: %i \n ",propertyCurrentValue);
		GfLogInfo("Change Needed: %i \n ",remainingChangeNeeded);
		GfLogInfo("StepChange: %i \n ",propertyStepChange);
		GfLogInfo("\n ");
		GfLogInfo("Direction: %i \n ",this->animationDirection);
		*/
		
		this->removeOldUi();
		this->createUi();

	}
	
	
	
	//CASE 2
	// no change needed while running the runOutAnimation
	if(remainingChangeNeeded == 0 && this->animationDirection == -1){

		//delette this message from the queque
		this->msglist.erase (this->msglist.begin());

		//we are no longer busy
		this->busy=false;

	}
	
	
	//CASE 3	
	// no change needed while running the runInAnimation: we have ended the runInAnimation
	if(remainingChangeNeeded == 0 && this->animationDirection == 1){
		if(this->animationRestStartTime==0){

			//we are just done runnig the runInAnimation
			//log the time we start waiting while fully displayed
			this->animationRestStartTime = std::clock();

		}else{

			//if rest time has expired: start the runOutAnimation
			if(((currentTime - this->animationRestStartTime) / (float) CLOCKS_PER_SEC) > this->animationRestTime){

				//change the animation direction
				this->animationDirection = -1;

				//reset the animation start time
				this->animationStartTime = this->animationLastExecTime = std::clock(); //when the animation started

				//read property info
				this->propertyChangeNeeded= (int)GfParmGetNum(this->menuXMLDescHdle, "dynamic controls/slide", "width", "null", 0);
				this->propertyFinalValue = propertyCurrentValue - this->propertyChangeNeeded;	

			}

		}

	}

}

void NotificationManager::removeOldUi(){

	//if there is a prev screen 
	if( GfuiScreenIsActive(this->prevScreenHandle) ){

		//if there is some prev ui around hide it
		if(this->notifyUiIdBg > 0){

			GfuiVisibilitySet(this->prevScreenHandle, this->notifyUiIdBg, GFUI_INVISIBLE);

		}
		
		//iterate trougth ui and set them invisible
		for (int i = 0; i < notifyUiId.size(); i++) {

			GfuiVisibilitySet(this->prevScreenHandle, this->notifyUiId[i], GFUI_INVISIBLE);

		 }

	}

	//delete the prev ui's
	this->notifyUiId.clear();
	this->notifyUiIdBg=-1;
}

void NotificationManager::createUi(){

	//create the new UI	
	this->notifyUiIdBg = GfuiMenuCreateStaticImageControl(this->screenHandle, this->menuXMLDescHdle, "slidebg");			
	GfuiVisibilitySet(this->screenHandle, this->notifyUiIdBg, GFUI_VISIBLE);
	
	//get first line vertical position
	int ypos=(int)GfParmGetNum(this->menuXMLDescHdle, "dynamic controls/slide", "y", "null", 0);
	int yposmod= ypos;

	//iterate trougth lines
	for (int i = 0; i < this->messageLines.size(); i++) {

		int uiId;
		uiId= GfuiMenuCreateLabelControl(this->screenHandle, this->menuXMLDescHdle, "slide");
		
		//change the vertical position
		int yposmod = ypos - (i+1)*(10);
		GfParmSetNum(this->menuXMLDescHdle, "dynamic controls/slide", "y", "null", yposmod);

		GfuiLabelSetText(this->screenHandle, uiId, this->messageLines[i].c_str());
		GfuiVisibilitySet(this->screenHandle, uiId, GFUI_VISIBLE);
		this->notifyUiId.push_back(uiId);

	 }

	//reset ypos
	GfParmSetNum(this->menuXMLDescHdle, "dynamic controls/slide", "y", "null", ypos);
}
void NotificationManager::updateWebserverStatusUi(){
	//todo: this was causing some random crash
	//if there is some prev ui around hide it
	if(this->notifyUiIdBusyIcon > 0 && this->screenHandle == this->prevScreenHandle){
		
		GfuiVisibilitySet(this->prevScreenHandle, this->notifyUiIdBusyIcon, GFUI_INVISIBLE);

	}	
	
	if(this->screenHandle > 0){
		//if webserver is not IDLE display busy icon (sending or receiving)
		if(webserverState != WEBSERVER_IDLE){
			
			std::string webServerIcon = "busyicon";
			webServerIcon.append(to_string(webserverState));

			this->notifyUiIdBusyIcon = GfuiMenuCreateStaticImageControl(this->screenHandle, this->menuXMLDescHdle, webServerIcon.c_str());			
			GfuiVisibilitySet(this->screenHandle, this->notifyUiIdBusyIcon, GFUI_VISIBLE);
			
		}
		
	}

}
//initialize the notification manager
NotificationManager notifications;






WebServer::WebServer(){
	
	//set some default
	this->raceId = -1;
	this->userId = -1;
	this->previousLaps = -1;
	this->raceEndSent = false;

	//initialize some curl var
	this->multi_handle = curl_multi_init();
	this->handle_count = 0;

	//initialize the configuration
	this->readConfiguration();

}
WebServer::~WebServer(){
	
	//cleanup curl
	curl_multi_cleanup(this->multi_handle);
	
}
int WebServer::updateAsyncStatus(){
	
	//if we have some ordered async request pending
	if( this->pendingAsyncRequestId == 0 && !this->orderedAsyncRequestQueque.empty() ){
		webRequest_t nextRequest;
		nextRequest = this->orderedAsyncRequestQueque.front();

		//replace the {{tags}} with actual data received from previous requests replies
		replaceAll(nextRequest.data, "{{race_id}}", to_string(this->raceId));
		replaceAll(nextRequest.data, "{{user_id}}", to_string(this->userId));	
		
		GfLogInfo("WebServer: Adding AsyncRequest from orderedAsyncRequestQueque with id: %i\n", nextRequest.id);
		this->pendingAsyncRequestId = nextRequest.id;
		this->addAsyncRequest(nextRequest.data);
	}
	

	//perform the pending requests
	curl_multi_perform(this->multi_handle, &this->handle_count);

	if( this->handle_count > 0){
		GfLogInfo("WebServer: Number of async request waiting for a reply from the server: %i\n", this->handle_count);
		//display some UI to the user to inform him we are waiting a reply from the server
		webserverState=WEBSERVER_RECEIVING;
	}else{
		webserverState=WEBSERVER_IDLE;
	}
	
	CURLMsg *msg;
	CURL *eh=NULL;
	CURLcode return_code;
	int http_status_code;
	const char *szUrl;
	
	while ((msg = curl_multi_info_read(this->multi_handle, &this->handle_count))) {
		if (msg->msg == CURLMSG_DONE) {
			eh = msg->easy_handle;

			return_code = msg->data.result;
			if(return_code!=CURLE_OK) {
				fprintf(stderr, "CURL error code: %d\n", msg->data.result);
				
				//something went wrong. anyway we are no more busy
				webserverState=WEBSERVER_IDLE;

				continue;
			}

			// Get HTTP status code
			http_status_code=0;
			szUrl = NULL;

			curl_easy_getinfo(eh, CURLINFO_RESPONSE_CODE, &http_status_code);

			curl_easy_getinfo(eh, CURLINFO_EFFECTIVE_URL, &szUrl);

			//the server replyed
			if(http_status_code==200) {
				GfLogInfo("WebServer: successfull reply from the server from url: %s\n", szUrl);

				GfLogInfo("WebServer: The reply from the server is:\n%s\n", this->curlServerReply.c_str());
				//manage server replyes...
				
				//read the xml reply of the server
				void *xmlReply;
				xmlReply = GfParmReadBuf(const_cast<char*>(this->curlServerReply.c_str()));

				if(GfParmExistsSection(xmlReply, "content")){
					int requestId = GfParmGetNum(xmlReply, "content", "request_id", "null",0);
					if( requestId == this->pendingAsyncRequestId ){
						//reset the pending request id
						this->pendingAsyncRequestId=0;
						// remove this request from the queque... so we can continue with the next
						GfLogInfo("WebServer: Removing successfull AsyncRequest from the orderedAsyncRequestQueque with id: %i\n", this->orderedAsyncRequestQueque.front().id);		
						this->orderedAsyncRequestQueque.erase (this->orderedAsyncRequestQueque.begin());
					}
				}


				//the server want we display something?
				if(GfParmExistsSection(xmlReply, "content/reply/messages")){
					int msgsCount = GfParmGetNum(xmlReply, "content/reply/messages", "number", "null",0);
					if( msgsCount > 0 ){
						for( int dispatchedMsgs = 0; dispatchedMsgs < msgsCount; dispatchedMsgs = dispatchedMsgs + 1 )
						   {
							std::string msgTag = "message";
							msgTag.append(to_string(dispatchedMsgs));
							GfLogInfo("WebServer: Adding messagge to be displayed to the NotificationQueque:\n%s\n", msgTag.c_str());
							
							notifications.msglist.push_back(GfParmGetStr(xmlReply, "content/reply/messages", msgTag.c_str(), "null"));
						   }
					}
				}

				//race reply
				//store the webServer assigned race id
				if(GfParmExistsSection(xmlReply, "content/reply/races")){
					if(GfParmGetNum(xmlReply, "content/reply/races", "id", "null", 0) != 0){
						this->raceId = (int)GfParmGetNum(xmlReply, "content/reply/races", "id", "null", 0);
						GfLogInfo("WebServer: Assigned raceId by the server is: %i\n", this->raceId);
					}
				}
						
				//login reply
				//store the webServer assigned race id				
				if(GfParmExistsSection(xmlReply, "content/reply/login")){
					if(GfParmGetNum(xmlReply, "content/reply/login", "id", "null",0) != 0){
						//store the webServer session and id assigned
						this->sessionId = GfParmGetStr(xmlReply, "content/reply/login", "sessionid", "null");
						this->userId = GfParmGetNum(xmlReply, "content/reply/login", "id", "null",0);
						GfLogInfo("WebServer: Successfull Login as userId: %i\n", this->userId);
					}else{
						GfLogInfo("WebServer: Login Failed: Wrong username or password.\n");
						notifications.msglist.push_back("WebServer: Login Failed: Wrong username or password.");
						return 1;				
					}
					GfLogInfo("WebServer: Assigned session id is: %s\n", this->sessionId);
				}				

				//empty the string
				this->curlServerReply.clear();
			} else {
				fprintf(stderr, "GET of %s returned http status code %d\n", szUrl, http_status_code);
			}
			curl_multi_remove_handle(this->multi_handle, eh);
			curl_easy_cleanup(eh);
			/* then cleanup the formpost chain */ 
			//curl_formfree(formpost);
		}
		else {
			fprintf(stderr, "error: after curl_multi_info_read(), CURLMsg=%d\n", msg->msg);
		}
	} 	

	return 0;
}

int WebServer::addAsyncRequest(std::string const data){
	GfLogInfo("WebServer: Performing ASYNC request:\n%s\n", data.c_str());

	CURL* curl = NULL;
	struct curl_httppost *formpost=NULL;
	struct curl_httppost *lastptr=NULL;

	curl = curl_easy_init();
	if(curl) {
		
		//set url to be called
		curl_easy_setopt(curl, CURLOPT_URL, this->url);
		
		// send all data to this function
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteStringCallback);
		
		//pass our std::string to the WriteStringCallback functions so it can write into it
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &this->curlServerReply);
		
		// some servers don't like requests that are made without a user-agent
		// field, so we provide one 
		curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
		
		//prepare the form-post to be sent to the server
		curl_formadd(&formpost,
				   &lastptr,
				   CURLFORM_COPYNAME, "data", //the field name where the data will be stored
				   CURLFORM_COPYCONTENTS, data.c_str(), //the actual data
				   CURLFORM_END);
				   
		//inform curl to send the form-post
		curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);	

	}
	
	//add the request to the queque
	curl_multi_add_handle(this->multi_handle, curl);

	//pending request
	webserverState=WEBSERVER_SENDING;
	
	return 0;	
}
int WebServer::addOrderedAsyncRequest(std::string data){
	//prepare the request object
	webRequest_t request;
	
	request.id = getUniqueId();
	request.data = data;
	
	replaceAll(request.data, "{{request_id}}", to_string(request.id));

	this->orderedAsyncRequestQueque.push_back(request);
	return 0;	
}

int WebServer::sendGenericRequest (std::string data, std::string& serverReply){
	CURL *curl;
	CURLcode res;

	GfLogInfo("WebServer: Performing SYNC request:\n%s\n", data.c_str()); 

	webserverState=WEBSERVER_SENDING;

	//insert "data=" before the actual data
	data.insert(0,"data=");
	const char *postthis=data.c_str();
 
	curl_global_init(CURL_GLOBAL_ALL);
	curl = curl_easy_init();
	if(curl) {
		
		curl_easy_setopt(curl, CURLOPT_URL, this->url);
		
		// send all data to this function
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteStringCallback);
		
		//pass our std::string to the WriteStringCallback functions so it can write into it
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &this->curlServerReply);
		
		// some servers don't like requests that are made without a user-agent
		// field, so we provide one 
		curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
	
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postthis);
		
		// if we don't provide POSTFIELDSIZE, libcurl will strlen() by
		// itself 
		curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)strlen(postthis));
		
		// Perform the request, res will get the return code 
		res = curl_easy_perform(curl);

		// Check for errors 
		if(res != CURLE_OK) {
			notifications.msglist.push_back("Failed to connect to the WebServer!");
			GfLogInfo("WebServer: Unable to perform SYNC request some error occured:\n", data.c_str()); 
			fprintf(stderr, "curl_easy_perform() failed: %s\n",
					curl_easy_strerror(res));
		}
		else {
			//
			// Now, our this->curlServerReply
			// contains the remote file (aka serverReply).
			//
			// Do something nice with it!
			// 

			GfLogInfo("WebServer: Receiving data from the WebServer:\n%s\n", this->curlServerReply.c_str());
			
			webserverState=WEBSERVER_RECEIVING;

			serverReply = this->curlServerReply;

			//empty the string
			this->curlServerReply.clear();			
		}
	 
		// always cleanup 
		curl_easy_cleanup(curl);
	 
		// we're done with libcurl, so clean it up 
		curl_global_cleanup();
	}
	return 0;
}

void WebServer::readConfiguration (){

	void *configHandle;
	char configFileUrl[256];
/*	
	//get the preferencies file location
	sprintf(configFileUrl, "%s%s", GfLocalDir(), "config/webserver.xml");

	//read the preferencies file
	configHandle = GfParmReadFile(configFileUrl, GFPARM_RMODE_REREAD);
	
	//get webServer url from the config
	this->url = GfParmGetStr(configHandle, "WebServer Settings", "url","val");
*/

	this->url ="http://www.madbad.altervista.org/speed-dreams/webserver.php";
//	this->url ="http://localhost/speed-dreams/webserver.php";

	//GfLogInfo("WebServer - webserver url is: %s\n", this->url);
}

int WebServer::readUserConfig (int userId){
	void *prHandle;
	char prefFileUrl[256];
	char xmlPath[256];

	// find the xmlPath to our specific user in the preferencies xml file
	sprintf(xmlPath, "%s%i", "Preferences/Drivers/", userId);

	//get the preferencies file location
	sprintf(prefFileUrl, "%s%s", GfLocalDir(), HM_PREF_FILE);

	//read the preferencies file
	prHandle = GfParmReadFile(prefFileUrl, GFPARM_RMODE_REREAD);

	//get webServer user id for current user
	this->username = GfParmGetStr(prHandle, xmlPath, "WebServerUsername","val");

	//get webServer user password for current user
	this->password = GfParmGetStr(prHandle, xmlPath, "WebServerPassword","val");

	return 0;
}

int WebServer::sendLogin (int userId){
	std::string serverReply;

	//read username and password and save it in as webserver properties 
	this->readUserConfig(userId);
	this->sendLogin(this->username, this->password);
/*	
	std::string username="username";
	std::string password="password";
	
	//if the user has not setup the webserver login info abort the login
	if(username==this->username && password==this->password){
		GfLogInfo("WebServer: Send of login info aborted (the user is not correctly setup in this client).\n");
		return 1;	
	}

	//prepare the string to send
	std::string dataToSend ("");
	dataToSend.append(	"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
						"<content>"
						"<request_id>{{request_id}}</request_id>"
						"<request>"			
						"<login>"
						"<username>{{username}}</username>"
						"<password>{{password}}</password>"
						"</login>"
						"</request>"
						"</content>");
						
	//replace the {{tags}} with the respecting values
	replaceAll(dataToSend, "{{username}}", this->username);
	replaceAll(dataToSend, "{{password}}", this->password);

	this->addOrderedAsyncRequest(dataToSend);
*/
	return 0;
}
int WebServer::sendLogin (const char* username, const char* password){
	std::string serverReply;

	//prepare the string to send
	std::string dataToSend ("");
	dataToSend.append(	"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
						"<content>"
						"<request_id>{{request_id}}</request_id>"
						"<request>"			
						"<login>"
						"<username>{{username}}</username>"
						"<password>{{password}}</password>"
						"</login>"
						"</request>"
						"</content>");
						
	//replace the {{tags}} with the respecting values
	replaceAll(dataToSend, "{{username}}", username);
	replaceAll(dataToSend, "{{password}}", password);

	this->addOrderedAsyncRequest(dataToSend);

	return 0;
}
int WebServer::sendLap (int race_id, double laptime, double fuel, int position, int wettness){
/*
	//Do some sanity-checks befor proceding... If something is wrong do nothing
	//are we logged in?
	if(this->sessionId=='\0'){
		GfLogInfo("Comunication of lap info aborted. No session ID assigned (we are not logged in)");			
		return 1;
	}
	
	//Was the raceStart comunication successfull?
	if(this->raceId > -1){
		GfLogInfo("Comunication of lap info aborted. This race was not being tracked.");			
		return 1;
	}
*/
	//prepare the string to send
	std::string dataToSend ("");
	dataToSend.append(	"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
						"<content>"
						"<request_id>{{request_id}}</request_id>"
						"<request>"
						"<laps>"
						"<race_id>{{race_id}}</race_id>"
						"<laptime>{{laptime}}</laptime>"
						"<fuel>{{fuel}}</fuel>"
						"<position>{{position}}</position>"
						"<wettness>{{wettness}}</wettness>"						
						"</laps>"
						"</request>"
						"</content>");
						
	//replace the {{tags}} with the respecting values						
	//The following tags will be replaced later because some other request must be done before: {{race_id}}
	replaceAll(dataToSend, "{{laptime}}", to_string(laptime));
	replaceAll(dataToSend, "{{fuel}}", to_string(fuel));
	replaceAll(dataToSend, "{{position}}", to_string(position));
	replaceAll(dataToSend, "{{wettness}}", to_string(wettness));	
	
	this->addOrderedAsyncRequest(dataToSend);

	return 0;
}
int WebServer::sendRaceStart (int user_skill, const char *track_id, char *car_id, int type, void *setup, int startposition, const char *sdversion){
	std::string serverReply;
	std::string mysetup;
	std::string dataToSend;
	this->raceEndSent = false;
	this->previousLaps = -1;

	//Do some sanity-checks befor proceding... If something is wrong do nothing
	//are we logged in?
/*
	if(this->sessionId=='\0'){
		GfLogInfo("Comunication of race start to the webserver aborted. No session ID assigned (we are not logged in)");			
		return 1;
	}
*/
	
	//Sanity-checks passed we continue
	//read the setup
	GfParmWriteString(setup, mysetup);

	//prepare the string to send
	dataToSend.append(	"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
						"<content>"
						"<request_id>{{request_id}}</request_id>"
						"<request>"
						"<races>"
						"<user_id>{{user_id}}</user_id>"
						"<user_skill>{{user_skill}}</user_skill>"
						"<track_id>{{track_id}}</track_id>"
						"<car_id>{{car_id}}</car_id>"
						"<type>{{type}}</type>"
						"<setup><![CDATA[{{setup}}]]></setup>"
						"<startposition>{{startposition}}</startposition>"
						"<sdversion>{{sdversion}}</sdversion>"
						"</races>"
						"</request>"
						"</content>");
						
	//replace the {{tags}} with the respecting values						
	//The following tags will be replaced later because some other request must be done before: {{user_id}}			
	replaceAll(dataToSend, "{{user_skill}}", to_string(user_skill));						
	replaceAll(dataToSend, "{{track_id}}", to_string(track_id));						
	replaceAll(dataToSend, "{{car_id}}", to_string(car_id));						
	replaceAll(dataToSend, "{{type}}", to_string(type));						
	replaceAll(dataToSend, "{{setup}}", mysetup);						
	replaceAll(dataToSend, "{{startposition}}", to_string(startposition));						
	replaceAll(dataToSend, "{{sdversion}}", to_string(sdversion));						
						
	this->addOrderedAsyncRequest(dataToSend);

	return 0;
}
int WebServer::sendRaceEnd (int race_id, int endposition){
	std::string serverReply;

	//Do some sanity-checks befor proceding... If something is wrong do nothing
	//are we logged in?
/*
	if(this->sessionId=='\0'){
		GfLogInfo("Comunication of race end to the webserver aborted. No session ID assigned (we are not logged in)");			
		return 1;
	}
	
	//Was the raceStart comunication successfull?
	if(this->raceId > -1){
		GfLogInfo("Comunication of race end to the webserver aborted. This race was not being tracked.");			
		return 1;
	}
*/
	//Sanity-checks passed we continue
	//prepare the string to send
	std::string dataToSend ("");
	dataToSend.append(	"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
						"<content>"
						"<request_id>{{request_id}}</request_id>"
						"<request>"
						"<races>"
						"<id>{{race_id}}</id>"
						"<endposition>{{endposition}}</endposition>"
						"</races>"
						"</request>"
						"</content>");
						
	//replace the {{tags}} with the respecting values
	//The following tags will be replaced later because some other request must be done before: {{race_id}}
	replaceAll(dataToSend, "{{endposition}}", to_string(endposition));

	this->addOrderedAsyncRequest(dataToSend);

	this->raceEndSent = true;
	//restore default race id
	//this->raceId = -1;

	return 0;
}

//initialize the web server
TGFCLIENT_API WebServer webServer;
#endif //WEBSERVER


