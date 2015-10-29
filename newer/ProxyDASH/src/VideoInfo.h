#ifndef VIDEOINFO_H_
#define VIDEOINFO_H_

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
#include <chrono>

#include "RequestManager.h"
#include "Miscellaneous.h"

using namespace std;

class VideoInfo
{
public:

private:
   string videoName;
   int fragmentNumber = 0;
   int clientPort = 0;
   struct in_addr clientIP;
   string url1;
   string url2;
   string url3;
   int lastRequest = 0;
   struct VideoFrame {
      bool isDownloading;
      bool isDownloaded;
      bool isRequested;
      string *file;
      int pid;
      unsigned long s_addr;
      VideoFrame(){
    	  isDownloaded = false;
    	  isDownloading = false;
    	  isRequested = false;
    	  pid = -1;
    	  file = NULL;
    	  s_addr = 0;
    	  //memset((void*) s_addr,0,sizeof(unsigned long));
      };
   };
public:
   vector<VideoFrame> frameArray;

public:
   void customErase(std::list<struct sockaddr_in> &if_to_update, int toCheck);
   void customGetRequest(int frameNumber,RequestManager rm, char *customGET, char* qual);

   void init(string duration,string durationSegment,string media,string timescale,string initSegment);
   void updateStatus();
   bool checkSegment(int number);
   int parsePath(const char* path);
   void initVector(int n);
   void print(void);
   void setRequested(int n);
   void freeMemory(void);

   std::string getVideoName(){
      return videoName;
   }

   int getLastRequest(){
	   return lastRequest;
   }

   int getSegmentNumber(){
	   return fragmentNumber;
   }

   string getSegmentFile(int frameNumber){
     return *frameArray[frameNumber].file;
   }

   void initClass(int size, string name) {
      videoName = name;
   }

   VideoInfo() {}
   virtual ~VideoInfo(void);
};

#endif /* VIDEOINFO_H_ */
