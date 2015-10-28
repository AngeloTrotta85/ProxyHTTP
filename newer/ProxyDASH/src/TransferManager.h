#ifndef TRANSFERMANAGER_H_
#define TRANSFERMANAGER_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <iostream>
#include <vector>
#include <list>
#include <fstream>
#include "pugixml.hpp"
#include <chrono>

#include "RequestManager.h"
#include "Miscellaneous.h"


class TransferManager
{

public:
   TransferManager();
   virtual ~TransferManager(void);

   static void settingsManifestParams(char *mName,VideoInfo* videoInfo);
   static bool manageRequest(RequestManager rm,struct sockaddr_in *if_to_use,int socketfd_client, VideoInfo* videoInfo, int counter);
   static void forkAndUpdateStats(struct sockaddr_in *addr_in);
   static bool sendGETtoDest(RequestManager rm,int &sockfd_VideoServer, struct sockaddr_in *if_to_bind);
   static void manageTransferFromDestToClient(struct sockaddr_in *if_used, RequestManager rm, int sockfd_VideoServer, int socketfd_VideoClient, VideoInfo* videoInfo);
   static void manageTransferOnStatUpdate(struct sockaddr_in *if_used, int sockfd_VideoServer);
   static bool getVideoFrame(const char* GET, RequestManager rm, struct sockaddr_in *if_used, int &localServerSocket);
   static void sendToClientFromFile(int clientSocket, const char* file);
   static void forkAndDownloadFrame(int frameNumber);
   static void customGetRequest(int frameNumber,RequestManager rm, char *customGET);
   static void settingsManifestParams(char *mName,VideoInfo videoInfo);
   static void manageTransferFromDest(int localServerSocket, char* filename);

};

#endif /* TRANSFERMANAGER_H_ */
