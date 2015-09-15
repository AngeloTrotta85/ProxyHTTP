/*
 * RequestManager.h
 *
 *  Created on: 20/apr/2015
 *      Author: angelo
 */

#ifndef REQUESTMANAGER_H_
#define REQUESTMANAGER_H_

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
#include <map>
#include <list>

#include "Miscellaneous.h"

class RequestManager {
public:
	enum MPEG_DASH_TYPE {
		MPEGDASH_NO_TYPE,
		MPEGDASH_MANIFEST,
		MPEGDASH_INIT,
		MPEGDASH_FRAME
	};

public:
	RequestManager();
	virtual ~RequestManager();

	bool load_req(char *str_req, int size_str);

	bool isLoaded(void);
	bool isConnectReq(void);
	bool isGET(void);
	bool isMPEGDASHreq(void);
	bool isMPEGDASH_M4S(void);

	const char* getHostName() const {
		return host_name;
	}

	const char* getPathName() const {
		return path_name;
	}

	int getServerPort() const {
		return server_port;
	}

	const char* getCopyOfGET() const {
		return buff_req;
	}

	in_addr_t getServerAddr() const {
		return server_addr;
	}

	void setServerAddr(in_addr_t serverAddr) {
		server_addr = serverAddr;
	}

	void setServerPort(int serverPort) {
		server_port = serverPort;
	}

private:
	void init(void);

private:
	char buff_req[4096];

	std::map <std::string, std::string> req_fields;

	// important field
	char host_name[64];
	char path_name[256];
	int server_port;
	in_addr_t server_addr;

	bool loaded;
	bool isget;
	bool isConnect;
	MPEG_DASH_TYPE mpeg_dash;
};

#endif /* REQUESTMANAGER_H_ */
