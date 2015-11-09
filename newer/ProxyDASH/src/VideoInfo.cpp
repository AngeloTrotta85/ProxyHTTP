#include "VideoInfo.h"
#include "InterfacesManager.h"

VideoInfo::~VideoInfo(){
	printf("VIDEOINFO::::: DISTRUTTORE \n");
}
void VideoInfo::setRequested(int n){
	frameArray[n].isRequested = true;
	lastRequest = n;
}
void VideoInfo::freeMemory(void) {
	frameArray.clear();
	frameArray.resize(0);
}

void VideoInfo::customErase(std::list<struct sockaddr_in> &if_to_update, int toCheck){
	std::list<struct sockaddr_in>::iterator i = if_to_update.begin();
	while (i != if_to_update.end())
	{

	    if (frameArray[toCheck].s_addr == (*i).sin_addr.s_addr)
	    {
	        if_to_update.erase(i++);  // alternatively, i = items.erase(i);
	    }
	    else
	    {
	        ++i;
	    }
	}
}

void VideoInfo::updateStatus(){
	//printf("THREAD:: VideoInfo: controllo se ci sono Segmenti finiti %d \n", fragmentNumber);
	fflush(stdout);
	for (int i = 1; i < fragmentNumber-1; ++i) {
		//printf("VideoInfo:: Update status ciclo: %d \n", i);
	   if(frameArray[i].isDownloading && frameArray[i].pid != -1){
			int status;
			pid_t result = waitpid(frameArray[i].pid, &status, WNOHANG);
			if (result == 0) {
				// Child still alive, Do nothing
			} else {
				frameArray[i].isDownloading = false;
				frameArray[i].isDownloaded = true;
				frameArray[i].pid = -1;
				InterfacesManager::getInstance().setFree(frameArray[i].s_addr);
				//customErase(if_to_update, i);
				//printf("THREAD:: Il processo ha finito il segmento %d %s \n", i, frameArray[i].isDownloaded? "true" : "false");
				//fflush(stdout);
			}
	   } else if(frameArray[i].isRequested){
		   if( lastRequest > i + 4 && frameArray[i].file !=  NULL && frameArray[i].isDownloaded){
			   if( std::remove( frameArray[i].file->c_str() ) != 0 )
			      perror( "Error deleting file" );
			   delete frameArray[i].file;
			   frameArray[i].file = NULL;
		   }
	   }
	}
}
bool VideoInfo::checkSegment(int number){
	return frameArray[number].isDownloaded;
}

void VideoInfo::customGetRequest(int frameNumber,RequestManager rm, char *customGET, char* qual){

   char text[4096];
   strcpy(text, rm.getCopyOfGET());
   char c[15];
   sprintf(c, "%d", frameNumber);

   std::string final;
   std::string str = std::string(text);

   size_t index = 0;
   /* Locate the substring to replace. */
   if(qual != NULL)
	   printf("CUSTOM CHILD::  Custom get whit null quality \n");
   if(qual != NULL){
	   index = str.find(".m4s", index);
	   if (index == std::string::npos)
		   return;
	   std::string s2 = str.substr(index);
	   index = 0;
	   index = str.find(url1, index);
	   if (index == std::string::npos)
	   	   return;
	   std::string s1 = str.substr(0,(index+ url1.size()));
	   std::string quals = std::string(qual);
	   final = s1 + quals + url2 + c + s2;
   }else {
	   index = str.find(".m4s", index);
	   if (index == std::string::npos)
		   return;
	   std::string s2 = str.substr(index);
	   index = 0;
	   index = str.find(url2, index);
	   if (index == std::string::npos)
		   return;
	   std::string s1 = str.substr(0,(index+ url2.size()));
	   final = s1 + c + s2;
   }

   strcpy(customGET, final.c_str());
}

int VideoInfo::parsePath(const char* path){

	string tmp_path = string(path);

	int pos1 = tmp_path.find(url1.c_str());
	int pos2 = tmp_path.find(url2.c_str());

	int pos3 = tmp_path.find(url3.c_str());
	if(pos1 > 0 && pos2 > 0 && pos3>0){
		/*save last bps*/
		string bpsString = tmp_path.substr(url1.size()+pos1,pos2 - (url1.size()+pos1));
		last_bps = atoi(bpsString.c_str());

		string numberString = tmp_path.substr(url2.size()+pos2, (url2.size()+pos2) - pos3);
		int number = atoi(numberString.c_str());
		printf("Thread:: Parser path segment number %d at %sbps \n",number,bpsString.c_str());

		return number;
	}

	return 0;
}
void VideoInfo::init(string duration,string durationSegment,string media,string timescale,string initSegment,  pugi::xml_node manifest){

	//Parse duration in time
	int pt_pos = duration.find("PT");
	int h_pos = duration.find("H");
	int m_pos = duration.find("M");
	int s_pos = duration.find(".");

	string mString = duration.substr(h_pos+1, m_pos-4);
	string sString = std::string( duration.begin() + m_pos+1, duration.end() - 4);
	int h = atoi(duration.substr(pt_pos+2, h_pos-1).c_str()) *3600;
	int m = (atoi(mString.c_str()) * 60) +h;
	int s = atoi(sString.c_str()) + m ;
	int timescalen = atoi(timescale.c_str());
	int durationSegmentn = atoi(durationSegment.c_str());

	int segmentNumber = s / (durationSegmentn/timescalen);

	segmentDuration = durationSegmentn/timescalen;
	int pos1 = media.find("$Bandwidth$");
	int pos2 = media.find("$Number$");
	string mediaLocal = string(media.c_str());

	string url11 = mediaLocal.substr(0, pos1);
	string url22 =  mediaLocal.substr(pos1+11, pos2-(pos1+11));
	string url33 =  mediaLocal.substr(pos2 + 8);
	url1 = url11;
	url2 = url22;
	url3 = url33;

	//print();
	//Insert for to init vector
	initVector(segmentNumber);
	initQualityVector(manifest);

	printf("THREAD::  Init VideoInfo Class %s, %s, %s, %s \n", duration.c_str(),durationSegment.c_str(), media.c_str(), timescale.c_str());

}

void VideoInfo::initQualityVector( pugi::xml_node manifest){


	qualityArray.clear();
	for (pugi::xml_node quality: manifest.children("Representation"))
	{
	    qualityArray.push_back(atol(quality.attribute("bandwidth").value()));
	}

}

void VideoInfo::print(){
	printf("THREAD::: Video base information %s::%s::%s\n"
		"			fragment number: %d\n", url1.c_str(), url2.c_str(), url3.c_str(), fragmentNumber);
	fflush(stdout);
}
void VideoInfo::initVector(int n){
	fragmentNumber = n + 2;
	frameArray.resize(n+1);
	for (int var = 0; var < n; ++var) {
//		VideoFrame v = VideoFrame();
//		printf("THREAD::  Inserisco nel vettore l'elemento %s \n", v.isDownloaded? "true" : "false");
//		fflush(stdout);
//		printf("THREAD::  Inserisco nel vettore l'elemento 2 %s \n", frameArray[var].isDownloaded? "true" : "false");
//		fflush(stdout);
		//frameArray.push_back(v);
	}
}

