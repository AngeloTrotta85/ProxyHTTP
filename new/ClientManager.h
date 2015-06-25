/*
 * VideoClient.h
 *
 *  Created on: 20/apr/2015
 *      Author: angelo
 */

#ifndef CLIENTMANAGER_H_
#define CLIENTMANAGER_H_

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

#include "RequestManager.h"
#include "Miscellaneous.h"

#define BIG_BUFF_LEN 16384

class ClientManager {
public:
	ClientManager();
	virtual ~ClientManager();

	bool startListeningForClient(int port);
	int acceptConnectionFromClient(void);

	int forkAndManageClient(void);

private:

	bool getRequestFromClient(void);
	bool manageRequest(void);
	void forkAndUpdateStats(struct sockaddr_in *addr_in);
	bool sendGETtoDest(struct sockaddr_in *if_to_bind);
	void manageTransferFromDestToClient(struct sockaddr_in *if_used);
	void manageTransferOnStatUpdate(struct sockaddr_in *if_used);

private:
	int sockfd_VideoClient;
	int new_sockfd_VideoClient;

	int sockfd_VideoServer;

	char buffer[16384];

	RequestManager rm;
};

#endif /* CLIENTMANAGER_H_ */
