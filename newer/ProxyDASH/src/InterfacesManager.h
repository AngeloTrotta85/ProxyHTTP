/*
 * InterfacesManager.h
 *
 *  Created on: 15/apr/2015
 *      Author: angelo
 */

#ifndef INTERFACESMANAGER_H_
#define INTERFACESMANAGER_H_

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

#include "Miscellaneous.h"
#include "VideoManager.h"

#define BLOCK_TOTAL_DIMENSION	20
#define BLOCK_SIZE				5
#define BLOCK_NUMBER			4

class InterfacesManager {
public:
	typedef struct mean_var {
		double mean;
		double variance;
		double standard_dev;
	} mean_var_t;

	typedef struct interface_stat {
		char if_name[16];
		struct in_addr addr_info;
		std::vector <mean_var_t> block_vector;
		int filled_block;
		double p_standardDev;
		double p_mean;
		double expected_thr;
	} interface_stat_t;

	struct stat_vector {
		unsigned int size;
		unsigned long int time;
		struct timeval timestamp;
	};
	struct interface_info {
		char name[16];
		in_addr_t addr_info;
		sem_t *statUpdate_sem;
		bool used;
		double expected_thr;
		struct stat_vector stats[BLOCK_TOTAL_DIMENSION];
	};
	VideoManager thread;

private:
	InterfacesManager() {
		interfaces_map = NULL;
		interfaces_map_vector_size = 0;
		timer_update = TIME_STAT_UPDATE;
		
		random_chioce = false;
		flag_update = true;

		sem_estimator = sem_open("proxy_estimator_semaphore", O_CREAT|O_EXCL, 0777, 1);
		sem_unlink("proxy_estimator_semaphore");
	};

	//InterfacesManager(InterfacesManager const&)	= delete;
	//void operator=(InterfacesManager const&)  	= delete;

public:
	static InterfacesManager& getInstance() {
		static InterfacesManager    instance; // Guaranteed to be destroyed.

		return instance;	// Instantiated on first use.
	}

	void printInterfaces(void);
	
	void setUpdateFlag(bool updateFlag);
	void setRandomChoice(bool r);
	void setTimerUpdate(int timer);

	void setUsed(in_addr_t addr_info);
	void setFree(in_addr_t addr_info);

	void checkInterfaces(std::list<std::string> &if2exclude);
	void freeMemory(void);

	void updateInterfaceStats (struct sockaddr_in *if_used, int pktSize, long long time_usec);

	void chooseIF(struct sockaddr_in &if_to_use, std::list<struct sockaddr_in> &if_to_update);

	void chooseIFMain(struct sockaddr_in &if_to_use_main, std::list<struct sockaddr_in> &if_use);

	void fullInterfaceList(struct sockaddr_in *if_to_use, std::list<struct sockaddr_in> &if_to_update );

	bool isAlreadyInTest(struct sockaddr_in *addr_in);

	double getExpectedThr(in_addr_t addr_info);

private:
	interface_info *interfaces_map;
	unsigned int interfaces_map_vector_size;

	sem_t *sem_estimator;
	
	bool flag_update;
	bool random_chioce;
	int timer_update;

};

#endif /* INTERFACESMANAGER_H_ */
