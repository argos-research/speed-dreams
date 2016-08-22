#ifndef SOCKET_H
#define SOCKET_H

int createSocket(int port);

bool makeSocketNonBlocking(int sockfd);

int waitForConnection(int sockfd);

void closeSocket(int sockfd, int newsockfd);

int socketRead(int sockfd, void* buf, int len);

int socketWrite(int sockfd, const void* buf, int len);

#endif