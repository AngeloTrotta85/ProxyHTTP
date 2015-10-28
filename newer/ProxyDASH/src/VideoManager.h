#ifndef VIDEOMANAGER_H_
#define VIDEOMANAGER_H_

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
#include <curl/curl.h>
#include <thread>
#include <atomic>
#include <chrono>

#include "RequestManager.h"
#include "Miscellaneous.h"
#include "VideoInfo.h"
#include "TransferManager.h"


class VideoManager
{
public:
   std::atomic<bool> done;
   pthread_mutex_t mutex;
   pthread_cond_t cond;

private:
   int counter = 0;
   int new_sockfd_VideoClient = -1;
   int sockfd_VideoServer = -1;
   int pSocket = -1; //Main socket, close it.
   bool is_init = false; //Is Thread init?
   bool stop_thread = false; // Super simple thread stopping.
   bool isLoadManifest = false; //Manifest check
   char algo;
   char* quality = NULL;
   int offset;
   int test_counter = 0;

   struct sockaddr_in if_main; //main interface
   //std::list<struct sockaddr_in> it_used;
   std::list<struct sockaddr_in> if_to_use;
   std::string videoName;
   RequestManager rm;
   VideoInfo videoInfo;

   std::thread the_thread;

public:
   void startVideoManager();
   std::string parseName(const char *path);
   void initVideoInfo(const char *path, int socket, char algo, int offset, char* quality);
   bool checkPacket(const char *path);
   void sendSignal(int socket, RequestManager rmt);
   void start(void);
   bool isInit(void);
   int forkAndManage(void);
   void loadManifest(int frameNumber);
   void checkInterfaceStatus(void);
   void forkAndUpdateStats(struct sockaddr_in *addr_in) ;

   VideoManager(){}
   virtual ~VideoManager(void);

private:
   void useInterface(struct sockaddr_in *addr_in);
   void customFrameDownload(struct sockaddr_in *addr_in);
   int selectFrame(void);
   void generateRandomFileName(int n, char* name);
   void checkUsedInterfaces(void);


/*   bool manageRequest(void);
   void forkAndUpdateStats(struct sockaddr_in *addr_in);
   bool sendGETtoDest(struct sockaddr_in *if_to_bind);
   void manageTransferFromDestToClient(struct sockaddr_in *if_used);
   void manageTransferOnStatUpdate(struct sockaddr_in *if_used);
   int forkAndManage(void);
   void manageTransferFromDest(int localServerSocket);
   void getVideoFrame(const char* GET);
   void sendToClientFromStruct(int clientSocket);
   void forkAndDownloadFrame(int frameNumber);
   const char* customGetRequest(int frameNumber,const char* getR);*/

};

#endif /* VIDEOMANAGER_H_ */
