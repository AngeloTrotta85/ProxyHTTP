//============================================================================
// Name        : ProxyDASH.cpp
// Author      : Angelo Trotta
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

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

#include <sys/shm.h>        /* shmat(), IPC_RMID        */
#include <semaphore.h>      /* sem_open(), sem_destroy(), sem_wait().. */
#include <fcntl.h>          /* O_CREAT, O_EXEC          */

#include "InterfacesManager.h"
#include "ClientManager.h"
#include "StatManager.h"

#define INTERFACE_UPDATE_INTERVAL_SEC	3

using namespace std;

void handle_sigchld(int sig) {
	while (waitpid((pid_t)(-1), 0, WNOHANG) > 0) {}
}

void handle_sigInt(int s){
	printf("Caught signal %d\n",s);
	exit(1);
}

void usage(char* progName)
{
  cout << progName << " [options]" << endl <<
      "Options:" << endl <<
      "-h         Print this help" << endl <<
      "-i         Interfaces to exclude (separated by ,)" << endl <<
      "-l         Log file path-name" << endl <<
      "[-r]       Random choice of out-interface" << endl <<
      "[-b]       Byte to update interfaces statistics (default 250000 byte)" << endl <<
      "[-t]       Timeout to update unused interfaces in seconds (default 10s)" << endl <<
      "[-u]       Execute updating on unused interfaces ['yes(default)' or 'no']" << endl <<
      "[-x]       Discard non MPEG-DASH requests" << endl <<
      "-p         The port number to listen" << endl;
}

int main(int argc,char* argv[]) {
	char *log_file = NULL;
	std::list<std::string> interface2exclude;
	int listening_port = 0;
	int char_opt;
	int timeUpdate = TIME_STAT_UPDATE;
	int byteUpdate = BLOCK_SIZE_STATS_BYTE;
	bool statupdate = true;
	bool randomC = false;
	bool ignore_nonDASH = false;
	ClientManager cm;

	/****************************************************************/
	/* CHECK PARAMETERS                                             */
	/****************************************************************/
	opterr = 0;

	while ((char_opt = getopt (argc, argv, "xrhp:i:l:b:u:t:")) != -1) {
		char tmpstr[64];
		char *pch;

		switch (char_opt) {
		case 'h':
			usage(argv[0]);
			return EXIT_SUCCESS;

		case 'r':
			randomC = true;
			break;

		case 'x':
			ignore_nonDASH = true;
			break;

		case 'p':
			listening_port = atoi(optarg);
			break;

		case 'b':
			byteUpdate = atoi(optarg);
			break;

		case 't':
			timeUpdate = atoi(optarg);
			break;

		case 'u':
			if (!strncmp(optarg, "yes", 3)) {
				statupdate = true;
			}
			else if (!strncmp(optarg, "no", 2)) {
				statupdate = false;
			}
			else {
				usage(argv[0]);
				return EXIT_FAILURE;
			}
			break;

		case 'i':
			strncpy(tmpstr, optarg, sizeof(tmpstr));
			pch = strtok (tmpstr, ",");
			while (pch != NULL)
			{
				interface2exclude.push_back(pch);
				pch = strtok (NULL, ",");
			}
			break;

		case 'l':
			log_file = optarg;
			break;

		case '?':
			if ((optopt == 'p') || (optopt == 'i') || (optopt == 'l') || (optopt == 'u') || (optopt == 't') || (optopt == 'b')) {
				fprintf(stderr, "Option -%c requires an argument.\n", optopt);
			}
			else if (isprint (optopt)) {
				fprintf (stderr, "Unknown option `-%c'.\n", optopt);
			}
			else {
				fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
			}
			usage(argv[0]);
			return EXIT_FAILURE;

		default:
			abort ();
		}
	}

	if (optind < argc) {
		for (int idx = optind; idx < argc; idx++) {
			fprintf (stderr, "Non-option argument %s\n", argv[idx]);
		}
		usage(argv[0]);
		return EXIT_FAILURE;
	}

	if (log_file == NULL) {
		fprintf(stderr, "Insert log file name\n");
		usage(argv[0]);
		return EXIT_FAILURE;
	}
	if (listening_port == 0) {
		fprintf(stderr, "Insert listening port\n");
		usage(argv[0]);
		return EXIT_FAILURE;
	}

	/*printf ("port = %hu, log = %s\n", listening_port, log_file);
	for (std::list<std::string>::iterator it_str = interface2exclude.begin(); it_str != interface2exclude.end(); it_str++ ) {
		printf ("interface %s\n", it_str->c_str());
	}*/

	/****************************************************************/
	/* STARTING PROXY                                               */
	/****************************************************************/
	time_t lastInterfaceUpdate, now;

	cout << "Start PROXY!!!" << endl;

	// register to signal to prevent zombie child
	struct sigaction sa;
	sa.sa_handler = &handle_sigchld;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
	if (sigaction(SIGCHLD, &sa, 0) == -1) {
		perror("sigaction error");
		exit(EXIT_FAILURE);
	}

	// register to signal catch the kill signal (not yet used)
	struct sigaction sigIntHandler;
	sigIntHandler.sa_handler = &handle_sigInt;
	sigemptyset(&sigIntHandler.sa_mask);
	sigIntHandler.sa_flags = 0;
	if (sigaction(SIGINT, &sigIntHandler, NULL) == -1) {
		perror("sigaction error");
		exit(EXIT_FAILURE);
	}

	// init the statModule
	StatManager::getInstance().setFileName(log_file);

	// init the interfaces
	InterfacesManager::getInstance().setUpdateFlag(statupdate);
	InterfacesManager::getInstance().setRandomChoice(randomC);
	InterfacesManager::getInstance().setTimerUpdate(timeUpdate);
	InterfacesManager::getInstance().checkInterfaces(interface2exclude);
	//InterfacesManager::getInstance().printInterfaces();
	time(&lastInterfaceUpdate);

	// init the ClientManager
	cm.setByteStat(byteUpdate);
	cm.setDiscardFlag(ignore_nonDASH);

	//listening on port
	if (cm.startListeningForClient(listening_port)) {

		// forever: accept new clients
		while(true) {

			int new_client_socket = cm.acceptConnectionFromClient();

			// must re-check interfaces?
			time(&now);
			if (difftime(now, lastInterfaceUpdate) > INTERFACE_UPDATE_INTERVAL_SEC) {
				InterfacesManager::getInstance().checkInterfaces(interface2exclude);
				time(&lastInterfaceUpdate);
			}


			if (new_client_socket >= 0) {

				cm.forkAndManageClient();	// only the parent process exit from this call

				// close the new socket
				close(new_client_socket);
			}
			else {
				perror("accept error");
			}
		}

	}
	else {
		perror("Error on start listening");
	}

	// free interface mmap memory
	InterfacesManager::getInstance().freeMemory();
	StatManager::getInstance().freeMemory();

	cout << "End PROXY!!!" << endl;
	return EXIT_SUCCESS;
}
