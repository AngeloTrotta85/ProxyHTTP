/*
 * VideoClient.cpp
 *
 *  Created on: 20/apr/2015
 *      Author: angelo
 */

#include "ClientManager.h"

#include "InterfacesManager.h"
#include "StatManager.h"

static long long timevaldiff_usec(struct timeval *start, struct timeval *end) {
	return (end->tv_sec * 1000000 + end->tv_usec) - (start->tv_sec * 1000000 + start->tv_usec);
}

ClientManager::ClientManager() {
	new_sockfd_VideoClient = -1;
	sockfd_VideoClient = -1;
	sockfd_VideoServer = -1;

	byte_update = BLOCK_SIZE_STATS_BYTE;
	discard_MPEGDASH = false;
	dummy_pkt_stat = false;
}

ClientManager::~ClientManager() { }

void ClientManager::setByteStat(int byteS) {
	byte_update = byteS;
}

void ClientManager::setDiscardFlag(bool discard) {
	discard_MPEGDASH = discard;
}

void ClientManager::setDummyPktStat(bool dummy_stat) {
	dummy_pkt_stat = dummy_stat;
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

	debug_high("[PID: %d] - BEGIN ClientManager::getRequestFromClient\n", getpid());

	if (new_sockfd_VideoClient < 0) {
		debug_high("[PID: %d] - END (false1) ClientManager::getRequestFromClient\n", getpid());
		return false;
	}

	bzero(buffer, sizeof(buffer));

	int nrcv = recv(new_sockfd_VideoClient, buffer, sizeof(buffer), 0);

	if (nrcv < 0) {
		perror("Error recv from client socket");
		debug_high("[PID: %d] - END (false2) ClientManager::getRequestFromClient\n", getpid());
		return false;
	}
	else if (nrcv == 0) {
		debug_high("ClientManager::getRequestFromClient - Connection closed by client\n");
		debug_high("[PID: %d] - END (false3) ClientManager::getRequestFromClient\n", getpid());
		return false;
	}
	else {
		// parsing the GET
		debug_high("[PID: %d] - DEBUG (before load_req) ClientManager::getRequestFromClient\n", getpid());
		if (!rm.load_req(buffer, nrcv)) {
			// not a get and hence I have to get the destination address in another way
			// fill at least host_name and server_port (if present)
			// assume s is a connected socket

			socklen_t len;
			struct sockaddr_storage addr;
			char ipstr[INET6_ADDRSTRLEN];
			int port;

			debug_high("[PID: %d] - DEBUG (false load_req) ClientManager::getRequestFromClient\n", getpid());

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

			if (rm.isConnectReq()) {
				debug_high("Replying to CONNECT request\n");
				char *rep =
				"HTTP/1.1 403 Forbidden\n"
				"Date: Thu, 19 Feb 2009 12:27:04 GMT\n"
				"Server: Apache/2.2.3\n"
				"Last-Modified: Wed, 18 Jun 2003 16:05:58 GMT\n"
				"ETag: \"56d-9989200-1132c580\"\n"
				"Content-Type: text/html\n"
				"Content-Length: 15\n"
				"Accept-Ranges: bytes\n"
				"Connection: close\n"
				"\n"
				"sdfkjsdnbfkjbsf";

				//snprintf(rep, sizeof(rep), "");

				int s = send (new_sockfd_VideoClient, rep, strlen(rep), 0);
				debug_high("Succesfully sent %d bytes\n", s);
			}

			debug_high("[PID: %d] - END (false4) ClientManager::getRequestFromClient\n", getpid());
			return false;	//TODO non so come si fa... per ora prendo solo GET

			// boh! non so come/dove leggere la destinazione
		}
	}

	debug_high("[PID: %d] - END (true) ClientManager::getRequestFromClient\n", getpid());
	return true;
}

bool ClientManager::manageRequest(void) {
	debug_high("[PID: %d] - BEGIN ClientManager::manageRequest\n", getpid());

	if (!rm.isLoaded()) {
		debug_high("[PID: %d] - END (false1) ClientManager::sendGETtoDest\n", getpid());
		return false;
	}

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
		debug_medium("Managing transparently non MPEG-DASH M4S frame get (no stats update)\n");
		StatManager::getInstance().actual_stats.isMS4 = false;
	}

	if (rm.isMPEGDASHreq() || (!discard_MPEGDASH)) {

		// Sending the REQUEST to destination and managing the transfer
		if (sendGETtoDest(if_to_use_act, false)) {
			manageTransferFromDestToClient(if_to_use_act);

			close (sockfd_VideoServer);
			sockfd_VideoServer = -1;

		}
		else {
			debug_low("Error sending request to the destination\n");
		}
	}
	else {
		debug_low("Discarding non MPEGH-DASH requests due to the '-x' parameter\n");
	}

	debug_high("[PID: %d] - END (true) ClientManager::manageRequest\n", getpid());
	return true;
}

bool ClientManager::sendGETtoDest(struct sockaddr_in *if_to_bind, bool dummy_req) {
	struct sockaddr_in host_addr;
	struct sockaddr_in dummy_if_to_bind;

	debug_high("[PID: %d] - BEGIN ClientManager::sendGETtoDest\n", getpid());

	host_addr.sin_port = htons(rm.getServerPort());
	host_addr.sin_family=AF_INET;
	//host_addr.sin_addr.s_addr = rm.getServerAddr();
	if (strcmp(inet_ntoa(host_addr.sin_addr), "0.0.0.0") == 0){
		inet_aton("143.205.176.132", &(host_addr.sin_addr));
	}
	else {
		host_addr.sin_addr.s_addr = rm.getServerAddr();
	}

	
	if (if_to_bind){
		debug_high("Start sending the GET to the server using %s\n", inet_ntoa(if_to_bind->sin_addr));
	}
	else {
		debug_high("[PID: %d] - DEBUG (making dummy address) ClientManager::sendGETtoDest\n", getpid());
		dummy_if_to_bind.sin_family = AF_INET;
		//if_to_use.sin_port=htons(0);
		int portToUse = (rand()%1000) + 9000;
		dummy_if_to_bind.sin_port = htons(portToUse);
		dummy_if_to_bind.sin_addr.s_addr = INADDR_ANY;
		if_to_bind = &dummy_if_to_bind;
	}


	sockfd_VideoServer = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sockfd_VideoServer < 0) {
		perror("Error on creating socket for the server connection");
		debug_high("[PID: %d] - END (false1) ClientManager::sendGETtoDest\n", getpid());
		return false;
	}
	else {
		int risBind = 0;	// if I don't have to execute the bind '0' will be OK

		debug_high("[PID: %d] - DEBUG (before bind) ClientManager::sendGETtoDest\n", getpid());

		if (if_to_bind != NULL) {
			debug_high("[PID: %d] - DEBUG (really binding %s:%d) ClientManager::sendGETtoDest\n", getpid(), inet_ntoa(if_to_bind->sin_addr), (int)(ntohs(if_to_bind->sin_port)));
			risBind = bind (sockfd_VideoServer, (struct sockaddr *)if_to_bind, sizeof(struct sockaddr_in));
		}

		debug_high("[PID: %d] - DEBUG (after bind) ClientManager::sendGETtoDest\n", getpid());

		if (risBind == 0) {

			int x = fcntl(sockfd_VideoServer, F_GETFL,0);              // Get socket flags
			fcntl(sockfd_VideoServer, F_SETFL, sockfd_VideoServer | O_NONBLOCK);   // Add non-blocking flag

			int ris_conn;
			bool tryRead = true;
			time_t start_t, end_t;
			double diff_t;

			time(&start_t);
			while(tryRead) {
				tryRead = false;
				ris_conn = connect(sockfd_VideoServer, (struct sockaddr*)&host_addr, sizeof(struct sockaddr));
				if ((ris_conn < 0) && ((errno == ETIMEDOUT) || (errno == ENETUNREACH) || (errno == EINPROGRESS) || (errno == EALREADY))) {
					usleep(100000);

					time(&end_t);
					diff_t = difftime(end_t, start_t);

					if (diff_t < 3) {
						tryRead = true;
					}
				}
			}

			//if (connect(sockfd_VideoServer, (struct sockaddr*)&host_addr, sizeof(struct sockaddr)) < 0) {à
			if (ris_conn < 0) {
				perror("Error in connecting to remote server");
				debug_high("[PID: %d] - END (err connecting to %s:%d) ClientManager::sendGETtoDest\n", getpid(), inet_ntoa(host_addr.sin_addr), (int)(ntohs(host_addr.sin_port)));
				return false;
			}
			else {
				//debug_high("Sending the original req to the destination: \n******************************\n%s\n******************************\n",
				//		rm.getCopyOfGET());

				debug_high("[PID: %d] - DEBUG (connected) ClientManager::sendGETtoDest\n", getpid());

				int n_send;
				if (dummy_req)	n_send = send(sockfd_VideoServer, rm.getDummyGET(), strlen(rm.getDummyGET()), 0);
				else 			n_send = send(sockfd_VideoServer, rm.getCopyOfGET(), strlen(rm.getCopyOfGET()), 0);
				if (n_send < 0) {
					perror("Error writing to server socket");
					debug_high("[PID: %d] - END (false3) ClientManager::sendGETtoDest\n", getpid());
					return false;
				}

				debug_high("[PID: %d] - DEBUG (sent %d bytes) ClientManager::sendGETtoDest\n", getpid(), n_send);
			}
		}
		else {
			perror("Error binding");
			debug_high("[PID: %d] - END (false4) ClientManager::sendGETtoDest\n", getpid());
			return false;
		}
	}
	
	if ((if_to_bind) && (if_to_bind != &dummy_if_to_bind)) {
		debug_high("End sending the GET to the server using %s\n", inet_ntoa(if_to_bind->sin_addr));
	}
	else {
		debug_high("End sending the GET to the server\n");
	}

	//start STATS
	time(&StatManager::getInstance().actual_stats.start_request_time);
	gettimeofday(&(StatManager::getInstance().actual_stats.start_request_timeval), NULL);

	debug_high("[PID: %d] - END (true) ClientManager::sendGETtoDest\n", getpid());

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
		//n_recv = recv(sockfd_VideoServer, buffer, sizeof(buffer), 0);

		bool tryRead = true;
		time_t start_t, end_t;
		double diff_t;

		//debug_medium("[%d] ----- Start receiving the STATISTICAL packet\n", getpid());

		time(&start_t);
		while(tryRead) {
			tryRead = false;

			n_recv = recv(sockfd_VideoServer, buffer, sizeof(buffer), MSG_DONTWAIT);
			//debug_medium("[%d] ----- Received %d byte for the the STATISTICAL packet\n", getpid(), n_recv);
			if ((n_recv < 0) && ((errno == EAGAIN) || (errno == EWOULDBLOCK))) {
				usleep(100000);

				time(&end_t);
				diff_t = difftime(end_t, start_t);
				double interval_update = InterfacesManager::getInstance().timer_update;
				if (interval_update < 8) {
					interval_update = 8;
				}
				if (diff_t < interval_update) {
					tryRead = true;
				}

				//debug_medium("[%d] ----- Received %d byte for the the STATISTICAL packet\n", getpid(), n_recv);
			}
		}

		if (n_recv > 0) {

			n_tot_recv += n_recv;
			block_stat_recv += n_recv;

			if (block_stat_recv > byte_update) {
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

	if (n_tot_recv == block_stat_recv) {	// never made stats (packet size less then "byte_update")
		gettimeofday(&time_en, NULL);
		InterfacesManager::getInstance().updateInterfaceStats(if_used, block_stat_recv, timevaldiff_usec(&time_st, &time_en));
	}
}

void ClientManager::manageTransferFromDestToClient(struct sockaddr_in *if_used) {
	int n_recv = 0;
	int n_tot_recv = 0;
	int block_stat_recv = 0;
	struct timeval time_st, time_en;

	debug_high("[PID: %d] - BEGIN ClientManager::manageTransferFromDestToClient\n", getpid());

	gettimeofday(&time_st, NULL);

	debug_high("Receiving from server and sending to the client\n");

	do {
		memset(buffer, 0, sizeof(buffer));
		//n_recv = recv(sockfd_VideoServer, buffer, sizeof(buffer), 0);
		bool tryRead = true;
		time_t start_t, end_t;
		double diff_t;

		time(&start_t);
		while(tryRead) {
			tryRead = false;

			n_recv = recv(sockfd_VideoServer, buffer, sizeof(buffer), MSG_DONTWAIT);
			if ((n_recv < 0) && ((errno == EAGAIN) || (errno == EWOULDBLOCK))) {
				usleep(100000);

				time(&end_t);
				diff_t = difftime(end_t, start_t);
				//if (diff_t < InterfacesManager::getInstance().timer_update) {
				if (diff_t < 8) {
					tryRead = true;
				}
			}
		}

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

				debug_high("\r%d of %d  ", n_recv, n_tot_recv);

				StatManager::getInstance().actual_stats.reply_ok = true;

				if (block_stat_recv >= byte_update) {
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
			perror("manageTransferFromDestToClient: Error receiving from server while forwarding");
		}

	} while (n_recv > 0);

	StatManager::getInstance().actual_stats.frag_bytesize = n_tot_recv;
	time(&StatManager::getInstance().actual_stats.end_request_time);
	gettimeofday(&StatManager::getInstance().actual_stats.end_request_timeval, NULL);

	debug_high("\n");

	if (n_tot_recv == block_stat_recv) {	// never made stats (packet size less then "byte_update")
		gettimeofday(&time_en, NULL);
		InterfacesManager::getInstance().updateInterfaceStats(if_used, block_stat_recv, timevaldiff_usec(&time_st, &time_en));
	}

	StatManager::getInstance().makeStat();

	debug_high("[PID: %d] - END ClientManager::manageTransferFromDestToClient\n", getpid());
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

			//InterfacesManager::getInstance().blockStatIF(addr_in);

			// set random seed
			srand(getpid());

			debug_medium("%d - Sono un nuovo processo che deve aggiornare l'interfaccia: %s\n", getpid(), inet_ntoa(addr_in->sin_addr));

			// close the connection from the client (not needed for testing the connection)
			// now the client can go on...
			close (new_sockfd_VideoClient);
			new_sockfd_VideoClient = -1;

			// Sending the REQUEST to destination and managing the transfer
			if (sendGETtoDest(addr_in, true)) {
				manageTransferOnStatUpdate(addr_in);

				close (sockfd_VideoServer);
				sockfd_VideoServer = -1;

			}
			else {
				debug_low("Error sending request to the destination on STATS UPDATE\n");
			}

			// free the Interface and Stat memory
			InterfacesManager::getInstance().freeMemory();
			StatManager::getInstance().freeMemory();

			//InterfacesManager::getInstance().freeStatIF(addr_in);

			// the child process will terminate now
			_exit(EXIT_SUCCESS);
		}
	}
}
