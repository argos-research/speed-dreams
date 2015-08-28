#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

#include "socket.h"

int createSocket(int port)
{
	printf("INFO: Create socket\n");
    struct sockaddr_in serv_addr;
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
    	printf("ERROR opening socket\n");
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    {
    	printf("ERROR on binding socket\n");
    }
    return sockfd;
}

int waitForConnection(int sockfd)
{
	socklen_t clilen;
	sockaddr_in cli_addr;

	printf("INFO: wait for tcp connection\n");
	listen(sockfd,5);
    clilen = sizeof(cli_addr);
    int newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
    if (newsockfd < 0)
    {
        printf("ERROR on accepting connection\n");
    }
    return newsockfd;
}

void closeSocket(int sockfd, int newsockfd)
{
	printf("INFO: close sockets\n");
	close(newsockfd);
	close(sockfd);
}

int socketRead(int sockfd, void* buf, int len)
{
	return recv(sockfd, buf, len, 0);
}

int socketWrite(int sockfd, const void* buf, int len)
{
	return send(sockfd, buf, len, 0);
}