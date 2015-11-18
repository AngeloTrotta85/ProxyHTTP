/*
 * StatManager.h
 *
 *  Created on: 26/mag/2015
 *      Author: angelo
 */

#ifndef STATMANAGER_H_
#define STATMANAGER_H_

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

class StatManager {
public:
	typedef struct stat_elements {
		char video_name[32];
		char small_video_name[16];
		int frag_seconds;
		int bps;
		int frag_number;
		int frag_bytesize;
		struct in_addr choosed_interface;
		time_t start_request_time;
		time_t end_request_time;
		struct timeval  start_request_timeval;
		struct timeval  end_request_timeval;
		bool reply_ok;
		bool isMS4;
		bool isCustom;
		char algo;
		int mode;
	} stat_elements_t;

private:
	StatManager() {
		memset(stat_file_name, 0, sizeof(stat_file_name));
		memset((char *)&actual_stats, 0, sizeof(stat_elements_t));

		sem_file_stat = sem_open("proxy_semaphore", O_CREAT|O_EXCL, 0644, 1);
		sem_unlink("proxy_semaphore");

		buffer_vec_size = 512 * sizeof(int);
		buffer_vec = (int *) mmap(NULL, buffer_vec_size * sizeof(int),	PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
		if (buffer_vec == MAP_FAILED) {
			perror("MAP_FAILED buffer_vec in StatManager");
			//exit(EXIT_FAILURE);
		}
		memset (buffer_vec, 0, buffer_vec_size * sizeof(int));

		t0 = (struct timeval *) mmap(NULL, sizeof(struct timeval),	PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
		if (t0 == MAP_FAILED) {
			perror("MAP_FAILED t0 in StatManager");
			//exit(EXIT_FAILURE);
		}
		memset (t0, 0, sizeof(struct timeval));
	};

public:
	static StatManager& getInstance() {
		static StatManager    instance; // Guaranteed to be destroyed.

		return instance;	// Instantiated on first use.
	}

	void setFileName(char *file_name);
	void makeStat();
	void fillFragmentField(const char *path);

	void freeMemory(void);

public:
	stat_elements_t actual_stats;

private:
	char stat_file_name[128];

	sem_t *sem_file_stat;

	int *buffer_vec;
	size_t buffer_vec_size;

	struct timeval *t0;
};

#endif /* STATMANAGER_H_ */
