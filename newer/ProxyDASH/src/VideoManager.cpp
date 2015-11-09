#include "VideoManager.h"

#include <pthread.h>
#include <cstdbool>
#include <string>

#include "ChoiceAlgorithms.h"
#include "InterfacesManager.h"
#include "StatManager.h"

inline bool exists (const std::string& name) {
	return ( access( name.c_str(), F_OK ) != -1 );
}

VideoManager::~VideoManager(){
	stop_thread = true;
	if(the_thread.joinable()) the_thread.join();
}

void VideoManager::start(){
	// This will start the thread. Notice move semantics!
	the_thread = std::thread(&VideoManager::startVideoManager,this);
}

bool VideoManager::isInit(){
	return is_init;
}

std::string VideoManager::parseName(const char *path){
	char tmp_video_name[32];
	char *ptr;
	char tmp_path[256];

	if (strncmp(path, "/ftp/datasets/DASHDataset2014/", 30) != 0) {
		return std::string("");
	}

	memset(tmp_path, 0, sizeof(tmp_path));
	strcpy(tmp_path, &path[30]);

	ptr = strtok (tmp_path, "/");
	sscanf(ptr, "%s", tmp_video_name);
	return (std::string(tmp_video_name));
}


void VideoManager::initVideoInfo(const char *path, int socket, char algo, int offset, char* quality) //, int fragNumber, struct in_addr clientIP, int clientPort)
{     
	this->algo = algo;
	this->offset = offset;
	this->quality = quality;
	done = false;
	is_init = true;
	pSocket = socket;
	videoName = parseName(path);
	if_to_use.resize(0);

	videoInfo.initClass(0, videoName);
	printf("THREAD:: init thread with video name: %s\n", videoName.c_str());

	pthread_mutex_init(&mutex, NULL);
	pthread_cond_init(&cond, NULL);
}

bool VideoManager::checkPacket(const char *path)//, struct in_addr clientIP, int clientPort){
{
	std::string videoNameL = parseName(path);

	//printf("THREAD::  check if is Thread name %s %s\n ", videoName.c_str(), videoNameL.c_str());
	if(!isLoadManifest)
		return false;
	if(videoName.compare(videoNameL) == 0)
		return true;
	return false;
}

void VideoManager::sendSignal(int socket, RequestManager rmt){
	pthread_mutex_lock(&mutex);
	new_sockfd_VideoClient = socket;
	rm = rmt;
	pthread_cond_signal(&cond);
	pthread_mutex_unlock(&mutex);
}

void VideoManager::startVideoManager()
{
	printf("THREAD::    Local name is: %s\n", (videoName.c_str()));
	printf("THREAD::    Treadh: %d\n", getpid());

	while(true){

		pthread_mutex_lock(&mutex);

		printf("THREAD::    waiting...\n");
		fflush(stdout);

		pthread_cond_wait(&cond, &mutex);


		printf("THREAD::  Aggiorno le intefaccie \n");
		fflush(stdout);

		if(!rm.isManifest() && !rm.isInit()){
			videoInfo.updateStatus();
			InterfacesManager::getInstance().chooseIFMain(if_main, if_to_use);
		}



		printf("THREAD::  Best interface is %s thr: \n", inet_ntoa(if_main.sin_addr));
		fflush(stdout);

		printf("THREAD::  Gestisco la richiesta \n");
		fflush(stdout);
		forkAndManage();

		//close socket used from the child
		close (new_sockfd_VideoClient);

		printf("THREAD::  Cerco se ci sono interfaccie libere da usare\n");
		fflush(stdout);

		if(!rm.isManifest() && !rm.isInit())
			checkUsedInterfaces();

		pthread_mutex_unlock(&mutex);

	}
}

int VideoManager::forkAndManage(){


	fflush(stdout);
	if(rm.isManifest() && !isLoadManifest){ //Manage manifest transfer
		struct sockaddr_in sockaddr_temp;
		sockaddr_temp.sin_family=AF_INET;
		sockaddr_temp.sin_port=htons(0);
		sockaddr_temp.sin_addr.s_addr = if_main.sin_addr.s_addr;
		//TODO:
		if (!TransferManager::manageRequest(rm, &sockaddr_temp, new_sockfd_VideoClient, &videoInfo, 0)) {
			perror("Error managing request from client");
		}
		videoInfo.print();
		isLoadManifest = true;
	}else{               //Manage normal transfer Segment
		int number = 0;
		if(!rm.isInit())
			number = videoInfo.parsePath(rm.getPathName());
		videoInfo.setRequested(number);
		counter++;
		bool isDownloaded = false;
		if(number > 0)
			isDownloaded = videoInfo.frameArray[number].isDownloaded;
		int pid = fork();

		if (pid < 0) {
			perror("ERROR on fork");
		}
		else if (pid > 0) {
			// parent process
			// do nothing
		}
		else {

			close(pSocket);
			// set random seed
			srand(getpid());


			//printf("CHILD %d::  Best interface is %s\n", counter, inet_ntoa(if_main.sin_addr));
			printf("CHILD %d::  Il pacchetto %d e' gia stato scaricato? %s\n", counter, number, isDownloaded ? "true" : "false");

			if(isDownloaded && !rm.isInit()){  //Manage already downloaded Segment

				//std::string *fileLocalName = videoInfo.getSegmentFile(number);
				TransferManager::sendToClientFromFile(new_sockfd_VideoClient, videoInfo.frameArray[number].file->c_str());

			}else if (!TransferManager::manageRequest(rm, &if_main, new_sockfd_VideoClient, NULL,  counter)) {
				perror("Error managing request from client");
			}

			// close the connection from the client
			close (new_sockfd_VideoClient);
			new_sockfd_VideoClient = -1;

			// free the Interface and Stat memory
			InterfacesManager::getInstance().freeMemory();
			StatManager::getInstance().freeMemory();
			videoInfo.freeMemory();

			printf("CHILD %d:: Child finished!%d\n", counter, number);
			// the child process will terminate now
			_exit(EXIT_SUCCESS);
		}
		return pid;
	}
	return -1;
}

void VideoManager::loadManifest(int frameNumber){

}

void VideoManager::checkUsedInterfaces(){

	std::list<struct sockaddr_in>::iterator it_use;

	for (it_use = if_to_use.begin(); it_use != if_to_use.end(); it_use++){
		if(it_use->sin_addr.s_addr != if_main.sin_addr.s_addr){
			printf("THREAD::  	Interface free: %s \n",  inet_ntoa(it_use->sin_addr));
			fflush(stdout);
			struct sockaddr_in *act_sock = &(*it_use);
			useInterface(act_sock);
			test_counter++;
		}
	}

}
void VideoManager::checkInterfaceStatus(){

	InterfacesManager::getInstance().chooseIF(if_main, if_to_use);

}

void VideoManager::useInterface(struct sockaddr_in *addr_in){

	InterfacesManager::getInstance().setUsed(addr_in->sin_addr.s_addr);
	customFrameDownload(addr_in);

}

void VideoManager::customFrameDownload(struct sockaddr_in *addr_in){

	char filename[256];
	int frameNumber = selectFrame(InterfacesManager::getInstance().getExpectedThr(addr_in->sin_addr.s_addr));
	generateRandomFileName(frameNumber, filename);

	//printf("THREAD:: Trovata interfaccia libera: %s scelto numero random: %d \n", inet_ntoa(addr_in->sin_addr), frameNumber);
	int pid = fork();

	if (pid < 0) {
		perror("ERROR on fork");
	}
	else if (pid > 0) {
		videoInfo.frameArray[frameNumber].isDownloading = true;
		videoInfo.frameArray[frameNumber].pid = pid;
		videoInfo.frameArray[frameNumber].file = new std::string(filename);
		videoInfo.frameArray[frameNumber].s_addr = addr_in->sin_addr.s_addr;

		//printf("THREAD:: Custom packet %d download. Pid: %d Filename: %s \n", frameNumber, videoInfo.frameArray[frameNumber].pid, videoInfo.frameArray[frameNumber].file.c_str() );
	}
	else {
		close(pSocket);
		close (new_sockfd_VideoClient);
		new_sockfd_VideoClient = -1;

		srand(getpid());
		printf("CUSTOM CHILD:: Gestisto il download custom del segmento %d\n", frameNumber);

		//Change GETRequest
		char ptrGET[4096];
		videoInfo.customGetRequest(frameNumber, rm, ptrGET, quality);

		//printf("CUSTOM CHILD %d:: COPY OF GET \n%s\n", frameNumber, ptrGET);
		//fflush(stdout);

		int socketl = -1;
		if(TransferManager::getVideoFrame(ptrGET, rm, addr_in, socketl)){
			//printf("CUSTOM CHILD :: Get inviata con sucesso, ricevo su file \n");
			TransferManager::manageTransferFromDest(socketl, filename);
			//printf("CUSTOM CHILD :: Finito di ricevere \n");
		}
		close(socketl);
		socketl = -1;


		// free the Interface and Stat memory
		InterfacesManager::getInstance().freeMemory();
		StatManager::getInstance().freeMemory();
		videoInfo.freeMemory();

		printf("CUSTOM CHILD:: Finito il download del segmento %d\n", frameNumber);
		fflush(stdout);
		// the child process will terminate now
		_exit(EXIT_SUCCESS);
	}

	return ;
}

void VideoManager::generateRandomFileName(int n, char* name){
	static const int len = 15;
	static const char alphanum[] =
			"0123456789"
			"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
			"abcdefghijklmnopqrstuvwxyz";
	static const char path[] = "tempf/";
	std::string test;
	do{
		memcpy((void*) name, (void*) path, 6);
		for (int i = 6; i < 14; ++i) {
			name[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
		}
		name[14] = '.';
		name[15] = 't';
		name[16] = 'x';
		name[17] = 't';
		name[18] = 0;
		test = std::string(name);
	} while(exists(test));
}

int VideoManager::selectFrame(long thr){

	int to_ret = 0;

	switch (algo) {
		case 'r':
			to_ret = ChoiceAlgorithms::random(videoInfo.getLastRequest(), offset, videoInfo);
			break;
		case 'f':
			to_ret = ChoiceAlgorithms::fixed(videoInfo.getLastRequest(), offset, videoInfo);
			break;
		case 'c':
			to_ret = ChoiceAlgorithms::caba(videoInfo, thr);
			break;
		default:
			to_ret = ChoiceAlgorithms::random(videoInfo.getLastRequest(), offset, videoInfo);

	}

	return to_ret;

}
