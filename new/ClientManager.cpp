/*
 * VideoClient.cpp
 *
 *  Created on: 20/apr/2015
 *      Author: angelo
 */

#include "ClientManager.h"

#include "InterfacesManager.h"
#include "StatManager.h"

static long int timevaldiff_usec(struct timeval *start, struct timeval *end) {
	return (end->tv_sec * 1000000 + end->tv_usec) - (start->tv_sec * 1000000 + start->tv_usec);
}

ClientManager::ClientManager() {
	// TODO Auto-generated constructor stub
	new_sockfd_VideoClient = -1;
	sockfd_VideoClient = -1;
	sockfd_VideoServer = -1;
}

ClientManager::~ClientManager() {
	// TODO Auto-generated destructor stub
}

bool ClientManager::startListeningForClient(int port) {
	struct sockaddr_in serv_addr;

	bzero((char*)&serv_addr,sizeof(serv_addr));

	serv_addr.sin_family=AF_INET;
	serv_addr.sin_port=htons(port);
	serv_addr.sin_addr.s_addr=INADDR_ANY;

	sockfd_VideoClient = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sockfd_VideoClient < 0) {
		perror("Problem in initializing socket");
		return false;
	}

	if (bind(sockfd_VideoClient,(struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
		perror("Error on binding");
		return false;
	}

	if (listen(sockfd_VideoClient, 50) < 0) {
		perror("Error on listen");
		return false;
	}

	return true;
}

int ClientManager::acceptConnectionFromClient(void) {
	struct sockaddr_in cli_addr;
	unsigned int clilen;

	bzero((char*)&cli_addr, sizeof(cli_addr));
	clilen = sizeof(cli_addr);

	new_sockfd_VideoClient = accept(sockfd_VideoClient, (struct sockaddr*)&cli_addr, &clilen);
	if (new_sockfd_VideoClient < 0) {
		perror("ERROR on accept");
	}

	return new_sockfd_VideoClient;
}

int ClientManager::forkAndManageClient(void) {
	// managing the GET by the child process
	int pid = fork();
	if (pid < 0) {
		perror("ERROR on fork");
	}
	else if (pid > 0) {
		// parent process
		// do nothing
	}
	else {

		// set random seed
		srand(getpid());

		// close immediately the client socket used for accepting new connections from the parent process
		close (sockfd_VideoClient);

		if (getRequestFromClient()) {
			if (!manageRequest()) {
				perror("Error managing request from client");
			}
			//else {
			//	// TODO
			//}
		}
		else {
			perror("Error getting request from client");
		}

		// close the connection from the client
		close (new_sockfd_VideoClient);
		new_sockfd_VideoClient = -1;

		// free the Interface and Stat memory
		InterfacesManager::getInstance().freeMemory();
		StatManager::getInstance().freeMemory();

		// the child process will terminate now
		_exit(EXIT_SUCCESS);
	}

	return pid;
}

bool ClientManager::getRequestFromClient(void) {

	if (new_sockfd_VideoClient < 0) return false;

	bzero(buffer, sizeof(buffer));

	int nrcv = recv(new_sockfd_VideoClient, buffer, sizeof(buffer), 0);

	if (nrcv < 0) {
		perror("Error recv from client socket");
		return false;
	}
	else if (nrcv == 0) {
		printf("Connection closed by client\n");
		return false;
	}
	else {
		// parsing the GET
		if (!rm.load_req(buffer, nrcv)) {
			// not a get and hence I have to get the destination address in another way
			// fill at least host_name and server_port (if present)
			// assume s is a connected socket

			socklen_t len;
			struct sockaddr_storage addr;
			char ipstr[INET6_ADDRSTRLEN];
			int port;

			len = sizeof addr;
			getpeername(new_sockfd_VideoClient, (struct sockaddr*)&addr, &len);

			// deal with IPv4 only:
			if (addr.ss_family == AF_INET) {
				struct sockaddr_in *s = (struct sockaddr_in *)&addr;

				port = ntohs(s->sin_port);
				inet_ntop(AF_INET, &s->sin_addr, ipstr, sizeof ipstr);

				printf("Peer IP address: %s\n", ipstr);
				printf("Peer port      : %d\n", port);

				rm.setServerPort(port);
				rm.setServerAddr(s->sin_addr.s_addr);
			}

			return false;	//TODO non so come si fa... per ora prendo solo GET

			// boh! non so come/dove leggere la destinazione
		}
	}

	return true;
}

bool ClientManager::manageRequest(void) {
	if (!rm.isLoaded()) return false;

	struct sockaddr_in if_to_use;
	struct sockaddr_in *if_to_use_act = NULL;		// pointer used to check later if the bind should be done or not

	//choose the interface to use and, in some cases, update the unused interfaces
	//if ((rm.isGET()) && (rm.isMPEGDASHreq())) {
	if ((rm.isGET()) && (rm.isMPEGDASH_M4S())) {
		std::list<struct sockaddr_in> if_to_update;
		std::list<struct sockaddr_in>::iterator it_update;

		InterfacesManager::getInstance().chooseIF(if_to_use, if_to_update);

		if_to_use_act = &if_to_use;

		StatManager::getInstance().actual_stats.isMS4 = true;
		StatManager::getInstance().actual_stats.choosed_interface.s_addr = if_to_use.sin_addr.s_addr;
		StatManager::getInstance().actual_stats.reply_ok = false;
		StatManager::getInstance().fillFragmentField(rm.getPathName());

		//before managing request check if some interfaces must be updated
		for (it_update = if_to_update.begin(); it_update != if_to_update.end(); it_update++){
			struct sockaddr_in *act_sock = &(*it_update);

			forkAndUpdateStats(act_sock);
			// updates are done by other processes
			// only the parent process should exit from this function...

		}
	}
	else {
		//tratto in maniera trasparente questa connessione tcp
		debug_medium("Is NOT an MPEG-DASH get\n");
		StatManager::getInstance().actual_stats.isMS4 = false;
	}

	// Sending the REQUEST to destination and managing the transfer
	if (sendGETtoDest(if_to_use_act)) {
		manageTransferFromDestToClient(if_to_use_act);

		close (sockfd_VideoServer);
		sockfd_VideoServer = -1;

	}
	else {
		debug_low("Error sending request to the destination\n");
	}

	return true;
}

bool ClientManager::sendGETtoDest(struct sockaddr_in *if_to_bind) {
	struct sockaddr_in host_addr;

	host_addr.sin_port = htons(rm.getServerPort());
	host_addr.sin_family=AF_INET;
	host_addr.sin_addr.s_addr = rm.getServerAddr();
	
	debug_high("Start sending the GET to the server using\n");

	sockfd_VideoServer = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sockfd_VideoServer < 0) {
		perror("Error on creating socket for the server connection");
		return false;
	}
	else {
		int risBind = 0;	// if I don't have to execute the bind '0' will be OK

		if (if_to_bind != NULL) {
			risBind = bind (sockfd_VideoServer, (struct sockaddr *)if_to_bind, sizeof(struct sockaddr_in));
		}

		if (risBind == 0) {

			if (connect(sockfd_VideoServer, (struct sockaddr*)&host_addr, sizeof(struct sockaddr)) < 0) {
				perror("Error in connecting to remote server");
				return false;
			}
			else {
				debug_high("Sending the original req to the destination: \n******************************\n%s\n******************************\n",
						rm.getCopyOfGET());

				int n_send = send(sockfd_VideoServer, rm.getCopyOfGET(), strlen(rm.getCopyOfGET()), 0);
				if (n_send < 0) {
					perror("Error writing to server socket");
					return false;
				}
			}
		}
		else {
			perror("Error binding");
			return false;
		}
	}

	//start STATS
	time(&StatManager::getInstance().actual_stats.start_request_time);
	gettimeofday(&StatManager::getInstance().actual_stats.start_request_timeval, NULL);

	return true;
}

void ClientManager::manageTransferOnStatUpdate(struct sockaddr_in *if_used) {
	int n_recv = 0;
	int n_tot_recv = 0;
	int block_stat_recv = 0;
	struct timeval time_st, time_en;

	gettimeofday(&time_st, NULL);

	debug_high("Receiving from client ONLY for statistics\n");

	do {
		memset(buffer, 0, sizeof(buffer));
		n_recv = recv(sockfd_VideoServer, buffer, sizeof(buffer), 0);

		if (n_recv > 0) {

			n_tot_recv += n_recv;
			block_stat_recv += n_recv;

			if (block_stat_recv > BLOCK_SIZE_STATS_BYTE) {
				gettimeofday(&time_en, NULL);

				InterfacesManager::getInstance().updateInterfaceStats(if_used, block_stat_recv, timevaldiff_usec(&time_st, &time_en));

				block_stat_recv = 0;
				gettimeofday(&time_st, NULL);
			}

		}
		else if (n_recv == 0) {
			//debug_high("connection closed by the server\n");
		}
		else {
			perror("Error receiving from client while forwarding");
		}

	} while (n_recv > 0);

	if (n_tot_recv == block_stat_recv) {	// never made stats
		gettimeofday(&time_en, NULL);
		InterfacesManager::getInstance().updateInterfaceStats(if_used, block_stat_recv, timevaldiff_usec(&time_st, &time_en));
	}
}

void ClientManager::manageTransferFromDestToClient(struct sockaddr_in *if_used) {
	int n_recv = 0;
	int n_tot_recv = 0;
	int block_stat_recv = 0;
	struct timeval time_st, time_en;

	gettimeofday(&time_st, NULL);

	debug_high("Receiving from client and sending to the server\n");

	do {
		memset(buffer, 0, sizeof(buffer));
		n_recv = recv(sockfd_VideoServer, buffer, sizeof(buffer), 0);

		if (n_recv > 0) {
			int n_sent = send(new_sockfd_VideoClient, buffer, n_recv, 0);

			if(n_sent < 0) {
				perror("Error sending to client the server reply");
			}
			else if(n_sent == 0) {
				debug_low("Connection closed by the client while sending back the message!?!?!\n");
			}
			else {

				n_tot_recv += n_recv;
				block_stat_recv += n_recv;

				debug_medium("\r%d of %d  ", n_recv, n_tot_recv);

				StatManager::getInstance().actual_stats.reply_ok = true;

				if (block_stat_recv >= BLOCK_SIZE_STATS_BYTE) {
					gettimeofday(&time_en, NULL);

					InterfacesManager::getInstance().updateInterfaceStats(if_used, block_stat_recv, timevaldiff_usec(&time_st, &time_en));

					block_stat_recv = 0;
					gettimeofday(&time_st, NULL);
				}

			}

		}
		else if (n_recv == 0) {
			debug_high("connection closed by the server\n");
		}
		else {
			perror("Error receiving from server while forwarding");
		}

	} while (n_recv > 0);

	StatManager::getInstance().actual_stats.frag_bytesize = n_tot_recv;
	time(&StatManager::getInstance().actual_stats.end_request_time);
	gettimeofday(&StatManager::getInstance().actual_stats.end_request_timeval, NULL);

	debug_medium("\n");

	if (n_tot_recv == block_stat_recv) {	// never made stats
		gettimeofday(&time_en, NULL);
		InterfacesManager::getInstance().updateInterfaceStats(if_used, block_stat_recv, timevaldiff_usec(&time_st, &time_en));
	}

	StatManager::getInstance().makeStat();
}

// only the parent process should exit from this function...
void ClientManager::forkAndUpdateStats(struct sockaddr_in *addr_in) {

	if (!(InterfacesManager::getInstance().isAlreadyInTest(addr_in))) {

		int pid = fork();

		if (pid < 0) {
			perror("ERROR on fork");
		}
		else if (pid > 0) {
			// parent process
			// do nothing
		}
		else {

			// set random seed
			srand(getpid());

			// Sending the REQUEST to destination and managing the transfer
			if (sendGETtoDest(addr_in)) {
				manageTransferFromDestToClient(addr_in);

				close (sockfd_VideoServer);
				sockfd_VideoServer = -1;

			}
			else {
				debug_low("Error sending request to the destination on STATS UPDATE\n");
			}

			// free the Interface and Stat memory
			InterfacesManager::getInstance().freeMemory();
			StatManager::getInstance().freeMemory();

			// the child process will terminate now
			_exit(EXIT_SUCCESS);
		}
	}
}
